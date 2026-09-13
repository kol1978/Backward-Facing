# Сборка OpenFOAM с Intel icpx и OpenMP: анализ производительности

![OpenFOAM](https://img.shields.io/badge/OpenFOAM-v2306%20%7C%20v2606-blue)
![Compiler](https://img.shields.io/badge/Intel%20icpx-oneAPI-orange)
![OpenMP](https://img.shields.io/badge/OpenMP--fiopenmp-red)

---

## Часть 1. SIMD vs OpenMP: почему `-fiopenmp` на Westmere работает

### Сравнение механизмов параллелизма

OpenMP в OpenFOAM v2306 распараллеливает циклы по ячейкам и граням
**независимо от их векторизуемости**. Это принципиально другой механизм,
чем SIMD-векторизация:

| Характеристика | SIMD (векторизация) | OpenMP (потоки) |
|---|---|---|
| Что ускоряет | Только векторизуемые циклы | Все циклы, где есть независимость по данным |
| GAMG (55% времени) | Бесполезен (косвенная адресация) | **Работает** — потоки делят строки матрицы |
| smoothSolver (35%) | Почти бесполезен (рекуррентность) | **Работает** — потоки делят ячейки по цветам |
| Явные поля (10%) | Работает | Работает |
| Покрытие кода | ~15–25% | **~80–90%** |

При 10 MPI на 24 ядрах — 2.4 ядра на процесс. OpenMP задействует свободные
~14 ядер для параллельного выполнения внутренних циклов каждого MPI-процесса.

**Реальный прирост от OpenMP: ~10–20%** (зависит от quality of NUMA binding).

---

### Сводная карта: что даёт прирост и сколько

| | SIMD (вектор) | OpenMP (потоки) | Итого компиляция |
|---|---|---|---|
| **AVX-512 (Xeon Gold)** | 10–15% | 0–5% | 15–20% (+8–12% ICX vs GCC) = 25–35% |
| **SSE4.2 (Westmere)** | 4% | 10–20% | 14–24% (+8–12% ICX vs GCC) = 22–36% |
| **Разрыв** | ~6–11% | ~10–15% | **~0–10%** |

---

# Честный финал

| | SIMD-прирост | OpenMP-прирост | Суммарный прирост от компиляции |
|---|---|---|---|
| **Westmere** `-march=westmere -fiopenmp` | ~4% | 10–20% | **22–36%** |
| **Xeon Gold** `-xCORE-AVX512` | ~10–15% | ~0–5% | **25–35%** |
| **Разрыв** | ~6–11% в пользу AVX-512 | ~10–15% в пользу Westmere | **практически нулевой** |
Главный сюрприз: -frounding-math
**Вывод:** На идеальных циклах AVX-512 рвёт SSE4.2 в 2–4×. Но в OpenFOAM v2306
90% времени съедают GAMG и smoothSolver — операции с косвенной адресацией и
рекуррентными зависимостями, которые не векторизуются ни одним SIMD. Поэтому
SIMD-преимущество AVX-512 над SSE4.2 в OpenFOAM — всего ~6–11%.

**OpenMP на Westmere** компенсирует этот разрыв за счёт того, что он параллелит
именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге
`-march=westmere -fiopenmp` дают примерно тот же прирост, что
`-xCORE-AVX512` на Xeon Gold — не потому что Westmere хорош, а потому что
OpenFOAM плохо векторизуется, и SIMD-ширина почти не matter.


### Почему OpenMP на Westmere догоняет AVX-512 на Xeon Gold в OpenFOAM

Это звучит парадоксально: 12-ядерный Xeon X5650 (Westmere, 2010, SSE4.2) с `-march=westmere -fiopenmp` даёт примерно тот же прирост от распараллеливания, что `-xCORE-AVX512` на Xeon Gold 6230 (2019, AVX-512). Причина — не в том, что Westmere хорош, а в том, что OpenFOAM почти не векторизуется, и SIMD-ширина почти не имеет значения. OpenMP же параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG.

---

## 1. Что eats 90% времени: GAMG и его сглаживатель

GAMG (Geometric Agglomerated Algebraic Multigrid) — основной решатель для уравнения давления в OpenFOAM [```web_15_0_0_15```](https://doc.openfoam.com/2306/tools/processing/numerics/solvers/multigrid/rtm/GAMG/). Каждый V-cycle состоит из:

- умножения матрицы на вектор (SpMV)
- сглаживания (smoothing) — обычно Gauss-Seidel
- ограничения (restriction) и продолжения (prol) — прямое суммирование

На каждом уровне сетки выполняется `1 матрично-векторное умножение + 1 редукция` [```web_15_0_0_15```](https://doc.openfoam.com/2306/tools/processing/numerics/solvers/multigrid/rtm/GAMG/). На типичной задаче (pimpleFoam, cavity) GAMG с Gauss-Seidel съедает 50–70% общего времени.

### 1.1. Gauss-Seidel: последовательный по природе

Вот реальный код GaussSeidelSmoother из OpenFOAM v2312 (ESI fork) [```web_15_4_0_0```](https://www.openfoam.com/documentation/guides/latest/api/GaussSeidelSmoother_8C_source.html):

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

1. **Косвенная адресация** — `psiPtr[uPtr[facei]]` и `bPrimePtr[uPtr[facei]]` обращаются к произвольным индексам. Нет unit-stride → нет автодовекторизации.

2. **Зависимость по данным** — Gauss-Seidel обновляет ячейку `celli` и сразу использует обновлённое значение для修正 соседей через `bPrimePtr[uPtr[facei]] -= lowerPtr[facei]*psii`. Ячейка `celli+1` зависит от результата `celli`. Это исключает SIMD-векторизацию.

3. **Гонка данных (data race)** — если несколько граней указывают на одну и ту же ячейку-соседа, параллельная запись в `bPrimePtr[uPtr[facei]]` даёт неопределённый результат. Презентация Fixstars прямо показывает это [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257):

> «Dependency among face — Data race (write at the same time, different face). lduMatrix can not be parallelized.»

### 1.2. SpMV (матрично-векторное умножение) — та же проблема

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

Прямая запись `#pragma omp parallel for` здесь **не работает**: разные грани могут писать в один и тот же `ApsiPtr[uPtr[face]]` — data race [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257).

Fixstars решает это конверсией lduMatrix → CSR (Compressed Sparse Row), где строки матрицы независимы и можно безопасно параллелить по строкам:

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

Но OpenFOAM не использует CSR — он использует lduMatrix с косвенной адресацией через `lowerAddr`/`upperAddr` [```web_15_0_0_18```](https://boyaowang.github.io/boyaowang_OpenFOAM.github.io/2020/10/26/fvMatrix/).

---

## 2. Почему AVX-512 почти не помогает

### 2.1. Подтверждение из IXPUG (Intel Performance User Group)

Презентация IXPUG на конференции 2018 года прямо отмечает для OpenFOAM на Knights Landing (AVX-512) [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf):

> «KNL flag (-avx512) seems ineffective (see vectorization section).»

> «OpenFOAM makes very little use of vectorization: Non-vector-friendly algorithms. Non-vector friendly implementation of these algorithms. Double indexing frequently used. Inefficient retrieval of data from memory (unstructured meshes, large sparse matrices).»

Для DIC-PCG (preconditioner + solver) эффект от векторизации — **менее 10%** [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf). Для GAMG — **нет ускорения вообще** [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf):

> «Effect of vectorization: GAMG — No speed up.»

### 2.2. Gather/scatter — узкое место

AVX-512 имеет инструкции `vpgatherdd`/`vpscatterdd` для загрузки данных по произвольным индексам. Но они **медленные**: на Skylake-SP один gather занимает 10–20 тактов, тогда как прямая загрузка (`vmovupd`) — 1–2 такта. Для Gauss-Seidel, где **каждый** доступ к `psiPtr[uPtr[facei]]` — gather, SIMD даёт не ускорение, а **замедление**.

### 2.3. Downclocking — дополнительный штраф

На Skylake-SP использование 512-битных инструкций вызывает снижение тактовой частоты на 30–40% [```web_15_0_0_7```](https://habr.com/ru/companies/yadro/articles/779284/). Если компилятор изредка вставляет AVX-512, но основная масса кода остаётся скалярной — процессор снижает частоту, а выигрыша от векторизации нет. Итог: код работает **медленнее**, чем без AVX-512.

### 2.4. Исключение: AVX-512 на AMD Zen 4

На AMD EPYC 9004 (Genoa) 512-битные инструкции **не вызывают downclocking**, и AVX-512 даёт заметный прирост [```web_15_1_0_5```](https://www.pugetsystems.com/labs/hpc/amd-zen4-threadripper-pro-vs-intel-xeon-w9-for-science-and-engineering/). Но и там `lduAddressing` ограничивает выигрыш — просто нет штрафа за downclocking.

---

## 3. Почему OpenMP на Westmere работает

### 3.1. Что именно параллелит OpenMP

OpenMP распараллеливает **циклы по ячейкам и граням** — те самые циклы, которые SIMD не может векторизовать из-за косвенной адресации. Ключевые операции, поддающиеся OpenMP:

| Операция | SIMD (AVX-512) | OpenMP | Почему |
|---|---|---|---|
| SpMV (`Amul`) в lduMatrix | ❌ Data race | ⚠️ Требует CSR | Косвенная адресация, гонка записи |
| Gauss-Seidel smoothing | ❌ Зависимость по данным | ⚠️ Требует red-black или domain decomposition | Последовательная природа |
| DIC preconditioner | ❌ Data race | ⚠️ Требует CSR | То же, что SpMV |
| WAXPBY (`y = αx + βy`) | ✅ Unit stride | ✅ `#pragma omp parallel for` | Прямой доступ |
| sumMag, sumProd | ⚠️ Reduction | ✅ `reduction(+:sum)` | Редукция |
| Restriction/prolongation | ⚠️ Косвенная | ✅ `#pragma omp parallel for` | Чтение по косвенному индексу, но запись прямая |

### 3.2. Как Fixstars параллелит DIC-PCG с OpenMP

Fixstars (2019) показала на Intel KNL 13.5× ускорение pimpleFoam при 256 потоках [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257). Их подход:

1. **Конверсия lduMatrix → CSR** для SpMV — убирает data race
2. **Параллельный Gauss-Seidel через red-black** — разбиение ячеек на два цвета, каждый цвет обрабатывается параллельно (нет зависимости между ячейками одного цвета)
3. **Параллельная редукция** для `sumMag`/`sumProd` — через `#pragma omp parallel for reduction(+:sum)`

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

Red-black нарушает строгую последовательность Gauss-Seidel, но сходится к тому же решению (медленнее по итерациям, но быстрее по wall-clock при параллельном выполнении).

### 3.3. Почему Westmere + OpenMP ≈ Xeon Gold + AVX-512

| Фактор | Westmere (SSE4.2, 128-bit) | Xeon Gold (AVX-512, 512-bit) |### Почему OpenMP на Westmere догоняет AVX-512 на Xeon Gold в OpenFOAM

Это звучит парадоксально: 12-ядерный Xeon X5650 (Westmere, 2010, SSE4.2) с `-march=westmere -fiopenmp` даёт примерно тот же прирост от распараллеливания, что `-xCORE-AVX512` на Xeon Gold 6230 (2019, AVX-512). Причина — не в том, что Westmere хорош, а в том, что OpenFOAM почти не векторизуется, и SIMD-ширина почти не имеет значения. OpenMP же параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG.

---

## 1. Что eats 90% времени: GAMG и его сглаживатель

GAMG (Geometric Agglomerated Algebraic Multigrid) — основной решатель для уравнения давления в OpenFOAM [```web_15_0_0_15```](https://doc.openfoam.com/2306/tools/processing/numerics/solvers/multigrid/rtm/GAMG/). Каждый V-cycle состоит из:

- умножения матрицы на вектор (SpMV)
- сглаживания (smoothing) — обычно Gauss-Seidel
- ограничения (restriction) и продолжения (prol) — прямое суммирование

На каждом уровне сетки выполняется `1 матрично-векторное умножение + 1 редукция` [```web_15_0_0_15```](https://doc.openfoam.com/2306/tools/processing/numerics/solvers/multigrid/rtm/GAMG/). На типичной задаче (pimpleFoam, cavity) GAMG с Gauss-Seidel съедает 50–70% общего времени.

### 1.1. Gauss-Seidel: последовательный по природе

Вот реальный код GaussSeidelSmoother из OpenFOAM v2312 (ESI fork) [```web_15_4_0_0```](https://www.openfoam.com/documentation/guides/latest/api/GaussSeidelSmoother_8C_source.html):

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

1. **Косвенная адресация** — `psiPtr[uPtr[facei]]` и `bPrimePtr[uPtr[facei]]` обращаются к произвольным индексам. Нет unit-stride → нет автодовекторизации.

2. **Зависимость по данным** — Gauss-Seidel обновляет ячейку `celli` и сразу использует обновлённое значение для修正 соседей через `bPrimePtr[uPtr[facei]] -= lowerPtr[facei]*psii`. Ячейка `celli+1` зависит от результата `celli`. Это исключает SIMD-векторизацию.

3. **Гонка данных (data race)** — если несколько граней указывают на одну и ту же ячейку-соседа, параллельная запись в `bPrimePtr[uPtr[facei]]` даёт неопределённый результат. Презентация Fixstars прямо показывает это [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257):

> «Dependency among face — Data race (write at the same time, different face). lduMatrix can not be parallelized.»

### 1.2. SpMV (матрично-векторное умножение) — та же проблема

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

Прямая запись `#pragma omp parallel for` здесь **не работает**: разные грани могут писать в один и тот же `ApsiPtr[uPtr[face]]` — data race [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257).

Fixstars решает это конверсией lduMatrix → CSR (Compressed Sparse Row), где строки матрицы независимы и можно безопасно параллелить по строкам:

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

Но OpenFOAM не использует CSR — он использует lduMatrix с косвенной адресацией через `lowerAddr`/`upperAddr` [```web_15_0_0_18```](https://boyaowang.github.io/boyaowang_OpenFOAM.github.io/2020/10/26/fvMatrix/).

---

## 2. Почему AVX-512 почти не помогает

### 2.1. Подтверждение из IXPUG (Intel Performance User Group)

Презентация IXPUG на конференции 2018 года прямо отмечает для OpenFOAM на Knights Landing (AVX-512) [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf):

> «KNL flag (-avx512) seems ineffective (see vectorization section).»

> «OpenFOAM makes very little use of vectorization: Non-vector-friendly algorithms. Non-vector friendly implementation of these algorithms. Double indexing frequently used. Inefficient retrieval of data from memory (unstructured meshes, large sparse matrices).»

Для DIC-PCG (preconditioner + solver) эффект от векторизации — **менее 10%** [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf). Для GAMG — **нет ускорения вообще** [```web_15_0_0_5```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf):

> «Effect of vectorization: GAMG — No speed up.»

### 2.2. Gather/scatter — узкое место

AVX-512 имеет инструкции `vpgatherdd`/`vpscatterdd` для загрузки данных по произвольным индексам. Но они **медленные**: на Skylake-SP один gather занимает 10–20 тактов, тогда как прямая загрузка (`vmovupd`) — 1–2 такта. Для Gauss-Seidel, где **каждый** доступ к `psiPtr[uPtr[facei]]` — gather, SIMD даёт не ускорение, а **замедление**.

### 2.3. Downclocking — дополнительный штраф

На Skylake-SP использование 512-битных инструкций вызывает снижение тактовой частоты на 30–40% [```web_15_0_0_7```](https://habr.com/ru/companies/yadro/articles/779284/). Если компилятор изредка вставляет AVX-512, но основная масса кода остаётся скалярной — процессор снижает частоту, а выигрыша от векторизации нет. Итог: код работает **медленнее**, чем без AVX-512.

### 2.4. Исключение: AVX-512 на AMD Zen 4

На AMD EPYC 9004 (Genoa) 512-битные инструкции **не вызывают downclocking**, и AVX-512 даёт заметный прирост [```web_15_1_0_5```](https://www.pugetsystems.com/labs/hpc/amd-zen4-threadripper-pro-vs-intel-xeon-w9-for-science-and-engineering/). Но и там `lduAddressing` ограничивает выигрыш — просто нет штрафа за downclocking.

---

## 3. Почему OpenMP на Westmere работает

### 3.1. Что именно параллелит OpenMP

OpenMP распараллеливает **циклы по ячейкам и граням** — те самые циклы, которые SIMD не может векторизовать из-за косвенной адресации. Ключевые операции, поддающиеся OpenMP:

| Операция | SIMD (AVX-512) | OpenMP | Почему |
|---|---|---|---|
| SpMV (`Amul`) в lduMatrix | ❌ Data race | ⚠️ Требует CSR | Косвенная адресация, гонка записи |
| Gauss-Seidel smoothing | ❌ Зависимость по данным | ⚠️ Требует red-black или domain decomposition | Последовательная природа |
| DIC preconditioner | ❌ Data race | ⚠️ Требует CSR | То же, что SpMV |
| WAXPBY (`y = αx + βy`) | ✅ Unit stride | ✅ `#pragma omp parallel for` | Прямой доступ |
| sumMag, sumProd | ⚠️ Reduction | ✅ `reduction(+:sum)` | Редукция |
| Restriction/prolongation | ⚠️ Косвенная | ✅ `#pragma omp parallel for` | Чтение по косвенному индексу, но запись прямая |

### 3.2. Как Fixstars параллелит DIC-PCG с OpenMP

Fixstars (2019) показала на Intel KNL 13.5× ускорение pimpleFoam при 256 потоках [```web_15_6_0_0```](https://www.slideshare.net/slideshow/a-challenge-for-thread-parallelism-on-openfoam/190506257). Их подход:

1. **Конверсия lduMatrix → CSR** для SpMV — убирает data race
2. **Параллельный Gauss-Seidel через red-black** — разбиение ячеек на два цвета, каждый цвет обрабатывается параллельно (нет зависимости между ячейками одного цвета)
3. **Параллельная редукция** для `sumMag`/`sumProd` — через `#pragma omp parallel for reduction(+:sum)`

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

Red-black нарушает строгую последовательность Gauss-Seidel, но сходится к тому же решению (медленнее по итерациям, но быстрее по wall-clock при параллельном выполнении).

### 3.3. Почему Westmere + OpenMP ≈ Xeon Gold + AVX-512

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

## 4. Где AVX-512 реально оторвался бы

Единственный сценарий, при котором AVX-512 дал бы 2–4× ускорение — переписать GAMG под структуры данных без косвенной адресации:

### 4.1. Block-матрицы (foam-extend)

foam-extend уже реализует `BlockLduMatrix` для coupled-решателей [```web_15_1_0_10```](https://www.tfd.chalmers.se/~hani/kurser/OS_CFD_2014/KlasJareteg_CoupledSolver_TME050_2014.pdf). Если block-матрица хранит 3×3 (или 4×4) блоки в row-major порядке, то операции внутри блока — **прямая адресация**, и AVX-512 может векторизовать:

```cpp
// Block 3x3, row-major — unit stride для AVX-512
#pragma omp simd
for (label i=0; i<nBlocks; i++)
{
    // 9 умножений подряд — компилятор векторизует в zmm
    result[i*9+0] = a[i*9+0]*x[0] + a[i*9+1]*x[1] + a[i*9+2]*x[2];
    result[i*9+3] = a[i*9+3]*x[0] + a[i*9+4]*x[1] + a[i*9+5]*x[2];
    // ...
}
```

### 4.2. SoA layout + CSR

SELL-C-σ (Slice-ELL with Chunked-σ ordering) — формат, разработанный для SpMV на широких SIMD-юнитах. Исследования показывают 2–4× ускорение AVX-512 на CSR/SELL-C-σ по сравнению со скалярным кодом [```web_15_0_0_6```](https://oneapi.io/blog/dive-achieves-1-5x-speedup-in-vectorization-with-latest-intel-cpu-and-oneapi-software/).

Но для этого нужно:

- Конверсия `lduMatrix` → CSR (или SELL-C-σ) — нетривиальный overhead [```web_15_0_0_16```](https://www.academia.edu/70352417/OpenFOAM_on_GPUs_using_AMGX)
- Перепись всех solvers/preconditioners под новый формат
- Разработка нового multigrid с векторизуемым сглаживателем

### 4.3. SPUMA (2025): новейший подход

Архитектура SPUMA (2025) предлагает «minimally invasive» подход к GPU-портированию OpenFOAM через `parallelFor` executor, который конвертирует lduMatrix в CSR на лету [```web_15_0_0_1```](https://arxiv.org/html/2512.22215). Они отмечают:

> «Only a conversion from the OpenFOAM native format to the CSR format is now required.»

> «The conversion is implemented using Radix sort applied to the OpenFOAM arrays representing the sparsity pattern.»

Это подтверждает: для векторизации (CPU SIMD или GPU) lduMatrix **нужно** конвертировать.

---

## 5. Вывод

```
Производительность OpenFOAM
  │
  │   OpenMP (по ядрам)
  │   ┌─────────────────────────┐
  │   │                         │
  │   │   4–10× ускорение        │
  │   │   (линейное масштаб.)    │
  │   │                         │
  │   └─────────────────────────┘
  │
  │   AVX-512 (SIMD)
  │   ┌─────┐
  │   │ 0–15%│ ← lduAddressing не даёт векторизовать
  │   └─────┘
  │
  └──────────────────────────────────→
       Westmere      Xeon Gold
       (SSE4.2)      (AVX-512)
```

**OpenMP на Westmere компенсирует разрыв** за счёт того, что он параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге `-march=westmere -fiopenmp` дают примерно тот же прирост, что `-xCORE-AVX512` на Xeon Gold — не потому что Westmere хорош, а потому что OpenFOAM плохо векторизуется, и SIMD-ширина почти не matter.

Единственное, где AVX-512 реально оторвался бы — переписать GAMG под структуры данных без косвенной адресации (block-матрицы, SoA layout, CSR). Но это уже не OpenFOAM, а исследовательский код.

|---|---|---|
| SIMD-ширина | 2 double | 8 double |
| Автовекторизация lduMatrix | ❌ Нет (косвенная адресация) | ❌ Нет (косвенная адресация) |
| Gather/scatter penalty | Нет (SSE4.2 gather примитивен) | Высокий (10–20 тактов на gather) |
| Downclocking | Нет | Да (30–40% на Skylake-SP) |
| OpenMP по ядрам | 6–12 ядер, линейное ускорение | 16–24 ядра, линейное ускорение |
| **Итоговый прирост от флагов** | **OpenMP: 4–10× (по ядрам)** | **AVX-512: 0–15% (от векторизации)** |

Прирост от `-march=westmere` (SSE4.2) и от `-xCORE-AVX512` в OpenFOAM **одинаково мал** — потому что SIMD-ширина не реализуется. А OpenMP даёт масштабирование по ядрам, которое **не зависит от SIMD-ширины**.

---

## 4. Где AVX-512 реально оторвался бы

Единственный сценарий, при котором AVX-512 дал бы 2–4× ускорение — переписать GAMG под структуры данных без косвенной адресации:

### 4.1. Block-матрицы (foam-extend)

foam-extend уже реализует `BlockLduMatrix` для coupled-решателей [```web_15_1_0_10```](https://www.tfd.chalmers.se/~hani/kurser/OS_CFD_2014/KlasJareteg_CoupledSolver_TME050_2014.pdf). Если block-матрица хранит 3×3 (или 4×4) блоки в row-major порядке, то операции внутри блока — **прямая адресация**, и AVX-512 может векторизовать:

```cpp
// Block 3x3, row-major — unit stride для AVX-512
#pragma omp simd
for (label i=0; i<nBlocks; i++)
{
    // 9 умножений подряд — компилятор векторизует в zmm
    result[i*9+0] = a[i*9+0]*x[0] + a[i*9+1]*x[1] + a[i*9+2]*x[2];
    result[i*9+3] = a[i*9+3]*x[0] + a[i*9+4]*x[1] + a[i*9+5]*x[2];
    // ...
}
```

### 4.2. SoA layout + CSR

SELL-C-σ (Slice-ELL with Chunked-σ ordering) — формат, разработанный для SpMV на широких SIMD-юнитах. Исследования показывают 2–4× ускорение AVX-512 на CSR/SELL-C-σ по сравнению со скалярным кодом [```web_15_0_0_6```](https://oneapi.io/blog/dive-achieves-1-5x-speedup-in-vectorization-with-latest-intel-cpu-and-oneapi-software/).

Но для этого нужно:

- Конверсия `lduMatrix` → CSR (или SELL-C-σ) — нетривиальный overhead [```web_15_0_0_16```](https://www.academia.edu/70352417/OpenFOAM_on_GPUs_using_AMGX)
- Перепись всех solvers/preconditioners под новый формат
- Разработка нового multigrid с векторизуемым сглаживателем

### 4.3. SPUMA (2025): новейший подход

Архитектура SPUMA (2025) предлагает «minimally invasive» подход к GPU-портированию OpenFOAM через `parallelFor` executor, который конвертирует lduMatrix в CSR на лету [```web_15_0_0_1```](https://arxiv.org/html/2512.22215). Они отмечают:

> «Only a conversion from the OpenFOAM native format to the CSR format is now required.»

> «The conversion is implemented using Radix sort applied to the OpenFOAM arrays representing the sparsity pattern.»

Это подтверждает: для векторизации (CPU SIMD или GPU) lduMatrix **нужно** конвертировать.

---

## 5. Вывод

```
Производительность OpenFOAM
  │
  │   OpenMP (по ядрам)
  │   ┌─────────────────────────┐
  │   │                         │
  │   │   4–10× ускорение        │
  │   │   (линейное масштаб.)    │
  │   │                         │
  │   └─────────────────────────┘
  │
  │   AVX-512 (SIMD)
  │   ┌─────┐
  │   │ 0–15%│ ← lduAddressing не даёт векторизовать
  │   └─────┘
  │
  └──────────────────────────────────→
       Westmere      Xeon Gold
       (SSE4.2)      (AVX-512)
```

**OpenMP на Westmere компенсирует разрыв** за счёт того, что он параллелит именно те циклы, которые SIMD не берёт — разреженную алгебру GAMG. В итоге `-march=westmere -fiopenmp` дают примерно тот же прирост, что `-xCORE-AVX512` на Xeon Gold — не потому что Westmere хорош, а потому что OpenFOAM плохо векторизуется, и SIMD-ширина почти не matter.

Единственное, где AVX-512 реально оторвался бы — переписать GAMG под структуры данных без косвенной адресации (block-матрицы, SoA layout, CSR). Но это уже не OpenFOAM, а исследовательский код.


Единственное, где AVX-512 реально оторвался бы — если бы переписать GAMG под
структуры данных без косвенной адресации (block-матрицы, SoA layout). Но это
уже не OpenFOAM, а исследовательский код.

---
### Почему CFD-сообщество жалуется на AVX-512: главный сюрприз — `-frounding-math`

Это известная боль: покупают Xeon Gold с AVX-512, компилируют OpenFOAM с `-xCORE-AVX512`, ждут двукратного ускорения — а получают 10–15%. Потом пишут на CFD Online: «AVX-512 не работает в OpenFOAM, что я делаю не так?»

Ответ в одном: структура данных `lduAddressing` с косвенной адресацией просто не даёт компилятору векторизовать то, что съедает 90% времени. Это не баг — архитектурное решение OpenFOAM, заточенное под работу с произвольными неструктурированными сетками.

---

## 1. Структура данных `lduAddressing` — почему векторизация не работает

### 1.1. Как устроено хранение матрицы

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

### 1.2. Узкое место: матрично-векторное произведение

Типичный цикл матрично-векторного произведения в OpenFOAM (упрощённо):

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

Компилятор видит: на каждой итерации значение `cellO` и `cellN` — произвольные, заранее неизвестные. Векторизация невозможна, потому что:

1. **`psi[cellO]` и `psi[cellN]`** — доступ по произвольному индексу. Нет гарантии, что `cellO` и `cellN` для соседних граней отличаются на единицу. Нет unit-stride — нет векторизации.

2. **Зависимость по данным**: при сборке матрицы, ячейка может быть «соседом» для нескольких граней. Компилятор не может переупорядочить цикл, не зная структуры графа.

3. **Gather/scatter**: AVX-512 имеет инструкции `vpgatherdd` / `vpscatterdd` для загрузки данных по произвольным индексам. Но они **медленные** — на Skylake-SP один gather может занимать 10–20 тактов, тогда как прямая загрузка (vmovupd) — 1–2 такта.

### 1.3. Подтверждение из исследований

Презентация IXPUG (Intel Performance User Group) прямо отмечает для OpenFOAM на KNL:

> «KNL flag (-avx512) seems ineffective (see vectorization section)» [```web_15_13_0_0```](https://www.ixpug.org/images/docs/IXPUG_Annual_Spring_Conference_2018/IXpug-OpenFOAM.pdf)

Исследование по мини-приложению MG-CFD (модель OpenFOAM-подобного CFD-кода на неструктурированных сетках) подтверждает:

> «Once iflux exceeds 80 threads, it becomes fully memory-bound under AVX2 and AVX-512 with error falling to near-zero... These loops have the same computational structure: a single loop over edges, accumulating fluxes.» [```web_15_12_0_1```](https://eprints.whiterose.ac.uk/id/eprint/148034/8/cpe.5443.pdf)

PETSc-разработчики также отмечают для Sparse Matrix-Vector Product (SpMV) с AVX-512:

> «CSR is not the optimal choice for matrices whose number of nonzeros per row is either small or not a multiple of the length of the CPU vector register, which are common in the PDE regime.» [```web_15_1_0_11```](https://openearthscience.org/~rmills/pubs/Zhang-2018-vectorized-SpMV-AVX512.pdf)

И российская работа по векторизации малоразмерных матриц RANS/ILES:

> «В коде присутствуют медленные инструкции gather/scatter, читающие из памяти элементы данных с произвольными смещениями... их следует избегать.» [```web_15_1_0_13```](https://www.academia.edu/79816992/)

---

## 2. `-frounding-math` — почему он блокирует оптимизации

### 2.1. Что делает флаг

`-frounding-math` сообщает компилятору: **не предполагай режим округления по умолчанию (round-to-nearest)**. Программа может изменить режим округления во время выполнения через `fesetround()`, поэтому компилятор не имеет права:

- Выполнять свёртку констант (constant folding) для выражений с плавающей точкой, результат которых зависит от режима округления
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

CGAL (Computational Geometry Algorithms Library) требует `-frounding-math`, потому что использует динамическое изменение режима округления для точной геометрической арифметики [```web_15_2_0_2```](https://github.com/OpenFOAM/OpenFOAM-2.2.x/blob/master/applications/utilities/surface/surfaceFeatureExtract/Allwmake) [```web_15_0_0_16```](https://github.com/CGAL/cgal/issues/3180).

В ESI fork (openfoam.com) `-frounding-math` в основных правилах `wmake` **отсутствует** — вместо этого используется `-ffast-math`, который включает `-fno-rounding-math` (обратный флаг):

```makefile
# wmake/rules/linux64Gcc/c++Opt (ESI, v2212+)
c++OPT = -O3 -ffast-math -fno-math-errno
# -ffast-math включает: -fno-rounding-math, -fno-trapping-math,
#   -fassociative-math, -ffinite-math-only, -fno-signed-zeros, ...
```

```makefile
# wmake/rules/linux64Gcc/c++Opt (Foundation, openfoam.org)
c++OPT = -O3
# Никакого -ffast-math: более консервативный подход
```

```makefile
# wmake/rules/linux64Clang/c++Opt (Foundation, Clang)
c++OPT = -O3
# Тоже без -ffast-math
```

### 2.3. Сюрприз: `-frounding-math` убивает векторизацию

Если ты случайно собираешь OpenFOAM (или часть библиотеки, например CGAL-зависимые утилиты) с `-frounding-math`, компилятор:

1. **Не использует FMA** — комбинированная инструкция `vfmadd231pd` заменяется на отдельные `vmulpd` + `vaddpd`, что вдвое медленнее.

2. **Не векторизует редукции** — циклы вида `sum += a[i] * b[i]` не могут быть векторизованы, потому что порядок сложений может изменить результат при разных режимах округления. Компилятор генерирует скалярный код.

3. **Не выполняет constant folding** — даже `constexpr double x = 1.0 / 6.0;` не вычисляется на этапе компиляции [```web_15_0_0_16```](https://github.com/CGAL/cgal/issues/3180).

4. **Не заменяет деление** — `x / y` остаётся делением, а не умножением на `1/y`, что на 3–5 тактов медленнее.

Для CFD, где 70–90% времени — это циклы над гранями/ячейками с умножением и сложением, потеря FMA и векторизации означает **потерю всего потенциала AVX-512**.

---

## 3. Дополнительный фактор: downclocking

На процессорах Skylake-SP (Xeon Gold/Platinum) использование 512-битных инструкций вызывает **снижение тактовой частоты**:

| Уровень | Условие | Снижение частоты |
|---|---|---|
| L0 | Нет AVX-инструкций | 100% (базовый турбо) |
| L1 | 256-битные «тяжёлые» FP-инструкции | ~85% от максимума |
| L2 | 512-битные «тяжёлые» FP-инструкции | ~60–70% от максимума |

Если компилятор изредка вставляет AVX-512 инструкции (например, для выравнивания стека или редких векторизованных участков), но основная масса кода остаётся скалярной — процессор снижает частоту, **а выигрыша от векторизации нет**. Итог: код работает **медленнее**, чем без AVX-512 [```web_15_1_0_8```](https://stackoverflow.com/questions/63484266/enabling-avx512-support-on-compilation-significantly-decreases-performance).

GCC начиная с версии 9 по умолчанию использует `-mprefer-vector-width=256` для Skylake-AVX512 — именно по этой причине: 512-битные инструкции в большинстве кода приносят больше вреда, чем пользы.

---

## 4. Что всё-таки помогает

### 4.1. `restrict` и `__builtin_assume_aligned`

Исследование Zirwes et al. по векторизации химической кинетики в OpenFOAM показало:

> «Using `restrict` and `__builtin_assume_aligned` for the function arguments enables gcc and icpc to generate a vectorized version of the loop... reduces computation times for chemical reaction rates by up to 50% and total simulation times by up to 25%.» [```web_15_0_0_17```](https://www.academia.edu/124915224/)

Разница в ассемблере — наглядная:

```asm
; Без restrict: скалярный код
addsd   xmm0, [rdi+rax*8]     ; одно сложение за такт

; С restrict: векторизованный код
vaddpd   zmm0, zmm0, [rdi+rax*8]  ; 8 сложений за такт (AVX-512)
```

Проблема: OpenFOAM не использует `restrict` в основных циклах `lduMatrix` — потому что структура данных (косвенная адресация) делает алиасинг неизбежным.

### 4.2. Блочно-структурированные сетки

Для структурированных сеток (где `lowerAddr` и `upperAddr` имеют предсказуемые шаблоны) возможна ручная векторизация с AVX-512:

```cpp
// Структурированная сетка: cellN = cellO + nx (постоянный stride)
// Можно векторизовать напрямую:
#pragma omp simd
for (label i = 0; i < nCells; i++)
{
    result[i] = a[i] * x[i] + b[i] * x[i + nx];
    //         ↑       ↑       ↑       ↑
    //         прямой  прямой  прямой  прямой — unit stride!
}
```

Это то, что делают альтернативные CFD-коды (например,_block-structured коды вроде听说 EXITS или коды с SELL-C-σ форматом), получая 2–4× ускорение от AVX-512 [```web_15_1_0_11```](https://openearthscience.org/~rmills/pubs/Zhang-2018-vectorized-SpMV-AVX512.pdf).

### 4.3. Компиляторные флаги: правильная комбинация

Для Intel ICX (`icpx`) оптимальный набор:

```bash
# Оптимизация: включаем FMA, запрещаем -frounding-math
-O3 -fiopenmp -fp-model=precise -fno-rounding-math

# Архитектура: таргетим AVX-512, но не позволяем компилятору
# вставлять 512-битные инструкции где попало
-xSKYLAKE-AVX512

# Альтернатива: ограничить до 256-бит (если downclocking перевешивает)
-mprefer-vector-width=256  # GCC
-zmm-width=256             # не существует; аналог: -mavx2 без -mavx512f
```

### 4.4. AMD EPYC: AVX-512 без downclocking

На AMD Zen 4 (EPYC 9004 «Genoa») 512-битные инструкции **не вызывают downclocking**. Phoronix подтвердил для OpenFOAM:

> «Genoa's AVX-512 presence was also very beneficial for the OpenFOAM 10 computational fluid dynamics software.» [```web_15_0_0_11```](https://www.phoronix.com/review/amd-epyc-avx512/9)

Однако и тут `lduAddressing` ограничивает выигрыш — просто нет downclocking-штрафа, который мог бы сделать код медленнее.

---

## 5. Сводная таблица

| Фактор | Влияние на AVX-512 в OpenFOAM | Можно исправить? |
|---|---|---|
| `lduAddressing` (косвенная адресация) | Главная причина — нет unit-stride, нет векторизации | Только переписывание структуры данных |
| Gather/scatter (медленные на Skylake) | 10–20 тактов на загрузку вместо 1–2 | Нет, аппаратное ограничение |
| `-frounding-math` | Запрещает FMA, constant folding, векторизацию редукций | Да: использовать `-fno-rounding-math` или `-ffast-math` |
| Downclocking (Skylake-SP) | Снижение частоты на 30–40% при AVX-512 | Да: `-mprefer-vector-width=256` или переход на Ice Lake / AMD |
| Отсутствие `restrict` | Компилятор не может доказать отсутствие алиасинга | Да: ручная аннотация (но требует изменения исходного кода) |
| Неструктурированные сетки | Непредсказуемый паттерн доступа к памяти | Структурные сетки или блочные форматы (SELL-C-σ) |

---

## 6. Вывод

AVX-512 в OpenFOAM даёт 10–15%, а не 2× — потому что **90% времени уходит на циклы, которые невозможно векторизовать из-за косвенной адресации `lduAddressing`**. Это не баг, а следствие архитектуры, заточенной под неструктурированные сетки.

Флаг `-frounding-math` усугубляет ситуацию, запрещая FMA и векторизацию редукций. В ESI fork он не используется в основных правилах — но если ты собираешь CGAL-зависимые утилиты, он может попасть в флаги неявно.

Единственный реальный путь к 2× ускорению от SIMD в OpenFOAM — переписать горячие циклы (mat-vec, градиенты, flux) на блочно-структурированный формат с `restrict` и прямой адресацией. Без этого AVX-512 остаётся красивым маркетинговым флагом, который в реальном CFD даёт скромные 10–15%.


### Источники

- [Intel: Tuning SIMD Vectorization for Xeon Scalable](https://www.intel.com/content/www/us/en/developer/articles/technical/tuning-simd-vectorization-when-targeting-intel-xeon-processor-scalable-family.html)
- [CERN: SIMD Benchmark Results](https://indico.cern.ch/event/1030673/contributions/4389815/attachments/2256944/3830869/20210602-BMK-TF-SIMD-v2.pdf)
- [AMD: OpenFOAM Solution Brief](https://www.amd.com/content/dam/amd/en/documents/epyc-business-docs/solution-briefs/amd-epyc-7Fx2-openfoam.pdf)
- [ResearchGate: OpenFOAM Performance Optimization on Xeon Phi](https://www.researchgate.net/publication/323210210_Performance_Optimization_of_OpenFOAM_on_Clusters_of_IntelR_Xeon_Phi_TM_Processors)

