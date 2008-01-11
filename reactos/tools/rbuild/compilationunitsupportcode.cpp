/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

CompilationUnitSupportCode::CompilationUnitSupportCode ( const Project& project )
	: project ( project )
{
}

CompilationUnitSupportCode::~CompilationUnitSupportCode ()
{
}

void
CompilationUnitSupportCode::Generate ( bool verbose )
{
	for ( size_t i = 0; i < project.modules.size (); i++ )
	{
		GenerateForModule ( *project.modules[i],
		                    verbose );
	}
}

void
CompilationUnitSupportCode::GenerateForModule ( Module& module,
                                                bool verbose )
{
	if ( verbose )
	{
		printf ( "\nGenerating compilation unit support code for %s",
		         module.name.c_str () );
	}

	for ( size_t i = 0; i < module.non_if_data.compilationUnits.size () ; i++ )
	{
		CompilationUnit& compilationUnit = *module.non_if_data.compilationUnits[i];
		if ( compilationUnit.GetFiles ().size () <= 1 )
			continue;
		WriteCompilationUnitFile ( module, compilationUnit );
	}
}

string
CompilationUnitSupportCode::GetCompilationUnitFilename ( Module& module,
                                                         CompilationUnit& compilationUnit )
{
	return NormalizeFilename ( Environment::GetIntermediatePath () + sSep + compilationUnit.name );
}

void
CompilationUnitSupportCode::WriteCompilationUnitFile ( Module& module,
                                                       CompilationUnit& compilationUnit )
{
	char* buf;
	char* s;

	buf = (char*) malloc ( 512*1024 );
	if ( buf == NULL )
		throw OutOfMemoryException ();

	s = buf;
	s = s + sprintf ( s, "/* This file is automatically generated. */\n" );
	s = s + sprintf ( s, "#define ONE_COMPILATION_UNIT\n" );

	for ( size_t i = 0; i < compilationUnit.GetFiles ().size () ; i++ )
	{
		const File& file = *compilationUnit.GetFiles ()[i];
		s = s + sprintf ( s, "#include <%s/%s>\n", ChangeSeparator ( file.file.relative_path, '\\', '/' ).c_str (), file.file.name.c_str () );
	}

	s = s + sprintf ( s, "\n" );

	FileSupportCode::WriteIfChanged ( buf, GetCompilationUnitFilename ( module, compilationUnit ) );

	free ( buf );
}
