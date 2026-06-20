# Backward-Facing
------------------RUN
Сздать сетку:
blockMesh

Проверить сетку:
 checkMesh
 или
 checkMesh -allTopology

Выполнить декомпозицию:
decomposePar -force

Запусть расчет на десяти ядрах (в файле decomposeParDict - numberOfSubdomains 10; указать нужное число ядер и запустить decomposePar -force):
mpirun -np 10 foamRun -parallel                //Здесь -np 10 задаёт 10 процессора; замените foamRun/icoFoam на нужный солвер.

Собрать папки временных шагов из "параллельных"(... processor9) папок:
reconstructPar

Просмотр результатов:
paraFoam -builtin

Создать файл визуализаци:
paraFoam -touch

Удалить файлы временных шагов расчета:
foamListTimes -rm

Постпроцессинг:
postProcess -list
postProcess -func "components(U)"

Замечание:
simpleFoam -postProcess
icoFoam не нужен -postProcess
------------------------------------
