 
# Репозиторий верификации и валидации для QGD/гибридных решателей

Основная цель репозитория — собрать и хранить актуальные задачи верификации и валидации, демонстрирующие сильные и слабые стороны вышеупомянутых вычислительных комплексов.

## Доступные кейсы

Доступны следующие кейсы (загружены в репозиторий):

- [1D-кейс задачи Сода](https://github.com/mkraposhin/VnV/tree/main/SodProblem);
- [1D задача о двух разрежениях](https://github.com/mkraposhin/VnV/tree/main/TwoRarefactionWaves);
- [1D задача о скачке с высоким коэффициентом давления](https://github.com/mkraposhin/VnV/tree/main/HighPressureRatioShock);
- [задача о двух скачках](https://github.com/mkraposhin/VnV/tree/main/TwoShocks);
- [случай диффузии двух газов](https://github.com/mkraposhin/VnV/tree/main/GasesDiffusion);
- [прямой шаг](https://github.com/mkraposhin/VnV/tree/main/ForwardStep);
- [задача Сода для двух газов](https://github.com/mkraposhin/VnV/tree/main/TwoGasesSodProblem);
- [одномерная реактивная труба](https://github.com/mkraposhin/VnV/tree/main/Reactive1dTube);
- [шаг назад с Re=100](https://github.com/mkraposhin/VnV/tree/main/BackwardStepIco);
- [распространение плоской акустической волны](https://github.com/mkraposhin/VnV/tree/main/PlanarAcousticWave);
- [сходяще-расходящееся сопло](https://github.com/mkraposhin/VnV/tree/main/ConverginDivergingNozzle);
- [задача о стационарном контактном разрыве](https://github.com/mkraposhin/VnV/tree/main/StationaryContact);
- [проблема разрыва движущегося контакта] (https://github.com/mkraposhin/VnV/tree/main/MovingContact).

## Случаи с автоматизированной процедурой проверки

- [ударная трубка Sod] (https://github.com/mkraposhin/VnV/tree/main/SodProblem);
- [одномерная реактивная трубка] (https://github.com/mkraposhin/VnV/tree/main/Reactive1dTube);
- [проблема разрыва стационарного контакта] (https://github.com/mkraposhin/VnV/tree/main/StationaryContact);
- [проблема разрыва движущегося контакта] (https://github.com/mkraposhin/VnV/tree/main/MovingContact).

## Структура хранилища

Структура хранилища следующая.

1. На первом уровне идут папки с названиями задач.
2. На втором уровне внутри папки с проблемами мы храним папки:
 — для справочных данных (reference/);
 — для литературы (docs/);
 - для результатов сравнения (comparison/) с эталонными данными и между различными решателями;
 - для случаев, рассчитанных с помощью разных решателей (например, QGDFoam/, pimpleCentralFoam/, rhoCentralFoam/ и т. д.);
 - для общих данных, используемых разными решателями (common/).
3. На третьем уровне, внутри папок с решателями, мы храним вычислительные случаи в папках с названиями, указывающими на плотность сетки (например, 10CPL, 20CPL и т. д.).

Список желаемых случаев для проверки поддерживается с помощью раздела [проблемы] (https://github.com/mkraposhin/VnV/issues) репозитория со всеми необходимыми ссылками на справочные данные и описание проблем.

## Примеры структуры

 SodProblem/
 ссылка/
 Документы/
 сравнение/
 QGDFoam/
 100CPL/
 200CPL/
 400CPL/
 pimpleCentralFoam/
 100CPL/
 200CPL/
 400CPL/
