# Сборка OpenFOAM с Intel icpx и OpenMP: анализ производительности

![OpenFOAM](https://img.shields.io/badge/OpenFOAM-v2306%20%7C%20v2606-blue)
![Compiler](https://img.shields.io/badge/Intel%20icpx-oneAPI-orange)
![OpenMP](https://img.shields.io/badge/OpenMP--fiopenmp-red)

---

## Краткий вывод

На идеальных циклах AVX-512 рвёт SSE4.2 в 2–4×. Но в OpenFOAM v2306 90% времени съедают GAMG и smoothSolver — операции с косвенной адресацией и рекуррентными зависимостями, которые не векторизуются ни одним SIMD. Поэтому SIMD-преимущество AVX-512 над SSE4.2 в OpenFOAM — всего ~6–11%.

OpenMP на Westmere компенсирует этот разрыв за счёт того, что он параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге `-march=westmere -fiopenmp` дают примерно тот же прирост, что `-xCORE-AVX512` на Xeon Gold — не потому что Westmere хорош, а потому что OpenFOAM плохо векторизуется, и SIMD-ширина почти не имеет значения.

Но есть второй, не менее важный фактор: **флаги компиляции**. Связка `-fp-model precise` + `-frounding-math` приближается к `-fp-model strict` по степени блокировки оптимизаций. Удаление `-frounding-math` и добавление `-no-prec-div` даёт ~3–8% прироста — сопоставимо с тем, что даёт AVX-512.

И есть третий фактор — **архитектурный**: в OpenFOAM v2606 expression templates сделали OpenMP несовместимым с design-целями релиза. Отключение OpenMP — не обходной путь, а соответствие архитектуре v2606.

---

## Часть 1. Почему CFD-сообщество жалуется на AVX-512

Это известная боль: покупают Xeon Gold с AVX-512, компилируют OpenFOAM с `-xCORE-AVX512`, ждут двукратного ускорения — а получают 10–15%. Потом пишут на CFD Online: «AVX-512 не работает в OpenFOAM, что я делаю не так?»

Ответ в одном: структура данных `lduAddressing` с косвенной адресацией просто не даёт компилятору векторизовать то, что съедает 90% времени. Это не баг — архитектурное решение OpenFOAM, заточенное под работу с произвольными неструктурированными сетками.

### 1.1. Структура данных `lduAddressing` — почему векторизация не работает

OpenFOAM хранит матрицу системы линейных уравнений в формате LDU (Lower-Diagonal-Upper). Ненулевые коэффициенты хранятся в трёх плоских массивах:

- `lower[]` — элементы нижнего треугольника (по граням)
- `upper[]` — элементы верхнего треугольника (по граням)
- `diag[]` — диагональные элементы (по ячейкам)

Адресация — через `lduAddressing`, который хранит два массива индексов:

```cpp
// lduAddressing.H (OpenFOAM)
const labelList& lowerAddr() const;  // индекс ячейки-владельца грани
const labelList& upperAddr() const;  // индекс ячейки-соседа грани
```

Каждая грань `f` связывает ячейку `lowerAddr[f]` с ячейкой `upperAddr[f]`. Это и есть **косвенная адресация**: чтобы вычислить матрично-векторное произведение, нужно для каждой грани прочитать значения поля по двум произвольным индексам.

Типичный цикл матрично-векторного произведения (упрощённо):

```cpp
for (label face = 0; face < nInternalFaces; face++)
{
    label cellO = lowerAddr[face];  // косвенный адрес
    label cellN = upperAddr[face];  // косвенный адрес

    scalar flux = upper[face] * psi[cellN] - lower[face] * psi[cellO];
    //           ↑                ↑                ↑
    //           прямой доступ     КОСВЕННЫЙ         КОСВЕННЫЙ
}
```

Компилятор видит: на каждой итерации `cellO` и `cellN` — произвольные, заранее неизвестные. Нет unit-stride — нет векторизации.

### 1.2. Подтверждение из исследований

Презентация IXPUG (Intel Performance User Group) прямо отмечает для OpenFOAM на KNL:

> «KNL flag (-avx512) seems ineffective (see vectorization section)»
> «Effect of vectorization: GAMG — No speed up.»

Исследование по мини-приложению MG-CFD (модель OpenFOAM-подобного CFD-кода на неструктурированных сетках) подтверждает:

> «Once iflux exceeds 80 threads, it becomes fully memory-bound under AVX2 and AVX-512 with error falling to near-zero... These loops have the same computational structure: a single loop over edges, accumulating fluxes.»

---

## Часть 2. Что съедает 90% времени: GAMG и его сглаживатель

GAMG (Geometric Agglomerated Algebraic Multigrid) — основной решатель для уравнения давления в OpenFOAM. Каждый V-cycle состоит из умножения матрицы на вектор (SpMV), сглаживания (обычно Gauss-Seidel), ограничения и продолжения. На типичной задаче GAMG съедает 50–70% общего времени.

### 2.1. Gauss-Seidel: последовательный по природе

Реальный код из OpenFOAM v2312 (ESI fork):

```cpp
// GaussSeidelSmoother.C, строки 145-170
for (label celli=0; celli<nCells; celli++)
{
    fStart = fEnd;
    fEnd = ownStartPtr[celli + 1];

    psii = bPrimePtr[celli];

    // Накопление вклада от соседей (upper triangle)
    for (label facei=fStart; facei<fEnd; facei++)
    {
        psii -= upperPtr[facei]*psiPtr[uPtr[facei]];
        //                   ↑            ↑
        //                   прямой       КОСВЕННАЯ адресация
    }

    psii /= diagPtr[celli];  // ← ДЕЛЕНИЕ: выиграет от -no-prec-div

    // Распространение обновлённого значения соседям (lower triangle)
    for (label facei=fStart; facei<fEnd; facei++)
    {
        bPrimePtr[uPtr[facei]] -= lowerPtr[facei]*psii;
        //      ↑                           ↑
        //      КОСВЕННАЯ                   updated value from THIS cell
        //                                 ← FMA: a*b+c → одна инструкция
    }

    psiPtr[celli] = psii;
}
```

Три причины, почему этот код **не векторизуется и не параллелится напрямую**:

1. **Косвенная адресация** — `psiPtr[uPtr[facei]]` обращается к произвольным индексам. Нет unit-stride → нет автодовекторизации.

2. **Зависимость по данным** — Gauss-Seidel обновляет ячейку `celli` и сразу использует обновлённое значение для соседей. Ячейка `celli+1` зависит от результата `celli`. Это исключает SIMD-векторизацию.

3. **Гонка данных (data race)** — если несколько граней указывают на одну и ту же ячейку-соседа, параллельная запись даёт неопределённый результат. Презентация Fixstars прямо показывает это:

> «Dependency among face — Data race (write at the same time, different face). lduMatrix can not be parallelized.»

### 2.2. SpMV — та же проблема

```cpp
// lduMatrixATmul.C — упрощённо
for (label face=0; face<nFaces; face++)
{
    ApsiPtr[uPtr[face]] += lowerPtr[face]*psiPtr[lPtr[face]];
    ApsiPtr[lPtr[face]] += upperPtr[face]*psiPtr[uPtr[face]];
    //         ↑                              ↑
    //         КОСВЕННАЯ                       КОСВЕННАЯ
}
```

Прямая запись `#pragma omp parallel for` здесь **не работает**: разные грани могут писать в один и тот же `ApsiPtr[uPtr[face]]` — data race.

Fixstars решает это конверсией lduMatrix → CSR, где строки матрицы независимы:

```cpp
// CSR формат — параллельно безопасно
#pragma omp parallel for
for (label row=0; row<nRows; row++)
{
    double sum = 0.0;
    for (label idx=rowStart[row]; idxrowStart[row+1]; idx++)
    {
        sum += values[idx] * psi[colIdx[idx]];
    }
    Apsi[row] = sum;  // каждый поток пишет в свою строку — нет data race
}
```

Но OpenFOAM не использует CSR — он использует lduMatrix с косвенной адресацией.

---

## Часть 3. Почему OpenMP на Westmere догоняет AVX-512

### 3.1. Сравнение механизмов параллелизма

OpenMP в OpenFOAM v2306 распараллеливает циклы по ячейкам и граням **независимо от их векторизуемости**. Это принципиально другой механизм, чем SIMD-векторизация:

| Характеристика | SIMD (векторизация) | OpenMP (потоки) |
|---|---|---|
| Что ускоряет | Только векторизуемые циклы | Все циклы, где есть независимость по данным |
| GAMG (55% времени) | Бесполезен (косвенная адресация) | **Работает** — потоки делят строки матрицы |
| smoothSolver (35%) | Почти бесполезен (рекуррентность) | **Работает** — потоки делят ячейки по цветам |
| Явные поля (10%) | Работает | Работает |
| Покрытие кода | ~15–25% | **~80–90%** |

### 3.2. Что именно параллелит OpenMP

| Операция | SIMD (AVX-512) | OpenMP | Почему |
|---|---|---|---|
| SpMV (`Amul`) в lduMatrix | ❌ Data race | ⚠️ Требует CSR | Косвенная адресация, гонка записи |
| Gauss-Seidel smoothing | ❌ Зависимость по данным | ⚠️ Требует red-black | Последовательная природа |
| DIC preconditioner | ❌ Data race | ⚠️ Требует CSR | То же, что SpMV |
| WAXPBY (`y = αx + βy`) | ✅ Unit stride | ✅ `#pragma omp parallel for` | Прямой доступ |
| sumMag, sumProd | ⚠️ Reduction | ✅ `reduction(+:sum)` | Редукция |
| Restriction/prolongation | ⚠️ Косвенная | ✅ `#pragma omp parallel for` | Чтение по косвенному индексу, но запись прямая |

### 3.3. Red-black Gauss-Seidel: параллельная альтернатива

Fixstars (2019) показала на Intel KNL 13.5× ускорение pimpleFoam при 256 потоках. Их подход — разбиение ячеек на два цвета, каждый цвет обрабатывается параллельно:

```cpp
// Red-black Gauss-Seidel: параллельно
#pragma omp parallel for
for (label celli=0; celli<nCells; celli++)
{
    if (color[celli] == RED)
    {
        // Все RED ячейки независимы друг от друга
        psii = bPrimePtr[celli];
        for (label facei=fStart; facei<fEnd; facei++)
        {
            psii -= upperPtr[facei]*psiPtr[uPtr[facei]];
        }
        psiPtr[celli] = psii / diagPtr[celli];
    }
}
// Барьер
#pragma omp barrier
// Затем то же для BLACK ячеек
```

### 3.4. Почему Westmere + OpenMP ≈ Xeon Gold + AVX-512

| Фактор | Westmere (SSE4.2, 128-bit) | Xeon Gold (AVX-512, 512-bit) |
|---|---|---|
| SIMD-ширина | 2 double | 8 double |
| Автовекторизация lduMatrix | ❌ Нет (косвенная адресация) | ❌ Нет (косвенная адресация) |
| Gather/scatter penalty | Нет (gather-инструкций нет в SSE4.2) | Высокий (10–20 тактов на gather) |
| Downclocking | Нет | Да (30–40% на Skylake-SP) |
| OpenMP по ядрам | 6–12 ядер, линейное ускорение | 16–24 ядра, линейное ускорение |
| **Итоговый прирост от флагов** | **OpenMP: 4–10× (по ядрам)** | **AVX-512: 0–15% (от векторизации)** |

Прирост от `-march=westmere` (SSE4.2) и от `-xCORE-AVX512` в OpenFOAM **одинаково мал** — потому что SIMD-ширина не реализуется. А OpenMP даёт масштабирование по ядрам, которое **не зависит от SIMD-ширины**.

> **Примечание:** SSE4.2 не имеет gather-инструкций вообще (они появились в AVX2). Поэтому компилятор даже не пытается делать gather-векторизацию на Westmere — отсюда «нет penalty». На AVX-512 gather-инструкции есть, но медленные (10–20 тактов), и компилятор может попытаться их использовать, получая замедление вместо ускорения.

---

## Часть 4. Аудит флагов компиляции: ожидаемая производительность

### 4.1. Сводная карта: что даёт прирост и сколько

| | SIMD (вектор) | OpenMP (потоки) | Флаги FP | Итого компиляция |
|---|---|---|---|---|
| **AVX-512 (Xeon Gold)** | 10–15% | 0–5% | 3–8% | 23–33% (+8–12% ICX vs GCC) |
| **SSE4.2 (Westmere)** | 4% | 10–20% | 3–8% | 22–36% (+8–12% ICX vs GCC) |
| **Разрыв** | ~6–11% | ~10–15% | **~0%** | **~0–10%** |

Флаги FP-оптимизации дают **одинаковую пользу** на любой архитектуре — потому что они влияют на скалярный код (FMA, деление, constant folding), а не на SIMD-ширину.

### 4.2. Полный аудит флагов с ожидаемой производительностью

Ниже — анализ каждого флага по документации Intel и LLVM, с оценкой влияния на OpenFOAM. Проценты — это **ожидаемый прирост к общему времени** (не к отдельному циклу), с учётом того, что GAMG + smoothSolver = ~90% времени, а векторизуемые операции = ~10–15%.

#### ✅ Корректные флаги

| Флаг | Документация | Влияние на OpenFOAM | Оценка |
|---|---|---|---|
| `-O3` | Базовый уровень: loop unrolling (в ICX/LLVM), inline, scalar replacement | Включает все высокоуровневые оптимизации | Базовый (100%) |
| `-march=westmere` | Включает SSE4.2 (128-bit SIMD) | Минимальный SIMD-прирост: ~4% (косвенная адресация блокирует векторизацию) | +4% |
| `-fiopenmp` | Intel OpenMP runtime (LLVM-based) | Главный источник прироста: 10–20% при гибридной схеме MPI+OpenMP | +10–20% |
| `-fPIC` | Position-independent code, нужно для shared libs | Лёгкий overhead на использование регистра (1–2%), но нельзя убрать — OpenFOAM использует shared libs | Обязательно |
| `-pthread` | Threading support | Требуется для OpenMP и std::thread | Обязательно |

#### ⚠️ Узкое место №1: `-fp-model precise`

| Аспект | Деталь |
|---|---|
| **Документация Intel** | «Tells the compiler to strictly adhere to value-safe optimizations when implementing floating-point calculations. It disables optimizations that can change the result of floating-point calculations» |
| **Что блокирует** | Реассоциацию FP (перестановку слагаемых), деление через обратную величину (A/B → A*(1/B)), zero folding (X+0→X, X\*0→0), flush-to-zero (denormals), approximate sqrt, векторизацию редукций |
| **Что разрешает** | FMA (fused multiply-add) — **не отключается** precise. Constant folding (вычисление констант) — разрешён, использует округление по умолчанию. Reordering across calls — разрешён (компилятор предполагает default FP environment) |
| **Default ICX** | `-fp-model=fast` (равно `fast=1`) при `-O2` и выше. Текущая сборка **отключает дефолт** |
| **Влияние на OpenFOAM** | FMA используется в скалярном коде Gauss-Seidel (`bPrimePtr[u] -= lowerPtr[facei]*psii` — это FMA) и **остаётся разрешённым** с `-fp-model precise`. Реассоциация заблокирована, но в OpenFOAM нечего реассоциировать (нет векторизуемых редукций в горячих циклах). Деление не заменяется на recip. **Итог: потеря ~2–4%** (в основном от запрета деления через reciprocal и zero folding) |

> **Важно:** Документация Intel и Clang сходится: `-fp-model precise` **не отключает FMA**. Intel FP Consistency document: «FMA generation is enabled by default, not disabled by /fp:precise». Clang User's Manual: «precise ... FP contraction (FMA) is enabled (-ffp-contract=on)».

#### ⚠️ Узкое место №2: `-frounding-math`

| Аспект | Деталь |
|---|---|
| **Документация Clang/LLVM** | «The option -frounding-math forces the compiler to honor the dynamically-set rounding mode. This prevents optimizations which might affect results if the rounding mode changes or is different from the default; for example, it prevents floating-point operations from being reordered across most calls and prevents constant-folding when the result is not exactly representable» |
| **LLVM D62731** | Патч предлагал: «This will make frounding-math synonymous with fp-model=strict». Но разработчик LLVM возразил: «I don't think we want to *define* -frounding-math as exactly equivalent to -ffp-model=strict». В итоговой реализации они **не эквивалентны** |
| **Что блокирует дополнительно к `-fp-model precise`** | Constant folding (для неточно представимых результатов), reordering across calls. **Не блокирует FMA** — FMA контролируется отдельным флагом `-ffp-contract` и `-fp-model precise` его разрешает |
| **Что НЕ блокирует (вопреки распространённому мнению)** | FMA-контракцию. `-fp-model=strict` отключает FMA, но `-frounding-math` — нет. Для отключения FMA нужен `-fp-model=strict`, `-no-fma`, или `-ffp-contract=off` |
| **Эквивалент** | `-fp-model precise` + `-frounding-math` ≈ `-fp-model strict` **минус** disable FMA и **минус** exception behavior. Ближе к strict, чем к precise, но **не полный эквивалент** |
| **Влияние на OpenFOAM** | Constant folding: `constexpr double x = 1.0/6.0` не вычисляется (результат неточно представим). Reordering across calls: заблокирован. FMA: **остаётся разрешённым** (precise его не отключает, rounding-math тоже). **Дополнительная потеря: ~1–3%** (поверх `-fp-model precise`) — в основном от constant folding и reordering |

**Суммарная потеря от связки `-fp-model precise` + `-frounding-math` vs `-fp-model fast`: ~3–8%**

(Не ~5–10%, как можно было бы ожидать, потому что FMA **не блокируется** этой связкой — только `-fp-model strict` или `-no-fma` его отключает.)

```asm
; С -fp-model precise (FMA разрешён — precise его не отключает):
vfmadd231sd  xmm0, xmm1, xmm2    ; одна инструкция: a*b+c

; С -fp-model strict (FMA заблокирован — strict его отключает):
vmulsd       xmm0, xmm1, xmm2    ; умножение
vaddsd       xmm0, xmm0, xmm3    ; сложение — две инструкции

; С -fp-model precise + -frounding-math (FMA всё ещё разрешён!):
vfmadd231sd  xmm0, xmm1, xmm2    ; FMA используется — как и с precise alone
```

> **Ключевое различие:** `-fp-model=strict` (документация Intel): «Enables precise, disables contractions (FMA), and enables pragma stdc fenv_access». `-frounding-math` (документация Clang): только honors dynamic rounding mode — **не упоминает FMA**. `-ffp-model=strict` в Clang: «Enables -frounding-math and -ffp-exception-behavior=strict, and disables contractions (FMA)» — то есть strict = rounding-math + exception-behavior + no-FMA. `-frounding-math` — лишь **компонент** strict.

#### ❌ Отсутствует: `-no-prec-div`

| Аспект | Деталь |
|---|---|
| **Документация Intel** | «-no-prec-div ... the compiler may change floating-point division computations into multiplication by the reciprocal of the denominator. A/B is computed as A * (1/B)» |
| **Где в OpenFOAM** | `psii /= diagPtr[celli]` — деление на диагональ в каждой ячейке Gauss-Seidel. Тысячи делений за V-cycle |
| **Использование в бенчмарках** | SPEC HPC: `-O3 -no-prec-div -fp-model fast=2`. Intel `-fast` метафлаг: `-ipo -O3 -no-prec-div -static -fp-model fast=2 -xHost` |
| **Влияние на OpenFOAM** | Деление `x/y` занимает 3–5 тактов, `x*(1/y)` — 1 такт (умножение) + 1 такт (vrcpsd). В Gauss-Seidel деление — на каждой ячейке. **Оценка: ~2–5% прироста** |

#### ❌ Отсутствует: `-fp-model fast=2`

| Аспект | Деталь |
|---|---|
| **Документация Intel** | «fast=2 may produce faster and less accurate results». `fast=1` (дефолт): распознаёт NaN и Inf. `fast=2`: не распознаёт — «no NaN or infinite values will be used or produced» |
| **Что даёт** | Всё из `fast=1` (реассоциация, approximate reciprocals, flush-to-zero) **плюс** предположение об отсутствии NaN/Inf, что разблокирует дополнительные оптимизации |
| **Default ICX** | `-fp-model=fast` (равно `fast=1`) при `-O2` и выше. `fast=2` — ещё агрессивнее |
| **Использование** | Входит в метафлаг `-fast`: `-ipo -O3 -no-prec-div -static -fp-model fast=2 -xHost` (документация Intel для Linux) |
| **Влияние на OpenFOAM** | Реассоциация разрешает векторизацию редукций (10–15% кода). FMA разрешён полностью (он и так разрешён с precise, но fast=2 разрешает и reassociation). **Оценка: ~2–6% прироста** (ограничена тем, что 90% кода не векторизуется). Внимание: `fast=2` может нарушить сходимость — нужно проверять результаты |

#### ❌ Отсутствует: `-funroll-loops`

| Аспект | Деталь |
|---|---|
| **Документация GCC** | GCC **не включает** `-funroll-loops` на `-O3` — нужен явный флаг. На `-O3` включаются `-floop-unroll-and-jam` и `-fpeel-loops`, но не простая развёртка циклов |
| **ICX (LLVM-based)** | LLVM выполняет partial unrolling во время векторизации на `-O2` и выше. Но явный `-funroll-loops` может дополнительно помочь с короткими циклами |
| **Использование в бенчмарках** | Intel SPEC HPC2021: `-funroll-loops`. Все top-конфигурации ICX/LLVM/GCC включают его |
| **Влияние на OpenFOAM** | Gauss-Seidel: внутренний цикл по граням ячейки (обычно 4–8 граней) — слишком короткий для развёртки. SpMV: цикл по граням — может выиграть. **Оценка: ~1–3% прироста** |

> **Уточнение:** GCC на `-O3` включает `-floop-unroll-and-jam` (развёртка внешнего цикла в гнезде циклов с последующим слиянием внутренних) и `-fpeel-loops` (отщепление итераций), но **не** включает `-funroll-loops` (простая развёртка с известным числом итераций). LLVM/Clang делает partial unrolling как часть векторизации, но также не включает полный `-funroll-loops` по умолчанию.

#### ⚠️ Можно улучшить: `-std=c++14` → `-std=c++17`

| Аспект | Деталь |
|---|---|
| **OpenFOAM v2306** | Требует C++14, поддерживает C++17 |
| **OpenFOAM v2606+** | Требует C++17 |
| **Что даёт C++17** | Guaranteed copy elision, `constexpr if`, inline variables — меньше runtime overhead. Но прямой эффект на численные циклы минимален |
| **Влияние на OpenFOAM** | Косвенный эффект: лучше inline, меньше временных объектов. **Оценка: ~0–2%** |

### 4.3. Откуда берётся `-frounding-math`

> **Важно:** OpenFOAM **не добавляет** `-frounding-math` в стандартные `wmake`-правила для GCC на x86_64. Ни ESI fork, ни Foundation fork не используют его в `linux64Gcc/c++Opt` — там стоит просто `-O3`.

Однако `-frounding-math` может появиться в сборке через:

1. **CGAL-зависимые утилиты** — `surfaceFeatureExtract/Allwmake` явно передаёт `EXE_FROUNDING_MATH=-frounding-math`, потому что CGAL использует динамическое изменение режима округления.

2. **Ручное добавление** — если пользователь или администратор добавил `-frounding-math` в `c++Opt` или в переменные окружения (`CFLAGS`, `CXXFLAGS`).

3. **Другие архитектуры** — в `linuxARM64Arm/c++Opt` (ESI fork) используется `-ffast-math`, который включает `-fno-rounding-math` (обратный флаг).


