EXPERIMENTAL DATA ON FAR-WAKE 

Data are contained in two files:

Dynamic-field-data.dat contains profiles of mean velocity, Reynolds stresses, k, epsilon and k turbulent diffusion.

Thermal-field-data.dat contains profiles of mean temperature, turbulent heat flux, temperature variance, epsilon_theta and temperature variance diffusion.


REFERENCES :

1. Browne, L.W.B., Antonia, R.A. and Shah, D.A. (1987)
   Turbulent energy dissipation in a wake. 
   J. of Fluid Mech., vol. 179, pp. 397-326.
2. Antonia, R.A. and Browne, L.W.B. (1986)
   Anisotropy of temperature dissipation in a turbulent wake.
   J. of Fluid Mech., vol. 163, pp. 393-403.

NOMENCLATURE :

d = diameter of circular cylinder
f = (Ue - U)/Uo
Ue = free stream velocity
U  = local velocity
Uo = max. velocity defect
L  = half-width (based on velocity defect)
Te = free stream temperature
T  = local temperature
To = max. excess temperature,(Te-T) along centre line
Ts = (T - Te)/To
k  = turbulent kinetic energy
uu, vv, ww = normal Reynolds stress components
Eps = rate of dissipation of k
Eps_h = rate of dissipation of k in homogeneous flow


tt = variance of temperature fluctuation
vt = cross-stream turbulent heat flux
Eps_t = rate of dissipation of THETAs
NU = kinematic viscosity

CHARACTERISTICS OF WAKE :
(i) Wake producing body : circular cylinder of diameter 2.67 mm.
(ii) Measuring station : x/d = 420
     At x/d = 420, the values are
          L = 12.3 mm.
	  Ue = 6.7 m/s.
	  Ue*d/NU = 1190
	  To = 0.82 deg. Celsius

REMARKS :
(1) The values of Eps_t are obtained by balancing the equation 
for THETAs. These values are slightly different from the experimental 
values of EPSILONtheta. (See Ref.2 above).

(2) The final three values in Thermal-field-data.dat for vt and 
D(theta2v)/2D(eta) (all zero) should be disregarded, since velocities
were not measured at these locations.
