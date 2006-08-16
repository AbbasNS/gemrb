/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GSUtils.cpp,v 1.71 2006/08/16 18:35:04 avenger_teambg Exp $
 *
 */

#include "GSUtils.h"
#include "Interface.h"
#include "TileMap.h"
#include "StringMgr.h"
#include "ResourceMgr.h"
#include "Video.h"
#include "SoundMgr.h"
#include "Item.h"
#include "Map.h"
#include "Game.h"
#include "GameControl.h"

#include "../../includes/strrefs.h"
#include "../../includes/defsounds.h"

int initialized = 0;
//these tables will get freed by Core
SymbolMgr* triggersTable;
SymbolMgr* actionsTable;
SymbolMgr* objectsTable;
TriggerFunction triggers[MAX_TRIGGERS];
ActionFunction actions[MAX_ACTIONS];
short actionflags[MAX_ACTIONS];
short triggerflags[MAX_TRIGGERS];
ObjectFunction objects[MAX_OBJECTS];
IDSFunction idtargets[MAX_OBJECT_FIELDS];
Cache SrcCache; //cache for string resources (pst)
Cache BcsCache; //cache for scripts
int ObjectIDSCount = 7;
int MaxObjectNesting = 5;
bool HasAdditionalRect = false;
bool HasTriggerPoint = false;
//released by ReleaseMemory
ieResRef *ObjectIDSTableNames;
int ObjectFieldsCount = 7;
int ExtraParametersCount = 0;
int InDebug = 0;
int happiness[3][20];
int rmodrep[20];
int rmodchr[25];
int RandomNumValue;
int *SkillStats=NULL;
int SkillCount=-1;

void InitScriptTables()
{
	//initializing the skill->stats conversion table
	{
	int table = core->LoadTable( "skillsta" );
	if (table!=-1) {
		TableMgr *tab = core->GetTable( table );
		int rowcount = tab->GetRowCount();
		SkillCount = rowcount;
		if (rowcount) {
			SkillStats = (int *) malloc(rowcount * sizeof(int) );
			while(rowcount--) {
				SkillStats[rowcount]=strtol(tab->QueryField(rowcount,0), NULL, 0);
			}
		}
	}
	}
	//initializing the happiness table
	{
	int table = core->LoadTable( "happy" );
	if (table!=-1) {
		TableMgr *tab = core->GetTable( table );
		for (int alignment=0;alignment<3;alignment++) {
			for (int reputation=0;reputation<20;reputation++) {
				happiness[alignment][reputation]=strtol(tab->QueryField(reputation,alignment), NULL, 0);
			}
		}
		core->DelTable( table );
	}
	}

	//initializing the reaction mod. reputation table
	{
	int table = core->LoadTable( "rmodrep" );
	if (table!=-1) {
		TableMgr *tab = core->GetTable( table );
		for (int reputation=0;reputation<20;reputation++) {
			rmodrep[reputation]=strtol(tab->QueryField(0,reputation), NULL, 0);
		}
		core->DelTable( table );
	}
	}

	//initializing the reaction mod. charisma table
	{
	int table = core->LoadTable( "rmodchr" );
	if (table!=-1) {
		TableMgr *tab = core->GetTable( table );
		for (int charisma=0;charisma<25;charisma++) {
			rmodchr[charisma]=strtol(tab->QueryField(0,charisma), NULL, 0);
		}
		core->DelTable( table );
	}
	}
}

int GetReaction(Scriptable* Sender)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int chr = ab->GetStat(IE_CHR)-1;
	int rep = core->GetGame()->Reputation/10;
	return 10 + rmodrep[rep]+rmodchr[chr];
}

int GetHappiness(Scriptable* Sender, int reputation)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int alignment = ab->GetStat(IE_ALIGNMENT)&AL_GNE_MASK; //good, neutral, evil
	if (reputation>19) {
		reputation=19;
	}
	return happiness[alignment][reputation/10];
}

int GetHPPercent(Scriptable* Sender)
{
	if (Sender->Type != ST_ACTOR) {
		return 0;
	}
	Actor* ab = ( Actor* ) Sender;
	int hp1 = ab->GetStat(IE_MAXHITPOINTS);
	if (hp1<1) {
		return 0;
	}
	int hp2 = ab->GetStat(IE_HITPOINTS);
	if (hp2<1) {
		return 0;
	}
	return hp2*100/hp1;
}

void HandleBitMod(ieDword &value1, ieDword value2, int opcode)
{
	switch(opcode) {
		case BM_AND:
			value1 = ( value1& value2 );
			break;
		case BM_OR:
			value1 = ( value1| value2 );
			break;
		case BM_XOR:
			value1 = ( value1^ value2 );
			break;
		case BM_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2 );
			break;
		case BM_SET: //this is a GemRB extension
			value1 = value2;
			break;
	}
}

// SPIT is not in the original engine spec, it is reserved for the
// enchantable items feature
//					0      1       2     3      4
static const char *spell_suffices[]={"SPIT","SPPR","SPWI","SPIN","SPCL"};

//this function handles the polymorphism of Spell[RES] actions
bool ResolveSpellName(ieResRef spellres, Action *parameters)
{
	if (parameters->string0Parameter[0]) {
		strnlwrcpy(spellres, parameters->string0Parameter, 8);
	} else {
		//resolve spell
		int type = parameters->int0Parameter/1000;
		int spellid = parameters->int0Parameter%1000;
		if (type>4) {
			return false;
		}
		sprintf(spellres, "%s%03d", spell_suffices[type], spellid);
	}
	return true;
}

bool StoreHasItemCore(ieResRef storename, ieResRef itemname)
{
	bool has_current=false;
	ieResRef current;
	CREItem item;

	Store *store = core->GetCurrentStore();
	if (!store) {
		store = core->SetCurrentStore(storename);
	} else {
		if (strnicmp(store->Name, storename, 8) ) {
			//not the current store, we need some dirty hack
			has_current = true;
			strnlwrcpy(current, store->Name, 8);
		}
	}
	bool ret = false;
	//don't use triggers (pst style), it would be possible to create infinite loops
	if (store->FindItem(itemname, false) ) {
		ret=true;
	}
	if (has_current) {
		//setting back old store (this will save our current store)
		core->SetCurrentStore(current);
	}
	return ret;
}

//check if an inventory (container or actor) has item (could be recursive ?)
bool HasItemCore(Inventory *inventory, ieResRef itemname, ieDword flags)
{
	if (inventory->HasItem(itemname, flags)) {
		return true;
	}
	int i=inventory->GetSlotCount();
	while (i--) {
		//maybe we could speed this up if we mark bag items with a flags bit
		CREItem *itemslot = inventory->GetSlotItem(i);
		if (!itemslot)
			continue;
		Item *item = core->GetItem(itemslot->ItemResRef);
		if (!item)
			continue;
		bool ret = false;
		if (core->CanUseItemType(item->ItemType,SLOT_BAG,0,0,NULL) ) {
			//the store is the same as the item's name
			ret = StoreHasItemCore(itemslot->ItemResRef, itemname);
		}
		core->FreeItem(item, itemslot->ItemResRef);
		if (ret) {
			return true;
		}
	}
	return false;
}

void DisplayStringCore(Scriptable* Sender, int Strref, int flags)
{
	StringBlock sb;

	memset(&sb,0,sizeof(sb));
	printf( "Displaying string on: %s\n", Sender->GetScriptName() );
	if (flags & DS_CONST) {
		if (Sender->Type!=ST_ACTOR) {
			printf("[GameScript] Verbal constant not supported for non actors!\n");
			return;
		}
		Actor* actor = ( Actor* ) Sender;
		if ((ieDword) Strref>=VCONST_COUNT) {
			printf("[GameScript] Invalid verbal constant!\n");
			return;
		}

		int tmp=(int) actor->StrRefs[Strref];
		if (tmp == -1) {
			actor->ResolveStringConstant( sb.Sound, (unsigned int) Strref);
		}
		Strref = tmp;
	}

	if (Strref != -1) {
		sb = core->strings->GetStringBlock( Strref );
		if (flags & DS_CONSOLE) {
			//can't play the sound here, we have to delay action
			//and for that, we have to know how long the text takes
			if(flags&DS_NONAME) {
				core->DisplayString( sb.text );
			} else {
				core->DisplayStringName( Strref,0xf0f0f0,Sender, 0);
			}
		}
		if (flags & DS_HEAD) {
			Sender->DisplayHeadText( sb.text );
			//don't free sb.text, it is residing in Sender
		} else {
			core->FreeString( sb.text );
		}
	}
	if (sb.Sound[0] && !(flags&DS_SILENT) ) {
		ieDword len = core->GetSoundMgr()->Play( sb.Sound );
		ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}

int CanSee(Scriptable* Sender, Scriptable* target, bool range)
{
	Map *map;
	unsigned int dist;

	map = target->GetCurrentArea();
	if ( map!=Sender->GetCurrentArea() ) {
		return 0;
	}

	if (range) {
		if (Sender->Type == ST_ACTOR) {
			Actor* snd = ( Actor* ) Sender;
			dist = snd->Modified[IE_VISUALRANGE];
		} else { 
			dist = 30;
		}
	
		if (Distance(target->Pos, Sender->Pos) > dist * 20) {
			return 0;
		}
	}

	return map->IsVisible(target->Pos, Sender->Pos);
}

int ValidForDialogCore(Scriptable* Sender, Actor *target)
{
	if (!CanSee(Sender, target, false) ) {
		return 0;
	}
	
	if (target->GetInternalFlag()&IF_REALLYDIED) {
		return 0;
	}
	//we should rather use STATE_SPEECHLESS_MASK
	if (target->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return 0;
	}
	return 1;
}

//transfering item from Sender to target
//if target has no inventory, the item will be destructed
//if target can't get it, it will be dropped at its feet
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags)
{
	Inventory *myinv;
	Map *map;

	if (!target) {
		return MIC_INVALID;
	}
	map=Sender->GetCurrentArea();
	switch(Sender->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) Sender)->inventory;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) Sender)->inventory;
			break;
		default:
			return MIC_INVALID;
	}
	CREItem *item;
	myinv->RemoveItem(resref, flags, &item);
	if (!item) {
		return MIC_NOITEM;
	}
	switch(target->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) target)->inventory;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) target)->inventory;
			break;
		default:
			myinv = NULL;
			break;
	}
	if (!myinv) {
		delete item;
		return MIC_GOTITEM;
	}
	if ( myinv->AddSlotItem(item, -1) !=2) {
		// drop it at my feet
		map->AddItemToLocation(target->Pos, item);
		return MIC_FULL;
	}
	return MIC_GOTITEM;
}

static Targets* ReturnActorAsTarget(Actor *aC)
{
		if (!aC) {
			return NULL;
		}
		//Ok :) we now have our Object. Let's create a Target struct and add the object to it
		Targets *tgts = new Targets( );
		tgts->AddTarget( aC, 0 );
		//return here because object name/IDS targeting are mutually exclusive
		return tgts;
}

/* returns actors that match the [x.y.z] expression */
static Targets* EvaluateObject(Scriptable* Sender, Object* oC)
{
	Map *map=Sender->GetCurrentArea();

	if (oC->objectName[0]) {
		//We want the object by its name... (doors/triggers don't play here!)
		Actor* aC = map->GetActor( oC->objectName );
		return ReturnActorAsTarget(aC);
	}

	if (oC->objectFields[0]==-1) {
		Actor* aC = map->GetActorByGlobalID( (ieDword) oC->objectFields[1] );
		return ReturnActorAsTarget(aC);
	}

	Targets *tgts=NULL;

	//else branch, IDS targeting
	for (int j = 0; j < ObjectIDSCount; j++) {
		if (!oC->objectFields[j]) {
			continue;
		}
		IDSFunction func = idtargets[j];
		if (!func) {
			printf("Unimplemented IDS targeting opcode!\n");
			continue;
		}
		if (tgts) {
			//we already got a subset of actors
			int i = tgts->Count();
			/*premature end, filtered everything*/
			if (!i) {
				break; //leaving the loop
			}
			targetlist::iterator m;
			const targettype *t = tgts->GetFirstTarget(m, -1);
			while (t) {
				if (t->actor->Type!=ST_ACTOR) {
//we should never stumble here
abort();
//					t = tgts->RemoveTargetAt(m);
					continue;
				}
				if (!func( (Actor *) (t->actor), oC->objectFields[j] ) ) {
					t = tgts->RemoveTargetAt(m);
					continue;
				}
				t = tgts->GetNextTarget(m, -1);
			}
		} else {
			//we need to get a subset of actors from the large array
			//if this gets slow, we will need some index tables
			int i = map->GetActorCount(true);
			tgts = new Targets();
			while (i--) {
				Actor *ac=map->GetActor(i,true);
				int dist = Distance(Sender->Pos, ac->Pos);
				if (ac && func(ac, oC->objectFields[j]) ) {
					tgts->AddTarget((Scriptable *) ac, dist);
				}
			}
		}
	}
	return tgts;
}

Targets* GetAllObjects(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets* tgts = EvaluateObject(Sender, oC);
	//if we couldn't find an endpoint by name or object qualifiers
	//it is not an Actor, but could still be a Door or Container (scriptable)
	if (!tgts && oC->objectName[0]) {
		return NULL;
	}
	//now lets do the object filter stuff, we create Targets because
	//it is possible to start from blank sheets using endpoint filters
	//like (Myself, Protagonist etc)
	if (!tgts) {
		tgts = new Targets();
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printMessage("GameScript"," ", YELLOW);
			printf("Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if (!tgts->Count()) {
			delete tgts;
			return NULL;
		}
	}
	return tgts;
}

Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets *tgts = GetAllObjects(Sender, oC);
	if (tgts) {
		//now this could return other than actor objects
		Scriptable *object = tgts->GetTarget(0,-1);
		delete tgts;
		return object;
	}

	if (oC->objectName[0]) {
		Scriptable * aC = Sender->GetCurrentArea()->TMap->GetDoor( oC->objectName );
		if (aC) {
			return aC;
		}

		//No... it was not a door... maybe an InfoPoint?
		aC = Sender->GetCurrentArea()->TMap->GetInfoPoint( oC->objectName );
		if (aC) {
			return aC;
		}
		//No... it was not an infopoint... maybe a Container?
		aC = Sender->GetCurrentArea()->TMap->GetContainer( oC->objectName );
		if (aC) {
			return aC;
		}
	}
	return NULL;
}

/*FIXME: what is 'base'*/
void PolymorphCopyCore(Actor *src, Actor *tar, bool base)
{
	tar->SetBase(IE_ANIMATION_ID, src->GetStat(IE_ANIMATION_ID) );
	if (!base) {
		tar->SetBase(IE_ARMOR_TYPE, src->GetStat(IE_ARMOR_TYPE) );
		for (int i=0;i<7;i++) {
			tar->SetBase(IE_COLORS+i, src->GetStat(IE_COLORS+i) );
		}
	}
	//add more attribute copying
}

void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags)
{
	DataStream* ds;
	Scriptable *tmp = GetActorFromObject( Sender, parameters->objects[1] );

	if (flags & CC_STRING1) {
		ds = core->GetResourceMgr()->GetResource( parameters->string1Parameter, IE_CRE_CLASS_ID );
	}
	else {
		ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter, IE_CRE_CLASS_ID );
	}
	Actor *ab = core->GetCreature(ds);

	if (flags & CC_SCRIPTNAME) {
		ab->SetScriptName(parameters->string1Parameter);
	}

	int radius;
	Point pnt;

	radius=0;
	switch (flags & CC_MASK) {
		//creates creature just off the screen
		case CC_OFFSCREEN:
			{
			Region vp = core->GetVideoDriver()->GetViewport();
			radius=vp.w/2; //actually it must be further divided by the tile size, hmm 16?
			}
			//falling through
		case CC_OBJECT://use object + offset
			if (tmp) Sender=tmp;
			//falling through
		case CC_OFFSET://use sender + offset
			pnt.x = parameters->pointParameter.x+Sender->Pos.x;
			pnt.y = parameters->pointParameter.y+Sender->Pos.y;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt.x = parameters->pointParameter.x;
			pnt.y = parameters->pointParameter.y;
			if (pnt.isempty()) {
				pnt.x = Sender->Pos.x;
				pnt.y = Sender->Pos.y;
			}
			break;
	}

	printf("CreateCreature: %s at [%d.%d] face:%d\n",parameters->string0Parameter, pnt.x,pnt.y,parameters->int0Parameter);
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition(map, pnt, flags&CC_CHECK_IMPASSABLE, radius );
	ab->SetOrientation(parameters->int0Parameter, false );
	map->AddActor( ab );

	//if string1 is animation, then we can't use it for a DV too
	if (flags & CC_PLAY_ANIM) {
		CreateVisualEffectCore( ab, ab->Pos, parameters->string1Parameter);
	} else {
		//setting the deathvariable if it exists (iwd2)
		if (parameters->string1Parameter[0]) {
			ab->SetScriptName(parameters->string1Parameter);
		}
	}

	if (flags & CC_COPY) {
		if (tmp && tmp->Type == ST_ACTOR) {
			PolymorphCopyCore ( (Actor *) tmp, ab, false);
		}
	}
}

void CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect)
{
//TODO: add engine specific VVC replacement methods
//stick to object flag, sounds, iterations etc.
	if (effect[0]) {
		ScriptedAnimation* vvc = core->GetScriptedAnimation(effect);
		vvc->XPos +=position.x;
		vvc->YPos +=position.y;
		Sender->GetCurrentArea( )->AddVVCell( vvc );
	}
}

//this destroys the current actor and replaces it with another
void ChangeAnimationCore(Actor *src, const char *resref, bool effect)
{
	DataStream* ds = core->GetResourceMgr()->GetResource( resref, IE_CRE_CLASS_ID );
	Actor *tar = core->GetCreature(ds);
	if (tar) {
		Map *map = src->GetCurrentArea();
		tar->SetPosition(map, src->Pos, 1);
		tar->SetOrientation(src->GetOrientation(), false );
		map->AddActor( tar );
		src->DestroySelf();
		if (effect) {
			CreateVisualEffectCore(tar, tar->Pos,"smokepuffeffect");
		}
	}
}

void EscapeAreaCore(Actor* src, const char* resref, Point &enter, Point &exit, int flags)
{
	char Tmp[256];

	if (flags &EA_DESTROY) {
		//this must be put into a non-const variable
		sprintf( Tmp, "DestroySelf()" );
	} else {
		sprintf( Tmp, "JumpToPoint(\"%s\",[%hd.%hd])", resref, enter.x, enter.y );
	}
	src->AddActionInFront( GenerateAction( Tmp) );
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", exit.x, exit.y );
	src->AddActionInFront( GenerateAction( Tmp) );
}

void GetPositionFromScriptable(Scriptable* scr, Point &position, bool dest)
{
	if (!dest) {
		position = scr->Pos;
		return;
	}
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL:
			position = scr->Pos; //fake
			break;
		case ST_ACTOR:
		//if there are other moveables, put them here
			position = ((Moveble *) scr)->GetMostLikelyPosition();
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
	}
}

static ieResRef PlayerDialogRes = "PLAYERx\0";

void BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;

	if (InDebug&ID_VARIABLES) {
		printf("BeginDialog core\n");
	}
	if (Flags & BD_OWN) {
		scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		if (Flags & BD_NUMERIC) {
			//the target was already set, this is a crude hack
			tar = core->GetGameControl()->GetTarget();
		}
		else {
			tar = GetActorFromObject( Sender, parameters->objects[1] );
		}
		scr = Sender;
	}
	if (!scr || scr->Type != ST_ACTOR) {
		printf("[GameScript]: Speaker for dialog couldn't be found (Sender: %s, Type: %d).\n", Sender->GetScriptName(), Sender->Type);
		Sender->ReleaseCurrentAction();
		return;
	}

	if (!tar || tar->Type!=ST_ACTOR) {
		printf("[GameScript]: Target for dialog couldn't be found (Sender: %s, Type: %d).\n", Sender->GetScriptName(), Sender->Type);
		if (Sender->Type == ST_ACTOR) {
			((Actor *) Sender)->DebugDump();
		}
		printf ("Target object: ");
		if (parameters->objects[1]) {
			parameters->objects[1]->Dump();
		} else {
			printf("<NULL>\n");
		}
		Sender->ReleaseCurrentAction();
		return;
	}

	Actor *speaker, *target;

	speaker = (Actor *) scr;
	target = (Actor *) tar;
	if (!ValidForDialogCore(scr, target) ) {
		printf("ValidForDialogCore returned false! Speaker and target are:\n");
		((Actor *) scr)->DebugDump();
		((Actor *) tar)->DebugDump();
		Sender->ReleaseCurrentAction();
		return;
	}
	if (speaker->GetStat(IE_STATE_ID)&STATE_DEAD) {
		printf("Speaker is dead, cannot start dialogue. Speaker and target are:\n");
		((Actor *) scr)->DebugDump();
		((Actor *) tar)->DebugDump();
		Sender->ReleaseCurrentAction();
		return;
	}

	//CHECKDIST works only for mobile scriptables
	if (Flags&BD_CHECKDIST) {
		if (Distance(speaker, target)>speaker->GetStat(IE_DIALOGRANGE)*20 ) {
			GoNearAndRetry(Sender, tar, true); //this is unsure, the target's path will be cleared
			Sender->ReleaseCurrentAction();
			return;
		}
	}
	//making sure speaker is the protagonist, player, actor
	bool swap = false;
	if ( target->InParty == 1) swap = true;
	else if ( speaker->InParty !=1 && target->InParty) swap = true;

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		printMessage( "GameScript","Dialog cannot be initiated because there is no GameControl.", YELLOW );
		Sender->ReleaseCurrentAction();
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
			printMessage( "GameScript","Dialog cannot be initiated because there is already one.", YELLOW );
			Sender->ReleaseCurrentAction();
			return;
		}
	}

	const char* Dialog = NULL;
	int pdtable = -1;

	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG) {
				speaker->SetDialog( Dialog );
			}
			break;
		case BD_SOURCE:
		case BD_TARGET:
			if (swap) Dialog = speaker->Dialog;
			else Dialog = target->Dialog;
			break;
		case BD_RESERVED:
			//what if playerdialog was initiated from Player2?
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
		case BD_INTERACT: //using the source for the dialog
			pdtable = core->LoadTable( "interdia" );
			const char* scriptingname = ((Actor *) scr)->GetScriptName();
			//Dialog is a borrowed reference, we cannot free pdtable while it is being used
			if (pdtable!=-1) {
				Dialog = core->GetTable( pdtable )->QueryField( scriptingname, "FILE" );
			}
			break;
	}


	//dialog is not meaningful
	if (!Dialog || Dialog[0]=='*') {
		Sender->ReleaseCurrentAction();
		goto end_of_quest;
	}
	//maybe we should remove the action queue, but i'm unsure
	//no, we shouldn't even call this!
	//Sender->ReleaseCurrentAction();

	if (speaker!=target) {
		if (swap) {
			Actor *tmp = target;
			target = speaker;
			speaker = tmp;
		} else {
			if (!(Flags & BD_INTERRUPT)) {
				if (tar->GetNextAction()) {
					core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
					Sender->ReleaseCurrentAction();
					goto end_of_quest;
				}
			}
		}
		
	}

	//don't clear target's actions, because a sequence like this will be broken:
	//StartDialog([PC]); SetGlobal("Talked","LOCALS",1);
	speaker->ClearActions();
	speaker->SetOrientation(GetOrient( target->Pos, speaker->Pos), true);
	target->SetOrientation(GetOrient( speaker->Pos, target->Pos), true);

	if (Dialog[0]) {
		//increasing NumTimesTalkedTo or NumTimesInteracted
		if (Flags & BD_TALKCOUNT) {
			gc->SetDialogueFlags(DF_TALKCOUNT, BM_OR);
		} else if ((Flags & BD_LOCMASK) == BD_INTERACT) {
			gc->SetDialogueFlags(DF_INTERACT, BM_OR);
		}

		core->GetDictionary()->SetAt("DialogChoose",(ieDword) -1);
		gc->InitDialog( speaker, target, Dialog);
	}
//if pdtable was allocated, free it now, it will release Dialog
end_of_quest:
	if (pdtable!=-1) {
		core->DelTable( pdtable );
	}
}

void MoveBetweenAreasCore(Actor* actor, const char *area, Point &position, int face, bool adjust)
{
	printMessage("GameScript", " ", WHITE);
	printf("MoveBetweenAreas: %s to %s [%d.%d] face: %d\n", actor->GetName(0), area,position.x,position.y, face);
	Map* map2;
	Game* game = core->GetGame();
	if (area[0]) { //do we need to switch area?
		Map* map1 = actor->GetCurrentArea();
		//we have to change the pathfinder
		//to the target area if adjust==true
		map2 = game->GetMap(area, false);
		if ( map1!=map2 ) {
			if (map1) {
				map1->RemoveActor( actor );
			}
			map2->AddActor( actor );
		}
	}
	else {
		map2=actor->GetCurrentArea();
	}
	actor->SetPosition(map2, position, adjust);
	if (face !=-1) {
		actor->SetOrientation( face, false );
	}
	GameControl *gc=core->GetGameControl();
	gc->SetScreenFlags(SF_CENTERONACTOR,BM_OR);
}

void MoveToObjectCore(Scriptable *Sender, Action *parameters, ieDword flags)
{
	if (Sender->Type != ST_ACTOR) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Scriptable* target = GetActorFromObject( Sender, parameters->objects[1] );
	if (!target) {
		Sender->ReleaseCurrentAction();
		return;
	}
	Actor* actor = ( Actor* ) Sender;
	actor->WalkTo( target->Pos, flags, 0 );
}

void CreateItemCore(CREItem *item, const char *resref, int a, int b, int c)
{
		    strncpy(item->ItemResRef, resref, 8);
		    if (a==-1) {
		            Item *origitem = core->GetItem(resref);
		            for(int i=0;i<3;i++) {
		                    ITMExtHeader *e = origitem->GetExtHeader(i);              
		                    item->Usages[i]=e?e->Charges:0;
		            }
		            core->FreeItem(origitem, resref, false);
		    } else {
		            item->Usages[0]=(ieWord) a;
		            item->Usages[1]=(ieWord) b;
		            item->Usages[2]=(ieWord) c;
		    }
		    item->Flags=0;
}

//It is possible to attack CONTAINERS/DOORS as well!!!
void AttackCore(Scriptable *Sender, Scriptable *target, Action *parameters, int flags)
{
	//this is a dangerous cast, make sure actor is Actor * !!!
	Actor *actor = (Actor *) Sender;

	//replace the action for the xth time
	if (flags&AC_REEVALUATE) {
		if (!parameters->int0Parameter) {
			//dropping the action, since it has 0 refcount, we should
			//delete it instead of release otherwise we trigger a tripwire
			//if ParamCopy will ever create it with 1 refcount, we could
			//change this to a Release()
			delete parameters;
			return;
		}
		parameters->int0Parameter--;
		Sender->ReleaseCurrentAction();
		//parameters action was created from the scratch, by adding it, it will
		//have one RefCount
		parameters->IncRef();
		Sender->CurrentAction=parameters;
		//we call GoNearAndRetry
		//Sender->AddAction(parameters);
	}

	//if distance is too much, insert a move action and requeue the action
	ITMExtHeader *header;
	unsigned int wrange = actor->GetWeapon(header) * 10;
	if ( wrange == 0) {
		wrange = 10;
	}

	if (!(flags&AC_NO_SOUND) ) {
		if (target->Type!=ST_ACTOR || actor->LastTarget != ((Actor *) target)->GetID() ) {
			//play attack sound
			DisplayStringCore(Sender, VB_ATTACK, DS_CONSOLE|DS_CONST );
			//display attack message
			core->DisplayConstantStringAction(STR_ACTION_ATTACK,0xf0f0f0, Sender, target);
		}
	}
	//action performed
	actor->SetTarget( target );

	if ( PersonalDistance(Sender, target) > wrange ) {
		//we couldn't perform the action right now
		//so we add it back to the queue with an additional movement
		//increases refcount of Sender->CurrentAction, by pumping it back
		//in the action queue
		GoNearAndRetry(Sender, target, true);
		Sender->ReleaseCurrentAction();
		return;
	}
	//it shouldn't be a problem to call this the second time (in case of attackreevaluate)
	//because ReleaseCurrentAction() allows NULL 
	Sender->ReleaseCurrentAction();
}

bool MatchActor(Scriptable *Sender, ieDword actorID, Object* oC)
{
	if (!Sender || !oC) {
		return false;
	}
	Targets *tgts = GetAllObjects(Sender, oC);
	bool ret = false;

	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			Actor *actor = (Actor *) tt->actor;
			if (actor->GetID() == actorID) {
				ret = true;
				break;
			}
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return ret;
}

int GetObjectCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	// EvaluateObject will return [PC]
	// GetAllObjects will also return Myself (evaluates object filters)
	// i believe we need the latter here
	Targets* tgts = GetAllObjects(Sender, oC);
	int count = tgts->Count();
	delete tgts;
	return count;
}

//TODO:
//check numcreaturesatmylevel(myself, 1)
//when the actor is alone
//it should (obviously) return true if the trigger
//evaluates object filters
//also check numcreaturesgtmylevel(myself,0) with
//actor having at high level
int GetObjectLevelCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	// EvaluateObject will return [PC]
	// GetAllObjects will also return Myself (evaluates object filters)
	// i believe we need the latter here
	Targets* tgts = GetAllObjects(Sender, oC);
	int count = 0;
	if (tgts) {
		targetlist::iterator m;
		const targettype *tt = tgts->GetFirstTarget(m, ST_ACTOR);
		while (tt) {
			count += ((Actor *) tt->actor)->GetXPLevel(true);
			tt = tgts->GetNextTarget(m, ST_ACTOR);
		}
	}
	delete tgts;
	return count;
}

//we need this because some special characters like _ or * are also accepted
inline bool ismysymbol(const char letter)
{
	if (letter==']') return false;
	if (letter=='[') return false;
	if (letter==')') return false;
	if (letter=='(') return false;
	if (letter=='.') return false;
	if (letter==',') return false;
	return true;
}

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	SymbolMgr *valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		//FIXME:missing ids file!!!
		if (InDebug&ID_TRIGGERS) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Missing IDS file %s for symbol %s!\n",idsname, symbol);
		}
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}
	char symbolname[64];
	int x;
	for (x=0;ismysymbol(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for (int i=0;i<ObjectFieldsCount;i++) {
			object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if (*src!='.') {
			break;
		}
	}
	src++; //skipping ]
}

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while (*str && ( *str != ',' ) && ( *str != ')' )) str++

static void ParseObject(const char *&str,const char *&src, Object *&object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
	case '"':
		//Scriptable Name
		src++;
		int i;
		for (i=0;i<(int) sizeof(object->objectName)-1 && *src!='"';i++)
		{
			object->objectName[i] = *src;
			src++;
		}
		object->objectName[i] = 0;
		src++;
		break;
	case '[':
		src++; //skipping [
		ParseIdsTarget(src, object);
		break;
	default: //nested object filters
		int Nesting=0;
		
		while (Nesting<MaxObjectNesting) {
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
			if (*src!='(') {
				break;
			}
			src++; //skipping (
			if (*src==')') {
				break;
			}
			Nesting++;
		}
		if (*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
}

/* this function was lifted from GenerateAction, to make it clearer */
Action* GenerateActionCore(const char *src, const char *str, int acIndex)
{
	Action *newAction = new Action(true);
	newAction->actionID = (unsigned short) actionsTable->GetValueIndex( acIndex );
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
	int stringsCount = 0;
	int intCount = 0;
	if (actionflags[newAction->actionID]&AF_DIRECT) {
		Object *tmp = new Object();
		tmp->objectFields[0] = -1;
		tmp->objectFields[1] = core->GetGameControl()->targetID;
		newAction->objects[objectCount++] = tmp;
	}
	//Here is the Action; Now we need to evaluate the parameters, if any
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newAction->int0Parameter = value;
				} else if (intCount == 1) {
					newAction->int1Parameter = value;
				} else {
					newAction->int2Parameter = value;
				}
			}
			break;

			case 'a':
			//Action
			 {
				SKIP_ARGUMENT();
				char action[257];
				int i = 0;
				int openParenthesisCount = 0;
				while (true) {
					if (*src == ')') {
						if (!openParenthesisCount)
							break;
						openParenthesisCount--;
					} else {
						if (*src == '(') {
							openParenthesisCount++;
						} else {
							if (( *src == ',' ) &&
								!openParenthesisCount)
								break;
						}
					}
					action[i] = *src;
					i++;
					src++;
				}
				action[i] = 0;
				Action* act = GenerateAction( action);
				act->objects[0] = newAction->objects[0];
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if (objectCount==3) {
					printf("Invalid object count!\n");
					abort();
				}
				ParseObject(str, src, newAction->objects[objectCount++]);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newAction->string0Parameter;
				} else {
					dst = newAction->string1Parameter;
				}
				//if there are 3 strings, the first 2 will be merged,
				//the last one will be left alone
				if (*str==')') {
					mergestrings = 0;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter;
					} else {
						dst = newAction->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

void GoNearAndRetry(Scriptable *Sender, Scriptable *target, bool flag)
{
	Point p;
	GetPositionFromScriptable(target,p,flag);
	GoNearAndRetry(Sender, p);
}

void GoNearAndRetry(Scriptable *Sender, Point &p)
{
	if (!Sender->CurrentAction) {
		printMessage("GameScript","NULL action retried???\n",LIGHT_RED);
		return;
	}
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", p.x, p.y );
	Sender->AddActionInFront( GenerateAction( Tmp) );
}

void FreeSrc(SrcVector *poi, const ieResRef key)
{
	int res = SrcCache.DecRef((void *) poi, key, true);
	if (res<0) {
		printMessage( "GameScript", "Corrupted Src cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Src name is: %.8s\n", key);
		abort();
	}
	if (!res) {
		delete poi;	
	}
}

SrcVector *LoadSrc(const ieResRef resname)
{
	SrcVector *src = (SrcVector *) SrcCache.GetResource(resname);
	if (src) {
		return src;
	}
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_SRC_CLASS_ID );
	if ( !str) {
		return NULL;
	}
	ieDword size=0;
	str->ReadDword(&size);
	src = new SrcVector(size);
	SrcCache.SetAt( resname, (void *) src );
	while (size--) {
		ieDword tmp;
		str->ReadDword(&tmp);
		src->at(size)=tmp;
		str->ReadDword(&tmp);
	}
	delete ( str );
	return src;
}

#define MEMCPY(a,b) memcpy((a),(b),sizeof(a) )

static Object *ObjectCopy(Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	MEMCPY( newObject->objectFields, object->objectFields );
	MEMCPY( newObject->objectFilters, object->objectFilters );
	MEMCPY( newObject->objectRect, object->objectRect );
	MEMCPY( newObject->objectName, object->objectName );
	return newObject;
}

Action *ParamCopy(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

Action *ParamCopyNoOverride(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	newAction->objects[0]= NULL;
	newAction->objects[1]= ObjectCopy( parameters->objects[1] );
	newAction->objects[2]= ObjectCopy( parameters->objects[2] );
	return newAction;
}

Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate)
{
	Trigger *newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex( trIndex )&0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergestrings = triggerflags[newTrigger->triggerID]&TF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Trigger; Now we need to evaluate the parameters
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newTrigger->int0Parameter = value;
				} else if (intCount == 1) {
					newTrigger->int1Parameter = value;
				} else {
					newTrigger->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'o': //Object
				ParseObject(str, src, newTrigger->objectParameter);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newTrigger->string0Parameter;
				} else {
					dst = newTrigger->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = (char) tolower(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter;
					} else {
						dst = newTrigger->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
							*dst++ = (char) tolower(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value)
{
	char newVarName[8+33];

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s%s\", %d)\n", Context,
			VarName, value );
	}
	strncpy( newVarName, Context, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( VarName, value );
		return;
	}

	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( VarName, value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in setvariable\n",Context, VarName);
		}
	}
	else {
		game->locals->SetAt( VarName, ( ieDword ) value );
	}
}

void SetVariable(Scriptable* Sender, const char* VarName, ieDword value)
{
	char newVarName[8];

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s\", %d)\n", VarName, value );
	}
	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( &VarName[6], value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( &VarName[6], value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in setvariable\n",VarName);
		}
	}
	else {
		game->locals->SetAt( &VarName[6], ( ieDword ) value );
	}
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( &VarName[6], value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( &VarName[6], value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( &VarName[6], value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( &VarName[6], value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in checkvariable\n",VarName);
		}
	}
	else {
		game->locals->Lookup( &VarName[6], value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s: %d\n",VarName, value);
	}
	return value;
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy(newVarName, Context, 6);
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( VarName, value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
		}
	} else {
		game->locals->Lookup( VarName, value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s%s: %d\n",Context, VarName, value);
	}
	return value;
}

int DiffCore(ieDword a, ieDword b, int diffmode)
{
	switch (diffmode) {
		case LESS_THAN:
			if (a<b) {
				return 1;
			}
			break;
		case EQUALS:
			if (a==b) {
				return 1;
			}
			break;
		case GREATER_THAN:
			if (a>b) {
				return 1;
			}
			break;
		case GREATER_OR_EQUALS:
			if (a>=b) {
				return 1;
			}
			break;
		case NOT_EQUALS:
			if (a!=b) {
				return 1;
			}
			break;
		case BINARY_LESS_OR_EQUALS:
			if ((a&b) == a) {
				return 1;
			}
			break;
		case BINARY_MORE:
			if ((a&b) != a) {
				return 1;
			}
			break;
		case BINARY_MORE_OR_EQUALS:
			if ((a&b) == b) {
				return 1;
			}
			break;
		case BINARY_LESS:
			if ((a&b) != b) {
				return 1;
			}
			break;
		case BINARY_INTERSECT:
			if (a&b) {
				return 1;
			}
			break;
		case BINARY_NOT_INTERSECT:
			if (!(a&b)) {
				return 1;
			}
			break;
		default: //less or equals
			if (a<=b) {
				return 1;
			}
			break;
	}
	return 0;
}

Targets *GetMyTarget(Scriptable *Sender, Actor *actor, Targets *parameters)
{
	if (!actor) {
		if (Sender->Type==ST_ACTOR) {
			actor = (Actor *) Sender;
		}
	}
	parameters->Clear();
	if (actor) {
		Actor *target = actor->GetCurrentArea()->GetActorByGlobalID(actor->LastTarget);
		if (target) {
			parameters->AddTarget(target, 0);
		}
	}
	return parameters;
}

Targets *XthNearestDoor(Targets *parameters, unsigned int count)
{
	//get the origin
	Scriptable *origin = parameters->GetTarget(0, -1);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	//get the doors based on it
	Map *map = origin->GetCurrentArea();
	unsigned int i =(unsigned int) map->TMap->GetDoorCount();
	if (count>i) {
		return parameters;
	}
	while (i--) {
		Door *door = map->TMap->GetDoor(i);
		unsigned int dist = Distance(origin->Pos, door->Pos);
		parameters->AddTarget(door, dist);
	}

	//now get the xth door
	origin = parameters->GetTarget(count, ST_DOOR);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	parameters->AddTarget(origin, 0);
	return parameters;
}

Targets *XthNearestOf(Targets *parameters, int count)
{
	Scriptable *origin;

	if (count<0) {
		const targettype *t = parameters->GetLastTarget(ST_ACTOR);
		origin = t->actor;
	} else {
		origin = parameters->GetTarget(count, ST_ACTOR);
	}
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	parameters->AddTarget(origin, 0);
	return parameters;
}

Targets *XthNearestMyGroupOfType(Scriptable *origin, Targets *parameters, unsigned int count)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 0; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 1;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (t->actor->Type!=ST_ACTOR) {
			t=parameters->RemoveTargetAt(m);
			continue;
		}
		Actor *actor = (Actor *) (t->actor);
		if (type) { //origin is enemy, so we remove PC groups
			if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		else {
			if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	return XthNearestOf(parameters,count);
}

Targets *NearestEnemySummoned(Scriptable *origin, Targets *parameters)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	actor = NULL;
	while ( t ) {
		Actor *tmp = (Actor *) (t->actor);
		if (tmp->GetStat(IE_SEX) != SEX_SUMMON) {
			continue;
		}
		if (type) { //origin is PC
			if (tmp->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				continue;
			}
		} else {
			if (tmp->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				continue;
			}
		}
		actor = tmp;
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	parameters->Clear();
	parameters->AddTarget(actor, 0);
	return parameters;
}

Targets *XthNearestEnemyOfType(Scriptable *origin, Targets *parameters, unsigned int count)
{
	if (origin->Type != ST_ACTOR) {
		parameters->Clear();
		return parameters;
	}

	targetlist::iterator m;
	const targettype *t = parameters->GetFirstTarget(m, ST_ACTOR);
	if (!t) {
		return parameters;
	}
	Actor *actor = (Actor *) origin;
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		parameters->Clear();
		return parameters;
	}

	while ( t ) {
		if (t->actor->Type!=ST_ACTOR) {
			t=parameters->RemoveTargetAt(m);
			continue;
		}
		Actor *actor = (Actor *) (t->actor);
		if (type) { //origin is PC
			if (actor->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		} else {
			if (actor->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				t=parameters->RemoveTargetAt(m);
				continue;
			}
		}
		t = parameters->GetNextTarget(m, ST_ACTOR);
	}
	return XthNearestOf(parameters,count);
}

Targets *XthNearestEnemyOf(Targets *parameters, int count)
{
	Actor *origin = (Actor *) parameters->GetTarget(0, ST_ACTOR);
	parameters->Clear();
	if (!origin) {
		return parameters;
	}
	//determining the allegiance of the origin
	int type = 2; //neutral, has no enemies
	if (origin->GetStat(IE_EA) <= EA_GOODCUTOFF) {
		type = 1; //PC
	}
	if (origin->GetStat(IE_EA) >= EA_EVILCUTOFF) {
		type = 0;
	}
	if (type==2) {
		return parameters;
	}
	Map *map = origin->GetCurrentArea();
	int i = map->GetActorCount(true);
	Actor *ac;
	while (i--) {
		ac=map->GetActor(i,true);
		int distance = Distance(ac, origin);
		if (type) { //origin is PC
			if (ac->GetStat(IE_EA) >= EA_EVILCUTOFF) {
				parameters->AddTarget(ac, distance);
			}
		}
		else {
			if (ac->GetStat(IE_EA) <= EA_GOODCUTOFF) {
				parameters->AddTarget(ac, distance);
			}
		}
	}
	return XthNearestOf(parameters,count);
}
