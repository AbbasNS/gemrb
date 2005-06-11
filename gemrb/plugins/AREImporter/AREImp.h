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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.h,v 1.25 2005/06/11 20:17:59 avenger_teambg Exp $
 *
 */

#ifndef AREIMP_H
#define AREIMP_H

#include "../Core/MapMgr.h"

class AREImp : public MapMgr {
private:
	DataStream* str;
	bool autoFree;
	int bigheader;
	ieResRef WEDResRef;
	ieDword LastSave;
	ieDword AreaFlags;
	ieWord  AreaType, WRain, WSnow, WFog, WLightning, WUnknown;
	ieDword ActorOffset, EmbeddedCreOffset, AnimOffset, AnimCount;
	ieDword VerticesOffset;
	ieDword DoorsCount, DoorsOffset;
	ieDword ExploredBitmapSize, ExploredBitmapOffset;
	ieDword EntrancesOffset, EntrancesCount;
	ieDword SongHeader, RestHeader;
	ieWord  ActorCount, VerticesCount, AmbiCount;
	ieWord  ContainersCount, InfoPointsCount, ItemsCount;
	ieDword VariablesCount;
	ieDword NoteCount;
	ieDword ContainersOffset, InfoPointsOffset, ItemsOffset;
	ieDword AmbiOffset, VariablesOffset;
	ieDword NoteOffset;
	ieDword SpawnOffset, SpawnCount;
	ieResRef Script;
public:
	AREImp(void);
	~AREImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Map* GetMap(const char* ResRef);
	int GetStoredFileSize(Map *map);
	/* stores an area in the Cache (swaps it out) */
	int PutArea(DataStream *stream, Map *map);
	void release(void)
	{
		delete this;
	}
private:
	Animation *GetAnimationPiece(AnimationFactory *af, int animCycle, AreaAnimation *a);
	CREItem* GetItem();
	int PutHeader(DataStream *stream, Map *map);
	int PutPoints(DataStream *stream, Point *p, unsigned int count);
	int PutDoors(DataStream *stream, Map *map, ieDword &VertIndex);
	int PutItems(DataStream *stream, Map *map);
	int PutContainers(DataStream *stream, Map *map, ieDword &VertIndex);
	int PutRegions(DataStream *stream, Map *map, ieDword &VertIndex);
	int PutVertices(DataStream *stream, Map *map);
	int PutSpawns(DataStream *stream, Map *map);
	int PutActors(DataStream *stream, Map *map);
	int PutAnimations(DataStream *stream, Map *map);
	int PutEntrances(DataStream *stream, Map *map);
	int PutVariables(DataStream *stream, Map *map);
	int PutAmbients(DataStream *stream, Map *map);
	int PutMapnotes(DataStream *stream, Map *map);
	int PutExplored(DataStream *stream, Map *map);
	int PutTiles(DataStream *stream, Map *map);
};

#endif
