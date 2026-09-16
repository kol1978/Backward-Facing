#!/bin/bash

foamCleanTutorials
blockMesh
#checkMesh
rm -rf 0
cp -r 0_org/ 0
setFields

#sonicFoam | tee log.solver
rhoPimpleFoam | tee log.solver

postProcess -func 'mag(U)' 
postProcess -func 'components(U)' 
postProcess -func sampleDict

python python/sodshocktube.py
#python3 python/sodshocktube.py

