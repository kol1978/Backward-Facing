/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Version:  14
     \\/     M anipulation  |
\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\
Иерархия и суть:
primitiveMesh — базовый класс с топологией (кто с кем соединён: точки, грани, ячейки).
polyMesh — расширяет primitiveMesh, добавляет геометрию полигональной сетки (центры граней/ячеек, площади, нормали и т. д.) и служит «чистым» представлением сетки без привязки к методу решения.
fvMesh — наследуется от polyMesh и добавляет всё, что нужно именно для метода конечных объёмов (FVM): объёмы ячеек, дополнительные вспомогательные поля, логику для численных схем и т. п.
То есть fvMesh = polyMesh + данные, специфичные для FV‑расчётов.

#include "createSpecifiedPolyMesh.H"   // создаёт объект mesh типа polyMesh
fvMesh fvMeshObj(mesh);               // конструирует fvMesh из polyMesh
const scalarField& V = fvMeshObj.V(); // получает объёмы ячеек

fvMesh — это объект в оперативной памяти, который строится при запуске OpenFOAM‑утилиты или решателя на основе файлов из polyMesh.

Процесс такой:
При старте утилиты выполняется макрос #include "setRootCase.H" — он находит кейс, читает аргументы командной строки.
Затем #include "createTime.H" — инициализирует время и, среди прочего, запускает построение fvMesh.
Внутри конструктора fvMesh(const polyMesh&):
берутся данные из polyMesh (точки, грани, owner/neighbour);
вычисляются производные геометрические величины: центры ячеек, центры граней, площади, нормали;
вычисляются объёмы ячеек (это самое важное для FVM);
строятся вспомогательные структуры для интерполяции, вычисления градиентов и потоков.

Почему это важно:
У polyMesh нет метода .V().
У fvMesh метод .V() есть и возвращает const scalarField& — именно это нужно для анализа минимальных/отрицательных объёмов, которые приводят к NaN и расходимости (актуально для DNS, boxTurb16 и т. д.).

В объекте fvMesh хранятся, например:

cellCentres() — центры ячеек.
faceCentres() — центры граней.
faceAreas() — площади граней.
faceNormals() — нормали к граням.
.V() — объёмы ячеек (как scalarField).
Структуры для вычисления потоков, градиентов, дивергенций (интерполяции между ячейками и гранями и т. п.).
Эти величины не пишутся на диск как отдельные файлы. Они пересчитываются при каждом запуске.

mesh — это объект класса polyMesh, который был создан макросом #include "createSpecifiedPolyMesh.H". Он «знает», где на диске лежит папка constant/<region>/polyMesh и файлы (points, faces, owner, neighbour и т. д.).
На диске есть файлы сетки в constant/<region>/polyMesh:
points — координаты всех вершин.
faces — списки вершин для каждой грани.
owner / neighbour — кто владелец грани и кто сосед (топология внутренних граней).
boundary — описание патчей (границ).
Макрос #include "createSpecifiedPolyMesh.H" (или "createMesh.H") вызывает конструктор polyMesh, который:
читает эти файлы;
инициализирует базовый объект primitiveMesh, передавая туда топологию и геометрию.
Внутри конструктора polyMesh происходит вызов конструктора базового класса primitiveMesh. Именно на этом этапе формируется структура primitiveMesh: заполняются внутренние списки (точки, грани, owner/neighbour и т. д.).
Затем polyMesh достраивает центры ячеек/граней, площади, нормали и т. п.
Потом fvMesh берёт готовый polyMesh (а значит, и его primitiveMesh) и добавляет FV‑специфичные поля (объёмы, LDU‑адресацию и т. д.).

В OpenFOAM класс primitiveMesh лежит в:
заголовок: src/OpenFOAM/meshes/primitiveMesh/primitiveMesh.H
реализация: src/OpenFOAM/meshes/primitiveMesh/primitiveMesh.C

\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
Вызов write() — без аргументов: cellVolumes.write(). Это штатный метод regIOobject, который берёт путь из IOobject.name_.

  Application
MinVol

  Description

which MinVol
ls -l $(which MinVol)
date -r $(which MinVol)
\*---------------------------------------------------------------------------*/
#include "argList.H"
#include "Time.H"
#include "fvMesh.H"
#include "fileName.H"
#include "OFstream.H"
#include "Pstream.H"
#include "volFields.H"
#include "dimensionedScalar.H"
#include "meshCheck.H"
#include "timeSelector.H"

using namespace Foam;

int main(int argc, char *argv[])
{
    // Регистрация опции для справки
    argList::addOption(
        "minV",
        "<scalar>",
        "порог минимального объёма ячейки (м^3)"
    );

    #include "addMeshOption.H"
    #include "addRegionOption.H"
    #include "setRootCase.H"      // создаёт args
    #include "createTime.H"       // создаёт runTime

    const instantList timeDirs = timeSelector::select0(runTime, args);

    // ✅ Оставляем createSpecifiedPolyMesh.H — он есть в OF-14 и создаёт polyMesh mesh
    #include "createSpecifiedPolyMesh.H"   // создаёт polyMesh& mesh

    scalar minVolThreshold = 1e-9;
    args.optionReadIfPresent("minV", minVolThreshold);

    if (args.optionFound("minV"))
    {
        Info << "Используется пользовательский порог минимального объёма: "
             << minVolThreshold << nl;
    }
    else
    {
        Info << "Используется порог по умолчанию: " << minVolThreshold << nl;
    }

    Info << "Порог минимального объёма = " << minVolThreshold << " м^3" << nl;

    // --------------------------------------------------------------------
    // Превращаем polyMesh в fvMesh, чтобы получить .V()
    // --------------------------------------------------------------------
    fvMesh fvMeshObj(mesh);              // конструктор fvMesh(const polyMesh&)
    const scalarField& V = fvMeshObj.V();

    Info << "Регион: " << fvMeshObj.name() << nl;
    Info << "Количество ячеек: " << fvMeshObj.nCells() << nl;

    scalar minVol = GREAT;
    scalar maxVol = -GREAT;
    label nNeg = 0;
    label nSmall = 0;

    forAll(V, i)
    {
        const scalar v = V[i];
        if (v < 0) nNeg++;
        if (v < minVol) minVol = v;
        if (v > maxVol) maxVol = v;
        if (v < minVolThreshold) nSmall++;
    }

    Info << "Минимальный объём ячейки: " << minVol << " м^3" << nl;
    Info << "Максимальный объём ячейки: " << maxVol << " м^3" << nl;
    Info << "Отрицательных объёмов: " << nNeg << nl;
    Info << "Ячеек с объёмом < " << minVolThreshold
         << " м^3: " << nSmall << nl;

    if (nNeg > 0)
    {
        Info << "ОШИБКА: обнаружены ячейки с отрицательным объёмом!" << nl;
        return 1;
    }

    // Создание папки mesh
    const fileName caseRoot = runTime.path();
    const fileName meshDir = caseRoot / "mesh";

    bool created = false;
    if (Pstream::master())
    {
        if (!isDir(meshDir))
        {
            created = mkDir(meshDir);
            if (created)
            {
                Info << "Папка создана (master): " << meshDir << nl;
            }
            else
            {
                FatalErrorInFunction
                    << "Не удалось создать папку '" << meshDir
                    << "'. Проверьте права доступа." << nl
                    << exit(FatalError);
            }
        }
        else
        {
            Info << "Папка уже существует (master): " << meshDir << nl;
        }
    }

    // Запись объёмов в case/mesh/meshV
    const fileName cellVolumesPath = meshDir / "meshV";

    OFstream volFile(cellVolumesPath);
    if (!volFile)
    {
        FatalErrorInFunction
            << "Не удалось открыть файл для записи: " << cellVolumesPath << nl
            << exit(FatalError);
    }

    volFile << "FoamFile\n{\n    version 2.0;\n    format ascii;\n    class volScalarField;\n    location \""
            << fvMeshObj.name() << "\";\n    object meshV;\n}\n"
            << dimVolume << nl
            << fvMeshObj.nCells() << "(" << nl;

    forAll(V, i)
    {
        volFile << V[i] << nl;
    }
    volFile << ")\n";

    Info << "Поле meshV сохранено в: " << cellVolumesPath << nl;

    return 0;
}


