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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.h,v 1.101 2006/03/26 16:06:25 avenger_teambg Exp $
 *
 */

class GameScript;
class Action;
class Scriptable;
class Selectable;
class Highlightable;
class Actor;
class Moveble;
class Door;
class SpriteCover;
class Gem_Polygon;

#ifndef ACTORBLOCK_H
#define ACTORBLOCK_H

#include "Sprite2D.h"
#include "CharAnimations.h"
#include "TileOverlay.h"
#include "Variables.h"
#include "Inventory.h"
#include "PathFinder.h"
#include "ImageMgr.h"
#include <list>

#define MAX_SCRIPTS		8
#define MAX_GROUND_ICON_DRAWN   3

#define SCR_OVERRIDE 0
#define SCR_AREA	 1
#define SCR_SPECIFICS 2
#define SCR_RESERVED  3
#define SCR_CLASS    4
#define SCR_RACE	 5
#define SCR_GENERAL  6
#define SCR_DEFAULT  7

//trigger flags
#define TRAP_INVISIBLE  1
#define TRAP_RESET      2
#define TRAVEL_PARTY    4
#define TRAP_DETECTABLE 8
//#define TRAP_16	 16
//#define TRAP_32	 32
#define TRAP_NPC	64
//#define TRAP_128	128
#define TRAP_DEACTIVATED  256
#define TRAVEL_NONPC      512
#define TRAP_USEPOINT       1024 //override usage point of travel regions
#define INFO_DOOR	 2048 //info trigger blocked by door

//door flags
#define DOOR_OPEN        1
#define DOOR_LOCKED      2
#define DOOR_RESET       4   //reset trap
#define DOOR_DETECTABLE  8   //trap detectable
#define DOOR_16          16  //unknown
#define DOOR_32          32  //unknown
#define DOOR_LINKED      64   //info trigger linked to this door
#define DOOR_SECRET      128  //door is secret
#define DOOR_FOUND       256  //secret door found
#define DOOR_TRANSPARENT 512  //obscures vision
#define DOOR_KEY         1024 //key removed when used
#define DOOR_SLIDE       2048 //impeded blocks ignored

//container flags
#define CONT_LOCKED      1
#define CONT_RESET       8
#define CONT_DISABLED    32

//internal actor flags
#define IF_GIVEXP     1     //give xp for this death
#define IF_JUSTDIED   2     //Died() will return true
#define IF_FROMGAME   4     //this is an NPC or PC
#define IF_REALLYDIED 8     //real death happened, actor will be set to dead
#define IF_NORECTICLE 16    //draw recticle (target mark)
#define IF_NOINT      32    //cannot interrupt the actions of this actor (save is not possible!)
#define IF_CLEANUP    64    //actor died chunky death, or other total destruction
#define IF_RUNNING    128   //actor is running
//these bits could be set by a WalkTo
#define IF_RUNFLAGS   (IF_RUNNING|IF_NORECTICLE|IF_NOINT)
#define IF_BECAMEVISIBLE 0x100//actor just became visible
//scriptable flags
#define IF_ACTIVE        0x1000
#define IF_CUTSCENEID    0x2000
#define IF_VISIBLE       0x4000
#define IF_ONCREATION    0x8000
#define IF_IDLE          0x10000

//CheckTravel return value
#define CT_CANTMOVE       0 //inactive
#define CT_ACTIVE         1 //actor can move
#define CT_GO_CLOSER      2 //entire team would move, but not close enough
#define CT_WHOLE          3 //team can move
#define CT_SELECTED       4 //not all selected actors are there
#define CT_MOVE_SELECTED  5 //all selected can move

//bits for binary trigger bitfield
#define BT_DIE            1
#define BT_ONCREATION     2
#define BT_BECAMEVISIBLE  4

//global bittriggers (set in game)
#define BT_PARTYRESTED    0x10000

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef enum ScriptableType { ST_ACTOR = 0, ST_PROXIMITY = 1, ST_TRIGGER = 2,
ST_TRAVEL = 3, ST_DOOR = 4, ST_CONTAINER = 5, ST_AREA = 6, ST_GLOBAL = 7 } ScriptableType;

typedef std::list<ieDword *> TriggerObjects;

//#define SEA_RESET		0x00000002
//#define SEA_PARTY_REQUIRED	0x00000004

class GEM_EXPORT Scriptable {
public:
	Scriptable(ScriptableType type);
	virtual ~Scriptable(void);
private:
	TriggerObjects tolist;
	ieDword bittriggers;
	unsigned long startTime;
	unsigned long interval;
	unsigned long WaitCounter;
protected: //let Actor access this
	Map *area;
	char scriptName[33];
	ieDword InternalFlags; //for triggers
	Scriptable* CutSceneId;
public:
	Variables* locals;
	ScriptableType Type;
	Point Pos;
	GameScript* Scripts[MAX_SCRIPTS];
	char* overHeadText;
	unsigned char textDisplaying;
	unsigned long timeStartDisplaying;
	ieDword LastTrigger;
	ieDword LastEntered;
	ieDword LastDisarmed;
	ieDword LastDisarmFailed;
	std::list< Action*> actionQueue;
	Action* CurrentAction;
	unsigned long playDeadCounter;
public:
	void SetScript(ieResRef aScript, int idx);
	void SetWait(unsigned long time);
	unsigned long GetWait();
	Scriptable *GetCutsceneID();
	void SetCutsceneID(Scriptable *csid);
	void NoInterrupt();
	void Hide();
	void Unhide();
	void Activate();
	void Deactivate();
	ieDword GetInternalFlag();
	char* GetScriptName();
	Map* GetCurrentArea();
	void SetMap(Map *map);
	void SetScript(int index, GameScript* script);
	void DisplayHeadText(const char* text);
	void SetScriptName(const char* text);
	//call this to enable script running as soon as possible
	void ImmediateEvent();
	void ExecuteScript(GameScript* Script);
	void AddAction(Action* aC);
	void AddActionInFront(Action* aC);
	Action* GetNextAction();
	Action* PopNextAction();
	void ClearActions();
	void ReleaseCurrentAction();
	bool InMove();
	virtual void ProcessActions();
	//these functions handle clearing of triggers that resulted a
	//true condition (whole triggerblock returned true)
	void InitTriggers();
	void ClearTriggers();
	void SetBitTrigger(ieDword bittrigger);
	void AddTrigger(ieDword *actorref);
	/* re/draws overhead text on the map screen */
	void DrawOverheadText(Region &screen);
};

class GEM_EXPORT Selectable : public Scriptable {
public:
	Selectable(ScriptableType type);
	virtual ~Selectable(void);
public:
	Region BBox;
	ieWord Selected; //could be 0x80 for unselectable
	bool Over;
	Color selectedColor;
	Color overColor;
	Sprite2D *circleBitmap[2];
	int size;
private:
	// current SpriteCover for wallgroups
	SpriteCover* cover;
public:
	void SetBBox(Region &newBBox);
	void DrawCircle(Region &vp);
	bool IsOver(Point &Pos);
	void SetOver(bool over);
	bool IsSelected();
	void Select(int Value);
	void SetCircle(int size, Color color, Sprite2D* normal_circle, Sprite2D* selected_circle);

	/* store SpriteCover */
	void SetSpriteCover(SpriteCover* c);
	/* get stored SpriteCover */
	SpriteCover* GetSpriteCover() const { return cover; }
	/* want dithered SpriteCover */
	int WantDither();
};

class GEM_EXPORT Highlightable : public Scriptable {
public:
	Highlightable(ScriptableType type);
	virtual ~Highlightable(void);
public:
	Gem_Polygon* outline;
	Color outlineColor;
	ieDword Cursor;
	bool Highlight;
	ieResRef DialogResRef;
	Point TrapLaunch;
	ieResRef KeyResRef;
public:
	bool IsOver(Point &Pos);
	void DrawOutline();
	void SetCursor(unsigned char CursorIndex);
	/** Gets the Dialog ResRef */
	const char* GetDialog(void) const
	{
		return DialogResRef;
	}
	const char* GetKey(void) const
	{
		if (KeyResRef[0]) return KeyResRef;
		return NULL;
	}
};

class GEM_EXPORT Moveble : public Selectable {
private: //these seem to be sensitive, so get protection
	unsigned char StanceID;
	unsigned char Orientation, NewOrientation;
public:
	Moveble(ScriptableType type);
	virtual ~Moveble(void);
	Point Destination;
	PathNode* path;
	PathNode* step;
	ieDword timeStartStep;
	Sprite2D* lastFrame;
	ieResRef Area;
public:
//inliners to protect data consistency
	inline unsigned char GetOrientation() {
		return Orientation;
	}
	inline unsigned char GetNextFace() {
		//slow turning
		if (Orientation != NewOrientation) {
			if ( ( (NewOrientation-Orientation) & (MAX_ORIENT-1) ) <= MAX_ORIENT/2) {
				Orientation++;
			} else {
				Orientation--;
			}
			Orientation = Orientation&(MAX_ORIENT-1);
		}

		return Orientation;
	}
	inline unsigned char GetStance() {
		return StanceID;
	}

	inline void SetOrientation(int value, bool slow) {
		//MAX_ORIENT == 16, so we can do this
		NewOrientation = (unsigned char) (value&(MAX_ORIENT-1));
		if (!slow) {
			Orientation = NewOrientation;
		}
	}

	void SetStance(unsigned int arg);
	void DoStep(unsigned int walk_speed);
	void AddWayPoint(Point &Des);
	void RunAwayFrom(Point &Des, int PathLength, int flags);
	void RandomWalk(bool can_stop);
	void MoveLine(int steps, int Pass);
	void FixPosition();
	void WalkTo(Point &Des, int MinDistance = 0);
	void MoveTo(Point &Des);
	void ClearPath();
	void DrawTargetPoint(Region &vp);
};

//Tiled objects are not used (and maybe not even implemented correctly in IE)
//they seem to be most closer to a door and probably obsoleted by it
//are they scriptable?
class GEM_EXPORT TileObject {
public:
	TileObject(void);
	~TileObject(void);
	void SetOpenTiles(unsigned short *indices, int count);
	void SetClosedTiles(unsigned short *indices, int count);

public:
	char Name[33];
	ieResRef Tileset; //or wed door ID?
	ieDword Flags;
	unsigned short* opentiles;
	ieDword opencount;
	unsigned short* closedtiles;
	ieDword closedcount;
};

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay* Overlay);
	~Door(void);
public:
	char LinkedInfo[33];
	ieResRef ID;    //WED ID
	TileOverlay* overlay;
	unsigned short* tiles;
	unsigned char count;
	ieDword Flags;
	int closedIndex;
	Gem_Polygon* open;
	Gem_Polygon* closed;
	Point* open_ib;   //impeded blocks stored in a Point array
	int oibcount;
	Point* closed_ib;
	int cibcount;
	Point toOpen[2];
	ieResRef OpenSound;
	ieResRef CloseSound;
	ieResRef LockSound;
	ieResRef UnLockSound;
	ieDword LockDifficulty; //this is a dword?
	ieWord TrapDetectionDiff;
	ieWord TrapRemovalDiff;
	ieDword TrapFlags;
	ieStrRef OpenStrRef;
	ieStrRef NameStrRef;
	ieResRef Dialog;
private:
	void ToggleTiles(int State, bool playsound = false);
	void ImpedeBlocks(int count, Point *points, unsigned int value);
	void UpdateDoor();
	bool BlockedOpen(bool Open, bool ForceOpen);
public:
	void SetName(const char* Name); // sets door ID
	void SetTiles(unsigned short* Tiles, int count);
	void SetDoorLocked(bool Locked, bool playsound);
	void SetDoorOpen(bool Open, bool playsound, ieDword ID);
	void SetPolygon(bool Open, Gem_Polygon* poly);
	bool IsOpen() const;
	void TryPickLock(Actor *actor);
	void DebugDump();
};

class GEM_EXPORT Container : public Highlightable {
public:
	Container(void);
	~Container(void);
	void DebugDump();
	//turns the container to a pile
	void DestroyContainer();
	//removes an item from the container's inventory
	CREItem *RemoveItem(unsigned int idx, unsigned int count);
	//adds an item to the container's inventory
	int AddItem(CREItem *item);
	//draws the ground icons
	void DrawPile(bool highlight, Region screen, Color tint);
	//returns dithering option
	int WantDither();
private:
	//updates the ground icons for a pile
	void RefreshGroundIcons();
	void FreeGroundIcons();
	void CreateGroundIconCover();
public:
	Point toOpen;
	ieWord Type;
	ieDword Flags;
	ieWord LockDifficulty;
	ieWord TrapDetectionDiff;
	ieWord TrapRemovalDiff;
	ieWord Trapped;
	ieWord TrapDetected;
	Inventory inventory;
	ieStrRef OpenFail;
	//these are not saved
	Sprite2D *groundicons[3];
	SpriteCover *groundiconcover;
	//keyresref is stored in Highlightable
};

class GEM_EXPORT InfoPoint : public Highlightable {
public:
	InfoPoint(void);
	~InfoPoint(void);
	//detect trap, set skill to 256 if you want sure fire
	void DetectTrap(int skill);
	//returns true if trap is visible, only_detected must be true
	//if you want to see discovered traps, false is for cheats
	bool VisibleTrap(bool only_detected);
	//returns true if trap has been triggered, tumble skill???
	bool TriggerTrap(int skill);
	//call this to check if an actor entered the trigger zone
	bool Entered(Actor *actor);
	//checks if the actor may use this travel trigger
	int CheckTravel(Actor *actor);
	void DebugDump();

public:
	ieResRef Destination;
	char EntranceName[33];
	ieDword Flags;
	ieWord TrapDetectionDiff;
	ieWord TrapRemovalDiff;
	ieWord Trapped;
	ieWord TrapDetected;
	//overheadtext contains the string, but we have to save this
	ieStrRef StrRef;
	Point UsePoint;
};

#endif
