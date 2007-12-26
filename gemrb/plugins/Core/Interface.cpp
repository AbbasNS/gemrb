/* GemRB - Infinity Engine Emulator
* Copyright (C) 2003-2005 The GemRB Project
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

#ifndef INTERFACE
#define INTERFACE
#endif

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/strrefs.h"

#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#endif

#include "Interface.h"
#include "FileStream.h"
#include "AnimationMgr.h"
#include "ArchiveImporter.h"
#include "WorldMapMgr.h"
#include "AmbientMgr.h"
#include "ItemMgr.h"
#include "SpellMgr.h"
#include "EffectMgr.h"
#include "StoreMgr.h"
#include "DialogMgr.h"
#include "MapControl.h"
#include "EffectQueue.h"
#include "MapMgr.h"
#include "TileMap.h"
#include "ScriptedAnimation.h"
#include "Video.h"
#include "ResourceMgr.h"
#include "PluginMgr.h"
#include "StringMgr.h"
#include "ScriptEngine.h"
#include "ActorMgr.h"
#include "Factory.h"
#include "Console.h"
#include "Label.h"
#include "Button.h"
#include "SoundMgr.h"
#include "SaveGameIterator.h"
#include "MusicMgr.h"
#include "MoviePlayer.h"
#include "GameControl.h"
#include "Game.h"
#include "DataFileMgr.h"
#include "SaveGameMgr.h"
#include "WorldMapControl.h"
#include "Palette.h"
#include "ProjectileServer.h"

GEM_EXPORT Interface* core;

#ifdef WIN32
GEM_EXPORT HANDLE hConsole;
#endif

//#include "../../includes/win32def.h"
//#include "../../includes/globals.h"
//#include "../../includes/strrefs.h"

//use DialogF.tlk if the protagonist is female, that's why we leave space
static const char dialogtlk[] = "dialog.tlk\0";

static int strref_table[STRREF_COUNT];

static int MaximumAbility = 25;
static ieWordSigned *strmod = NULL;
static ieWordSigned *strmodex = NULL;
static ieWordSigned *intmod = NULL;
static ieWordSigned *dexmod = NULL;
static ieWordSigned *conmod = NULL;
static ieWordSigned *chrmod = NULL;

Interface::Interface(int iargc, char* iargv[])
{
	argc = iargc;
	argv = iargv;
#ifdef WIN32
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
	textcolor( LIGHT_WHITE );
	printf( "GemRB Core Version v%s Loading...\n", VERSION_GEMRB );
	projserv = NULL;
	video = NULL;
	key = NULL;
	strings = NULL;
	guiscript = NULL;
	windowmgr = NULL;
	vars = NULL;
	tokens = NULL;
	RtRows = NULL;
	music = NULL;
	soundmgr = NULL;
	sgiterator = NULL;
	INIparty = NULL;
	INIbeasts = NULL;
	INIquests = NULL;
	INIresdata = NULL;
	game = NULL;
	worldmap = NULL;
	CurrentStore = NULL;
	CurrentContainer = NULL;
	UseContainer = false;
	InfoTextPalette = NULL;
	timer = NULL;
	evntmgr = NULL;
	opcodemgrs = NULL;
	console = NULL;
	slottypes = NULL;
	slotmatrix = NULL;

	ModalWindow = NULL;
	tooltip_x = 0;
	tooltip_y = 0;
	tooltip_ctrl = NULL;
	plugin = NULL;
	factory = NULL;

	pal16 = NULL;
	pal32 = NULL;
	pal256 = NULL;

	CursorCount = 0;
	Cursors = NULL;

	ConsolePopped = false;
	CheatFlag = false;
	FogOfWar = 1;
	QuitFlag = QF_NORMAL;
	EventFlag = EF_CONTROL;
#ifndef WIN32
	CaseSensitive = true; //this is the default value, so CD1/CD2 will be resolved
#else
	CaseSensitive = false;
#endif
	GameOnCD = false;
	SkipIntroVideos = false;
	DrawFPS = false;
	TooltipDelay = 100;
	GUIScriptsPath[0] = 0;
	GamePath[0] = 0;
	SavePath[0] = 0;
	GemRBPath[0] = 0;
	PluginsPath[0] = 0;
	CachePath[0] = 0;
	GameName[0] = 0;
	strncpy( GameOverride, "override", sizeof(GameOverride) );
	strncpy( GameSounds, "sounds", sizeof(GameSounds) );
	strncpy( GameScripts, "scripts", sizeof(GameScripts) );
	strncpy( GamePortraits, "portraits", sizeof(GamePortraits) );
	strncpy( GameCharacters, "characters", sizeof(GameCharacters) );
	strncpy( GameData, "data", sizeof(GameData) );
	strncpy( INIConfig, "baldur.ini", sizeof(INIConfig) );
	strncpy( ButtonFont, "STONESML", sizeof(ButtonFont) );
	strncpy( TooltipFont, "STONESML", sizeof(TooltipFont) );
	strncpy( MovieFont, "STONESML", sizeof(MovieFont) );
	strncpy( CursorBam, "CAROT", sizeof(CursorBam) );
	strncpy( ScrollCursorBam, "CURSARW", sizeof(ScrollCursorBam) );
	strncpy( GlobalScript, "BALDUR", sizeof(GlobalScript) );
	strncpy( WorldMapName, "WORLDMAP", sizeof(WorldMapName) );
	strncpy( Palette16, "MPALETTE", sizeof(Palette16) );
	strncpy( Palette32, "PAL32", sizeof(Palette32) );
	strncpy( Palette256, "MPAL256", sizeof(Palette256) );
	strcpy( TooltipBackResRef, "\0" );
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		strcpy( GroundCircleBam[size], "\0" );
		GroundCircleScale[size] = 0;
	}
	TooltipColor.r = 0;
	TooltipColor.g = 255;
	TooltipColor.b = 0;
	TooltipColor.a = 255;
	TooltipMargin = 10;

	TooltipBack = NULL;
	DraggedItem = NULL;
	DefSound = NULL;
	DSCount = -1;
	GameFeatures = 0;
	GameFeatures2 = 0;
	memset( WindowFrames, 0, sizeof( WindowFrames ));
	memset( GroundCircles, 0, sizeof( GroundCircles ));
	memset(FogSprites, 0, sizeof( FogSprites ));
	//memset(ArrowSprites, 0, sizeof( ArrowSprites ));
	AreaAliasTable = NULL;
	ItemExclTable = NULL;
	ItemDialTable = NULL;
	ItemDial2Table = NULL;
	ItemTooltipTable = NULL;
}

#define FreeInterfaceVector(type, variable, member) \
{ \
	std::vector<type>::iterator i; \
	for(i = (variable).begin(); i != (variable).end(); ++i) { \
	if (!(*i).free) { \
		FreeInterface((*i).member); \
		(*i).free = true; \
	} \
	} \
}

#define FreeResourceVector(type, variable) \
{ \
	size_t i=variable.size(); \
	while(i--) { \
		if (variable[i]) { \
		delete variable[i]; \
		} \
	} \
	variable.clear(); \
}

static void ReleaseItem(void *poi)
{
	delete ((Item *) poi);
}

static void ReleaseSpell(void *poi)
{
	delete ((Spell *) poi);
}

static void ReleaseEffect(void *poi)
{
	delete ((Effect *) poi);
}

static void ReleasePalette(void *poi)
{
	//we allow nulls, but we shouldn't release them
	if (!poi) return;
	//as long as palette has its own refcount, this should be Release
	((Palette *) poi)->Release();
}

static void ReleaseItemList(void *poi)
{
	delete ((ItemList *) poi);
}

void FreeAbilityTables()
{
	if (strmod) {
		free(strmod);
	}
	strmod = NULL;
	if (strmodex) {
		free(strmodex);
	}
	strmodex = NULL;
	if (intmod) {
		free(intmod);
	}
	intmod = NULL;
	if (dexmod) {
		free(dexmod);
	}
	dexmod = NULL;
	if (conmod) {
		free(conmod);
	}
	conmod = NULL;
	if (chrmod) {
		free(chrmod);
	}
	chrmod = NULL;
}

void Interface::FreeResRefTable(ieResRef *&table, int &count)
{
	if (table) {
		free( table );
		count = -1;
	}
}

static void ReleaseItemTooltip(void *poi)
{
	free(poi);
}

Interface::~Interface(void)
{
	DragItem(NULL,NULL);
	delete AreaAliasTable;

	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (soundmgr) {
		AmbientMgr *ambim = soundmgr->GetAmbientMgr();
		if (ambim) ambim->deactivate();
	}
	//destroy the highest objects in the hierarchy first!
	delete game;
	delete worldmap;

	FreeAbilityTables();
	//aww, i'm sure this could be implemented better
	MapMgr* mm = ( MapMgr* ) GetInterface( IE_ARE_CLASS_ID );
	if (mm!=NULL) {
		mm->ReleaseMemory();
		FreeInterface(mm);
	}

	EffectQueue_ReleaseMemory();
	CharAnimations::ReleaseMemory();
	delete CurrentStore;
	ItemCache.RemoveAll(ReleaseItem);
	SpellCache.RemoveAll(ReleaseSpell);
	EffectCache.RemoveAll(ReleaseEffect);
	PaletteCache.RemoveAll(ReleasePalette);

	FreeResRefTable(DefSound, DSCount);

	free( slottypes );
	free( slotmatrix );

	FreeInterface( music );
	FreeInterface( soundmgr );

	delete sgiterator;

	if (Cursors) {
		for (int i = 0; i < CursorCount; i++) {
			video->FreeSprite( Cursors[i] );
		}
		delete[] Cursors;
	}

	FreeResourceVector( Font, fonts );
	FreeResourceVector( Window, windows );

	delete projserv;

	delete console;

	FreeInterface( key );
	FreeInterface( pal256 );
	FreeInterface( pal32 );
	FreeInterface( pal16 );

	delete timer;

	FreeInterface( windowmgr );

	if (video) {
		unsigned int i;

		//for(i=0;i<sizeof(ArrowSprites)/sizeof(Sprite2D *);i++ ) {
		//	video->FreeSprite(ArrowSprites[i]);
		//}

		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			video->FreeSprite(FogSprites[i]);
		}

		for(i=0;i<4;i++) {
			video->FreeSprite(WindowFrames[i]);
		}

		for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
			for(i=0;i<6;i++) {
				video->FreeSprite(GroundCircles[size][i]);
			}
		}

		if (TooltipBack) {
			for(i=0;i<3;i++) {
				//freesprite checks for null pointer
				video->FreeSprite(TooltipBack[i]);
			}
			delete[] TooltipBack;
		}
		if (InfoTextPalette) {
			FreePalette(InfoTextPalette);
		}

		video->SetDragCursor(NULL);
	}

	delete factory;

	FreeInterface( video );

	delete evntmgr;

	FreeInterface( guiscript );

	delete vars;
	delete tokens;
	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
		delete RtRows;
	}
	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
		delete ItemExclTable;
	}
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
		delete ItemDialTable;
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
		delete ItemDial2Table;
	}
	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
		delete ItemTooltipTable;
	}

	FreeInterfaceVector( Table, tables, tm );
	FreeInterfaceVector( Symbol, symbols, sm );

	if (opcodemgrs) {
		FreeInterfaceVector( InterfaceElement, *opcodemgrs, mgr );
		delete opcodemgrs;
		opcodemgrs=NULL;
	}

	FreeInterface(INIquests);
	FreeInterface(INIbeasts);
	FreeInterface(INIparty);
	FreeInterface(INIresdata);

	Map::ReleaseMemory();
	GameScript::ReleaseMemory();
	Actor::ReleaseMemory();

	FreeInterface( strings );

	delete plugin;

	// Removing all stuff from Cache, except bifs
	DelTree((const char *) CachePath, true);
}

void Interface::SetWindowFrame(int i, Sprite2D *Picture)
{
	video->FreeSprite(WindowFrames[i]);
	WindowFrames[i]=Picture;
}

GameControl* Interface::StartGameControl()
{
	//making sure that our window is the first one
	if (ConsolePopped) {
		PopupConsole();
	}
	DelWindow(0xffffu);//deleting ALL windows
	DelTable(0xffffu); //dropping ALL tables
	Window* gamewin = new Window( 0xffff, 0, 0, (ieWord) Width, (ieWord) Height );
	gamewin->WindowPack[0]=0;
	GameControl* gc = new GameControl();
	gc->XPos = 0;
	gc->YPos = 0;
	gc->Width = (ieWord) Width;
	gc->Height = (ieWord) Height;
	gc->Owner = gamewin;
	gc->ControlID = 0x00000000;
	gc->ControlType = IE_GUI_GAMECONTROL;
	gamewin->AddControl( gc );
	AddWindow( gamewin );
	SetVisible( 0, WINDOW_VISIBLE );
	//setting the focus to the game control
	evntmgr->SetFocused(gamewin, gc);
	if (guiscript->LoadScript( "MessageWindow" )) {
		guiscript->RunFunction( "OnLoad" );
		gc->UnhideGUI();
	}

	return gc;
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
these events are pending until conditions are right
*/
void Interface::HandleEvents()
{
	GameControl *gc = GetGameControl();
	if (gc && (!gc->Owner || !gc->Owner->Visible)) {
		gc=NULL;
	}

	if (EventFlag&EF_UPDATEANIM) {
		EventFlag&=~EF_UPDATEANIM;
		guiscript->RunFunction( "UpdateAnimation", false);
	}

	if (EventFlag&EF_PORTRAIT) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "PortraitWindow", tmp );
		if (tmp != (ieDword) ~0) {
			EventFlag&=~EF_PORTRAIT;
			guiscript->RunFunction( "UpdatePortraitWindow" );
		}
	}

	if ((EventFlag&EF_CONTROL) && gc) {
		EventFlag&=~EF_CONTROL;
		guiscript->RunFunction( "UpdateControlStatus" );
		//this is the only value we can use here
		if (game->ControlStatus & CS_HIDEGUI)
			gc->HideGUI();
		else
			gc->UnhideGUI();
		return;
	}
	if ((EventFlag&EF_SHOWMAP) && gc) {
		ieDword tmp = (ieDword) ~0;
		vars->Lookup( "OtherWindow", tmp );
		if (tmp == (ieDword) ~0) {
			EventFlag &= ~EF_SHOWMAP;
			guiscript->RunFunction( "ShowMap" );
		}
		return;
	}

	if (EventFlag&EF_SEQUENCER) {
		EventFlag&=~EF_SEQUENCER;
		guiscript->RunFunction( "OpenSequencerWindow" );
		return;
	}

	if (EventFlag&EF_IDENTIFY) {
		EventFlag&=~EF_IDENTIFY;
		guiscript->RunFunction( "OpenIdentifyWindow" );
		return;
	}
}

/* handle main loop events that might destroy or create windows
thus cannot be called from DrawWindows directly
*/
void Interface::HandleFlags()
{
	EventFlag = EF_CONTROL; //clear events because the context changed

	if (QuitFlag&(QF_QUITGAME|QF_EXITGAME) ) {
		// when reaching this, quitflag should be 1 or 2
		// if Exitgame was set, we'll set Start.py too
		QuitGame (QuitFlag&QF_EXITGAME);
		QuitFlag &= ~(QF_QUITGAME|QF_EXITGAME);
	}

	if (QuitFlag&QF_LOADGAME) {
		QuitFlag &= ~QF_LOADGAME;
		LoadGame(LoadGameIndex);
	}

	if (QuitFlag&QF_ENTERGAME) {
		QuitFlag &= ~QF_ENTERGAME;
		if (game) {
			timer->Init();
	
			//rearrange party slots
			game->ConsolidateParty();
			GameControl* gc = StartGameControl();
			//switch map to protagonist
			Actor* actor = game->FindPC (1);
			if (!actor) {
				actor = game->GetPC (0, false);
			}
			if (actor) {
				gc->ChangeMap(actor, true);
			}
		} else {
			printMessage("Core", "No game to enter...\n", LIGHT_RED);
			QuitFlag = QF_QUITGAME;
		}
	}

	if (QuitFlag&QF_CHANGESCRIPT) {
		QuitFlag &= ~QF_CHANGESCRIPT;
		guiscript->LoadScript( NextScript );
		guiscript->RunFunction( "OnLoad" );
	}
}

bool GenerateAbilityTables()
{
	FreeAbilityTables();

	//range is: 0 - maximumability
	int tablesize = MaximumAbility+1;
	strmod = (ieWordSigned *) malloc (tablesize * 4 * sizeof(ieWordSigned) );
	if (!strmod)
		return false;
	strmodex = (ieWordSigned *) malloc (101 * 4 * sizeof(ieWordSigned) );
	if (!strmodex)
		return false;
	intmod = (ieWordSigned *) malloc (tablesize * 3 * sizeof(ieWordSigned) );
	if (!intmod)
		return false;
	dexmod = (ieWordSigned *) malloc (tablesize * 3 * sizeof(ieWordSigned) );
	if (!dexmod)
		return false;
	conmod = (ieWordSigned *) malloc (tablesize * 5 * sizeof(ieWordSigned) );
	if (!conmod)
		return false;
	chrmod = (ieWordSigned *) malloc (tablesize * 1 * sizeof(ieWordSigned) );
	if (!chrmod)
		return false;
	return true;
}

bool Interface::ReadAbilityTable(const ieResRef tablename, ieWordSigned *mem, int columns, int rows)
{
	TableMgr * tab;
	int table=LoadTable( tablename );

	if (table<0) {
		return false;
	}
	tab = GetTable( table );
	if (!tab) {
		DelTable(table);
		return false;
	}
	//this is a hack for rows not starting at 0 in some cases
	int fix = 0;
	const char * tmp = tab->GetRowName(0);
	if (tmp && (tmp[0]!='0')) {
		fix = atoi(tmp);
		for (int i=0;i<fix;i++) {
			for (int j=0;j<columns;j++) {
				mem[rows*j+i]=(ieWordSigned) strtol(tab->QueryField(0,j),NULL,0 );
			}
		}
	}
	for (int j=0;j<columns;j++) {
		for( int i=0;i<rows-fix;i++) {
			mem[rows*j+i+fix] = (ieWordSigned) strtol(tab->QueryField(i,j),NULL,0 );
		}
	}
	DelTable(table);
	return true;
}

bool Interface::ReadAbilityTables()
{
	bool ret = GenerateAbilityTables();
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmod", strmod, 4, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("strmodex", strmodex, 4, 101);
	//3rd ed doesn't have strmodex, but has a maximum of 40
	if (!ret && (MaximumAbility<=25) )
		return ret;
	ret = ReadAbilityTable("intmod", intmod, 3, MaximumAbility + 1);
	if (!ret)
		return ret;
	ret = ReadAbilityTable("dexmod", dexmod, 3, MaximumAbility + 1);
	//no dexmod in iwd2???
	ret = ReadAbilityTable("hpconbon", conmod, 5, MaximumAbility + 1);
	if (!ret)
		return ret;
	//this table is a single row (not a single column)
	ret = ReadAbilityTable("chrmodst", chrmod, MaximumAbility + 1, 1);
	if (!ret)
		return ret;
	return true;
}

bool Interface::ReadAuxItemTables()
{
	int idx;
	TableMgr* aa;
	int table;
	bool flag = true;

	if (ItemExclTable) {
		ItemExclTable->RemoveAll(NULL);
	} else {
		ItemExclTable = new Variables();
		ItemExclTable->SetType(GEM_VARIABLES_INT);
	}
	table = LoadTable( "itemexcl" );

	if (table < 0) {
		//don't report error when the file doesn't exist
		goto aux_1;
	}
	aa = GetTable( table );
	if (!aa) {
		DelTable( table );
		flag = false;
		goto aux_1;
	}
	idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key;

		strnlwrcpy(key,aa->GetRowName(idx),8);
		ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
		ItemExclTable->SetAt(key, value);
	}
	DelTable( table );
aux_1:
	if (ItemDialTable) {
		ItemDialTable->RemoveAll(NULL);
	} else {
		ItemDialTable = new Variables();
		ItemDialTable->SetType(GEM_VARIABLES_INT);
	}
	if (ItemDial2Table) {
		ItemDial2Table->RemoveAll(NULL);
	} else {
		ItemDial2Table = new Variables();
		ItemDial2Table->SetType(GEM_VARIABLES_STRING);
	}
	table = LoadTable( "itemdial" );

	if (table < 0) {
		//don't report error when the file doesn't exist
		goto aux_2;
	}
	aa = GetTable( table );
	if (!aa) {
		DelTable( table );
		flag = false;
		goto aux_2;
	}
	idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key, dlgres;

		strnlwrcpy(key,aa->GetRowName(idx),8);
		ieDword value = strtol(aa->QueryField(idx,0),NULL,0);
		ItemDialTable->SetAt(key, value);
		strnlwrcpy(dlgres,aa->QueryField(idx,1),8);
		ItemDial2Table->SetAtCopy(key, dlgres);
	}
	DelTable( table );
aux_2:
	if (ItemTooltipTable) {
		ItemTooltipTable->RemoveAll(ReleaseItemTooltip);
	} else {
		ItemTooltipTable = new Variables();
		ItemTooltipTable->SetType(GEM_VARIABLES_POINTER);
	}
	table = LoadTable( "tooltip" );

	if (table < 0) {
		//don't report error when the file doesn't exist
		goto aux_3;
	}
	aa = GetTable( table );
	if (!aa) {
		DelTable( table );
		flag = false;
		goto aux_3;
	}
	idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key;
		int *tmppoi = (int *) malloc(sizeof(int)*3);

		strnlwrcpy(key,aa->GetRowName(idx),8);
		for (int i=0;i<3;i++) {
			tmppoi[i] = atoi(aa->QueryField(idx,i));
		}
		ItemTooltipTable->SetAt(key, (void*)tmppoi);
	}
	DelTable( table );
aux_3:
	return flag;
}

int Interface::GetItemExcl(const ieResRef itemname) const
{
	ieDword value;

	if (ItemExclTable && ItemExclTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return 0;
}

int Interface::GetItemTooltip(const ieResRef itemname, int header)
{
	int *value = NULL;

	if (ItemTooltipTable) {
		void* lookup = NULL;
		ItemTooltipTable->Lookup(itemname, lookup);
		value = (int*)lookup;
	}
	if (value && (value[header]>=0)) {
		return value[header];
	}
	Item *item = GetItem(itemname);
	if (!item) {
		return -1;
	}
	int ret = item->ItemName;
	FreeItem(item, itemname, 0);
	return ret;
}

int Interface::GetItemDialStr(const ieResRef itemname) const
{
	ieDword value;

	if (ItemDialTable && ItemDialTable->Lookup(itemname, value)) {
		return (int) value;
	}
	return -1;
}

//second value is the item dialog resource returned by this method
int Interface::GetItemDialRes(const ieResRef itemname, ieResRef retval) const
{
	if (ItemDial2Table && ItemDial2Table->Lookup(itemname, retval, sizeof(ieResRef))) {
		return 1;
	}
	return 0;
}

bool Interface::ReadAreaAliasTable(const ieResRef tablename)
{
	if (AreaAliasTable) {
		AreaAliasTable->RemoveAll(NULL);
	} else {
		AreaAliasTable = new Variables();
		AreaAliasTable->SetType(GEM_VARIABLES_INT);
	}

	int table = LoadTable( tablename );

	if (table < 0) {
		//don't report error when the file doesn't exist
		return true;
	}
	TableMgr* aa = GetTable( table );
	if (!aa) {
		DelTable( table );
		return false;
	}
	int idx = aa->GetRowCount();
	while (idx--) {
		ieResRef key;

		strnlwrcpy(key,aa->GetRowName(idx),8);
		ieDword value = atoi(aa->QueryField(idx,0));
		AreaAliasTable->SetAt(key, value);
	}
	DelTable( table );
	return true;
}

//this isn't const
int Interface::GetAreaAlias(const ieResRef areaname) const
{
	ieDword value;

	if (AreaAliasTable && AreaAliasTable->Lookup(areaname, value)) {
		return (int) value;
	}
	return -1;
}

static const Color white = {0xff,0xff,0xff,0xff};
static const Color black = {0x00,0x00,0x00,0xff};
static const Region bg( 0, 0, 100, 30 );

/** this is the main loop */
void Interface::Main()
{
	video->CreateDisplay( Width, Height, Bpp, FullScreen );
	video->SetDisplayTitle( GameName, GameType );
	ieDword brightness = 10;
	ieDword contrast = 5;
	ieDword speed = 10;
	vars->Lookup("Brightness Correction", brightness);
	vars->Lookup("Gamma Correction", contrast);
	vars->Lookup("Mouse Scroll Speed", speed);
	video->SetGamma(brightness, contrast);
	video->SetMouseScrollSpeed((int) speed);
	vars->Lookup("Tooltips", TooltipDelay);

	Font* fps = GetFont( ( unsigned int ) 0 );
	char fpsstring[40]={"???.??? fps"};
	unsigned long frame = 0, time, timebase;
	GetTime(timebase);
	double frames = 0.0;
	Palette* palette = CreatePalette( white, black );
	do {
		//don't change script when quitting is pending

		while (QuitFlag) {
			HandleFlags();
		}
		if (EventFlag) {
			HandleEvents();
		}

		DrawWindows();
		if (DrawFPS) {
			frame++;
			GetTime( time );
			if (time - timebase > 1000) {
				frames = ( frame * 1000.0 / ( time - timebase ) );
				timebase = time;
				frame = 0;
				sprintf( fpsstring, "%.3f fps", frames );
			}
			video->DrawRect( bg, black );
			fps->Print( bg,
				( unsigned char * ) fpsstring, palette,
				IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE, true );
		}
	} while (video->SwapBuffers() == GEM_OK);
	FreePalette( palette );
}

bool Interface::ReadStrrefs()
{
	int i;
	TableMgr * tab;
	int table=LoadTable("strings");
	memset(strref_table,-1,sizeof(strref_table) );
	if (table<0) {
		return false;
	}
	tab = GetTable(table);
	if (!tab) {
		goto end;
	}
	for(i=0;i<STRREF_COUNT;i++) {
		strref_table[i]=atoi(tab->QueryField(i,0));
	}
end:
	DelTable(table);
	return true;
}

int Interface::ReadResRefTable(const ieResRef tablename, ieResRef *&data)
{
	int count = 0;

	if (data) {
		free(data);
		data = NULL;
	}
	int table = LoadTable( tablename );
	if (table < 0) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find %s.2da.\n",tablename );
		return 0;
	}
	TableMgr* tm = GetTable( table );
	if (tm) {
		count = tm->GetRowCount();
		data = (ieResRef *) calloc( count, sizeof(ieResRef) );
		for (int i = 0; i < count; i++) {
			strnlwrcpy( data[i], tm->QueryField( i, 0 ), 8 );
		}
		DelTable( table );
	}
	return count;
}

int Interface::LoadSprites()
{
	ieDword i;
	int size;
	int table = -1;
	if (!IsAvailable( IE_2DA_CLASS_ID )) {
		printf( "No 2DA Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	//loading cursors
	printMessage( "Core", "Loading Cursors...\n", WHITE );

	AnimationFactory* anim;
	anim = (AnimationFactory*)GetResourceMgr()->GetFactoryResource("cursors", IE_BAM_CLASS_ID);
	if (anim)
	{
		CursorCount = anim->GetCycleCount();
		Cursors = new Sprite2D * [CursorCount];
		for (int i = 0; i < CursorCount; i++) {
			Cursors[i] = anim->GetFrame( 0, (ieByte) i );
		}
	}

	// this is the last existing cursor type
	if (CursorCount<IE_CURSOR_WAY) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	video->SetCursor( Cursors[0], Cursors[1] );
	printStatus( "OK", LIGHT_GREEN );

	// Load arrow cursors
	//str = key->GetResource( "cursarw", IE_BAM_CLASS_ID );
	//printMessage( "Core", "Loading arrow cursor bitmaps...\n", WHITE );
	//anim->Open( str, true );
	//if (anim->GetCycleCount( ) != MAX_ORIENT/2) {
	//	printStatus( "ERROR", LIGHT_RED );
	//	goto end_of_init;
	//}

	//for(i=0;i<MAX_ORIENT/2;i++) {
	//	ArrowSprites[i] = anim->GetFrameFromCycle( i, 0);
	//}

	// Load fog-of-war bitmaps
	printMessage( "Core", "Loading Fog-Of-War bitmaps...\n", WHITE );
	anim = (AnimationFactory*)GetResourceMgr()->GetFactoryResource("fogowar", IE_BAM_CLASS_ID);
	if (anim->GetCycleSize( 0 ) != 8) {
		// unknown type of fog anim
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}

	FogSprites[0] = NULL;
	FogSprites[1] = anim->GetFrame( 0, 0 );
	FogSprites[2] = anim->GetFrame( 1, 0 );
	FogSprites[3] = anim->GetFrame( 2, 0 );

	FogSprites[4] = video->MirrorSpriteVertical( FogSprites[1], false );

	FogSprites[5] = NULL;

	FogSprites[6] = video->MirrorSpriteVertical( FogSprites[3], false );

	FogSprites[7] = NULL;

	FogSprites[8] = video->MirrorSpriteHorizontal( FogSprites[2], false );

	FogSprites[9] = video->MirrorSpriteHorizontal( FogSprites[3], false );

	FogSprites[10] = NULL;
	FogSprites[11] = NULL;

	FogSprites[12] = video->MirrorSpriteHorizontal( FogSprites[6], false );

	FogSprites[16] = anim->GetFrame( 3, 0 );
	FogSprites[17] = anim->GetFrame( 4, 0 );
	FogSprites[18] = anim->GetFrame( 5, 0 );
	FogSprites[19] = anim->GetFrame( 6, 0 );

	FogSprites[20] = video->MirrorSpriteVertical( FogSprites[17], false );

	FogSprites[21] = NULL;

	FogSprites[23] = NULL;

	FogSprites[24] = video->MirrorSpriteHorizontal( FogSprites[18], false );

	FogSprites[25] = anim->GetFrame( 7, 0 );

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[25], false );
		FogSprites[22] = video->MirrorSpriteHorizontal( tmpsprite, false );
		video->FreeSprite( tmpsprite );
	}

	FogSprites[26] = NULL;
	FogSprites[27] = NULL;

	{
		Sprite2D *tmpsprite = video->MirrorSpriteVertical( FogSprites[19], false );
		FogSprites[28] = video->MirrorSpriteHorizontal( tmpsprite, false );
		video->FreeSprite( tmpsprite );
	}

	i = 0;
	vars->Lookup("3D Acceleration", i);
	if (i) {
		for(i=0;i<sizeof(FogSprites)/sizeof(Sprite2D *);i++ ) {
			if (FogSprites[i]) {
				Sprite2D* alphasprite = video->CreateAlpha( FogSprites[i] );
				video->FreeSprite ( FogSprites[i] );
				FogSprites[i] = alphasprite;
			}
		}
	}

	printStatus( "OK", LIGHT_GREEN );

	// Load ground circle bitmaps (PST only)
	printMessage( "Core", "Loading Ground circle bitmaps...\n", WHITE );
	//block required due to msvc6.0 incompatibility
	for (size = 0; size < MAX_CIRCLE_SIZE; size++) {
		if (GroundCircleBam[size][0]) {
			anim = (AnimationFactory*)GetResourceMgr()->GetFactoryResource(GroundCircleBam[size], IE_BAM_CLASS_ID);
			if (!anim || anim->GetCycleCount() != 6) {
				// unknown type of circle anim
				printStatus( "ERROR", LIGHT_RED );
				return GEM_ERROR;
			}
	
			for (int i = 0; i < 6; i++) {
				Sprite2D* sprite = anim->GetFrame( 0, (ieByte) i );
				if (GroundCircleScale[size]) {
					GroundCircles[size][i] = video->SpriteScaleDown( sprite, GroundCircleScale[size] );
					video->FreeSprite( sprite );
				} else {
					GroundCircles[size][i] = sprite;
				}
			}
		}
	}

	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Loading Fonts...\n", WHITE );
	table = LoadTable( "fonts" );
	if (table < 0) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find fonts.2da.\nTermination in Progress...\n" );
		return GEM_ERROR;	
	} else {
		AnimationMgr* bamint = ( AnimationMgr* ) GetInterface( IE_BAM_CLASS_ID );
		if (!bamint) {
			printf( "No BAM Importer Available.\nTermination in Progress...\n" );
			return GEM_ERROR;
		}
		DataStream* str = NULL;

		TableMgr* tab = GetTable( table );
		int count = tab->GetRowCount();
		for (int i = 0; i < count; i++) {
			const char* ResRef = tab->QueryField( i, 0 );
			int needpalette = atoi( tab->QueryField( i, 1 ) );
			int first_char = atoi( tab->QueryField( i, 2 ) );
			str = key->GetResource( ResRef, IE_BAM_CLASS_ID );
			if (!bamint->Open( str, true )) {
				continue;
			}
			Font* fnt = bamint->GetFont();
			if (!fnt) {
				continue;
			}
			strncpy( fnt->ResRef, ResRef, 8 );
			if (needpalette) {
		
				Color fore = {0xff, 0xff, 0xff, 0};
				Color back = {0x00, 0x00, 0x00, 0};
				if (!strnicmp( TooltipFont, ResRef, 8) ) {
					if (TooltipColor.a==0xff) {
						fore = TooltipColor;
					} else {
						fore = back;
						back = TooltipColor;
					}
				}
				Palette* pal = CreatePalette( fore, back );
				pal->CreateShadedAlphaChannel();
				fnt->SetPalette(pal);
				FreePalette( pal );
			}
			fnt->SetFirstChar( (ieByte) first_char );
			fonts.push_back( fnt );
		}
		DelTable( table );

		FreeInterface(bamint);
	}
	printMessage( "Core", "Fonts Loaded...", WHITE );
	printStatus( "OK", LIGHT_GREEN );

	if (TooltipBackResRef[0]) {
		anim = (AnimationFactory*)GetResourceMgr()->GetFactoryResource(TooltipBackResRef, IE_BAM_CLASS_ID);
		printMessage( "Core", "Initializing Tooltips...", WHITE );
		if (!anim) {
			printStatus( "ERROR", LIGHT_RED );
			return GEM_ERROR;
		}
		TooltipBack = new Sprite2D * [3];
		for (int i = 0; i < 3; i++) {
			TooltipBack[i] = anim->GetFrame( 0, (ieByte) i );
			TooltipBack[i]->XPos = 0;
			TooltipBack[i]->YPos = 0;
		}
		printStatus( "OK", LIGHT_GREEN );
	}

	return GEM_OK;
}

int Interface::Init()
{
	printMessage( "Core", "Initializing Variables Dictionary...", WHITE );
	vars = new Variables();
	if (!vars) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}

	vars->SetType( GEM_VARIABLES_INT );
	vars->SetAt( "Volume Ambients", 100 );
	vars->SetAt( "Volume Movie", 100 );
	vars->SetAt( "Volume Music", 100 );
	vars->SetAt( "Volume SFX", 100 );
	vars->SetAt( "Volume Voices", 100 );
	printStatus( "OK", LIGHT_GREEN );

	if (!LoadConfig()) {
		return GEM_ERROR;
	}
	printMessage( "Core", "Starting Plugin Manager...\n", WHITE );
	plugin = new PluginMgr( PluginsPath );
	if (plugin && plugin->GetPluginCount()) {
		printMessage( "Core", "Plugin Loading Complete...", WHITE );
		printStatus( "OK", LIGHT_GREEN );
	} else {
		printMessage( "Core", "Plugin Loading Failed, check path...", YELLOW);
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printMessage( "Core", "Creating Object Factory...", WHITE );
	factory = new Factory();
	printStatus( "OK", LIGHT_GREEN );
	time_t t;
	t = time( NULL );
	srand( ( unsigned int ) t );
#ifdef _DEBUG
	FileStreamPtrCount = 0;
	CachedFileStreamPtrCount = 0;
#endif
	printMessage( "Core", "GemRB Core Initialization...\n", WHITE );
	printMessage( "Core", "Searching for Video Driver...", WHITE );
	if (!IsAvailable( IE_VIDEO_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No Video Driver Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Video Plugin...", WHITE );
	video = ( Video * ) GetInterface( IE_VIDEO_CLASS_ID );
	if (video->Init() == GEM_ERROR) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot Initialize Video Driver.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	Color defcolor={255,255,255,200};
	SetInfoTextColor(defcolor);

	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Searching for KEY Importer...", WHITE );
	if (!IsAvailable( IE_KEY_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No KEY Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Resource Manager...\n", WHITE );
	key = ( ResourceMgr * ) GetInterface( IE_KEY_CLASS_ID );
	char ChitinPath[_MAX_PATH];
	PathJoin( ChitinPath, GamePath, "chitin.key", NULL );
	ResolveFilePath( ChitinPath );
	if (!key->LoadResFile( ChitinPath )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot Load Chitin.key\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printMessage( "Core", "Reading Game Options...\n", WHITE );
	if (!LoadGemRBINI())
	{
		printf( "Cannot Load INI\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	//loading baldur.ini
	{
		char ini_path[_MAX_PATH];
		PathJoin( ini_path, GamePath, INIConfig, NULL );
		ResolveFilePath( ini_path );
		LoadINI( ini_path );
		int i;
		for (i = 0; i < 8; i++) {
			if (INIConfig[i] == '.')
				break;
			GameNameResRef[i] = INIConfig[i];
		}
		GameNameResRef[i] = 0;
	}

	printMessage( "Core", "Creating Projectile Server...\n", WHITE );
	projserv = new ProjectileServer();
	if (!projserv->GetHighestProjectileNumber()) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No projectiles are available...\n" );
	}

	printMessage( "Core", "Checking for Dialogue Manager...\n", WHITE );
	if (!IsAvailable( IE_TLK_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No TLK Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings = ( StringMgr * ) GetInterface( IE_TLK_CLASS_ID );
	printMessage( "Core", "Loading Dialog.tlk file...", WHITE );
	char strpath[_MAX_PATH];
	PathJoin( strpath, GamePath, dialogtlk, NULL );
	ResolveFilePath( strpath );
	FileStream* fs = new FileStream();
	if (!fs->Open( strpath, true )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find Dialog.tlk.\nTermination in Progress...\n" );
		delete fs;
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings->Open( fs, true );

	printMessage( "Core", "Loading Palettes...\n", WHITE );
	DataStream* bmppal16 = NULL;
	DataStream* bmppal32 = NULL;
	DataStream* bmppal256 = NULL;
	if (!IsAvailable( IE_BMP_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No BMP Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	bmppal16 = key->GetResource( Palette16, IE_BMP_CLASS_ID );
	if (bmppal16) {
		pal16 = ( ImageMgr * ) GetInterface( IE_BMP_CLASS_ID );
		pal16->Open( bmppal16, true );
	} else {
		pal16 = NULL;
	}
	bmppal32 = key->GetResource( Palette32, IE_BMP_CLASS_ID );
	if (bmppal32) {
		pal32 = ( ImageMgr * ) GetInterface( IE_BMP_CLASS_ID );
		pal32->Open( bmppal32, true );
	} else {
		pal32 = NULL;
	}
	bmppal256 = key->GetResource( Palette256, IE_BMP_CLASS_ID );
	if (bmppal256) {
		pal256 = ( ImageMgr * ) GetInterface( IE_BMP_CLASS_ID );
		pal256->Open( bmppal256, true );
	} else {
		pal256 = NULL;
	}
	printMessage( "Core", "Palettes Loaded\n", WHITE );

	if (!IsAvailable( IE_BAM_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No BAM Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printMessage( "Core", "Initializing stock sounds...\n", WHITE );
	DSCount = ReadResRefTable ("defsound", DefSound);
	if (DSCount == 0) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find defsound.2da.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printMessage( "Core", "Initializing the Event Manager...", WHITE );
	evntmgr = new EventMgr();
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "BroadCasting Event Manager...", WHITE );
	video->SetEventMgr( evntmgr );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Window Manager...", WHITE );
	windowmgr = ( WindowMgr * ) GetInterface( IE_CHU_CLASS_ID );
	if (windowmgr == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing GUI Script Engine...", WHITE );
	guiscript = ( ScriptEngine * ) GetInterface( IE_GUI_SCRIPT_CLASS_ID );
	if (guiscript == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!guiscript->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strcpy( NextScript, "Start" );

	int ret = LoadSprites();
	if (ret) return ret;
	 
	printMessage( "Core", "Setting up the Console...\n", WHITE );
	QuitFlag = QF_CHANGESCRIPT;
	console = new Console();
	console->XPos = 0;
	console->YPos = (ieWord) (Height - 25);
	console->Width = (ieWord) Width;
	console->Height = 25;
	console->SetFont( fonts[0] );
	Sprite2D *tmpsprite = GetCursorSprite();
	if (!tmpsprite) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	console->SetCursor (tmpsprite);
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Starting up the Sound Manager...\n", WHITE );
	soundmgr = ( SoundMgr * ) GetInterface( IE_WAV_CLASS_ID );
	if (soundmgr == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!soundmgr->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Allocating SaveGameIterator...\n", WHITE );
	sgiterator = new SaveGameIterator();
	if (sgiterator == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	//no need of strdup, variables do copy the key!
	vars->SetAt( "SkipIntroVideos", (unsigned long)SkipIntroVideos );

	printMessage( "Core", "Initializing Token Dictionary...\n", WHITE );
	tokens = new Variables();
	if (!tokens) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	tokens->SetType( GEM_VARIABLES_STRING );
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Music Manager...\n", WHITE );
	music = ( MusicMgr * ) GetInterface( IE_MUS_CLASS_ID );
	if (!music) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	if (HasFeature( GF_RESDATA_INI )) {
		printMessage( "Core", "Loading resource data File...\n", WHITE );
		INIresdata = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs = new FileStream();
		char tINIresdata[_MAX_PATH];
		PathJoin( tINIresdata, GamePath, "resdata.ini", NULL );
		ResolveFilePath( tINIresdata );
		// FIXME: crashes if file does not open
		fs->Open( tINIresdata, true );
		if (!INIresdata->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}

	if (HasFeature( GF_HAS_PARTY_INI )) {
		printMessage( "Core", "Loading precreated teams setup...\n",
			WHITE );
		INIparty = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs = new FileStream();
		char tINIparty[_MAX_PATH];
		PathJoin( tINIparty, GamePath, "Party.ini", NULL );
		ResolveFilePath( tINIparty );
		fs->Open( tINIparty, true );
		if (!INIparty->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}
	if (HasFeature( GF_HAS_BEASTS_INI )) {
		printMessage( "Core", "Loading beasts definition File...\n",
			WHITE );
		INIbeasts = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs = new FileStream();
		char tINIbeasts[_MAX_PATH];
		PathJoin( tINIbeasts, GamePath, "beast.ini", NULL );
		ResolveFilePath( tINIbeasts );
		// FIXME: crashes if file does not open
		fs->Open( tINIbeasts, true );
		if (!INIbeasts->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}

		printMessage( "Core", "Loading quests definition File...\n",
			WHITE );
		INIquests = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs2 = new FileStream();
		char tINIquests[_MAX_PATH];
		PathJoin( tINIquests, GamePath, "quests.ini", NULL );
		ResolveFilePath( tINIquests );
		// FIXME: crashes if file does not open
		fs2->Open( tINIquests, true );
		if (!INIquests->Open( fs2, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}
	game = NULL;

	printMessage( "Core", "Bringing up the Global Timer...\n", WHITE );
	timer = new GlobalTimer();
	if (!timer) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing effect opcodes...\n", WHITE );
	opcodemgrs = GetInterfaceVector(IE_FX_CLASS_ID);
	if (!opcodemgrs || !opcodemgrs->size()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}

	printf("Loaded %d opcode blocks\n", (int) opcodemgrs->size());
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing effects...\n", WHITE );
	if (! Init_EffectQueue()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Inventory Management...\n", WHITE );
	ret = InitItemTypes();
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing string constants...\n", WHITE );
	ret = ReadStrrefs();
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing random treasure...\n", WHITE );
	ret = ReadRandomItems();
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	}
	else {
		printStatus( "ERROR", LIGHT_RED );
	}

	printMessage( "Core", "Initializing ability tables...\n", WHITE );
	ret = ReadAbilityTables();
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing area aliases...\n", WHITE );
	ret = ReadAreaAliasTable( "WMAPLAY" );
	if (ret) {
		printStatus( "OK", LIGHT_GREEN );
	}
	else {
		printStatus( "NOT FOUND", YELLOW );
	}

	printMessage( "Core", "Reading item tables...\n", WHITE);
	ret = ReadAuxItemTables();
	if (!ret) {
		printStatus( "ERROR", LIGHT_RED );
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Core Initialization Complete!\n", WHITE );

	return GEM_OK;
}

bool Interface::IsAvailable(SClass_ID filetype) const
{
	return plugin->IsAvailable( filetype );
}

WorldMap *Interface::GetWorldMap(const char *map)
{
	int index = worldmap->FindAndSetCurrentMap(map?map:game->CurrentArea);
	return worldmap->GetWorldMap(index);
}

void* Interface::GetInterface(SClass_ID filetype) const
{
	if (!plugin) {
		return NULL;
	}
	return plugin->GetPlugin( filetype );
}

std::vector<InterfaceElement>* Interface::GetInterfaceVector(SClass_ID filetype) const
{
	if (!plugin) {
		return NULL;
	}
	return plugin->GetAllPlugin( filetype );
}

ProjectileServer* Interface::GetProjectileServer() const
{
	return projserv;
}

Video* Interface::GetVideoDriver() const
{
	return video;
}

ResourceMgr* Interface::GetResourceMgr() const
{
	return key;
}

const char* Interface::TypeExt(SClass_ID type) const
{
	switch (type) {
		case IE_2DA_CLASS_ID:
			return ".2da";
	
		case IE_ACM_CLASS_ID:
			return ".acm";
	
		case IE_ARE_CLASS_ID:
			return ".are";
	
		case IE_BAM_CLASS_ID:
			return ".bam";
	
		case IE_BCS_CLASS_ID:
			return ".bcs";
	
		case IE_BS_CLASS_ID:
			return ".bs";

		case IE_BIF_CLASS_ID:
			return ".bif";
	
		case IE_BMP_CLASS_ID:
			return ".bmp";
	
		case IE_PNG_CLASS_ID:
			return ".png";
	
		case IE_CHR_CLASS_ID:
			return ".chr";
	
		case IE_CHU_CLASS_ID:
			return ".chu";
	
		case IE_CRE_CLASS_ID:
			return ".cre";
	
		case IE_DLG_CLASS_ID:
			return ".dlg";
	
		case IE_EFF_CLASS_ID:
			return ".eff";
	
		case IE_GAM_CLASS_ID:
			return ".gam";
	
		case IE_IDS_CLASS_ID:
			return ".ids";
	
		case IE_INI_CLASS_ID:
			return ".ini";
	
		case IE_ITM_CLASS_ID:
			return ".itm";
	
		case IE_KEY_CLASS_ID:
			return ".key";
	
		case IE_MOS_CLASS_ID:
			return ".mos";
	
		case IE_MUS_CLASS_ID:
			return ".mus";
	
		case IE_MVE_CLASS_ID:
			return ".mve";
	
		case IE_PLT_CLASS_ID:
			return ".plt";
	
		case IE_PRO_CLASS_ID:
			return ".pro";
	
		case IE_SAV_CLASS_ID:
			return ".sav";
	
		case IE_SPL_CLASS_ID:
			return ".spl";
	
		case IE_SRC_CLASS_ID:
			return ".src";
	
		case IE_STO_CLASS_ID:
			return ".sto";
	
		case IE_TIS_CLASS_ID:
			return ".tis";
	
		case IE_TLK_CLASS_ID:
			return ".tlk";
	
		case IE_TOH_CLASS_ID:
			return ".toh";
	
		case IE_TOT_CLASS_ID:
			return ".tot";
	
		case IE_VAR_CLASS_ID:
			return ".var";
	
		case IE_VVC_CLASS_ID:
			return ".vvc";
	
		case IE_WAV_CLASS_ID:
			return ".wav";
	
		case IE_WED_CLASS_ID:
			return ".wed";
	
		case IE_WFX_CLASS_ID:
			return ".wfx";
	
		case IE_WMP_CLASS_ID:
			return ".wmp";
	}
	return NULL;
}

void Interface::FreeString(char *&str)
{
	if (str) {
		strings->FreeString(str);
	}
	str = NULL;
}

char* Interface::GetString(ieStrRef strref, ieDword options)
{
	ieDword flags = 0;

	if (!(options & IE_STR_STRREFOFF)) {
		vars->Lookup( "Strref On", flags );
	}
	return strings->GetString( strref, flags | options );
}

void Interface::FreeInterface(void* ptr)
{
	plugin->FreePlugin( ptr );
}

Factory* Interface::GetFactory(void) const
{
	return factory;
}

void Interface::SetFeature(int flag, int position)
{
	if (position>=32) {
		position-=32;
		if (flag) {
			GameFeatures2 |= 1 << position;
		} else {
			GameFeatures2 &= ~( 1 << position );
		}
		return;
	}
	if (flag) {
		GameFeatures |= 1 << position;
	} else {
		GameFeatures &= ~( 1 << position );
	}
}

ieDword Interface::HasFeature(int position) const
{
	if (position>=32) {
		return GameFeatures2 & ( 1 << (position-32) );
	}
	return GameFeatures & ( 1 << position );
}

/** Search directories and load a config file */
bool Interface::LoadConfig(void)
{
#ifndef WIN32
	char path[_MAX_PATH];
	char name[_MAX_PATH];

	// Find directory where user stores GemRB configurations (~/.gemrb).
	// FIXME: Create it if it does not exist
	// Use current dir if $HOME is not defined (or bomb out??)

	char* s = getenv( "HOME" );
	if (s) {
		strcpy( UserDir, s );
		strcat( UserDir, "/."PACKAGE"/" );
	} else {
		strcpy( UserDir, "./" );
	}

	// Find basename of this program. It does the same as basename (3),
	// but that's probably missing on some archs
	s = strrchr( argv[0], PathDelimiter );
	if (s) {
		s++;
	} else {
		s = argv[0];
	}

	strcpy( name, s );
	//if (!name[0])		// FIXME: could this happen?
	//	strcpy (name, PACKAGE); // ugly hack

	// If we were called as $0 -c <filename>, load config from filename
	if (argc > 2 && ! strcmp("-c", argv[1])) {
		if (LoadConfig( argv[2] )) {
			return true;
		} else {
			// Explicitly specified cfg file HAS to be present
			return false;
		}
	}

	// FIXME: temporary hack, to be deleted??
	if (LoadConfig( "GemRB.cfg" )) {
		return true;
	}

	PathJoin( path, UserDir, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	// Don't try with default binary name if we have tried it already
	if (!strcmp( name, PACKAGE )) {
		return false;
	}

	PathJoin( path, UserDir, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	return false;
#else // WIN32
	strcpy( UserDir, ".\\" );
	return LoadConfig( "GemRB.cfg" );
#endif// WIN32
}

bool Interface::LoadConfig(const char* filename)
{
	FILE* config;

	printMessage("Config","Trying to open ", WHITE);
	textcolor(LIGHT_WHITE);
	printf("%s ", filename);
	config = fopen( filename, "rb" );
	if (config == NULL) {
		printStatus("NOT FOUND", LIGHT_RED);
		return false;
	}
	char name[65], value[_MAX_PATH + 3];

	//one GemRB own format is working well, this might be set to 0
	SaveAsOriginal = 1;

	while (!feof( config )) {
		char rem;
		fread( &rem, 1, 1, config );
		if (rem == '#') {
			fscanf( config, "%*[^\r\n]%*[\r\n]" );
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		fscanf( config, "%64[^=]=%[^\r\n]%*[\r\n]", name, value );
		if (stricmp( name, "Width" ) == 0) {
			Width = atoi( value );
		} else if (stricmp( name, "Height" ) == 0) {
			Height = atoi( value );
		} else if (stricmp( name, "Bpp" ) == 0) {
			Bpp = atoi( value );
		} else if (stricmp( name, "FullScreen" ) == 0) {
			FullScreen = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "SkipIntroVideos" ) == 0) {
			SkipIntroVideos = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "DrawFPS" ) == 0) {
			DrawFPS = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "EnableCheatKeys" ) == 0) {
			EnableCheatKeys ( atoi( value ) );
		} else if (stricmp( name, "FogOfWar" ) == 0) {
			FogOfWar = atoi( value );
		} else if (stricmp( name, "EndianSwitch" ) == 0) {
			DataStream::SetEndianSwitch(atoi(value) );
		} else if (stricmp( name, "CaseSensitive" ) == 0) {
			CaseSensitive = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "MultipleQuickSaves" ) == 0) {
			GameControl::MultipleQuickSaves(atoi(value));
		} else if (stricmp( name, "GameOnCD" ) == 0) {
			GameOnCD = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "GameDataPath" ) == 0) {
			strncpy( GameData, value, sizeof(GameData) );
		} else if (stricmp( name, "GameOverridePath" ) == 0) {
			strncpy( GameOverride, value, sizeof(GameOverride) );
		} else if (stricmp( name, "GameScriptsPath" ) == 0) {
			strncpy( GameScripts, value, sizeof(GameScripts) );
		} else if (stricmp( name, "GameSoundsPath" ) == 0) {
			strncpy( GameSounds, value, sizeof(GameSounds) );
		} else if (stricmp( name, "GamePortraitsPath" ) == 0) {
			strncpy( GamePortraits, value, sizeof(GamePortraits) );
		} else if (stricmp( name, "GameCharactersPath" ) == 0) {
			strncpy( GameCharacters, value, sizeof(GameCharacters) );
		} else if (stricmp( name, "GameName" ) == 0) {
			strncpy( GameName, value, sizeof(GameName) );
		} else if (stricmp( name, "GameType" ) == 0) {
			strncpy( GameType, value, sizeof(GameType) );
		} else if (stricmp( name, "SaveAsOriginal") == 0) {
			SaveAsOriginal = atoi(value);
		} else if (stricmp( name, "GemRBPath" ) == 0) {
			strcpy( GemRBPath, value );
		} else if (stricmp( name, "ScriptDebugMode" ) == 0) {
			SetScriptDebugMode(atoi(value));
		} else if (stricmp( name, "CachePath" ) == 0) {
			strncpy( CachePath, value, sizeof(CachePath) );
		} else if (stricmp( name, "GUIScriptsPath" ) == 0) {
			strncpy( GUIScriptsPath, value, sizeof(GUIScriptsPath) );
#ifndef WIN32
			ResolveFilePath( GUIScriptsPath );
#endif
		} else if (stricmp( name, "PluginsPath" ) == 0) {
			strncpy( PluginsPath, value, sizeof(PluginsPath) );
#ifndef WIN32
			ResolveFilePath( PluginsPath );
#endif
		} else if (stricmp( name, "GamePath" ) == 0) {
			strncpy( GamePath, value, sizeof(GamePath) );
#ifndef WIN32
			ResolveFilePath( GamePath );
#endif
		} else if (stricmp( name, "SavePath" ) == 0) {
			strncpy( SavePath, value, sizeof(SavePath) );
#ifndef WIN32
			ResolveFilePath( SavePath );
#endif
		} else if (stricmp( name, "CD1" ) == 0) {
			strncpy( CD1, value, sizeof(CD1) );
#ifndef WIN32
			ResolveFilePath( CD1 );
#endif
		} else if (stricmp( name, "CD2" ) == 0) {
			strncpy( CD2, value, sizeof(CD2) );
#ifndef WIN32
			ResolveFilePath( CD2 );
#endif
		} else if (stricmp( name, "CD3" ) == 0) {
			strncpy( CD3, value, sizeof(CD3) );
#ifndef WIN32
			ResolveFilePath( CD3 );
#endif
		} else if (stricmp( name, "CD4" ) == 0) {
			strncpy( CD4, value, sizeof(CD4) );
#ifndef WIN32
			ResolveFilePath( CD4 );
#endif
		} else if (stricmp( name, "CD5" ) == 0) {
			strncpy( CD5, value, sizeof(CD5) );
#ifndef WIN32
			ResolveFilePath( CD5 );
#endif
		} else if (stricmp( name, "CD6" ) == 0) {
			strncpy( CD6, value, sizeof(CD6) );
#ifndef WIN32
			ResolveFilePath( CD6 );
#endif
		}
	}
	fclose( config );


	if (!GameType[0]) {
		strcpy( GameType, "gemrb" );
	}

#ifdef DATADIR
	if (!GemRBPath[0]) {
		strcpy( GemRBPath, DATADIR );
	}
#endif

	if (!PluginsPath[0]) {
#ifdef PLUGINDIR
		strcpy( PluginsPath, PLUGINDIR );
#else
		PathJoin( PluginsPath, GemRBPath, "plugins", NULL );
#endif
	}

	if (!GUIScriptsPath[0]) {
		strcpy( GUIScriptsPath, GemRBPath );
	}

	if (!GameName[0]) {
		strcpy( GameName, GEMRB_STRING );
	}

	if (!SavePath[0]) {
		// FIXME: maybe should use UserDir instead of GamePath
		strcpy( SavePath, GamePath );
	}

	if (! CachePath[0]) {
		PathJoin( CachePath, UserDir, "Cache", NULL );
	}


	FixPath( GUIScriptsPath, true );
	FixPath( PluginsPath, true );
	FixPath( GemRBPath, true );

	FixPath( SavePath, false );
	mkdir( SavePath, S_IREAD|S_IWRITE|S_IEXEC );
	chmod( SavePath, S_IREAD|S_IWRITE|S_IEXEC );
	FixPath( SavePath, true );

	FixPath( CachePath, false );
	mkdir( CachePath, S_IREAD|S_IWRITE|S_IEXEC );
	chmod( CachePath, S_IREAD|S_IWRITE|S_IEXEC );

	printStatus( "OK", LIGHT_GREEN );
	if ( StupidityDetector( CachePath )) {
		printMessage("Core"," ",LIGHT_RED);
		printf( "Cache path %s doesn't exist, not a folder or contains alien files!\n", CachePath );
		return false;
	}
	DelTree((const char *) CachePath, false);
	FixPath( CachePath, true );

	return true;
}

static void upperlower(int upper, int lower)
{
	pl_uppercase[lower]=(ieByte) upper;
	pl_lowercase[upper]=(ieByte) lower;
}

static const char *game_flags[GF_COUNT+1]={
	"HasKaputz",          //0 GF_HAS_KAPUTZ
		"AllStringsTagged",   //1 GF_ALL_STRINGS_TAGGED
		"HasSongList",        //2 GF_HAS_SONGLIST
		"TeamMovement",       //3 GF_TEAM_MOVEMENT
		"UpperButtonText",    //4 GF_UPPER_BUTTON_TEXT
		"LowerLabelText",     //5 GF_LOWER_LABEL_TEXT
		"HasPartyIni",        //6 GF_HAS_PARTY_INI
		"SoundFolders",       //7 GF_SOUNDFOLDERS
		"IgnoreButtonFrames", //8 GF_IGNORE_BUTTON_FRAMES
		"OneByteAnimationID", //9 GF_ONE_BYTE_ANIMID
		"HasDPLAYER",         //10GF_HAS_DPLAYER
		"HasEXPTABLE",        //11GF_HAS_EXPTABLE
		"HasBeastsIni",       //12GF_HAS_BEASTS_INI
		"HasDescIcon",        //13GF_HAS_DESC_ICON
		"HasPickSound",       //14GF_HAS_PICK_SOUND
		"IWDMapDimensions",   //15GF_IWD_MAP_DIMENSIONS
		"AutomapIni",         //16GF_AUTOMAP_INI
		"SmallFog",           //17GF_SMALL_FOG
		"ReverseDoor",        //18GF_REVERSE_DOOR
		"ProtagonistTalks",   //19GF_PROTAGONIST_TALKS
		"HasSpellList",       //20GF_HAS_SPELLLIST
		"IWD2ScriptName",     //21GF_IWD2_SCRIPTNAME
		"DialogueScrolls",    //22GF_DIALOGUE_SCROLLS
		"KnowWorld",          //23GF_KNOW_WORLD
		"ReverseToHit",       //24GF_REVERSE_TOHIT
		"SaveForHalfDamage",  //25GF_SAVE_FOR_HALF
		"CharNameIsGabber",   //26GF_CHARNAMEISGABBER
		"MagicBit",           //27GF_MAGICBIT
		"CheckAbilities",     //28GF_CHECK_ABILITIES
		"ChallengeRating",    //29GF_CHALLENGERATING
		"SpellBookIconHack",  //30GF_SPELLBOOKICONHACK
		"EnhancedEffects",    //31GF_ENHANCED_EFFECTS
		"DeathOnZeroStat",    //32GF_DEATH_ON_ZERO_STAT
		"SpawnIni",           //33GF_SPAWN_INI
		"IWDDeathVarFormat",  //34GF_IWD_DEATHVARFORMAT
		"HasResDataIni",      //35GF_RESDATA_INI
		NULL                  //for our own safety, this marks the end of the pole
};

/** Loads gemrb.ini */
bool Interface::LoadGemRBINI()
{
	DataStream* inifile = key->GetResource( "gemrb", IE_INI_CLASS_ID );
	if (! inifile) {
		printStatus( "ERROR", LIGHT_RED );
		return false;
	}

	printMessage( "Core", "Loading game type-specific GemRB setup...\n", WHITE );
	printf( "%s",inifile->originalfile);

	if (!IsAvailable( IE_INI_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "[Core]: No INI Importer Available.\n" );
		return false;
	}
	DataFileMgr* ini = ( DataFileMgr* ) GetInterface( IE_INI_CLASS_ID );
	ini->Open( inifile, true ); //autofree

	printStatus( "OK", LIGHT_GREEN );

	const char *s;

	// Resrefs are already initialized in Interface::Interface()
	s = ini->GetKeyAsString( "resources", "CursorBAM", NULL );
	if (s)
		strcpy( CursorBam, s );

	s = ini->GetKeyAsString( "resources", "ScrollCursorBAM", NULL );
	if (s)
		strcpy( ScrollCursorBam, s );

	s = ini->GetKeyAsString( "resources", "ButtonFont", NULL );
	if (s)
		strcpy( ButtonFont, s );

	s = ini->GetKeyAsString( "resources", "TooltipFont", NULL );
	if (s)
		strcpy( TooltipFont, s );

	s = ini->GetKeyAsString( "resources", "MovieFont", NULL );
	if (s)
		strcpy( MovieFont, s );

	s = ini->GetKeyAsString( "resources", "TooltipBack", NULL );
	if (s)
		strcpy( TooltipBackResRef, s );

	s = ini->GetKeyAsString( "resources", "TooltipColor", NULL );
	if (s) {
		if (s[0] == '#') {
			unsigned long c = strtoul (s + 1, NULL, 16);
			// FIXME: check errno
			TooltipColor.r = (unsigned char) (c >> 24);
			TooltipColor.g = (unsigned char) (c >> 16);
			TooltipColor.b = (unsigned char) (c >> 8);
			TooltipColor.a = (unsigned char) (c);
		}
	}

	//which stat determines the fist weapon (defaults to class)
	Actor::SetFistStat(ini->GetKeyAsInt( "resources", "FistStat", IE_CLASS));

	TooltipMargin = ini->GetKeyAsInt( "resources", "TooltipMargin", TooltipMargin );

	// The format of GroundCircle can be:
	// GroundCircleBAM1 = wmpickl/3
	// to denote that the bitmap should be scaled down 3x
	for (int size = 0; size < MAX_CIRCLE_SIZE; size++) {
		char name[30];
		sprintf( name, "GroundCircleBAM%d", size+1 );
		s = ini->GetKeyAsString( "resources", name, NULL );
		if (s) {
			const char *pos = strchr( s, '/' );
			if (pos) {
				GroundCircleScale[size] = atoi( pos+1 );
				strncpy( GroundCircleBam[size], s, pos - s );
				GroundCircleBam[size][pos - s] = '\0';
			} else {
				strcpy( GroundCircleBam[size], s );
			}
		}
	}

	s = ini->GetKeyAsString( "resources", "INIConfig", NULL );
	if (s)
		strcpy( INIConfig, s );

	s = ini->GetKeyAsString( "resources", "Palette16", NULL );
	if (s)
		strcpy( Palette16, s );

	s = ini->GetKeyAsString( "resources", "Palette32", NULL );
	if (s)
		strcpy( Palette32, s );

	s = ini->GetKeyAsString( "resources", "Palette256", NULL );
	if (s)
		strcpy( Palette256, s );

	unsigned int i;
	for(i=0;i<256;i++) {
		pl_uppercase[i]=(ieByte) toupper(i);
		pl_lowercase[i]=(ieByte) tolower(i);
	}

	i = (unsigned int) ini->GetKeyAsInt ("charset", "CharCount", 0);
	if (i>99) i=99;
	while(i--) {
		char key[10];
		snprintf(key,9,"Letter%d", i+1);
		s = ini->GetKeyAsString( "charset", key, NULL );
		if (s) {
			const char *s2 = strchr(s,',');
			if (s2) {
				upperlower(atoi(s), atoi(s2+1) );
			}
		}
	}

	MaximumAbility = ini->GetKeyAsInt ("resources", "MaximumAbility", 25 );

	RedrawTile = ini->GetKeyAsInt( "resources", "RedrawTile", 0 )!=0;

	for (i=0;i<GF_COUNT;i++) {
		if (!game_flags[i]) {
			printf("Fix the game flags!\n");
			abort();
		}
		SetFeature( ini->GetKeyAsInt( "resources", game_flags[i], 0 ), i );
		printMessage("Option", "", GREEN);
		printf("%s = %s\n", game_flags[i], HasFeature(i)?"yes":"no");
	}

	ForceStereo = ini->GetKeyAsInt( "resources", "ForceStereo", 0 );

	FreeInterface( ini );
	return true;
}

Palette *Interface::GetPalette(const ieResRef resname)
{
	Palette *palette = (Palette *) PaletteCache.GetResource(resname);
	if (palette) {
		return palette;
	}
	//additional hack for allowing NULL's
	if (PaletteCache.RefCount(resname)!=-1) {
		return NULL;
	}
	DataStream* str = key->GetResource( resname, IE_BMP_CLASS_ID );
	ImageMgr* im = ( ImageMgr* ) GetInterface( IE_BMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}
	if (!im->Open( str, true )) {
		PaletteCache.SetAt(resname, NULL);
		FreeInterface( im );
		return NULL;
	}

	palette = new Palette();
	im->GetPalette(0,256,palette->col);
	FreeInterface( im );
	palette->named=true;
	PaletteCache.SetAt(resname, (void *) palette);
	return palette;
}

Palette* Interface::CreatePalette(const Color &color, const Color &back)
{
	Palette* pal = new Palette();
	pal->col[0].r = 0;
	pal->col[0].g = 0xff;
	pal->col[0].b = 0;
	pal->col[0].a = 0;
	for (int i = 1; i < 256; i++) {
		pal->col[i].r = back.r +
			( unsigned char ) ( ( ( color.r - back.r ) * ( i ) ) / 255.0 );
		pal->col[i].g = back.g +
			( unsigned char ) ( ( ( color.g - back.g ) * ( i ) ) / 255.0 );
		pal->col[i].b = back.b +
			( unsigned char ) ( ( ( color.b - back.b ) * ( i ) ) / 255.0 );
		pal->col[i].a = back.a +
			( unsigned char ) ( ( ( color.a - back.a ) * ( i ) ) / 255.0 );
	}
	return pal;
}

void Interface::FreePalette(Palette *&pal, const ieResRef name)
{
	int res;

	if (!pal) {
		return;
	}
	if (!name || !name[0]) {
		if(pal->named) {
			printf("Palette is supposed to be named, but got no name!\n");
			abort();
		} else {
			pal->Release();
			pal=NULL;
		}
		return;
	}
	if (!pal->named) {
		printf("Unnamed palette, it should be %s!\n", name);
		abort();
	}
	res=PaletteCache.DecRef((void *) pal, name, true);
	if (res<0) {
		printMessage( "Core", "Corrupted Palette cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Palette name is: %.8s\n", name);
		abort();
	}
	if (!res) {
		pal->Release();
	}
	pal = NULL;
}

/** No descriptions */
Color* Interface::GetPalette(int index, int colors, Color *pal) const
{
	//Color* pal = NULL;
	if (colors == 32) {
		//pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal32->GetPalette( index, colors, pal );
	} else if (colors <= 32) {
		//pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal16->GetPalette( index, colors, pal );
	} else if (colors == 256) {
		//pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal256->GetPalette( index, colors, pal );
	}
	return pal;
}
/** Returns a preloaded Font */
Font* Interface::GetFont(const char *ResRef) const
{
	for (unsigned int i = 0; i < fonts.size(); i++) {
		if (strnicmp( fonts[i]->ResRef, ResRef, 8 ) == 0) {
			return fonts[i];
		}
	}
	return NULL;
}

Font* Interface::GetFont(unsigned int index) const
{
	if (index >= fonts.size()) {
		return NULL;
	}
	return fonts[index];
}

Font* Interface::GetButtonFont() const
{
	return GetFont( ButtonFont );
}

/** Returns the Event Manager */
EventMgr* Interface::GetEventMgr() const
{
	return evntmgr;
}

/** Returns the Window Manager */
WindowMgr* Interface::GetWindowMgr() const
{
	return windowmgr;
}

/** Get GUI Script Manager */
ScriptEngine* Interface::GetGUIScriptEngine() const
{
	return guiscript;
}

Actor *Interface::GetCreature(DataStream *stream, unsigned char InParty)
{
	ActorMgr* actormgr = ( ActorMgr* ) GetInterface( IE_CRE_CLASS_ID );
	if (!actormgr->Open( stream, true )) {
		FreeInterface( actormgr );
		return NULL;
	}
	Actor* actor = actormgr->GetActor(InParty);
	FreeInterface( actormgr );
	return actor;
}

int Interface::LoadCreature(const char* ResRef, int InParty, bool character)
{
	DataStream *stream;

	if (character) {
		char nPath[_MAX_PATH], fName[16];
		snprintf( fName, sizeof(fName), "%s.chr", ResRef);
		PathJoin( nPath, GamePath, "characters", fName, NULL );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream *fs = new FileStream();
		fs -> Open( nPath, true );
		stream = (DataStream *) fs;
	}
	else {
		stream = key->GetResource( ResRef, IE_CRE_CLASS_ID );
	}
	Actor* actor = GetCreature(stream, InParty);
	if ( !actor ) {
		return -1;
	}
	//actor->InParty = (ieByte) InParty;
	//both fields are of length 9, make this sure!
	memcpy(actor->Area, game->CurrentArea, sizeof(actor->Area) );
	if (actor->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		actor->SetStance( IE_ANI_TWITCH );
	} else {
		actor->SetStance( IE_ANI_AWAKE );
	}
	actor->SetOrientation( 0, false );

	if ( InParty ) {
		return game->JoinParty( actor, JP_JOIN|JP_INITPOS );
	}
	else {
		return game->AddNPC( actor );
	}
}

//NOTE: if there were more summoned creatures, it will return only the last
Actor *Interface::SummonCreature(const ieResRef resource, const ieResRef vvcres, Actor *Owner, Actor *target, Point &position, int eamod, int level)
{
	level = level * 100;
	//maximum number of monsters summoned
	int cnt=10;
	Actor * ab = NULL;

	while(cnt-- && level>0) {
		DataStream* ds = key->GetResource( resource, IE_CRE_CLASS_ID );
		ab = GetCreature(ds);
		if (!ab) {
			return NULL;
		}

		ab->LastSummoner = Owner->GetID();
		level -= ab->GetStat(IE_XP);

		int enemyally;

		if (eamod==EAM_SOURCEALLY || eamod==EAM_SOURCEENEMY) {
			enemyally = Owner->GetStat(IE_EA)>EA_GOODCUTOFF;
		} else {
			if (target) {
				enemyally = target->GetStat(IE_EA)>EA_GOODCUTOFF;
			} else {
				enemyally = true;
			}
		}

		switch (eamod) {
		case EAM_SOURCEALLY:
		case EAM_ALLY:
			if (enemyally) {
			ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_ALLY); //is this the summoned EA?
			}
			break;
		case EAM_SOURCEENEMY:
		case EAM_ENEMY:
			if (enemyally) {
				ab->SetBase(IE_EA, EA_ALLY); //is this the summoned EA?
			} else {
				ab->SetBase(IE_EA, EA_ENEMY); //is this the summoned EA?
			}
			break;
		case EAM_NEUTRAL:
			ab->SetBase(IE_EA, EA_NEUTRAL);
			break;
		default:
			break;
		}

		Map *map = target->GetCurrentArea();
		map->AddActor(ab);
		ab->SetPosition(position, true, 0);
		ab->RefreshEffects(NULL);
		if (vvcres[0]) {
			ScriptedAnimation* vvc = GetScriptedAnimation(vvcres, false);
			if (vvc) {
				vvc->XPos=position.x;
				vvc->YPos=position.y;
				map->AddVVCell( vvc );
			}
		}
	}
	return ab;
}

void Interface::RedrawControls(const char *varname, unsigned int value)
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->RedrawControls(varname, value);
		}
	}
}

void Interface::RedrawAll()
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win != NULL && win->Visible!=WINDOW_INVALID) {
			win->Invalidate();
		}
	}
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool Interface::LoadWindowPack(const char* name)
{
	DataStream* stream = key->GetResource( name, IE_CHU_CLASS_ID );
	if (stream == NULL) {
		printMessage( "Interface", "Error: Cannot find ", LIGHT_RED );
		printf( "%s.chu\n", name );
		return false;
	}
	if (!GetWindowMgr()->Open( stream, true )) {
		printMessage( "Interface", "Error: Cannot Load ", LIGHT_RED );
		printf( "%s.chu\n", name );
		return false;
	}

	strncpy( WindowPack, name, sizeof( WindowPack ) );
	WindowPack[sizeof( WindowPack ) - 1] = '\0';

	return true;
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
	unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		Window *win = windows[i];
		if (win == NULL)
			continue;
		if (win->Visible==WINDOW_INVALID) {
			continue;
		}
		if (win->WindowID == WindowID &&
			!strnicmp( WindowPack, win->WindowPack, sizeof(WindowPack) )) {
			SetOnTop( i );
			win->Invalidate();
			return i;
		}
	}
	Window* win = windowmgr->GetWindow( WindowID );
	if (win == NULL) {
		return -1;
	}
	memcpy( win->WindowPack, WindowPack, sizeof(WindowPack) );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}
// FIXME: it's a clone of LoadWindow
/** Creates a Window in the Window Manager */
int Interface::CreateWindow(unsigned short WindowID, int XPos, int YPos, unsigned int Width, unsigned int Height, char* Background)
{
	unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL)
			continue;
		if (windows[i]->WindowID == WindowID && !stricmp( WindowPack,
			windows[i]->WindowPack )) {
			SetOnTop( i );
			windows[i]->Invalidate();
			return i;
		}
	}

	Window* win = new Window( WindowID, (ieWord) XPos, (ieWord) YPos, (ieWord) Width, (ieWord) Height );
	if (Background[0]) {
		if (IsAvailable( IE_MOS_CLASS_ID )) {
			DataStream* bkgr = key->GetResource( Background,
				IE_MOS_CLASS_ID );
			if (bkgr != NULL) {
				ImageMgr* mos = ( ImageMgr* )
					GetInterface( IE_MOS_CLASS_ID );
				mos->Open( bkgr, true );
				win->SetBackGround( mos->GetImage(), true );
				FreeInterface( mos );
			} else
				printf( "[Core]: Cannot Load BackGround, skipping\n" );
		} else
			printf( "[Core]: No MOS Importer Available, skipping background\n" );
	}

	strcpy( win->WindowPack, WindowPack );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}

/** Sets a Window on the Top */
void Interface::SetOnTop(int Index)
{
	std::vector<int>::iterator t;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		if((*t) == Index) {
			topwin.erase(t);
			break;
		}
	}
	if(topwin.size() != 0)
		topwin.insert(topwin.begin(), Index);
	else
		topwin.push_back(Index);
}
/** Add a window to the Window List */
void Interface::AddWindow(Window * win)
{
	int slot = -1;
	for(unsigned int i = 0; i < windows.size(); i++) {
		Window *w = windows[i];

		if(w==NULL) {
			slot = i;
			break;
		}
	}
	if(slot == -1) {
		windows.push_back(win);
		slot=(int)windows.size()-1;
	}
	else
		windows[slot] = win;
	win->Invalidate();
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned long ControlID) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	int i = 0;
	while (true) {
		Control* ctrl = win->GetControl( (unsigned short) i );
		if (ctrl == NULL)
			return -1;
		if (ctrl->ControlID == ControlID)
			return i;
		i++;
	}
}
/** Adjust the Scrolling factor of a control (worldmap atm) */
int Interface::AdjustScrolling(unsigned short WindowIndex,
		unsigned short ControlIndex, short x, short y)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	switch(ctrl->ControlType) {
		case IE_GUI_WORLDMAP:
			((WorldMapControl *) ctrl)->AdjustScrolling(x,y);
			break;
		default: //doesn't work for these
			return -1;
	}
	return 0;
}

/** Set the Text of a Control */
int Interface::SetText(unsigned short WindowIndex,
		unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetText( string );
}
/** Set the Tooltip text of a Control */
int Interface::SetTooltip(unsigned short WindowIndex,
		unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetTooltip( string );
}

void Interface::DisplayTooltip(int x, int y, Control *ctrl)
{
	tooltip_x = x;
	tooltip_y = y;
	tooltip_ctrl = ctrl;
}

int Interface::GetVisible(unsigned short WindowIndex) const
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	return win->Visible;
}
/** Set a Window Visible Flag */
int Interface::SetVisible(unsigned short WindowIndex, int visible)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	if (visible!=WINDOW_FRONT) {
		win->Visible = (char) visible;
	}
	switch (visible) {
		case WINDOW_GRAYED:
			win->Invalidate();
			//here is a fallthrough
		case WINDOW_INVISIBLE:
			//hiding the viewport if the gamecontrol window was made invisible
			if (win->WindowID==65535) {
				video->SetViewport( 0,0,0,0 );
			}
			evntmgr->DelWindow( win );
			break;
	
		case WINDOW_VISIBLE:
			if (win->WindowID==65535) {
				video->SetViewport( win->XPos, win->YPos, win->Width, win->Height);
			}
			//here is a fallthrough
		case WINDOW_FRONT:
			if (win->Visible==WINDOW_VISIBLE) {
				evntmgr->AddWindow( win );
			}
			win->Invalidate();
			SetOnTop( WindowIndex );
			break;
	}
	return 0;
}


/** Set the Status of a Control in a Window */
int Interface::SetControlStatus(unsigned short WindowIndex,
		unsigned short ControlIndex, unsigned long Status)
{
	//don't set the status of an already invalidated window
	Window* win = GetWindow(WindowIndex);
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	if (Status&IE_GUI_CONTROL_FOCUSED) {
		evntmgr->SetFocused( win, ctrl);
	}
	if (ctrl->ControlType != ((Status >> 24) & 0xff) ) {
		return -2;
	}
	switch (ctrl->ControlType) {
		case IE_GUI_BUTTON:
			//Button
			{
				Button* btn = ( Button* ) ctrl;
				btn->SetState( ( unsigned char ) ( Status & 0x7f ) );
			}
			break;
		default:
			ctrl->Value = Status & 0x7f;
			break;
	}
	return 0;
}

/** Show a Window in Modal Mode */
int Interface::ShowModal(unsigned short WindowIndex, int Shadow)
{
	if (WindowIndex >= windows.size()) {
		printMessage( "Core", "Window not found", LIGHT_RED );
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		printMessage( "Core", "Window already freed", LIGHT_RED );
		return -1;
	}
	win->Visible = WINDOW_FRONT;
	//don't destroy the other window handlers
	//evntmgr->Clear();
	SetOnTop( WindowIndex );
	evntmgr->AddWindow( win );

	ModalWindow = NULL;
	DrawWindows();
	win->Invalidate();

	Color gray = {
		0, 0, 0, 128
	};
	Color black = {
		0, 0, 0, 255
	};

	Region r( 0, 0, Width, Height );

	if (Shadow == MODAL_SHADOW_GRAY) {
		video->DrawRect( r, gray );
	} else if (Shadow == MODAL_SHADOW_BLACK) {
		video->DrawRect( r, black );
	}

	ModalWindow = win;
	return 0;
}

void Interface::DrawWindows(void)
{
	GameControl *gc = GetGameControl();
	if (!gc) {
		GSUpdate(false);
	}
	else {
		//this variable is used all over in the following hacks
		int flg = gc->GetDialogueFlags();
		GSUpdate(!(flg & DF_FREEZE_SCRIPTS) );

		//the following part is a series of hardcoded gui behaviour

		//updating panes according to the saved game
		//pst requires this before initiating dialogs because it has
		//no dialog window by default

		//initiating dialog
		if (flg & DF_IN_DIALOG) {
			// -3 noaction
			// -2 close
			// -1 open
			// choose option
			ieDword var = (ieDword) -3;
			vars->Lookup("DialogChoose", var);
			if ((int) var == -2) {
				gc->EndDialog();
			} else if ( (int)var !=-3) {
				gc->DialogChoose(var);
				vars->SetAt("DialogChoose", (ieDword) -3);
			}
			if (flg & DF_OPENCONTINUEWINDOW) {
				guiscript->RunFunction( "OpenContinueMessageWindow" );
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			} else if (flg & DF_OPENENDWINDOW) {
				guiscript->RunFunction( "OpenEndMessageWindow" );
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW|DF_OPENENDWINDOW, BM_NAND);
			}
		}

		//handling container
		if (CurrentContainer && UseContainer) {
			if (!(flg & DF_IN_CONTAINER) ) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_OR);
				guiscript->RunFunction( "OpenContainerWindow" );
			}
		} else {
			if (flg & DF_IN_CONTAINER) {
				gc->SetDialogueFlags(DF_IN_CONTAINER, BM_NAND);
				guiscript->RunFunction( "CloseContainerWindow" );
			}
		}
		//end of gui hacks
	}

	//here comes the REAL drawing of windows
	if (ModalWindow) {
		ModalWindow->DrawWindow();
		return;
	}
	size_t i = topwin.size();
	while(i--) {
		unsigned int t = topwin[i];

		if ( t >=windows.size() )
			continue;

		//visible ==1 or 2 will be drawn
		Window* win = windows[t];
		if (win != NULL) {
			if (win->Visible == WINDOW_INVALID) {
				topwin.erase(topwin.begin()+i);
				evntmgr->DelWindow( win );
				delete win;
				windows[t]=NULL;
			} else if (win->Visible) {
				win->DrawWindow();
			}
		}
	}
}

void Interface::DrawTooltip ()
{
	if (! tooltip_ctrl || !tooltip_ctrl->Tooltip)
		return;

	Font* fnt = GetFont( TooltipFont );
	char *tooltip_text = tooltip_ctrl->Tooltip;

	int w1 = 0;
	int w2 = 0;
	int w = fnt->CalcStringWidth( tooltip_text ) + 8;
	int h = fnt->maxHeight;

	if (TooltipBack) {
		h = TooltipBack[0]->Height;
		w1 = TooltipBack[1]->Width;
		w2 = TooltipBack[2]->Width;
		w += TooltipMargin*2;
		//multiline in case of too much text
		if (w>TooltipBack[0]->Width)
			w=TooltipBack[0]->Width;
	}

	int x = tooltip_x - w / 2;
	int y = tooltip_y - h / 2;

	// Ensure placement within the screen
	if (x < 0) x = 0;
	else if (x + w + w1 + w2 > Width)
		x = Width - w - w1 - w2;
	if (y < 0) y = 0;
	else if (y + h > Height)
		y = Height - h;


	// FIXME: add tooltip scroll animation for bg. also, take back[0] from
	// center, not from left end
	Region r2 = Region( x, y, w, h );
	if (TooltipBack) {
		video->BlitSprite( TooltipBack[0], x + TooltipMargin, y, true, &r2 );
		video->BlitSprite( TooltipBack[1], x, y, true );
		video->BlitSprite( TooltipBack[2], x + w, y, true );
	}

	r2.x+=TooltipMargin;
	fnt->Print( r2, (ieByte *) tooltip_text, NULL,
		IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, true );
}

//interface for higher level functions, if the window was
//marked for deletion it is not returned
Window* Interface::GetWindow(unsigned short WindowIndex) const
{
	if (WindowIndex < windows.size()) {
		Window *win = windows[WindowIndex];
		if (win && (win->Visible!=WINDOW_INVALID) ) {
			return win;
		}
	}
	return NULL;
}

// this function will determine if wnd is a valid window pointer
// by checking if its WindowID is the same as the reference
bool Interface::IsValidWindow(unsigned short WindowID, Window *wnd) const
{
	size_t WindowIndex = windows.size();
	while (WindowIndex--) {
		if (windows[WindowIndex] == wnd) {
			return wnd->WindowID == WindowID;
		}
	}
	return false;
}

//this function won't delete the window, just mark it for deletion
//it will be deleted in the next DrawWindows cycle
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
int Interface::DelWindow(unsigned short WindowIndex)
{
	if (WindowIndex == 0xffff) {
		//we clear ALL windows immediately, don't call this
		//from a guiscript
		vars->SetAt("MessageWindow", (ieDword) ~0);
		vars->SetAt("OptionsWindow", (ieDword) ~0);
		vars->SetAt("PortraitWindow", (ieDword) ~0);
		vars->SetAt("ActionsWindow", (ieDword) ~0);
		vars->SetAt("TopWindow", (ieDword) ~0);
		vars->SetAt("OtherWindow", (ieDword) ~0);
		vars->SetAt("FloatWindow", (ieDword) ~0);
		for(unsigned int WindowIndex=0; WindowIndex<windows.size();WindowIndex++) {
			Window* win = windows[WindowIndex];
			delete win;
		}
		windows.clear();
		topwin.clear();
		evntmgr->Clear();
		ModalWindow = NULL;
		return 0;
	}
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if ((win == NULL) || (win->Visible==WINDOW_INVALID) ) {
		printMessage( "Core", "Window deleted again", LIGHT_RED );
		return -1;
	}
	if (win == ModalWindow) {
		ModalWindow = NULL;
		RedrawAll(); //marking windows for redraw
	}
	evntmgr->DelWindow( win );
	win->release();
	//re-capturing new (old) modal window if any
	size_t tw = topwin.size();
	for(size_t i=0;i<tw;i++) {
		Window *tmp = windows[topwin[i]];
		if (tmp->Visible==WINDOW_FRONT) {
			ModalWindow = tmp;
			break;
		}
	}
	return 0;
}

/** Popup the Console */
void Interface::PopupConsole()
{
	ConsolePopped = !ConsolePopped;
	RedrawAll();
	console->Changed = true;
}

/** Draws the Console */
void Interface::DrawConsole()
{
	console->Draw( 0, 0 );
}

/** Get the Sound Manager */
SoundMgr* Interface::GetSoundMgr() const
{
	return soundmgr;
}
/** Get the Sound Manager */
SaveGameIterator* Interface::GetSaveGameIterator() const
{
	return sgiterator;
}
/** Sends a termination signal to the Video Driver */
bool Interface::Quit(void)
{
	return video->Quit();
}
/** Returns the variables dictionary */
Variables* Interface::GetDictionary() const
{
	return vars;
}
/** Returns the token dictionary */
Variables* Interface::GetTokenDictionary() const
{
	return tokens;
}
/** Get the Music Manager */
MusicMgr* Interface::GetMusicMgr() const
{
	return music;
}
/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
int Interface::LoadTable(const ieResRef ResRef)
{
	int ind = GetTableIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	//printf("(%s) Table not found... Loading from file\n", ResRef);
	DataStream* str = key->GetResource( ResRef, IE_2DA_CLASS_ID );
	if (!str) {
		return -1;
	}
	TableMgr* tm = ( TableMgr* ) GetInterface( IE_2DA_CLASS_ID );
	if (!tm) {
		delete str;
		return -1;
	}
	if (!tm->Open( str, true )) {
		FreeInterface( tm );
		return -1;
	}
	Table t;
	t.free = false;
	strncpy( t.ResRef, ResRef, 8 );
	t.tm = tm;
	ind = -1;
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].free) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		tables[ind] = t;
		return ind;
	}
	tables.push_back( t );
	return ( int ) tables.size() - 1;
}
/** Gets the index of a loaded table, returns -1 on error */
int Interface::GetTableIndex(const char* ResRef) const
{
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].free)
			continue;
		if (strnicmp( tables[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Table by its index, returns NULL on error */
TableMgr* Interface::GetTable(unsigned int index) const
{
	if (index >= tables.size()) {
		return NULL;
	}
	if (tables[index].free) {
		return NULL;
	}
	return tables[index].tm;
}
/** Frees a Loaded Table, returns false on error, true on success */
bool Interface::DelTable(unsigned int index)
{
	if (index==0xffffffff) {
		FreeInterfaceVector( Table, tables, tm );
		tables.clear();
		return true;
	}
	if (index >= tables.size()) {
		return false;
	}
	if (tables[index].free) {
		return false;
	}
	FreeInterface( tables[index].tm );
	tables[index].free = true;
	return true;
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const char* ResRef)
{
	int ind = GetSymbolIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	DataStream* str = key->GetResource( ResRef, IE_IDS_CLASS_ID );
	if (!str) {
		return -1;
	}
	SymbolMgr* sm = ( SymbolMgr* ) GetInterface( IE_IDS_CLASS_ID );
	if (!sm) {
		delete str;
		return -1;
	}
	if (!sm->Open( str, true )) {
		FreeInterface( sm );
		return -1;
	}
	Symbol s;
	s.free = false;
	strncpy( s.ResRef, ResRef, 8 );
	s.sm = sm;
	ind = -1;
	for (size_t i = 0; i < symbols.size(); i++) {
		if (symbols[i].free) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		symbols[ind] = s;
		return ind;
	}
	symbols.push_back( s );
	return ( int ) symbols.size() - 1;
}
/** Gets the index of a loaded Symbol Table, returns -1 on error */
int Interface::GetSymbolIndex(const char* ResRef) const
{
	for (size_t i = 0; i < symbols.size(); i++) {
		if (symbols[i].free)
			continue;
		if (strnicmp( symbols[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
SymbolMgr* Interface::GetSymbol(unsigned int index) const
{
	if (index >= symbols.size()) {
		return NULL;
	}
	if (symbols[index].free) {
		return NULL;
	}
	return symbols[index].sm;
}
/** Frees a Loaded Symbol Table, returns false on error, true on success */
bool Interface::DelSymbol(unsigned int index)
{
	if (index >= symbols.size()) {
		return false;
	}
	if (symbols[index].free) {
		return false;
	}
	FreeInterface( symbols[index].sm );
	symbols[index].free = true;
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(const char* ResRef)
{
	MoviePlayer* mp = ( MoviePlayer* ) GetInterface( IE_MVE_CLASS_ID );
	if (!mp) {
		return 0;
	}
	DataStream* str = key->GetResource( ResRef, IE_MVE_CLASS_ID );
	if (!str) {
		FreeInterface( mp );
		return -1;
	}
	if (!mp->Open( str, true )) {
		FreeInterface( mp );
		// since mp was opened with autofree, this delete would cause double free
		//	delete str;
		return -1;
	}

	ieDword subtitles = 0;
	Font *SubtitleFont = NULL;
	Palette *palette = NULL;
	ieDword *frames = NULL;
	ieDword *strrefs = NULL;
	int cnt = 0;
	vars->Lookup("Display Movie Subtitles", subtitles);
	if (subtitles) {
		int table = LoadTable(ResRef);
		TableMgr *sttable = GetTable(table);
		if (sttable) {
			cnt = sttable->GetRowCount()-3;
			if (cnt>0) {
				frames = (ieDword *) malloc(cnt * sizeof(ieDword) );
				strrefs = (ieDword *) malloc(cnt * sizeof(ieDword) );
			} else {
				cnt = 0;
			}
			if (frames && strrefs) {
				for (int i=0;i<cnt;i++) {
					frames[i] = atoi (sttable->QueryField(i+3, 0) );
					strrefs[i] = atoi (sttable->QueryField(i+3, 1) );
				}
			}
			int r = atoi(sttable->QueryField("red", "frame"));
			int g = atoi(sttable->QueryField("green", "frame"));
			int b = atoi(sttable->QueryField("blue", "frame"));
			SubtitleFont = GetFont (MovieFont); //will change
			DelTable(table);
			if (SubtitleFont) {
				Color fore = {(unsigned char) r,(unsigned char) g,(unsigned char) b, 0x00};
				Color back = {0x00, 0x00, 0x00, 0x00};
				palette = CreatePalette( fore, back );
			}
		}
	}

	//shutting down music and ambients before movie
	if (music)
		music->HardEnd();
	AmbientMgr *ambim = soundmgr->GetAmbientMgr();
	if (ambim) ambim->deactivate();
	video->SetMovieFont(SubtitleFont, palette );
	mp->CallBackAtFrames(cnt, frames, strrefs);
	mp->Play();
	FreeInterface( mp );
	FreePalette( palette );
	if (frames)
		free(frames);
	if (strrefs)
		free(strrefs);
	//restarting music
	if (music)
		music->Start();
	if (ambim) ambim->activate();
	//this will fix redraw all windows as they looked like
	//before the movie
	RedrawAll();

	//Setting the movie name to 1
	vars->SetAt( ResRef, 1 );
	return 0;
}

int Interface::Roll(int dice, int size, int add) const
{
	if (dice < 1) {
		return add;
	}
	if (size < 1) {
		return add;
	}
	if (dice > 100) {
		return add + dice * size / 2;
	}
	for (int i = 0; i < dice; i++) {
		add += rand() % size + 1;
	}
	return add;
}

static char bmp_suffix[6]="M.BMP";
static char png_suffix[6]="M.PNG";

int Interface::GetPortraits(TextArea* ta, bool smallorlarge)
{
	int count = 0;
	char Path[_MAX_PATH];

	if (smallorlarge) {
		bmp_suffix[0]='S';
		png_suffix[0]='S';
	} else {
		bmp_suffix[0]='M';
		png_suffix[0]='M';
	}
	PathJoin( Path, GamePath, GamePortraits, NULL );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return -1;
	}
	//Lookup the first entry in the Directory
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		if (de->d_name[0] == '.')
			continue;
		char dtmp[_MAX_PATH];
		PathJoin( dtmp, Path, de->d_name, NULL );
		struct stat fst;
		stat( dtmp, &fst );
		if ( S_ISDIR( fst.st_mode ))
			continue;
		strupr(de->d_name);
		char *pos = strstr(de->d_name,bmp_suffix);
		if (!pos && IsAvailable(IE_PNG_CLASS_ID) ) {
			pos = strstr(de->d_name,png_suffix);
		}
		if (!pos) continue;
		pos[1]=0;
		count++;
		ta->AppendText( de->d_name, -1 );
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	return count;
}

int Interface::GetCharSounds(TextArea* ta)
{
	bool hasfolders;
	int count = 0;
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameSounds, NULL );
	hasfolders = ( HasFeature( GF_SOUNDFOLDERS ) != 0 );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return -1;
	}
	//Lookup the first entry in the Directory
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		if (de->d_name[0] == '.')
			continue;
		char dtmp[_MAX_PATH];
		PathJoin( dtmp, Path, de->d_name, NULL );
		struct stat fst;
		stat( dtmp, &fst );
		if (hasfolders == !S_ISDIR( fst.st_mode ))
			continue;
		if (!hasfolders) {
			strupr(de->d_name);
			char *pos = strstr(de->d_name,"A.WAV");
			if (!pos) continue;
			*pos=0;
		}
		count++;
		ta->AppendText( de->d_name, -1 );
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	return count;
}

int Interface::GetCharacters(TextArea* ta)
{
	int count = 0;
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameCharacters, NULL );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return -1;
	}
	//Lookup the first entry in the Directory
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		if (de->d_name[0] == '.')
			continue;
		char dtmp[_MAX_PATH];
		PathJoin( dtmp, Path, de->d_name, NULL );
		struct stat fst;
		stat( dtmp, &fst );
		strupr(de->d_name);
		char *pos = strstr(de->d_name,".CHR");
		if (!pos) continue;
		*pos=0;
		count++;
		ta->AppendText( de->d_name, -1 );
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	return count;
}

bool Interface::LoadINI(const char* filename)
{
	FILE* config;
	config = fopen( filename, "rb" );
	if (config == NULL) {
		return false;
	}
	char name[65], value[_MAX_PATH + 3];
	while (!feof( config )) {
		name[0] = 0;
		value[0] = 0;
		char rem;
		fread( &rem, 1, 1, config );
		if (( rem == '#' ) ||
			( rem == '[' ) ||
			( rem == '\r' ) ||
			( rem == '\n' ) ||
			( rem == ';' )) {
			if (rem == '\r') {
				fgetc( config );
				continue;
			} else if (rem == '\n')
				continue;
			fscanf( config, "%*[^\r\n]%*[\r\n]" );
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		fscanf( config, "%[^=]=%[^\r\n]%*[\r\n]", name, value );
		if (( value[0] >= '0' ) && ( value[0] <= '9' )) {
			vars->SetAt( name, atoi( value ) );
		}
	}
	fclose( config );
	return true;
}

/** Enables/Disables the Cut Scene Mode */
void Interface::SetCutSceneMode(bool active)
{
	GameControl *gc = GetGameControl();
	if (gc) {
		gc->SetCutSceneMode( active );
	}
	if (game) {
		if (active) {
			game->ControlStatus |= CS_HIDEGUI;
		} else {
			game->ControlStatus &= ~CS_HIDEGUI;
		}
	}
	video->SetMouseEnabled(!active);
	video->moveX = 0;
	video->moveY = 0;
}

bool Interface::InCutSceneMode() const
{
	return (GetGameControl()->GetScreenFlags()&SF_DISABLEMOUSE)!=0;
}

void Interface::QuitGame(int BackToMain)
{
	SetCutSceneMode(false);
	if (timer) {
		//clear cutscenes
		//clear fade/screenshake effects
		timer->Init();
		timer->SetFadeFromColor(0);
	}

	DelWindow(0xffff); //delete all windows, including GameControl

	//shutting down ingame music
	//(do it before deleting the game)
	if (music) {
		music->HardEnd();
	}
	// stop any ambients which are still enqueued
	if (soundmgr) {
		AmbientMgr *ambim = soundmgr->GetAmbientMgr();
		if (ambim) ambim->deactivate();
	}
	//delete game, worldmap
	if (game) {
		delete game;
		game=NULL;
	}
	if (worldmap) {
		delete worldmap;
		worldmap=NULL;
	}
	if (BackToMain) {
		strcpy(NextScript, "Start");
		QuitFlag |= QF_CHANGESCRIPT;
	}
	GSUpdate(true);
}

void Interface::LoadGame(int index)
{
	// This function has rather painful error handling,
	// as it should swap all the objects or none at all
	// and the loading can fail for various reasons

	// Yes, it uses goto. Other ways seemed too awkward for me.

	strings->CloseAux();
	tokens->RemoveAll(NULL); //clearing the token dictionary
	DataStream* gam_str = NULL;
	DataStream* sav_str = NULL;
	DataStream* wmp_str = NULL;

	SaveGameMgr* gam_mgr = NULL;
	WorldMapMgr* wmp_mgr = NULL;

	Game* new_game = NULL;
	WorldMapArray* new_worldmap = NULL;

	LoadProgress(15);
	DelTree((const char *) CachePath, true);
	LoadProgress(20);

	if (index == -1) {
		//Load the Default Game
		gam_str = key->GetResource( GameNameResRef, IE_GAM_CLASS_ID );
		sav_str = NULL;
		wmp_str = key->GetResource( WorldMapName, IE_WMP_CLASS_ID );
	} else {
		SaveGame* sg = sgiterator->GetSaveGame( index );
		if (!sg)
			return;
		gam_str = sg->GetGame();
		sav_str = sg->GetSave();
		wmp_str = sg->GetWmap();
		delete sg;
	}

	if (!gam_str || !wmp_str)
		goto cleanup;

	// Load GAM file
	gam_mgr = ( SaveGameMgr* ) GetInterface( IE_GAM_CLASS_ID );
	if (!gam_mgr)
		goto cleanup;

	if (!gam_mgr->Open( gam_str, true ))
		goto cleanup;

	new_game = gam_mgr->LoadGame(new Game());
	if (!new_game)
		goto cleanup;

	FreeInterface( gam_mgr );
	gam_mgr = NULL;
	gam_str = NULL;

	// Load WMP (WorldMap) file
	wmp_mgr = ( WorldMapMgr* ) GetInterface( IE_WMP_CLASS_ID );
	if (! wmp_mgr)
		goto cleanup;

	if (!wmp_mgr->Open( wmp_str, true ))
		goto cleanup;

	new_worldmap = wmp_mgr->GetWorldMapArray( );

	FreeInterface( wmp_mgr );
	wmp_mgr = NULL;
	wmp_str = NULL;

	LoadProgress(30);
	// Unpack SAV (archive) file to Cache dir
	if (sav_str) {
		ArchiveImporter * ai = (ArchiveImporter*)GetInterface(IE_BIF_CLASS_ID);
		if (ai) {
			ai->DecompressSaveGame(sav_str);
			FreeInterface( ai );
			ai = NULL;
		}
		delete sav_str;
		sav_str = NULL;
	}

	// Let's assume that now is everything loaded OK and swap the objects

	delete game;
	delete worldmap;

	game = new_game;
	worldmap = new_worldmap;

	strings->OpenAux();
	LoadProgress(100);
	return;
cleanup:
	// Something went wrong, so try to clean after itself

	delete new_game;
	delete new_worldmap;

	if (gam_mgr) {
		FreeInterface( gam_mgr );
		gam_str = NULL;
	}
	if (wmp_mgr) {
		FreeInterface( wmp_mgr );
		wmp_str = NULL;
	}

	delete gam_str;
	delete wmp_str;
	delete sav_str;
}

GameControl *Interface::GetGameControl() const
{
 	Window *window = GetWindow( 0 );
	// in the beginning, there's no window at all
	if (! window)
		return NULL;

	Control* gc = window->GetControl(0);
	if (gc->ControlType!=IE_GUI_GAMECONTROL) {
		return NULL;
	}
	return (GameControl *) gc;
}

bool Interface::InitItemTypes()
{
	if (slotmatrix) {
		free(slotmatrix);
	}
	int ItemTypeTable = LoadTable( "itemtype" );
	TableMgr *it = GetTable(ItemTypeTable);

	ItemTypes = 0;
	if (it) {
		ItemTypes = it->GetRowCount(); //number of itemtypes
		if (ItemTypes<0) {
			ItemTypes = 0;
		}
		int InvSlotTypes = it->GetColumnCount();
		if (InvSlotTypes > 32) { //bit count limit
			InvSlotTypes = 32;
		}
		//make sure unsigned int is 32 bits
		slotmatrix = (ieDword *) malloc(ItemTypes * sizeof(ieDword) );
		for (int i=0;i<ItemTypes;i++) {
			unsigned int value = 0;
			unsigned int k = 1;
			for (int j=0;j<InvSlotTypes;j++) {
				if (strtol(it->QueryField(i,j),NULL,0) ) {
					value |= k;
				}
				k <<= 1;
			}
			slotmatrix[i] = (ieDword) value;
		}
		DelTable(ItemTypeTable);
	}

	//slottype describes the inventory structure
	Inventory::Init(HasFeature(GF_MAGICBIT));
	int SlotTypeTable = LoadTable( "slottype" );
	TableMgr *st = GetTable(SlotTypeTable);
	if (slottypes) {
		free(slottypes);
		slottypes = NULL;
	}
	SlotTypes = 0;
	if (st) {
		SlotTypes = st->GetRowCount();
		//make sure unsigned int is 32 bits
		slottypes = (SlotType *) malloc(SlotTypes * sizeof(SlotType) );
		memset(slottypes, -1, SlotTypes * sizeof(SlotType) );
		for (unsigned int row = 0; row < SlotTypes; row++) {
			bool alias;
			unsigned int i = (ieDword) strtol(st->GetRowName(row),NULL,0 );
			if (i>=SlotTypes) continue;
			if (slottypes[i].sloteffects!=0xffffffffu) {
				slottypes[row].slot = i;
				i=row;
				alias = true;
			} else {
				slottypes[row].slot = i;
				alias = false;
			}
			slottypes[i].slottype = (ieDword) strtol(st->QueryField(row,0),NULL,0 );
			slottypes[i].slotid = (ieDword) strtol(st->QueryField(row,1),NULL,0 );
			strnlwrcpy( slottypes[i].slotresref, st->QueryField(row,2), 8 );
			slottypes[i].slottip = (ieDword) strtol(st->QueryField(row,3),NULL,0 );
			//don't fill sloteffects for aliased slots (pst)
			if (alias) {
				continue;
			}
			slottypes[i].sloteffects = (ieDword) strtol(st->QueryField(row,4),NULL,0 );
			//setting special slots
			if (slottypes[i].slottype&SLOT_ITEM) {
				if (slottypes[i].slottype&SLOT_INVENTORY) {
					Inventory::SetInventorySlot(i);
				} else {
					Inventory::SetQuickSlot(i);
				}
			}
			switch (slottypes[i].sloteffects) {
				//fist slot, not saved, default weapon
			case SLOT_EFFECT_FIST: Inventory::SetFistSlot(i); break;
				//magic weapon slot, overrides all weapons
			case SLOT_EFFECT_MAGIC: Inventory::SetMagicSlot(i); break;
				//weapon slot, Equipping marker is relative to it
			case SLOT_EFFECT_MELEE: Inventory::SetWeaponSlot(i); break;
				//ranged slot
			case SLOT_EFFECT_MISSILE: Inventory::SetRangedSlot(i); break;
				//right hand
			case SLOT_EFFECT_LEFT: Inventory::SetShieldSlot(i); break;
				//head (for averting critical hit)
			case SLOT_EFFECT_HEAD: Inventory::SetHeadSlot(i); break;
			default:;
			}
		}
		DelTable( SlotTypeTable );
	}
	return (it && st);
}

ieDword Interface::FindSlot(unsigned int idx) const
{
	ieDword i;

	for (i=0;i<SlotTypes;i++) {
		if (idx==slottypes[i].slot) {
			break;
		}
	}
	return i;
}

ieDword Interface::QuerySlot(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slot;
}

ieDword Interface::QuerySlotType(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottype;
}

ieDword Interface::QuerySlotID(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slotid;
}

ieDword Interface::QuerySlottip(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].slottip;
}

ieDword Interface::QuerySlotEffects(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return 0;
	}
	return slottypes[idx].sloteffects;
}

const char *Interface::QuerySlotResRef(unsigned int idx) const
{
	if (idx>=SlotTypes) {
		return "";
	}
	return slottypes[idx].slotresref;
}

// checks the itemtype vs. slottype, and also checks the usability flags
// vs. Actor's stats (alignment, class, race, kit etc.)
int Interface::CanUseItemType(int slottype, Item *item, Actor *actor, bool feedback) const
{
	//inventory is a special case, we allow any items to enter it
	if ( slottype==SLOT_ALL ) {
		return SLOT_INVENTORY;
	}
	//if we look for ALL slot types, then SLOT_SHIELD shouldn't interfere
	//with twohandedness
	if ((slottype&SLOT_SHIELD) && (slottype!=SLOT_ANY) ) {
		if (item->Flags&IE_INV_ITEM_TWOHANDED) {
			//cannot equip twohanded in offhand
			if (feedback) core->DisplayConstantString(STR_2HANDEDOFFHAND, 0xf0f0f0);
			return 0;
		}
	}

	if ( (unsigned int) item->ItemType>=(unsigned int) ItemTypes) {
		//invalid itemtype
		if (feedback) core->DisplayConstantString(STR_WRONGITEMTYPE, 0xf0f0f0);
		return 0;
	}

	//if actor is supplied, check its usability fields
	if (actor) {
		ieStrRef str = actor->Unusable(item);
		if (str) {
			if (feedback) core->DisplayConstantString(str, 0xf0f0f0);
			return 0;
		}
	}

	//if any bit is true, the answer counts as true
	int ret = (slotmatrix[item->ItemType]&slottype);
	if (!ret) {
		if (feedback) core->DisplayConstantString(STR_WRONGITEMTYPE, 0xf0f0f0);
	}
	return ret;
}

Label *Interface::GetMessageLabel() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "OtherWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageLabel", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_LABEL)
				return (Label *) ctrl;
		}
	}
	return NULL;
}

TextArea *Interface::GetMessageTextArea() const
{
	ieDword WinIndex = (ieDword) -1;
	ieDword TAIndex = (ieDword) -1;

	vars->Lookup( "MessageWindow", WinIndex );
	if (( WinIndex != (ieDword) -1 ) &&
		( vars->Lookup( "MessageTextArea", TAIndex ) )) {
		Window* win = GetWindow( (unsigned short) WinIndex );
		if (win) {
			Control *ctrl = win->GetControl( (unsigned short) TAIndex );
			if (ctrl && ctrl->ControlType==IE_GUI_TEXTAREA)
				return (TextArea *) ctrl;
		}
	}
	return NULL;
}

void Interface::DisplayString(const char* Text)
{
	Label *l = GetMessageLabel();
	if (l) {
		l->SetText(Text, 0);
	}
	TextArea *ta = GetMessageTextArea();
	if (ta)
		ta->AppendText( Text, -1 );
}

static const char* DisplayFormatName = "[color=%lX]%s - [/color][p][color=%lX]%s[/color][/p]";
static const char* DisplayFormatAction = "[color=%lX]%s - [/color][p][color=%lX]%s %s[/color][/p]";
static const char* DisplayFormat = "[/color][p][color=%lX]%s[/color][/p]";
static const char* DisplayFormatValue = "[/color][p][color=%lX]%s: %d[/color][/p]";

ieStrRef Interface::GetStringReference(int stridx)
{
	return strref_table[stridx];
}

void Interface::DisplayConstantString(int stridx, unsigned int color)
{
	if (stridx<0) return;
	char* text = GetString( strref_table[stridx], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen( text ) + 12);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormat, color, text );
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

void Interface::DisplayString(int stridx, unsigned int color, ieDword flags)
{
	if (stridx<0) return;
	char* text = GetString( stridx, flags);
	int newlen = (int)(strlen( DisplayFormat) + strlen( text ) + 10);
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, DisplayFormat, color, text );
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

void Interface::DisplayConstantStringValue(int stridx, unsigned int color, ieDword value)
{
	if (stridx<0) return;
	char* text = GetString( strref_table[stridx], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen( text ) + 28);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatValue, color, text, (int) value );
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

#define PALSIZE 8
static Color ActorColor[PALSIZE];

void Interface::DisplayConstantStringName(int stridx, unsigned int color, Scriptable *speaker)
{
	unsigned int speaker_color;
	const char *name;

	if (stridx<0) return;
	switch (speaker->Type) {
		case ST_ACTOR:
			name = ((Actor *) speaker)->GetName(-1);
			GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR), PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			name = GetString( ((InfoPoint *) speaker)->DialogName );
			speaker_color = 0xc0c0c0;
			break;
		default:
			name = "";
			speaker_color = 0x800000;
			break;
	}

	char* text = GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	int newlen = (int)(strlen( DisplayFormatName ) + strlen( name ) +
		+ strlen( text ) + 18);
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, DisplayFormatName, speaker_color, name, color,
		text );
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

void Interface::DisplayConstantStringAction(int stridx, unsigned int color, Scriptable *attacker, Scriptable *target)
{
	unsigned int attacker_color;
	const char *name1;
	const char *name2;

	if (stridx<0) return;
	switch (attacker->Type) {
		case ST_ACTOR:
			name1 = ((Actor *) attacker)->GetName(-1);
			GetPalette( ((Actor *) attacker)->GetStat(IE_MAJOR_COLOR),PALSIZE, ActorColor );
			attacker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		default:
			name1 = "";
			attacker_color = 0x800000;
			break;
	}
	switch (target->Type) {
		case ST_ACTOR:
			name2 = ((Actor *) target)->GetName(-1);
			break;
		default:
			name2 = "";
			break;
	}

	char* text = GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	int newlen = (int)(strlen( DisplayFormatAction ) + strlen( name1 ) +
		+ strlen( name2 ) + strlen( text ) + 18);
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, DisplayFormatAction, attacker_color, name1, color,
		text, name2);
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

void Interface::DisplayStringName(int stridx, unsigned int color, Scriptable *speaker, ieDword flags)
{
	unsigned int speaker_color;
	const char *name;

	if (stridx<0) return;
	switch (speaker->Type) {
		case ST_ACTOR:
			name = ((Actor *) speaker)->GetName(-1);
			GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR), PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			name = GetString( ((InfoPoint *) speaker)->DialogName );
			speaker_color = 0xc0c0c0;
			break;
		default:
			name = "";
			speaker_color = 0x800000;
			break;
	}

	char* text = GetString( stridx, flags);
	int newlen = (int)(strlen( DisplayFormatName ) + strlen( name ) +
		+ strlen( text ) + 10);
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, DisplayFormatName, speaker_color, name, color,
		text );
	FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

static const char *saved_extensions[]={".are",".sto",".tot",".toh",0};

//returns true if file should be saved
bool Interface::SavedExtension(const char *filename)
{
	const char *str=strchr(filename,'.');
	if (!str) return false;
	int i=0;
	while(saved_extensions[i]) {
		if (!stricmp(saved_extensions[i], str) ) return true;
		i++;
	}
	return false;
}

static const char *protected_extensions[]={".exe",".dll",".so",0};

//returns true if file should be saved
bool Interface::ProtectedExtension(const char *filename)
{
	const char *str=strchr(filename,'.');
	if (!str) return false;
	int i=0;
	while(protected_extensions[i]) {
		if (!stricmp(protected_extensions[i], str) ) return true;
		i++;
	}
	return false;
}

void Interface::RemoveFromCache(const ieResRef resref, SClass_ID ClassID)
{
	char filename[_MAX_PATH];

	snprintf(filename, _MAX_PATH, "%s%.8s%s", CachePath, resref, TypeExt( ClassID ) );
	unlink ( filename);
}

//this function checks if the path is eligible as a cache
//if it contains a directory, or suspicious file extensions
//we bail out, because the cache will be purged regularly.
bool Interface::StupidityDetector(const char* Pt)
{
	char Path[_MAX_PATH];
	strcpy( Path, Pt );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		printf("\n**cannot open**\n");
		return true; //no directory?
	}
	struct dirent* de = readdir( dir ); //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		printf("\n**cannot read**\n");
		return true; //cannot read it?
	}
	do {
		char dtmp[_MAX_PATH];
		struct stat fst;
		snprintf( dtmp, _MAX_PATH, "%s%s%s", Path, SPathDelimiter, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode )) {
			if (de->d_name[0] == '.')
				continue;
			closedir( dir );
			printf("\n**contains another dir**\n");
			return true; //a directory in there???
		}
		if (ProtectedExtension(de->d_name) ) {
			closedir( dir );
			printf("\n**contains alien files**\n");
			return true; //an executable file in there???
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	//ok, we got a good conscience
	return false;
}

void Interface::DelTree(const char* Pt, bool onlysave)
{
	char Path[_MAX_PATH];

	if (!Pt[0]) return; //Don't delete the root filesystem :)
	strcpy( Path, Pt );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return;
	}
	struct dirent* de = readdir( dir ); //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return;
	}
	do {
		char dtmp[_MAX_PATH];
		struct stat fst;
		snprintf( dtmp, _MAX_PATH, "%s%s%s", Path, SPathDelimiter, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode ))
			continue;
		if (de->d_name[0] == '.')
			continue;
		if (!onlysave || SavedExtension(de->d_name) ) {
			unlink( dtmp );
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
}

void Interface::LoadProgress(int percent)
{
	vars->SetAt("Progress", percent);
	RedrawControls("Progress", percent);
	RedrawAll();
	DrawWindows();
	video->SwapBuffers();
}

void Interface::ReleaseDraggedItem()
{
	DraggedItem=NULL; //shouldn't free this
	video->SetDragCursor (NULL);
}

void Interface::DragItem(CREItem *item, const ieResRef Picture)
{
	//We should drop the dragged item and pick this up,
	//we shouldn't have a valid DraggedItem at this point.
	//Anyway, if there is still a dragged item, it will be destroyed.
	if (DraggedItem) {
		printMessage("Core","Forgot to call ReleaseDraggedItem when leaving inventory (item destroyed)!\n",YELLOW);
		delete DraggedItem;
	}
	DraggedItem = item;
	if (video) {
		Sprite2D* DraggedCursor = NULL;
		if (item) {
			DraggedCursor = GetBAMSprite( Picture, 0, 0 );
		}
		video->SetDragCursor (DraggedCursor);
	}
}

bool Interface::ReadItemTable(const ieResRef TableName, const char * Prefix)
{
	ieResRef ItemName;
	TableMgr * tab;
	int i,j;

	int table=LoadTable(TableName);
	if (table<0) {
		return false;
	}
	tab = GetTable(table);
	if (!tab) {
		goto end;
	}
	i=tab->GetRowCount();
	for(j=0;j<i;j++) {
		if (Prefix) {
			snprintf(ItemName,sizeof(ItemName),"%s%02d",Prefix, j+1);
		} else {
			strncpy(ItemName,tab->GetRowName(j),sizeof(ieResRef) );
		}
		//Variable elements are free'd, so we have to use malloc
		//well, not anymore, we can use ReleaseFunction
		int l=tab->GetColumnCount(j);
		if (l<1) continue;
		int cl = atoi(tab->GetColumnName(0));
		ItemList *itemlist = new ItemList(l, cl);
		for(int k=0;k<l;k++) {
			strncpy(itemlist->ResRefs[k],tab->QueryField(j,k),sizeof(ieResRef) );
		}
		ItemName[8]=0;
		strlwr(ItemName);
		RtRows->SetAt(ItemName, (void*)itemlist);
	}
end:
	DelTable(table);
	return true;
}

bool Interface::ReadRandomItems()
{
	ieResRef RtResRef;
	int i;
	TableMgr * tab;

	int table=LoadTable( "randitem" );
	ieDword difflev=0; //rt norm or rt fury
	vars->Lookup("Nightmare Mode", difflev);
	if (RtRows) {
		RtRows->RemoveAll(ReleaseItemList);
	}
	else {
		RtRows=new Variables(10, 17); //block size, hash table size
		if (!RtRows) {
			return false;
		}
		RtRows->SetType( GEM_VARIABLES_POINTER );
	}
	if (table<0) {
		return false;
	}
	tab = GetTable( table );
	if (!tab) {
		goto end;
	}
	if (difflev>=tab->GetColumnCount()) {
		difflev = tab->GetColumnCount()-1;
	}
	strncpy( GoldResRef, tab->QueryField((unsigned int) 0,(unsigned int) 0), sizeof(ieResRef) ); //gold
	if ( GoldResRef[0]=='*' ) {
		DelTable( table );
		return false;
	}
	strncpy( RtResRef, tab->QueryField( 1, difflev ), sizeof(ieResRef) );
	i=atoi( RtResRef );
	if (i<1) {
		ReadItemTable( RtResRef, 0 ); //reading the table itself
		goto end;
	}
	if (i>5) {
		i=5;
	}
	while(i--) {
		strncpy( RtResRef,tab->QueryField(2+i,difflev), sizeof(ieResRef) );
		ReadItemTable( RtResRef,tab->GetRowName(2+i) );
	}
end:
	DelTable( table );
	return true;
}

CREItem *Interface::ReadItem(DataStream *str)
{
	CREItem *itm = new CREItem();

	str->ReadResRef( itm->ItemResRef );
	str->ReadWord( &itm->Expired );
	str->ReadWord( &itm->Usages[0] );
	str->ReadWord( &itm->Usages[1] );
	str->ReadWord( &itm->Usages[2] );
	str->ReadDword( &itm->Flags );
	if (ResolveRandomItem(itm) ) {
		return itm;
	}
	delete itm;
	return NULL;
}

#define MAX_LOOP 10

//This function generates random items based on the randitem.2da file
//there could be a loop, but we don't want to freeze, so there is a limit
bool Interface::ResolveRandomItem(CREItem *itm)
{
	if (!RtRows) return true;
	for(int loop=0;loop<MAX_LOOP;loop++) {
		int i,j,k;
		char *endptr;
		ieResRef NewItem;

		void* lookup;
		if ( !RtRows->Lookup( itm->ItemResRef, lookup ) ) {
			return true;
		}
		ItemList *itemlist = (ItemList*)lookup;
		if (itemlist->WeightOdds) {
			//instead of 1d19 we calculate with 2d10 (which also has 19 possible values)
			i=Roll(2,(itemlist->Count+1)/2,-2);
		} else {
			i=Roll(1,itemlist->Count,-1);
		}
		strncpy( NewItem, itemlist->ResRefs[i], sizeof(ieResRef) );
		char *p=(char *) strchr(NewItem,'*');
		if (p) {
			*p=0; //doing this so endptr is ok
			k=strtol(p+1,NULL,10);
		} else {
			k=1;
		}
		j=strtol(NewItem,&endptr,10);
		if (*endptr) {
			strnlwrcpy(itm->ItemResRef,NewItem,sizeof(ieResRef) );
		} else {
			strnlwrcpy(itm->ItemResRef, GoldResRef, sizeof(ieResRef) );
			itm->Usages[0]=(ieWord) Roll(j,k,0);
		}
		if ( !memcmp( itm->ItemResRef,"NO_DROP",8 ) ) {
			itm->ItemResRef[0]=0;
		}
		if (!itm->ItemResRef[0]) return false;
	}
	printf("Loop detected while generating random item:%s",itm->ItemResRef);
	printStatus("ERROR", LIGHT_RED);
	return false;
}

Item* Interface::GetItem(const ieResRef resname)
{
	Item *item = (Item *) ItemCache.GetResource(resname);
	if (item) {
		return item;
	}
	DataStream* str = key->GetResource( resname, IE_ITM_CLASS_ID );
	ItemMgr* sm = ( ItemMgr* ) GetInterface( IE_ITM_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		FreeInterface( sm );
		return NULL;
	}

	item = new Item();
	//this is required for storing the 'source'
	strnlwrcpy(item->Name, resname, 8);
	sm->GetItem( item );
	if (item == NULL) {
		FreeInterface( sm );
		return NULL;
	}

	FreeInterface( sm );
	ItemCache.SetAt(resname, (void *) item);
	return item;
}

//you can supply name for faster access
void Interface::FreeItem(Item const *itm, const ieResRef name, bool free)
{
	int res;

	res=ItemCache.DecRef((void *) itm, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Item cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Item name is: %.8s\n", name);
		abort();
	}
	if (res) return;
	if (free) delete itm;
}

Spell* Interface::GetSpell(const ieResRef resname)
{
	Spell *spell = (Spell *) SpellCache.GetResource(resname);
	if (spell) {
		return spell;
	}
	DataStream* str = key->GetResource( resname, IE_SPL_CLASS_ID );
	SpellMgr* sm = ( SpellMgr* ) GetInterface( IE_SPL_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		FreeInterface( sm );
		return NULL;
	}

	spell = new Spell();
	//this is required for storing the 'source'
	strnlwrcpy(spell->Name, resname, 8);
	sm->GetSpell( spell );
	if (spell == NULL) {
		FreeInterface( sm );
		return NULL;
	}

	FreeInterface( sm );

	SpellCache.SetAt(resname, (void *) spell);
	return spell;
}

void Interface::FreeSpell(Spell *spl, const ieResRef name, bool free)
{
	int res;

	res=SpellCache.DecRef((void *) spl, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Spell cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Spell name is: %.8s or %.8s\n", name, spl->Name);
		abort();
	}
	if (res) return;
	if (free) delete spl;
}

Effect* Interface::GetEffect(const ieResRef resname)
{
	Effect *effect = (Effect *) EffectCache.GetResource(resname);
	if (effect) {
		return effect;
	}
	DataStream* str = key->GetResource( resname, IE_EFF_CLASS_ID );
	EffectMgr* em = ( EffectMgr* ) GetInterface( IE_EFF_CLASS_ID );
	if (em == NULL) {
		delete ( str );
		return NULL;
	}
	if (!em->Open( str, true )) {
		FreeInterface( em );
		return NULL;
	}

	effect = em->GetEffect(new Effect() );
	if (effect == NULL) {
		FreeInterface( em );
		return NULL;
	}

	FreeInterface( em );

	EffectCache.SetAt(resname, (void *) effect);
	return effect;
}

void Interface::FreeEffect(Effect *eff, const ieResRef name, bool free)
{
	int res;

	res=EffectCache.DecRef((void *) eff, name, free);
	if (res<0) {
		printMessage( "Core", "Corrupted Effect cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Effect name is: %.8s\n", name);
		abort();
	}
	if (res) return;
	if (free) delete eff;
}

//now that we store spell name in spl, i guess, we shouldn't pass 'ieResRef name'
//these functions are needed because Win32 doesn't allow freeing memory from
//another dll. So we allocate all commonly used memories from core
ITMExtHeader *Interface::GetITMExt(int count)
{
	return new ITMExtHeader[count];
}

SPLExtHeader *Interface::GetSPLExt(int count)
{
	return new SPLExtHeader[count];
}

Effect *Interface::GetEffect(ieDword opcode)
{
	if (opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if (!fx) {
		return NULL;
	}
	memset(fx,0,sizeof(Effect));
	fx->Opcode=opcode;
	return fx;
}

Effect *Interface::GetFeatures(int count)
{
	return new Effect[count];
}

void Interface::FreeITMExt(ITMExtHeader *p, Effect *e)
{
	delete [] p;
	delete [] e;
}

void Interface::FreeSPLExt(SPLExtHeader *p, Effect *e)
{
	delete [] p;
	delete [] e;
}

WorldMapArray *Interface::NewWorldMapArray(int count)
{
	return new WorldMapArray(count);
}

Container *Interface::GetCurrentContainer()
{
	return CurrentContainer;
}

int Interface::CloseCurrentContainer()
{
	UseContainer = false;
	if ( !CurrentContainer) {
		return -1;
	}
	//remove empty ground piles on closeup
	CurrentContainer->GetCurrentArea()->TMap->CleanupContainer(CurrentContainer);
	CurrentContainer = NULL;
	return 0;
}

void Interface::SetCurrentContainer(Actor *actor, Container *arg, bool flag)
{
	//abort action if the first selected PC isn't the original actor
	if (actor!=GetFirstSelectedPC(false)) {
		CurrentContainer = NULL;
		return;
	}
	CurrentContainer = arg;
	UseContainer = flag;
}

Store *Interface::GetCurrentStore()
{
	return CurrentStore;
}

int Interface::CloseCurrentStore()
{
	if ( !CurrentStore ) {
		return -1;
	}
	StoreMgr* sm = ( StoreMgr* ) GetInterface( IE_STO_CLASS_ID );
	if (sm == NULL) {
		return -1;
	}
	int size = sm->GetStoredFileSize (CurrentStore);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( CurrentStore->Name, IE_STO_CLASS_ID );
		int ret = sm->PutStore (&str, CurrentStore);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Store removed: %s\n", CurrentStore->Name);
			RemoveFromCache(CurrentStore->Name, IE_STO_CLASS_ID);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Store removed: %s\n", CurrentStore->Name);
		RemoveFromCache(CurrentStore->Name, IE_STO_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	FreeInterface( sm );
	delete CurrentStore;
	CurrentStore = NULL;
	return 0;
}

Store *Interface::SetCurrentStore(const ieResRef resname, const ieVariable owner)
{
	if ( CurrentStore ) {
		if ( !strnicmp(CurrentStore->Name, resname, 8) ) {
			return CurrentStore;
		}

		//not simply delete the old store, but save it
		CloseCurrentStore();
	}

	DataStream* str = key->GetResource( resname, IE_STO_CLASS_ID );
	StoreMgr* sm = ( StoreMgr* ) GetInterface( IE_STO_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		FreeInterface( sm );
		return NULL;
	}

	// FIXME - should use some already allocated in core
	// not really, only one store is open at a time, then it is
	// unloaded, we don't really have to cache it, it will be saved in
	// Cache anyway!
	CurrentStore = sm->GetStore( new Store() );
	if (CurrentStore == NULL) {
		FreeInterface( sm );
		return NULL;
	}
	FreeInterface( sm );
	strnlwrcpy(CurrentStore->Name, resname, 8);
	if (owner) {
		CurrentStore->SetOwner(owner);
	}
	return CurrentStore;
}

ieStrRef Interface::GetRumour(const ieResRef dlgref)
{
	DialogMgr* dm = ( DialogMgr* ) GetInterface( IE_DLG_CLASS_ID );
	dm->Open( key->GetResource( dlgref, IE_DLG_CLASS_ID ), true );
	Dialog *dlg = dm->GetDialog();
	FreeInterface( dm );

	if (!dlg) {
		printMessage("Interface"," ", LIGHT_RED);
		printf( "Cannot load dialog: %s\n", dlgref );
		return (ieStrRef) -1;
	}
	Scriptable *pc=game->GetPC( game->GetSelectedPCSingle(), false );

	ieStrRef ret = (ieStrRef) -1;
	int i = dlg->FindRandomState( pc );
	if (i>=0 ) {
		ret = dlg->GetState( i )->StrRef;
	}
	delete dlg;
	return ret;
}

void Interface::DoTheStoreHack(Store *s)
{
	size_t size = s->PurchasedCategoriesCount * sizeof( ieDword );
	s->purchased_categories=(ieDword *) malloc(size);

	size = s->CuresCount * sizeof( STOCure );
	s->cures=(STOCure *) malloc(size);

	size = s->DrinksCount * sizeof( STODrink );
	s->drinks=(STODrink *) malloc(size);

	for(size=0;size<s->ItemsCount;size++)
		s->items.push_back( new STOItem() );
}

//plays stock sound listed in defsound.2da
void Interface::PlaySound(int index)
{
	if (index<=DSCount) {
		soundmgr->Play(DefSound[index]);
	}
}

bool Interface::Exists(const char *ResRef, SClass_ID type)
{
	// TODO: check various caches before going to KeyImp
	return key->HasResource( ResRef, type );
}

//if the default setup doesn't fit for an animation
//create a vvc for it!
ScriptedAnimation* Interface::GetScriptedAnimation( const char *effect, bool doublehint)
{
	ScriptedAnimation *ret = NULL;

	if (Exists( effect, IE_VVC_CLASS_ID ) ) {
		DataStream *ds = key->GetResource( effect, IE_VVC_CLASS_ID );
		ret = new ScriptedAnimation(ds, true);
	} else {
		AnimationFactory *af = (AnimationFactory *)
			key->GetFactoryResource( effect, IE_BAM_CLASS_ID, IE_NORMAL );
		if (af) {
			ret = new ScriptedAnimation();
			ret->LoadAnimationFactory( af, doublehint?2:0);
		}
	}
	if (ret) {
		strnlwrcpy(ret->ResName, effect, 8);
	}
	return ret;
}

Actor *Interface::GetFirstSelectedPC(bool forced)
{
	int partySize = game->GetPartySize( false );
	if (!partySize) return NULL;
	for (int i = 0; i < partySize; i++) {
		Actor* actor = game->GetPC( i,false );
		if (actor->IsSelected()) {
			return actor;
		}
	}

	if (forced) {
		return game->GetPC(0,false);
	}
	return NULL;
}

// Return single BAM frame as a sprite. Use if you want one frame only,
// otherwise it's not efficient
Sprite2D* Interface::GetBAMSprite(const ieResRef ResRef, int cycle, int frame)
{
	Sprite2D *tspr;
	AnimationFactory* af = ( AnimationFactory* )
		key->GetFactoryResource( ResRef, IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) return 0;
	if (cycle == -1)
		tspr = af->GetFrameWithoutCycle( (unsigned short) frame );
	else
		tspr = af->GetFrame( (unsigned short) frame, (unsigned char) cycle );
	return tspr;
}

Sprite2D *Interface::GetCursorSprite()
{
	return GetBAMSprite(CursorBam, 0, 0);
}

Sprite2D *Interface::GetScrollCursorSprite(int frameNum, int spriteNum)
{
	return GetBAMSprite(ScrollCursorBam, frameNum, spriteNum);
}

/* we should return -1 if it isn't gold, otherwise return the gold value */
int Interface::CanMoveItem(const CREItem *item) const
{
	if (item->Flags & IE_INV_ITEM_UNDROPPABLE)
		return 0;
	//not gold, we allow only one single coin ResRef, this is good
	//for all of the original games
	if (strnicmp(item->ItemResRef, GoldResRef, 8 ) )
		return -1;
	//gold, returns the gold value (stack size)
	return item->Usages[0];
}

// dealing with applying effects
void Interface::ApplySpell(const ieResRef resname, Actor *actor, Actor *caster, int level)
{
	Spell *spell = GetSpell(resname);
	if (!spell) {
		return;
	}

	actor->RollSaves();

	level = spell->GetHeaderIndexFromLevel(level);
	EffectQueue *fxqueue = spell->GetEffectBlock(level);

	//check effect immunities
	int res = fxqueue->CheckImmunity ( actor );
	if (res) {
		if (res == -1 ) {
			actor = caster;
		}
		fxqueue->SetOwner( caster );
		fxqueue->AddAllEffects(actor, actor->Pos);
	}
	delete fxqueue;
}

void Interface::ApplySpellPoint(const ieResRef resname, Scriptable* /*area*/, Point &/*pos*/, Actor *caster, int level)
{
	Spell *spell = GetSpell(resname);
	if (!spell) {
		return;
	}
	level = spell->GetHeaderIndexFromLevel(level);
	EffectQueue *fxqueue = spell->GetEffectBlock(level);
	fxqueue->SetOwner( caster );
	//add effect to area???
	delete fxqueue;
}

void Interface::ApplyEffect(Effect *effect, Actor *actor, Actor *caster)
{
	EffectQueue *fxqueue = new EffectQueue();
	fxqueue->AddEffect( effect );
	//hmm, don't delete the effect

	int res = fxqueue->CheckImmunity ( actor );
	if (res) {
		if (res == -1 ) {
			actor = caster;
		}
		fxqueue->SetOwner( caster );
		fxqueue->AddAllEffects( actor, actor->Pos );
	}
	delete fxqueue;
}

void Interface::ApplyEffect(const ieResRef resname, Actor *actor, Actor *caster, int level)
{
	Effect *effect = GetEffect(resname);
	if (!effect) {
		return;
	}
	if (!level) {
		level = 1;
	}
	effect->Power=level;
	ApplyEffect(effect, actor, caster);
}

// dealing with saved games
int Interface::SwapoutArea(Map *map)
{
	MapMgr* mm = ( MapMgr* ) GetInterface( IE_ARE_CLASS_ID );
	if (mm == NULL) {
		return -1;
	}
	int size = mm->GetStoredFileSize (map);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( map->GetScriptName(), IE_ARE_CLASS_ID );
		int ret = mm->PutArea (&str, map);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Area removed: %s\n", map->GetScriptName());
			RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Area removed: %s\n", map->GetScriptName());
		RemoveFromCache(map->GetScriptName(), IE_ARE_CLASS_ID);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	FreeInterface( mm );
	return 0;
}

int Interface::WriteCharacter(const char *name, Actor *actor)
{
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, GameCharacters, NULL );
	if (!actor) {
		return -1;
	}
	ActorMgr* gm = ( ActorMgr* ) GetInterface( IE_CRE_CLASS_ID );
	if (gm == NULL) {
		return -1;
	}
	FileStream str;

	str.Create( Path, name, IE_CHR_CLASS_ID );

	//this is not needed, because the chr header writer automatically
	//calls it
	//int size = gm->GetStoredFileSize (actor);
	int ret = gm->PutActor(&str, actor, true);
	if (ret <0) {
		printMessage("Core"," ", YELLOW);
		printf("Character cannot be saved: %s\n", name);
	}
	FreeInterface(gm);
	return 0;
}

int Interface::WriteGame(const char *folder)
{
	SaveGameMgr* gm = ( SaveGameMgr* ) GetInterface( IE_GAM_CLASS_ID );
	if (gm == NULL) {
		return -1;
	}

	int size = gm->GetStoredFileSize (game);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( folder, GameNameResRef, IE_GAM_CLASS_ID );
		int ret = gm->PutGame (&str, game);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Game cannot be saved: %s\n", GameNameResRef);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Internal error, game cannot be saved: %s\n", GameNameResRef);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	FreeInterface( gm );
	return 0;
}

int Interface::WriteWorldMap(const char *folder)
{
	WorldMapMgr* wmm = ( WorldMapMgr* ) GetInterface( IE_WMP_CLASS_ID );
	if (wmm == NULL) {
		return -1;
	}

	int size = wmm->GetStoredFileSize (worldmap);
	if (size > 0) {
		//created streams are always autofree (close file on destruct)
		//this one will be destructed when we return from here
		FileStream str;

		str.Create( folder, WorldMapName, IE_WMP_CLASS_ID );
		int ret = wmm->PutWorldMap (&str, worldmap);
		if (ret <0) {
			printMessage("Core"," ", YELLOW);
			printf("Internal error, worldmap cannot be saved: %s\n", WorldMapName);
		}
	} else {
		printMessage("Core"," ", YELLOW);
		printf("Internal error, worldmap cannot be saved: %s\n", WorldMapName);
	}
	//make sure the stream isn't connected to sm, or it will be double freed
	FreeInterface( wmm );
	return 0;
}

int Interface::CompressSave(const char *folder)
{
	FileStream str;

	str.Create( folder, GameNameResRef, IE_SAV_CLASS_ID );
	DIR* dir = opendir( CachePath );
	if (dir == NULL) {
		return -1;
	}
	struct dirent* de = readdir( dir ); //Lookup the first entry in the Directory
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	//BIF and SAV are the same
	ArchiveImporter * ai = (ArchiveImporter*)GetInterface(IE_BIF_CLASS_ID);
	ai->CreateArchive( &str);

	do {
		char dtmp[_MAX_PATH];
		struct stat fst;
		snprintf( dtmp, _MAX_PATH, "%s%s", CachePath, de->d_name );
		stat( dtmp, &fst );
		if (S_ISDIR( fst.st_mode ))
			continue;
		if (de->d_name[0] == '.')
			continue;
		if (SavedExtension(de->d_name) ) {
			FileStream fs;
			fs.Open(dtmp, true);
			ai->AddToSaveGame(&str, &fs);
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	FreeInterface( ai );
	return 0;
}

int Interface::GetMaximumAbility() const { return MaximumAbility; }

int Interface::GetStrengthBonus(int column, int value, int ex) const
{
	//to hit, damage, open doors, weight allowance
	if (column<0 || column>3)
		return -9999;

	if (value<0)
		value = 0;
	else if (value>25)
		value = 25;

	if (ex<0)
		ex=0;
	else if (ex>100)
		ex=100;

	return strmod[column*(MaximumAbility+1)+value]+strmodex[column*101+ex];
}

//only the first 3 columns are supported
int Interface::GetIntelligenceBonus(int column, int value) const
{
	//learn spell, max spell level, max spell number on level
	if (column<0 || column>2)
		return -9999;

	return intmod[column*(MaximumAbility+1)+value];
}

int Interface::GetDexterityBonus(int column, int value) const
{
	//reaction, missile, ac
	if (column<0 || column>2)
		return -9999;

	return dexmod[column*(MaximumAbility+1)+value];
}

int Interface::GetConstitutionBonus(int column, int value) const
{
	//normal, warrior, minimum, regen hp, regen fatigue
	if (column<0 || column>4)
		return -9999;

	return conmod[column*(MaximumAbility+1)+value];
}

int Interface::GetCharismaBonus(int column, int value) const
{
	//?reaction
	if (column<0 || column>0)
		return -9999;

	return chrmod[column*(MaximumAbility+1)+value];
}

// -3, -2 if request is illegal or in cutscene
// -1 if pause is already active
// 0 if pause was not allowed
// 1 if autopause happened

int Interface::Autopause(ieDword flag)
{
	GameControl *gc = GetGameControl();
	if (!gc) {
		return -3;
	}
	if (InCutSceneMode()) {
		return -2;
	}
	if (gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) {
		return -1;
	}
	ieDword autopause_flags = 0;

	vars->Lookup("Auto Pause State", autopause_flags);
	if (autopause_flags & (1<<flag)) {
		DisplayConstantString(STR_AP_UNUSABLE+flag, 0xff0000);
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
		return 1;
	}
	return 0;
}

void Interface::RegisterOpcodes(int count, const EffectRef *opcodes)
{
	EffectQueue_RegisterOpcodes(count, opcodes);
}

void Interface::SetInfoTextColor(Color &color)
{
	if (InfoTextPalette) {
		FreePalette(InfoTextPalette);
	}
	InfoTextPalette = CreatePalette(color, black);
}

//todo row?
void Interface::GetResRefFrom2DA(const ieResRef resref, ieResRef resource1, ieResRef resource2, ieResRef resource3)
{
	if (!resource1) {
		return;
	}
	resource1[0]=0;
	if (resource2) {
		resource2[0]=0;
	}
	if (resource3) {
		resource3[0]=0;
	}
	int table = LoadTable(resref);
	TableMgr *tab = GetTable(table);

	if (tab) {
		unsigned int row = (unsigned int) Roll(1,tab->GetRowCount(),-1);
		strnuprcpy(resource1, tab->QueryField(row,0), 8);
		if (resource2)
			strnuprcpy(resource2, tab->QueryField(row,1), 8);
		if (resource3)
			strnuprcpy(resource3, tab->QueryField(row,2), 8);
	}
}

ieDword *Interface::GetListFrom2DA(const ieResRef resref)
{
	ieDword *ret;

	int table = LoadTable(resref);
	TableMgr *tab = GetTable(table);

	if (tab) {
		ieDword cnt = tab->GetRowCount();
		ret = (ieDword *) malloc((1+cnt)*sizeof(ieDword));
		ret[0]=cnt;
		while(cnt) {
			ret[cnt]=strtol(tab->QueryField(cnt-1, 0),NULL, 0);
			cnt--;
		}
		return ret;
	}
	ret = (ieDword *) malloc(sizeof(ieDword));
	ret[0]=0;
	return ret;
}

//returns a numeric value associated with a stat name (symbol) from stats.ids
ieDword Interface::TranslateStat(const char *stat_name) 
{
	long tmp;

	if (valid_number(stat_name, tmp)) {
		return (ieDword) tmp;
	}

	int symbol = LoadSymbol( "stats" );
	SymbolMgr *sym = GetSymbol( symbol );
	ieDword stat = (ieDword) sym->GetValue( stat_name );
	if (stat==(ieDword) ~0) {
		printMessage("Core"," ",YELLOW);
		printf("Cannot translate symbol: %s\n", stat_name);
	}
	return stat;
}
