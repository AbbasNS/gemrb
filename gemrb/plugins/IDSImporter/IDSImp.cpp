/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IDSImporter/IDSImp.cpp,v 1.13 2004/04/13 22:53:50 doc_wagon Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "IDSImp.h"
#include "IDSImpDefs.h"
#include <ctype.h>

IDSImp::IDSImp(void)
{
	str = NULL;
	autoFree = false;
}

IDSImp::~IDSImp(void)
{
	if (str && autoFree) {
		delete( str );
	}

	for (unsigned int i = 0; i < ptrs.size(); i++) {
		free( ptrs[i] );
	}
}

bool IDSImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str) {
		return false;
	}
	str = stream;
	this->autoFree = autoFree;

	bool Encrypted = str->CheckEncrypted();
	char tmp[11];
	str->ReadLine( tmp, 10 );
	tmp[10] = 0;
	if (tmp[0] != 'I') {
		str->Seek( Encrypted ? 2 : 0, GEM_STREAM_START );
		str->Pos = 0;
	}
	while (true) {
		char* line = ( char* ) malloc( 256 );
		int len = str->ReadLine( line, 256 );
		strlwr( line );
		if (len == -1) {
			free( line );
			break;
		}
		if (len == 0) {
			free( line );
			continue;
		}
		if (len < 256)
			line = ( char * ) realloc( line, len + 1 );
		char* str = strtok( line, " " );
		Pair p;
		p.val = strtoul( str, NULL, 0 );
		str = strtok( NULL, " " );
		p.str = str;
		if (str != NULL) {
			ptrs.push_back( line );
			pairs.push_back( p );
		} else {
			free( line );
		}
	}

	return true;
}

long IDSImp::GetValue(const char* txt)
{
	for (unsigned int i = 0; i < pairs.size(); i++) {
		if (stricmp( pairs[i].str, txt ) == 0) {
			return pairs[i].val;
		}
	}
	return -1;
}

char* IDSImp::GetValue(int val)
{
	for (unsigned int i = 0; i < pairs.size(); i++) {
		if (pairs[i].val == val) {
			return pairs[i].str;
		}
	}
	return NULL;
}

char* IDSImp::GetStringIndex(unsigned int Index)
{
	if (Index >= pairs.size()) {
		return NULL;
	}
	return pairs[Index].str;
}

long IDSImp::GetValueIndex(unsigned int Index)
{
	if (Index >= pairs.size()) {
		return 0;
	}
	return pairs[Index].val;
}

int IDSImp::FindString(char *str, int len)
{
	int i=pairs.size();
	while(i--) {
		if (strnicmp(pairs[i].str, str, len) == 0) {
			return i;
		}
	}
	return -1;
}

int IDSImp::FindValue(int val)
{
	int i=pairs.size();
	while(i--) {
		if(pairs[i].val==val) {
			return i;
		}
	}
	return -1;
}

