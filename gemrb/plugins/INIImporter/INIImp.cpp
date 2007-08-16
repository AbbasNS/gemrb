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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "INIImp.h"
#include "../Core/FileStream.h"

INIImp::INIImp(void)
{
	str = NULL;
	autoFree = false;
}

INIImp::~INIImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
	for (unsigned int i = 0; i < tags.size(); i++)
		delete( tags[i] );
}

bool INIImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	int cnt = 0;
	char* strbuf = ( char* ) malloc( 4097 );
	INITag* lastTag = NULL;
	do {
		cnt = str->ReadLine( strbuf, 4096 );
		if (cnt == -1)
			break;
		if (cnt == 0)
			continue;
		if (strbuf[0] == ';') //Rem
			continue;
		if (strbuf[0] == '[') {
			// this is a tag
			char* sbptr = strbuf + 1;
			char* TagName = sbptr;
			while (*sbptr != '\0') {
				if (*sbptr == ']') {
					*sbptr = 0;
					break;
				}
				sbptr++;
			}
			INITag* it = new INITag( TagName );
			tags.push_back( it );
			lastTag = it;
			continue;
		}
		if (lastTag == NULL)
			continue;
		lastTag->AddLine( strbuf );
	} while (true);
	free( strbuf );
	return true;
}

const char* INIImp::GetKeyAsString(const char* Tag, const char* Key,
	const char* Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsString( Key, Default );
		}
	}
	return Default;
}

const int INIImp::GetKeyAsInt(const char* Tag, const char* Key,
	const int Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsInt( Key, Default );
		}
	}
	return Default;
}

const bool INIImp::GetKeyAsBool(const char* Tag, const char* Key,
	const bool Default)
{
	for (unsigned int i = 0; i < tags.size(); i++) {
		const char* TagName = tags[i]->GetTagName();
		if (stricmp( TagName, Tag ) == 0) {
			return tags[i]->GetKeyAsBool( Key, Default );
		}
	}
	return Default;
}
