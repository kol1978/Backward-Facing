
The description of the data files are given in the following.

I.  Measurement-domain description
    : the upper half of the duct was divided into five domains
      since three different boundaries were to be measured.

--  measurement domain : up1, in1, ou1, in2, ou2
     
                top(flat) wall
              ------------------
              |  |          |  |
              |  |          |  |
              |  |          |  |
          in1--> |   up1    | <-- ou1
              |  |          |  |
              |  |          |  |
              ------------------
              |       |        | 
  inside wall |       |        |  outside wall
   (convex)   |       |        |   (concave)
              |       |        |
              |  in2  |  ou2   |
              |       |        |
              |       |        |
              |       |        |
              |       |        |
              |       |        |
              |       |        |
              ------------------ Symmetry plane of the duct
              |                |
              |                |



II. File description              

A. pressure.dat
   : pressure coeff. along the symmetry plane of the duct
   pressure-tab.dat
   : tabulated pressure coeff.

B. m$$@@@.dat  ($$:station, @@@:measurement domain)
   : mean flow (U,V,W) data of @@@ domain at $$ station

C. t$$@@@.dat  ($$:station, @@@:measurement domain)
   : Reynolds-stress (uu,vv,ww,uv,uw) data of @@@ domain at $$ station

D. s$$%%%.dat ($$:station, %%%:wall)
   : wall shear stress at top(up), inside(in), outside(out) walls

E. m$$sel.dat ($$:station)
   : selective mean flow (U,V,W) data along Z=constant line

F. t$$sel.dat ($$:station)
   : selective Reynolds-stress data along Z=constant line

(note) all data are non-dimensionalized by freestream velocity (16 m/sec)
       at station U1 and the duct width ( 8 inch = 20.3 cm).


