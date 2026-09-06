/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Бенчмарк гибридной модели MPI+OpenMP для OpenFOAM v2306
    Компилятор: icpx (Intel oneAPI) с -fiopenmp
-------------------------------------------------------------------------------
Описание
    Тест параллельности с тремя режимами:
    1. PURE_MPI   — чистый MPI, 1 поток на процесс (базовая линия)
    2. HYBRID     — MPI для коммуникации + OpenMP для вычислений
    3. PURE_OMP   — один MPI-процесс, только OpenMP (верхняя граница)

    Три фазы с таймингом:
    - Фаза 1: генерация данных (тяжёлые вычисления — sin/cos)
    - Фаза 2: MPI-распределение (mapDistribute)
    - Фаза 3: обработка полученных данных (тяжёлые вычисления)

Использование
    mpirun -np 4 ./test-parallel-hybrid -parallel -N 100000 -mode hybrid
    mpirun -np 1 ./test-parallel-hybrid -N 100000 -mode pure_omp
    mpirun -np 12 ./test-parallel-hybrid -parallel -N 100000 -mode pure_mpi
\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "List.H"
#include "mapDistribute.H"
#include "IPstream.H"
#include "OPstream.H"
#include "vector.H"
#include "IOstreams.H"
#include "Random.H"
#include "Tuple2.H"
#include "globalIndex.H"       // Для вычисления глобальных индексов

#include <omp.h>               // OpenMP: omp_get_thread_num, omp_get_wtime

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//
// Режимы работы бенчмарка
//
enum class BenchMode
{
    PURE_MPI,     // Чистый MPI: 1 MPI-процесс = 1 ядро, без OpenMP
    HYBRID,       // Гибрид: меньше MPI-процессов, OpenMP внутри каждого
    PURE_OMP      // Только OpenMP: 1 MPI-процесс, все ядра через потоки
};

//
// Преобразование строки в режим
//
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

//
// Строка-название режима для вывода
//
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

//
// Проверка, использовать ли OpenMP в вычислительных фазах
//
inline bool useOpenMP(BenchMode mode)
{
    return (mode == BenchMode::HYBRID || mode == BenchMode::PURE_OMP);
}

//
// Проверка, использовать ли MPI-коммуникацию
//
inline bool useMPI(BenchMode mode)
{
    return (mode == BenchMode::PURE_MPI || mode == BenchMode::HYBRID);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//                          ОСНОВНОЙ ТЕСТ                                   //
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void testHybrid(const label N, const BenchMode mode)
{
    // ------------------------------------------------------------------
    // ИНФОРМАЦИЯ О КОНФИГУРАЦИИ
    // ------------------------------------------------------------------
    const label nProcs    = Pstream::nProcs();
    const label myProc    = Pstream::myProcNo();
    const int  nThreads   = omp_get_max_threads();
    const bool useOMP     = useOpenMP(mode);
    const bool useComm    = useMPI(mode);

    if (Pstream::master())
    {
        Info<< "\n=========================================================\n"
            << "  Бенчмарк: " << modeName(mode) << "\n"
            << "  N = " << N << " элементов на процесс\n"
            << "  MPI-процессов: " << nProcs << "\n"
            << "  OpenMP-потоков: " << (useOMP ? nThreads : 1) << "\n"
            << "  MPI-коммуникация: " << (useComm ? "да" : "нет") << "\n"
            << "=========================================================\n"
            << endl;
    }

    // ------------------------------------------------------------------
    // ФАЗА 1: ГЕНЕРАЦИЯ ДАННЫХ (тяжёлые вычисления)
    // ------------------------------------------------------------------
    // Каждый элемент: Tuple2<label, List<scalar>>
    //   .first()  — номер процесса-получателя (0..nProcs-1)
    //   .second() — 3 скаляра, которые подвергаются "тяжёлой" обработке
    //
    // В реальном CFD-коде аналогом является:
    //   - вычисление градиентов в ячейках
    //   - обновление полей через уравнения состояния
    //   - турбулентные модели (тензорные вычисления)
    // ------------------------------------------------------------------

    List<Tuple2<label, List<scalar>>> complexData(N);

    double t0 = omp_get_wtime();

    if (useOMP)
    {
        // ---- РЕЖИМ OpenMP ----
        // Thread-local генератор случайных чисел:
        // Каждый поток получает свой экземпляр Random с уникальным seed.
        // Это решает проблему гонки данных — Random не потокобезопасен.
        //
        // Формула seed: базовый seed (зависит от procNo) + смещение потока
        //   — гарантирует разные последовательности между потоками
        //   — и воспроизводимость при повторном запуске с теми же параметрами

        #pragma omp parallel
        {
            // Локальный RNG для этого потока
            Random tlRndGen
            (
                43544 * myProc
              + 1000 * omp_get_thread_num()
            );

            // Распределение итераций по потокам:
            //   schedule(static) — каждый поток получает непрерывный чанк
            //   Это лучше для кэша (прямолинейный доступ к памяти) и для NUMA
            //   (данные, генерируемые потоком на сокете 0, остаются на сокете 0)
            #pragma omp for schedule(static)
            forAll(complexData, i)
            {
                // Номер процесса-получателя — случайный
                complexData[i].first() = tlRndGen.position
                (
                    0,
                    useComm ? (nProcs - 1) : 0
                );

                // Три скаляра — инициализация случайными значениями
                complexData[i].second().setSize(3);
                complexData[i].second()[0] = tlRndGen.GaussNormal();
                complexData[i].second()[1] = tlRndGen.GaussNormal();
                complexData[i].second()[2] = tlRndGen.GaussNormal();

                // Имитация тяжёлых вычислений (аналог градиента/дивергенции):
                // 100 итераций sin/cos — загрузка FPU, не упирается в память
                // Это создаёт вычислительную нагрузку ~500-1000 нс на элемент
                // что достаточно для эффективного OpenMP-распараллеливания
                auto& s = complexData[i].second();
                for (label j = 0; j < 100; j++)
                {
                    s[0] = Foam::sin(s[0] + 0.001 * j);
                    s[1] = Foam::cos(s[1] + 0.001 * j);
                    s[2] = Foam::sin(s[2] + 0.001 * j);
                }
            }
        }
    }
    else
    {
        // ---- ПОСЛЕДОВАТЕЛЬНЫЙ РЕЖИМ (PURE_MPI) ----
        // Один поток, один генератор — классический подход OpenFOAM
        Random rndGen(43544 * myProc);

        forAll(complexData, i)
        {
            complexData[i].first() = rndGen.position
            (
                0,
                useComm ? (nProcs - 1) : 0
            );

            complexData[i].second().setSize(3);
            complexData[i].second()[0] = rndGen.GaussNormal();
            complexData[i].second()[1] = rndGen.GaussNormal();
            complexData[i].second()[2] = rndGen.GaussNormal();

            // Та же "тяжёлая" обработка
            auto& s = complexData[i].second();
            for (label j = 0; j < 100; j++)
            {
                s[0] = Foam::sin(s[0] + 0.001 * j);
                s[1] = Foam::cos(s[1] + 0.001 * j);
                s[2] = Foam::sin(s[2] + 0.001 * j);
            }
        }
    }

    double t1 = omp_get_wtime();
    double tPhase1 = t1 - t0;

    // ------------------------------------------------------------------
    // ФАЗА 2: ПОДГОТОВКА К MPI-РАСПРЕДЕЛЕНИЮ
    // ------------------------------------------------------------------
    // Подсчёт количества элементов для каждого процесса-получателя
    // и сборка карты отправки (sendMap).
    //
    // Эта фаза — лёгкая (O(N)), но содержит зависимость по данным
    // (инкремент счётчика), поэтому распараллеливание требует reduction.
    // ------------------------------------------------------------------

    double t2a = omp_get_wtime();

    labelList nSend(nProcs, Zero);

    if (useOMP && N > 10000)
    {
        // OpenMP reduction на массиве — расширение OpenMP 4.5+
        // icpx (Intel) поддерживает. GCC 9+ тоже.
        // Каждый поток считает локальный массив nSendLocal,
        // в конце редукция суммированием.
        #pragma omp parallel for reduction(+:nSend[:nProcs])
        forAll(complexData, i)
        {
            nSend[complexData[i].first()]++;
        }
    }
    else
    {
        // Последовательный подсчёт
        forAll(complexData, i)
        {
            nSend[complexData[i].first()]++;
        }
    }

    // Сборка sendMap — всегда последовательно (зависимость по индексу)
    // Альтернатива: parallel prefix sum, но для N < 10M это не оправдано
    labelListList sendMap(nProcs);
    forAll(sendMap, proci)
    {
        sendMap[proci].resize_nocopy(nSend[proci]);
        nSend[proci] = 0;  // Переиспользуем как счётчик
    }
    forAll(complexData, i)
    {
        const label proci = complexData[i].first();
        sendMap[proci][nSend[proci]++] = i;
    }

    double t2b = omp_get_wtime();
    double tPhase2 = t2b - t2a;

    // ------------------------------------------------------------------
    // ФАЗА 3: MPI-РАСПРЕДЕЛЕНИЕ ДАННЫХ
    // ------------------------------------------------------------------
    // mapDistribute — обёртка над MPI_Alltoallv
    // Пересылает элементы complexData между процессами согласно sendMap
    //
    // В режиме PURE_OMP эта фаза пропускается — нет MPI-коммуникации
    // ------------------------------------------------------------------

    double tPhase3 = 0;
    label nReceived = N;  // Размер полученных данных

    if (useComm)
    {
        double t3a = omp_get_wtime();

        mapDistribute map(std::move(sendMap));
        map.distribute(complexData);

        double t3b = omp_get_wtime();
        tPhase3 = t3b - t3a;
        nReceived = complexData.size();
    }

    // ------------------------------------------------------------------
    // ФАЗА 4: ОБРАБОТКА ПОЛУЧЕННЫХ ДАННЫХ (тяжёлые вычисления)
    // ------------------------------------------------------------------
    // Аналог: вычисление дивергенции, обновление полей, турбулентность
    //
    // Каждый элемент обрабатывается независимо — идеальный кандидат
    // для #pragma omp parallel for с reduction
    // ------------------------------------------------------------------

    scalar localSum = 0;

    double t4a = omp_get_wtime();

    if (useOMP)
    {
        // reduction(+:localSum) — каждый поток накапливает локальную сумму,
        // в конце потоки складывают свои суммы в общую переменную.
        // Это避免了 ложное разделение кэша (false sharing):
        // потоки не пишут в одну ячейку памяти.
        #pragma omp parallel for reduction(+:localSum) schedule(static)
        forAll(complexData, i)
        {
            const auto& s = complexData[i].second();

            // Тяжёлая обработка: норма вектора + sin/cos
            scalar norm = Foam::sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);

            // Дополнительная нагрузка (аналог нескольких итераций PIMPLE)
            for (label j = 0; j < 100; j++)
            {
                norm = Foam::sin(norm + 0.001 * j)
                     + Foam::cos(norm + 0.001 * j);
            }

            localSum += norm;
        }
    }
    else
    {
        // Последовательная обработка
        forAll(complexData, i)
        {
            const auto& s = complexData[i].second();

            scalar norm = Foam::sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);

            for (label j = 0; j < 100; j++)
            {
                norm = Foam::sin(norm + 0.001 * j)
                     + Foam::cos(norm + 0.001 * j);
            }

            localSum += norm;
        }
    }

    double t4b = omp_get_wtime();
    double tPhase4 = t4b - t4a;

    // ------------------------------------------------------------------
    // ФАЗА 5: ГЛОБАЛЬНАЯ РЕДУКЦИЯ (MPI)
    // ------------------------------------------------------------------
    // Сумма localSum по всем процессам — MPI_Allreduce
    // В режиме PURE_OMP — нет MPI, просто используем localSum
    // ------------------------------------------------------------------

    double t5a = omp_get_wtime();

    scalar globalSum = localSum;

    if (useComm)
    {
        reduce(globalSum, sumOp<scalar>());
    }

    double t5b = omp_get_wtime();
    double tPhase5 = t5b - t5a;

    // ------------------------------------------------------------------
    // ВЫВОД РЕЗУЛЬТАТОВ
    // ------------------------------------------------------------------
    // Каждый процесс выводит своё время фаз.
    // Мастер дополнительно выводит глобальную сумму для проверки.
    // ------------------------------------------------------------------

    // Сбор времени фаз с всех процессов для анализа масштабируемости
    double tPhase1_max = tPhase1;
    double tPhase4_max = tPhase4;
    double tTotal = tPhase1 + tPhase2 + tPhase3 + tPhase4 + tPhase5;

    if (useComm)
    {
        reduce(tPhase1_max, maxOp<double>());
        reduce(tPhase4_max, maxOp<double>());
    }

    // Максимальное общее время (самый медленный процесс определяет wall time)
    double tTotal_max = tTotal;
    if (useComm)
    {
        reduce(tTotal_max, maxOp<double>());
    }

    if (Pstream::master())
    {
        Info<< "\n---------------------------------------------------------\n"
            << "  РЕЗУЛЬТАТЫ: " << modeName(mode) << "\n"
            << "---------------------------------------------------------\n"
            << "  N на процесс:           " << N << "\n"
            << "  Получено элементов:     " << nReceived << "\n"
            << "  MPI-процессов:          " << nProcs << "\n"
            << "  OpenMP-потоков:         " << (useOMP ? nThreads : 1) << "\n"
            << "  Глобальная сумма:       " << globalSum << "\n"
            << "---------------------------------------------------------\n"
            << "  Фаза 1 (генерация):     " << tPhase1 * 1e3 << " мс\n"
            << "  Фаза 2 (подготовка):    " << tPhase2 * 1e3 << " мс\n"
            << "  Фаза 3 (MPI distribute):" << tPhase3 * 1e3 << " мс\n"
            << "  Фаза 4 (обработка):     " << tPhase4 * 1e3 << " мс\n"
            << "  Фаза 5 (редукция):      " << tPhase5 * 1e3 << " мс\n"
            << "---------------------------------------------------------\n"
            << "  Общее время (мастер):   " << tTotal * 1e3 << " мс\n"
            << "  Max общее время:        " << tTotal_max * 1e3 << " мс\n"
            << "  Max фаза 1:             " << tPhase1_max * 1e3 << " мс\n"
            << "  Max фаза 4:             " << tPhase4_max * 1e3 << " мс\n"
            << "=========================================================\n"
            << endl;
    }
    else
    {
        // Слейвы выводят кратко
        Pout<< "  [P" << myProc << "] Ф1=" << tPhase1 * 1e3
            << " Ф3=" << tPhase3 * 1e3
            << " Ф4=" << tPhase4 * 1e3
            << " Total=" << tTotal * 1e3 << " мс" << endl;
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
    // -N     : размер массива (число элементов на процесс)
    // -mode  : режим работы (pure_mpi | hybrid | pure_omp)
    // -repeat: число повторов (для усреднения)
    // ------------------------------------------------------------------

    argList::addOption("N", "label", "Размер массива (по умолчанию 100000)");
    argList::addOption
    (
        "mode",
        "word",
        "Режим: pure_mpi | hybrid | pure_omp (по умолчанию hybrid)"
    );
    argList::addOption
    (
        "repeat",
        "label",
        "Число повторов (по умолчанию 3)"
    );

    argList::noCheckProcessorDirectories();
    #include "setRootCase.H"
    #include "createTime.H"

    // Чтение параметров
    label N = 100000;
    args.readIfPresent("N", N);

    word modeStr = "hybrid";
    args.readIfPresent("mode", modeStr);
    BenchMode mode = parseMode(modeStr);

    label repeat = 3;
    args.readIfPresent("repeat", repeat);

    // Вывод информации о сборке
    if (Pstream::master())
    {
        Info<< "\n=========================================================\n"
            << "  test-parallel-hybrid — бенчмарк MPI+OpenMP\n"
            << "  OpenFOAM " << OPENFOAM << " | build " << WM_COMPILE_OPTION
            << "\n"
            << "  OpenMP max threads: " << omp_get_max_threads() << "\n"
            << "========================================================="
            << endl;
    }

    // ------------------------------------------------------------------
    // ЗАПУСК БЕНЧМАРКА
    // ------------------------------------------------------------------
    // Прогрев (warmup) — первый прогон не учитывается:
    //   - создание пула потоков OpenMP (~50-100 мкс)
    //   - прогрев кэшей
    //   - инициализация MPI-буферов
    // ------------------------------------------------------------------

    if (Pstream::master())
    {
        Info<< "\n--- Прогрев (warmup) ---" << endl;
    }
    testHybrid(N, mode);

    // ------------------------------------------------------------------
    // ОСНОВНЫЕ ПРОГОНЫ
    // ------------------------------------------------------------------
    // Усреднение по repeat прогонам — уменьшает влияние шумов ОС
    // ------------------------------------------------------------------

    if (Pstream::master())
    {
        Info<< "\n--- Основные прогоны (" << repeat << " повторов) ---"
            << endl;
    }

    for (label r = 0; r < repeat; r++)
    {
        if (Pstream::master())
        {
            Info<< "\n>>> Прогон " << (r + 1) << "/" << repeat << endl;
        }
        testHybrid(N, mode);
    }

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
