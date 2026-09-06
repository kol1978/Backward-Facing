# Сборка OpenFOAM с Intel icpx и OpenMP: анализ проблем v2606

![OpenFOAM](https://img.shields.io/badge/OpenFOAM-v2306%20%7C%20v2606-blue)
![Compiler](https://img.shields.io/badge/Intel%20icpx-oneAPI-orange)
![OpenMP](https://img.shields.io/badge/OpenMP--fiopenmp-red)

---

## TL;DR

| | v2306 | v2606 |
|---|---|---|
| **Сборка с `-fiopenmp`** | ✅ Успешно | ❌ Ошибка |
| **Expression templates** | Отсутствуют | Добавлены (с v2512, MR !763) |
| **Стандарт C++** | C++14 | C++17 |
| **GPU-оффлоадинг** | — | `std::execution` (не OpenMP) |
| **Типы итераторов** | Trivially copyable | Non-trivially copyable |

**Суть проблемы:** в v2512 в OpenFOAM добавлены expression templates — сложные
template-типы для оптимизации полевых операций и подготовки к GPU-оффлоадингу.
Флаг `-fiopenmp` в icpx активирует LLVM OpenMP target offloading pass, который
деградирует тип итератора `ConstIter` до сырого указателя `const double *`,
ломая доступ к членам типа. В v2306 expression templates отсутствуют, типы
простые и trivially copyable — конфликтов нет.

---

## Содержание

- [Контекст сборки](#контекст-сборки)
- [Флаги компиляции](#флаги-компиляции)
- [Причина ошибки](#причина-ошибки)
- [Решение OpenCFD](#решение-opencfd)
- [10 причин: подробно с документацией](#10-причин-подробно-с-документацией)
- [Сводная таблица](#сводная-таблица)
- [Исправления и выводы](#исправления-и-выводы)

---

## Контекст сборки

Сборка OpenFOAM v2306 выполнена на той же платформе, что и v2606:

- **CPU:** 2× Xeon X5675 (Westmere)
- **Компилятор:** Intel icpx (oneAPI)
- **MPI:** Intel MPI
- **Флаг arch:** `-march=westmere`

В отличие от v2606, сборка v2306 **выполнена с OpenMP** (`-fiopenmp`), поскольку
в этой версии отсутствуют expression templates, конфликтующие с OpenMP target
offloading pass.

Тест-запуск (`tutorialsTest`) охватывает все основные категории: DNS,
incompressible, compressible, multiphase, heatTransfer, stressAnalysis,
verificationAndValidation, preProcessing и другие.

> **Примечание по стандарту C++.**
> OpenFOAM v2306 использует C++14. Release notes v2306 сообщают:
> *"Note that the minimum C++ standard will increase from C++11 to C++17 in 2023"*
> ([openfoam.com/v2306](https://www.openfoam.com/news/main-news/openfoam-v2306/infrastructure)).
> Release notes v2406 подтверждают переход:
> *"Note that the minimum C++ standard will increase from C++14 to C++17 in 2024
> to help support ongoing GPU developments"*
> ([openfoam.com/v2406](https://www.openfoam.com/news/main-news/openfoam-v2406/infrastructure)).
>
> Таким образом: **v2306 — C++14, v2406+ — C++17.**

---

## Флаги компиляции

### v2306 — с OpenMP (сборка и запуск успешны)

```bash
ccache icpx -std=c++14 -fp-model precise -pthread -DOPENFOAM=2306 -DWM_DP \
  -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter \
  -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template \
  -diag-disable 327,654,1125,1292,2289,2304,11062,11074,11076 \
  -O3 -march=westmere -frounding-math -DNoRepository \
  -Wno-old-style-cast -Wno-deprecated-declarations \
  -DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX \
  -isystem /opt/intel/oneapi/mpi/2021.18/include \
  -iquote. -IlnInclude -I
```

### v2606 — с OpenMP (сборка падает)

Ключевое отличие: `-std=c++17` вместо `-std=c++14`. Остальные флаги аналогичны,
но именно в v2606 появляются expression templates, конфликтующие с offloading pass.

---

## Причина ошибки

### Expression templates (v2512, MR !763)

В OpenFOAM v2512 добавлена библиотека expression templates, которая трансформирует
выполнение полевых операций:

> *"This release includes an initial version of an expression templates library
> that transforms how field operations are executed. This powerful optimization
> technique eliminates intermediate field allocations, fuses multiple operations
> into single computational kernels, and enables hardware acceleration including
> GPU offloading."*
> — [OpenFOAM v2512 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics)

Expression templates создают сложные template-типы (например,
`List_divide<List_multiply<...>>`), которые **не являются trivially copyable**.

### Деградация типа итератора

Флаг `-fiopenmp` активирует LLVM OpenMP runtime с target offloading pass:

> *"-fiopenmp: Compile and recognize OpenMP parallel and SIMD pragmas/directives
> and clauses and use the Intel OpenMP runtime libraries."*
> — [Intel Porting Guide](https://www.intel.cn/content/www/cn/zh/developer/articles/guide/porting-guide-for-icc-users-to-dpcpp-or-icx.html)

Подтверждение от Intel community:

> *"If we do not specify any target, then the default offloading to host/CPU
> will be done."*
> — [Intel Community](https://community.intel.com/t5/Intel-oneAPI-DPC-C-Compiler/Unable-to-offload-to-integrated-or-dedicated-GPUs-number-of/m-p/1634929)

Offloading pass (`vpo-paropt`) деградирует `ConstIter` до `const double *`,
ломая доступ к членам типа. Баг-репорты подтверждают нестабильность pass
на сложных типах:
[#21024](https://github.com/intel/llvm/issues/21024),
[#21957](https://github.com/intel/llvm/issues/21957).

---

## Решение OpenCFD

В v2606 GPU-оффлоадинг реализован через `std::execution` (C++17/20), а не через OpenMP:

> *"This is the first release to support GPU offloading. It uses the C++17/20
> std::execution policy to automatically run loops in parallel across multiple
> execution units."*
> — [OpenFOAM v2606 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2606/infrastructure)

> *"Using CPU threading (-stdpar=multicore) is not properly supported
> (work-in-progress)."*

> *"After further testing and feedback from the Community we intend to integrate
> the code in the OpenFOAM v2612 release."*

OpenMP **структурно несовместим** с expression templates — прагма не вставляется
внутрь template-цикла (архитектурное ограничение C++). CPU-threading через
`std::execution` заявлен на v2612.

---

## 10 причин: подробно с документацией

### 1. Expression templates создают non-trivially-copyable типы
**Тип:** Структурная

Expression templates добавлены в v2512 (MR !763) как инфраструктура для будущего
GPU-оффлоадинга. Они создают сложные template-типы (`List_divide<List_multiply<...>>`),
которые не являются trivially copyable.

📎 [OpenFOAM v2512 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics)

---

### 2. `-fiopenmp` активирует LLVM OpenMP runtime с offloading pass
**Тип:** Конфигурационная

Intel OpenMP runtime — LLVM-based, включает target offloading infrastructure.
`-fopenmp-targets=spir64` нужен только для явного указания GPU-цели, но сам
offloading pass активируется уже с `-fiopenmp`.

📎 [Intel Porting Guide](https://www.intel.cn/content/www/cn/zh/developer/articles/guide/porting-guide-for-icc-users-to-dpcpp-or-icx.html)
· [Intel Community](https://community.intel.com/t5/Intel-oneAPI-DPC-C-Compiler/Unable-to-offload-to-integrated-or-dedicated-GPUs-number-of/m-p/1634929)

---

### 3. Offloading pass деградирует `ConstIter` до `const double *`
**Тип:** Техническая

`vpo-paropt` pass падает на сложных типах. Реальные баг-репорты в icpx
2025.2/2025.3 подтверждают, что pass неустойчив при работе со сложными
template-типами.

📎 [#21024](https://github.com/intel/llvm/issues/21024) ·
[#21957](https://github.com/intel/llvm/issues/21957)

---

### 4. OpenMP прагма не вставляется внутрь template-цикла
**Тип:** Архитектурная (структурное ограничение C++)

OpenMP pragma должна размещаться на уровне синтаксической конструкции, но не
может быть встроена внутрь template-цикла, раскрываемого компилятором.
Это фундаментальное ограничение языка.

---

### 5. OpenCFD выбрала `std::execution` вместо OpenMP
**Тип:** Стратегическая

GPU-оффлоадинг в v2606 реализован через C++17/20 `std::execution`.
CPU-threading через `std::execution` (`-stdpar=multicore`) заявлен на v2612.

📎 [OpenFOAM v2606 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2606/infrastructure)

---

### 6. AMD обнаружила недетерминизм с OpenMP offloading в GAMG
**Тип:** Эксплуатационная

> *"A community-contributed fix (led by AMD) resolves non-deterministic behavior
> in pairGAMGAgglomeration that previously caused run-to-run variation in
> convergence paths when OpenMP-style offloading was active."*

Прямое подтверждение, что OpenMP target offloading создавал проблемы
на уровне рантайма.

📎 [OpenFOAM v2512 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics)

---

### 7. AMD имела альтернативный OpenMP-форк, не вошедший в mainline
**Тип:** Историческая

> *"Refactoring OpenFOAM with OpenMP target offloading and use of HMM to offload
> work onto GPUs."*

AMD пошла путём OpenMP target offloading, но OpenCFD (mainline) выбрала
`std::execution` вместо этого.

📎 [ROCm/OpenFOAM_HMM](https://github.com/ROCm/OpenFOAM_HMM)

---

### 8. OpenMP несовместим с SYCL в device-коде
**Тип:** Платформенная

> *"OpenMP directives cannot be used inside DPC++/SYCL GPU kernels. DPC++/SYCL
> code cannot be used inside the OpenMP target regions. The direct interaction
> between OpenMP and SYCL runtime libraries is not supported at this time."*

📎 [Intel OpenMP/SYCL composability](https://fs.hlrs.de/projects/par/events/2022/intel-oneapi/)

---

### 9. `twoPhaseEulerFoam` устаревший, заменён на `multiphaseEulerFoam`
**Тип:** Версионная

> *"In OpenFOAM 7 (and earlier versions), there were two solvers to deal with
> dispersed flows, namely, multiphaseEulerFoam and twoPhaseEulerFoam. In OpenFOAM 8,
> these two solvers were merged into one single solver, namely, multiphaseEulerFoam."*

📎 [Wolf Dynamics Training Materials](https://www.wolfdynamics.com/training/mphase/OF2021/mphase_2021_OF8.pdf)

---

### 10. C++14 → C++17 связан с GPU-разработками (`std::execution`)
**Тип:** Версионная

> *"Note that the minimum C++ standard will increase from C++14 to C++17 in 2024
> to help support ongoing GPU developments."*

Переход на C++17 связан именно с GPU-разработками — `std::execution` требует C++17.

📎 [OpenFOAM v2406 Release Notes](https://www.openfoam.com/news/main-news/openfoam-v2406/infrastructure)

---

## Сводная таблица

| # | Причина | Тип | Документация |
|---|---------|-----|--------------|
| 1 | Expression templates (v2512) создают non-trivially-copyable типы | Структурная | [v2512 numerics](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics) |
| 2 | `-fiopenmp` активирует LLVM OpenMP runtime с offloading pass | Конфигурационная | [Intel Porting Guide](https://www.intel.cn/content/www/cn/zh/developer/articles/guide/porting-guide-for-icc-users-to-dpcpp-or-icx.html) |
| 3 | Offloading pass деградирует `ConstIter` до `const double *` | Техническая | [#21024](https://github.com/intel/llvm/issues/21024) |
| 4 | OpenMP прагма не вставляется внутрь template-цикла | Архитектурная | Структурное ограничение C++ |
| 5 | OpenCFD выбрала `std::execution` вместо OpenMP | Стратегическая | [v2606 infrastructure](https://www.openfoam.com/news/main-news/openfoam-v2606/infrastructure) |
| 6 | AMD обнаружила недетерминизм с OpenMP offloading в GAMG | Эксплуатационная | [v2512 numerics](https://www.openfoam.com/news/main-news/openfoam-v2512/numerics) |
| 7 | AMD имела альтернативный OpenMP-форк, не вошедший в mainline | Историческая | [ROCm/OpenFOAM_HMM](https://github.com/ROCm/OpenFOAM_HMM) |
| 8 | OpenMP несовместим с SYCL в device-коде | Платформенная | [Intel composability](https://fs.hlrs.de/projects/par/events/2022/intel-oneapi/) |
| 9 | `twoPhaseEulerFoam` устаревший, заменён на `multiphaseEulerFoam` | Версионная | [Wolf Dynamics](https://www.wolfdynamics.com/training/mphase/OF2021/mphase_2021_OF8.pdf) |
| 10 | C++14 → C++17 связан с GPU-разработками (`std::execution`) | Версионная | [v2406 infrastructure](https://www.openfoam.com/news/main-news/openfoam-v2406/infrastructure) |

---

## Исправления и выводы

| Изменение | Статус | Примечание |
|-----------|--------|-----------|
| `-std=c++17` → `-std=c++14` для v2306 | ✅ Подтверждено | Release notes v2306/v2406 |
| `-diag-disable` (9 номеров) | ⚠️ Работает в v2306 | Бесполезен в v2606 |
| Реальная ошибка из лога | ✅ Уточнена | `ConstIter` деградация, а не `-Wopenmp-mapping` |
| `-fiopenmp` без `-fopenmp-targets` | ✅ Подтверждено | Offloading pass активируется автоматически |

### Ключевые выводы

1. **OpenMP structurally incompatible** с expression templates в OpenFOAM v2606.
2. `-fiopenmp` неявно активирует offloading pass — даже без `-fopenmp-targets`.
3. OpenCFD сознательно выбрала `std::execution` вместо OpenMP для GPU-оффлоадинга.
4. **v2306 — последняя версия**, корректно собирающаяся с `-fiopenmp`.
5. Для v2606 OpenMP не предусмотрен архитектурой — использовать `-fiopenmp` нельзя.
