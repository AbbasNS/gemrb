/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/2DAImporter/2DAImp.cpp,v 1.20 2005/06/14 22:29:35 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "2DAImp.h"
#include "../Core/FileStream.h"

p2DAImp::p2DAImp(void)
{
	str = NULL;
	autoFree = false;
}

p2DAImp::~p2DAImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	for (unsigned int i = 0; i < ptrs.size(); i++) {
		free( ptrs[i] );
	}
}

bool p2DAImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[256];
	str->CheckEncrypted();

	str->ReadLine( Signature, sizeof(Signature) );
	char* strp = Signature;
	while (*strp == ' ')
		strp++;
	if (strncmp( strp, "2DA V1.0", 8 ) != 0) {
		printMessage("2DAImporter", "Bad signature!\n",YELLOW );
// we don't care about this, so exptable.2da of iwd2 won't cause a bigger problem
//		return false;
	}
	Signature[0] = 0;
	str->ReadLine( Signature, sizeof(Signature) );
	strncpy(defVal, Signature, sizeof(defVal) );
	bool colHead = true;
	int row = 0;
	while (true) {
		char* line = ( char* ) malloc( 1024 );
		int len = str->ReadLine( line, 1023 );
		if (len <= 0 || line[0] == '#') {
			free( line );
			break;
		}
		if (len < 1024)
			line = ( char * ) realloc( line, len + 1 );
		ptrs.push_back( line );
		if (colHead) {
			colHead = false;
			char* str = strtok( line, " " );
			while (str != NULL) {
				colNames.push_back( str );
				str = strtok( NULL, " " );
			}
		} else {
			char* str = strtok( line, " " );
			if (str == NULL)
				continue;
			rowNames.push_back( str );
			RowEntry r;
			rows.push_back( r );
			while (( str = strtok( NULL, " " ) ) != NULL) {
				rows[row].push_back( str );
			}
			row++;
		}
	}
	return true;
}
