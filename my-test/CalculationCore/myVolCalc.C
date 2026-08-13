/*
myVolCalc.C (точка входа)

Запуск:

myVolCalc -case ./cavity -threshold 1e-9

*/
#include "argList.H"
#include "Time.H"
#include "fvMesh.H"
#include "IOstream.H"

#include "volumeTools.H"

using namespace Foam;


// Регистрируем опции ДО того, как setRootCase.H попытается их проверить
Foam::stringList argsOptions()
{
    Foam::stringList opts;
    opts.append("threshold <scalar> порог минимального объёма (м^3)");
    return opts;
}

int main(int argc, char *argv[])
{
        argList::addOption(
        "threshold",
        "<scalar>",
        "порог минимального объёма (м^3)"
    );

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"

    // ПРАВИЛЬНО: сначала проверка, потом шаблонный optionRead
    scalar thresh = 1e-9;  // значение по умолчанию
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
