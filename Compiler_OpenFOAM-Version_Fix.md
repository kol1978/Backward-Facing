# Багрепорт: Compilation failure with Intel ICX (oneAPI 2026.1)

> **Краткий вывод:** Дело **не в компиляторе Intel**. Проблема — в дизайне `tmp<T>` в OpenFOAM и в том, что проект тестируют только с GCC. GCC «прощает» код, формально неоднозначный по стандарту C++. ICX (как и любой LLVM-based компилятор) строго следует стандарту и сообщает об неоднозначности. Проблема известна с 2012 года и затрагивает все LLVM-based компиляторы: Clang, ICX, Apple Clang.

---

## Оглавление

1. [Багрепорт для QGDsolver](#1-багрепорт-для-qgdsolver)
2. [Багрепорт для hybridCentralSolvers](#2-багрепорт-для-hybridcentralsolvers)
3. [Root Cause: почему дело не в компиляторе Intel](#3-root-cause-почему-дело-не-в-компиляторе-intel)
4. [Подтверждение из источников](#4-подтверждение-из-источников)
5. [Оценка багрепорта](#5-оценка-багрепорта)
6. [Оценка скрипта-патча](#6-оценка-скрипта-патча)
7. [Сводная таблица всех случаев](#7-сводная-таблица-всех-случаев)
8. [References](#8-references)

---

## 1. Багрепорт для QGDsolver

### Title

**Compilation failure with Intel ICX (oneAPI 2026.1) — ambiguous `tmp<>` conversion in `leastSquaresStencil.C`**

**Labels:** `bug`, `compilation`

### Description

`QGDsolver` fails to compile with Intel ICX compiler (oneAPI 2026.1.1, LLVM-based) due to ambiguous implicit conversion from `tmp<surfaceVectorField>` to `surfaceVectorField`. The same issue affects `hybridCentralSolvers` (`vofTwoPhaseCentralFoamEqns.C`).

GCC compiles the code without errors. This is a known long-standing issue with Clang/LLVM-based compilers and OpenFOAM's `tmp<T>` class — see [References](#8-references) below.

### Environment

```
Compiler:     Intel(R) oneAPI DPC++/C++ Compiler 2026.1.1 (2026.1.1.20260724)
Target:       x86_64-unknown-linux-gnu
LLVM backend: 18+
OpenFOAM:     v2312 (api=2312, patch=0)
OS:           Linux x86_64
Platform:     linux64IcxDPInt64Opt
```

### Error

```
leastSquaresStencil.C:147:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  147 |     surfaceVectorField gradComp0col = Grad(iVF.component(0));
      |                           ^          ~~~~~~~~~~~~~~~~~~~~~~~~
OpenFOAM-v2312/src/OpenFOAM/lnInclude/tmp.H:320:9: note: candidate function
  320 |         operator const T&() const { return cref(); }
OpenFOAM-v2312/src/OpenFOAM/lnInclude/GeometricField.H:345:9: note: candidate constructor
  345 |         GeometricField

leastSquaresStencil.C:148:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  148 |     surfaceVectorField gradComp1col = Grad(iVF.component(1));

leastSquaresStencil.C:149:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  149 |     surfaceVectorField gradComp2col = Grad(iVF.component(2));

leastSquaresStencil.C:206:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  206 |     surfaceVectorField gradComp0 = Grad(iVF.component(0));

leastSquaresStencil.C:207:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  207 |     surfaceVectorField gradComp1 = Grad(iVF.component(1));

leastSquaresStencil.C:208:27: error: conversion from 'tmp<surfaceVectorField>' to 'surfaceVectorField' is ambiguous
  208 |     surfaceVectorField gradComp2 = Grad(iVF.component(2));

6 errors generated.
```

### Root cause

OpenFOAM's `tmp<T>` class provides two conversion paths to `T`:

- **Conversion operator:** `tmp::operator const T&() const`
- **Constructor:** `GeometricField(const tmp<GeometricField>&)`

For copy-initialization (`Type x = expr;`), the C++ standard considers both paths. GCC resolves the ambiguity by preferring the constructor. Clang/ICX considers both paths equally valid and reports an ambiguity.

This is a defect in the C++ standard (core issue 2077), not a compiler bug. See: Stack Overflow discussion *"Clang and GCC disagree on legality of direct-initialization with conversion operator"*.

### Affected files and lines

**File:** `fvsc/leastSquares/leastSquaresStencil.C` — 6 lines:

| Line | Code |
|------|------|
| 147 | `surfaceVectorField gradComp0col = Grad(iVF.component(0));` |
| 148 | `surfaceVectorField gradComp1col = Grad(iVF.component(1));` |
| 149 | `surfaceVectorField gradComp2col = Grad(iVF.component(2));` |
| 206 | `surfaceVectorField gradComp0 = Grad(iVF.component(0));` |
| 207 | `surfaceVectorField gradComp1 = Grad(iVF.component(1));` |
| 208 | `surfaceVectorField gradComp2 = Grad(iVF.component(2));` |

### Fix

Replace copy-initialization (`=`) with direct-initialization (`()`):

```diff
--- a/fvsc/leastSquares/leastSquaresStencil.C
+++ b/fvsc/leastSquares/leastSquaresStencil.C
@@ -147,3 +147,3 @@
-    surfaceVectorField gradComp0col = Grad(iVF.component(0));
-    surfaceVectorField gradComp1col = Grad(iVF.component(1));
-    surfaceVectorField gradComp2col = Grad(iVF.component(2));
+    surfaceVectorField gradComp0col(Grad(iVF.component(0)));
+    surfaceVectorField gradComp1col(Grad(iVF.component(1)));
+    surfaceVectorField gradComp2col(Grad(iVF.component(2)));
@@ -206,3 +206,3 @@
-    surfaceVectorField gradComp0 = Grad(iVF.component(0));
-    surfaceVectorField gradComp1 = Grad(iVF.component(1));
-    surfaceVectorField gradComp2 = Grad(iVF.component(2));
+    surfaceVectorField gradComp0(Grad(iVF.component(0)));
+    surfaceVectorField gradComp1(Grad(iVF.component(1)));
+    surfaceVectorField gradComp2(Grad(iVF.component(2)));
```

This fix:

- ✅ Does not change runtime behavior (semantically identical)
- ✅ Does not affect GCC compilation (GCC accepts both forms)
- ✅ Does not affect performance (same code generation)
- ✅ Follows the recommendation from the OpenFOAM development team (issue #3138)

### Alternative fix

Use `.cref()` to explicitly select the conversion path (as done in the preCICE adapter):

```cpp
surfaceVectorField gradComp0col = Grad(iVF.component(0)).cref();
```

This is more explicit but requires modifying every expression individually.

---

## 2. Багрепорт для hybridCentralSolvers

### Title

**Compilation failure with Intel ICX (oneAPI 2026.1) — ambiguous `tmp<>` conversion in `vofTwoPhaseCentralFoamEqns.C`**

**Labels:** `bug`, `compilation`

### Description

`vofTwoPhaseCentralFoam` fails to compile with Intel ICX compiler (oneAPI 2026.1.1, LLVM-based) due to ambiguous implicit conversion from `tmp<GeometricField<...>>` to `GeometricField<...>`. The same issue affects `QGDsolver` (`leastSquaresStencil.C`).

GCC compiles the code without errors. This is a known long-standing issue with Clang/LLVM-based compilers and OpenFOAM's `tmp<T>` class — see [References](#8-references) below.

### Environment

```
Compiler:     Intel(R) oneAPI DPC++/C++ Compiler 2026.1.1 (2026.1.1.20260724)
Target:       x86_64-unknown-linux-gnu
LLVM backend: 18+
OpenFOAM:     v2312 (api=2312, patch=0)
Branch:       digitef-dev-2312
OS:           Linux x86_64
Platform:     linux64IcxDPInt64Opt
```

### Error

```
vofTwoPhaseCentralFoamEqns.C:108:24: error: conversion from 'tmp<GeometricField<double, Foam::fvPatchField, Foam::volMesh>>'
to 'volScalarField' is ambiguous
  108 |         volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);
      |                        ^              ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
OpenFOAM-v2312/src/OpenFOAM/lnInclude/tmp.H:320:9: note: candidate function
  320 |         operator const T&() const { return cref(); }
OpenFOAM-v2312/src/OpenFOAM/lnInclude/GeometricField.H:345:9: note: candidate constructor
  345 |         GeometricField

vofTwoPhaseCentralFoamEqns.C:109:24: error: conversion from 'tmp<...>' to 'volScalarField' is ambiguous
  109 |         volScalarField alpha_gas_bd = 1.0 - alpha_liq_bd;

vofTwoPhaseCentralFoamEqns.C:173:20: error: conversion from 'tmp<...>' to 'volScalarField' is ambiguous
  173 |     volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);

vofTwoPhaseCentralFoamEqns.C:175:30: error: conversion from 'tmp<...>' to 'const surfaceScalarField' is ambiguous
  175 |     const surfaceScalarField Unf =
  176 |     phi_ / mesh.magSf();

vofTwoPhaseCentralFoamEqns.C:416:24: error: conversion from 'tmp<...>' to 'surfaceScalarField' is ambiguous
  416 |     surfaceScalarField phiCorr   = linearInterpolate(rho*oneByA_)*fvc::ddtCorr(U_, phi_);

vofTwoPhaseCentralFoamEqns.C:417:24: error: conversion from 'tmp<...>' to 'surfaceScalarField' is ambiguous
  417 |     surfaceScalarField HbyAKappa = phiHbyA_ + phiCorr;
#1-багрепорт-для-qgdsolver
6 errors generated.
```

### Root cause

OpenFOAM's `tmp<T>` class provides two conversion paths to `T`:
Посадка риса — ключевой этап его возделывания, требующий строгого соблюдения агротехнических норм. Рис выращивают на затопляемых полях (чеках), и способ посадки напрямую влияет на урожайность.
fancrd.ru
agroclasses.svoevagro.ru
sfera.fm
Что такое "рисовые чеки"?
Подготовка почвы
Перед посадкой почву тщательно готовят. Цель — уничтожить сорняки, создать рыхлый слой для заделки семян и тщательно выровнять поверхность поля для равномерного затопления.
fancrd.ru
sfera.fm
Способы подготовки:
Мокрый метод применяется на низменных участках. По периметру чека формируют земляные валы, чтобы удержать воду, затем поле затапливают через оросительную систему.
Сухой метод используют в регионах с обильными сезонными дождями. После формирования насыпей и дренажных каналов рис сажают в сухую почву, а затопление происходит за счёт естественных осадков.

sfera.fm
Подготовка семян
Для посева используют только качественные, районированные семена высокоурожайных сортов. Перед посевом их тщательно сортируют и калибруют, чтобы обеспечить равномерные всходы.
cyberleninka.ru
infourok.ru
Дополнительные этапы подготовки:
Замачивание в тёплой воде на 24–48 часов для ускорения прорастания.
Обработка слабым раствором марганцовки для дезинфекции.
Протравливание семян для защиты от болезней.

rudesignshop.ru
infourok.ru
Способы посадки
Существует два основных метода: посев семенами в поле и высадка готовой рассады.
agroclasses.svoevagro.ru
Посев семенами:
Рядовой посев — наиболее распространённый способ. Семена высевают ровными рядами, что обеспечивает равномерное распределение растений и упрощает механизированный уход.
Разбросной (итальянский) метод выполняется центробежными разбрасывателями. Перед посевом поверхность чека обрабатывают ребристыми катками, чтобы создать бороздки глубиной 15–20 мм. При поливе семена скатываются в бороздки, обеспечивая хороший контакт с почвой.

sfera.fm
Высадка рассады:
Рассаду выращивают в специальных ячейках, теплицах или на отдельных полосах поля. Семена проращивают, а через 15–21 день, когда у растений появляется четыре листика, их пересаживают на поле.
Высаживают рассаду вручную или с помощью специальных машин — трансплантеров. Они аккуратно отделяют ростки от «ковра», сажают их на расстоянии около 20 см друг от друга и присыпают почвой.

agroclasses.svoevagro.ru
Норма высева: при посеве семенами — 60–80 кг/га, при высадке рассады — около 40 кг/га.
agroclasses.svoevagro.ru
Глубина заделки и затопление
Глубина заделки семян зависит от способа посева и сроков:
При рядовом посеве сеялками семена заделывают на глубину 1,5–2,0 см.
При разбросном посеве глубина может быть больше.

fancrd.ru
moluch.ru
После посева:
При посеве семенами чеки затапливают на 3–5 дней для намачивания семян.
После появления всходов воду временно сбрасывают, чтобы растения могли выйти на поверхность.
В период появления 1–2 листьев чеки снова заливают водой слоем 12–15 см, который поддерживают до начала восковой спелости зерна.

patents.google.com
Уход после посадки
После посадки начинается этап ухода, который включает:
Поддержание уровня воды. Уровень затопления поддерживают на отметке 10–15 см, что помогает бороться с сорняками и создаёт стабильный микроклимат для корней.
sfera.fm
Подкормки. Азотные удобрения вносят в фазы начала кущения и трубкования.
patents.google.com
Борьба с сорняками. Для этого используют гербициды, а также механические методы (боронование).
patents.google.com
Защита от вредителей и болезней. Применяют интегрированную систему, включающую севооборот, устойчивые сорта, протравливание семян и обработку посевов средствами защиты растений.
svoefermerstvo.ru
Важные нюансы
Сроки посадки определяют по температуре почвы на глубине 5–6 см, которая должна достичь 10–12 °C.
Качество семян напрямую влияет на урожайность. Не стоит использовать обычный пищевой рис — он может быть термически обработан и не прорастёт.
Равномерность посадки критически важна. Изреженные участки могут привести к снижению урожая.

moluch.ru
rudesignshop.ru
Таким образом, посадка риса — это сложный и многоэтапный процесс, требующий учёта климатических условий, типа почвы и выбора подходящего метода.
- **Conversion operator:** `tmp::operator const T&() const`
- **Constructor:** `GeometricField(const tmp<GeometricField>&)`

For copy-initialization (`Type x = expr;`), the C++ standard considers both paths. GCC resolves the ambiguity by preferring the constructor. Clang/ICX considers both paths equally valid and reports an ambiguity.

This is a defect in the C++ standard (core issue 2077), not a compiler bug.

### Affected files and lines

**File:** `vofTwoPhaseCentralFoam/vofTwoPhaseCentralFoamEqns.C` — 6 lines:

| Line | Code |
|------|------|
| 108 | `volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);` |
| 109 | `volScalarField alpha_gas_bd = 1.0 - alpha_liq_bd;` |
| 173 | `volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);` |
| 175–176 | `const surfaceScalarField Unf = phi_ / mesh.magSf();` |
| 416 | `surfaceScalarField phiCorr = linearInterpolate(rho*oneByA_)*fvc::ddtCorr(U_, phi_);` |
| 417 | `surfaceScalarField HbyAKappa = phiHbyA_ + phiCorr;` |

### Fix

Replace copy-initialization (`=`) with direct-initialization (`()`):

```diff
- volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);
+ volScalarField alpha_liq_bd(max(min(volumeFraction1_sharp_, 1.0), 0.0));

- volScalarField alpha_gas_bd = 1.0 - alpha_liq_bd;
+ volScalarField alpha_gas_bd(1.0 - alpha_liq_bd);

- volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0);
+ volScalarField alpha_liq_bd(max(min(volumeFraction1_sharp_, 1.0), 0.0));

- const surfaceScalarField Unf =
-     phi_ / mesh.magSf();
+ const surfaceScalarField Unf(
+     phi_ / mesh.magSf());

- surfaceScalarField phiCorr   = linearInterpolate(rho*oneByA_)*fvc::ddtCorr(U_, phi_);
+ surfaceScalarField phiCorr  (linearInterpolate(rho*oneByA_)*fvc::ddtCorr(U_, phi_));

- surfaceScalarField HbyAKappa = phiHbyA_ + phiCorr;
+ surfaceScalarField HbyAKappa(phiHbyA_ + phiCorr);
```

This fix:

- ✅ Does not change runtime behavior (semantically identical)
- ✅ Does not affect GCC compilation (GCC accepts both forms)
- ✅ Does not affect performance (same code generation)
- ✅ Follows the recommendation from the OpenFOAM development team (issue #3138)

### Alternative fix

Use `.cref()` to explicitly select the conversion path (as done in the preCICE adapter):

```cpp
volScalarField alpha_liq_bd = max(min(volumeFraction1_sharp_, 1.0), 0.0).cref();
```

This is more explicit but requires modifying every expression individually.

---

## 3. Root Cause: почему дело не в компиляторе Intel

### 3.1. ICX — это не «Intel-компилятор» в старом смысле

Intel ICX (oneAPI) построен на **LLVM/Clang frontend**. Старый Intel ICC (классический, до 2021 года) имел собственный frontend и **компилировал этот код без ошибок** (подтверждено в bug #717: ICC 12.1.3 собирал OpenFOAM 2.1.x без проблем). Так что дело не в том, что «Intel странный» — дело в том, что ICX ведёт себя как Clang.

### 3.2. Стандарт C++ не разрешает этот случай однозначно

**Core Issue 2077** в списке активных проблем комитета C++ (CWG) имеет статус **open** — то есть стандарт **до сих пор не специфицирует**, что должно происходить при copy-initialization, когда есть и conversion operator, и converting constructor.

### 3.3. Два кандидата

```cpp
// Путь 1: conversion operator из tmp<T>
operator const T&() const { return cref(); }

// Путь 2: constructor из tmp<T>
GeometricField(const tmp<GeometricField>&);
```

При `Type x = expr;` (copy-initialization) стандарт говорит: «рассмотрите все user-defined conversions». Оба пути — user-defined conversions. Стандарт **не даёт правила приоритета** между ними.

| Компилятор | Поведение |
|------------|----------|
| **GCC** | Добавляет нестандартный tie-breaker: предпочитает конструктор. Код компилируется. |
| **Clang/ICX** | Следует стандарту буквально: оба пути равнозначны → `ambiguous`. |

Подтверждение из Stack Overflow: *«Clang and GCC disagree on legality of direct initialization with conversion operator»* — и это связано именно с CWG 2077.

### 3.4. Архитектурная проблема OpenFOAM

Класс `tmp<T>` в OpenFOAM специально спроектирован с двумя путями конверсии:

- `operator const T&() const` — для неявного доступа к содержимому.
- `GeometricField(const tmp<GeometricField>&)` — для конструирования из `tmp`.

Это создаёт **неоднозначность по дизайну**. На Clang-подобных компиляторах любой `Type x = expr;`, где `expr` возвращает `tmp<T>`, будет `ambiguous`. Это не баг конкретной строки — это **системная проблема API**, которая проявляется только на компиляторах, строго следующих стандарту.

### 3.5. Что означает комментарий разработчика

> «Мы тестируем только с GCC, и я впервые вижу эту ошибку.»

Это **не значит** «код правильный, компилятор Intel неправильный». Это значит: **мы никогда не запускали код на компиляторе, который сообщает об этой проблеме**. Проблема существует с 2012 года (bug #510), была частично исправлена в ядре OpenFOAM 2.2.0 (bug #717, 2013), но **снова появилась в v2312** (issue #3138, 2024) — потому что в пользовательском коде и сторонних солверах copy-initialization от `tmp<T>` остался неисправленным.

---

## 4. Подтверждение из источников

### OpenFOAM bug #717 (2013)

```
cylindricalInletVelocityFvPatchVectorField.C:135:23:
error: conversion from 'tmp<Field<...>>' to 'const vectorField' is ambiguous

tmp.H:121:16: note: candidate function
    inline operator const T&() const;

Field.H:186:9: note: candidate constructor
    Field(const tmp<Field<Type>>>&);
```

Компилятор: Clang 3.3. GCC 4.7.2 и Intel ICC 12.1.3 собирали без ошибок. Фикс: **правка ядра OpenFOAM 2.2.0** — copy-initialization заменён на direct-initialization во всех проблемных местах. Баг закрыт как resolved/fixed.

### OpenFOAM issue #3138 (2024)

```
error: conversion from 'tmp<GeometricField<double, fvPatchField, volMesh>>'
to 'GeometricField<double, fvPatchField, volMesh>' is ambiguous
```

**Решение от @mark (Mark Olesen, ведущий разработчик OpenFOAM):** *«wrapping the return type with Type»* — то есть direct-initialization.

### preCICE adapter (2024–2025)

Пользователь собирал OpenFOAM v2406 с Intel oneAPI 2023.2. Та же ошибка:

```
error: conversion from 'tmp<vectorField>' to 'vectorField' is ambiguous
```

Решение от разработчика adapter'а (MakisH, март 2025): использование `.cref()`:

```cpp
vectorField n = U_->boundaryField()[patchID].patch().nf().cref();
```

### gerlero/openfoam-app #87 (2022, macOS/Apple Clang)

Пользователь на macOS M1 (Apple Clang) получил ту же ошибку:

```
error: conversion from 'tmp<Field<double>>' to 'Foam::scalarField' is ambiguous
```

### OpenFOAM wiki: Coding Patterns — Memory (Mark Olesen, 2021)

В wiki OpenFOAM Mark Olesen описывает рекомендуемые паттерны работы с `tmp<T>`, рекомендуя direct-initialization.

---

## 5. Оценка багрепорта

### ✅ Что подтверждено источниками

**1. Корневая причина описана верно.**

В OpenFOAM класс `tmp<T>` действительно предоставляет два пути конверсии в `T`:

- **Оператор преобразования:** `tmp::operator const T&() const` — возвращает `const`-ссылку на внутренний объект.
- **Конструктор:** `GeometricField(const tmp<GeometricField>&)` — конструирует поле из `tmp`.

При copy-initialization (`Type x = expr;`) стандарт C++ рассматривает оба пути как равнозначные кандидаты. GCC разрешает неоднозначность в пользу конструктора, а Clang/ICX — нет, и сообщает об ошибке. Это подтверждается:

- **OpenFOAM issue #3138** — идентичная ошибка с Clang 15.0.7, та же формулировка про `tmp<GeometricField<...>>` → `GeometricField<...>` ambiguous. Разработчик `@mark` предложил тот же фикс — обернуть возвращаемый тип.
- **OpenFOAM bug #717** — та же проблема ещё в 2013 году с Clang 3.3, для OpenFOAM 2.1.x. Та же структура ошибки: `operator const T&()` vs `Field(const tmp<Field<Type>>>&)`. Закрыта как fixed в 2.2.0.
- **preCICE adapter** — разработчик MakisH (март 2025) применил фикс `.cref()` для той же ошибки с Intel oneAPI 2023.2 + OpenFOAM v2406. PR: *«Avoid ambiguous type conversion when accessing `tmp<vectorField>`»*.

**2. Упоминание C++ core issue 2077 обосновано.**

В списке активных проблем комитета C++ действительно есть issue, связанный с copy-initialization и conversion functions. Clang и GCC расходятся в трактовке copy-initialization, когда есть и conversion operator, и converting constructor — это известный дефект стандарта, а не баг конкретного компилятора.

**3. Фикс (direct-initialization) правильный и общепринятый.**

Замена `Type name = expr;` на `Type name(expr);` — это стандартное решение, которое:

- используется в самом OpenFOAM после фикса #717,
- рекомендовано разработчиками OpenFOAM в issue #3138,
- семантически идентично (не меняет поведение),
- не ломает сборку под GCC.

**4. Альтернативный фикс через `.cref()` тоже верный.**

preCICE-адаптер использовал именно `.cref()`, и это работает. Но `.cref()` возвращает `const T&`, что не подходит, если нужен неконстантный объект. Direct-initialization универсальнее.

**5. Затронутые строки и файлы — корректные.**

Строки 108, 109, 173, 175–176, 416, 417 в `vofTwoPhaseCentralFoamEqns.C` и строки 147–149, 206–208 в `leastSquaresStencil.C` — все содержат именно copy-initialization от выражений, возвращающих `tmp<...>`. Это согласуется с паттерном, описанным в issue #3138.

### ⚠️ Что стоит уточнить

**1. «This is a defect in the C++ standard (core issue 2077), not a compiler bug.»**

Это формально верно, но с оговоркой. Issue 2077 помечен как **drafting** (в процессе проработки), а не как resolved. То есть стандарт ещё не зафиксировал однозначное поведение. Корректнее сказать: *«стандарт не специфицирует этот случай однозначно, и компиляторы расходятся в трактовке»*.

**2. Версия oneAPI 2026.1.1**

Багрепорт указывает очень свежую версию (2026.1.1.20260724, LLVM 18+). В источниках подтверждена та же проблема на:

- Clang 3.3 (2013)
- Clang 15.0.7 (2024)
- Intel oneAPI 2023.2 (2024)
- Clang на macOS (Apple LLVM)

Так что проблема существует уже **13+ лет** и не зависит от конкретной версии — она структурная, связанная с дизайном `tmp<T>` в OpenFOAM и трактовкой стандарта C++ компиляторами на базе LLVM.

**3. Упоминание QGDsolver (`leastSquaresStencil.C`)**

В багрепорте сказано, что та же проблема затрагивает QGDsolver. Это правдоподобно — любой код, использующий copy-initialization от выражений, возвращающих `tmp<GeometricField<...>>` или `tmp<Field<...>>`, будет давать ту же ошибку на Clang/ICX. Но в найденных источниках конкретно `leastSquaresStencil.C` не упоминается — это собственный опыт автора (мой опыт - Синьков Николай Леонидович).

---

## 6. Оценка скрипта-патча

Python-скрипт, меняющий copy-init на direct-init — это **правильный и достаточный фикс** именно для этой проблемы. Он:

- ✅ решает root cause (убирает ambiguous conversion),
- ✅ не требует правки ядра OpenFOAM (только пользовательский код),
- ✅ идентичен по подходу к тому, как сами разработчики OpenFOAM фиксят эту проблему.

Единственное, что стоит добавить в скрипт — **проверку на MISMATCH с прерыванием**, чтобы при смещении строк (после обновления кода) скрипт не молча пропустил неравенства:

```python
mismatch = False
for lineno, old, new in replacements:
    idx = lineno - 1
    if lines[idx] == old:
        lines[idx] = new
        print(f"  Line {lineno}: OK")
    else:
        print(f"  Line {lineno}: MISMATCH!")
        mismatch = True

if mismatch:
    print("Aborting: some lines did not match. File NOT saved.")
    exit(1)

with open(path, 'w') as f:
    f.writelines(lines)
print("Done.")
```

Так избегаем ситуации, когда скрипт «отработал», но ничего не заменил.PS скрипт пока не менял... находил поиском и в редакторе nano изменял текст вручную (копипаста->поиск->замена=проверка) и проверял.

---

## 7. Сводная таблица всех случаев

| Год | Источник | Компилятор | Версия OpenFOAM | Фикс |
|-----|----------|------------|-----------------|------|
| 2012 | bug #510 | Clang | 2.1.x | Правка ядра (copy → direct init) |
| 2013 | bug #717 | Clang 3.3 | 2.1.x | Fixed в 2.2.0 |
| 2022 | openfoam-app #87 | Apple Clang (M1) | v2206, v2112 | Не решён в issue |
| 2024 | issue #3138 | Clang 15.0.7 | v2312 (dev) | `@mark`: «wrap return type» (direct-init) |
| 2024–2025 | preCICE adapter | Intel oneAPI 2023.2 | v2406 | `.cref()` |
| 2026 | Данный багрепорт | Intel ICX 2026.1.1 | v2312 | Direct-init (Python-скрипт) |

---

## 8. References

1. **OpenFOAM issue #3138:** Ambiguous type conversion when using Clang 15.0 — [develop.openfoam.com/Development/openfoam/-/issues/3138](https://develop.openfoam.com/Development/openfoam/-/issues/3138)
2. **OpenFOAM bug #717:** Clang fails to build finiteVolume — [bugs.openfoam.org/view.php?id=717](https://bugs.openfoam.org/view.php?id=717)
3. **preCICE adapter:** Avoid ambiguous type conversion — [github.com/precice/openfoam-adapter](https://github.com/precice/openfoam-adapter)
4. **C++ core issue 2077:** Clang/GCC disagree on copy-initialization — [stackoverflow.com/questions/40006925](https://stackoverflow.com/questions/40006925/clang-and-gcc-disagree-on-legality-of-direct-initialization-with-conversion-oper)
