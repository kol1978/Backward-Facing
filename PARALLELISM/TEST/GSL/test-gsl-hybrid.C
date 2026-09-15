/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Бенчмарк GSL + OpenFOAM: гибридная модель MPI+OpenMP
    OpenFOAM v2312 | icpx (Intel oneAPI) | SSE4.2 (Westmere)
-------------------------------------------------------------------------------
Описание
    Сравнение адаптивной квадратуры GSL (gsl_integration_qags) и
    векторизованной фиксированной квадратуры Гаусса-Лежандра (SSE4.2)
    в трёх режимах распараллеливания:

    1. PURE_MPI  - чистый MPI, 1 поток на процесс
    2. HYBRID    - MPI + OpenMP (GSL workspace на каждый поток)
    3. PURE_OMP  - один MPI-процесс, только OpenMP

    Параметры читаются из system/controlDict (секция "benchmark").
    Командная строка переопределяет значения из controlDict.

    Пять фаз с таймингом:
    - Фаза 1: генерация подынтегральных функций (имитация полей OpenFOAM)
    - Фаза 2: адаптивное интегрирование GSL (workspace на поток)
    - Фаза 3: векторизованная квадратура SSE4.2 (Gauss-Legendre, 8 узлов)
    - Фаза 4: решение СЛАУ (GSL, распределённое по процессам)
    - Фаза 5: глобальная редукция и статистика

Использование:

LOGFILE="log_pure_mpi_$(date +%Y%m%d_%H%M%S).log"
    mpirun -np 4 ./test-gsl-hybrid -parallel -mode hybrid | tee "$LOGFILE"
    mpirun -np 1 ./test-gsl-hybrid -mode pure_omp | tee "$LOGFILE"
    mpirun -np 12 ./test-gsl-hybrid -parallel -mode pure_mpi | tee "$LOGFILE"

    Переопределение из командной строки:
    mpirun -np 1 ./test-gsl-hybrid -N 5000000 -mode pure_omp

Компиляция (в каталоге с Make/files):
    test-gsl-hybrid.C

    EXE = $(FOAM_USER_APPBIN)/test-gsl-hybrid

    EXE_INC = \
        -I$(LIB_SRC)/finiteVolume/lnInclude \
        -I$(LIB_SRC)/meshTools/lnInclude \
        -I/usr/include \
        -march=westmere

    EXE_LIBS = \
        -lfiniteVolume \
        -lgsl -lgslcblas -lm
\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "List.H"
#include "vector.H"
#include "IOstreams.H"
#include "Random.H"
#include "globalIndex.H"
#include "dictionary.H"

#include <omp.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_statistics.h>

#include <xmmintrin.h>    /* SSE   */
#include <pmmintrin.h>    /* SSE3  (_mm_hadd_pd) */

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                          РЕЖИМЫ РАБОТЫ                                    //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

enum class BenchMode
{
    PURE_MPI,    // Чистый MPI: 1 MPI-процесс = 1 ядро, без OpenMP
    HYBRID,      // Гибрид: меньше MPI-процессов, OpenMP внутри каждого
    PURE_OMP     // Только OpenMP: 1 MPI-процесс, все ядра через потоки
};

BenchMode parseMode(const word& modeStr)
{
    if (modeStr == "pure_mpi" || modeStr == "mpi")
        return BenchMode::PURE_MPI;
    if (modeStr == "hybrid" || modeStr == "mpi_omp")
        return BenchMode::HYBRID;
    if (modeStr == "pure_omp" || modeStr == "omp")
        return BenchMode::PURE_OMP;

    Info<< "Неизвестный режим: " << modeStr
        << ", используется hybrid по умолчанию" << endl;
    return BenchMode::HYBRID;
}

word modeName(BenchMode mode)
{
    switch (mode)
    {
        case BenchMode::PURE_MPI:  return "PURE_MPI";
        case BenchMode::HYBRID:    return "HYBRID (MPI+OpenMP)";
        case BenchMode::PURE_OMP:  return "PURE_OPENMP";
    }
    return "UNKNOWN";
}

inline bool useOpenMP(BenchMode mode)
{
    return (mode == BenchMode::HYBRID || mode == BenchMode::PURE_OMP);
}

inline bool useMPI(BenchMode mode)
{
    return (mode == BenchMode::PURE_MPI || mode == BenchMode::HYBRID);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                      КОНФИГУРАЦИЯ БЕНЧМАРКА                               //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

/* Все настраиваемые параметры собраны в одной структуре.
 * Чтение происходит из controlDict (секция "benchmark"),
 * командная строка имеет приоритет над controlDict. */

struct BenchConfig
{
    label N = 1000000;           // Число интегралов на процесс
    word modeStr = "hybrid";     // Режим (строка для парсинга)
    BenchMode mode = BenchMode::HYBRID;  // Распарсенный режим
    label repeat = 3;            // Число повторов

    double gslTolerance = 1e-7;   // Точность GSL QAGS
    label gslWorkspace = 1000;   // Размер workspace GSL

    double aMin = 0.0;            // Нижняя граница для a
    double aMax = 0.5;            // Верхняя граница для a
    double bMin = 0.5;            // Нижняя граница для b
    double bMax = 1.5;            // Верхняя граница для b
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//          SSE4.2: КВАДРАТУРА ГАУССА-ЛЕЖАНДРА (8 УЗЛОВ)                     //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

static const double gl_nodes[8] =
{
   -0.9602898564975363,
   -0.7966664774136267,
   -0.5255324099163290,
   -0.1834346424956498,
    0.1834346424956498,
    0.5255324099163290,
    0.7966664774136267,
    0.9602898564975363
};

static const double gl_weights[8] =
{
    0.1012285362903763,
    0.2223810344533745,
    0.3137066458778873,
    0.3626837833783620,
    0.3626837833783620,
    0.3137066458778873,
    0.2223810344533745,
    0.1012285362903763
};

__attribute__((unused))
static inline double f_scalar(double x)
{
    return x * x;
}

double integrate_sse4(double a, double b)
{
    __m128d v_half_len = _mm_set1_pd(0.5 * (b - a));
    __m128d v_mid      = _mm_set1_pd(0.5 * (a + b));
    __m128d v_acc = _mm_setzero_pd();

    for (int i = 0; i < 8; i += 2)
    {
        __m128d v_t = _mm_loadu_pd(&gl_nodes[i]);
        __m128d v_w = _mm_loadu_pd(&gl_weights[i]);

        __m128d v_x = _mm_add_pd(_mm_mul_pd(v_half_len, v_t), v_mid);
        __m128d v_fx = _mm_mul_pd(v_x, v_x);
        __m128d v_wf = _mm_mul_pd(v_w, v_fx);

        v_acc = _mm_add_pd(v_acc, v_wf);
    }

    __m128d v_sum = _mm_hadd_pd(v_acc, v_acc);
    double scalar_sum = _mm_cvtsd_f64(v_sum);

    return (0.5 * (b - a)) * scalar_sum;
}

__attribute__((unused))
double integrate_scalar(double (*func)(double, void*), void *params,
                         double a, double b)
{
    const double half_len = 0.5 * (b - a);
    const double midpoint = 0.5 * (a + b);

    double sum = 0.0;
    for (int i = 0; i < 8; i++)
    {
        double x = half_len * gl_nodes[i] + midpoint;
        sum += gl_weights[i] * func(x, params);
    }
    return half_len * sum;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                    СТРУКТУРА ДАННЫХ ИНТЕГРАЛА                            //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

struct IntegralTask
{
    double a, b;
    double gslResult;
    double gslError;
    double sseResult;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                          ОСНОВНОЙ ТЕСТ                                    //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void testGslHybrid(const BenchConfig& cfg)
{
    const label N      = cfg.N;
    const BenchMode mode = cfg.mode;

    // ------------------------------------------------------------------
    // ИНФОРМАЦИЯ О КОНФИГУРАЦИИ
    // ------------------------------------------------------------------
    const label nProcs  = Pstream::nProcs();
    const label myProc  = Pstream::myProcNo();
    const int  nThreads = omp_get_max_threads();
    const bool useOMP   = useOpenMP(mode);
    const bool useComm  = useMPI(mode);

    if (Pstream::master())
    {
        Info<< "\n=========================================================\n"
            << "  Бенчмарк GSL + SSE4.2: " << modeName(mode) << "\n"
            << "  N = " << N << " интегралов на процесс\n"
            << "  MPI-процессов: " << nProcs << "\n"
            << "  OpenMP-потоков: " << (useOMP ? nThreads : 1) << "\n"
            << "  MPI-коммуникация: " << (useComm ? "да" : "нет") << "\n"
            << "  GSL tolerance: " << cfg.gslTolerance << "\n"
            << "  GSL workspace: " << cfg.gslWorkspace << "\n"
            << "  Интервалы: a=[" << cfg.aMin << ", " << cfg.aMax << "]"
            << " b=[" << cfg.bMin << ", " << cfg.bMax << "]\n"
            << "=========================================================\n"
            << endl;
    }

    // ------------------------------------------------------------------
    // ФАЗА 1: ГЕНЕРАЦИЯ ДАННЫХ (пределы интегрирования)
    // ------------------------------------------------------------------

    List<IntegralTask> tasks(N);

    double t0 = omp_get_wtime();

    if (useOMP)
    {
        #pragma omp parallel
        {
            Random tlRndGen(43544 * myProc + 1000 * omp_get_thread_num());

            #pragma omp for schedule(static)
            forAll(tasks, i)
            {
                tasks[i].a = tlRndGen.position(cfg.aMin, cfg.aMax);
                tasks[i].b = tlRndGen.position(cfg.bMin, cfg.bMax);

                tasks[i].gslResult = 0.0;
                tasks[i].gslError = 0.0;
                tasks[i].sseResult = 0.0;
            }
        }
    }
    else
    {
        Random rndGen(43544 * myProc);

        forAll(tasks, i)
        {
            tasks[i].a = rndGen.position(cfg.aMin, cfg.aMax);
            tasks[i].b = rndGen.position(cfg.bMin, cfg.bMax);

            tasks[i].gslResult = 0.0;
            tasks[i].gslError = 0.0;
            tasks[i].sseResult = 0.0;
        }
    }

    double t1 = omp_get_wtime();
    double tPhase1 = t1 - t0;

    // ------------------------------------------------------------------
    // ФАЗА 2: АДАПТИВНОЕ ИНТЕГРИРОВАНИЕ GSL
    // ------------------------------------------------------------------

    double t2 = omp_get_wtime();

    gsl_function F;
    F.function = [](double x, void *params) -> double
    {
        (void)params;
        return x * x;
    };
    F.params = nullptr;

    double gslSum = 0.0;

    if (useOMP)
    {
        gsl_integration_workspace **workspaces =
            (gsl_integration_workspace **)
            malloc(nThreads * sizeof(gsl_integration_workspace *));

        for (int t = 0; t < nThreads; t++)
        {
            workspaces[t] = gsl_integration_workspace_alloc(cfg.gslWorkspace);
        }

        #pragma omp parallel for reduction(+:gslSum) schedule(static)
        forAll(tasks, i)
        {
            int tid = omp_get_thread_num();

            gsl_integration_qags
            (
                &F,
                tasks[i].a, tasks[i].b,
                0, cfg.gslTolerance, cfg.gslWorkspace,
                workspaces[tid],
                &tasks[i].gslResult,
                &tasks[i].gslError
            );

            gslSum += tasks[i].gslResult;
        }

        for (int t = 0; t < nThreads; t++)
        {
            gsl_integration_workspace_free(workspaces[t]);
        }
        free(workspaces);
    }
    else
    {
        gsl_integration_workspace *w =
            gsl_integration_workspace_alloc(cfg.gslWorkspace);

        forAll(tasks, i)
        {
            gsl_integration_qags
            (
                &F,
                tasks[i].a, tasks[i].b,
                0, cfg.gslTolerance, cfg.gslWorkspace,
                w,
                &tasks[i].gslResult,
                &tasks[i].gslError
            );

            gslSum += tasks[i].gslResult;
        }

        gsl_integration_workspace_free(w);
    }

    double t3 = omp_get_wtime();
    double tPhase2 = t3 - t2;

    // ------------------------------------------------------------------
    // ФАЗА 3: ВЕКТОРИЗОВАННАЯ КВАДРАТУРА SSE4.2
    // ------------------------------------------------------------------

    double t4 = omp_get_wtime();

    double sseSum = 0.0;

    if (useOMP)
    {
        #pragma omp parallel for reduction(+:sseSum) schedule(static)
        forAll(tasks, i)
        {
            tasks[i].sseResult = integrate_sse4(tasks[i].a, tasks[i].b);
            sseSum += tasks[i].sseResult;
        }
    }
    else
    {
        forAll(tasks, i)
        {
            tasks[i].sseResult = integrate_sse4(tasks[i].a, tasks[i].b);
            sseSum += tasks[i].sseResult;
        }
    }

    double t5 = omp_get_wtime();
    double tPhase3 = t5 - t4;

    // ------------------------------------------------------------------
    // ФАЗА 4: РЕШЕНИЕ СЛАУ (GSL, распределённое по процессам)
    // ------------------------------------------------------------------

    double t6 = omp_get_wtime();

    struct System2x2
    {
        double m00, m01, m10, m11;
        double b0, b1;
    };

    System2x2 systems[] =
    {
        {2.0, 1.0, 1.0, 3.0, 1.0, 2.0},
        {1.0, 4.0, 2.0, 1.0, 3.0, 1.0},
        {3.0, 2.0, 1.0, 5.0, 2.0, 4.0},
        {4.0, 1.0, 1.0, 2.0, 1.0, 3.0},
    };
    int nSystems = sizeof(systems) / sizeof(systems[0]);

    double linalgSum = 0.0;

    for (int s = myProc; s < nSystems; s += nProcs)
    {
        gsl_matrix *m = gsl_matrix_alloc(2, 2);
        gsl_vector *b = gsl_vector_alloc(2);
        gsl_vector *x = gsl_vector_alloc(2);

        gsl_matrix_set(m, 0, 0, systems[s].m00);
        gsl_matrix_set(m, 0, 1, systems[s].m01);
        gsl_matrix_set(m, 1, 0, systems[s].m10);
        gsl_matrix_set(m, 1, 1, systems[s].m11);
        gsl_vector_set(b, 0, systems[s].b0);
        gsl_vector_set(b, 1, systems[s].b1);

        gsl_linalg_HH_solve(m, b, x);

        linalgSum += gsl_vector_get(x, 0) + gsl_vector_get(x, 1);

        gsl_matrix_free(m);
        gsl_vector_free(b);
        gsl_vector_free(x);
    }

    double t7 = omp_get_wtime();
    double tPhase4 = t7 - t6;

    // ------------------------------------------------------------------
    // ФАЗА 5: ГЛОБАЛЬНАЯ РЕДУКЦИЯ И СТАТИСТИКА
    // ------------------------------------------------------------------

    double t8 = omp_get_wtime();

    double globalGslSum = gslSum;
    double globalSseSum = sseSum;
    double globalLinalgSum = linalgSum;

    if (useComm)
    {
        reduce(globalGslSum, sumOp<double>());
        reduce(globalSseSum, sumOp<double>());
        reduce(globalLinalgSum, sumOp<double>());
    }

    List<double> diffs(N);
    forAll(tasks, i)
    {
        diffs[i] = tasks[i].gslResult - tasks[i].sseResult;
    }

    double meanDiff = gsl_stats_mean(diffs.data(), 1, N);
    double maxDiff = 0.0;
    forAll(diffs, i)
    {
        double ad = fabs(diffs[i]);
        if (ad > maxDiff) maxDiff = ad;
    }

    if (useComm)
    {
        reduce(maxDiff, maxOp<double>());
    }

    double t9 = omp_get_wtime();
    double tPhase5 = t9 - t8;

    // ------------------------------------------------------------------
    // ВЫВОД РЕЗУЛЬТАТОВ
    // ------------------------------------------------------------------

    double tTotal = tPhase1 + tPhase2 + tPhase3 + tPhase4 + tPhase5;
    double tTotal_max = tTotal;
    double tPhase2_max = tPhase2;
    double tPhase3_max = tPhase3;

    if (useComm)
    {
        reduce(tTotal_max, maxOp<double>());
        reduce(tPhase2_max, maxOp<double>());
        reduce(tPhase3_max, maxOp<double>());
    }

    if (Pstream::master())
    {
        Info<< "\n---------------------------------------------------------\n"
            << "  РЕЗУЛЬТАТЫ: " << modeName(mode) << "\n"
            << "---------------------------------------------------------\n"
            << "  N на процесс:           " << N << "\n"
            << "  MPI-процессов:          " << nProcs << "\n"
            << "  OpenMP-потоков:         " << (useOMP ? nThreads : 1) << "\n"
            << "  GSL tolerance:          " << cfg.gslTolerance << "\n"
            << "  GSL workspace:          " << cfg.gslWorkspace << "\n"
            << "---------------------------------------------------------\n"
            << "  Сумма интегралов GSL:   " << globalGslSum << "\n"
            << "  Сумма интегралов SSE4.2: " << globalSseSum << "\n"
            << "  Разница GSL-SSE:        " << (globalGslSum - globalSseSum)
            << "\n"
            << "  Max |различие|:         " << maxDiff << "\n"
            << "  Mean |различие|:         " << fabs(meanDiff) << "\n"
            << "  Сумма СЛАУ (x0+x1):     " << globalLinalgSum << "\n"
            << "---------------------------------------------------------\n"
            << "  Фаза 1 (генерация):      " << tPhase1 * 1e3 << " мс\n"
            << "  Фаза 2 (GSL QAGS):       " << tPhase2 * 1e3 << " мс\n"
            << "  Фаза 3 (SSE4.2 GL):      " << tPhase3 * 1e3 << " мс\n"
            << "  Фаза 4 (СЛАУ):           " << tPhase4 * 1e3 << " мс\n"
            << "  Фаза 5 (редукция):       " << tPhase5 * 1e3 << " мс\n"
            << "---------------------------------------------------------\n"
            << "  Общее время (мастер):    " << tTotal * 1e3 << " мс\n"
            << "  Max общее время:         " << tTotal_max * 1e3 << " мс\n"
            << "  Max фаза 2 (GSL):        " << tPhase2_max * 1e3 << " мс\n"
            << "  Max фаза 3 (SSE):        " << tPhase3_max * 1e3 << " мс\n"
            << "---------------------------------------------------------\n"
            << "  Ускорение SSE/GSL:       ";

        if (tPhase3_max > 0)
        {
            Info<< tPhase2_max / tPhase3_max << "x";
        }
        else
        {
            Info<< "N/A";
        }

        Info<< "\n=========================================================\n"
            << endl;
    }
    else
    {
        Pout<< "  [P" << myProc << "] "
            << "Ф1=" << tPhase1 * 1e3
            << " Ф2(GSL)=" << tPhase2 * 1e3
            << " Ф3(SSE)=" << tPhase3 * 1e3
            << " Ф4=" << tPhase4 * 1e3
            << " Total=" << tTotal * 1e3 << " мс"
            << endl;
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                              MAIN                                        //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    // ------------------------------------------------------------------
    // АРГУМЕНТЫ КОМАНДНОЙ СТРОКИ
    // ------------------------------------------------------------------
    argList::addOption("N", "label", "Число интегралов (переопределяет controlDict)");
    argList::addOption
    (
        "mode",
        "word",
        "Режим: pure_mpi | hybrid | pure_omp (переопределяет controlDict)"
    );
    argList::addOption
    (
        "repeat",
        "label",
        "Число повторов (переопределяет controlDict)"
    );

    argList::noCheckProcessorDirectories();
    #include "setRootCase.H"
    #include "createTime.H"

    // ------------------------------------------------------------------
    // ЧТЕНИЕ ПАРАМЕТРОВ ИЗ controlDict
    // ------------------------------------------------------------------
    // Параметры читаются из секции "benchmark" в system/controlDict.
    // Значения по умолчанию заданы в структуре BenchConfig.
    // Командная строка имеет приоритет над controlDict.
    // ------------------------------------------------------------------

    BenchConfig cfg;

    const dictionary& ctrlDict = runTime.controlDict();

    if (ctrlDict.found("benchmark"))
    {
        const dictionary& benchDict = ctrlDict.subDict("benchmark");

        benchDict.readIfPresent("N", cfg.N);
        benchDict.readIfPresent("mode", cfg.modeStr);
        benchDict.readIfPresent("repeat", cfg.repeat);
        benchDict.readIfPresent("gslTolerance", cfg.gslTolerance);
        benchDict.readIfPresent("gslWorkspace", cfg.gslWorkspace);
        benchDict.readIfPresent("aMin", cfg.aMin);
        benchDict.readIfPresent("aMax", cfg.aMax);
        benchDict.readIfPresent("bMin", cfg.bMin);
        benchDict.readIfPresent("bMax", cfg.bMax);

        if (Pstream::master())
        {
            Info<< "\n  Параметры из controlDict [benchmark]:\n"
                << "    N = " << cfg.N << "\n"
                << "    mode = " << cfg.modeStr << "\n"
                << "    repeat = " << cfg.repeat << "\n"
                << "    gslTolerance = " << cfg.gslTolerance << "\n"
                << "    gslWorkspace = " << cfg.gslWorkspace << "\n"
                << "    aMin/aMax = " << cfg.aMin << " / " << cfg.aMax << "\n"
                << "    bMin/bMax = " << cfg.bMin << " / " << cfg.bMax << "\n"
                << endl;
        }
    }
    else
    {
        if (Pstream::master())
        {
            Info<< "\n  Секция 'benchmark' не найдена в controlDict.\n"
                << "  Используются значения по умолчанию.\n"
                << endl;
        }
    }

    // ------------------------------------------------------------------
    // ПЕРЕОПРЕДЕЛЕНИЕ ИЗ КОМАНДНОЙ СТРОКИ
    // ------------------------------------------------------------------
    // Командная строка имеет приоритет над controlDict.
    // ------------------------------------------------------------------

    bool overridden = false;

    if (args.readIfPresent("N", cfg.N))
    {
        overridden = true;
    }
    if (args.readIfPresent("mode", cfg.modeStr))
    {
        overridden = true;
    }
    if (args.readIfPresent("repeat", cfg.repeat))
    {
        overridden = true;
    }

    if (overridden && Pstream::master())
    {
        Info<< "  Переопределено из командной строки:\n"
            << "    N = " << cfg.N << "\n"
            << "    mode = " << cfg.modeStr << "\n"
            << "    repeat = " << cfg.repeat << "\n"
            << endl;
    }

    cfg.mode = parseMode(cfg.modeStr);

    // ------------------------------------------------------------------
    // ИНИЦИАЛИЗАЦИЯ GSL
    // ------------------------------------------------------------------
    gsl_set_error_handler_off();

    // ------------------------------------------------------------------
    // ВЫВОД ИНФОРМАЦИИ О СБОРКЕ
    // ------------------------------------------------------------------
    if (Pstream::master())
    {
        Info<< "\n=========================================================\n"
            << "  test-gsl-hybrid — бенчмарк GSL + SSE4.2 + OpenFOAM\n"
            << "  OpenFOAM " << OPENFOAM << " | icpx build\n"
            << "  OpenMP max threads: " << omp_get_max_threads() << "\n"
            << "  GSL error handler: OFF (thread-safe)\n"
            << "========================================================="
            << endl;
    }

    // ------------------------------------------------------------------
    // ПРОГРЕВ (warmup)
    // ------------------------------------------------------------------
    if (Pstream::master())
    {
        Info<< "\n--- Прогрев (warmup) ---" << endl;
    }
    testGslHybrid(cfg);

    // ------------------------------------------------------------------
    // ОСНОВНЫЕ ПРОГОНЫ
    // ------------------------------------------------------------------
    if (Pstream::master())
    {
        Info<< "\n--- Основные прогоны (" << cfg.repeat << " повторов) ---"
            << endl;
    }

    for (label r = 0; r < cfg.repeat; r++)
    {
        if (Pstream::master())
        {
            Info<< "\n>>> Прогон " << (r + 1) << "/" << cfg.repeat << endl;
        }
        testGslHybrid(cfg);
    }

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
