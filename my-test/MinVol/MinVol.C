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

Почему это важно:
У polyMesh нет метода .V().
У fvMesh метод .V() есть и возвращает const scalarField& — именно это нужно для анализа минимальных/отрицательных объёмов, которые приводят к NaN и расходимости (актуально для DNS, boxTurb16 и т. д.).
\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "fvMesh.H"
#include "IOmanip.H"

using namespace Foam;

int main(int argc, char *argv[])
{
    // ВАЖНО: НЕ создавай argList вручную.
    // Макросы setRootCase.H и createTime.H сами создадут args.
    // Поэтому здесь вообще нет строки argList args(...).

    // Сначала добавляем опции региона и сетки.
    // В этом месте args ещё не создан, но addMeshOption/addRegionOption
    // только регистрируют ключи командной строки — это безопасно.
    #include "addMeshOption.H"
    #include "addRegionOption.H"

    // Теперь макросы инициализируют кейс, создают args и runTime.
    #include "setRootCase.H"
    #include "createTime.H"

    // Создаём polyMesh для выбранного региона (-region или region0 по умолчанию)
    #include "createSpecifiedPolyMesh.H"

    // Преобразуем в fvMesh, чтобы получить доступ к .V()
    fvMesh fvMeshObj(mesh);

    const scalar minVolThreshold = 1e-9;
    Info << "Порог минимального объёма = " << minVolThreshold << " м^3" << endl;
    Info << "Регион: " << fvMeshObj.name() << endl;
    Info << "Количество ячеек: " << fvMeshObj.nCells() << endl;

    const scalarField& V = fvMeshObj.V();

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

    Info << "Минимальный объём ячейки: " << minVol << " м^3" << endl;
    Info << "Максимальный объём ячейки: " << maxVol << " м^3" << endl;
    Info << "Отрицательных объёмов: " << nNeg << endl;
    Info << "Ячеек с объёмом < " << minVolThreshold
         << " м^3: " << nSmall << endl;

    if (nNeg > 0)
    {
        Info << "ОШИБКА: обнаружены ячейки с отрицательным объёмом!" << endl;
        return 1;
    }

    return 0;
}







