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
#line 0 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
#include "transform.H"
#include "transform.H"
#include "Field.H"
#include "HashTable.H"

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
        void CAT3(codeBlock_fd20a72bfa785080e7a82b048952931175db6e62, _, index)                             \
        (                                                                      \
            Ostream& os,                                                       \
            const dictionary& dict                                             \
        )

    #define CODE_BLOCK_DICT_FUNCTION(index)                                    \
        void CAT3(codeBlock_fd20a72bfa785080e7a82b048952931175db6e62, _, index)                             \
        (                                                                      \
            dictionary& dict,                                                  \
            Istream& is                                                        \
        )

//{{{ begin code
    #line 0 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
CODE_BLOCK_STREAM_FUNCTION(0)
{
    #line 20 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (dict.lookupScoped<doubleScalar>("a", true, false)*dict.lookupScoped<doubleScalar>("b", true, false));
}

CODE_BLOCK_STREAM_FUNCTION(1)
{
    #line 28 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (dict.lookupScoped<doubleScalar>("a", true, false) / dict.lookupScoped<doubleScalar>("b", true, false));
}

CODE_BLOCK_STREAM_FUNCTION(2)
{
    #line 32 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << ((dict.lookupScoped<doubleScalar>("a", true, false))/dict.lookupScoped<doubleScalar>("b", true, false) + 1);
}

CODE_BLOCK_STREAM_FUNCTION(3)
{
    #line 39 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (dict.lookupScoped<doubleScalar>("a", true, false) / dict.lookupScoped<doubleScalar>("d/b", true, false));
}

CODE_BLOCK_STREAM_FUNCTION(4)
{
    #line 44 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (dict.lookupScoped<doubleScalar>("a", true, false) / dict.lookupScoped<doubleScalar>("d/b", true, false));
}

CODE_BLOCK_STREAM_FUNCTION(5)
{
    #line 48 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (dict.lookupScoped<string>("s", true, false) + "Name");
}

CODE_BLOCK_STREAM_FUNCTION(6)
{
    #line 51 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << ("movingBox_" + name(dict.lookupScoped<int64_t>("time", true, false)) + ".obj");
}

CODE_BLOCK_STREAM_FUNCTION(7)
{
    #line 52 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << ( "movingBox_" + name(dict.lookupScoped<int64_t>("time", true, false)) + ".obj" );
}

CODE_BLOCK_STREAM_FUNCTION(8)
{
    #line 59 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (transform(Ry(dict.lookupScoped<scalar>("angle", true, false)), vector(0, 0, 1)));
}

CODE_BLOCK_STREAM_FUNCTION(9)
{
    #line 60 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (transform(Ry(dict.lookupScoped<scalar>("angle", true, false)), vector(1, 0, 0)));
}

CODE_BLOCK_STREAM_FUNCTION(10)
{
    #line 64 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (mag(dict.lookupScoped<vector>("testCalc2!U", true, false)));
}

CODE_BLOCK_STREAM_FUNCTION(11)
{
    #line 65 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (1.5*magSqr(0.05*dict.lookupScoped<vector>("/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc2!U", true, false)));
}

CODE_BLOCK_STREAM_FUNCTION(12)
{
    #line 75 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"

    const vector U(dict.lookupScoped<vector>("testCalc2!U", true, false));
    const int nAngles = dict.lookupScoped<int64_t>("nAngles", true, false);
    const scalar angleStep = (dict.lookupScoped<scalar>("maxAngle", true, false))/(nAngles - 1);
    List<vector> Us(nAngles);
    for(int i=0; i<nAngles; i++)
    {
        const scalar angle = i*angleStep;
        Us[i] = transform(Ry(angle), U);
    }
    os << Us;

}

CODE_BLOCK_STREAM_FUNCTION(13)
{
    #line 90 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (mag(dict.lookupScoped<List<vector>>("listU", true, false)[1]));
}

CODE_BLOCK_STREAM_FUNCTION(14)
{
    #line 94 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (mag(dict.lookupScoped<Field<vector>>("listU", true, false)));
}

CODE_BLOCK_STREAM_FUNCTION(15)
{
    #line 95 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (sqr(dict.lookupScoped<Field<scalar>>("magUs", true, false)[1]));
}

CODE_BLOCK_STREAM_FUNCTION(16)
{
    #line 99 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (mag(dict.lookupScoped<HashTable<vector>>("namedU", true, false)["U2"]));
}

CODE_BLOCK_STREAM_FUNCTION(17)
{
    #line 103 "/home/kol/OpenFOAM/OpenFOAM-14/test/dictionary/testCalc!#codeBlock"
os << (mag(dict.lookupCompoundScoped<List<vector>>("listU2", true, false)[1]));
}


//}}} end code
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

