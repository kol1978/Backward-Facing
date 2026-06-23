/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) YEAR AUTHOR, AFFILIATION
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

#include "dictionary.H"
#include "Ostream.H"
#include "Pstream.H"
#include "pointField.H"
#include "tensor.H"
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

extern "C" void codeStream_472f28590f547329a82fbf7d658a0981516858a5(Foam::Ostream& os, const Foam::dictionary& dict)
{
//{{{ begin code
    os << (
    // Lookup the coefficients
    const scalar A = 1;
    const scalar B = 1;

    // Initial guess for x
    scalar x = 0;

    scalar x0 = x;

    do
    {
        // Store the previous iteration x for the convergence check
        x0 = x;

        // Temporary sub-function evaluations
        const scalar f1 = 1 + sqr(x);
        const scalar f2 = sqrt(f1);

        // Evaluate the function
        const scalar f = x + B*x/f2 - A;

        // Evaluate the derivative
        const scalar df = 1 + B/(f1*f2);

        // Update x
        x = x0 - f/df;

        // Test for convergence
    } while (mag(x - x0) > 1e-6);

    os << x;
);
//}}} end code
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

