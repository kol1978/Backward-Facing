# Сборка OpenFOAM с Intel icpx и OpenMP: анализ производительности

![OpenFOAM](https://img.shields.io/badge/OpenFOAM-v2306%20%7C%20v2606-blue)
![Compiler](https://img.shields.io/badge/Intel%20icpx-oneAPI-orange)
![OpenMP](https://img.shields.io/badge/OpenMP--fiopenmp-red)

---

## SIMD vs OpenMP: почему `-fiopenmp` на Westmere работает

### Сравнение механизмов параллелизма

OpenMP в OpenFOAM v2306 распараллеливает циклы по ячейкам и граням **независимо от их векторизуемости**. Это принципиально другой механизм, чем SIMD-векторизация:

| Характеристика | SIMD (векторизация) | OpenMP (потоки) |
|---|---|---|
| Что ускоряет | Только векторизуемые циклы | Все циклы, где есть независимость по данным |
| GAMG (55% времени) | Бесполезен (косвенная адресация) | **Работает** — потоки делят строки матрицы |
| smoothSolver (35%) | Почти бесполезен (рекуррентность) | **Работает** — потоки делят ячейки по цветам |
| Явные поля (10%) | Работает | Работает |
| Покрытие кода | ~15–25% | **~80–90%** |

При 10 MPI на 24 ядрах — 2.4 ядра на процесс. OpenMP задействует свободные ~14 ядер для параллельного выполнения внутренних циклов каждого MPI-процесса.

**Реальный прирост от OpenMP: ~10–20%** (зависит от quality of NUMA binding).

---

### Сводная карта: что даёт прирост и сколько

| | SIMD (вектор) | OpenMP (потоки) | Итого компиляция |
|---|---|---|---|
| **AVX-512 (Xeon Gold)** | 10–15% | 0–5% | 15–20% (+8–12% ICX vs GCC) = 25–35% |
| **SSE4.2 (Westmere)** | 4% | 10–20% | 14–24% (+8–12% ICX vs GCC) = 22–36% |
| **Разрыв** | ~6–11% | ~10–15% | **~0–10%** |

**Вывод:** На идеальных циклах AVX-512 рвёт SSE4.2 в 2–4×. Но в OpenFOAM v2306 90% времени съедают GAMG и smoothSolver — операции с косвенной адресацией и рекуррентными зависимостями, которые не векторизуются ни одним SIMD. Поэтому SIMD-преимущество AVX-512 над SSE4.2 в OpenFOAM — всего ~6–11%.

**OpenMP на Westmere** компенсирует этот разрыв за счёт того, что он параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге `-march=westmere -fiopenmp` дают примерно тот же прирост, что `-xCORE-AVX512` на Xeon Gold — не потому что Westmere хорош, а потому что OpenFOAM плохо векторизуется, и SIMD-ширина почти не matter.

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

PETSc-разработчики также отмечают для SpMV с AVX-512:

> «CSR is not the optimal choice for matrices whose number of nonzeros per row is either small or not a multiple of the length of the CPU vector register, which are common in the PDE regime.»

---

## Часть 2. `-frounding-math` — главный сюрприз

### 2.1. Что делает флаг

`-frounding-math` сообщает компилятору: **не предполагай режим округления по умолчанию (round-to-nearest)**. Программа может изменить режим округления во время выполнения через `fesetround()`, поэтому компилятор не имеет права:

- Выполнять свёртку констант (constant folding) для выражений с плавающей точкой
- Переставлять операции, если результат может измениться при другом режиме округления
- Заменять деление на умножение (на обратную величину)
- Объединять умножение и сложение в FMA, если это может изменить результат при нестандартном округлении

```bash
# GCC по умолчанию: -fno-rounding-math
#   → предполагается round-to-nearest
#   → разрешены все оптимизации

# Явно включено: -frounding-math
#   → режим округления неизвестен
#   → запрещены многие оптимизации, включая часть векторизации
```

### 2.2. Где встречается в OpenFOAM

В репозитории OpenFOAM (Foundation fork, openfoam.org) `-frounding-math` используется для сборки `surfaceFeatureExtract` с поддержкой CGAL:

```makefile
# OpenFOAM-2.2.x/applications/utilities/surface/surfaceFeatureExtract/Allwmake
EXE_FROUNDING_MATH=-frounding-math
```

CGAL требует `-frounding-math`, потому что использует динамическое изменение режима округления для точной геометрической арифметики.

### 2.3. Состояние `wmake`-правил в обоих форках

**Важно:** Ни ESI fork, ни Foundation fork **не используют** `-ffast-math` в стандартных правилах `linux64Gcc/c++Opt`:

```makefile
# wmake/rules/linux64Gcc/c++Opt (ESI, v2212+)
c++OPT = -O3
# Нет -ffast-math, нет -frounding-math
```

```makefile
# wmake/rules/linux64Gcc/c++Opt (Foundation, openfoam.org)
c++OPT = -O3
# Тоже без -ffast-math
```

```makefile
# wmake/rules/linux64Clang/c++Opt (Foundation, Clang)
c++OPT = -O3
# Тоже без -ffast-math
```

Флаг `-ffast-math` появляется только в архитектурно-специфичных правилах — например, `linuxARM64Arm/c++Opt` в ESI fork:

```makefile
# wmake/rules/linuxARM64Arm/c++Opt (ESI)
c++OPT = -ffp-contract=fast -ffast-math -O3 -funsafe-math-optimizations -fsimdmath -armpl
```

**Вывод:** На стандартной x86_64 + GCC сборке OpenFOAM не использует ни `-ffast-math`, ни `-frounding-math` в основных правилах. Риск возникает только при сборке CGAL-зависимых утилит (где `-frounding-math` добавляется локально) или при ручном изменении `c++Opt`.

### 2.4. Сюрприз: `-frounding-math` убивает векторизацию

Если `-frounding-math` попадает в флаги (через CGAL-зависимые утилиты или ручную правку), компилятор:

1. **Не использует FMA** — `vfmadd231pd` заменяется на отдельные `vmulpd` + `vaddpd`, что вдвое медленнее.

2. **Не векторизует редукции** — циклы вида `sum += a[i] * b[i]` не могут быть векторизованы, потому что порядок сложений может изменить результат при разных режимах округления. Компилятор генерирует скалярный код.

3. **Не выполняет constant folding** — даже `constexpr double x = 1.0 / 6.0;` не вычисляется на этапе компиляции.

4. **Не заменяет деление** — `x / y` остаётся делением, а не умножением на `1/y`, что на 3–5 тактов медленнее.

Для CFD, где 70–90% времени — это циклы над гранями/ячейками с умножением и сложением, потеря FMA и векторизации означает **потерю всего потенциала AVX-512**.

---

## Часть 3. Дополнительный фактор: downclocking

На процессорах Skylake-SP (Xeon Gold/Platinum) использование 512-битных инструкций вызывает **снижение тактовой частоты**:

| Уровень | Условие | Снижение частоты |
|---|---|---|
| L0 | Нет AVX-инструкций | 100% (базовый турбо) |
| L1 | 256-битные «тяжёлые» FP-инструкции | ~85% от максимума |
| L2 | 512-битные «тяжёлые» FP-инструкции | ~60–70% от максимума |

Если компилятор изредка вставляет AVX-512 инструкции, но основная масса кода остаётся скалярной — процессор снижает частоту, **а выигрыша от векторизации нет**. Итог: код работает **медленнее**, чем без AVX-512.

GCC начиная с версии 9 по умолчанию использует `-mprefer-vector-width=256` для Skylake-AVX512 — именно по этой причине.

На AMD EPYC 9004 (Genoa, Zen 4) 512-битные инструкции **не вызывают downclocking**, и AVX-512 даёт заметный прирост. Но и там `lduAddressing` ограничивает выигрыш — просто нет штрафа за downclocking.

---

## Часть 4. Что съедает 90% времени: GAMG и его сглаживатель

GAMG (Geometric Agglomerated Algebraic Multigrid) — основной решатель для уравнения давления в OpenFOAM. Каждый V-cycle состоит из умножения матрицы на вектор (SpMV), сглаживания (обычно Gauss-Seidel), ограничения и продолжения. На типичной задаче GAMG съедает 50–70% общего времени.

### 4.1. Gauss-Seidel: последовательный по природе

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

    psii /= diagPtr[celli];

    // Распространение обновлённого значения соседям (lower triangle)
    for (label facei=fStart; facei<fEnd; facei++)
    {
        bPrimePtr[uPtr[facei]] -= lowerPtr[facei]*psii;
        //      ↑                           ↑
        //      КОСВЕННАЯ                   updated value from THIS cell
    }

    psiPtr[celli] = psii;
}
```

Три причины, почему этот код **не векторизуется и не параллелится напрямую**:

1. **Косвенная адресация** — `psiPtr[uPtr[facei]]` обращается к произвольным индексам. Нет unit-stride → нет автодовекторизации.

2. **Зависимость по данным** — Gauss-Seidel обновляет ячейку `celli` и сразу использует обновлённое значение для соседей. Ячейка `celli+1` зависит от результата `celli`. Это исключает SIMD-векторизацию.

3. **Гонка данных (data race)** — если несколько граней указывают на одну и ту же ячейку-соседа, параллельная запись даёт неопределённый результат. Презентация Fixstars прямо показывает это:

> «Dependency among face — Data race (write at the same time, different face). lduMatrix can not be parallelized.»

### 4.2. SpMV — та же проблема

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
    for (label idx=rowStart[row]; idx<rowStart[row+1]; idx++)
    {
        sum += values[idx] * psi[colIdx[idx]];
    }
    Apsi[row] = sum;  // каждый поток пишет в свою строку — нет data race
}
```

Но OpenFOAM не использует CSR — он использует lduMatrix с косвенной адресацией.

---

## Часть 5. Почему OpenMP на Westmere догоняет AVX-512

### 5.1. Что именно параллелит OpenMP

OpenMP распараллеливает **циклы по ячейкам и граням** — те самые циклы, которые SIMD не может векторизовать:

| Операция | SIMD (AVX-512) | OpenMP | Почему |
|---|---|---|---|
| SpMV (`Amul`) в lduMatrix | ❌ Data race | ⚠️ Требует CSR | Косвенная адресация, гонка записи |
| Gauss-Seidel smoothing | ❌ Зависимость по данным | ⚠️ Требует red-black | Последовательная природа |
| DIC preconditioner | ❌ Data race | ⚠️ Требует CSR | То же, что SpMV |
| WAXPBY (`y = αx + βy`) | ✅ Unit stride | ✅ `#pragma omp parallel for` | Прямой доступ |
| sumMag, sumProd | ⚠️ Reduction | ✅ `reduction(+:sum)` | Редукция |
| Restriction/prolongation | ⚠️ Косвенная | ✅ `#pragma omp parallel for` | Чтение по косвенному индексу, но запись прямая |

### 5.2. Как Fixstars параллелит DIC-PCG с OpenMP

Fixstars (2019) показала на Intel KNL 13.5× ускорение pimpleFoam при 256 потоках. Их подход:

1. **Конверсия lduMatrix → CSR** для SpMV — убирает data race
2. **Параллельный Gauss-Seidel через red-black** — разбиение ячеек на два цвета, каждый цвет обрабатывается параллельно
3. **Параллельная редукция** для `sumMag`/`sumProd`

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

### 5.3. Почему Westmere + OpenMP ≈ Xeon Gold + AVX-512

| Фактор | Westmere (SSE4.2, 128-bit) | Xeon Gold (AVX-512, 512-bit) |
|---|---|---|
| SIMD-ширина | 2 double | 8 double |
| Автовекторизация lduMatrix | ❌ Нет (косвенная адресация) | ❌ Нет (косвенная адресация) |
| Gather/scatter penalty | Нет (SSE4.2 gather примитивен) | Высокий (10–20 тактов на gather) |
| Downclocking | Нет | Да (30–40% на Skylake-SP) |
| OpenMP по ядрам | 6–12 ядер, линейное ускорение | 16–24 ядра, линейное ускорение |
| **Итоговый прирост от флагов** | **OpenMP: 4–10× (по ядрам)** | **AVX-512: 0–15% (от векторизации)** |

Прирост от `-march=westmere` (SSE4.2) и от `-xCORE-AVX512` в OpenFOAM **одинаково мал** — потому что SIMD-ширина не реализуется. А OpenMP даёт масштабирование по ядрам, которое **не зависит от SIMD-ширины**.

---

## Часть 6. Что всё-таки помогает

### 6.1. `restrict` и `__builtin_assume_aligned`

Исследование Zirwes et al. по векторизации химической кинетики в OpenFOAM показало:

> «Using `restrict` and `__builtin_assume_aligned` for the function arguments enables gcc and icpc to generate a vectorized version of the loop... reduces computation times for chemical reaction rates by up to 50% and total simulation times by up to 25%.»

Разница в ассемблере — наглядная:

```asm
; Без restrict: скалярный код
addsd   xmm0, [rdi+rax*8]     ; одно сложение за такт

; С restrict: векторизованный код
vaddpd   zmm0, zmm0, [rdi+rax*8]  ; 8 сложений за такт (AVX-512)
```

Проблема: OpenFOAM не использует `restrict` в основных циклах `lduMatrix` — потому что структура данных (косвенная адресация) делает алиасинг неизбежным.

### 6.2. Блочно-структурированные сетки

Для структурированных сеток (где `lowerAddr` и `upperAddr` имеют предсказуемые шаблоны) возможна ручная векторизация:

```cpp
// Структурированная сетка: cellN = cellO + nx (постоянный stride)
#pragma omp simd
for (label i = 0; i < nCells; i++)
{
    result[i] = a[i] * x[i] + b[i] * x[i + nx];
    //         ↑       ↑       ↑       ↑
    //         прямой  прямой  прямой  прямой — unit stride!
}
```

### 6.3. Block-матрицы (foam-extend)

foam-extend реализует `BlockLduMatrix` для coupled-решателей. Если block-матрица хранит 3×3 блоки в row-major порядке, то операции внутри блока — **прямая адресация**, и AVX-512 может векторизовать:

```cpp
// Block 3x3, row-major — unit stride для AVX-512
#pragma omp simd
for (label i=0; i<nBlocks; i++)
{
    result[i*9+0] = a[i*9+0]*x[0] + a[i*9+1]*x[1] + a[i*9+2]*x[2];
    result[i*9+3] = a[i*9+3]*x[0] + a[i*9+4]*x[1] + a[i*9+5]*x[2];
    // ...
}
```

### 6.4. SoA layout + CSR

SELL-C-σ (Slice-ELL with Chunked-σ ordering) — формат для SpMV на широких SIMD-юнитах. Исследования показывают 2–4× ускорение AVX-512 на CSR/SELL-C-σ. Но для этого нужно конверсия `lduMatrix` → CSR, перепись всех solvers/preconditioners и разработка нового multigrid.

### 6.5. Компиляторные флаги: правильная комбинация

Для Intel ICX (`icpx`) оптимальный набор:

```bash
# Оптимизация: включаем FMA, запрещаем -frounding-math
-O3 -fiopenmp -fp-model=precise -fno-rounding-math

# Архитектура: таргетим AVX-512, но не позволяем компилятору
# вставлять 512-битные инструкции где попало
-xSKYLAKE-AVX512

# Альтернатива: ограничить до 256-бит (если downclocking перевешивает)
-mprefer-vector-width=256  # GCC
```

---

## Часть 7. Сводная таблица факторов

| Фактор | Влияние на AVX-512 в OpenFOAM | Можно исправить? |
|---|---|---|
| `lduAddressing` (косвенная адресация) | Главная причина — нет unit-stride, нет векторизации | Только переписывание структуры данных |
| Gather/scatter (медленные на Skylake) | 10–20 тактов на загрузку вместо 1–2 | Нет, аппаратное ограничение |
| `-frounding-math` | Запрещает FMA, constant folding, векторизацию редукций | Да: использовать `-fno-rounding-math` или не добавлять `-frounding-math` |
| Downclocking (Skylake-SP) | Снижение частоты на 30–40% при AVX-512 | Да: `-mprefer-vector-width=256` или переход на Ice Lake / AMD |
| Отсутствие `restrict` | Компилятор не может доказать отсутствие алиасинга | Да: ручная аннотация (но требует изменения исходного кода) |
| Неструктурированные сетки | Непредсказуемый паттерн доступа к памяти | Структурные сетки или блочные форматы (SELL-C-σ) |

---

## Часть 8. Итоговый вывод

```
Производительность OpenFOAM
  │
  │   OpenMP (по ядрам)
  │   ┌─────────────────────────┐
  │   │                         │
  │   │  4–10× ускорение        │
  │   │  (линейное масштаб.)    │
  │   │                         │
  │   └─────────────────────────┘
  │
  │   AVX-512 (SIMD)
  │   ┌─────┐
  │   │0–15%│ ← lduAddressing не даёт векторизовать
  │   └─────┘
  │
  └──────────────────────────────────→
       Westmere      Xeon Gold
       (SSE4.2)      (AVX-512)
```

AVX-512 в OpenFOAM даёт 10–15%, а не 2× — потому что **90% времени уходит на циклы, которые невозможно векторизовать из-за косвенной адресации `lduAddressing`**. Это не баг, а следствие архитектуры, заточенной под неструктурированные сетки.

Флаг `-frounding-math` усугубляет ситуацию, запрещая FMA и векторизацию редукций. В стандартных `wmake`-правилах обоих форков (ESI и Foundation) на x86_64 + GCC он **отсутствует** — но при сборке CGAL-зависимых утилит может попасть в флаги локально.

**OpenMP на Westmere компенсирует разрыв** за счёт того, что он параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге `-march=westmere -fiopenmp` дают примерно тот же прирост, что `-xCORE-AVX512` на Xeon Gold.

Единственное, где AVX-512 реально оторвался бы — переписать GAMG под структуры данных без косвенной адресации (block-матрицы, SoA layout, CSR). Но это уже не OpenFOAM, а исследовательский код.

---

### Источники

- [Intel: Tuning SIMD Vectorization for Xeon Scalable](https://www.intel.com/content/www/us/en/developer/articles/technical/tuning-simd-vectorization-when-targeting-intel-xeon-processor-scalable-family.html)
- [IXPUG: OpenFOAM on Knights Landing (AVX-512 ineffective)](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf)
- [Fixstars: Thread-Parallelism Challenge on OpenFOAM](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257)
- [MG-CFD mini-app: memory-bound under AVX2/AVX-512](https://eprints.whiterose.ac.uk/id/eprint/148034/8/cpe.5443.pdf)
- [Zhang: Vectorized SpMV AVX-512](https://openearthscience.org/~rmills/pubs/Zhang-2018-vectorized-SpMV-AVX512.pdf)
- [Zirwes: Vectorization of chemistry in OpenFOAM](https://www.academia.edu/124915224/)
- [OpenFOAM-2.2.x: surfaceFeatureExtract Allwmake with -frounding-math](https://github.com/OpenFOAM/OpenFOAM-2.2.x/blob/master/applications/utilities/surface/surfaceFeatureExtract/Allwmake)
- [OpenFOAM-dev: linux64Clang/c++Opt (just -O3)](https://github.com/OpenFOAM/OpenFOAM-dev/blob/master/wmake/rules/linux64Clang/c%2B%2BOpt)
- [OpenFOAM ESI Issue #3336: Arm c++Opt with -ffast-math](https://develop.openfoam.com/Development/openfoam/-/issues/3336)
- [AUR openfoam-com: compilation log v2212 (no -ffast-math for linux64Gcc)](https://aur.archlinux.org/packages/openfoam-com?O=10)
- [Habr: -ffast-math in GCC 11](https://habr.com/ru/companies/ruvds/articles/586386/)
- [Krister Walfridsson: Optimizations enabled by -ffast-math](https://kristerw.github.io/2021/10/19/fast-math/)
- [StackOverflow: AVX-512 decreases performance](https://stackoverflow.com/questions/63484266/enabling-avx512-support-on-compilation-significantly-decreases-performance)
- [Phoronix: AMD EPYC AVX-512 benefits for OpenFOAM](https://www.phoronix.com/review/amd-epyc-avx512/9)
- [Habr: Skylake-SP AVX-512 downclocking](https://habr.com/ru/companies/yadro/articles/779284/)

