/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#include "PathFinder.h"
#include "Ambient.h"
#include "../../includes/strrefs.h"
#include "AmbientMgr.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "ScriptedAnimation.h"
#include "Projectile.h"
#include "ImageMgr.h"
#include "Video.h"
#include "ResourceMgr.h"
#include "SoundMgr.h"
#include "MusicMgr.h"
#include "Game.h"
#include "WorldMap.h"
#include "GameControl.h"
#include "Palette.h"

#include <cmath>
#include <cassert>

#ifndef WIN32
#include <sys/time.h>
#else
extern HANDLE hConsole;
#endif

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static int MAX_CIRCLESIZE = 8;
static int PersonalPerimeter;
static int MaxVisibility = 30;
static int VisibilityPerimeter; //calculated from MaxVisibility
static int NormalCost = 10;
static int AdditionalCost = 4;
static int Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 0, 1, 8, 0, 0, 0, 3, 1
};
static Point **VisibilityMasks=NULL;
static Point **PersonalSpaces=NULL;

static bool PathFinderInited = false;
static Variables Spawns;
static int LargeFog;
static ieWord globalActorCounter;

void ReleaseSpawnGroup(void *poi)
{
	delete (SpawnGroup *) poi;
}
void Map::ReleaseMemory()
{
	if (VisibilityMasks) {
		for (int i=0;i<MaxVisibility;i++) {
			free(VisibilityMasks[i]);
		}
		free(VisibilityMasks);
		VisibilityMasks=NULL;
	}

	if (PersonalSpaces) {
		for (int i=0;i<MAX_CIRCLESIZE;i++) {
			free(PersonalSpaces[i]);
		}
		free(PersonalSpaces);
		PersonalSpaces=NULL;
	}

	Spawns.RemoveAll(ReleaseSpawnGroup);
	PathFinderInited = false;
}

inline static AnimationObjectType SelectObject(Actor *actor, AreaAnimation *a, ScriptedAnimation *sca, Particles *spark, Projectile *pro)
{
	int actorh;
	if (actor) {
		actorh = actor->Pos.y;
	} else {
		actorh = 0x7fffffff;
	}

	int aah;
	if (a) {
		aah = a->Pos.y;//+a->height;
	} else {
		aah = 0x7fffffff;
	}

	int scah;
	if (sca) {
		scah = sca->YPos;//+sca->ZPos;
	} else {
		scah = 0x7fffffff;
	}

	int spah;
	if (spark) {
		//no idea if this should be plus or minus (or here at all)
		spah = spark->GetHeight();//+spark->pos.h;
	} else {
		spah = 0x7fffffff;
	}

	int proh;
	if (pro) {
		proh = pro->GetHeight();
	} else {
		proh = 0x7fffffff;
	}

	if (proh<actorh && proh<scah && proh<aah && proh<spah) return AOT_PROJECTILE;

	if (spah<actorh && spah<scah && spah<aah) return AOT_SPARK;

	if (aah<actorh && aah<scah) return AOT_AREA;

	if (scah<actorh) return AOT_SCRIPTED;

	return AOT_ACTOR;
}

//returns true if creature must be embedded in the area
inline static bool MustSave(Actor *actor)
{
	if (actor->InParty) {
		return false;
	}
	//check for familiars, summons?
	return true;
}

void InitSpawnGroups()
{
	ieResRef GroupName;
	int i;
	TableMgr * tab;

	int table=core->LoadTable( "spawngrp" );

	Spawns.RemoveAll(NULL);
	Spawns.SetType( GEM_VARIABLES_POINTER );

	if (table<0) {
		return;
	}
	tab = core->GetTable( table );
	if (!tab) {
		goto end;
	}
	i=tab->GetColNamesCount();
	while (i--) {
		int j=tab->GetRowCount();
		while (j--) {
			const char *crename = tab->QueryField( j,i );
			if (crename[0] != '*') break;
		}
		if (j>0) {
			SpawnGroup *creatures = new SpawnGroup(j);
			creatures->Level = (ieDword) atoi( tab->QueryField(i,0) );
			//difficulty
			for (;j;j--) {
				strnlwrcpy( creatures->ResRefs[j-1], tab->QueryField(j,i), 8 );
			}
			strnlwrcpy( GroupName, tab->GetColumnName( i ), 8 );
			Spawns.SetAt( GroupName, (void*) creatures );
		}
	}
end:
	core->DelTable( table );
}

void InitPathFinder()
{
	PathFinderInited = true;
	int passabletable = core->LoadTable( "pathfind" );
	if (passabletable >= 0) {
		TableMgr* tm = core->GetTable( passabletable );
		if (tm) {
			const char* poi;

			for (int i = 0; i < 16; i++) {
				poi = tm->QueryField( 0, i );
				if (*poi != '*')
					Passable[i] = atoi( poi );
			}
			poi = tm->QueryField( 1, 0 );
			if (*poi != '*')
				NormalCost = atoi( poi );
			poi = tm->QueryField( 1, 1 );
			if (*poi != '*')
				AdditionalCost = atoi( poi );
			core->DelTable( passabletable );
		}
	}
}

void AddLOS(int destx, int desty, int slot)
{
	for (int i=0;i<MaxVisibility;i++) {
		int x=(destx*i+MaxVisibility/2)/MaxVisibility*16;
		int y=(desty*i+MaxVisibility/2)/MaxVisibility*16;
		if (LargeFog) {
			x += 16;
			y += 16;
		}
		VisibilityMasks[i][slot].x=(short) x;
		VisibilityMasks[i][slot].y=(short) y;
	}
}

void AddSpace(int destx, int desty, int slot)
{
	for (int i=0;i<MAX_CIRCLESIZE;i++) {
		int x=(destx*i+MAX_CIRCLESIZE/2)/MAX_CIRCLESIZE*16;
		int y=(desty*i+MAX_CIRCLESIZE/2)/MAX_CIRCLESIZE*12;
		PersonalSpaces[i][slot].x=(short) x;
		PersonalSpaces[i][slot].y=(short) y;
	}
}

void InitExplore()
{
	LargeFog = !core->HasFeature(GF_SMALL_FOG);

	//circle perimeter size for MaxVisibility
	int x = MaxVisibility;
	int y = 0;
	int xc = 1 - ( 2 * MaxVisibility );
	int yc = 1;
	int re = 0;
	VisibilityPerimeter = 0;
	while (x>=y) {
		VisibilityPerimeter+=8;
		y++;
		re += yc;
		yc += 2;
		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	int i;
	VisibilityMasks = (Point **) malloc(MaxVisibility * sizeof(Point *) );
	for (i=0;i<MaxVisibility;i++) {
		VisibilityMasks[i] = (Point *) malloc(VisibilityPerimeter*sizeof(Point) );
	}

	x = MaxVisibility;
	y = 0;
	xc = 1 - ( 2 * MaxVisibility );
	yc = 1;
	re = 0;
	VisibilityPerimeter = 0;
	while (x>=y) {
		AddLOS (x, y, VisibilityPerimeter++);
		AddLOS (-x, y, VisibilityPerimeter++);
		AddLOS (-x, -y, VisibilityPerimeter++);
		AddLOS (x, -y, VisibilityPerimeter++);
		AddLOS (y, x, VisibilityPerimeter++);
		AddLOS (-y, x, VisibilityPerimeter++);
		AddLOS (-y, -x, VisibilityPerimeter++);
		AddLOS (y, -x, VisibilityPerimeter++);
		y++;
		re += yc;
		yc += 2;
		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	//circle perimeter size for MaxCirclesize
	x = MAX_CIRCLESIZE;
	y = 0;
	xc = 1 - ( 2 * MAX_CIRCLESIZE );
	yc = 1;
	re = 0;
	PersonalPerimeter = 0;
	while (x>=y) {
		PersonalPerimeter+=8;
		y++;
		re += yc;
		yc += 2;
		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	PersonalSpaces = (Point **) malloc(MAX_CIRCLESIZE * sizeof(Point *) );
	for(i=0;i<MAX_CIRCLESIZE;i++) {
		PersonalSpaces[i] = (Point *) malloc(PersonalPerimeter*sizeof(Point) );
	}
	x = MAX_CIRCLESIZE;
	y = 0;
	xc = 1 - ( 2 * MAX_CIRCLESIZE );
	yc = 1;
	re = 0;
	PersonalPerimeter = 0;
	while (x>=y) {
		AddSpace (x, y, PersonalPerimeter++);
		AddSpace (-x, y, PersonalPerimeter++);
		AddSpace (-x, -y, PersonalPerimeter++);
		AddSpace (x, -y, PersonalPerimeter++);
		AddSpace (y, x, PersonalPerimeter++);
		AddSpace (-y, x, PersonalPerimeter++);
		AddSpace (-y, -x, PersonalPerimeter++);
		AddSpace (y, -x, PersonalPerimeter++);
		y++;
		re += yc;
		yc += 2;
		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}
}

Map::Map(void)
	: Scriptable( ST_AREA )
{
	area=this;
	TMap = NULL;
	LightMap = NULL;
	SearchMap = NULL;
	HeightMap = NULL;
	SmallMap = NULL;
	MapSet = NULL;
	Walls = NULL;
	WallCount = 0;
	queue[PR_SCRIPT] = NULL;
	queue[PR_DISPLAY] = NULL;
	INISpawn = NULL;
	//no one needs this queue
	//queue[PR_IGNORE] = NULL;
	Qcount[PR_SCRIPT] = 0;
	Qcount[PR_DISPLAY] = 0;
	//no one needs this queue
	//Qcount[PR_IGNORE] = 0;
	lastActorCount[PR_SCRIPT] = 0;
	lastActorCount[PR_DISPLAY] = 0;
	//no one needs this
	//lastActorCount[PR_IGNORE] = 0;
	if (!PathFinderInited) {
		InitPathFinder();
		InitSpawnGroups();
		InitExplore();
		globalActorCounter = 0;
	}
	ExploredBitmap = NULL;
	VisibleBitmap = NULL;
	version = 0;
	localActorCounter = 0;
	MasterArea = core->GetGame()->MasterArea(scriptName);
}

Map::~Map(void)
{
	unsigned int i;

	if (MapSet) {
		free( MapSet );
	}
	if (TMap) {
		delete TMap;
	}
	if (INISpawn) {
		delete INISpawn;
	}
	aniIterator aniidx;
	for (aniidx = animations.begin(); aniidx != animations.end(); aniidx++) {
		delete (*aniidx);
	}

	for (i = 0; i < actors.size(); i++) {
		Actor* a = actors[i];
		//don't delete NPC/PC
		if (a && !a->Persistent() ) {
			delete a;
		}
	}

	for (i = 0; i < entrances.size(); i++) {
		delete( entrances[i] );
	}
	for (i = 0; i < spawns.size(); i++) {
		delete( spawns[i] );
	}
	if (LightMap)
		core->FreeInterface( LightMap );
	if (SearchMap)
		core->FreeInterface( SearchMap );
	if (HeightMap)
		core->FreeInterface( HeightMap );
	if (SmallMap)
		core->FreeInterface( SmallMap );
	for (i = 0; i < QUEUE_COUNT; i++) {
		if (queue[i]) {
			free(queue[i]);
			queue[i] = NULL;
		}
	}

	proIterator pri;

	for (pri = projectiles.begin(); pri != projectiles.end(); pri++) {
		delete (*pri);
	}

	scaIterator sci;

	for (sci = vvcCells.begin(); sci != vvcCells.end(); sci++) {
		delete (*sci);
	}

	spaIterator spi;

	for (spi = particles.begin(); spi != particles.end(); spi++) {
		delete (*spi);
	}

	for (i = 0; i < ambients.size(); i++) {
		delete ambients[i];
	}
	for (i = 0; i < mapnotes.size(); i++) {
		delete mapnotes[i];
	}

	//malloc-d in AREImp
	if (ExploredBitmap) {
		free( ExploredBitmap );
	}
	if (VisibleBitmap) {
		free( VisibleBitmap );
	}
	if (Walls) {
		for(i=0;i<WallCount;i++) {
			delete(Walls[i]);
		}
		free( Walls );
	}
	WallCount=0;
}

void Map::AddTileMap(TileMap* tm, ImageMgr* lm, ImageMgr* sr, ImageMgr* sm, ImageMgr* hm)
{
	TMap = tm;
	LightMap = lm;
	SearchMap = sr;
	HeightMap = hm;
	SmallMap = sm;
	Width=TMap->XCellCount * 4;
	Height=( TMap->YCellCount * 64 ) / 12;
	//Filling Matrices
	MapSet = (unsigned short *) malloc(sizeof(unsigned short) * Width * Height);
	//converting searchmap to internal format
	int y=SearchMap->GetHeight();
	while(y--) {
		int x=SearchMap->GetWidth();
		while(x--) {
			SearchMap->SetPixelIndex(x,y,Passable[SearchMap->GetPixelIndex(x,y)&PATH_MAP_AREAMASK]);
		}
	}
}

void Map::MoveToNewArea(const char *area, const char *entrance, int EveryOne, Actor *actor)
{
	char command[256];

	//change loader MOS image here
	//check worldmap entry, if that doesn't contain anything,
	//make a random pick

	if (EveryOne==CT_WHOLE) {
		core->GetGameControl()->AutoSave();
	}
	Game* game = core->GetGame();
	Map* map = game->GetMap(area, false);
	if (!map) {
		printMessage("Map", " ", LIGHT_RED);
		printf("Invalid map: %s\n",area);
		command[0]=0;
		return;
	}
	Entrance* ent = map->GetEntrance( entrance );
	int X,Y, face;
	if (!ent) {
		printMessage("Map", " ", YELLOW);
		printf( "WARNING!!! %s EntryPoint does not exist\n", entrance );
		X = map->TMap->XCellCount * 64;
		Y = map->TMap->YCellCount * 64;
		face = -1;
	} else {
		X = ent->Pos.x;
		Y = ent->Pos.y;
		face = ent->Face;
	}
	//LeaveArea is the same in ALL engine versions
	sprintf(command, "LeaveArea(\"%s\",[%d.%d],%d)", area, X, Y, face);

	if (EveryOne&CT_GO_CLOSER) {
		int i=game->GetPartySize(false);
		while (i--) {
			Actor *pc = game->GetPC(i,false);
			if (pc->GetCurrentArea()==this) {
				pc->ClearPath();
				pc->ClearActions();
				pc->AddAction( GenerateAction( command ) );
				pc->ProcessActions(true);
			}
		}
		return;
	}
	if (EveryOne&CT_SELECTED) {
		int i=game->GetPartySize(false);
		while (i--) {
			Actor *pc = game->GetPC(i,false);

			if (!pc->IsSelected()) {
				continue;
			}
			if (pc->GetCurrentArea()==this) {
				pc->ClearPath();
				pc->ClearActions();
				pc->AddAction( GenerateAction( command ) );
				pc->ProcessActions(true);
			}
		}
		return;
	}

	actor->ClearPath();
	actor->ClearActions();
	actor->AddAction( GenerateAction( command ) );
	actor->ProcessActions(true);
}

void Map::UseExit(Actor *actor, InfoPoint *ip)
{
	Game *game=core->GetGame();

	int EveryOne = ip->CheckTravel(actor);
	switch(EveryOne) {
	case CT_GO_CLOSER:
		core->DisplayConstantString(STR_WHOLEPARTY,0xffffff); //white
		if (game->EveryoneStopped()) {
			ip->Flags&=~TRAP_RESET; //exit triggered
		}
		return;
		//no ingame message for these events
	case CT_CANTMOVE: case CT_SELECTED:
		return;
	case CT_ACTIVE: case CT_WHOLE: case CT_MOVE_SELECTED:
		break;
	}

	ip->Flags&=~TRAP_RESET; //exit triggered
	if (ip->Destination[0] != 0) {
		MoveToNewArea(ip->Destination, ip->EntranceName, EveryOne, actor);
		return;
	}
	if (ip->Scripts[0]) {
		ip->LastTrigger = ip->LastEntered = actor->GetID();
		ip->ExecuteScript( 1 );
		ip->ProcessActions(true);
	}
}

void Map::UpdateScripts()
{
	// if masterarea, then we allow 'any' actors
	// if not masterarea, we allow only players
	if (!GetActorCount(MasterArea) ) {
		return;
	}

	//Run the Map Script
	ExecuteScript( 1 );

	//Execute Pending Actions
	//if it is only here, then the drawing will fail
	ProcessActions(false);

	// If scripts frozen, return.
	// This fixes starting a new IWD game. The above ProcessActions pauses the
	// game for a textscreen, but one of the actor->ProcessActions calls
	// below starts a cutscene, hiding the mouse. - wjp, 20060805
	if (core->GetGameControl()->GetDialogueFlags() & DF_FREEZE_SCRIPTS) return;

	//Run actor scripts (only for 0 priority)
	int q=Qcount[PR_SCRIPT];

	Game *game = core->GetGame();
	Actor *timestop_owner = game->timestop_owner;
	bool timestop = game->timestop_end>game->GameTime;

	while (q--) {
		Actor* actor = queue[PR_SCRIPT][q];
		//actor just moved away, don't run its script from this side
		if (actor->GetCurrentArea()!=this) {
			continue;
		}
		if (timestop && actor!=timestop_owner && actor->Modified[IE_DISABLETIMESTOP] ) {
			continue;
		}
		actor->ExecuteScript( MAX_SCRIPTS );
		actor->ProcessActions(false);

		actor->inventory.CalculateWeight();
		actor->SetBase( IE_ENCUMBRANCE, actor->inventory.GetWeight() );
		//TODO:calculate actor speed!
		int speed = (int) actor->GetStat(IE_MOVEMENTRATE);
		if (speed) {
			speed = 1500/speed;
		}

		if (actor->Modified[IE_DONOTJUMP]<2) {
			BlockSearchMap( actor->Pos, actor->size, 0);
			if (actor->GetNextStep()) {
				//we should actually wait for a short time and check then
				PathNode * step = actor->GetNextStep();
				if (!(GetBlocked(step->x,step->y)&PATH_MAP_PASSABLE)) {
					actor->NewPath();
				}
			}
		}
		if (!(actor->GetBase(IE_STATE_ID)&STATE_CANTMOVE) ) {
			if (!actor->Immobile()) {
				actor->DoStep( speed );
			}
			BlockSearchMap( actor->Pos, actor->size, actor->InParty?PATH_MAP_PC:PATH_MAP_NPC);
		}
	}

	//Check if we need to start some door scripts
	int doorCount = 0;
	while (true) {
		Door* door = TMap->GetDoor( doorCount++ );
		if (!door)
			break;
		if (!door->Scripts[0])
			continue;
		door->ExecuteScript( 1 );
		//Execute Pending Actions
		door->ProcessActions(false);
	}

	//Check if we need to start some trap scripts
	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		//If this InfoPoint has no script and it is not a Travel Trigger, skip it
		if (!ip->Scripts[0] && ( ip->Type != ST_TRAVEL )) {
			continue;
		}
		//it was turned off
		if (ip->Flags&TRAP_DEACTIVATED) {
			continue;
		}
		//If this InfoPoint is a Switch Trigger
		if (ip->Type == ST_TRIGGER) {
			//Check if this InfoPoint was activated
			if (ip->LastTrigger) {
				//Run the InfoPoint script
				ip->ExecuteScript( 1 );
				//Execute Pending Actions
				ip->ProcessActions(false);
			}
			continue;
		}

		q=Qcount[PR_SCRIPT];
		while (q--) {
			Actor* actor = queue[PR_SCRIPT][q];
			if (ip->Type == ST_PROXIMITY) {
				if(ip->Entered(actor)) {
					//if trap triggered, then mark actor
					actor->SetInTrap(ipCount);
				}
			} else {
				//ST_TRAVEL
				//don't move if doing something else
				if (actor->GetNextAction())
					continue;
				//this is needed, otherwise the travel
				//trigger would be activated anytime
				if (!(ip->Flags&TRAP_RESET))
					continue;
				if (ip->Entered(actor)) {
					UseExit(actor, ip);
				}
			}
		}

		if (ip->Type==ST_PROXIMITY) {
			ip->ExecuteScript( 1 );
			//Execute Pending Actions
			ip->ProcessActions(false);
		}
	}
}

void Map::DrawContainers( Region screen, Container *overContainer)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	unsigned int i = 0;
	Container *c;

	while ( (c = TMap->GetContainer(i++))!=NULL ) {
		Color tint = LightMap->GetPixel( c->Pos.x / 16, c->Pos.y / 12);
		tint.a = 255;

		if ((c != overContainer) && (c->Type==IE_CONTAINER_PILE) ) {
			if (c->outline->BBox.InsideRegion( vp )) {
				c->DrawPile(false, screen, tint);
			}
		}
	}
	//draw overcontainer with highlight
	if (overContainer) {
		if (overContainer->Type==IE_CONTAINER_PILE) {
			Color tint = LightMap->GetPixel( overContainer->Pos.x / 16, overContainer->Pos.y / 12);
			tint.a = 255;
			overContainer->DrawPile(true, screen, tint);
		} else {
			overContainer->DrawOutline();
		}
	}
}

Actor *Map::GetNextActor(int &q, int &index)
{
retry:
	switch(q) {
		case PR_SCRIPT:
			if (index--)
				return queue[q][index];
			q--;
			return NULL;
		case PR_DISPLAY:
			if (index--)
				return queue[q][index];
			q--;
			index = Qcount[q];
			goto retry;
		default:
			return NULL;
	}
}

AreaAnimation *Map::GetNextAreaAnimation(aniIterator &iter, ieDword gametime)
{
retry:
	if (iter==animations.end()) {
		return NULL;
	}
	AreaAnimation *a = *(iter++);
	if (!a->Schedule(gametime) ) {
		goto retry;
	}
	if (!IsVisible( a->Pos, !(a->Flags & A_ANI_NOT_IN_FOG)) ) {
		goto retry;
	}
	return a;
}

Particles *Map::GetNextSpark(spaIterator &iter)
{
	if (iter==particles.end()) {
		return NULL;
	}
	return *iter;
}

//doesn't increase iterator, because we might need to erase it from the list
Projectile *Map::GetNextProjectile(proIterator &iter)
{
	if (iter==projectiles.end()) {
		return NULL;
	}
	return *iter;
}

//doesn't increase iterator, because we might need to erase it from the list
ScriptedAnimation *Map::GetNextScriptedAnimation(scaIterator &iter)
{
	if (iter==vvcCells.end()) {
		return NULL;
	}
	return *iter;
}

static ieDword oldgametime = 0;

void Map::DrawMap(Region screen, GameControl* gc)
{
	if (!TMap) {
		return;
	}
	Game *game = core->GetGame();
	ieDword gametime = game->GameTime;

	if (INISpawn) {
		INISpawn->CheckSpawn();
	}

	int rain;
	if (HasWeather()) {
		//zero when the weather particles are all gone
		rain = game->weather->GetPhase()-P_EMPTY;
	} else {
		rain = 0;
	}
	TMap->DrawOverlays( screen, rain );

	//Blit the Background Map Animations (before actors)
	Video* video = core->GetVideoDriver();

	//Draw Selected Door Outline
	if (gc->overDoor) {
		gc->overDoor->DrawOutline();
	}
	DrawContainers( screen, gc->overContainer );
	Region vp = video->GetViewport();
	//if it is only here, then the scripting will fail?
	GenerateQueues();
	SortQueues();
	//drawing queues 1 and 0
	//starting with lower priority
	//so displayed, but inactive actors (dead) will be drawn over
	int q = PR_DISPLAY;
	int index = Qcount[q];
	Actor* actor = GetNextActor(q, index);
	aniIterator aniidx = animations.begin();
	scaIterator scaidx = vvcCells.begin();
	proIterator proidx = projectiles.begin();
	spaIterator spaidx = particles.begin();

	AreaAnimation *a = GetNextAreaAnimation(aniidx, gametime);
	ScriptedAnimation *sca = GetNextScriptedAnimation(scaidx);
	Projectile *pro = GetNextProjectile(proidx);
	Particles *spark = GetNextSpark(spaidx);

	while (actor || a || sca || spark || pro) {
		switch(SelectObject(actor,a,sca,spark,pro)) {
		case AOT_ACTOR:
			actor->Draw( screen );
			actor = GetNextActor(q, index);
			break;
		case AOT_AREA:
			//draw animation
			{
				int animcount=a->animcount;

				//always draw the animation tinted because tint is also used for
				//transparency
				Color tint = {255,255,255,255-(ieByte) a->transparency};
				if ((a->Flags&A_ANI_NO_SHADOW)) {
					tint = LightMap->GetPixel( a->Pos.x / 16, a->Pos.y / 12);
					tint.a = 255-(ieByte) a->transparency;
				}
				while (animcount--) {
					Animation *anim = a->animation[animcount];
					video->BlitGameSprite( anim->NextFrame(),
						a->Pos.x + screen.x, a->Pos.y + screen.y,
						BLIT_TINTED, tint, 0, a->palette, &screen );
				}
			}
			a = GetNextAreaAnimation(aniidx,gametime);
			break;
		case AOT_SCRIPTED:
			{
				Point Pos(0,0);

				Color tint = LightMap->GetPixel( sca->XPos / 16, sca->YPos / 12);
				tint.a = 255;
				bool endReached = sca->Draw(screen, Pos, tint, this, 0, -1);
				if (endReached) {
					delete( sca );
					scaidx=vvcCells.erase(scaidx);
				} else {
					scaidx++;
				}
			}
			sca = GetNextScriptedAnimation(scaidx);
			break;
		case AOT_PROJECTILE:
			{
				int drawn;
				if (gametime>oldgametime) {
					drawn = pro->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					pro->Draw( screen );
					proidx++;
				} else {
					delete( pro );
					proidx = projectiles.erase(proidx);
				}
			}
			pro = GetNextProjectile(proidx);
			break;
		case AOT_SPARK:
			{
				int drawn;
				if (gametime>oldgametime) {
					drawn = spark->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					spark->Draw( screen );
					spaidx++;
				} else {
					delete( spark );
					spaidx=particles.erase(spaidx);
				}
			}
			spark = GetNextSpark(spaidx);
			break;
		default:
			abort();
		}
	}

	if ((core->FogOfWar&FOG_DRAWSEARCHMAP) && SearchMap) {
		DrawSearchMap(screen);
	} else {
		if ((core->FogOfWar&FOG_DRAWFOG) && TMap) {
			TMap->DrawFogOfWar( ExploredBitmap, VisibleBitmap, screen );
		}
	}

	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		ip->DrawOverheadText(screen);
	}
	oldgametime=gametime;
}

void Map::DrawSearchMap(Region &screen)
{
	Color inaccessible = { 128, 128, 128, 128 };
	Video *vid=core->GetVideoDriver();
	Region rgn=vid->GetViewport();
	Region block;

	block.w=16;
	block.h=12;
	int w = screen.w/16;
	int h = screen.h/12;

	for(int x=0;x<w;x++) {
		for(int y=0;y<h;y++) {
			if (!(GetBlocked( x+rgn.x/16, y+rgn.y/12) & PATH_MAP_PASSABLE) ) {
				block.x=screen.x+x*16;
				block.y=screen.y+y*12;
				vid->DrawRect(block,inaccessible);
			}
		}
	}
}

//adding animation in order, based on its height parameter
void Map::AddAnimation(AreaAnimation* anim)
{
	//this hack is to make sure animations flagged with background
	//are always drawn first (-9999 seems sufficiently small)
	if (anim->Flags&A_ANI_BACKGROUND) {
		anim->height=-9999;
	}

	aniIterator iter;
	for(iter=animations.begin(); (iter!=animations.end()) && ((*iter)->height<anim->height); iter++);
	animations.insert(iter, anim);
}

//reapplying all of the effects on the actors of this map
//this might be unnecessary later
void Map::UpdateEffects()
{
	size_t i = actors.size();
	while (i--) {
		actors[i]->RefreshEffects(NULL);
	}
}

void Map::Shout(Actor* actor, int shoutID, unsigned int radius)
{
	size_t i=actors.size();
	while (i--) {
		Actor *listener = actors[i];

		if (radius) {
			if (Distance(actor->Pos, listener->Pos)>radius) {
				continue;
			}
		}
		if (shoutID) {
			listener->LastHeard = actor->GetID();
			listener->LastShout = shoutID;
		} else {
			listener->LastHelp = actor->GetID();
		}
	}
}

bool Map::AnyEnemyNearPoint(Point &p)
{
	ieDword gametime = core->GetGame()->GameTime;
	size_t i = actors.size();
	while (i--) {
		Actor *actor = actors[i];

		if (actor->Schedule(gametime) ) {
			continue;
		}
		if (Distance(actor->Pos, p) > 400) {
			continue;
		}
		if (actor->GetStat(IE_EA)<EA_EVILCUTOFF) {
			continue;
		}
		return true;
	}
	return false;
}

void Map::AddActor(Actor* actor)
{
	//setting the current area for the actor as this one
	strnlwrcpy(actor->Area, scriptName, 8);
	//if actor->globalID was already set, don't change it
	actor->SetMap(this, ++localActorCounter,
		actor->globalID?actor->globalID:++globalActorCounter);
	actors.push_back( actor );
	//if a visible aggressive actor was put on the map, it is an autopause reason
	//guess game is always loaded? if not, then we'll crash
	ieDword gametime = core->GetGame()->GameTime;

	if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
		if (IsVisible(actor->Pos, false) && actor->Schedule(gametime) ) {
			core->Autopause(AP_ENEMY);
		}
	}
}

bool Map::AnyPCSeesEnemy()
{
	ieDword gametime = core->GetGame()->GameTime;
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
			if (IsVisible(actor->Pos, false) && actor->Schedule(gametime) ) {
				return true;
			}
		}
	}
	return false;
}

void Map::DeleteActor(int i)
{
	Actor *actor = actors[i];
	Game *game = core->GetGame();
	game->LeaveParty( actor );
	game->DelNPC( game->InStore(actor) );
	actors.erase( actors.begin()+i );
	delete (actor);
}

Actor* Map::GetActorByGlobalID(ieDword objectID)
{
	if (!objectID) {
		return NULL;
	}
	//truncation is intentional
	ieWord globalID = (ieWord) objectID;
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];

		if (actor->globalID==globalID) {
			return actor;
		}
	}
	return NULL;
}

/** flags:
 GA_SELECT    16  - unselectable actors don't play
 GA_NO_DEAD   32  - dead actors don't play
 GA_POINT     64  - not actor specific
 GA_NO_HIDDEN 128 - hidden actors don't play
*/
Actor* Map::GetActor(Point &p, int flags)
{
	ieDword gametime = core->GetGame()->GameTime;
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];

		if (!actor->IsOver( p ))
			continue;
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		if (!actor->Schedule(gametime) ) {
			continue;
		}
		return actor;
	}
	return NULL;
}

Actor* Map::GetActorInRadius(Point &p, int flags, unsigned int radius)
{
	ieDword gametime = core->GetGame()->GameTime;
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];

		if (PersonalDistance( p, actor ) > radius)
			continue;
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		if (!actor->Schedule(gametime) ) {
			continue;
		}
		return actor;
	}
	return NULL;
}

Actor **Map::GetAllActorsInRadius(Point &p, int flags, unsigned int radius)
{
	ieDword count = 1;
	size_t i;

	ieDword gametime = core->GetGame()->GameTime;
	i = actors.size();
	while (i--) {
		Actor* actor = actors[i];

		if (PersonalDistance( p, actor ) > radius)
			continue;
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		if (!actor->Schedule(gametime) ) {
			continue;
		}
		count++;
	}

	Actor **ret = (Actor **) malloc( sizeof(Actor*) * count);
	i = actors.size();
	int j = 0;
	while (i--) {
		Actor* actor = actors[i];

		if (PersonalDistance( p, actor ) > radius)
			continue;
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		if (!actor->Schedule(gametime) ) {
			continue;
		}
		ret[j++]=actor;
	}

	ret[j]=NULL;
	return ret;
}


Actor* Map::GetActor(const char* Name, int flags)
{
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (strnicmp( actor->GetScriptName(), Name, 32 ) == 0) {
			if (!actor->ValidTarget(flags) ) {
				return NULL;
			}
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorCount(bool any) const
{
	if (any) {
		return (int) actors.size();
	}
	int ret = 0;
	size_t i=actors.size();
	while (i--) {
		if (MustSave(actors[i])) {
			ret++;
		}
	}
	return ret;
}

void Map::JumpActors(bool jump)
{
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (actor->Modified[IE_DONOTJUMP]&DNJ_JUMP) {
			if (jump) {
				actor->FixPosition();
			}
			actor->SetBase(IE_DONOTJUMP,0);
		}
	}
}

//before writing the area out, perform some cleanups
void Map::PurgeArea(bool items)
{
	InternalFlags |= IF_JUSTDIED; //area marked for swapping out

	//1. remove dead actors without 'keep corpse' flag
	int i=(int) actors.size();
	while (i--) {
		Actor *ac = actors[i];

		if (ac->Modified[IE_STATE_ID]&STATE_NOSAVE) {
			if (ac->Modified[IE_MC_FLAGS] & MC_KEEP_CORPSE) {
				continue;
			}
			delete ac;
			actors.erase( actors.begin()+i );
		}
	}
	//2. remove any non critical items
	if (items) {
		i=(int) TMap->GetContainerCount();
		while (i--) {
			Container *c = TMap->GetContainer(i);
			unsigned int j=c->inventory.GetSlotCount();
			while (j--) {
				CREItem *itemslot = c->inventory.GetSlotItem(j);
				if (itemslot->Flags&IE_INV_ITEM_CRITICAL) {
					continue;
				}
			}
			TMap->CleanupContainer(c);
		}
	}
}

Actor* Map::GetActor(int index, bool any)
{
	if (any) {
		return actors[index];
	}
	unsigned int i=0;
	while (i<actors.size() ) {
		Actor *ac = actors[i++];
		if (MustSave(ac) ) {
			if (!index--) {
				return ac;
			}
		}
	}
	return NULL;
}

Actor* Map::GetActorByDialog(const char *resref)
{
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (strnicmp( actor->GetDialog(false), resref, 8 ) == 0) {
			return actor;
		}
	}
	return NULL;
}

//this function finds an actor by its original resref (not correct yet)
Actor* Map::GetActorByResource(const char *resref)
{
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (strnicmp( actor->GetScriptName(), resref, 8 ) == 0) { //temporarily!
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorInRect(Actor**& actorlist, Region& rgn, bool onlyparty)
{
	actorlist = ( Actor * * ) malloc( actors.size() * sizeof( Actor * ) );
	int count = 0;
	size_t i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
//use this function only for party?
		if (onlyparty && !actor->InParty)
			continue;
		if (!actor->ValidTarget(GA_SELECT|GA_NO_DEAD) )
			continue;
		if ((actor->Pos.x<rgn.x) || (actor->Pos.y<rgn.y))
			continue;
		if ((actor->Pos.x>rgn.x+rgn.w) || (actor->Pos.y>rgn.y+rgn.h) )
			continue;
		actorlist[count++] = actor;
	}
	actorlist = ( Actor * * ) realloc( actorlist, count * sizeof( Actor * ) );
	return count;
}

void Map::PlayAreaSong(int SongType)
{
	//you can speed this up by loading the songlist once at startup
	int column;
	const char* tablename;

	if (core->HasFeature( GF_HAS_SONGLIST )) {
		column = 1;
		tablename = "songlist";
	} else {
		/*since bg1 and pst has no .2da for songlist,
		we must supply one in the gemrb/override folder.
		It should be: music.2da, first column is a .mus filename
		*/
		column = 0;
		tablename = "music";
	}
	int songlist = core->LoadTable( tablename );
	if (songlist < 0) {
		return;
	}
	TableMgr* tm = core->GetTable( songlist );
	if (!tm) {
		core->DelTable( songlist );
		return;
	}
	const char* poi = tm->QueryField( SongHeader.SongList[SongType], column );
	core->GetMusicMgr()->SwitchPlayList( poi, true );
}

int Map::GetBlocked(unsigned int x, unsigned int y)
{
	int ret = SearchMap->GetPixelIndex( x, y );
	if (ret&(PATH_MAP_DOOR_TRANSPARENT|PATH_MAP_ACTOR)) {
//  if (ret&PATH_MAP_DOOR_TRANSPARENT) {
		ret&=~PATH_MAP_PASSABLE;
	}
	if (ret&PATH_MAP_DOOR_OPAQUE) {
		ret=PATH_MAP_NO_SEE;
	}
	return ret;
}

int Map::GetBlocked(Point &c)
{
	return GetBlocked(c.x/16, c.y/12);
}

// flags: 0 - never dither (full cover)
//        1 - dither if polygon wants it
//        2 - always dither

SpriteCover* Map::BuildSpriteCover(int x, int y, int xpos, int ypos,
	unsigned int width, unsigned int height, int flags)
{
	SpriteCover* sc = new SpriteCover;
	sc->worldx = x;
	sc->worldy = y;
	sc->XPos = xpos;
	sc->YPos = ypos;
	sc->Width = width;
	sc->Height = height;

	Video* video = core->GetVideoDriver();
	video->InitSpriteCover(sc, flags);

	unsigned int wpcount = GetWallCount();
	unsigned int i;

	for (i = 0; i < wpcount; ++i)
	{
		Wall_Polygon* wp = GetWallGroup(i);
		if (!wp) continue;
		if (!wp->PointCovered(x, y)) continue;

		video->AddPolygonToSpriteCover(sc, wp);
	}

	return sc;
}

void Map::ActivateWallgroups(unsigned int baseindex, unsigned int count, int flg)
{
	unsigned int i;

	if (!Walls) {
		return;
	}
	for(i=baseindex; i < baseindex+count; ++i) {
		Wall_Polygon* wp = GetWallGroup(i);
		if (!wp)
			continue;
		ieDword value=wp->GetPolygonFlag();
		if (flg)
			value&=~WF_DISABLED;
		else
			value|=WF_DISABLED;
		wp->SetPolygonFlag(value);
	}
	//all actors will have to generate a new spritecover
	i=(int) actors.size();
	while(i--) {
		actors[i]->SetSpriteCover(NULL);
	}
}


//this function determines actor drawing order
//it should be extended to wallgroups, animations, effects!
void Map::GenerateQueues()
{
	int priority;

	unsigned int i=(unsigned int) actors.size();
	for (priority=0;priority<QUEUE_COUNT;priority++) {
		if (lastActorCount[priority] != i) {
			if (queue[priority]) {
				free(queue[priority]);
				queue[priority] = NULL;
			}
			queue[priority] = (Actor **) calloc( i, sizeof(Actor *) );
			lastActorCount[priority] = i;
		}
		Qcount[priority] = 0;
	}

	ieDword gametime = core->GetGame()->GameTime;
	while (i--) {
		Actor* actor = actors[i];

		if (actor->CheckOnDeath()) {
			DeleteActor( i );
			continue;
		}

		ieDword stance = actor->GetStance();
		ieDword internalFlag = actor->GetInternalFlag();

		if (internalFlag&IF_ACTIVE) {
			if ((stance == IE_ANI_TWITCH) && (internalFlag&IF_IDLE) ) {
				priority = PR_DISPLAY; //display
			} else {
				priority = PR_SCRIPT; //run scripts and display
			}
		} else {
			//dead actors are always visible on the map, but run no scripts
			if (stance == IE_ANI_TWITCH || stance == IE_ANI_DIE) {
				priority = PR_DISPLAY;
			} else {
				//isvisible flag is false (visibilitymap) here,
				//coz we want to reactivate creatures that
				//just became visible
				if (IsVisible(actor->Pos, false) && actor->Schedule(gametime) ) {
					priority = PR_SCRIPT; //run scripts and display, activated now
					actor->Unhide();
					if (!(actor->GetInternalFlag()&IF_STOPATTACK)) {
						if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
							core->Autopause(AP_ENEMY);
						}
					}
				} else {
					priority = PR_IGNORE;
				}
			}
		}

		//we ignore priority 2
		if (priority>=PR_IGNORE) continue;

		queue[priority][Qcount[priority]] = actor;
		Qcount[priority]++;
	}
}

//the original qsort implementation was flawed
void Map::SortQueues()
{
	for (int q=0;q<QUEUE_COUNT;q++) {
		Actor **baseline=queue[q];
		int n = Qcount[q];
		int i = n/2;
		int parent, child;
		Actor * tmp;

		for (;;) {
			if (i>0) {
				i--;
				tmp = baseline[i];
			} else {
				n--;
				if (n<=0) break; //breaking loop
				tmp = baseline[n];
				baseline[n] = baseline[0];
			}
			parent = i;
			child = i*2+1;
			while(child<n) {
				int chp = child+1;
				if (chp<n && baseline[chp]->Pos.y < baseline[child]->Pos.y) {
					child=chp;
				}
				if (baseline[child]->Pos.y<tmp->Pos.y) {
					baseline[parent] = baseline[child];
					parent = child;
					child = parent*2+1;
				} else
					break;
			}
			baseline[parent]=tmp;
		}
	}
}

void Map::AddProjectile(Projectile* pro, Point &source, ieWord actorID)
{
	proIterator iter;

	pro->MoveTo(this,source);
	pro->SetTarget(actorID);
	int height = pro->GetHeight();
	for(iter=projectiles.begin();iter!=projectiles.end() && (*iter)->GetHeight()<height; iter++);
	projectiles.insert(iter, pro);
}

//adding projectile in order, based on its height parameter
void Map::AddProjectile(Projectile* pro, Point &source, Point &dest)
{
	proIterator iter;

	pro->MoveTo(this,source);
	pro->SetTarget(dest);
	int height = pro->GetHeight();
	for(iter=projectiles.begin();iter!=projectiles.end() && (*iter)->GetHeight()<height; iter++);
	projectiles.insert(iter, pro);
}

//adding videocell in order, based on its height parameter
void Map::AddVVCell(ScriptedAnimation* vvc)
{
	scaIterator iter;

	for(iter=vvcCells.begin();iter!=vvcCells.end() && (*iter)->ZPos<vvc->ZPos; iter++);
	vvcCells.insert(iter, vvc);
}

AreaAnimation* Map::GetAnimation(const char* Name)
{
	aniIterator iter;

	for(iter=animations.begin();iter!=animations.end();iter++) {
		AreaAnimation *anim = *iter;

		if (anim->Name && (strnicmp( anim->Name, Name, 32 ) == 0)) {
			return anim;
		}
	}
	return NULL;
}

Spawn *Map::AddSpawn(char* Name, int XPos, int YPos, ieResRef *creatures, unsigned int count)
{
	Spawn* sp = new Spawn();
	strnspccpy(sp->Name, Name, 32);
	if (count>MAX_RESCOUNT) {
		count=MAX_RESCOUNT;
	}
	sp->Pos.x = (ieWord) XPos;
	sp->Pos.y = (ieWord) YPos;
	sp->Count = count;
	sp->Creatures = (ieResRef *) calloc( count, sizeof(ieResRef) );
	for( unsigned int i=0;i<count;i++) {
		strnlwrcpy(sp->Creatures[i],creatures[i],8);
	}
	spawns.push_back( sp );
	return sp;
}

void Map::AddEntrance(char* Name, int XPos, int YPos, short Face)
{
	Entrance* ent = new Entrance();
	strncpy( ent->Name, Name, 32 );
	ent->Pos.x = (ieWord) XPos;
	ent->Pos.y = (ieWord) YPos;
	ent->Face = (ieWord) Face;
	entrances.push_back( ent );
}

Entrance* Map::GetEntrance(const char* Name)
{
	size_t i=entrances.size();
	while (i--) {
		Entrance *e = entrances[i];

		if (stricmp( e->Name, Name ) == 0) {
			return e;
		}
	}
	return NULL;
}

bool Map::HasActor(Actor *actor)
{
	size_t i=actors.size();
	while (i--) {
		if (actors[i] == actor) {
			return true;
		}
	}
	return false;
}

void Map::RemoveActor(Actor* actor)
{
	size_t i=actors.size();
	while (i--) {
		if (actors[i] == actor) {
			BlockSearchMap(actor->Pos, actor->size,0);
			actors.erase( actors.begin()+i );
			return;
		}
	}
}

//returns true if none of the partymembers are on the map
bool Map::CanFree()
{
	size_t i=actors.size();
	while (i--) {
		if (actors[i]->InParty) {
			return false;
		}
	}
	//we expect the area to be swapped out, so we simply remove the corpses now
	PurgeArea(false);
	return true;
}

void Map::DebugDump()
{
	printf( "DebugDump of Area %s:\n", scriptName );
}

/******************************************************************************/

void Map::Leveldown(unsigned int px, unsigned int py,
	unsigned int& level, Point &n, unsigned int& diff)
{
	int pos;
	unsigned int nlevel;

	if (( px >= Width ) || ( py >= Height )) {
		return;
	} //walked off the map
	pos = py * Width + px;
	nlevel = MapSet[pos];
	if (!nlevel) {
		return;
	} //not even considered
	if (level <= nlevel) {
		return;
	}
	unsigned int ndiff = level - nlevel;
	if (ndiff > diff) {
		level = nlevel;
		diff = ndiff;
		n.x = (ieWord) px;
		n.y = (ieWord) py;
	}
}

void Map::SetupNode(unsigned int x, unsigned int y, unsigned int Cost)
{
	unsigned int pos;

	if (( x >= Width ) || ( y >= Height )) {
		return;
	}
	pos = y * Width + x;
	if (MapSet[pos]) {
		return;
	}
	if (!( GetBlocked(x,y) & PATH_MAP_PASSABLE)) {
		MapSet[pos] = 65535;
		return;
	}
	MapSet[pos] = (ieWord) Cost;
	InternalStack.push( ( x << 16 ) | y );
}

bool Map::AdjustPositionX(Point &goal, unsigned int radius)
{
	unsigned int minx = 0;
	if ((unsigned int) goal.x > radius)
		minx = goal.x - radius;
	unsigned int maxx = goal.x + radius + 1;
	if (maxx > Width)
		maxx = Width;

	for (unsigned int scanx = minx; scanx < maxx; scanx++) {
		if ((unsigned int) goal.y >= radius) {
			if (GetBlocked( scanx, goal.y - radius ) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) scanx;
				goal.y = (ieWord) (goal.y - radius);
				return true;
			}
		}
		if (goal.y + radius < Height) {
			if (GetBlocked( scanx, goal.y + radius ) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) scanx;
				goal.y = (ieWord) (goal.y + radius);
				return true;
			}
		}
	}
	return false;
}

bool Map::AdjustPositionY(Point &goal, unsigned int radius)
{
	unsigned int miny = 0;
	if ((unsigned int) goal.y > radius)
		miny = goal.y - radius;
	unsigned int maxy = goal.y + radius + 1;
	if (maxy > Height)
		maxy = Height;
	for (unsigned int scany = miny; scany < maxy; scany++) {
		if ((unsigned int) goal.x >= radius) {
			if (GetBlocked( goal.x - radius, scany ) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) (goal.x - radius);
				goal.y = (ieWord) scany;
				return true;
			}
		}
		if (goal.x + radius < Width) {
			if (GetBlocked( goal.x - radius, scany ) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) (goal.x + radius);
				goal.y = (ieWord) scany;
				return true;
			}
		}
	}
	return false;
}

void Map::AdjustPosition(Point &goal, unsigned int radius)
{
	unsigned int maxr = Width;
	if (maxr < Height) {
		maxr = Height;
	}
	if ((unsigned int) goal.x > Width) {
		goal.x = (ieWord) Width;
	}
	if ((unsigned int) goal.y > Height) {
		goal.y = (ieWord) Height;
	}

	for (; radius < maxr; radius++) {
		//lets make it slightly random where the actor will appear
		if (rand()&1) {
			if (AdjustPositionX(goal, radius)) {
				return;
			}
			if (AdjustPositionY(goal, radius)) {
				return;
			}
		} else {
			if (AdjustPositionY(goal, radius)) {
				return;
			}
			if (AdjustPositionX(goal, radius)) {
				return;
			}
		}
	}
}

//run away from dX, dY (ie.: find the best path of limited length that brings us the farthest from dX, dY)
PathNode* Map::RunAway(Point &s, Point &d, unsigned int PathLen, int flags)
{
	Point start(s.x/16, s.y/12);
	Point goal (d.x/16, d.y/12);
	unsigned int dist;

	//MapSet entries are made of 16 bits
	if (PathLen>65535) {
		PathLen = 65535;
	}

	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( GetBlocked( start.x, start.y) & PATH_MAP_PASSABLE )) {
		AdjustPosition( start );
	}
	unsigned int pos = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[start.y * Width + start.x] = 1;
	dist = 0;
	Point best = start;
	while (InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;
		long tx = ( x - goal.x );
		long ty = ( y - goal.y );
		unsigned int distance = (unsigned int) sqrt( ( double ) ( tx* tx + ty* ty ) );
		if (dist<distance) {
			best.x=(ieWord) x;
			best.y=(ieWord) y;
			dist=distance;
		}

		unsigned int Cost = MapSet[y * Width + x] + NormalCost;
		if (Cost > PathLen) {
			//printf("Path not found!\n");
			break;
		}
		SetupNode( x - 1, y - 1, Cost );
		SetupNode( x + 1, y - 1, Cost );
		SetupNode( x + 1, y + 1, Cost );
		SetupNode( x - 1, y + 1, Cost );

		Cost += AdditionalCost;
		SetupNode( x, y - 1, Cost );
		SetupNode( x + 1, y, Cost );
		SetupNode( x, y + 1, Cost );
		SetupNode( x - 1, y, Cost );
	}

	//find path backwards from best to start
	PathNode* StartNode = new PathNode;
	PathNode* Return = StartNode;
	StartNode->Next = NULL;
	StartNode->x = best.x;
	StartNode->y = best.y;
	if (flags) {
		StartNode->orient = GetOrient( start, best );
	} else {
		StartNode->orient = GetOrient( best, start );
	}
	Point p = best;
	unsigned int pos2 = start.y * Width + start.x;
	while (( pos = p.y * Width + p.x ) != pos2) {
		Return = new PathNode;
		StartNode->Parent = Return;
		Return->Next = StartNode;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		Point n;
		Leveldown( p.x, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y, level, n, diff );
		Leveldown( p.x - 1, p.y, level, n, diff );
		Leveldown( p.x, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y - 1, level, n, diff );
		Return->x = n.x;
		Return->y = n.y;

		if (flags) {
			Return->orient = GetOrient( p, n );
		} else {
			Return->orient = GetOrient( n, p );
		}
		p = n;
		if (!diff) {
			break;
		}
	}
	Return->Parent = NULL;
	return Return;
}

bool Map::TargetUnreachable(Point &s, Point &d)
{
	Point start( s.x/16, s.y/12 );
	Point goal ( d.x/16, d.y/12 );
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( GetBlocked( goal.x, goal.y ) & PATH_MAP_PASSABLE )) {
		return true;
	}
	if (!( GetBlocked( start.x, start.y ) & PATH_MAP_PASSABLE )) {
		return true;
	}

	unsigned int pos = ( goal.x << 16 ) | goal.y;
	unsigned int pos2 = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[goal.y * Width + goal.x] = 1;

	while (InternalStack.size() && pos!=pos2) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;

		SetupNode( x - 1, y - 1, 1 );
		SetupNode( x + 1, y - 1, 1 );
		SetupNode( x + 1, y + 1, 1 );
		SetupNode( x - 1, y + 1, 1 );
		SetupNode( x, y - 1, 1 );
		SetupNode( x + 1, y, 1 );
		SetupNode( x, y + 1, 1 );
		SetupNode( x - 1, y, 1 );
	}
	return pos!=pos2;
}

/* Use this function when you target something by a straight line projectile (like a lightning bolt, arrow, etc)
*/

PathNode* Map::GetLine(Point &start, Point &dest, int flags)
{
	int Orientation = GetOrient(start, dest);
	return GetLine(start, dest, 1, Orientation, flags);
}

PathNode* Map::GetLine(Point &start, int Steps, int Orientation, int flags)
{
	Point dest=start;
	int count = Steps;

	while(count) {
		int st = Steps>MaxVisibility?MaxVisibility:Steps;
		int p = VisibilityPerimeter*Orientation/MAX_ORIENT;
		dest.x += VisibilityMasks[Steps][p].x;
		dest.y += VisibilityMasks[Steps][p].y;
		count-=st;
	}
	//FIXME: calculate dest based on distance and orientation
	return GetLine(start, dest, Steps, Orientation, flags);
}

PathNode* Map::GetLine(Point &start, Point &dest, int Speed, int Orientation, int flags)
{
	PathNode* StartNode = new PathNode;
	PathNode *Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x = start.x;
	StartNode->y = start.y;
	StartNode->orient = Orientation;

	int Count = 0;
	int Max = Distance(start,dest);
	for (int Steps = 0; Steps<Max; Steps++) {
		if (!Count) {
			StartNode->Next = new PathNode;
			StartNode->Next->Parent = StartNode;
			StartNode = StartNode->Next;
			StartNode->Next = NULL;
			Count=Speed;
		} else {
			Count--;
		}

		Point p;
		p.x = (ieWord) start.x + ((dest.x - start.x) * Steps / Max);
		p.y = (ieWord) start.y + ((dest.y - start.y) * Steps / Max);
		StartNode->x = p.x;
		StartNode->y = p.y;
		StartNode->orient = Orientation;
		bool wall = !( GetBlocked( p ) & PATH_MAP_PASSABLE );
		if (wall) switch (flags) {
			case GL_REBOUND:
				Orientation = (Orientation + 8) &15;
				//recalculate dest (mirror it)
				//
				break;
			case GL_PASS:
				break;
			default: //premature end
				return Return;
		}
	}

	return Return;
}

PathNode* Map::FindPath(const Point &s, const Point &d, int MinDistance)
{
	Point start( s.x/16, s.y/12 );
	Point goal ( d.x/16, d.y/12 );
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( GetBlocked( goal.x, goal.y ) & PATH_MAP_PASSABLE )) {
		AdjustPosition( goal );
	}
	unsigned int pos = ( goal.x << 16 ) | goal.y;
	unsigned int pos2 = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[goal.y * Width + goal.x] = 1;

	while (InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;

		if (pos == pos2) {
			//We've found _a_ path
			//printf("GOAL!!!\n");
			break;
		}
		unsigned int Cost = MapSet[y * Width + x] + NormalCost;
		if (Cost > 65500) {
			//printf("Path not found!\n");
			break;
		}
		SetupNode( x - 1, y - 1, Cost );
		SetupNode( x + 1, y - 1, Cost );
		SetupNode( x + 1, y + 1, Cost );
		SetupNode( x - 1, y + 1, Cost );

		Cost += AdditionalCost;
		SetupNode( x, y - 1, Cost );
		SetupNode( x + 1, y, Cost );
		SetupNode( x, y + 1, Cost );
		SetupNode( x - 1, y, Cost );
	}

	//find path from start to goal
	PathNode* StartNode = new PathNode;
	PathNode* Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x = start.x;
	StartNode->y = start.y;
	StartNode->orient = GetOrient( goal, start );
	if (pos != pos2) {
		return Return;
	}
	Point p = start;
	pos2 = goal.y * Width + goal.x;
	while (( pos = p.y * Width + p.x ) != pos2) {
		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode = StartNode->Next;
		StartNode->Next = NULL;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		Point n;
		Leveldown( p.x, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y, level, n, diff );
		Leveldown( p.x - 1, p.y, level, n, diff );
		Leveldown( p.x, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y - 1, level, n, diff );
		if (!diff)
			return Return;
		StartNode->x = n.x;
		StartNode->y = n.y;
		StartNode->orient = GetOrient( n, p );
		p = n;
	}
	//stepping back on the calculated path
	if (MinDistance) {
		while (StartNode->Parent) {
			Point tar;

			tar.x=StartNode->Parent->x*16;
			tar.y=StartNode->Parent->y*12;
			int dist = Distance(tar,d);
			if (dist+14>=MinDistance) {
				break;
			}
			StartNode = StartNode->Parent;
			delete StartNode->Next;
			StartNode->Next = NULL;
		}
	}
	return Return;
}

//single point visible or not (visible/exploredbitmap)
//if explored = true then explored otherwise currently visible
bool Map::IsVisible(Point &pos, int explored)
{
	if (!VisibleBitmap)
		return false;
	int sX=pos.x/32;
	int sY=pos.y/32;
	if (sX<0) return false;
	if (sY<0) return false;
	int w = TMap->XCellCount * 2 + LargeFog;
	int h = TMap->YCellCount * 2 + LargeFog;
	if (sX>=w) return false;
	if (sY>=h) return false;
 	int b0 = (sY * w) + sX;
	int by = b0/8;
	int bi = 1<<(b0%8);

	if (explored) return (ExploredBitmap[by] & bi)!=0;
	return (VisibleBitmap[by] & bi)!=0;
}

//point a is visible from point b (searchmap)
bool Map::IsVisible(Point &s, Point &d)
{
	int sX=s.x/16;
	int sY=s.y/12;
	int dX=d.x/16;
	int dY=d.y/12;
	int diffx = sX - dX;
	int diffy = sY - dY;
	if (abs( diffx ) >= abs( diffy )) {
		//vertical
		double elevationy = fabs((double)diffx ) / diffy;
		if (sX > dX) {
			for (int startx = sX; startx > dX; startx--) {
				if (GetBlocked( startx, sY - ( int ) ( ( sX - startx ) / elevationy ) ) & PATH_MAP_NO_SEE)
					return false;
			}
		} else {
			for (int startx = sX; startx < dX; startx++) {
				if (GetBlocked( startx, sY + ( int ) ( ( sX - startx ) / elevationy ) ) & PATH_MAP_NO_SEE)
					return false;
			}
		}
	} else {
		double elevationx = fabs((double)diffy ) / diffx;
		if (sY > dY) {
			for (int starty = sY; starty > dY; starty--) {
				if (GetBlocked( sX - ( int ) ( ( sY - starty ) / elevationx ), starty ) & PATH_MAP_NO_SEE)
					return false;
			}
		} else {
			for (int starty = sY; starty < dY; starty++) {
				if (GetBlocked( sX + ( int ) ( ( sY - starty ) / elevationx ), starty ) & PATH_MAP_NO_SEE)
					return false;
			}
		}
	}
	return true;
}

//returns direction of area boundary, returns -1 if it isn't a boundary
int Map::WhichEdge(Point &s)
{
	unsigned int sX=s.x/16;
	unsigned int sY=s.y/12;
	if (!(GetBlocked( sX, sY )&PATH_MAP_TRAVEL)) {
		printMessage("Map"," ",YELLOW);
		printf("This isn't a travel region [%d.%d]?\n",sX, sY);
		return -1;
	}
	sX*=Height;
	sY*=Width;
	if (sX>sY) { //north or east
		if (Width*Height>sX+sY) { //
			return WMP_NORTH;
		}
		return WMP_EAST;
	}
	//south or west
	if (Width*Height<sX+sY) { //
		return WMP_SOUTH;
	}
	return WMP_WEST;
}

//--------ambients----------------
void Map::SetupAmbients()
{
	AmbientMgr *ambim = core->GetSoundMgr()->GetAmbientMgr();
	if (!ambim) return;
	ambim->reset();
	ambim->setAmbients( ambients );
}
//--------mapnotes----------------
//text must be a pointer we can claim ownership of
void Map::AddMapNote(Point &point, int color, char *text)
{
	MapNote *mn = new MapNote;

	mn->Pos=point;
	mn->color=(ieWord) color;
	mn->text=text;
	RemoveMapNote(point); //delete previous mapnote
	mapnotes.push_back(mn);
}

void Map::RemoveMapNote(Point &point)
{
	size_t i = mapnotes.size();
	while (i--) {
		if ((point.x==mapnotes[i]->Pos.x) &&
			(point.y==mapnotes[i]->Pos.y)) {
			delete mapnotes[i];
			mapnotes.erase(mapnotes.begin()+i);
		}
	}
}

MapNote *Map::GetMapNote(Point &point)
{
	size_t i = mapnotes.size();
	while (i--) {
		if (Distance(point, mapnotes[i]->Pos) < 10 ) {
			return mapnotes[i];
		}
	}
	return NULL;
}
//--------spawning------------------
void Map::LoadIniSpawn()
{
	INISpawn = new IniSpawn(this);
	INISpawn->InitSpawn(WEDResRef);
}

void Map::SpawnCreature(Point &pos, const char *CreName, int radius)
{
	SpawnGroup *sg=NULL;
	Actor *creature;
	void* lookup;
	if ( !Spawns.Lookup( CreName, lookup) ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( CreName, IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream);
		if ( creature ) {
			AddActor(creature);
			creature->SetPosition( pos, true, radius );
			creature->RefreshEffects(NULL);
		}
		return;
	}
	sg = (SpawnGroup*)lookup;
	//adjust this with difflev too
	unsigned int count = sg->Count;
	//unsigned int difficulty = sg->Level;
	while ( count-- ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( sg->ResRefs[count], IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream);
		if ( creature ) {
			AddActor(creature);
			creature->SetPosition( pos, true, radius );
			creature->RefreshEffects(NULL);
		}
	}
}

void Map::TriggerSpawn(Spawn *spawn)
{
	//is it still active
	if (!spawn->Flags) {
		return;
	}
	//check schedule
	ieDword bit = 1<<(core->GetGame()->GameTime%7200/300);
	if (!(spawn->appearance & bit)) {
		return;
	}

	//check day or night chance
	if (rand()%100>spawn->DayChance) {
		return;
	}
	//check difficulty
	//create spawns
	for(unsigned int i = 0;i<spawn->Count;i++) {
		SpawnCreature(spawn->Pos, spawn->Creatures[i], 0);
	}
	//disable spawnpoint
	spawn->Flags = 0;
}

//--------restheader----------------
bool Map::Rest(Point &pos, int hours, int day)
{
	//based on ingame timer
	int chance=day?RestHeader.DayChance:RestHeader.NightChance;
	if ( !RestHeader.CreatureNum) return false;
	for (int i=0;i<hours;i++) {
		if ( rand()%100<chance ) {
			int idx = rand()%RestHeader.CreatureNum;
			core->DisplayString( RestHeader.Strref[idx], 0x00404000, IE_STR_SOUND );
			SpawnCreature(pos, RestHeader.CreResRef[idx], 20 );
			return true;
		}
	}
	return false;
}

//--------explored bitmap-----------
int Map::GetExploredMapSize() const
{
	int x = TMap->XCellCount*2;
	int y = TMap->YCellCount*2;
	if (LargeFog) {
		x++;
		y++;
	}
	return (x*y+7)/8;
}

void Map::Explore(int setreset)
{
	memset (ExploredBitmap, setreset, GetExploredMapSize() );
}

void Map::SetMapVisibility(int setreset)
{
	memset( VisibleBitmap, setreset, GetExploredMapSize() );
}

// x, y are in tile coordinates
void Map::ExploreTile(Point &pos)
{
	int h = TMap->YCellCount * 2 + LargeFog;
	int y = pos.y/32;
	if (y < 0 || y >= h)
		return;

	int w = TMap->XCellCount * 2 + LargeFog;
	int x = pos.x/32;
	if (x < 0 || x >= w)
		return;

	int b0 = (y * w) + x;
	int by = b0/8;
	int bi = 1<<(b0%8);

	ExploredBitmap[by] |= bi;
	VisibleBitmap[by] |= bi;
}

void Map::ExploreMapChunk(Point &Pos, int range, int los)
{
	Point Tile;

	if (range>MaxVisibility) {
		range=MaxVisibility;
	}
	int p=VisibilityPerimeter;
	while (p--) {
		int Pass = 2;
		bool block = false;
		bool sidewall = false;
		for (int i=0;i<range;i++) {
			Tile.x = Pos.x+VisibilityMasks[i][p].x;
			Tile.y = Pos.y+VisibilityMasks[i][p].y;

			if (los) {
				if (!block) {
					int type = GetBlocked(Tile);
					if (type & PATH_MAP_NO_SEE) {
						block=true;
					} else if (type & PATH_MAP_SIDEWALL) {
						sidewall=true;
					} else if (sidewall) block=true;
				}
				if (block) {
					Pass--;
					if (!Pass) break;
				}
			}
			ExploreTile(Tile);
		}
	}
}

void Map::UpdateFog()
{
	if (!(core->FogOfWar&FOG_DRAWFOG) ) {
		SetMapVisibility( -1 );
		return;
	}

	SetMapVisibility( 0 );
	for (unsigned int e = 0; e<actors.size(); e++) {
		Actor *actor = actors[e];
		if (!actor->Modified[ IE_EXPLORE ] ) continue;
		int state = actor->Modified[IE_STATE_ID];
		if (state & STATE_CANTSEE) continue;
		int vis2 = actor->Modified[IE_VISUALRANGE];
		if ((state&STATE_BLIND) || (vis2<2)) vis2=2; //can see only themselves
		ExploreMapChunk (actor->Pos, vis2, 1);
		Spawn *sp = GetSpawnRadius(actor->Pos, SPAWN_RANGE); //30 * 12
		if (sp) {
			TriggerSpawn(sp);
		}
	}
}

//Valid values are - PATH_MAP_FREE, PATH_MAP_PC, PATH_MAP_NPC
void Map::BlockSearchMap(Point &Pos, int size, unsigned int value)
{
	if (size>MAX_CIRCLESIZE) {
		size=MAX_CIRCLESIZE;
	}
	int p=PersonalPerimeter;
	while (p--) {
		for (int i=0;i<size;i++) {
			unsigned int x = (Pos.x+PersonalSpaces[i][p].x)/16;
			unsigned int y = (Pos.y+PersonalSpaces[i][p].y)/12;

			unsigned int tmp=SearchMap->GetPixelIndex(x,y)&PATH_MAP_NOTACTOR;//keep door flags too
			SearchMap->SetPixelIndex(x,y,tmp|value);
		}
	}
}

Spawn* Map::GetSpawn(const char* Name)
{
	for (size_t i = 0; i < spawns.size(); i++) {
		Spawn* sp = spawns[i];

		if (stricmp( sp->Name, Name ) == 0)
			return sp;
	}
	return NULL;
}

Spawn *Map::GetSpawnRadius(Point &point, unsigned int radius)
{
	for (size_t i = 0; i < spawns.size(); i++) {
		Spawn* sp = spawns[i];

		if (Distance(point, sp->Pos)<radius) {
			return sp;
		}
	}
	return NULL;
}

int Map::ConsolidateContainers()
{
	int itemcount = 0;
	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);

		if (TMap->CleanupContainer(c) ) {
			continue;
		}
		itemcount += c->inventory.GetSlotCount();
	}
	return itemcount;
}

//Pos could be [-1,-1] in which case it copies the ground piles to their
//original position in the second area
void Map::CopyGroundPiles(Map *othermap, Point &Pos)
{
	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);
		if (c->Type==IE_CONTAINER_PILE) {
			//creating (or grabbing) the container in the other map at the given position
			Container *othercontainer;
			if (Pos.isempty()) {
				 othercontainer = othermap->GetPile(c->Pos);
			} else {
				 othercontainer = othermap->GetPile(Pos);
			}
			//transfer the pile to the other container
			unsigned int i=c->inventory.GetSlotCount();
			while (i--) {
				CREItem *item = c->RemoveItem(i, 0);
				othercontainer->AddItem(item);
			}
		}
	}
}

Container *Map::GetPile(Point &position)
{
	Point tmp[4];
	char heapname[32];

	//converting to search square
	position.x=position.x/16;
	position.y=position.y/12;
	sprintf(heapname,"heap_%hd.%hd",position.x,position.y);
	//pixel position is centered on search square
	position.x=position.x*16+8;
	position.y=position.y*12+6;
	Container *container = TMap->GetContainer(position,IE_CONTAINER_PILE);
	if (!container) {
		//bounding box covers the search square
		tmp[0].x=position.x-8;
		tmp[0].y=position.y-6;
		tmp[1].x=position.x+8;
		tmp[1].y=position.y-6;
		tmp[2].x=position.x+8;
		tmp[2].y=position.y+6;
		tmp[3].x=position.x-8;
		tmp[3].y=position.y+6;
		Gem_Polygon* outline = new Gem_Polygon( tmp, 4 );
		container = AddContainer(heapname, IE_CONTAINER_PILE, outline);
		container->Pos=position;
	}
	return container;
}

void Map::AddItemToLocation(Point &position, CREItem *item)
{
	Container *container = GetPile(position);
	container->AddItem(item);
}

Container* Map::AddContainer(const char* Name, unsigned short Type,
	Gem_Polygon* outline)
{
	Container* c = new Container();
	c->SetScriptName( Name );
	c->Type = Type;
	c->outline = outline;
	c->SetMap(this);
	TMap->AddContainer( c );
	return c;
}

int Map::GetCursor( Point &p)
{
	if (!IsVisible( p, true ) ) {
		return IE_CURSOR_INVALID;
	}
	switch (GetBlocked( p ) & (PATH_MAP_PASSABLE|PATH_MAP_TRAVEL)) {
		case 0:
			return IE_CURSOR_BLOCKED;
		case PATH_MAP_PASSABLE:
			return IE_CURSOR_WALK;
		default:
			return IE_CURSOR_TRAVEL;
	}
}

bool Map::HasWeather()
{
	if ((AreaType & (AT_WEATHER|AT_OUTDOOR) ) != (AT_WEATHER|AT_OUTDOOR) ) {
		return false;
	}
	return true;
}

int Map::GetWeather()
{
	if (Rain>=core->Roll(1,100,0) ) {
		if (Lightning>=core->Roll(1,100,0) ) {
			return WB_LIGHTNING|WB_RAIN;
		}
		return WB_RAIN;
	}
	if (Snow>=core->Roll(1,100,0) ) {
		return WB_SNOW;
	}
	if (Fog>=core->Roll(1,100,0) ) {
		return WB_FOG;
	}
	return WB_NORMAL;
}

#define SPARKLE_PUFF   1
#define SPARKLE_SHOWER 3

void Map::FadeSparkle(Point &pos, bool forced)
{
	spaIterator iter;

	for(iter=particles.begin(); iter!=particles.end();iter++) {
		if ((*iter)->MatchPos(pos) ) {
			if (forced) {
				//particles.erase(iter);
				(*iter)->SetPhase(P_EMPTY);
			} else {
				(*iter)->SetPhase(P_FADE);
			}
			return;
		}
	}
}

void Map::Sparkle(ieDword color, ieDword type, Point &pos)
{
	Particles *sparkles = new Particles(100);
	sparkles->SetOwner(this);
	sparkles->SetRegion(pos.x-20, pos.y-80, 40, 80);

	int style, path, grow;

	//the high word is ignored in the original engine (compatibility hack)
	switch(type&0xffff) {
	case SPARKLE_SHOWER:
		style = SP_TYPE_POINT;
		path = SP_PATH_FALL;
		grow = SP_SPAWN_FULL;
		break;
	case SPARKLE_PUFF:
		style = SP_TYPE_POINT;
		path = SP_PATH_FOUNT;
		grow = SP_SPAWN_FULL;
		break;
	default:
		style = SP_TYPE_POINT;
		path = SP_PATH_FLIT;
		grow = SP_SPAWN_SOME;
		break;
	}
	sparkles->SetType(style, path, grow);
	sparkles->SetColor(color);
	sparkles->SetPhase(P_GROW);
	printf("sparkle: %d %d\n", color, type);
	printf("Position: %d.%d\n", pos.x,pos.y);

	spaIterator iter;
	//this is pos.y we just set
	//int h = sparkles->GetHeight();
	for(iter=particles.begin(); (iter!=particles.end()) && ((*iter)->GetHeight()<pos.y); iter++);
	particles.insert(iter, sparkles);
}

//remove flags from actor if it has left the trigger area it had last entered
void Map::ClearTrap(Actor *actor, ieDword InTrap)
{
	InfoPoint *trap = TMap->GetInfoPoint(InTrap);
	if (!trap) {
		actor->SetInTrap(0);
	} else {
		if(!trap->outline->PointIn(actor->Pos)) {
			actor->SetInTrap(0);
		}
	}
}

void Map::SetTrackString(ieStrRef strref, bool flg, int difficulty)
{
	trackString = strref;
	trackFlag = flg;
	trackDiff = (ieWord) difficulty;
}

bool Map::DisplayTrackString(Actor *target)
{
	ieDword skill = target->GetStat(IE_TRACKING);
	//remove this, if it can succeed without skill
	if (!skill) return true;
	if (core->Roll(1,20,skill)<trackDiff) {
		core->DisplayConstantStringName(STR_TRACKINGFAILED, 0xd7d7be, target);
		return true;
	}
	if (trackFlag) {
			char * str = core->GetString( trackString);
			core->GetTokenDictionary()->SetAt( "CREATURE", str);
			core->DisplayConstantStringName(STR_TRACKING, 0xd7d7be, target);
			return false;
	}
	core->DisplayStringName(trackString, 0xd7d7be, target, 0);
	return false;
}

////////////////////AreaAnimation//////////////////
//Area animation

AreaAnimation::AreaAnimation()
{
	animation=NULL;
	animcount=0;
	palette=NULL;
}

AreaAnimation::~AreaAnimation()
{
	for(int i=0;i<animcount;i++) {
		if (animation[i]) {
			delete (animation[i]);
		}
	}
	free(animation);
	core->FreePalette(palette, PaletteRef);
}

void AreaAnimation::SetPalette(ieResRef Pal)
{
	Flags |= A_ANI_PALETTE;
	core->FreePalette(palette, PaletteRef);
	strnlwrcpy(PaletteRef, Pal, 8);
	palette = core->GetPalette(PaletteRef);
	if (Flags&A_ANI_BLEND) {
		//re-blending after palette change
		BlendAnimation();
	}
}

void AreaAnimation::BlendAnimation()
{
	//Warning! This function will modify a shared palette
	if (!palette) {
		// CHECKME: what should we do here? Currently copying palette
		// from first frame of first animation

		if (animcount == 0 || !animation[0]) return;
		Sprite2D* spr = animation[0]->GetFrame(0);
		if (!spr) return;
		palette = core->GetVideoDriver()->GetPalette(spr)->Copy();
	}
	palette->CreateShadedAlphaChannel();
}

bool AreaAnimation::Schedule(ieDword gametime)
{
	if (!(Flags&A_ANI_ACTIVE) ) {
		return false;
	}

	//check for schedule
	ieDword bit = 1<<(gametime%7200/300);
	if (appearance & bit) {
		return true;
	}
	return false;
}
