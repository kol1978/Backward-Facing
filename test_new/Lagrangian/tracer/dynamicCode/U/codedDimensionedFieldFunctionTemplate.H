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
    Template for use with dynamic code generation of a DimensionedFieldFunction.

    - without state

SourceFiles
    codedDimensionedFieldFunctionTemplate.C

\*---------------------------------------------------------------------------*/

#ifndef codedDimensionedFieldFunctionTemplate_H
#define codedDimensionedFieldFunctionTemplate_H

#include "DimensionedFieldFunction.H"
#include "DimensionedFvPatchFields.H"
#include "volFields.H"

//{{{ begin codeInclude

//}}} end codeInclude

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace DimensionedFieldFunctions
{

/*---------------------------------------------------------------------------*\
                     A templated DimensionedFieldFunction
\*---------------------------------------------------------------------------*/

class UDimensionedFieldFunctionvolVectorField__Internal
:
    public DimensionedFieldFunction<volVectorField::Internal>
{
    typedef typename volVectorField::Internal::Type_ Type;
    typedef typename volVectorField::Internal::GeoMesh_ GeoMesh;
    typedef typename GeoMesh::Zone Zone;


    // Private Data

        //- Local reference to the field
        volVectorField::Internal& field;


public:

    // Runtime type information
    TypeName("U");


    // Constructors

        //- Construct from entry name and dictionary
        UDimensionedFieldFunctionvolVectorField__Internal
        (
            const dictionary& dict,
            volVectorField::Internal& field
        );

        //- Construct a copy for the given field
        UDimensionedFieldFunctionvolVectorField__Internal
        (
            const
            UDimensionedFieldFunctionvolVectorField__Internal& dff,
            volVectorField::Internal& field
        );

        //- Construct and return a clone for the specified field
        virtual autoPtr<DimensionedFieldFunction<volVectorField::Internal>>
        clone(volVectorField::Internal& field) const;


    //- Destructor
    virtual ~UDimensionedFieldFunctionvolVectorField__Internal();


    // Member Functions

        //- Evaluate the function and set the field
        virtual void evaluate();

        //- Update the field if the function has changed
        virtual bool update();

        //- Write data to dictionary stream
        virtual void write(Ostream& os) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace DimensionedFieldFunctions
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

