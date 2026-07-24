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

\*---------------------------------------------------------------------------*/

#include "codedDimensionedFieldFunctionTemplate.H"
#include "read.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace DimensionedFieldFunctions
{
    defineTypeNameAndDebug
    (
        UDimensionedFieldFunctionvolVectorField__Internal,
        0
    );
}

DimensionedFieldFunction<volVectorField::Internal>::
addRemovabledictionaryConstructorToTable
<
    DimensionedFieldFunctions::
    UDimensionedFieldFunctionvolVectorField__Internal
>
UDimensionedFieldFunctionvolVectorField__InternalConstructorToTable_;

}


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

extern "C"
{
    // Unique function name that can be checked
    // to ensure the correct library version has been loaded
    void U_4b1792a711fb2657a1235345651e527383b63fc8(bool load)
    {
        if (load)
        {
            // code that can be explicitly executed after loading
        }
        else
        {
            // code that can be explicitly executed before unloading
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::
UDimensionedFieldFunctionvolVectorField__Internal
(
    const dictionary& dict,
    volVectorField::Internal& field_
)
:
    DimensionedFieldFunction<volVectorField::Internal>(dict, field_),
    field(field_)
{
    if (false)
    {
        Info<< "Construct U sha1: 4b1792a711fb2657a1235345651e527383b63fc8 from dictionary\n";
    }
}


Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::
UDimensionedFieldFunctionvolVectorField__Internal
(
    const UDimensionedFieldFunctionvolVectorField__Internal& dff,
    volVectorField::Internal& field_
)
:
    DimensionedFieldFunction<volVectorField::Internal>(dff, field_),
    field(field_)
{}


Foam::autoPtr<Foam::DimensionedFieldFunction<Foam::volVectorField::Internal>>
Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::
clone
(
    volVectorField::Internal& field_
) const
{
    return autoPtr<DimensionedFieldFunction<volVectorField::Internal>>
    (
        new UDimensionedFieldFunctionvolVectorField__Internal
        (
            *this,
            field_
        )
    );
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::
~UDimensionedFieldFunctionvolVectorField__Internal()
{
    if (false)
    {
        Info<< "Destroy U sha1: 4b1792a711fb2657a1235345651e527383b63fc8\n";
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::evaluate()
{
    using namespace dimensions;

    // Local reference to time
    const dimensionedScalar& t(field.time());
    ignore(t);

    // Local reference to the field value locations
    // (points, cell centres, face centres)
    const DimensionedField<vector, GeoMesh, Field>& C(field.mesh().C());
    ignore(C);

//{{{ begin code
    #line 26 "/home/kol/OpenFOAM/OpenFOAM-14/test/Lagrangian/tracer/0/U!internalField"

        const dimensionedVector omega =
            Foam::read<dimensionedScalar>("60 [ rpm]")*vector(0, 0, 1);

        const dimensionedVector O(dimLength, field.mesh().bounds().midpoint());

        field = omega^(C - O);
    
//}}} end code
}


bool Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::update()
{
    using namespace dimensions;

//{{{ begin code
    bool updated = false;
    
    return updated;
//}}} end code
}


void Foam::DimensionedFieldFunctions::
UDimensionedFieldFunctionvolVectorField__Internal::
write
(
    Ostream& os
) const
{
    NotImplemented;
}


// ************************************************************************* i/

