/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) YEAR OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Description
    Template for use with codeBlock.

\*---------------------------------------------------------------------------*/

#include "dictionaryEntry.H"
#include "fieldTypes.H"
#include "Ostream.H"
#include "Pstream.H"
#include "read.H"
#include "units.H"

//{{{ begin codeInclude
#line 0 "/home/kol/OpenFOAM/kol-12/Backward-Facing_github/nacaAirfoil_n0012familyII2hex_compressible_SST/0/U!#codeBlock"
#include "transform.H"

//}}} end codeInclude

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

//{{{ begin localCode

//}}} end localCode


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

extern "C"
{
    #define CODE_BLOCK_STREAM_FUNCTION(index)                                  \
        void CAT3(codeBlock_830e46ad76846b4e80d4edaa059490117bf804d1, _, index)                             \
        (                                                                      \
            Ostream& os,                                                       \
            const dictionary& dict                                             \
        )

    #define CODE_BLOCK_DICT_FUNCTION(index)                                    \
        void CAT3(codeBlock_830e46ad76846b4e80d4edaa059490117bf804d1, _, index)                             \
        (                                                                      \
            dictionary& dict,                                                  \
            Istream& is                                                        \
        )

//{{{ begin code
    #line 0 "/home/kol/OpenFOAM/kol-12/Backward-Facing_github/nacaAirfoil_n0012familyII2hex_compressible_SST/0/U!#codeBlock"
CODE_BLOCK_STREAM_FUNCTION(0)
{
    #line 20 "/home/kol/OpenFOAM/kol-12/Backward-Facing_github/nacaAirfoil_n0012familyII2hex_compressible_SST/0/U!#codeBlock"
os << (transform(Ry(-dict.lookupScoped<scalar>("angleOfAttack", true, false)), vector(0, 0, 1)));
}

CODE_BLOCK_STREAM_FUNCTION(1)
{
    #line 21 "/home/kol/OpenFOAM/kol-12/Backward-Facing_github/nacaAirfoil_n0012familyII2hex_compressible_SST/0/U!#codeBlock"
os << (transform(Ry( dict.lookupScoped<scalar>("angleOfAttack", true, false)), vector(1, 0, 0)));
}

CODE_BLOCK_STREAM_FUNCTION(2)
{
    #line 23 "/home/kol/OpenFOAM/kol-12/Backward-Facing_github/nacaAirfoil_n0012familyII2hex_compressible_SST/0/U!#codeBlock"
os << (dict.lookupScoped<int64_t>("speed", true, false)*dict.lookupScoped<vector>("dragDir", true, false));
}


//}}} end code
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

