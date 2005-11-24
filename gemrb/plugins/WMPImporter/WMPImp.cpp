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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WMPImporter/WMPImp.cpp,v 1.18 2005/11/24 17:44:10 wjpalenstijn Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/ResourceMgr.h"
#include "../Core/ImageMgr.h"
#include "../Core/AnimationMgr.h"
#include "WMPImp.h"

WMPImp::WMPImp(void)
{
	str = NULL;
	autoFree = false;
}

WMPImp::~WMPImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool WMPImp::Open(DataStream* stream, bool autoFree)
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
	if (strncmp( Signature, "WMAPV1.0", 8 ) != 0) {
		printf( "[WMPImporter]: This file is not a valid WMP File\n" );
		return false;
	}
	str->ReadDword( &WorldMapsCount );
	str->ReadDword( &WorldMapsOffset );

	return true;
}

WorldMapArray* WMPImp::GetWorldMapArray()
{
	WorldMapArray* ma = core->NewWorldMapArray(WorldMapsCount);
	for (unsigned int i=0;i<WorldMapsCount; i++) {
		WorldMap *m = ma->NewWorldMap( i );
		GetWorldMap( m, i );
	}
	return ma;
}

void WMPImp::GetWorldMap(WorldMap *m, unsigned int index)
{
	unsigned int i;

	str->Seek( WorldMapsOffset + index * 184, GEM_STREAM_START );

	str->ReadResRef( m->MapResRef );
	str->ReadDword( &m->Width );
	str->ReadDword( &m->Height );
	str->ReadDword( &m->MapNumber );
	str->ReadDword( &m->AreaName );
	str->ReadDword( &m->unknown1 );
	str->ReadDword( &m->unknown2 );
	str->ReadDword( &m->AreaEntriesCount );
	str->ReadDword( &m->AreaEntriesOffset );
	str->ReadDword( &m->AreaLinksOffset );
	str->ReadDword( &m->AreaLinksCount );
	str->ReadResRef( m->MapIconResRef );

	str->Seek( m->AreaEntriesOffset, GEM_STREAM_START );

	WMPAreaLink al;
	WMPAreaEntry ae;
	for (i = 0; i < m->AreaEntriesCount; i++) {
		m->SetAreaEntry(i,GetAreaEntry(&ae));
	}

	str->Seek( m->AreaLinksOffset, GEM_STREAM_START );
	for (i = 0; i < m->AreaLinksCount; i++) {
		m->SetAreaLink(i,GetAreaLink(&al));
	}

	// Load map bitmap
	if (!core->IsAvailable( IE_MOS_CLASS_ID )) {
		printMessage( "WMPImporter","No MOS Importer Available.\n", LIGHT_RED );
	} else {
		ImageMgr* mos = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
		DataStream* mosfile = core->GetResourceMgr()->GetResource( m->MapResRef, IE_MOS_CLASS_ID );
		mos->Open( mosfile, true ); //autofree
		m->SetMapMOS(mos->GetImage());
		core->FreeInterface( mos );
	}

	// Load location icon
	if (!core->IsAvailable( IE_BAM_CLASS_ID )) {
		printMessage( "WMPImporter","No BAM Importer Available.\n", LIGHT_RED );
		return;
	}
	AnimationMgr* icon = ( AnimationMgr* ) core->GetInterface( IE_BAM_CLASS_ID );
	DataStream* iconfile = core->GetResourceMgr()->GetResource( m->MapIconResRef, IE_BAM_CLASS_ID );
	icon->Open( iconfile, true ); //autofree

	for(i=0;i<m->AreaEntriesCount;i++) {
		WMPAreaEntry *ae = m->GetEntry(i);

		ae->MapIcon = icon->GetFrameFromCycle( ae->IconSeq, 0 );
	}
	core->FreeInterface( icon );
}

WMPAreaEntry* WMPImp::GetAreaEntry(WMPAreaEntry* ae)
{
	str->ReadResRef( ae->AreaName );
	str->ReadResRef( ae->AreaResRef );
	str->Read( ae->AreaLongName, 32 );
	str->ReadDword( &ae->AreaStatus );
	str->ReadDword( &ae->IconSeq );
	str->ReadDword( &ae->X );
	str->ReadDword( &ae->Y );
	str->ReadDword( &ae->LocCaptionName );
	str->ReadDword( &ae->LocTooltipName );
	str->ReadResRef( ae->LoadScreenResRef );

	for (unsigned int dir = 0; dir < 4; dir++) {
		str->ReadDword( &ae->AreaLinksIndex[dir] );
		str->ReadDword( &ae->AreaLinksCount[dir] );
	}
	str->Seek( 128, GEM_CURRENT_POS );

	return ae;
}

WMPAreaLink* WMPImp::GetAreaLink(WMPAreaLink* al)
{
	str->ReadDword( &al->AreaIndex );
	str->Read( al->DestEntryPoint, 32 );
	str->ReadDword( &al->DistanceScale );
	str->ReadDword( &al->DirectionFlags );
	for (unsigned k = 0; k < 5; k++) {
		str->ReadResRef( al->EncounterAreaResRef[k] );
	}
	str->ReadDword( &al->EncounterChance );
	str->Seek( 128, GEM_CURRENT_POS );

	return al;
}

int WMPImp::GetStoredFileSize(WorldMapArray *wmap)
{
	int headersize = 16;
	WorldMapsOffset = headersize;
	WorldMapsCount = wmap->GetMapCount();

	headersize += WorldMapsCount * 184;

	for (unsigned int i=0;i<WorldMapsCount; i++) {
		WorldMap *map = wmap->GetWorldMap(i);

		ieDword AreaEntriesCount = map->GetEntryCount();
		headersize += AreaEntriesCount * 240;

		ieDword AreaLinksCount = map->GetLinkCount();
		headersize += AreaLinksCount * 216;

	}

	return headersize;
}

int WMPImp::PutWorldMap(DataStream *stream, WorldMapArray *wmap)
{
	if (!stream || !wmap) {
		return -1;
	}

	stream->Write( "WMAPV1.0", 8);
	stream->WriteDword( &WorldMapsCount);
	stream->WriteDword( &WorldMapsOffset);

	return PutMaps( stream, wmap);
}

int WMPImp::PutLinks(DataStream *stream, WorldMap *wmap)
{
	char filling[128];

	memset (filling,0,sizeof(filling));
	for(unsigned i=0;i<wmap->AreaLinksCount;i++) {
		WMPAreaLink *al = wmap->GetLink(i);

		stream->WriteDword( &al->AreaIndex );
		stream->Write( al->DestEntryPoint, 32 );
		stream->WriteDword( &al->DistanceScale );
		stream->WriteDword( &al->DirectionFlags );
		for (unsigned k = 0; k < 5; k++) {
			stream->WriteResRef( al->EncounterAreaResRef[k] );
		}
		stream->WriteDword( &al->EncounterChance );
		stream->Write(filling,128);
	}
	return 0;
}

int WMPImp::PutAreas(DataStream *stream, WorldMap *wmap)
{
	char filling[128];

	memset (filling,0,sizeof(filling));
	for(unsigned i=0;i<wmap->AreaEntriesCount;i++) {
		WMPAreaEntry *ae = wmap->GetEntry(i);

		stream->WriteResRef( ae->AreaName );
		stream->WriteResRef( ae->AreaResRef );
		stream->Write( ae->AreaLongName, 32 );
		stream->WriteDword( &ae->AreaStatus );
		stream->WriteDword( &ae->IconSeq );
		stream->WriteDword( &ae->X );
		stream->WriteDword( &ae->Y );
		stream->WriteDword( &ae->LocCaptionName );
		stream->WriteDword( &ae->LocTooltipName );
		stream->WriteResRef( ae->LoadScreenResRef );

		for (unsigned int dir = 0; dir < 4; dir++) {
			stream->WriteDword( &ae->AreaLinksIndex[dir] );
			stream->WriteDword( &ae->AreaLinksCount[dir] );
		}
		stream->Write(filling,128);
	}
	return 0;
}

int WMPImp::PutMaps(DataStream *stream, WorldMapArray *wmap)
{
	unsigned int i;
	int ret;
	char filling[128];

	memset (filling,0,sizeof(filling));
	ieDword AreaEntriesOffset = WorldMapsOffset + WorldMapsCount * 184;
	ieDword AreaLinksOffset = AreaEntriesOffset;
	for (i=0;i<WorldMapsCount; i++) {
		WorldMap *map = wmap->GetWorldMap(i);

		AreaLinksOffset += map->GetEntryCount() * 240;
	}

	//map headers
	for (i=0;i<WorldMapsCount; i++) {
		ieDword AreaEntriesCount, AreaLinksCount;

		WorldMap *map = wmap->GetWorldMap(i);
		AreaLinksCount = map->GetLinkCount();
		AreaEntriesCount = map->GetEntryCount();

		stream->WriteResRef( map->MapResRef );
		stream->WriteDword( &map->Width );
		stream->WriteDword( &map->Height );
		stream->WriteDword( &map->MapNumber );
		stream->WriteDword( &map->AreaName );
		stream->WriteDword( &map->unknown1 );
		stream->WriteDword( &map->unknown2 );
		//???

		stream->WriteDword( &AreaEntriesCount );
		stream->WriteDword( &AreaEntriesOffset );
		stream->WriteDword( &AreaLinksOffset );
		stream->WriteDword( &AreaLinksCount );
		stream->WriteResRef( map->MapIconResRef );
		AreaEntriesOffset += AreaEntriesCount * 240;
		AreaLinksOffset += AreaLinksCount * 216;

		stream->Write( filling, 128);
	}

	//area entries
	for (i=0;i<WorldMapsCount; i++) {
		WorldMap *map = wmap->GetWorldMap(i);

		ret = PutAreas( stream, map);
		if (ret) {
			return ret;
		}
	}

	//links
	for (i=0;i<WorldMapsCount; i++) {
		WorldMap *map = wmap->GetWorldMap(i);

		ret = PutLinks( stream, map);
		if (ret) {
			return ret;
		}
	}
	return 0;
}
