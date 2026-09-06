Compiling enabled on 24 cores
gcc=/usr/bin/gcc
clang=
mpirun=/opt/intel/oneapi/mpi/2021.18/bin/mpirun
make=/usr/bin/make
cmake=/usr/bin/cmake
wmake=/home/kol/OpenFOAM/OpenFOAM-v2606/wmake/wmake
m4=/usr/bin/m4
flex=/usr/bin/flex

compiler=/opt/intel/oneapi/compiler/2026.1/bin/icpx
Intel(R) oneAPI DPC++/C++ Compiler 2026.1.1 (2026.1.1.20260724)
cxxflags="-std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template -O3 -march=westmere -frounding-math -DNoRepository -fPIC -fiopenmp"

========================================
2026-09-05 14:19:30 +0800
Starting compile OpenFOAM-v2606 Allwmake
  Icx system compiler [+openmp]
  linux64IcxDPInt64Opt, with INTELMPI intelmpi
========================================

built wmake-bin (linux64Icx)

========================================
Start ThirdParty Allwmake
========================================
using:  icx -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -O3 -march=westmere -frounding-math -fPIC
using:  icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template -O3 -march=westmere -frounding-math -DNoRepository -fPIC -fiopenmp

========================================
Build MPI libraries if required
    /opt/intel/oneapi/mpi/2021.18
Found sources: sources/scotch/scotch_6.1.0

========================================
scotch decomposition (scotch_6.1.0)
    Makefile.inc  : 
    scotch include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64IcxDPInt64/scotch_6.1.0/include
    scotch library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64IcxDPInt64/lib

========================================
pt-scotch decomposition (scotch_6.1.0 with intelmpi)
    ptscotch include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64IcxDPInt64/scotch_6.1.0/include/intelmpi
    ptscotch library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64IcxDPInt64/lib/intelmpi

========================================
KAHIP decomposition
    kahip include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/kahip-3.15/include
    kahip library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64IcxDPInt64/lib (libkahip.so)

========================================
METIS decomposition
Did not find ... metis-5.1.0
Missing sources: 'metis-5.1.0'
Possible download locations for metis :
    http://glaros.dtc.umn.edu/gkhome/metis/metis/overview
    https://github.com/KarypisLab/METIS/archive/refs/tags/v5.2.1.tar.gz

    ---------------------------------------------------
    Optional component (METIS) had build issues
    OpenFOAM will nonetheless remain largely functional
    ---------------------------------------------------


========================================
cgal/boost
    boost include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/boost_1_74_0/include
    boost library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/boost_1_74_0/lib64 (libboost_system.so)

========================================
FFTW
    fftw include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/fftw-3.3.10/include
    fftw library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/fftw-3.3.10/lib (libfftw3.so)

========================================
ADIOS2
    adios2 include: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/ADIOS2-2.12.1/intelmpi/include
    adios2 library: /home/kol/OpenFOAM/ThirdParty-v2606/platforms/linux64Icx/ADIOS2-2.12.1/intelmpi/lib (libadios2_cxx_mpi.so)

========================================
Done ThirdParty Allwmake
========================================

========================================
Compile OpenFOAM libraries
========================================
    ln: OpenFOAM/lnInclude
    ln: OSspecific/POSIX/lnInclude
    found <sys/inotify.h> -- using inotify for file monitoring
wmake libo (POSIX)
wmake -no-openmp dummy (mpi=INTELMPI)
wmake dummy
wmake -no-openmp (mpi=INTELMPI:intelmpi)
wmake mpi
wmake OpenFOAM
Making dependencies: pointFields.cxx
Making dependencies: codedFixedValuePointPatchFields.cxx
Making dependencies: nonuniformTransformCyclicPointPatchFields.cxx
Making dependencies: cyclicSlipPointPatchFields.cxx
Making dependencies: cyclicPointPatchFields.cxx
Making dependencies: pointMesh.C
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c meshes/pointMesh/pointMesh.C -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/meshes/pointMesh/pointMesh.o
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c fields/pointPatchFields/constraint/cyclic/cyclicPointPatchFields.cxx -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/fields/pointPatchFields/constraint/cyclic/cyclicPointPatchFields.o
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c fields/pointPatchFields/constraint/cyclicSlip/cyclicSlipPointPatchFields.cxx -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/fields/pointPatchFields/constraint/cyclicSlip/cyclicSlipPointPatchFields.o
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c fields/pointPatchFields/constraint/nonuniformTransformCyclic/nonuniformTransformCyclicPointPatchFields.cxx -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/fields/pointPatchFields/constraint/nonuniformTransformCyclic/nonuniformTransformCyclicPointPatchFields.o
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c fields/pointPatchFields/derived/codedFixedValue/codedFixedValuePointPatchFields.cxx -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/fields/pointPatchFields/derived/codedFixedValue/codedFixedValuePointPatchFields.o
icpx -std=c++17 -pthread -fp-model=precise -DOPENFOAM=2606 -DWM_DP -DWM_LABEL_SIZE=64 -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-parameter -Wno-invalid-offsetof -Wno-unknown-pragmas -Wno-undefined-var-template  -O3 -march=westmere -frounding-math  -DNoRepository  -I/home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM -DHAVE_LIBZ -DHAVE_EXTRAE -iquote. -IlnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OpenFOAM/lnInclude -I/home/kol/OpenFOAM/OpenFOAM-v2606/src/OSspecific/POSIX/lnInclude   -fPIC -fiopenmp -c fields/GeometricFields/pointFields/pointFields.cxx -o /home/kol/OpenFOAM/OpenFOAM-v2606/build/linux64IcxDPInt64Opt/src/OpenFOAM/fields/GeometricFields/pointFields/pointFields.o
