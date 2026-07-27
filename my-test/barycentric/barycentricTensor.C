/*Application
    Test-barycentric

Description
Этот код — учебный пример работы с барицентрическими координатами и тетраэдрами в OpenFOAM. Он демонстрирует, как:

задать тетраэдр через 4 точки;
построить по нему барицентрический тензор;
переводить точки между декартовыми и барицентрическими координатами;
использовать тензор для преобразования координат.
опыт с CFMesh, сеткой и проверкой её качества, этот пример особенно полезен: барицентрические координаты активно используются в методах интерполяции, построении сеток и определении положения точки внутри ячейки (в том числе тетраэдрической).

CFMesh и локализация точек. Когда у тебя есть индексы 4 вершин ячейки и массив pointField с координатами всех точек, ты делаешь:
cpp
const tetrahedron<point, const point&> t(pts[v0], pts[v1], pts[v2], pts[v3]);
vector v = t.barycentricToPoint(b);
Это быстро и надёжно.
Экспорт в HDF5. Если ты сохраняешь барицентрические координаты точек внутри ячеек, этот код даёт эталонный способ их вычисления и проверки обратимости (точка → барицентр → точка).
Проверка качества ячеек. Можно добавить проверку объёма тетраэдра (tetA.mag()): если он близок к нулю — ячейка вырождена, и барицентрические преобразования будут численно неустойчивыми.
\*---------------------------------------------------------------------------*/
#include "barycentricTensor.H"
#include "tetrahedron.H"
#include "IOstreams.H"

using namespace Foam;

int main(int argc, char *argv[])
{
    // Именованные точки — теперь нет «unnamed temporaries»
    const point pA(0, 0, 0);
    const point pB(1, 0, 0);
    const point pC(1, 1, 0);
    const point pD(1, 1, 1);

    // Тетраэдр по именованным точкам
    const tetrahedron<point, const point&> tetA(pA, pB, pC, pD);

    // Барицентрический тензор: теперь ссылки на pA..pD — всё легально
    const barycentricTensor baryT(pA, pB, pC, pD);

    Info<< nl << "Tet: " << tetA << nl;
    Info<< "tens:" << baryT << nl;

    List<barycentric> baryList
    ({
        {0.25, 0.25, 0.25, 0.25},
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    });

    for (const barycentric& bary : baryList)
    {
        const vector v = tetA.barycentricToPoint(bary);
        const barycentric b = tetA.pointToBarycentric(v);

        Info<< nl
            << "bary: " << bary << nl
            << "vec:  " << v << nl
            << "Vec (via tensor): " << (baryT & bary) << nl
            << "bary (recovered): " << b << nl;
    }

    Info<< "\nEnd\n" << nl;

    return 0;
}
