*
*				program post.f
*
*      Purpose: Perform post processing calculations on NPARC2D
* 		 solutions
*
*  11/26        Modified to  integrate b.layer parameters using 
*               ym = distance to the first maximum in y.
*
*  06/30/98     Modified equations to have consistent units.
*
* Variable     Metric(SI)           British(BG)             English(EE)
* --------   ---------------   ---------------------   ----------------------
*   x            m                  ft                        ft
*   F            N                  lbf                       lbf
*   P            N/m2               lbf/ft2                   lbf/ft2
*   T            K                  R                         R
*   rho          kg/m3              slug/ft3                  lbm/ft3
*   U            m/s                ft/s                      ft/s
*   Rgas     287 N*m/(kg*K)    1716 lbf*ft/(slug*R)    53.35  lbf*ft/(lbm*R)
*   gc         1 kg*m/(N*s2)      1 slug*ft/(lbf*s2)   32.174 lbm*ft/(lbf*s2)
*   mu           N*s/m2             lbf*s/ft2                 lbf*s/ft2 
*   nu           m2/s               ft2/s                     ft2/s
*
* Dimensional Equations which work with the above units
*   P    = rho*Rgas*T
*   a    = sqrt(gamma*Rgas*T*gc)
*   tau  = mu*dudy
*   Utau = sqrt(gc*tau/rho)
*   mdot = rho*U*A
*   nu   = gc*mu/rho
*   mu   = 1.450E-06*T**1.5/(T+110   K) N*s/m2
*        = 2.270E-08*T**1.5/(T+198.6 R) lbf*s/ft2
*   F    ~ mdot*U/gc
*   P    ~ rho*U*U/gc
*	
*     Inputs:
*
*     amur	-	reference viscosity (mu@Tref)
*     csuth	-	constant in Sutherland's Law
*     gamma	-	ratio of specific heats
*     gc	-	gravitational constant
*     jcp	-	j-index for reference pressure in 
*			computing cp
*     jsv(m)	-	array of j-indexes at which to write out 
*			velocity profiles
*     kstep	-	k-index of step (x/H)=1
*     msv	-	number of stations to write out velocity
*			and turbulent kinetic energy profiles
*     pr	-	reference pressure    used in restart file
*     tr	-	reference temperature used in restart file
*     ttau(m,n)	-	turbulent shear stress
*     ur	-       reference velocity used in nondimensionalizing
*			output
*     xlr	-	reference length used in nparc restart file
*     x,y	-	nondimensional x and y coordinates
*     H         -       step height
*
*
*     Outputs:
*     10        -       axial boundary layer info (Cf,disp,momth)
*     11        -       pressure coefficient info 
*     12        -       profile data at several axial stations
*     13        -       yplus at wall for each axial location
*     14        -       massflow through duct
*
*     

      parameter(nx=400, ny=250)
      dimension x(nx,ny), y(nx,ny), q(nx,ny,4), tq(nx,ny,3)
      real cf(nx), en(nx,ny),  mach(nx,ny), momenx(nx,ny),
     1     momax(nx), mu(nx,ny), ps(nx,ny), pt(nx,ny), rho(nx,ny),
     2     sumgra,
     3     ts(nx,ny), tauw(nx), ttau(nx,ny),vxmax(nx), vt(nx,ny), 
     4     vx(nx,ny), vy(nx,ny), yplus(nx) 
      real delst, thmom, mdot
      integer nmax(nx), jsv(40) 
      namelist /inputs/csuth, gamma, rgas, gc, jcp, jsv, kstep, msv,
     1                 pr, tr, tsuth, ur, xr, H
*
*-----initialization
*     
      do 10 m = 1, 40
      jsv(m) = 0
10    continue
*
*-----reference conditions
*
      write(*,*) 'Reading namelist inputs from fort.5'
      read(5,inputs)
      if (H.lt.0.0) H=-1./H  ! -12 ft = 1/12 ft = 1 inch
      gami = gamma - 1.0
      ar = sqrt(gamma*rgas*tr*gc)
      amur = csuth*tr**(1.5)/(tr + tsuth)
      rhor = pr/rgas/tr
*
*-----read NPARC restart file
*
      write(*,*) 'Reading NPARC restart file from fort.4'
      read(4) nc1
      read(4) jmax, kmax
      read(4) ((x(j,k), j=1,jmax), k=1,kmax),
     &        ((y(j,k), j=1,jmax), k=1,kmax) 
      read(4) ((( q(j,k,n), j=1,jmax), k=1,kmax), n=1,4)
      read(4) (((tq(j,k,n), j=1,jmax), k=1,kmax), n=1,3)
*
*-----Begin computations:
*
    9 format(5a)
      write(*,*) 'Computing dimensional values'
      write(*,*) 'Variables="j","kVxmax","Vxmax"'
      write(*,19) nc1
   19 format('Zone T="',i6,'", F=POINT')
      write(*,*) '#     x-index Vxmax-index      Vxmax'
      do 40 m = 1, jmax
        vxmax(m) = 0.00001
        momax(m) = 0.00001
        nmax(m)  = 1
*-------Compute dimensional flow variables
        do 20 n = 1, kmax
          x(m,n)   = x(m,n)*xr
          y(m,n)   = y(m,n)*xr
          rho(m,n) = q(m,n,1)*rhor
          vx(m,n)  = q(m,n,2)/q(m,n,1)*ar
          vy(m,n)  = q(m,n,3)/q(m,n,1)*ar
          vt(m,n)  = sqrt(vx(m,n)**2 + vy(m,n)**2)
          en(m,n)  = q(m,n,4)*gamma*pr
          ps(m,n)  = (en(m,n) - 0.5*rho(m,n)*vt(m,n)**2/gc)*gami
          ts(m,n)  = ps(m,n)/rho(m,n)/rgas
          mach(m,n)   = vt(m,n)/(gamma*rgas*ts(m,n)*gc)**0.5
          momenx(m,n) = rho(m,n)*vx(m,n)
          pt(m,n)  = ps(m,n)*(1. + .5*gami*mach(m,n)**2)**(gamma/gami)
          mu(m,n)  = csuth*(ts(m,n)**1.5)/(ts(m,n) + tsuth)
*---------Search for maximima
          if (vx(m,n) .gt. vxmax(m)) then
            if (x(m,n).gt.0.0 .or. n.ge.kstep) then
             vxmax(m) = vx(m,n)
             nmax(m)  = n
            endif
          endif
          if (momenx(m,n) .gt. momax(m)) momax(m) = momenx(m,n)
20      continue
        write(*,*) m, nmax(m), vxmax(m)
40    continue
*
*-----Compute shear stress on lower wall, y+ at first grid point off wall
*
      write(*,*) 'Computing shear stress'
      write(13,9) '# x-index     x/H        yplus'
      do 50 m = 1, jmax
*-------accounting for different indexes upstream of step
        k1 = 1
        if (x(m,1) .le. 0.0) k1 = kstep
        k2 = k1 + 1
        dely     = y(m,k2)-y(m,k1)
        tauw(m)  = mu(m,k1)*vx(m,k2)/dely
        cf(m)    = tauw(m) / (0.5*rho(m,nmax(m))*vxmax(m)**2/gc)
        utau     = sqrt(gc*abs(tauw(m))/rho(m,k1))
        yplus(m) = dely*utau*rho(m,k2)/(mu(m,k1)*gc)
        write(13,1000) m, x(m,k1)/H, yplus(m)
50    continue
*
*-----Check y+ in b.layer at last streamwise station
c      write(13,*) 'yplus distribution at last streamwise location'
c      write(13,*) 'y-index    y/H        yplus'
c      do 55 n = 2, kmax-2
c        utau = sqrt(tauw(jmax)/rho(jmax,2))
c        yplus(n) = y(jmax,n)*utau/mu(jmax,n)*rho(jmax,n)
c        write(13,1000) n, y(jmax,n)/H, yplus(n)
c55    continue
*
*-----Compute Momentum and Displacement Thickness
*
      write(*,*) 'Computing displacement + momentum thicknesses'
      write(10,9)
     &   'Variables="x_H","Cf","dCf","Vxmax","delst_H","momth_H"'
      write(10,19) nc1
      write(10,9) '#     x/H           Cf          dCf        '
     &           ,'Vxmax      delst/H      momth/H'
      do 70 m = 1, jmax
        vmax  = 0.0
        delst = 0.0
        thmom = 0.0
        k1 = 1
        if (x(m,1) .le. 0.0) k1 = kstep
        do 60 n = k1, kmax
c          if (vx(m,n) .gt. vmax) vmax = vx(m,n)
c          if (vx(m,n) .lt. vmax .and. vx(m,n) .gt. 0.0) go to 60
          if (n .gt. nmax(m)) go to 60
          f1 = 1.0 - momenx(m,n  )/momax(m)
          f2 = 1.0 - momenx(m,n+1)/momax(m)
          g1 = momenx(m,n  )/momax(m)*(1.0 - vx(m,n  )/vxmax(m))
          g2 = momenx(m,n+1)/momax(m)*(1.0 - vx(m,n+1)/vxmax(m))
          dely  = y(m,n+1) - y(m,n)
          delst = delst + 0.5*dely*(f1 + f2)
          thmom = thmom + 0.5*dely*(g1 + g2)
60      continue
        write(10, 1000) x(m,1)/H, cf(m), 0.,
     &                  vxmax(m), delst/H, thmom/H
70    continue
*
*-----Compute static pressure coefficient on stepside and opposite walls
*
*-----Change jcp indexes for backstep
      write(*,*) 'Computing static pressure coefficient'
      write(11,9) 'Variables="x_H","Cpstep","Cpopp"'
      write(11,19) nc1
      write(11,9) '# x/H     Cp(step) Cp(opposite)'
      do 90 m = 1, jmax
        k1=1
        if (x(m,1) .le. 0.0) k1 = kstep
        ur2r  = vxmax(jcp)**2*rho(jcp,1)
        pfact = 0.5*ur2r
        cps   = (ps(m,k1)   - ps(jcp,k1)  )/pfact
        cpo   = (ps(m,kmax) - ps(jcp,kmax))/pfact 
        write(11,1000) x(m,1)/H, cps, cpo
90    continue
*
*-----Velocity, Turbulent Kinetic Energy profiles, Turbulent Shear
*     Stress (automated to print out at desired stations specified
*     through namelist input)
*
*-----First compute turbulent shear stress (assumes top & bottom
*      walls are solid.  Also accounts for step.)
*
c      write(12,9) 'x-index   y-index   y/H       dely/H      delv   ttau'
      write(*,*) 'Generating profile data'
      do 120 m = 1, jmax-1
        k1=1
        if (x(m,1).lt.0.0) k1=kstep
        do 110 n = 1, kmax
*-------use 1st order diff at walls, 2nd order central elsewhere
        if (m .eq. 1) then
           delx  =  x(m+1,n) -  x(m,n) 
           delvy = vy(m+1,n) - vy(m,n)
        else
           delx  =  x(m+1,n) - x(m-1,n)
           delvy = vy(m+1,n) - vy(m-1,n)
        endif
        if (n .eq. k1) then
           dely  = y(m,n+1)-y(m,n)
           delvx = vx(m,n+1)
        else if (n .eq. kmax) then
           dely  = y(m,n)-y(m,n-1)
           delvx = vx(m,n-1)
        else
           dely  = (y(m,n+1)-y(m,n-1)) 
           delvx =  vx(m,n+1) - vx(m,n-1)
        endif
        sumgra = abs(delvx/dely + delvy/delx)
c       ttau = -uv/ur^2
        ttau(m,n) = tq(m,n,3)*amur*sumgra/(rho(m,n)*ur**2/gc)
c        write(12,*) y(m,n)/H, dely/H, delv, ttau(m,n)
110     continue
120   continue
      write(12,9)
     &  'Variables="y_H","Vx_U","Vy_U","k_U2","ttau_rhoU2"'
      do 140 m = 1, msv
        mi = jsv(m)
        if (x(mi,1) .le. 0.0) then
           k1 = kstep
           ystep = -1.0
        else
           k1 = 1
           ystep = 0.0
        endif
        write(12,1100) x(mi,1)/H
        write(12,1050) mi
        write(12,9) '#     y/H        Vx/ur        ',
     &              'Vy/ur      k/ur2        ttau/rhoU2'
        do 130 n = k1, kmax
          write(12,1000) y(mi,n)/H      , vx(mi,n)/ur, vy(mi,n)/ur,
     1                  tq(mi,n,1)*ar**2/ur**2, 
     2                  ttau(mi,n)
130     continue
140   continue
1000  format(6e13.5)
1050  format('# x-index location = ',i3)
c1100  format('Zone T="x/H=',f8.5,'" F=POINT')
1100  format('Zone T="x/H=',f5.2,'" F=POINT')
c
c-----compute massflow at each x-location
      write(*,*) 'Computing massflow'
      write(14,9) 'Variables="j","x/H","mdot"'
      write(14,19) nc1
      do j=1,jmax
        mdot=0.0
        k1=1
        if (x(j,1).le.0.0) k1=kstep
        do k=k1,kmax
          kp=min(k+1,kmax)
          km=max(k-1,k1)
          dy=0.5*(y(j,kp)-y(j,km))
          mdot=mdot + rho(j,k)*vx(j,k)*dy
        enddo
        write(14,*) j,x(j,1)/H,mdot
      enddo
      write(*,*) 'Program complete'
      end
