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
    void codeStream_a4ac43e05fa17ec4c86b282b2bad33df412cd2ca
    (
        Ostream& os,
        const dictionary& dict
    )
    {
//{{{ begin code
        #line 158 "/home/kol/OpenFOAM/OpenFOAM-13/test/Lagrangian/boundaries/system/blockMeshDict/#codeStream"

            const List<scalar> ys0(dict.lookupScoped<List<scalar>>("ys0", true, false));

            const wordList patchNames(dict.lookupScoped<wordList>("patchNames", true, false));
            const wordList patchTypes(dict.lookupScoped<wordList>("patchTypes", true, false));
            const List<wordList> patchGroups(dict.lookupScoped<List<wordList>>("patchGroups", true, false));

            forAll(patchNames, yi)
            {
                for (label xi = 0; xi < 4; ++ xi)
                {
                    os  << word(patchNames[yi] + Foam::name(xi))
                        << token::BEGIN_BLOCK;

                    writeEntry(os, "type", patchTypes[yi]);

                    os  << "faces " << token::BEGIN_LIST
                        << FixedList<label, 4>({
                            4*ys0.size() + yi*4+xi,
                            4*ys0.size() + (yi + 1)*4+xi,
                            (yi + 1)*4+xi,
                            yi*4+xi})
                        << token::END_LIST << token::END_STATEMENT;

                    if (!patchGroups[yi].empty())
                    {
                        writeEntry(os, "inGroups", patchGroups[yi]);
                    }

                    if (patchTypes[yi] == "cyclic")
                    {
                        writeEntry<word>
                        (
                            os,
                            "neighbourPatch",
                            patchNames[yi] + Foam::name(xi + (xi % 2 ? -1 : +1))
                        );
                    }

                    os  << token::END_BLOCK;
                }
            }

            wordList frontAndBackPatchNames({"back", "front"});
            wordList frontAndBackPatchTypes({"empty", "wedge"});

            for (label xi = 0; xi < 4; xi += 2)
            {
                for (label zi = 0; zi < 2; ++ zi)
                {
                    os  << word
                        (
                            frontAndBackPatchNames[zi]
                          + frontAndBackPatchTypes[xi/2].capitalise()
                          + Foam::name(zi)
                        )
                        << token::BEGIN_BLOCK;

                    writeEntry(os, "type", frontAndBackPatchTypes[xi/2]);

                    os  << "faces " << token::BEGIN_LIST;
                    forAll(patchNames, yi)
                    {
                        os  << FixedList<label, 4>({
                                zi*4*ys0.size() + yi*4+xi,
                                zi*4*ys0.size() + yi*4+1+xi,
                                zi*4*ys0.size() + (yi + 1)*4+1+xi,
                                zi*4*ys0.size() + (yi + 1)*4+xi});
                    }
                    os  << token::END_LIST << token::END_STATEMENT;
                    os  << token::END_BLOCK;
                }
            }
        
//}}} end code
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

