
TEST CASE  6:1 PROLATE SPHEROID
===========================================================================
===========================================================================

Data are available for three cases:

===========================================================================
***  CASE 1  ***
DLR LOW SPEED WIND TUNNEL GOETTINGEN (NWG)
ANGLE OF INCIDENCE: ALPHA= 10 DEG.
REYNOLDS NUMBER   : RE   = 7.7E06
FIXED TRANSITION AT X0/2A= 0.2

===========================================================================
***  CASE 2  ***
DLR LOW SPEED WIND TUNNEL GOETTINGEN (NWG)
ANGLE OF INCIDENCE: ALPHA= 30 DEG.
REYNOLDS NUMBER   : RE   = 6.5E06
FREE TRANSITION

===========================================================================
***  CASE 3  ***
ONERA LOW SPEED WIND TUNNEL LE FAUGA (F1)
ANGLE OF INCIDENCE: ALPHA= 30 DEG.
REYNOLDS NUMBER   : RE   = 43.0E06 / 40E06
FREE TRANSITION

===========================================================================
Data fro each case is contained within a separate directory

nwg10:  Case 1
nwg30:  Case 2
f1_30:  Case 3

Within each directory there is:

A file called xxxxx_cp.dat containing the pressure coefficient data

A subdirectory called xxxxx_cf containing skin friction corefficient data
(each file in the directory contains data around the spheroid, at a particular x location)

A subdiretcory called xxxxxuff or xxxxxubl contining mean velocity data
(each file in the directory contains a velocity profile, in a direction normal to the
spheroid surface, at a particular x and phi location).
