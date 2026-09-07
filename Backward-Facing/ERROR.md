    Decomposing surfaceScalarFields

        phi
        phi_0
Физический смысл: phi — это массовый или объёмный поток через грань (в зависимости от размерности и контекста). В несжимаемых задачах это объёмный поток [м3/с], в сжимаемых — часто массовый [кг/с][кгс]
===================
--> FOAM FATAL ERROR:
Symmetry plane 'frontAndBack' is not planar
At patch face #233824 with centre (0.09987738171471169 0.09191259977283596 0.0762) the normal (0 0 1) differs from the average normal (-1.80022399478053e-18 2.731993109079805e-18 -1.588475600817372e-20) by 1
Either split the patch into planar parts or use the symmetry patch type

    From function virtual void Foam::symmetryPlanePolyPatch::calcGeometry(Foam::PstreamBuffers&)
    in file meshes/polyMesh/polyPatches/constraint/symmetryPlane/symmetryPlanePolyPatch.C at line 85.

FOAM exiting


=====================
В 2D‑постановке нет полноценного каскада энергии по масштабам: в двумерной турбулентности работает обратный каскад (энергия уходит в большие масштабы), а не прямой (как в 3D). Это фундаментально меняет физику.
Подавляются важные типы вихревых взаимодействий: многие механизмы генерации и разрушения вихрей требуют трёх измерений.

----------------------------
Выбор типа модели переноса импульса LES
Выбор модели турбулентности LES kEqn
Выбор типа дельты LES cubeRootVol
--> Предупреждение FOAM:
Из функции void Foam::LESModels::cubeRootVolDelta::calcDelta()
в файле LES/LESdeltas/cubeRootVolDelta/cubeRootVolDelta.C в строке 55
Случай 2D, LES не совсем применим
