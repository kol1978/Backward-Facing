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
    void codeStream_a807f7449b4d12fdb5f65f3a1fdcd3b46743a3d9
    (
        Ostream& os,
        const dictionary& dict
    )
    {
//{{{ begin code
        #line 91 "/home/kol/OpenFOAM/OpenFOAM-13/test/Lagrangian/boundaries/system/blockMeshDict/#codeStream"

            const List<scalar> ys0(dict.lookupScoped<List<scalar>>("ys0", true, false));

            const label nX = dict.lookupScoped<label>("nX", true, false);
            const List<label> nYs(dict.lookupScoped<List<label>>("nYs", true, false));

            const wordList zoneNames(dict.lookupScoped<wordList>("zoneNames", true, false));

            forAll(zoneNames, yi)
            {
                for (label xi = 0; xi < 4; xi += 2)
                {
                    os  << "hex "
                        << FixedList<label, 8>({
                            4*ys0.size() + yi*4+xi,
                            4*ys0.size() + yi*4+1+xi,
                            4*ys0.size() + (yi + 1)*4+1+xi,
                            4*ys0.size() + (yi + 1)*4+xi,
                            yi*4+xi,
                            yi*4+1+xi,
                            (yi + 1)*4+1+xi,
                            (yi + 1)*4+xi})
                        << zoneNames[yi]
                        << " (" << nX << ' ' << nYs[yi] << " 1) "
                        << "simpleGrading (1 1 1)";
                }
            }
        
//}}} end code
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

