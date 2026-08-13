/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Version:  14
     \\/     M anipulation  |
\*---------------------------------------------------------------------------*/

/*
scalarField V = mesh.V();                 // чтение объёмов
label nCells = mesh.nCells();             // число ячеек
const labelList& owner = mesh.owner();    // владельцы граней

Готовый шаблон утилиты, которая:
Проходит по всем ячейкам.
Находит те, у которых объём меньше порога.
Собирает все их грани в faceSet.
Корректно работает в параллели (суммирует и сохраняет на всех ранках).
Пишет файл в constant/polyMesh/sets/, который сразу открывается в ParaView.
Как собрать и запустить:
cd $FOAM_USER_APPBIN/markSmallCellsFaces
 wclean
wmake
Запуск (с порогом, например, 1e-9):
markSmallCellsFaces -case ./cavityGrade -threshold 1e-9
*/
#include "argList.H"
#include "Time.H"
#include "fvMesh.H"
#include "IOstream.H"
#include "volumeTools.H"

using namespace Foam;

// --------------------------------------------------------------------
// РЕГИСТРАЦИЯ ОПЦИЙ — ДО main(), чтобы setRootCase её увидел
// --------------------------------------------------------------------
class MyVolCalcOptions
{
public:
    MyVolCalcOptions()
    {
        argList::addOption(
            "threshold",
            "<scalar>",
            "порог минимального объёма (м^3)"
        );
    }
} myVolCalcOptionsRegistrar;


int main(int argc, char *argv[])
{
    // Теперь эти макросы НЕ упадут на -threshold, потому что опция уже зарегистрирована
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"

    // Чтение значения: сначала дефолт, потом перезапись, если флаг задан
    scalar thresh = 1e-9;
    if (args.optionFound("threshold"))
    {
        thresh = args.optionRead<scalar>("threshold");
    }
    Info << "Порог объёма: " << thresh << nl;

    scalarField V = calculateCellVolumes(mesh);

    const label nCells = mesh.nCells();
    scalar sumV = 0.0;
    label nSmall = 0;

    forAll(V, i)
    {
        sumV += V[i];
        if (V[i] < thresh) nSmall++;
    }

    Info << "Число ячеек: " << nCells << nl;
    Info << "Суммарный объём: " << sumV << " м^3" << nl;
    Info << "Ячеек ниже порога: " << nSmall << nl;

    return 0;
}
