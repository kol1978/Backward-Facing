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
    void codeStream_468200271ae88f4350f7fe4370bb6e7e33e7724b
    (
        Ostream& os,
        const dictionary& dict
    )
    {
//{{{ begin code
        #line 44 "/home/kol/OpenFOAM/OpenFOAM-13/test/Lagrangian/boundaries/system/blockMeshDict/#codeStream"

            const List<scalar> xs(dict.lookupScoped<List<scalar>>("xs", true, false));
            const vector c = dict.lookupScoped<vector>("c", true, false);
            const List<scalar> ys0(dict.lookupScoped<List<scalar>>("ys0", true, false));
            const List<scalar> ys1(dict.lookupScoped<List<scalar>>("ys1", true, false));
            const List<scalar> zs(dict.lookupScoped<List<scalar>>("zs", true, false));
            const scalar tanTheta = tan(degToRad(dict.lookupScoped<scalar>("theta", true, false)));
            const scalar tanPhi = tan(degToRad(dict.lookupScoped<scalar>("phi", true, false)));

            forAll(ys0, yi)
            {
                os  << vector(xs[0], ys0[yi], zs[0])
                    << vector(xs[1], ys1[yi], zs[0])
                    << vector(
                        c.x() - tanTheta*(ys0[yi] - c.y()),
                        ys0[yi],
                        c.z() - tanPhi*(ys0[yi] - c.y()))
                    << vector(
                        c.x() + tanTheta*(ys1[yi] - c.y()),
                        ys1[yi],
                        c.z() - tanPhi*(ys1[yi] - c.y()))
                    << nl;
            }

            forAll(ys0, yi)
            {
                os  << vector(xs[0], ys0[yi], zs[1])
                    << vector(xs[1], ys1[yi], zs[1])
                    << vector(
                        c.x() - tanTheta*(ys0[yi] - c.y()),
                        ys0[yi],
                        c.z() + tanPhi*(ys0[yi] - c.y()))
                    << vector(
                        c.x() + tanTheta*(ys1[yi] - c.y()),
                        ys1[yi],
                        c.z() + tanPhi*(ys1[yi] - c.y()))
                    << nl;
            }
        
//}}} end code
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

