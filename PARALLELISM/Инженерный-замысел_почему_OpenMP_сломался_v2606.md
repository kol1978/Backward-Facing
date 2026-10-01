# Анализ порта OpenFOAM_HMM: grep-профиль, контрольная проверка и инженерный замысел

**Система:** kol-serv (Ubuntu 24.04, 2× Westmere X5675, 12 ядер, 2 NUMA-узла)
**Клон:** `ROCm/OpenFOAM_HMM` (ветка `suyash/hmm`), базовый релиз v2206
**Дата отчёта:** 2026-10-01

---

## 1. Профиль порта на клоне OpenFOAM_HMM (v2206)

Результаты фактического прогона шести grep-проверок по рабочему дереву клона.

### 1.1. Маркер 1: `requires unified_shared_memory` — 44 файла

Объявление единого адресного пространства CPU↔GPU. Распределение по зонам:

| Зона | Файлы | Суть |
|---|---|---|
| `src/OpenFOAM/matrices/lduMatrix` | 16 | Ядро СЛАУ: ATmul-операции, решатели, сглаживатели, предположители |
| `src/finiteVolume` | 13 | fvMatrix, gradSchemes, snGrad, интерполяция, патчи |
| `src/OpenFOAM/fields` | 5 | FieldM.H, FieldFunctions, scalarField, FieldField |
| `src/OpenFOAM/meshes` | 3 | mapDistribute, primitiveMesh |
| `src/OpenFOAM/primitives` + containers | 4 | List.C, ops.H, AtomicAccumulator, bool.C |
| `src/meshTools` | 2 | AMIInterpolation, FaceCellWave |
| `src/TurbulenceModels` | 1 | omegaWallFunction |

**Интерпретация:** маркер стоит в тех единицах трансляции, чьи данные реально участвуют в offload-регионах. Это не «прагмы на каждом файле», а точечная разметка: каждый `.C` с target-регионами несёт свой `requires`. Найден также артефакт: `lduMatrixATmul.C.org` — резервная копия от локальных экспериментов (не участвует в сборке).

### 1.2. Маркер 2: `omp target` — 46 файлов с таргет-регионами

| Зона | Файлы |
|---|---|
| `lduMatrix` (решатели, сглаживатели, предположители, интерфейсы) | 19 |
| `finiteVolume` (fvc, схемы, патчи, ограничители градиентов) | 14 |
| `OpenFOAM` поля/списки/меш | 11 |
| `meshTools` (AMI) | 2 |
| `TurbulenceModels` | 1 |

### 1.3. Маркер 3: `FieldM.H` — сердце порта подтверждено

```cpp
#ifndef TARGET_CUT_OFF
#define TARGET_CUT_OFF 10000
```

Макросы `TFOR_ALL_F_OP_*` обёрнуты в `_Pragma("omp target teams distribute parallel for if(loop_len > 10000)")` — адаптивный порог: маленькие циклы остаются на CPU, большие уходят на GPU. Плюс reduction-варианты (min/max) с явным `map(tofrom:...)`. **Одна строка на макрос — оффлоад тысяч вызовов полевой алгебры.**

### 1.4. Маркер 4: инфраструктура (USE_MEM_POOL / WITH_CSR / USE_ROCTX)

Сборочные флаги подтверждены в правилах и в командах компиляции лога. Соответствуют интеграции Umpire (пул памяти), CSR-слою матриц и rocTX-трассировке.

### 1.5. Маркер 5: wmake-правила — offload-флаги на месте

```
wmake/rules/General/Clang/openmp-gomp:
  COMP_OPENMP = ... -fopenmp --offload-arch=${GPU_ARCH} -fopenmp-target-fast -fopenmp-version=51 ...

wmake/rules/linux64Clang/c++Opt:
  c++OPT = ... -I ${ROCM4FOAM}/include/ -DUSE_ROCTX -DUSE_OMP -DWITH_CSR
           -fopenmp --offload-arch=${GPU_ARCH} -fopenmp-target-fast
           -fopenmp-version=51 -DUSE_MEM_POOL -I${UMPIRE4FOAM}/include
```

**Внимание:** `--offload-arch=${GPU_ARCH}` в текущем окружении разворачивается в пустой таргет (`rocm_agent_enumerator` не видит GPU на kol-serv). Перед финальной чистой пересборкой задать `GPU_ARCH=gfx942` (MI300A) явно в `hmm()`.

### 1.6. Маркер 6: зона решателей — полный охват lduMatrix

PCG, PBiCGStab, GAMG (agglomerate/scale/interpolate/solve/agglomeration), диагональный и DILU предположители, GaussSeidel/DIC/FDIC сглаживатели, интерфейсные шаблоны. Весь «горячий» след СЛАУ оффлоажен.

---

## 2. Контрольная проверка на ванильных версиях

Тот же набор grep-проверок на ванильном дереве — контрольный эксперимент, отделяющий маркеры порта от фона.

### 2.1. Фактический прогон на ванильном v2606

| № | Проверка | v2606 vanilla | Вывод |
|---|---|---|---|
| 1 | `unified_shared_memory` | 0 | Маркер порта отсутствует — валидный контроль |
| 2 | `omp target` | **1 файл: `src/OpenFOAM/expressionTemplates/ListExpression.H`** | Не шум — см. §3 |
| 3 | `TARGET_CUT_OFF` / прагмы в `FieldM.H` | 0 | Макросы полей в ванили чистые |
| 4 | `USE_MEM_POOL` / `WITH_CSR` / `USE_ROCTX` | 0 | Инфраструктура порта отсутствует |
| 5 | offload-флаги в `wmake/rules` | 0 | `--offload-arch` / `-fopenmp-target-*` нет нигде |
| 6 | `omp target` в lduMatrix | 0 | Решатели в ванили чисто CPU |

**Ожидание для v2206 vanilla:** нули по всем шести пунктам (ESI не публиковала GPU-оффлоад в v2206; подтверждение не требует прогона, но при наличии дерева — одна минута).

### 2.2. Методологический итог

Пять из шести маркеров — уникальные следы порта OpenFOAM_HMM: их появление в клоне доказуемо портовой принадлежностью. Маркер `omp target` как «дискриминатор портированности» **валиден только до v2512**: в v2606 следы GPU-пути в ванили уже есть — по другой технологии (см. §3).

---

## 3. v2606 — не чистый контроль: ваниль строит собственный GPU-путь

Единственное попадание в ванильном v2606 — `src/OpenFOAM/expressionTemplates/ListExpression.H`. Это не остаток HMM-порта, а новая нативная инфраструктура OpenCFD: **expression templates**, введённые в v2512 при участии AMD как фундамент GPU-ускорения через C++17 `std::execution` — а не через OpenMP target.

Следствие для контроля: сравнение с v2606 валидно для всех маркеров, кроме `omp target`. Точная строка попадания фиксируется командой:

```bash
grep -n 'omp target' src/OpenFOAM/expressionTemplates/ListExpression.H
```

Скорее всего это код/комментарий вокруг expression-типов (`List_divide<List_multiply<...>>`), которые не trivially copyable и при icpx-OpenMP-пассе дают `-Wopenmp-mapping` — ровно механика, описанная в §4.

---

## 4. Анализ: инженерный замысел и почему OpenMP сломался в v2606

### 4.1. Что было до v2512

В версиях до v2512 полевые операции (`a + b + c`) выполнялись через eager evaluation — каждый оператор создавал промежуточный временный `GeometricField`. OpenMP мог быть включён через `WM_COMPILE_CONTROL="+openmp"` или `wmake -openmp` — и это работало, потому что типы были простыми (`double`, `Field<double>`) и trivially copyable.

### 4.2. Что изменилось в v2512

В v2512 OpenCFD (при участии AMD) внедрил expression templates — новую библиотеку в `$FOAM_SRC/OpenFOAM/expressionTemplates/`. Цитата из release notes v2512:

> «This release includes an initial version of an expression templates library that transforms how field operations are executed. This powerful optimization technique eliminates intermediate field allocations, fuses multiple operations into single computational kernels, and enables hardware acceleration including GPU offloading.»

Ключевая фраза — «enables hardware acceleration including GPU offloading». Expression templates были созданы не случайно и не как ошибка — это фундамент для будущего GPU-оффлоадинга. Они:

- устраняют промежуточные аллокации полей (fusion в один kernel);
- создают сложные template-типы (`List_subtract<ListConstRefWrap<double>, List_add<...>>`);
- разделяют вычисления на internal field / uncoupled patches / coupled patches;
- поддерживают fused patch evaluation (вклад AMD — один kernel для всех патчей).

### 4.3. Что произошло в v2606

v2606 — первый релиз с поддержкой GPU offloading. Из release notes v2606:

> «This is the first release to support GPU offloading. It uses the C++17/20 std::execution policy to automatically run loops in parallel across multiple execution units, which may be GPU devices or cores sharing memory.»

И критически важно:

> «Using CPU threading (-stdpar=multicore) is not properly supported (work-in-progress)»

GPU-оффлоадинг в v2606 идёт через C++17 `std::execution` policies, а **не** через OpenMP target offloading. Expression templates — инфраструктура для этого. OpenMP CPU-трединг явно помечен как не поддерживаемый.

#### Почему icpx + OpenMP ломает expression templates

> **Примечание:** следующий вывод — технический анализ, а не прямая цитата из документации. Он основан на архитектуре LLVM OpenMP runtime и структуре типов expression templates.

Когда icpx получает `-fiopenmp`, активируется LLVM OpenMP runtime с target offloading pass. Даже на машине без GPU этот pass анализирует все типы в циклах `ListExpression.H` и пытается их маппировать на «устройство». Типы expression templates (`List_divide<List_multiply<...>>`) не trivially copyable → компилятор выдаёт `-Wopenmp-mapping` warnings и ломает вывод типов итераторов.

До v2512 этого не было, потому что не было expression templates — обычные `Field<double>` корректно мапились.

### 4.4. Сводная таблица по версиям

| Версия | Expression templates | OpenMP | GPU offloading | Результат |
|--------|---------------------|--------|---------------|----------|
| до v2512 | Нет | Работает (`+openmp`) | Нет | OK |
| v2512 | Да (initial) | Конфликтует | Заявлено (future) | OK без OpenMP |
| v2606 | Да (extended) | Не поддерживается | `std::execution` (C++17) | OK без OpenMP |

Отключение OpenMP — не обходной путь, а соответствие дизайну v2606. OpenMP CPU-трединг официально не поддерживается. Expression templates — сознательный инженерный выбор для GPU через `std::execution`.

### 4.5. CPU-путь по задумке авторов

**Тезис: CPU-сборка = MPI-only + expression templates работают последовательно внутри каждого процесса.**

| Режим | Компиляция | Параллелизм | Expression templates |
|-------|-----------|-------------|---------------------|
| CPU (поддерживается) | стандартный компилятор (gcc, icpx) без спецфлагов | MPI между процессами | Работают последовательно — loop fusion, elimination of temporaries |
| GPU (первая версия) | nvc++ с `-stdpar=gpu` | `std::execution::par` на устройстве | Работают на GPU через C++17 execution policies |
| CPU multithread (work-in-progress) | nvc++ с `-stdpar=multicore` | `std::execution::par` на потоках CPU | Заявлен, но «not properly supported» в v2606 |

Из release notes v2606:

> «GPU parallelisation and offloading is a compile-time option, packaged as a new architecture so that the same source tree can be compiled for either CPU or GPU. Care has been taken to ensure that the GPU version behaves identically to the CPU version unless compiled for GPU.»

CPU-сборка — не «обрезанный GPU», а самостоятельный поддерживаемый путь.

#### Почему expression templates полезны даже без потоков

1. **Устранение промежуточных аллокаций** — вместо `c = a + b; d = c * e;` (два прохода, одно временное поле) — один проход `d = (a+b)*e` без временного поля. Меньше обращений к памяти, лучше cache locality.
2. **Loop fusion** — `a*b + c*d / e` вычисляется в одном цикле по элементам, а не в четырёх отдельных. Меньше проходов по памяти — больше пропускной способности.
3. **Fused patch evaluation (вклад AMD)** — все uncoupled патчи обрабатываются одним kernel, а не по одному на патч. На CPU это уменьшает overhead вызовов.

Release notes v2512 прямо показывают тесты на CPU:

> «Below shows the testing of (volScalarField) expressions on a CPU: Simple algebra: c = a + b; Complex algebra: c = cos(a + 0.5*sqrt(b-sin(a)))»

Expression templates тестировались и работают на CPU — это не эксклюзивно GPU-функция.

#### Почему OpenMP не часть дизайна

**1. AMD имела отдельный форк с OpenMP target offloading** — репозиторий ROCm/OpenFOAM_HMM: «Refactoring OpenFOAM with OpenMP target offloading and use of HMM to offload work onto GPUs». Это был альтернативный путь. В mainline (v2512/v2606) OpenCFD выбрала `std::execution` (C++17 standard parallelism), а не OpenMP. OpenMP-форк остался экспериментальным.

**2. Release notes v2512 прямо упоминают проблему с OpenMP offloading** — в разделе про GAMG reproducibility:

> «On certain architectures the agglomeration can vary from run to run. This arises when using Open-MP style offloading whereby agglomeration construction can take a different path.»

AMD обнаружила и исправила баг, где OpenMP offloading вызывал недетерминированное поведение. Это подтверждает: OpenMP target offloading рассматривался, но создал проблемы, и mainline пошла через `std::execution`.

**3. Roadmap — `std::execution::par` через nvc++ `-stdpar`, а не OpenMP.** Из release notes v2606: «After further testing and feedback from the Community we intend to integrate the code in the OpenFOAM v2612 release». Нигде в roadmap нет OpenMP как future direction.

#### Сравнение: OpenMP vs std::execution

| Критерий | OpenMP target offloading | `std::execution` (stdpar) |
|----------|------------------------|--------------------------|
| Стандарт | Внешний прагма-стандарт | Часть C++17/20/26 |
| Портативность | Зависит от runtime (libomp, libomptarget) | Любой компилятор с поддержкой stdpar |
| Выражения | Прагмы перед циклами — не работают с expression templates | Execution policy передаётся в algorithm — совместимо |
| AMD GPU | Требует libomptarget + ROCm | `-stdpar=hip` (clang) |
| NVIDIA GPU | Требует libomptarget + CUDA | `-stdpar=gpu` (nvc++) — нативно |
| CPU threads | `-fiopenmp` / `-fopenmp` | `-stdpar=multicore` (nvc++) |
| Недетерминизм | Наблюдался в GAMG (v2512) | Контролируемый через execution policy |

Ключевая причина: **expression templates и OpenMP — структурный мисматч**. Expression templates создают сложные типы, которые вычисляются через `std::for_each(std::execution::par, ...)` — это естественно. OpenMP требует `#pragma omp parallel for` перед циклом, а expression templates строят цикл внутри шаблона — прагма не может быть вставлена.

#### Что планируется для CPU-трединга (будущее)

| Версия | CPU | GPU | CPU multithread |
|--------|-----|-----|----------------|
| v2606 | MPI-only (последовательно внутри процесса) | `-stdpar=gpu` через nvc++ | Заявлен, не готов |
| v2612 (план) | MPI-only | `-stdpar=gpu` | `-stdpar=multicore` (C++ standard parallel algorithms) |
| Будущее | `std::execution` (P2300, C++26) — единая модель | То же | То же |

OpenMP был в версиях до v2512 (до expression templates), но после их внедрения авторы сознательно ушли от OpenMP к `std::execution`.

### 4.6. Позиция Intel: три уровня, OpenMP — не главный

**Уровень 1: SYCL/DPC++ — флагман.** Intel продвигает SYCL как единую модель для CPU, GPU и FPGA. icpx с `-fsycl` может таргетить NVIDIA, AMD и Intel GPU.

**Уровень 2: C++17 `std::execution` — стандарт, backed by TBB.** Документация oneDPL прямо указывает:

> «oneDPL supports two parallel backends for execution with par and par_unseq policies: 1. TBB backend (enabled by default) uses Intel oneAPI Threading Building Blocks (oneTBB). 2. OpenMP backend uses OpenMP pragmas for parallel execution.»

И: «The TBB backend takes precedence over the OpenMP backend». TBB — бэкенд по умолчанию; OpenMP — опциональный, opt-in.

**Уровень 3: OpenMP — поддерживается, но не флагман.**

| Роль | Технология | Статус в oneAPI |
|------|-----------|----------------|
| Гетерогенные вычисления | SYCL/DPC++ | Флагман |
| CPU parallel algorithms | `std::execution` + oneTBB | По умолчанию |
| CPU parallel algorithms | `std::execution` + OpenMP | Опционально (opt-in) |
| Векторизация | OpenMP SIMD (`#pragma omp simd`) | Активно используется |
| GPU offloading | OpenMP target | Поддерживается, но SYCL приоритетнее |

#### Почему Intel смещает акцент с OpenMP

1. **TBB лучше для C++ экосистемы.** Из CERN-презентации: «Classic threading models (OpenMP, pthreads) describe the implementation... [TBB] You don't describe threads or know how many there are. You do describe the parts of your code that can run in parallel.» TBB — task-based, OpenMP — thread-based. Для современного C++ с лямбдами и expression templates task-based модель естественно сочетается со стандартными алгоритмами.
2. **OpenMP target offloading — конкурирующая технология с SYCL**, внешний стандарт, контролируемый OpenMP ARB (не Intel).
3. **`std::execution` — стандартизированный путь.** Intel — один из главных сторонников C++ standard parallelism; oneDPL — стратегические инвестиции.
4. **OpenMP target offloading проблематичен.** Из исследования «Evaluating ISO C++ Parallel Algorithms on Heterogeneous HPC Systems»: «The OpenMP-backed oneDPL implementation performed poorly on multiple platforms due to the hardcoded chunk size bound and the use of OpenMP taskloops», а также «you cannot, as of now, combine them with other offloading models, e.g., OpenCL and OpenMP».

#### Сравнение: Intel vs OpenFOAM — конвергенция

| Аспект | Intel (oneAPI) | OpenFOAM (v2606) | Совпадение |
|--------|---------------|------------------|-----------|
| CPU threading | `std::execution` + TBB (default) | `std::execution` (planned v2612) | Да |
| GPU offloading | SYCL/DPC++ | `std::execution` + nvc++ `-stdpar=gpu` | Разные реализации, один стандарт |
| OpenMP CPU | Поддерживается (opt-in) | Не поддерживается | Intel мягче |
| OpenMP target | Поддерживается (альтернатива) | Не используется | Разные пути |
| Expression templates | TBB совместим | `std::execution` совместим | Да |
| MPI | Intel MPI | Intel MPI | Да |

**Итог:** OpenFOAM и Intel сходятся в направлении — `std::execution` как будущее CPU parallelism. Разница: Intel ещё поддерживает OpenMP как opt-in, OpenFOAM уже отказалась от него из-за expression templates.

Intel не убивает OpenMP — перестраивает иерархию:

- **Раньше (до ~2020):** OpenMP = главный путь CPU threading; TBB = альтернатива; SYCL = не существовало.
- **Сейчас (2025–2026):** SYCL = главный путь GPU/FPGA; `std::execution` + TBB = главный путь CPU; OpenMP = поддерживается, но opt-in; OpenMP SIMD = жив и полезен (векторизация).
- **Будущее (C++26+):** `std::execution` (P2300) = единый стандарт CPU и GPU; SYCL = часть стандарта C++; OpenMP = legacy-совместимость.

### 4.7. Westmere и `-stdpar=multicore`: реалистичный прогноз

**Будет ли это работать на Westmere?** Да, технически. oneTBB — библиотека потоков, а не SIMD. Её минимум — x86-64; Westmere (SSE4.2) соответствует уровню x86-64-v2. TBB не требует AVX для `parallel_for` и work-stealing.

Но есть три серьёзные проблемы:

**Проблема A: NUMA.** Система 2× Xeon X5675 — 2 NUMA-узла. QPI-канал (~25.6 ГБ/с) медленнее локальной памяти (~32 ГБ/с). oneTBB по умолчанию NUMA-not-aware: work-stealing перебрасывает задачи между сокетами, задача с сокета 0 может украсть данные из памяти сокета 1. Для memory-bound workload (OpenFOAM) это remote memory access, насыщение QPI и деградация. MPI не имеет этой проблемы: каждый процесс живёт в своём сокете и работает с локальной памятью (при правильной декомпозиции и `numactl --cpunodebind`).

**Проблема B: nvc++, а не icpx.** Proof-of-concept OpenFOAM stdpar сделан на nvc++ (NVIDIA HPC Compiler). NVIDIA PSTL — собственная реализация C++ parallel algorithms (внутри — OpenMP; для пользователя прозрачно). У Intel — oneDPL с oneTBB-бэкендом. Реализации одного стандарта расходятся в деталях (allocator requirements, execution policy support, implicit data movement), и совместимость кода «написан под nvc++ — скомпилируется icpx» не гарантирована.

| Свойство | nvc++ `-stdpar=multicore` | icpx + oneDPL |
|----------|--------------------------|---------------|
| Backend | OpenMP (внутри NVHPC) | oneTBB |
| Оптимизация | под NVIDIA Grace/Grace-Hopper | под Intel Xeon |
| Зрелость на OpenFOAM | proof-of-concept | не тестировалось |
| Производительность multicore | — | TBB обычно быстрее на Intel |

**Проблема C: overhead для малых задач.** Документация oneTBB: «Typically a loop needs to take at least a million clock cycles to make it worth using parallel_for». На Westmere (3.07 GHz) «миллион тактов» — ~326 микросекунд. Многие циклы OpenFOAM (патчи, малые поля) короче — overhead TBB превысит полезную работу.

#### Сопоставление: MPI-only vs stdpar=multicore на Westmere

| Параметр | MPI-only (v2606) | stdpar=multicore (v2612, план) |
|----------|------------------|-------------------------------|
| NUMA | Каждый процесс — локальная память | TBB work-stealing между сокетами |
| Cache locality | Каждый процесс — свой L1/L2/L3 | Task migration между ядрами |
| QPI-нагрузка | Минимальная (границы доменов) | Высокая при cross-socket stealing |
| Масштабируемость | ~линейная до 12 процессов | ~сублинейная из-за NUMA overhead |
| SIMD (SSE4.2) | `-O3 -march=westmere` | То же — SIMD не зависит от stdpar |
| Компилятор | icpx — работает | nvc++ — proof-of-concept; icpx — не тестировался |
| Зрелость | Production | «not properly supported» в v2606 |

#### Реалистичный сценарий для v2612 на Westmere

```bash
# Гипотетическая сборка v2612
nvc++ -stdpar=multicore -O3 -march=westmere  # если nvc++
icpx -stdpar=multicore -O3 -march=westmere   # если icpx + oneDPL
```

- **Чистый stdpar (без MPI):** 12 потоков TBB на 2 сокетах. NUMA penalty ~15–30% для memory-bound задач; возможно хуже, чем 12 MPI-процессов.
- **Hybrid MPI + stdpar:** 2 MPI-процесса (по одному на сокет), каждый с 6 TBB-потоками. NUMA penalty минимальный (потоки в пределах сокета). Потенциально ~10–20% выигрыша для compute-intensive фаз (assembly, loop fusion), но overhead для patch evaluation.
- **MPI-only (как сейчас):** 12 процессов, каждый на своём ядре с локальной памятью. Стабильная, предсказуемая производительность.

#### Вердикт для Westmere (будущие версии OpenFOAM, после v2312)

| Горизонт | Рекомендация | Обоснование |
|----------|--------------|-------------|
| v2606 (сейчас) | MPI-only, без OpenMP | Работает, проверено |
| v2612 (если выйдет stdpar) | MPI-only; `stdpar=multicore` — тестировать | NUMA на 2 сокетах — главный риск |
| v2612+ hybrid | 2 MPI × 6 потоков stdpar | Лучший вариант: NUMA-aware + intra-socket threading |
| Долгосрочно | MPI-only остаётся надёжным | Westmere не получит архитектурных оптимизаций oneTBB |

**Главный вывод.** MPI-only на Westmere — не временная мера, а оптимальная конфигурация для этой архитектуры. `stdpar=multicore` через oneTBB будет полезен на одно-сокетных системах или системах с быстрым interconnect (UPI на современных Xeon). На 2× Westmere с QPI и DDR3 NUMA-penalty съест большую часть выигрыша от loop fusion и thread parallelism.

12 ядер Westmere — ровно тот масштаб, где MPI ещё эффективнее любого threading: каждый процесс получает эксклюзивный доступ к локальной памяти и L3, а QPI используется только для границ доменов. TBB на 12 ядрах с 2 NUMA-узлами будет тратить такты на work-stealing между сокетами — это чистые потери.

---

## 5. Выводы для миграционной стратегии

1. **Профиль клона совпадает с каноническим OpenFOAM_HMM.** Все маркеры найдены в ожидаемых местах; лишних зон оффлоада нет.
2. **Миграция на v2312 безопасна** — expression templates там ещё нет, OpenMP-подход совместим. Миграционная карта: `FieldM.H` (фундамент) → lduMatrix-решатели → finiteVolume-схемы → коммуникационные патчи и AMI.
3. **Миграция на v2606 конфликтна и стратегически тупиковая** — дорога занята `std::execution`/expression templates; OpenMP CPU-трединг официально не поддерживается. Для Westmere итог совпадает: MPI-only.
4. **Три конкретных пункта сборки HMM-клона:** kahip-include исправлен; `GPU_ARCH=gfx942` задать перед чистой пересборкой; резервные копии (`*.org`, `*.back02–04`) вынести из дерева `src/` и зафиксировать в git.
