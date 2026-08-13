#ifndef volumeTools_H
#define volumeTools_H

#include "fvMesh.H"
#include "scalarField.H"

namespace Foam
{
    // Объявление функции: она возвращает scalarField и принимает const ссылку на fvMesh
    scalarField calculateCellVolumes(const fvMesh& mesh);
}

#endif // volumeTools_H
