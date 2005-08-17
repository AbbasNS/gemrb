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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DLGImporter/DLGImp.cpp,v 1.18 2005/08/17 18:29:29 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "DLGImp.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"

DLGImp::DLGImp(void)
{
	str = NULL;
	autoFree = false;
	Version = 0;
}

DLGImp::~DLGImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool DLGImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strnicmp( Signature, "DLG V1.0", 8 ) != 0) {
		printMessage( "DLGImporter", "Not a valid DLG File...", WHITE );
		printStatus( "ERROR", LIGHT_RED );
		Version = 0;
		return false;
	}
	str->ReadDword( &StatesCount );
	str->ReadDword( &StatesOffset );
	// bg2
	if (StatesOffset == 0x34 ) {
		Version = 104;
	}
	else {
		Version = 100;
	}
	str->ReadDword( &TransitionsCount );
	str->ReadDword( &TransitionsOffset );
	str->ReadDword( &StateTriggersOffset );
	str->ReadDword( &StateTriggersCount );
	str->ReadDword( &TransitionTriggersOffset );
	str->ReadDword( &TransitionTriggersCount );
	str->ReadDword( &ActionsOffset );
	str->ReadDword( &ActionsCount );
	if (Version == 104) {
		str->ReadDword( &Flags );
	}
	else {
		Flags = 0;
	}
	return true;
}

Dialog* DLGImp::GetDialog()
{
	if(!Version) {
		return NULL;
	}
	Dialog* d = new Dialog();
	d->Flags = Flags;
	d->TopLevelCount = StatesCount;
	d->Order = (unsigned int *) calloc (StatesCount, sizeof(unsigned int *) );
	d->initialStates = (DialogState **) calloc (StatesCount, sizeof(DialogState *) );
	for (unsigned int i = 0; i < StatesCount; i++) {
		DialogState* ds = GetDialogState( d, i );
		d->initialStates[i] = ds;
	}
	return d;
}

DialogState* DLGImp::GetDialogState(Dialog *d, unsigned int index)
{
	DialogState* ds = new DialogState();
	//16 = sizeof(State)
	str->Seek( StatesOffset + ( index * 16 ), GEM_STREAM_START );
        ieDword  FirstTransitionIndex;
        ieDword  TriggerIndex;
	str->ReadDword( &ds->StrRef );
	str->ReadDword( &FirstTransitionIndex );
	str->ReadDword( &ds->transitionsCount );
	str->ReadDword( &TriggerIndex );
	ds->trigger = GetStateTrigger( TriggerIndex );
	ds->transitions = GetTransitions( FirstTransitionIndex, ds->transitionsCount );
	if (TriggerIndex<StatesCount)
		d->Order[TriggerIndex] = index;
	return ds;
}

DialogTransition** DLGImp::GetTransitions(unsigned int firstIndex, unsigned int count)
{
	DialogTransition** trans = ( DialogTransition** )
		malloc( count*sizeof( DialogTransition* ) );
	for (unsigned int i = 0; i < count; i++) {
		trans[i] = GetTransition( firstIndex + i );
	}
	return trans;
}

DialogTransition* DLGImp::GetTransition(unsigned int index)
{
	if (index >= TransitionsCount) {
		return NULL;
	}
	//32 = sizeof(Transition)
	str->Seek( TransitionsOffset + ( index * 32 ), GEM_STREAM_START );
	DialogTransition* dt = new DialogTransition();
	str->ReadDword( &dt->Flags );
	str->ReadDword( &dt->textStrRef );
	if (!(dt->Flags & IE_DLG_TR_TEXT)) {
		dt->textStrRef = 0xffffffff;
	}
	str->ReadDword( &dt->journalStrRef );
	if (!(dt->Flags & IE_DLG_TR_JOURNAL)) {
		dt->journalStrRef = 0xffffffff;
	}
	ieDword TriggerIndex;
	ieDword ActionIndex;
	str->ReadDword( &TriggerIndex );
	str->ReadDword( &ActionIndex );
	str->ReadResRef( dt->Dialog );
	str->ReadDword( &dt->stateIndex );
	if (dt->Flags &IE_DLG_TR_TRIGGER) {
		dt->trigger = GetTransitionTrigger( TriggerIndex );
	}
	else {
		dt->trigger = NULL;
	}
	if (dt->Flags & IE_DLG_TR_ACTION) {
		dt->action = GetAction( ActionIndex );
	}
	else {
		dt->action = NULL;
	}
	return dt;
}

DialogString* DLGImp::GetStateTrigger(unsigned int index)
{
	if (index >= StateTriggersCount) {
		return NULL;
	}
	//8 = sizeof(VarOffset)
	str->Seek( StateTriggersOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	//a zero length trigger counts as no trigger
	//a // comment counts as true(), so we simply ignore zero
	//length trigger text like it isn't there
	if (!Length) {
		return NULL;
	}
	DialogString* ds = new DialogString();
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

DialogString* DLGImp::GetTransitionTrigger(unsigned int index)
{
	if (index >= TransitionTriggersCount) {
		return NULL;
	}
	str->Seek( TransitionTriggersOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	DialogString* ds = new DialogString();
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

DialogString* DLGImp::GetAction(unsigned int index)
{
	if (index >= ActionsCount) {
		return NULL;
	}
	str->Seek( ActionsOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	DialogString* ds = new DialogString();
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

int GetActionLength(const char* string)
{
	int i;
	int level = 0;
	bool quotes = true;
	const char* poi = string;

	for (i = 0; *poi; i++) {
		switch (*poi++) {
			case '"':
				quotes = !quotes;
				break;
			case '(':
				if (quotes) {
					level++;
				}
				break;
			case ')':
				if (quotes && level) {
					level--;
					if (level == 0) {
						return i + 1;
					}
				}
				break;
			default:
				break;
		}
	}
	return i;
}

#define MyIsSpace(c) (((c) == ' ') || ((c) == '\n') || ((c) == '\r'))

/* this function will break up faulty script strings that lack the CRLF
   between commands, common in PST dialog */
char** DLGImp::GetStrings(char* string, unsigned int& count)
{
	int col = 0;
	int level = 0;
	bool quotes = true;
	bool ignore = false;
	char* poi = string;

	count = 0;
	while (*poi) {
		switch (*poi++) {
			case '/':
				if(col==0) {
					if(*poi=='/') {
						poi++;
						ignore=true;
					}
				}
			case '"':
				quotes = !quotes;
				break;
			case '(':
				if (quotes) {
					level++;
				}
				break;
			case ')':
				if (quotes && level) {
					level--;
					if (level == 0) {
						if(!ignore) {
							count++;
						}
						ignore=false;
					}
				}
				break;
			default:
				break;
		}
	}
	if(!count) {
		return NULL;
	}
	char** strings = ( char** ) calloc( count, sizeof( char* ) );
	if (strings == NULL) {
		count = 0;
		return strings;
	}
	poi = string;
	for (unsigned int i = 0; i < count; i++) {
		while (MyIsSpace( *poi ))
			poi++;
		int len = GetActionLength( poi );
		if((*poi=='/') && (*(poi+1)=='/') ) {
			poi+=len;
			continue;
		}
		strings[i] = ( char * ) malloc( len + 1 );
		int j;
		for (j = 0; len; poi++,len--) {
			if (isspace( *poi ))
				continue;
			strings[i][j++] = *poi;
		}
		strings[i][j] = 0;
	}
	return strings;
}
