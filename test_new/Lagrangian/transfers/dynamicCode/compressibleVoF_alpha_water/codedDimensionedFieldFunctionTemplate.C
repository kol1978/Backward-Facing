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
        compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal,
        0
    );
}

DimensionedFieldFunction<volScalarField::Internal>::
addRemovabledictionaryConstructorToTable
<
    DimensionedFieldFunctions::
    compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal
>
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__InternalConstructorToTable_;

}


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

extern "C"
{
    // Unique function name that can be checked
    // to ensure the correct library version has been loaded
    void compressibleVoF_alpha_water_3728ad66820da6763c421033000b6f83e057d886(bool load)
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
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal
(
    const dictionary& dict,
    volScalarField::Internal& field_
)
:
    DimensionedFieldFunction<volScalarField::Internal>(dict, field_),
    field(field_)
{
    if (false)
    {
        Info<< "Construct compressibleVoF_alpha_water sha1: 3728ad66820da6763c421033000b6f83e057d886 from dictionary\n";
    }
}


Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal
(
    const compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal& dff,
    volScalarField::Internal& field_
)
:
    DimensionedFieldFunction<volScalarField::Internal>(dff, field_),
    field(field_)
{}


Foam::autoPtr<Foam::DimensionedFieldFunction<Foam::volScalarField::Internal>>
Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::
clone
(
    volScalarField::Internal& field_
) const
{
    return autoPtr<DimensionedFieldFunction<volScalarField::Internal>>
    (
        new compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal
        (
            *this,
            field_
        )
    );
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::
~compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal()
{
    if (false)
    {
        Info<< "Destroy compressibleVoF_alpha_water sha1: 3728ad66820da6763c421033000b6f83e057d886\n";
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::evaluate()
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
    #line 27 "/home/kol/OpenFOAM/OpenFOAM-14/test/Lagrangian/transfers/0/compressibleVoF/alpha.water!internalField"

        const volScalarField::Internal z(C.component(2));

        const dimensionedScalar h = Foam::read<dimensionedScalar>("2.000000000000e-01 [ m]");
        const dimensionedScalar dh = Foam::read<dimensionedScalar>("4.000000000000e-02 [ m]");

        // Linear ramp over the interface thickness
        tmp<volScalarField::Internal> r =
            min(max(0.5 + (h - z)/dh, scalar(0)), scalar(1));

        // Smooth cosine function for the volume fraction
        field = (1 - cos(constant::mathematical::pi*r))/2;
    
//}}} end code
}


bool Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::update()
{
    using namespace dimensions;

//{{{ begin code
    bool updated = false;
    
    return updated;
//}}} end code
}


void Foam::DimensionedFieldFunctions::
compressibleVoF_alpha_waterDimensionedFieldFunctionvolScalarField__Internal::
write
(
    Ostream& os
) const
{
    NotImplemented;
}


// ************************************************************************* i/

