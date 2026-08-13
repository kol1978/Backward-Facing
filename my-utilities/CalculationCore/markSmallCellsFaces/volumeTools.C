/*
 volumeTools.C (Никаких main, только реализация функции)
 **/
#include "volumeTools.H"
#include "volFields.H"
#include "IOstream.H"

// Лучше не делать using namespace в .C, чтобы не путать области видимости
// namespace Foam { ... } можно не писать, если используем префикс Foam::

Foam::scalarField Foam::calculateCellVolumes(const Foam::fvMesh& mesh)
{
    const Foam::scalarField& Vref = mesh.V();
    Foam::scalarField V = Vref;

    const scalar smallVol = 1e-12;
    forAll(V, i)
    {
        if (V[i] < smallVol)
        {
            V[i] = 0.0;
        }
    }

    return V;
}

