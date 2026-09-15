# hybridCentralSolvers

Объединенная коллекция гибридных центральных решателей — однофазных, двухфазных и многокомпонентных версий.

При использовании этих решателей, пожалуйста, цитируйте следующие работы:

* Крапошин М.В., Банхольцер М., Пфицнер М., Марчевский И.К. Гибридный решатель на основе давления для неидеальных однофазных потоков жидкости на всех скоростях. Int J Numer Meth Fluids. 2018;88:79–99. https://doi.org/10.1002/fld.4512
* Крапошин М. В., Стрижак С. В., Бовтрикова А. Адаптация численной схемы Курганова-Тадмора для применения в сочетании с методом PISO при численном моделировании течений в широком диапазоне чисел Маха. Procedia Computer Science. 2015;66:43-52. https://doi.org/10.1016/j.procs.2015.11.007

---

### Что сделано

* **Устранена ошибка ambiguous `tmp<T>` conversion** — 12 строк copy-initialization (`Type x = expr;`) заменены на direct-initialization (`Type x(expr);`) в `vofTwoPhaseCentralFoamEqns.C` (6 строк) и `leastSquaresStencil.C` (6 строк).
* **Корневая причина** — дефект дизайна `tmp<T>` в OpenFOAM (два пути неявной конверсии) в сочетании с CWG 2077 (стандарт C++ не специфицирует приоритет). GCC разрешает неоднозначность нестандартным tie-breaker; Clang/ICX следуют стандарту и сообщают об ошибке.
* **Фикс семантически идентичен** — не меняет поведение, не влияет на производительность, не ломает сборку под GCC. Соответствует рекомендации разработчиков OpenFOAM (issue #3138).
* **Проблема подтверждена источниками** за период 2012–2025: OpenFOAM bug #717, issue #3138, preCICE adapter.

---
*  Обсуждение: https://vk.ru/wall-39301819_938

## Совместимость с Intel ICX (oneAPI)

Коллекция адаптирована для сборки компилятором Intel ICX (oneAPI 2026.1, LLVM-based) в дополнение к GCC. Подробнее об изменениях, причинах и применённых патчах — в отчёте: [Compiler_OpenFOAM-Version_Fix.md](../Compiler_OpenFOAM-Version_Fix.md)


### Сборка

Файл [log.icpx_v2312_2026-09-09.log](log.icpx_v2312_2026-09-09.log) — полный журнал компиляции `hybridCentralSolvers` компилятором Intel ICX (oneAPI 2026.1.1) на OpenFOAM v2312. Содержит:

* версию компилятора и LLVM backend;
* флаги компиляции (`linux64IcxDPInt64Opt`);
* переменные окружения и пути к библиотекам oneAPI;
* полный вывод компилятора по всем файлам, включая исправленные строки;
* результат сборки — успех после применения патчей.


### Лог сборки (справочный артефакт)

📜 **Лог сохранён как справочный артефакт:** он фиксирует точное состояние окружения, на котором была воспроизведена и решена проблема `ambiguous tmp<T>` conversion.

⚠️ **Это не инструкция по сборке.** Это доказательство того, что после патчей код компилируется ICX без ошибок.

📄 Файл: [log.icpx_v2312_2026-09-09.log](log.icpx_v2312_2026-09-09.log) — содержит полный вывод компиляции, флаги и переменные окружения.

## Каналы обсуждения

- **Обсуждение проекта и совместимости с ICX** (в русскоязычном OpenFOAM‑сообществе): [VK: wall‑39301819_938](https://vk.ru/wall-39301819_938)
  Здесь — общий контекст исследования, как солверы ведут себя при сборке под Intel oneAPI и на больших сетках.
- **Вопросы по коду, баги, фичи, PR**: [GitHub Discussions](https://github.com/kol1978/Backward-Facing/discussions)
  Технический канал для обсуждения патчей, флагов компиляции и исправлений (например, `tmp<T>` ambiguity).

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Источник: https://github.com/unicfdlab/hybridCentralSolvers?ysclid=mtwzgak09x498010334
# Содержание

1. [Доступные решатели с гибридной аппроксимацией](#Available-solvers-with-hybrid-approximation)
2. [Точки соприкосновения пользователей и разработчиков](#Meeting-points-for-users-and-developers)
3. [Доступные версии OpenFOAM](#Available-OpenFOAM-versions)
4. [Производные проекты](#Derived-projects)
5. [Исследования, в которых была полезна библиотека](#Research-studies-where-the-library-was-useful)
6. [Для цитирования](#For-citations)

# Доступные решатели с гибридной аппроксимацией
[К содержанию](#Contents)

Объединенная коллекция гибридных центральных решателей на основе центрально-поперечных схем Курганова и Тадмора и поддержки LTS для расчетов стационарных режимов: однофазные, двухфазные и многокомпонентные версии.

С 2018 года поддерживается только версия OpenFOAM+ технологии OpenFOAM. Фреймворк содержит следующие решатели:

 1. Решатели для сжимаемых однофазных потоков:
 - **pimpleCentralFoam** — полунеявный решатель на основе давления для сжимаемых потоков идеального газа;
 - **rhoPimpleCentralFoam** — полунеявный решатель на основе давления для сжимаемых потоков реального газа;
 - ** pimpleCentralDyMFoam ** - полуявный решатель на основе давления для сжимаемого потока идеального газа с движением сетки и AMR;
 - ** chtMultiRegionCentralFoam ** - полуявный решатель на основе давления для сопряженного моделирования идеального потока сжимаемого газа (Маха
 число находится в диапазоне от 0 до 6) и теплопередача твердого тела.
2. Многокомпонентные решатели:
 - **reactingPimpleCentralFoam** — полунеявный решатель на основе давления для сжимаемых потоков с горением и химическими реакциями;
 - **reactingPimpleCentralDyMFoam** — полунеявный решатель на основе давления для сжимаемых потоков с горением, химическими реакциями и динамической сеткой (вкл. AMI или AMR);
 - **reactingLagrangianPimpleCentralFoam** — полунеявный решатель на основе давления для сжимаемых потоков с горением, движением частиц, фазовыми переходами и химическими реакциями.
3. Многофазные решатели:
 - **vofTwoPhaseCentralFoam** — улучшенная версия (начиная с OpenFOAM+ 2312) решателя **interTwoPhaseCentralFoam**, использующая объемные потоки для переноса (повышенная надежность).
 - **interTwoPhaseCentralFoam** - решатель на основе давления для сжимаемых (0-4 числа Маха) потоков двухфазных сред с учетом вязкости и силы тяжести. Решатель использует метод Вольфа для определения границы раздела фаз и метод ACID ( [https://doi.org/10.1016/j.jcp.2018.04.028]( https://doi.org/10.1016/j.jcp.2018.04.028)) для расчета свойств в области, где присутствуют обе фазы.
 - **twoPhaseMixingCentralFoam** - двухфазный решатель на основе метода Эйлера. Жидкость и газ рассматриваются как сжимаемые среды. Массообмен на границе раздела фаз не учитывается.
 - **twoPhaseMixingCentralDyMFoam** - двухфазный решатель на основе метода Эйлера с динамическими сетками. Жидкость и газ рассматриваются как сжимаемые среды. Массообмен на границе раздела фаз не учитывается.

# Точки соприкосновения пользователей и разработчиков
[К содержанию](#Contents)

# Доступные версии OpenFOAM
[К содержанию](#Contents)

Библиотека доступна для следующих версий OpenFOAM:
* OpenFOAM 3.1 — [основная ветка](https://github.com/unicfdlab/hybridCentralSolvers/tree/master)
* OpenFOAM 4.1 — [ветка dev-of4.1](https://github.com/unicfdlab/hybridCentralSolvers/tree/dev-of4.1)
* OpenFOAM 6 — [ветка dev-of6](https://github.com/unicfdlab/hybridCentralSolvers/tree/dev-of6)
* OpenFOAM+ 1812 - [digitef-dev-1812](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-1812)
* OpenFOAM+ 1912 - [digitef-dev-1912](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-1912)
* OpenFOAM+ 2012 — [digitef-dev-2012](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2012)
* OpenFOAM+ 2112 — [digitef-dev-2112](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2112)
* OpenFOAM+ 2212 — [digitef-dev-2212](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2212)
* OpenFOAM+ 2312 — [digitef-dev-2312](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2312)
* OpenFOAM+ 2412 — [digitef-dev-2412](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2412)
* OpenFOAM+ 2512 — [digitef-dev-2512](https://github.com/unicfdlab/hybridCentralSolvers/tree/digitef-dev-2512)

**Последние изменения и исправления ошибок применяются только в ветках, соответствующих последней версии OpenFOAM.**

# Производные проекты
[К содержанию](#Contents)

Библиотека или подход использовались в следующих проектах:
* [multiRegionRectingPimpleCentralFoam](https://github.com/TonkomoLLC/hybridCentralSolvers/tree/master/OpenFOAM-4.1/multiRegionReactingPimpleCentralFoam) — решатель для совместного моделирования газовой динамики и теплопередачи с использованием гибридного приближения KT/PIMPLE для конвективных потоков
* [adjointReactingRhoPimpleCentralFoam](https://github.com/clapointe2011/public/tree/master/discreteAdjointOpenFOAM/applications/solvers/adjoint/adjointReactingRhoPimpleCentralFoam) — решатель для сопряжённой оптимизации формы области с газовым потоком, смоделированным с использованием гибридной аппроксимации конвективных потоков KT/PIMPLE
* [HLLCFoam](https://github.com/AAShevelev/HLLCFoam) — решатель для динамики идеального газа, использующий гибридный подход HLLC/PIMPLE (расширение гибридной схемы KNP/PIMPLE для приближенного решателя HLLC).


# Исследования, в которых использовалась библиотека
[К содержанию](#Contents)

Если вы хотите, чтобы ваше исследование было включено в этот список, напишите в [раздел «Проблемы»](https://github.com/unicfdlab/hybridCentralSolvers/issues).

## <p align="center"> >>>>> 2026 <<<<< </p>
| Название | Описание |
|------|-------------|
|Численный анализ новой концепции воздухозаборника для трансзвуковой воздушно-реактивной двигательной установки: **докторская диссертация**| --- |
|Механизм эволюции ударных волн, вызванных мгновенным испарением, и регулирование взаимодействия струй в реактивных двигателях высокого давления: **докторская диссертация**| --- |
|[Численное исследование влияния впрыска воды на температуру осевой линии струи и избыточное давление воспламенения с помощью OpenFOAM](https://arc.aiaa.org/doi/abs/10.2514/6.2026-0950): **Статья**| --- |
|[На пути к эффективному моделированию горения в широком диапазоне скоростей: модель ANN-FGM и ее валидация](https://www.sciencedirect.com/science/article/abs/pii/S1270963826002762): **Статья**| --- |
|[Влияние предварительного нагрева основной топливной струи прямоточного воздушно-реактивного двигателя с эжектором на его характеристики](https://arc.aiaa.org/doi/abs/10.2514/1.B40070): **Статья**| --- |
|[Обзор конвективных схем, используемых для моделирования детонации в OpenFOAM, после десяти лет разработки](https://www.mdpi.com/2075-1680/15/4/282): **Статья**|![ Типичная детонация в разные моменты времени](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/axioms-15-00282-g002.png)|
|[АДАПТАЦИЯ УСОВЕРШЕНСТВОВАННЫХ ЧИСЛЕННЫХ СХЕМ ДЛЯ РЕШЕНИЯ ЗАДАЧ СЖИМАЕМОЙ СРЕДЫ НА ОСНОВЕ ДАВЛЕНИЯ В OPENFOAM](https://cdn.apub.kr/journalsite/sites/kscfe/2026-031-01/N0500310103/N0500310103.pdf): **Статья**|![Изобары](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/kim-pressure-contours.png)|
|[Моделирование недорасширенных струй аммиака для применения в перспективных двигательных установках](https://www.sciencedirect.com/science/article/pii/S0017931025016023): **Статья**| ![Визуализация потока в эксперименте](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0017931025016023-gr5.jpg) |
|[Влияние неопределенности скорости реакции на динамику двумерной детонации](https://www.sciencedirect.com/science/article/abs/pii/S0010218025007667): **Статья**| --- |
|[Гибридная модель на основе давления для до- и сверхзвукового сжимаемого двухфазного потока с неравновесным фазовым переходом](https://www.researchgate.net/publication/397581917_A_pressure-based_hybrid_framework_for_sub-_and_supersonic_compressible_two-phase_flow_with_non-equilibrium_phase_change): **Статья**| ![Распределение числа Маха в струе мгновенного вскипания](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/2026-01-01_15-13.png)|


## <p align="center"> >>>>> 2025 <<<<< </p>
| Название | Описание |
|------|-------------|
| На пути к точному численному прогнозированию детонаций: **докторская диссертация**| --- |
|[Численный анализ шума реактивного двигателя с числом Маха 0,75 с использованием метода моделирования отсоединенных вихрей](http://jvs.isav.ir/article_724358.html?lang=en&lang=en): **Статья**| --- |
|[Численное исследование цилиндрических капель воды, подвергающихся ударной нагрузке при высоком числе Вебера](https://www.mdpi.com/2311-5521/10/4/81)|![Тандемная конфигурация по размаху: численные контуры Шлирен-картины](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/fluids-10-00081-g016.png)|
|[CFD-прогнозирование аэробного разрушения тандемных водяных колонн с использованием кодов с открытым исходным кодом](https://link.springer.com/chapter/10.1007/978-3-031-97000-9_3): **Статья**| --- |
|[Оценка эффективности модели турбулентности RANS в сочетании с решателем на основе давления для сверхзвукового потока](https://doi.org/10.1063/5.0286015)| --- |
|[Визуальная и количественная оценка точности решателя OpenFOAM при моделировании наклонного ударного слоя](http://sv-journal.org/2025-4/02/): **Статья**|![Схема течения в сужающемся канале](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/shock-train-pattern.png)|
|[Применение метода сингулярного разложения и автокодировщика для сверхзвукового обтекания обратного уступа](https://avestia.com/FFHMT2025_Proceedings/files/paper/FFHMT_170.pdf): **Статья**|![Четыре сингулярных вектора для u-компоненты на последнем временном шаге](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/2026-01-01_15-33.png)|
|[Оценка инструментов с разной степенью точности для аэродинамического анализа маломасштабного сверхзвукового беспилотного летательного аппарата](https://ucalgary.scholaris.ca/items/771b2a53-0587-4fb6-a3b5-ed95a4fd87b6): **магистерская диссертация**|![MUFASA B. Адаптировано из (Fyfe, 2025)](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/2026-01-01_15-23.png)|
|[Оценка моделирования турбулентности с учетом эффектов сжимаемости в прямоточных воздушно-реактивных двигателях с эжектором](https://ucalgary.scholaris.ca/items/771b2a53-0587-4fb6-a3b5-ed95a4fd87b6) **Магистерская диссертация**|![Распределение поля турбулентной кинетической энергии вдоль смесительной трубы](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/2026-01-01_15-28.png)|
|[Сверхзвуковое горение этилена в конфигурации «пилон-полость» с изогнутыми пилонами](https://doi.org/10.1080/00102202.2025.2491102): **Статья**| --- |
|[Оценка современных схем улавливания ударных волн для высокоскоростных потоков в рамках OpenFOAM](https://arxiv.org/abs/2510.24146): **Статья**| --- |
|[Рециркуляция и унос эжектора](https://doi.org/10.2514/1.B39246): **Статья**| --- |
|[Численное исследование сверхзвукового двухкомпонентного струйного течения](https://doi.org/10.1063/5.0291051): **Статья**|---|
|[Экспериментально-численное сравнение детонаций водорода в воздухе: влияние химических реакций и эффектов диффузии азота](https://www.mdpi.com/2226-4310/12/4/297): **Статья**|![Схематическое изображение расчетной области](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/aerospace-12-00297-g002.png)|
|[Исследование аэродинамики с помощью открытых программных комплексов в области течения с переменным временем](https://doi.org/10.17341/gazimmfd.1156600): **Статья**|![Турбулентный поток над аэродинамическим профилем](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Ekinci-Zafer.png)|
|[Модификация показателя преломления и плазменные характеристики нити, индуцированной фемтосекундным лазером, в азоте](http://iopscience.iop.org/article/10.1088/1361-6463/adb498): **Статья**|![Изменение показателя преломления для зондирующего лазерного импульса с длиной волны 532 нм](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/RefrIndex.png)|
|[Оценка химико-кинетических моделей для моделирования водородных взрывов путем сравнения с экспериментальными данными](https://www.sciencedirect.com/science/article/pii/S2666352X2400061X): **Статья**|![пик термичности](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/1-s2.0-S2666352X2400061X-gr12.jpg)|


## <p align="center"> >>>>> 2024 <<<<< </p>
| Название | Описание |
|------|-------------|
|[Моделирование турбулентного горения в эжекторной прямоточной воздушно-реактивной двигательной установке](https://prism.ucalgary.ca/items/d01ad206-7b26-4711-8f52-231407a8bfd0): **магистерская диссертация**|![Поле средней температуры, полученное с помощью метода RANS в режиме горения PaSR по умолчанию](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/T_RANS_PASR.png)|
|[Исследование механизма дробления капель под действием ударной волны и детонации на основе гибридных решателей](https://www.researchgate.net/publication/382754845_Study_of_the_mechanism_of_shock-induced_and_detonation-induced_droplet_breakup_based_on_hybrid_solvers): **Статья**|---|
|[Улучшение аэроакустических характеристик при старте ракет-носителей](https://riunet.upv.es/handle/10251/204805?show=full): **докторская диссертация**|![Геометрия и сетка воздуховода](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/duct-geom-and-mesh.png)|
|[Проверка высокоскоростного решателя реактивных потоков в OpenFOAM с детальным химическим моделированием](https://journal.openfoam.com/index.php/ofj/article/view/125): **Статья**|![Детонационные ячейки](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/submission_resized.png)|
|[Реально-жидкостный низкодиссипативный решатель для моделирования мгновенного испарения неравновесных смесей](https://www.sciencedirect.com/science/article/pii/S0017931024002229): **Статья**|![Визуализация распыления пропана: сравнение эксперимента с текущими расчетами](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/propane_spray.jpg)|

## <p align="center"> >>>>> 2023 <<<<< </p>
| Название | Описание |
|------|-------------|
|[Экспериментальное и численное сравнение слабо неустойчивой детонации с использованием планарной лазерной флуоресцентной визуализации оксида азота](http://www.icders.org/ICDERS2023/abstracts/ICDERS2023-093.pdf): **Статья**|![NO-PLIF: эксперимент и численные результаты](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/NO-PLIF_exp_vs_num.png)|
|[Исследование механизма ударного разрушения капель на основе гибридного решателя](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=4676348): **Статья**|![Количество капель в зависимости от числа Маха](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/droplet_breakup_12022024.png)|
|[Моделирование распространения тепла и влаги в каналах с препятствиями: волнистые каналы в сравнении с препятствиями типа «забор»](https://www.researchgate.net/publication/378431094_Simulation_of_DDT_in_obstructed_channels_wavy_channels_vs_fence-type_obstacles): **Статья**|![Распределение температуры в каналах разного профиля](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Wavy-vs-Fenced-T.png)|
|[Влияние формы дефлектора на генерацию и распространение аэроакустического шума](https://www.sciencedirect.com/science/article/abs/pii/S0094576523004599): **Статья**|![Акустическое давление вокруг ракеты](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0094576523004599-gr5.jpg)|
|[Численное моделирование шума сверхзвукового реактивного двигателя с использованием программного обеспечения с открытым исходным кодом](https://link.springer.com/chapter/10.1007/978-3-031-36030-5_24): **Статья**|---|
|[Дифракция и повторное инициирование детонационной волны в предварительно перемешанной смеси H2–O2–Ar](https://pubs.aip.org/aip/pof/article/35/9/095109/2909845): **Статья**| ![Распределение ячеек в случаях с разным соотношением D/d](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/BoZhan_difraction.jpeg) |
|[Численное исследование вынужденных нелинейных акустических колебаний газа в трубе под действием двух поршней со сдвигом фаз](https://www.sciencedirect.com/science/article/abs/pii/S0165212522000373): **Статья**|![Эскиз резонатора с двумя поршнями](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/two_pistons_sketch.png): **Статья**|
|[Самосогласованная модель и численный подход для описания неравновесной плазмы, вызванной лазерным излучением](https://pubs.aip.org/aip/jap/article-abstract/134/22/223301/2929689/Self-consistent-model-and-numerical-approach-for?redirectedFrom=fulltext): **Статья**|![Архитектура решателя плазмы](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/plasmaSolver_arch.png)|
|[О разрешении ошибок аппроксимации в ансамбле численных решений](https://link.springer.com/chapter/10.1007/978-3-031-36030-5_51): **Статья**|---|
|[Численный и экспериментальный анализ самовоспламенения, вызванного фокусировкой ударной волны](http://www.icders.org/ICDERS2023/abstracts/ICDERS2023-037.pdf): **Статья**|![Схематическое изображение экспериментальных установок](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Autoignition_exp_facility.png)|
|[Исследования впрыска водорода с использованием подхода на основе реальных жидкостей](https://doi.org/10.4271/2023-01-0312): **Статья**|![Скорость идеального газа и реальной жидкости, а также массовая доля газа](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/F-Rahantamialisoa-jet.png)|
|[Численные исследования псевдокипения и многокомпонентного смешения в транс- и сверхкритических условиях для применения в двигателях](https://doi.org/10.1080/00102202.2023.2214947): **Статья**|---|
|[Численный и экспериментальный анализ детонации, вызванной фокусировкой ударной волны](https://www.researchgate.net/publication/369020856_Numerical_and_experimental_analysis_of_detonation_induced_by_shock_wave_focusing): **Статья**|![Сравнение экспериментальных данных и результатов численного моделирования](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Zezhong_Yang_Figure3.png)|
|[Validation and Verification of reactingPimpleCentralFOAM for Ejector Ramjet Applications](https://www.researchgate.net/publication/367311913_Validation_and_Verification_of_reactingPimpleCentralFOAM_for_Ejector_Ramjet_Applications): **Статья**|![Схема вычислительной области](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Volvo_Schematic.png)|
|[Формирование пучка для лазерной абляции в воздухе](https://www.researchgate.net/publication/367312214_Beam_Shaping_for_the_Laser_Energy_Deposition_in_Air): **Статья**|---|
|[Анализ колебаний, вызванных сверхзвуковой струей, используемой для получения нановолокон](https://doi.org/10.1016/j.ijmecsci.2022.107826): **Статья**|![Изображение процессов выдувания расплава и кофибласа](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0020740322007068-gr1_lrg.jpg)|

## <p align="center"> >>>>> 2022 <<<<< </p>

| Название | Описание |
|------|-------------|
|[Расчет профиля скорости и экспериментальные наблюдения при импульсной инжекции газа в камеру ПФ (на русском языке) Расчеты профиля плотности при импульсной инжекции рабочего газа в камеру ПФ и экппериментальные результаты](https://sciencejournals.ru/view-article/?j=fizplaz&y=2022&v=48&n=11&a=FizPlaz2260111Lototskii): **Статья**|![Поле газовой динамики внутри пластины камеры PF](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Galanin-et-al.png)|
|[Аэротермодинамический анализ экспериментальной ракеты, предназначенной для тестирования технологий микроракетных носителей](https://ubibliorum.ubi.pt/handle/10400.6/13027): **магистерская диссертация**|![Ракета с шлейфом](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/ValeSimoes.png)|
|[CFD-моделирование недорасширенных водородных струй при условиях инжекции высокого давления](https://www.researchgate.net/publication/366521065_CFD_simulations_of_under-expanded_hydrogen_jets_under_high-pressure_injection_conditions): **Статья**|![Распределение температуры в различных струях](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/Underexp-jet-temp.png)|
|[Устойчивое вращение скачка Маха: экспериментальные и численные доказательства](https://hal.archives-ouvertes.fr/hal-03867085/): **Статья**|![Численные теневые изображения скачков](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/rotating_mach_shock.png)|
|[Моделирование больших вихрей в дозвуковых и сверхзвуковых потоках с использованием гибридного решателя на основе давления](http://117.232.118.84/handle/123456789/253): **магистерская диссертация**|![Мгновенное поле скоростей струи](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/jet_Uinst.png)|
|[Валидация и верификация pimpleCentralFOAM и решателя 1D-ERAM для анализа прямоточного воздушно-реактивного двигателя с эжектором](https://www.researchgate.net/publication/361452262_Validation_and_Verification_of_pimpleCentralFOAM_and_a_1D-ERAM_Solver_for_Analysis_of_a_Ejector-Ramjet): **Статья**|![Схема системы водозабора](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/intake-sys.png)|
|[Реализация алгоритма PIMPLE высокого порядка для анализа влияния сжимаемости трансзвукового крыла методом временных сечений с предварительной подготовкой на высоких числах Маха](https://www.researchgate.net/publication/360633506_Implementation_of_Higher-order_PIMPLE_Algorithm_for_Time_Marching_Analysis_of_Transonic_Wing_Compressibility_Effects_with_High_Mach_Pre-conditioning): **Статья**|![Высокоскоростные обтекающие линии вокруг крыла самолета](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/wing_streamlines.png) |
|[Расширение системы решений на основе давления для всех чисел Маха при численном моделировании двухфазных потоков с границей раздела](https://www.researchgate.net/publication/360962690_An_extension_of_the_all-Mach_number_pressure-based_solution_framework_for_numerical_modelling_of_two-phase_flows_with_interface): **Статья**|![Сравнение экспериментальных и расчетных шлирен-картин для случая взаимодействия взрыва и капель](https://github.com/mkraposhin/hybridCentralSolvers/blob/master/Figs/blastToDroplet.png)|
|[Численное моделирование вынужденных акустических колебаний газа с большой амплитудой в замкнутой трубе](https://www.researchgate.net/publication/360583545_Numerical_simulation_of_forced_acoustic_gas_oscillations_with_large_amplitude_in_closed_tube): **Статья**|![Сравнение расчетных и экспериментальных данных](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0165212522000373-ga1_lrg.jpg)|
Динамика токовой оболочки в самосжимающемся плазменном заряде с дополнительной закачкой газа [Электронный ресурс, Динамика токовой оболочки в самосжимающемся плазменном разряде с дополнительной инжекцией газа](http://vant.iterru.ru/vant_2022_1/12.pdf ) [На английском языке: Расчеты профиля плотности для импульсного впрыска рабочего газа в камеру PF и экспериментальные результаты](https://link.springer.com/article/10.1134/S1063780X22601201): **Статья** |![Массовая доля D2 в камере](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/D2_massfraction.png)|
|[Анализ аэроакустической среды ракеты-носителя с помощью метода URANS](https://www.researchgate.net/publication/359494772_URANS_Analysis_of_a_Launch_Vehicle_Aero-Acoustic_Environment): **Статья**|![Схема шумового излучения](https://www.mdpi.com/applsci/applsci-12-03356/article_deploy/html/images/applsci-12-03356-g001.png)|
|[Исследование влияния сетки на моделирование ракетного шлейфа](https://www.researchgate.net/publication/358555519_A_study_of_the_mesh_effect_on_rocket_plume_simulation): **Статья**|![Газовый шлейф после выхода из сопла](https://ars.els-cdn.com/content/image/1-s2.0-S2590123022000366-gr2.jpg)|
|[Анализ воспламенения, вызванного фокусировкой ударной волны, с использованием конических и полусферических отражателей](https://www.researchgate.net/publication/355077511_Analysis_of_the_ignition_induced_by_shock_wave_focusing_equipped_with_conical_and_hemispherical_reflectors): **Статья** |![Распределение температуры](https://ars.els-cdn.com/content/image/1-s2.0-S001021802100506X-gr5.jpg)|
|[Трехмерные эффекты при двухимпульсном лазерном осаждении](https://www.researchgate.net/publication/357597475_Three-dimensional_Effects_in_Dual-pulse_Laser_Energy_Deposition): **Статья**|![Плазменный импульс](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/axis_timed_shc.png)|


## <p align="center"> >>>>> 2021 <<<<< </p>

| Название | Описание |
|------|-------------|
|[Эйлеровско-лагранжианский подход к численному исследованию акустического поля, создаваемого высокоскоростным газокапельным потоком](https://www.mdpi.com/2311-5521/6/8/274): **Статья**| ![Логотип струи с частицами](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/fluids-06-00274-ag.png) |
|[Численное исследование термоакустических волн в полости при быстром нагреве стенки](https://www.researchgate.net/publication/355012423_Numerical_Study_of_Thermoacoustic_Waves_in_a_Cavity_under_Rapid_Wall_Heating): **Статья** |![Изменение составляющей скорости ](https://media.springernature.com/lw685/springer-static/image/art%3A10.1134%2FS1995080221090122/MediaObjects/12202_2021_6494_Fig3_HTML.png?as=webp)|
|[Эффекты реального газа и однофазная неустойчивость при впрыске, смешивании и сгорании в условиях высокого давления](https://www.researchgate.net/publication/354224374_Real-Gas_Effects_and_Single-Phase_Instabilities_during_Injection_Mixing_and_Combustion_under_High-Pressure_Conditions): **докторская диссертация** |![p-T-диаграмма струйного впрыска](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/p-T-jet.png)|
|[Динамика передачи и распространения детонации в искривленной камере: численный и экспериментальный анализ](https://doi.org/10.1016/j.combustflame.2020.09.032): **Статья** |![Эксперимент против расчета](https://ars.els-cdn.com/content/image/1-s2.0-S0010218020304168-gr2.jpg)|
|[Моделирование сверхзвуковых и дозвуковых потоков с использованием гибридного решателя на основе давления в Openfoam](https://doi.org/10.11159/ffhmt21.107): **Статья** |![Схема горелки Bluff body](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Bluff-body-sketch.png)|

## <p align="center"> >>>>> 2020 <<<<< </p>
| Название | Описание |
|------|-------------|
|[Численное моделирование экспериментов по охлаждению при транспирации в сверхзвуковом потоке с использованием OpenFOAM](https://link.springer.com/article/10.1007/s12567-019-00292-6) : **Статья** |![Схематическое изображение модели приложенного пористого интерфейса](https://media.springernature.com/full/springer-static/image/art%3A10.1007%2Fs12567-019-00292-6/MediaObjects/12567_2019_292_Fig1_HTML.png?as=webp)|
|[Об оценке неопределенности с помощью ансамбля независимых численных решений](https://doi.org/10.1016/j.jocs.2020.101114): **Статья** |![Схема потока](https://ars.els-cdn.com/content/image/1-s2.0-S1877750319310695-gr1.jpg)|
|[Влияние водородного эквивалента на сверхзвуковое горение по данным моделирования больших вихрей](https://doi.org/10.1016/j.ijhydene.2020.02.054): **Статья** |![Модель прямоточного воздушно-реактивного двигателя](https://ars.els-cdn.com/content/image/1-s2.0-S036031992030584X-gr1.jpg)|
|[Квазипрямое численное моделирование сжимаемых реагирующих потоков](https://doi.org/10.1016/j.compfluid.2020.104718): **Статья**|![Обмен данными между OpenFOAM и Cantera](https://ars.els-cdn.com/content/image/1-s2.0-S0045793020302887-gr1.jpg)|
|[О ПОСТРОЕНИИ ОБОБЩЕННОГО ВЫЧИСЛИТЕЛЬНОГО ЭКСПЕРИМЕНТА В ЗАДАЧАХ ВЕРИФИКАЦИИ](https://lppm3.ru/files/journal/XLVIII/MathMontXLVIII-Alekseev.pdf): **Статья** |![#D поток вокруг конуса](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Cone-exact-3D.png)|
|[Усовершенствованный алгоритм на основе давления для объединения с алгоритмом «давление – скорость – энтальпия» для потоков с любым числом Маха](https://link.springer.com/article/10.1007/s42405-020-00337-9): **Статья** |![Исходный алгоритм p–h-coupling на основе давления](https://media.springernature.com/lw685/springer-static/image/art%3A10.1007%2Fs42405-020-00337-9/MediaObjects/42405_2020_337_Fig1_HTML.png?as=webp)|
|[Система решений на основе давления для неидеальных течений при любых числах Маха](https://link.springer.com/chapter/10.1007/978-3-030-49626-5_4): **Статья** |![Полностью сформировавшаяся структура струи н-гексана, впрыскиваемой в неподвижную атмосферу азота](https://media.springernature.com/lw785/springer-static/image/chp%3A10.1007%2F978-3-030-49626-5_4/MediaObjects/492738_1_En_4_Fig3_HTML.png)|
|[Разработка имитационной модели коммутационных дуг в разрядниках](https://publikationsserver.tu-braunschweig.de/servlets/MCRFileNodeServlet/dbbs_derivate_00047899/Diss_Sander_Christian.pdf): **докторская диссертация** |![Spark](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/spark-tubraunscheig.png)|
|[Метод решения на основе давления для дозвуковых и сверхзвуковых потоков с учетом эффектов реального газа и фазового разделения в условиях, характерных для двигателей](https://doi.org/10.1016/j.compfluid.2020.104452): **Статья**|![Визуализация струи](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0045793020300281-gr15.jpg)|
|[Смешивание и самовоспламенение недорасширенных струй метана в условиях высокого давления](https://www.research-collection.ethz.ch/handle/20.500.11850/474652): **докторская диссертация**, результаты расчетов с использованием гибридного подхода были использованы в качестве эталона для STAR-CCM|![Подходы CFD и CMC](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/CFD-and-CMC.png)|

## <p align="center"> >>>>> 2019 <<<<< </p>

| Название | Описание |
|------|-------------|
|[Численный метод моделирования детонационного горения водородно-воздушной смеси в защитной оболочке](https://doi.org/10.1080/19942060.2019.1660219): **Статья** | ![Containement](https://www.tandfonline.com/na101/home/literatum/publisher/tandf/journals/content/tcfm20/2019/tcfm20.v013.i01/19942060.2019.1660219/20191106/images/medium/tcfm_a_1660219_f0018_oc.jpg)|
|[Численное исследование самовоспламенения при кратковременном впрыске водорода в сверхзвуковой поток](https://doi.org/10.1016/j.ijhydene.2019.07.215): **Статья** |![Теневая фотография струи](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/1-s2.0-S0360319919328514-gr4.jpg)|
|[Проверка на ансамбле независимых численных решений](https://link.springer.com/chapter/10.1007/978-3-030-22750-0_25): **Статья** |![Сравнение решателей](https://media.springernature.com/lw785/springer-static/image/chp%3A10.1007%2F978-3-030-22750-0_25/MediaObjects/485772_1_En_25_Fig2_HTML.png)|
|[Вычислительное исследование смешения реагентов во вращающейся детонационной камере сгорания с использованием сжимаемой модели турбулентности Рейнольдса](https://link.springer.com/article/10.1007/s10494-019-00097-x): **Статья** |![Детальная структура ударной волны в базовом сценарии течения в области инжекции, полученная на основе графика распределения числа Маха в продольной средней плоскости](https://media.springernature.com/lw685/springer-static/image/art%3A10.1007%2Fs10494-019-00097-x/MediaObjects/10494_2019_97_Fig5_HTML.png?as=webp)|
|[Численное исследование характеристик течения недорасширенных струй метана](https://doi.org/10.1063/1.5092776): **Статья**|![Визуализация течения струй метана](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/methane-jet.jpeg)|
|[Численное моделирование сжимаемой струи при малых числах Рейнольдса с использованием OpenFOAM](https://doi.org/10.1051/e3sconf/201912810008): **Статья**|![Q-критерий для струи Re3600 Ma0.9](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Re3600M0.9-jet.jpeg)|
|[Моделирование ракетного шлейфа URANS с использованием OpenFOAM](https://doi.org/10.1016/j.rineng.2019.100056): **Статья**|![Теневой график ракетного шлейфа](https://ars.els-cdn.com/content/image/1-s2.0-S2590123019300568-gr3.jpg)|
|[Numerische Modellierung und Untersuchung der Hochdruckeindüsung nicht-idealer Fluide bei überkritischen Druckverhältnissen (на немецком языке)](https://athene-forschung.unibw.de/130039): **Докторская диссертация**|![Струи с разделением](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/overexpjets-sep.png)|

## <p align="center"> >>>>> 2018 <<<<< </p>
| Название | Описание |
|------|-------------|
|[Эффекты реального газа и фазовое разделение в недорасширенных струях при условиях, характерных для двигателей](https://doi.org/10.2514/6.2018-1815): **Статья** |![История развития реактивного двигателя](https://www.researchgate.net/profile/Christoph-Traxinger/publication/322309300/figure/fig5/AS:622107033612289@1525333283581/figure-fig5_W640.jpg)|
|[Анализ точности решателей OpenFOAM для задачи о сверхзвуковом обтекании конуса](https://link.springer.com/chapter/10.1007/978-3-319-93713-7_18): **Статья** |![Эскиз конуса](https://media.springernature.com/lw785/springer-static/image/chp%3A10.1007%2F978-3-319-93713-7_18/MediaObjects/469704_1_En_18_Fig1_HTML.gif)|
|[Разработка нового решателя OpenFOAM с использованием регуляризованных уравнений газовой динамики](https://doi.org/10.1016/j.compfluid.2018.02.010): **Статья** |![Струя Ладенбурга](https://ars.els-cdn.com/content/image/1-s2.0-S0045793018300641-gr14.jpg)|
|[Гибридный метод решения для неидеальных однофазных потоков жидкости на всех скоростях](https://doi.org/10.1002/fld.4512): **Статья** |![Эксперимент и расчет](https://onlinelibrary.wiley.com/cms/asset/16108f70-6fec-4197-9aab-f84cbc5c2a1d/fld4512-fig-0005-m.jpg)|
|[Сравнение производительности пакетов CFD с открытым исходным кодом и коммерческих пакетов для моделирования сверхзвуковых сжимаемых струйных течений](https://doi.org/10.1109/IVMEM.2018.00019): **Статья** |![Эскиз вычислительной области подушки безопасности](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Airbag.png)|
|[Численное моделирование двумерных течений идеального газа методом RKDG на неструктурированных сетках](https://doi.org/10.1063/1.5065323): **Статья** |![RKDG (а) в сравнении с rhoPimpleCentralFoam (б)](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/forwardStep-RKDG-vs-RPCF.png) |
|[Методологии вычислительной гидродинамики для сжимаемых распыляющих и кавитационных многофазных потоков](https://eprints.utas.edu.au/28677/): **докторская диссертация** |![Изолинии струйного течения](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Jet-Hongjiang.png)|

## <p align="center"> >>>>> 2017 <<<<< </p>
| Название | Описание |
|------|-------------|
|[Численное исследование массива резонаторов Гельмгольца для снижения микронапорных волн в современных и будущих высокоскоростных железнодорожных тоннельных системах](https://doi.org/10.1016/j.jsv.2017.04.022): **Статья** | ![Сетка массива резонаторов Гельмгольца](https://ars.els-cdn.com/content/image/1-s2.0-S0022460X17303280-gr10.jpg)|
|[Сравнительное исследование точности решателей OpenFOAM](https://doi.org/10.1109/ISPRAS.2017.00028): **Статья** |![Обтекание конуса](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/index.jpeg)|
|[ Анализ дискретизации излучения при моделировании искрового промежутка для импульсных токов ](https://doi.org/10.14311/ppt.2017.1.56): **Статья** |![Схема искрового промежутка](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Plasma-spark-sketch.png)|
|[Численный анализ кавитации вокруг гребных винтов морских судов с использованием сжимаемого многофазного метода дробных шагов VOF](https://www.researchgate.net/publication/319306852_Numerical_analysis_of_cavitation_about_marine_propellers_using_a_compressible_multiphase_VOF_fractional_step_method): **Статья** |![Сравнение морфологии кавитации в cFSMVOF и других коммерческих программах и программах с открытым исходным кодом](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/cFSMVOF.png)|
|[Вычислительный анализ и снижение воздействия микронапорных волн в тоннелях высокоскоростных поездов](https://doi.org/10.25560/72653): **докторская диссертация**|![Типичный высокоскоростной поезд](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Train-1.png)|
|[Численное исследование характерных режимов и частот потока в высокоскоростных компрессорах (на английском языке)](https://doi.org/10.15514/ISPRAS-2017-29(1)-2): **Статья** |![Схема насоса ERCOFTAC](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/ERCOFTAC-pump.png)|
|[Реализация решателя для совместного моделирования теплопередачи в газах и твердых телах — 12-й семинар OpenFOAM](https://www.researchgate.net/publication/320924871_Implementation_of_the_solver_for_coupled_simulation_for_heat_transfer_in_gas_and_solid_-_12th_OpenFOAM_Workshop): **Презентация** |![Постановка задачи](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/cht-supersonic-cone.png)|
 |[Численное моделирование сжимаемых потоков с гибридным приближением конвективных потоков (на русском языке)](https://keldysh.ru/council/3/D00202403/kraposhin_diss.pdf): **кандидатская диссертация**|![Сравнение подходов, основанных на плотности и давлении](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Dens-rho-vs-pres.png)|
|[Применение программного обеспечения с открытым исходным кодом для решения промышленных задач газовой динамики при отрыве транспортного средства от поверхности (на русском языке)](https://journals.ssau.ru/vestnik/article/view/5610): **Статья**|![Сравнение эксперимента Эггерса с расчётами](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Eggers-exp-vs-calc.png)|
|[Оптимизация внутренних турбулентных сжимаемых потоков с использованием сопряженных задач](https://www.researchgate.net/publication/318144074_Optimization_for_Internal_Turbulent_Compressible_Flows_Using_Adjoints): **Статья** |![Поля пористости и скорости](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/OptIntCompFlows_AIAA.png)|

## <p align="center"> >>>>> 2016 <<<<< </p>

| Название | Описание |
|------|-------------|
|[О стабильности сверхзвуковых пограничных слоев с инжекцией](https://thesis.library.caltech.edu/9755/): **докторская диссертация** | ![Схема взаимодействия пограничного слоя с реактивной струей](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/boundary-layer.png)|
|[Исследование возможностей гибридной схемы для аппроксимации членов адвекции в математических моделях сжимаемых течений (на русском языке)](https://ispranproceedings.elpub.ru/jour/article/view/121): **Статья** |![Жидкостно-кольцевой вакуумный насос](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/LRVP.png)|
|[Методы дискретизации LES для неструктурированных сеток на основе метода конечных объемов](https://doi.org/10.21656/1000-0887.370228): **Статья** |![Завихренность: обтекание цилиндра](https://github.com/unicfdlab/hybridCentralSolvers/blob/master/Figs/Vorticity-vs-scheme.png)|

# Для цитирования
[К содержанию](#Contents)

 При использовании этих решателей, пожалуйста, ссылайтесь на следующие работы:
 * [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.3878441.svg)](https://doi.org/10.5281/zenodo.3878441)
 * Что касается однофазного моделирования и произвольных уравнений состояния, [Крапошин М. В., Банхольцер М., Пфицнер М., Марчевский И. К. Гибридный решатель на основе давления для неидеальных однофазных потоков жидкости на всех скоростях. Int J Numer Meth Fluids. 2018;88:79–99](https://www.researchgate.net/publication/325025590_A_hybrid_pressure-based_solver_for_non-ideal_single-phase_fluid_flows_at_all_speeds_Non-ideal_single-phase_fluid_flow_solver). https://doi.org/10.1002/fld.4512
 * Что касается основных принципов гибридных центральных решателей, [Крапошин М. В., Стрижак С. В., Бовтрикова А. Адаптация численной схемы Курганова-Тадмора для применения в сочетании с методом PISO при численном моделировании течений в широком диапазоне чисел Маха. Procedia Computer Science. 2015;66:43-52](https://www.researchgate.net/publication/284913682_Adaptation_of_Kurganov-Tadmor_Numerical_Scheme_for_Applying_in_Combination_with_the_PISO_Method_in_Numerical_Simulation_of_Flows_in_a_Wide_Range_of_Mach_Numbers). https://doi.org/10.1016/j.procs.2015.11.007
 * Для современных гибридных центральных решателей VoF для сжимаемых двухфазных задач [Крапошин М., Кухарский А., Виктория и Шевелев А. (2022). Расширение концепции решения на основе давления для численного моделирования двухфазных потоков с границей раздела. Промышленные процессы и технологии, 2(3(5), 6–27. ](https://www.researchgate.net/publication/365897832_An_extension_of_the_all-Mach_number_pressure-based_solution_framework_for_numerical_modelling_of_two-phase_flows_with_interface) https://doi.org/10.37816/2713-0789-2022-2-3(5)-6-27
 * Для многокомпонентного моделирования и/или моделирования в сочетании с движением частиц [Мельникова В., Епихин А. и Крапошин М. Эйлеровско-лагранжианский подход к численному исследованию акустического поля, создаваемого высокоскоростным газокапельным потоком. Fluids 2021, 6(8), 274; https://doi.org/10.3390/fluids6080274](https://www.mdpi.com/2311-5521/6/8/274)
 * Для случаев с двухфазными (или n-фазными) однородными потоками [Сюй Л., Ли Ю., Ма С., Го Х., Шуай С., Шевелев А. и Крапошин М. Гибридная модель на основе давления для дозвуковых и сверхзвуковых сжимаемых двухфазных потоков с неравновесным фазовым переходом. Computers & Fluids, том 305, 2026](https://www.sciencedirect.com/science/article/abs/pii/S004579302500372X)
