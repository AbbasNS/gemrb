/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Game.h,v 1.74 2005/12/20 23:20:52 avenger_teambg Exp $
 *
 */

/**
 * @file Game.h
 * Declares Game class, object representing current game state.
 * @author The GemRB Project
 */


class Game;

#ifndef GAME_H
#define GAME_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#include <vector>
#include "../../includes/ie_types.h"
#include "Actor.h"
#include "Map.h"
#include "Variables.h"

//for global triggers
#define IF_PARTYRESTED 1

//joinparty flags
#define JP_JOIN     1  //refresh join time
#define JP_INITPOS  2  //init startpos

//protagonist mode
#define PM_NO       0  //no death checks
#define PM_YES      1  //if protagonist dies, game over
#define PM_TEAM     2  //if team dies, game over

// Flags bits for SelectActor()
// !!! Keep these synchronized with GUIDefines.py !!!
#define SELECT_NORMAL   0x00
#define SELECT_REPLACE  0x01 // when selecting actor, deselect all others
#define SELECT_QUIET    0x02 // do not run handler when changing selection

// Flags bits for EveryoneNearPoint()
#define ENP_CANMOVE     1    // also check if the PC can move
#define ENP_ONLYSELECT  2    // check only selected PC

// GUI Control Status flags (saved in game)
#define CS_PARTY_AI  1   //enable party AI
#define CS_MEDIUM    2   //medium dialog
#define CS_LARGE     6   //large dialog, both bits set
#define CS_DIALOGSIZEMASK 6
#define CS_DIALOG    8   //dialog is running
#define CS_HIDEGUI   16  //hide all gui
#define CS_ACTION    32  //hide action pane
#define CS_PORTRAIT  64  //hide portrait pane

//Weather bits
#define WB_NORMAL    0
#define WB_RAIN      1
#define WB_SNOW      2
#define WB_FOG       3
#define WB_LIGHTNING 8
#define WB_START     0x80

/**
 * @struct PCStruct
 * Information about party member.
 */

typedef struct PCStruct {
	ieWord   Selected;
	ieWord   PartyOrder;
	ieDword  OffsetToCRE;
	ieDword  CRESize;
	ieResRef CREResRef;
	ieDword  Orientation;
	ieResRef Area;
	ieWord   XPos;
	ieWord   YPos;
	ieWord   ViewXPos;
	ieWord   ViewYPos;
	ieWord   ModalState;
	ieWord   Happiness;
	unsigned char Unknown2c[96];
	ieWord   QuickWeaponSlot[4];
	unsigned char Unknown94[8];
	ieResRef QuickSpellResRef[3];
	ieWord   QuickItemSlot[3];
	unsigned char UnknownBA[6];
	char Name[32];
	ieDword  TalkCount;
	unsigned char QSlots[9];
} PCStruct;

#define IE_GAM_JOURNAL 0
#define IE_GAM_QUEST_UNSOLVED 1
#define IE_GAM_QUEST_DONE  2
#define IE_GAM_JOURNAL_USER 3

/**
 * @struct GAMJournalEntry
 * Single entry in a journal
 */

typedef struct GAMJournalEntry {
	ieStrRef Text;
	ieDword  GameTime; // in game time seconds
	ieByte   Chapter;
	ieByte   unknown09;
	ieByte   Section;
	ieByte   Group;   // this is a GemRB extension
} GAMJournalEntry;

/**
 * @class Game
 * Object representing current game state, mostly party.
 */

class GEM_EXPORT Game : public Scriptable {
public:
	Game(void);
	~Game(void);
private:
	std::vector< Actor*> PCs;
	std::vector< Actor*> NPCs;
	std::vector< Map*> Maps;
	std::vector< GAMJournalEntry*> Journals;
	std::vector< char*> mastarea;
	int MapIndex;
public:
	std::vector< Actor*> selected;
	int version;
	Variables* kaputz;
	ieByte* beasts;
	ieByte* mazedata; //only in PST
	ieResRef Familiars[9];
	ieDword CombatCounter;

	/** Index of PC selected in non-walking environment (shops, inventory...) */
	int SelectedSingle;
	/** 0 if the protagonist's death doesn't cause game over */
	/** 1 if the protagonist's death causes game over */
	/** 2 if no check is needed (pst) */
	int protagonist;
	/** if party size exceeds this amount, a callback will be called */
	size_t partysize;
public:
	ieDword Ticks;
	ieDword interval;
	ieDword GameTime;
	ieDword RealTime;
	ieWord  WhichFormation;
	ieWord  Formations[5];
	ieDword PartyGold;
	ieDword WeatherBits;
	ieDword Unknown48;
	ieDword Reputation;
	ieDword ControlStatus;// used in bg2, iwd (where you can switch panes off)
	ieResRef AnotherArea;
	ieResRef CurrentArea;
	ieResRef LoadMos;
public:
	/** Returns the PC's slot count for partyID */
	int FindPlayer(unsigned int partyID);
	/** Returns actor by slot */
	Actor* GetPC(unsigned int slot, bool onlyalive);
	/** Finds an actor in party by party ID, returns Actor, if not there, returns NULL*/
	Actor* FindPC(unsigned int partyID);
	Actor* FindNPC(unsigned int partyID);
	/** Finds an actor in party, returns slot, if not there, returns -1*/
	int InParty(Actor* pc) const;
	/** Finds an actor in store, returns slot, if not there, returns -1*/
	int InStore(Actor* pc) const;
	/** Finds an actor in party by scripting name*/
	Actor* FindPC(const char *deathvar);
	/** Finds an actor in store by scripting name*/
	Actor* FindNPC(const char *deathvar);
	/** Joins party */
	int JoinParty(Actor* pc, int join=JP_JOIN);
	/** Return current party size */
	int GetPartySize(bool onlyalive) const;
	/** Returns the npcs count */
	int GetNPCCount() const { return (int)NPCs.size(); }
	/** Sends the hotkey trigger to all selected pcs */
	void SetHotKey(unsigned long Key);
	/** Select PC for non-walking environment (shops, inventory, ...) */
	bool SelectPCSingle(int index);
	/** Get index of selected PC for non-walking env (shops, inventory, ...) */
	int GetSelectedPCSingle() const;
	/** (De)selects actor. */
	bool SelectActor( Actor* actor, bool select, unsigned flags );

	/** Return current party level count for xp calculations */
	int GetPartyLevel(bool onlyalive) const;
	/** Reassigns inparty numbers, call it after party creation */
	void ConsolidateParty();
	/** Removes actor from party (if in there) */
	int LeaveParty(Actor* pc);
	/** Returns slot*/
	int DelPC(unsigned int slot, bool autoFree = false);
	int DelNPC(unsigned int slot, bool autoFree = false);
	/** Returns map in index */
	Map* GetMap(unsigned int index) const;
	/** Returns a map from area name, loads it if needed
	 * use it for the biggest safety, change = true will change the current map */
	Map* GetMap(const char *areaname, bool change);
	/** Returns slot of the map if found */
	int FindMap(const char *ResRef);
	/* use GetCurrentArea() */
	//Map * GetCurrentMap();
	int AddMap(Map* map);
	/** Determine if area is master area*/
	bool MasterArea(const char *area);
	/** Dynamically adding an area to master areas*/
	void SetMasterArea(const char *area);
	/** Returns slot of the map, if it was already loaded,
	 * don't load it again, set changepf == true,
	 * if you want to change the pathfinder too. */
	int LoadMap(const char* ResRef);
	int DelMap(unsigned int index, int forced = 0);
	int AddNPC(Actor* npc);
	Actor* GetNPC(unsigned int Index);

	//journal entries
	void DeleteJournalEntry(ieStrRef strref);
	void DeleteJournalGroup(ieByte Group);
	/** Adds a journal entry from dialog data.
	 * Time and chapter are calculated on the fly
	 * Returns false if the entry already exists */
	bool AddJournalEntry(ieStrRef strref, int section, int group);
	/** Adds a journal entry while loading the .gam structure */
	void AddJournalEntry(GAMJournalEntry* entry);
	int GetJournalCount() const;
	GAMJournalEntry* FindJournalEntry(ieStrRef strref);
	GAMJournalEntry* GetJournalEntry(unsigned int Index);

	char *GetFamiliar(unsigned int Index);

	bool IsBeastKnown(unsigned int Index) const {
		if (!beasts) {
			return false;
		}
		return beasts[Index] != 0;
	}
	void SetBeastKnown(unsigned int Index) {
		if (!beasts) {
			return;
		}
		beasts[Index] = 1;
	}
	ieWord GetFormation() const {
		if (WhichFormation>4) {
			return 0;
		}
		return Formations[WhichFormation];
	}

	void ShareXP(int XP, bool divide);
	/** returns true if we should start the party overflow window */
	bool PartyOverflow();
	/** returns true if the party death condition is true */
	bool EveryoneDead() const;
	/** returns true if no one moves */
	bool EveryoneStopped() const;
	bool EveryoneNearPoint(Map *map, Point &p, int flags) const;
	/** returns true if a PC just died */
	int PartyMemberDied() const;
	/** Increments chapter variable and refreshes kill stats */
	void IncrementChapter();
	/** Sets party reputation */
	void SetReputation(ieDword r);
	/** Sets the gamescreen control status (pane states, dialog textarea size) */
	void SetControlStatus(int value, int operation);
	/** Sets party size (1-32000) */
	void SetPartySize(int value);
	/** Sets protagonist mode to 0-none,1-protagonist,2-team */
	void SetProtagonistMode(int value);
	void StartRainOrSnow(bool conditional, int weather);
	size_t GetLoadedMapCount() const { return Maps.size(); }
	/** Adds or removes gold */
	void AddGold(ieDword add);
	/** Adds ticks to game time */
	void AdvanceTime(ieDword add);
	/** Runs the script engine on the global script and the area scripts 
            areas run scripts on door, infopoint, container, actors too */
	void UpdateScripts();
	/** runs area functionality, sets partyrested trigger */
	void RestParty(bool noareacheck);
	/** Dumps information about the object */
	void DebugDump();
};

#endif  // ! GAME_H
