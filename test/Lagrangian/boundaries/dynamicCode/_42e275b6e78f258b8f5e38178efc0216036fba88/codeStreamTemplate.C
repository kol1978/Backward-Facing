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
    Template for use with codeStream.

\*---------------------------------------------------------------------------*/

#include "dictionaryEntry.H"
#include "fieldTypes.H"
#include "Ostream.H"
#include "Pstream.H"
#include "read.H"
#include "unitConversion.H"

//{{{ begin codeInclude

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
    void codeStream_42e275b6e78f258b8f5e38178efc0216036fba88
    (
        Ostream& os,
        const dictionary& dict
    )
    {
//{{{ begin code
        #line 127 "/home/kol/OpenFOAM/OpenFOAM-13/test/Lagrangian/boundaries/system/blockMeshDict/#codeStream"

            const List<scalar> ys0(dict.lookupScoped<List<scalar>>("ys0", true, false));

            const scalar theta = dict.lookupScoped<scalar>("theta", true, false);
            const scalar tanPhi = tan(degToRad(dict.lookupScoped<scalar>("phi", true, false)));

            forAll(ys0, yi)
            {
                for (label zi = 0; zi < 2; ++ zi)
                {
                    os  << "arc "
                        << zi*4*ys0.size() + yi*4+2 << ' '
                        << zi*4*ys0.size() + yi*4+3
                        << ' ' << 2*theta << vector(0, zi ? tanPhi : -tanPhi, -1);
                }
            }
        
//}}} end code
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

