##### Создание справки Doxygen для OpenFOAM
#### Оглавление
1. Doxygen<br>
2. Настройка<br>
3. Запуск Doxygen<br>
#### 1 Doxygen
HTML-документацию OpenFOAM можно создать с помощью программы Doxygen. Изображения в документации создаются с помощью dot — программы из пакета graphviz. Например, чтобы собрать документацию в системе Ubuntu GNU/Linux,<br> пользователь должен установить пакеты doxygen и graphviz, например, введя в окне терминала:<br>
`````bash
sudo apt-get install doxygen graphviz
`````
Документация Doxygen будет собрана автоматически для пользователя, настроенного на работу с OpenFOAM, то есть с заданными переменными среды, такими как $WM_PROJECT_DIR set. Пользователь также должен убедиться, что у него есть права на запись в каталог, в который Doxygen записывает файлы.

#### 2 Конфигурация
Файл конфигурации Doygen, Doxyfile, в каталоге $WM_PROJECT_DIR/doc/Doxygen настроен для работы с версиями Doxygen 1.6.3–1.8.5.

Верхний и нижний колонтитулы, а также таблица стилей генерируются автоматически:
`````bash
doxygen -w html header.html footer.html customdoxygen.css
`````
См.: https://www.stack.nl/~dimitri/doxygen/manual/customize.html

#### 3 Запуск Doxygen
В каталоге $WM_PROJECT_DIR/doc/Doxygen введите<br>
`````bash
./Allwmake
`````
Это создаст каталог $WM_PROJECT_DIR/doc/Doxygen/html, содержащий документацию по исходному коду OpenFOAM. Загрузите index.html файл в браузер, например:<br>
`````bash
firefox $WM_PROJECT_DIR/doc/Doxygen/html/index.html
`````
Дата: 18 июня 2016 года<br>

Создано: 2016-06-18, сб, 10:13<br>

Emacs 24.5 (Org mode 8.2.10)<br>
