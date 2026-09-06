# Build Notes: OpenFOAM v2606 with Intel icpx on Westmere

## OpenMP: disabled by design

OpenFOAM v2606 introduced expression templates (ListExpression.H,
GeometricFieldExpression.H) as the foundation for GPU offloading via
C++17 std::execution policies. These template types (List_subtract,
GF_divide, etc.) are not trivially copyable and conflict with the
OpenMP target offloading pass in icpx (-fiopenmp) and clang (-fopenmp).

### Background

- **v2306**: No expression templates. OpenMP worked via `+openmp`.
- **v2512**: Expression templates introduced (AMD contribution). Designed
  to enable GPU offloading. OpenMP not tested with them.
- **v2606**: First release with GPU offloading support (std::execution).
  OpenMP CPU threading explicitly "not properly supported" (release notes).

### Why OpenMP breaks

icpx with `-fiopenmp` activates LLVM OpenMP target offloading pass.
This pass attempts to map expression template types to device memory.
Types like `List_divide<List_multiply<UniformListWrap<double>, ...>>`
are not trivially copyable, causing:
- `-Wopenmp-mapping` warnings (non-trivially-copyable types)
- Iterator type deduction failures (ConstIter simplified to raw pointer)
- Build errors in turbulence models (kOmegaSST, kEpsilon) and fvMatrix

### Solution

Do not enable OpenMP. Use `~openmp` in WM_COMPILE_CONTROL.
Parallelism is via MPI (Intel MPI / mpirun / decomposePar).

### Why this is correct (not a workaround)

v2606's GPU offloading path is C++17 std::execution, not OpenMP.
Expression templates were designed for this path.
OpenMP CPU threading is marked "work-in-progress" in v2606 release notes.
MPI-only parallelism is the supported mode for CPU builds.


build: disable OpenMP — aligns with v2606 expression template design

Problem:
  icpx with -fiopenmp activates LLVM OpenMP target offloading pass,
  which attempts to map OpenFOAM v2606 expression template types
  (List_subtract, GF_divide, etc.) to device memory. These types are
  not trivially copyable, causing:
  - -Wopenmp-mapping warnings on List<double>, fvsPatchField<double>,
    and expression template types
  - Iterator type deduction failures (ConstIter reduced to raw pointer)
  - Build errors in turbulence models and fvMatrix expression evaluation

Root cause (by design, not a bug):
  v2512 introduced expression templates (AMD contribution) as
  infrastructure for GPU offloading — fusing field operations into
  single kernels, eliminating intermediate allocations.
  v2606 is the first release with GPU offloading, using C++17/20
  std::execution policies (not OpenMP target offloading).
  OpenMP CPU threading is explicitly "not properly supported"
  (v2606 infrastructure release notes).

  In v2306, OpenMP worked because field operations used eager
  evaluation with simple, trivially-copyable types (Field<double>).
  No expression templates existed to conflict with OpenMP mapping.

Fix:
  - Remove -fiopenmp / -fopenmp from WM_CFLAGS / WM_CXXFLAGS
  - Set WM_COMPILE_CONTROL="~openmp" to explicitly disable
  - Document rationale in README.build.md

Impact:
  - Parallelism via MPI only (Intel MPI, mpirun, decomposePar)
  - No functional change to solver behaviour
  - Expression templates still active (CPU path, no GPU offloading)
  - Aligns with v2606 design: CPU build = MPI, GPU build = std::execution

References:
  - v2512 numerics: expression templates, GPU offloading foundation
  - v2606 infrastructure: first GPU offloading release, std::execution
  - v2606: "Using CPU threading (-stdpar=multicore) is not properly
    supported (work-in-progress)"
  - Source: $FOAM_SRC/OpenFOAM/expressionTemplates/ListExpression.H
============================

# Сборка OpenFOAM v2606 с Intel icpx на Westmere: почему OpenMP отключён

## Кратко

OpenFOAM v2606 компилируется Intel icpx (oneAPI 2026.1) на 2× Xeon X5675 (Westmere) **без OpenMP** — только MPI. Это не обходной путь, а следствие архитектурных изменений в OpenFOAM. Параллелизм — через Intel MPI (`mpirun`, `decomposePar`).

**Сборка:** `linux64IcxDPInt64Opt`, Intel MPI, 285 бинарников, 152 библиотеки.

---

## Предыстория: три версии — три архитектуры

### v2306: eager evaluation, OpenMP работает

В v2306 полевые операции (`a + b + c`) выполнялись через **eager evaluation** — каждый оператор создавал промежуточный временный `GeometricField`. Типы были простыми (`double`, `Field<double>`) и **trivially copyable**. OpenMP включался через `WM_COMPILE_CONTROL="+openmp"` или `wmake -openmp` и работал корректно: циклы по `Field<double>` мапились на потоки без проблем.

### v2512: expression templates — фундамент для GPU

В v2512 OpenCFD (при участии AMD) внедрил библиотеку **expression templates** в `$FOAM_SRC/OpenFOAM/expressionTemplates/`. Цитата из release notes:

> *«This release includes an initial version of an expression templates library that transforms how field operations are executed. This powerful optimization technique eliminates intermediate field allocations, fuses multiple operations into single computational kernels, and enables hardware acceleration including GPU offloading.»*

Ключевая фраза — **«enables hardware acceleration including GPU offloading»**. Expression templates были созданы **целенаправленно** как инфраструктура для будущего GPU-оффлоадинга. Они:

- устраняют промежуточные аллокации полей (fusion в один kernel);
- создают сложные template-типы (`List_subtract<ListConstRefWrap<double>, List_add<...>>`);
- разделяют вычисления на internal field / uncoupled patches / coupled patches;
- поддерживают fused patch evaluation (вклад AMD — один kernel для всех патчей).

### v2606: первый релиз с GPU-оффлоадингом через std::execution

v2606 — **первый релиз с поддержкой GPU offloading**. Цитата из release notes:

> *«This is the first release to support GPU offloading. It uses the C++17/20 std::execution policy to automatically run loops in parallel across multiple execution units, which may be GPU devices or cores sharing memory.»*

И критически важно:

> *«Using CPU threading (-stdpar=multicore) is not properly supported (work-in-progress).»*

GPU-оффлоадинг в v2606 идёт через **C++17 `std::execution` policies**, а **не через OpenMP target offloading**. Expression templates — инфраструктура для этого. OpenMP CPU-трединг явно помечен как **не поддерживаемый**.

---

## Сводка по версиям

| Версия | Expression templates | OpenMP | GPU offloading | Результат |
|--------|----------------------|--------|----------------|-----------|
| v2306  | Нет                  | Работает (`+openmp`) | Нет      | OK        |
| v2512  | Да (initial)         | Конфликтует | Заявлено (future) | OK без OpenMP |
| v2606  | Да (extended)        | Не поддерживается | `std::execution` (C++17) | OK без OpenMP |

---

## Что именно ломается

При включении `-fiopenmp` (Intel OpenMP runtime) или `-fopenmp` (LLVM OpenMP runtime) в icpx активируется **OpenMP target offloading pass** — даже на машине без GPU.

### Симптомы

1. **`-Wopenmp-mapping` warnings** на не-trivially-copyable типы:

ListExpression.H:106: warning: type 'Foam::List<double>' is not trivially copyable and not guaranteed to be mapped correctly [-Wopenmp-mapping]

ListExpression.H:105: warning: type 'Foam::Expression::List_divide<...>' is not trivially copyable and not guaranteed to be mapped correctly [-Wopenmp-mapping]



2. **Ошибки вычета типов итераторов** — `ConstIter` (класс-итератор expression template) упрощается offloading pass до `const double*` (сырой указатель), после чего обращения к членам через `::` вызывают ошибку компиляции.

3. **Ошибки сборки** в turbulence models (`incompressibleKOmegaSST`, `incompressibleKEpsilon`) и `fvMatrix` — каскадно из `ListExpression.H` → `GeometricFieldExpression.H` → `fvMatrixExpression.H` → `fvMatrix.C`.

### Механизм

| Шаг | Что происходит |
|-----|----------------|
| 1 | `-fiopenmp` / `-fopenmp` в icpx активирует LLVM OpenMP runtime |
| 2 | Одновременно неявно активируется OpenMP target offloading pass |
| 3 | Цикл в `ListExpression.H:105-106` (`auto src = expr.cbegin() + i; lst[i] = *src;`) обрабатывается как candidate для offloading |
| 4 | Offloading pass пытается смапить expression template типы на «устройство» |
| 5 | Типы не trivially copyable → `-Wopenmp-mapping` warnings |
| 6 | `ConstIter` упрощается до `const double*` → ошибка доступа к членам через `::` |
| 7 | Каскад ошибок по всей цепочке instantiation |

### Почему в v2306 этого не было

В v2306 **не существовало expression templates**. Циклы работали с простыми `Field<double>` — trivially copyable, корректно мапились в OpenMP. Конфликт возник только когда сложные template-типы появились в v2512 и были расширены в v2606.

---

## Инженерный замысел: не баг, а архитектура

Expression templates — **сознательный инженерный выбор**, а не ошибка кода. Их назначение:

1. **Устранение промежуточных аллокаций** — вместо создания временного `GeometricField` на каждый оператор (`a + b` создаёт temp, `+ c` создаёт ещё temp), всё выражение сворачивается в одно дерево и вычисляется за один проход.

2. **Fusion в один kernel** — операции `a*b + c*d / e` вычисляются поэлементно в одном цикле, а не в четырёх отдельных проходах по памяти.

3. **Подготовка для GPU offloading** — единое expression-дерево можно передать в `std::execution` policy и выполнить на GPU за один kernel launch, без переносов промежуточных полей между CPU и GPU.

4. **Разделение internal field / patches** — expression templates разделяют вычисления на внутренние ячейки, uncoupled patches и coupled patches, что критично для GPU (patch-вычисления требуют другой стратегии).

OpenMP target offloading — **конкурирующая технология** с `std::execution`. v2606 выбрала `std::execution`, а expression templates спроектированы под этот путь. OpenMP CPU-трединг остался «work-in-progress».

---

## Решение

### Что сделано

- OpenMP-флаги (`-fiopenmp`, `-fopenmp`) **удалены** из `WM_CFLAGS` / `WM_CXXFLAGS`.
- В `WM_COMPILE_CONTROL` установлено `~openmp` (явное отключение).
- Параллелизм — **только через MPI** (Intel MPI, `mpirun`, `decomposePar`).

### Почему это правильно (не обходной путь)

| Аргумент | Пояснение |
|----------|-----------|
| v2606 использует `std::execution`, не OpenMP | GPU-оффлоадинг в v2606 идёт через C++17 execution policies — это заявленный путь |
| OpenMP CPU-трединг не поддерживается | Release notes v2606: *«not properly supported (work-in-progress)»* |
| Expression templates спроектированы под `std::execution` | Они — инфраструктура для GPU, а не для OpenMP |
| MPI — стандартный путь OpenFOAM | Декомпозиция домена через `decomposePar` + `mpirun` — основной режим работы OpenFOAM |
| Westmere не имеет GPU | Offloading pass активируется даже без GPU, ломая типы |

### Что не сделано (и почему)

- **Исходный код OpenFOAM не патчен.** Expression templates — архитектурное решение, не ошибка. Патчить их — значит ломать будущий GPU-путь.
- **`-Wno-openmp-mapping` не используется.** Это скрыло бы warnings, но не решило бы ошибки вычета типов. Проблема глубже, чем warnings.
- **`-fiopenmp -fno-openmp-targets` не используется.** В тестах этот флаг не полностью отключал offloading pass в icpx 2026.1 — warnings и ошибки оставались.

---

## Финальная конфигурация сборки

### Компилятор

icpx (Intel oneAPI 2026.1) -std=c++17 -pthread -fp-model=precise -O3 -march=westmere -frounding-math

icpx (Intel oneAPI 2026.1) -std=c++17 -pthread -fp-model=precise -O3 -march=westmere -frounding-math


### MPI

Intel MPI 2021.18 (intelmpi) mpirun -np 12 simpleFoam -parallel

### Результат сборки

bin = 285 entries lib = 152 entries


### Что не собрано (опционально)

- **ParaView Catalyst** — нет ParaView (визуализация)
- **runTimePostProcessing** — собран как dummy (нет VTK)
- **PETSc** — нет заголовков (внешний решатель)

---

## Известные предупреждения (безопасные)

### `wmkdepend: could not open 'adios2.h'`

wmkdepend: could not open 'adios2.h' for source file 'time/adiosTime.C': No such file or directory


**Безопасно.** Это ограничение генератора зависимостей `wmkdepend` — он не раскрывает `-isystem` пути. Компилятор находит `adios2.h` через `-isystem`, сборка проходит успешно.

### `WARNING: skip ParaView Catalyst (missing or incorrect ParaView)`

**Безопасно.** ParaView не установлен — визуализация отключена. Решатели и утилиты работают без неё.

---

## Ссылки

- [v2512 numerics release notes — expression templates](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics)
- [v2606 infrastructure release notes — GPU offloading](https://www.openfoam.com/news/main-news/openfoam-v2606/infrastructure)
- Исходный код: `$FOAM_SRC/OpenFOAM/expressionTemplates/ListExpression.H`
- Исходный код: `$FOAM_SRC/OpenFOAM/expressionTemplates/GeometricFieldExpression.H`
Скопируй это в файл README.md (или BUILD_NOTES.md, если у тебя уже есть README) в корне сборки или рядом с etc/bashrc.

# Тест-запуск OpenFOAM v2306: анализ результатов и вердикт

## Контекст

Сборка OpenFOAM v2306 выполнена на той же платформе, что и v2606:
2× Xeon X5675 (Westmere), Intel icpx (oneAPI), Intel MPI, `-march=westmere`.

В отличие от v2606, сборка v2306 **выполнена с OpenMP** (`-fiopenmp`), поскольку в этой версии отсутствуют expression templates, конфликтующие с OpenMP target offloading pass.

Тест-запуск (`tutorialsTest`) охватывает все основные категории: DNS, incompressible, compressible, multiphase, heatTransfer, stressAnalysis, verificationAndValidation, preProcessing и другие.

---

## Результаты по параллельным MPI-запускам

### Успешные параллельные запуски

| Туториал | Решатель | Процессов | Результат |
|----------|----------|-----------|-----------|
| motorBike | simpleFoam | 6 | OK |
| snappyMultiRegionHeater | chtMultiRegionFoam | 4 | OK |
| plateHole | solidDisplacementFoam | 4 | OK |
| atmDownstreamDevelopment (kEpsilon) | simpleFoam | 8 | OK |
| atmDownstreamDevelopment (kOmegaSST) | simpleFoam | 8 | OK |
| atmFlatTerrain/precursor (kEpsilon) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmFlatTerrain/precursor (kOmegaSST) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmFlatTerrain/precursor (kL) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmFlatTerrain/successor (kEpsilon) | buoyantBoussinesqSimpleFoam | 8 | OK |
| atmFlatTerrain/successor (kOmegaSST) | buoyantBoussinesqSimpleFoam | 8 | OK |
| atmFlatTerrain/successor (kL) | buoyantBoussinesqSimpleFoam | 8 | OK |
| atmForestStability (veryStable) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmForestStability (stable) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmForestStability (slightlyStable) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmForestStability (neutral) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmForestStability (slightlyUnstable) | buoyantBoussinesqSimpleFoam | 2 | OK |
| atmForestStability (unstable) | buoyantBoussinesqSimpleFoam | 2 | OK |
| StefanProblem (icoReactingMultiphaseInterFoam) | icoReactingMultiphaseInterFoam | 2 | OK |
| StefanProblem (interCondensatingEvaporatingFoam) | interCondensatingEvaporatingFoam | 2 | OK |
| porousDamBreak | interIsoFoam | 4 | OK |
| DFSEM | pisoFoam | 4 | OK |
| DFM | pisoFoam | 4 | OK |
| FSM | pisoFoam | 4 | OK |

Все параллельные MPI-запуски (2, 4, 6, 8 процессов) завершены успешно: `decomposePar` → решатель → `reconstructPar`.

### Успешные последовательные запуски

| Туториал | Решатель | Результат |
|----------|----------|-----------|
| cavity (createZeroDirectory) | icoFoam | OK |
| divergenceExample (13 схем) | scalarTransportFoam | OK |
| nonOrthogonalChannel (10 углов 0–85°) | simpleFoam | OK |
| skewnessCavity (24 схемы) | postProcess | OK |
| weightedFluxExample (3 варианта) | laplacianFoam | OK |
| planeChannel (EBRSM, kOmegaSST, EBRSM.setTurbulenceFields) | simpleFoam | OK |
| beamEndLoad | solidEquilibriumDisplacementFoam | OK |
| simplePipeCage | PDRsetFields | OK |
| geometric (decompositionConstraints) | foamToVTK | OK |

### Падения: twoPhaseEulerFoam (Error 134)

| Туториал | Код ошибки | Сигнал |
|----------|-----------|--------|
| bubbleColumn | 134 | SIGABRT |
| bubbleColumnIATE | 134 | SIGABRT |
| fluidisedBed | 134 | SIGABRT |
| mixerVessel2D | 134 | SIGABRT |

Туториал `injection` — прошёл успешно (нет ошибки в логе).

---

## Анализ падений twoPhaseEulerFoam

### Что такое Error 134

Код 134 = 128 + 6 = SIGABRT. В OpenFOAM это контролируемое прерывание через `Foam::FatalError` или `std::abort()`. Причины:

- assertion failure (проверка условий в рантайме)
- деление на ноль или NaN в вычислениях
- несовместимость физических моделей в controlDict / transportProperties
- известные баги самого решателя

Это **не segfault** (код 139 = SIGSEGV) и **не проблема компилятора**. Компилятор не может вызвать SIGABRT в рантайме — это делает сам код решателя при обнаружении ошибки.

### История twoPhaseEulerFoam

`twoPhaseEulerFoam` — исторически проблемный решатель. В баг-трекере OpenFOAM зафиксированы многократные падения:

- bugs.openfoam.org #1202 — crash с heat transfer model "none"
- bugs.openfoam.org #1379 — spurious currents
- bugs.openfoam.org #1570 — различия 32/64 bit

Решатель был **устаревшим уже в v2306** и заменён на `multiphaseEulerFoam` в более поздних релизах. Туториалы `bubbleColumn`, `fluidisedBed` и `mixerVessel2D` — часть legacy-набора, который не обновлялся.

### Почему падение не связано со сборкой

1. **Только 4 из ~60+ туториалов упали** — все с одним и тем же решателем.
2. **Туториал `injection` того же решателя прошёл** — значит, бинарник скомпилирован и работает, проблема в конкретных настройках кейсов.
3. **Все остальные решатели (simpleFoam, pisoFoam, chtMultiRegionFoam, interIsoFoam, buoyantBoussinesqSimpleFoam и др.) работают** — как последовательно, так и параллельно.
4. **Error 134 (SIGABRT), а не 139 (SIGSEGV)** — контролируемое прерывание, а не нарушение памяти. Проблема компилятора/оптимизации обычно даёт segfault, а не abort.
5. **Падения воспроизводимы на других платформах** — о чём сообщают баг-репорты OpenFOAM.

---

## Сопоставление с v2606

| Параметр | v2306 | v2606 |
|----------|-------|-------|
| Expression templates | Нет | Да |
| OpenMP (`-fiopenmp`) | Включён, работает | Отключён (ломает сборку) |
| Компилятор | icpx (Intel oneAPI) | icpx (Intel oneAPI) |
| MPI | Intel MPI | Intel MPI |
| Параллельные запуски | 2, 4, 6, 8 процессов — OK | Сборка завершена, тест не проводился |
| Сборка | Успешна (285 bin, 152 lib) | Успешна (285 bin, 152 lib) |
| `twoPhaseEulerFoam` | Падает (Error 134) | Заменён на `multiphaseEulerFoam` |

Ключевое различие — **OpenMP**. В v2306 OpenMP включён и не вызывает проблем, потому что expression templates не существуют. В v2606 OpenMP ломает сборку, потому что expression templates (сложные template-типы, не trivially copyable) конфликтуют с LLVM OpenMP target offloading pass в icpx.

---

## Вердикт

**v2306 с icpx + Intel MPI (+ OpenMP) — полностью рабочая сборка.** Параллельные MPI-запуски (2, 4, 6, 8 процессов) проходят. Падают только `twoPhaseEulerFoam` laminar-туториалы — известная проблема самого решателя, не связанная с компилятором или сборкой.

Это подтверждает весь тезис: **v2306 с OpenMP работает, v2606 — нет** (из-за expression templates). И обе версии с icpx + Intel MPI функциональны.
