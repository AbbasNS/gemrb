/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
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
 * $Id$
 *
 */

// This class handles the special spawn structures of planescape torment
// (stored in .ini format)

#include "../../includes/win32def.h"
#include "IniSpawn.h"
#include "Game.h"
#include "Actor.h"
#include "Map.h"
#include "ResourceMgr.h"
#include "Interface.h"
#include "GSUtils.h"

static const int StatValues[9]={
IE_EA, IE_FACTION, IE_TEAM, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, 
IE_SEX, IE_ALIGNMENT };

IniSpawn::IniSpawn(Map *owner)
{
	map = owner;
	NamelessSpawnArea[0] = 0;
	NamelessState = 35;
	eventspawns = NULL;
	eventcount = 0;
	last_spawndate = 0;
}

IniSpawn::~IniSpawn()
{
	if (eventspawns) {
		delete[] eventspawns;
	}
}

DataFileMgr *GetIniFile(const ieResRef DefaultArea)
{
	DataStream* inifile = core->GetResourceMgr()->GetResource( DefaultArea, IE_INI_CLASS_ID );
	if (!inifile) {
		printStatus( "NOT_FOUND", LIGHT_RED );
		return NULL;
	}
	if (!core->IsAvailable( IE_INI_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printMessage( "IniSpawn","No INI Importer Available.\n",LIGHT_RED );
		return NULL;
	}

	DataFileMgr *ini = ( DataFileMgr* )core->GetInterface( IE_INI_CLASS_ID );
	ini->Open(inifile, true ); //autofree
	return ini;
}

/*** initializations ***/

inline int CountElements(const char *s, char separator)
{
	int ret = 1;
	while(*s) {
		if (*s==separator) ret++;
		s++;
	}
	return ret;
}

inline void GetElements(const char *s, ieResRef *storage, int count)
{
	while(count--) {
		ieResRef *field = storage+count;
		strnuprcpy(*field, s, sizeof(ieResRef)-1);
		for(size_t i=0;i<sizeof(ieResRef) && (*field)[i];i++) {
			if ((*field)[i]==',') {
				(*field)[i]='\0';
				break;
			}
		}
		if (!count) break;
		while(*s && *s!=',') s++;
		s++;
		if (*s==' ') s++; //this is because there is one single screwed up entry in ar1100.ini
	}
}

inline void GetElements(const char *s, ieVariable *storage, int count)
{
	while(count--) {
		ieVariable *field = storage+count;
		strnuprcpy(*field, s, sizeof(ieVariable)-1);
		for(size_t i=0;i<sizeof(ieVariable) && (*field)[i];i++) {
			if ((*field)[i]==',') {
				(*field)[i]='\0';
				break;
			}
		}
		while(*s && *s!=',') s++;
		s++;
	}
}

// possible values implemented in DiffMode, but not needed here
// BINARY_LESS_OR_EQUALS 6 //(left has only bits in right)
// BINARY_MORE_OR_EQUALS 7 //(left has equal or more bits than right)
// BINARY_INTERSECT 8      //(left and right has at least one common bit)
// BINARY_NOT_INTERSECT 9  //(no common bits)
// BINARY_MORE 10          //left has more bits than right
// BINARY_LESS 11          //left has less bits than right

int IniSpawn::GetDiffMode(const char *keyword)
{
	if (keyword[0]==0) return NO_OPERATION; //-1
	if (!stricmp(keyword,"less_or_equal_to") ) return LESS_OR_EQUALS; //0 (gemrb ext)
	if (!stricmp(keyword,"equal_to") ) return EQUALS; // 1
	if (!stricmp(keyword,"less_than") ) return LESS_THAN; // 2
	if (!stricmp(keyword,"greater_than") ) return GREATER_THAN; //3
	if (!stricmp(keyword,"greater_or_equal_to") ) return GREATER_THAN; //4 (gemrb ext)
	if (!stricmp(keyword,"not_equal_to") ) return NOT_EQUALS; //5
	return NO_OPERATION;
}

//unimplemented tags:
// check_crowd
// good_mod, law_mod, lady_mod, murder_mod
// control_var
// spec_area
// spec_var_inc
// death_faction
// death_team
// check_by_view_port
// do_not_spawn
// time_of_day
// hold_selected_point_key
// inc_spawn_point_index
// Save_selected_point, Save_selected_facing
// find_safest_point
// exit
// spawn_time_of_day
// PST only
// auto_buddy
// detail_level
void IniSpawn::ReadCreature(DataFileMgr *inifile, const char *crittername, CritterEntry &critter)
{
	const char *s;
	int ps;
	
	memset(&critter,0,sizeof(critter));

	//all specvars are using global, but sometimes it is explicitly given
	s = inifile->GetKeyAsString(crittername,"spec_var",NULL);
	if (s && (strlen(s)>9) && s[6]==':' && s[7]==':') {
		strnuprcpy(critter.SpecContext, s, 6);
		strnlwrcpy(critter.SpecVar, s+8, 32);
	} else {
		strnuprcpy(critter.SpecContext, "GLOBAL", 6);
		strnlwrcpy(critter.SpecVar, s, 32);
	}

	//add this to specvar at each spawn
	ps = inifile->GetKeyAsInt(crittername,"spec_var_inc", 0);
	critter.SpecVarInc=ps;

	//use this value with spec_var_operation to determine spawn
	ps = inifile->GetKeyAsInt(crittername,"spec_var_value",0);
	critter.SpecVarValue=ps;
	//this operation uses DiffCore
	s = inifile->GetKeyAsString(crittername,"spec_var_operation","");
	critter.SpecVarOperator=GetDiffMode(s);
	//the amount of critters to spawn
	critter.TotalQuantity = inifile->GetKeyAsInt(crittername,"spec_qty",1);
	critter.SpawnCount = inifile->GetKeyAsInt(crittername,"create_qty",critter.TotalQuantity);

	//the creature resource(s)
	s = inifile->GetKeyAsString(crittername,"cre_file",NULL);
	if (s) {
		critter.creaturecount = CountElements(s,',');
		critter.CreFile=new ieResRef[critter.creaturecount];
		GetElements(s, critter.CreFile, critter.creaturecount);
	}
	s = inifile->GetKeyAsString(crittername,"point_select",NULL);
	
	if (s) {
		ps=s[0];
	} else {
		ps=0;
	}

	s = inifile->GetKeyAsString(crittername,"spawn_point",NULL);
	if (s) {
		//expect more than one spawnpoint
		if (ps=='r') {
			//select one of the spawnpoints randomly
			int count = core->Roll(1,CountElements(s,']'),-1);
			//go to the selected spawnpoint
			while(count--) {
				while(*s++!=']');
			}
		}
		//parse the selected spawnpoint
		int x,y,o;
		if (sscanf(s,"[%d.%d:%d]", &x, &y, &o)==3) {
			critter.SpawnPoint.x=(short) x;
			critter.SpawnPoint.y=(short) y;
			critter.Orientation=o;
		}
	}
	
	//store or retrieve spawn point
	s = inifile->GetKeyAsString(crittername,"spawn_point_global", NULL);
	if (s) {
		switch (ps) {
		case 'e':
			critter.SpawnPoint.fromDword(CheckVariable(map, s+8,s));
			break;
		default:
			SetVariable(map, s+8, s, critter.SpawnPoint.asDword());
			break;
		}
	}

	//take facing from variable
	s = inifile->GetKeyAsString(crittername,"spawn_facing_global", NULL);
	if (s) {
		switch (ps) {
		case 'e':
			critter.Orientation=(int) CheckVariable(map, s+8,s);
			break;
		default:
			SetVariable(map, s+8, s, (ieDword) critter.Orientation);
			break;
		}
	}

	//sometimes only the orientation is given, the point is stored in a variable
	ps = inifile->GetKeyAsInt(crittername,"facing",-1);
	if (ps!=-1) critter.Orientation = ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_ea",-1);
	if (ps!=-1) critter.SetSpec[AI_EA] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_team",-1);
	if (ps!=-1) critter.SetSpec[AI_TEAM] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_general",-1);
	if (ps!=-1) critter.SetSpec[AI_GENERAL] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_race",-1);
	if (ps!=-1) critter.SetSpec[AI_RACE] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_class",-1);
	if (ps!=-1) critter.SetSpec[AI_CLASS] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_specifics",-1);
	if (ps!=-1) critter.SetSpec[AI_SPECIFICS] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_gender",-1);
	if (ps!=-1) critter.SetSpec[AI_GENDER] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_alignment",-1);
	if (ps!=-1) critter.SetSpec[AI_ALIGNMENT] = (ieByte) ps;

	s = inifile->GetKeyAsString(crittername,"spec",NULL);
	int x[9];

	ps = sscanf(s,"[%d.%d.%d.%d.%d.%d.%d.%d.%d]", x, x+1, x+2, x+3, x+4, x+5,
		x+6, x+7, x+8);
	while(ps--) {
		critter.Spec[ps]=(ieByte) x[ps];
	}

	s = inifile->GetKeyAsString(crittername,"script_name",NULL);
	if (s) {
		strnuprcpy(critter.ScriptName, s, 32);
	}

	//iwd2 script names
	//todo
	s = inifile->GetKeyAsString(crittername,"script_special_1",NULL);
	if (s) {
		strnuprcpy(critter.GeneralScript,s, 8);
	}
	//todo
	s = inifile->GetKeyAsString(crittername,"script_special_2",NULL);
	if (s) {
		strnuprcpy(critter.OverrideScript,s, 8);
	}
	//todo
	s = inifile->GetKeyAsString(crittername,"script_special_3",NULL);
	if (s) {
		strnuprcpy(critter.DefaultScript,s, 8);
	}
	//todo
	s = inifile->GetKeyAsString(crittername,"script_team",NULL);
	if (s) {
		strnuprcpy(critter.ClassScript,s, 8);
	}

	//combat == race
	s = inifile->GetKeyAsString(crittername,"script_combat",NULL);
	if (s) {
		strnuprcpy(critter.RaceScript,s, 8);
	}
	//todo
	s = inifile->GetKeyAsString(crittername,"script_movement",NULL);
	if (s) {
		strnuprcpy(critter.SpecificScript,s, 8);
	}

	//pst script names
	s = inifile->GetKeyAsString(crittername,"script_override",NULL);
	if (s) {
		strnuprcpy(critter.OverrideScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_class",NULL);
	if (s) {
		strnuprcpy(critter.ClassScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_race",NULL);
	if (s) {
		strnuprcpy(critter.RaceScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_general",NULL);
	if (s) {
		strnuprcpy(critter.GeneralScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_default",NULL);
	if (s) {
		strnuprcpy(critter.DefaultScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_area",NULL);
	if (s) {
		strnuprcpy(critter.AreaScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_specifics",NULL);
	if (s) {
		strnuprcpy(critter.SpecificScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"dialog",NULL);
	if (s) {
		strnuprcpy(critter.Dialog,s, 8);
	}

	//flags
	if (inifile->GetKeyAsBool(crittername,"death_scriptname",false)) {
		critter.Flags|=CF_DEATHVAR;
	}
	if (inifile->GetKeyAsBool(crittername,"ignore_can_see",false)) {
		critter.Flags|=CF_IGNORENOSEE;
	}
	if (inifile->GetKeyAsBool(crittername,"check_view_port", false)) {
		critter.Flags|=CF_CHECKVIEWPORT;
	}
	if (inifile->GetKeyAsBool(crittername,"check_crowd", false)) {
		critter.Flags|=CF_CHECKCROWD;
	}
	if (inifile->GetKeyAsBool(crittername,"find_safest_point", false)) {
		critter.Flags|=CF_SAFESTPOINT;
	}
	if (inifile->GetKeyAsBool(crittername,"area_diff_1", false)) {
		critter.Flags|=CF_NO_DIFF_1;
	}
	if (inifile->GetKeyAsBool(crittername,"area_diff_2", false)) {
		critter.Flags|=CF_NO_DIFF_2;
	}
	if (inifile->GetKeyAsBool(crittername,"area_diff_3", false)) {
		critter.Flags|=CF_NO_DIFF_3;
	}
}

void IniSpawn::ReadSpawnEntry(DataFileMgr *inifile, const char *entryname, SpawnEntry &entry)
{
	const char *s;
	
	entry.interval = (unsigned int) inifile->GetKeyAsInt(entryname,"interval",0);
	s = inifile->GetKeyAsString(entryname,"critters",NULL);
	int crittercount = CountElements(s,',');
	entry.crittercount=crittercount;
	entry.critters=new CritterEntry[crittercount];
	ieVariable *critters = new ieVariable[crittercount];
	GetElements(s, critters, crittercount);
	while(crittercount--) {
		ReadCreature(inifile, critters[crittercount], entry.critters[crittercount]);
	}
	delete[] critters;
}

void IniSpawn::InitSpawn(const ieResRef DefaultArea)
{
	DataFileMgr *inifile;
	const char *s;

	inifile = GetIniFile(DefaultArea);
	if (!inifile) {
		strnuprcpy(NamelessSpawnArea, DefaultArea, 8);
		return;
	}

	s = inifile->GetKeyAsString("nameless","destare",DefaultArea);
	strnuprcpy(NamelessSpawnArea, s, 8);
	s = inifile->GetKeyAsString("nameless","point","[0.0]");
	int x,y;
	if (sscanf(s,"[%d.%d]", &x, &y)!=2) {
		x=0;
		y=0;
	}
	NamelessSpawnPoint.x=x;
	NamelessSpawnPoint.y=y;
	//35 - already standing
	//36 - getting up
	NamelessState = inifile->GetKeyAsInt("nameless","state",35);

	s = inifile->GetKeyAsString("spawn_main","enter",NULL);
	if (s) {
		ReadSpawnEntry(inifile, s, enterspawn);
	}
	s = inifile->GetKeyAsString("spawn_main","events",NULL);
	if (s) {
		eventcount = CountElements(s,',');
		eventspawns = new SpawnEntry[eventcount];
		ieVariable *events = new ieVariable[eventcount];
		GetElements(s, events, eventcount);
		int ec = eventcount;
		while(ec--) {
			ReadSpawnEntry(inifile, events[ec], eventspawns[ec]);
		}
		delete[] events;
	}
	core->FreeInterface(inifile);
	//maybe not correct
	InitialSpawn();
}


/*** events ***/

//respawn nameless after he bit the dust
void IniSpawn::RespawnNameless()
{
	Game *game = core->GetGame();
	Actor *nameless = game->FindPC(1);

	if (NamelessSpawnPoint.isnull()) {
		core->GetGame()->JoinParty(nameless,JP_INITPOS);
		NamelessSpawnPoint=nameless->Pos;
	}

	nameless->Resurrect();
	//hardcoded!!!
	if (NamelessState==36) {
		nameless->SetStance(IE_ANI_PST_START);
	}
	for (int i=0;i<game->GetPartySize(false);i++) {
		MoveBetweenAreasCore(game->GetPC(i, false),NamelessSpawnArea,NamelessSpawnPoint,-1, true);
	}
}

void IniSpawn::SpawnCreature(CritterEntry &critter)
{
	ieDword specvar = CheckVariable(map, critter.SpecVar, critter.SpecContext);

	if (critter.SpecVar[0]) {
		if (critter.SpecVarOperator>=0) {
			// dunno if this should be negated
			if (DiffCore(specvar, critter.SpecVarValue, critter.SpecVarOperator) ) {
				return;
			}
		} else {
			if (!specvar) {
				return;
			}
		}
	}

	if (critter.ScriptName[0]) {
		//maybe this one needs to be using getobjectcount as well
		//currently we cannot count objects with scriptname???
		if (map->GetActor( critter.ScriptName, 0 )) {
			return;
		}
	} else {
		//Object *object = new Object();
		Object object;
		//objectfields based on spec
		object.objectFields[0]=critter.Spec[0];
		object.objectFields[1]=critter.Spec[1];
		object.objectFields[2]=critter.Spec[2];
		object.objectFields[3]=critter.Spec[3];
		object.objectFields[4]=critter.Spec[4];
		object.objectFields[5]=critter.Spec[5];
		object.objectFields[6]=critter.Spec[6];
		object.objectFields[7]=critter.Spec[7];
		object.objectFields[8]=critter.Spec[8];
		int cnt = GetObjectCount(map, &object);
		if (cnt>=critter.TotalQuantity) {
			return;
		}
	}

	int x = core->Roll(1,critter.creaturecount,-1);
	DataStream *stream = core->GetResourceMgr()->GetResource( critter.CreFile[x], IE_CRE_CLASS_ID );
	if (!stream) {
		return;
	}
	Actor* cre = core->GetCreature(stream);
	if (!cre) {
		return;
	}

	SetVariable(map, critter.SpecVar, critter.SpecContext, specvar+(ieDword) critter.SpecVarInc);
	map->AddActor(cre);
	for (x=0;x<9;x++) {
		if (critter.SetSpec[x]) {
			cre->SetBase(StatValues[x], critter.SetSpec[x]);
		}
	}
	cre->SetPosition( critter.SpawnPoint, 0, 0);//maybe critters could be repositioned
	cre->SetOrientation(critter.Orientation,false);
	if (critter.ScriptName[0]) {
		cre->SetScriptName(critter.ScriptName);
	}
	if (critter.OverrideScript[0]) {
		cre->SetScript(critter.OverrideScript, SCR_OVERRIDE);
	}
	if (critter.ClassScript[0]) {
		cre->SetScript(critter.ClassScript, SCR_CLASS);
	}
	if (critter.RaceScript[0]) {
		cre->SetScript(critter.RaceScript, SCR_RACE);
	}
	if (critter.GeneralScript[0]) {
		cre->SetScript(critter.GeneralScript, SCR_GENERAL);
	}
	if (critter.DefaultScript[0]) {
		cre->SetScript(critter.DefaultScript, SCR_DEFAULT);
	}
	if (critter.AreaScript[0]) {
		cre->SetScript(critter.AreaScript, SCR_AREA);
	}
	if (critter.SpecificScript[0]) {
		cre->SetScript(critter.SpecificScript, SCR_SPECIFICS);
	}
	if (critter.Dialog[0]) {
		cre->SetDialog(critter.Dialog);
	}
}

void IniSpawn::SpawnGroup(SpawnEntry &event)
{
	if (!event.critters) {
		return;
	}
	unsigned int interval = event.interval;
	if (interval) {
		if(core->GetGame()->GameTime/interval<=last_spawndate/interval) {
			return;
		}
	}
	last_spawndate=core->GetGame()->GameTime;
	
	for(int i=0;i<event.crittercount;i++) {
		CritterEntry* critter = event.critters+i;
		for(int j=0;j<critter->SpawnCount;j++) {
			SpawnCreature(*critter);
		}
	}
}

//execute the initial spawn
void IniSpawn::InitialSpawn()
{
	SpawnGroup(enterspawn);
}

//checks if a respawn event occurred
void IniSpawn::CheckSpawn()
{
	for(int i=0;i<eventcount;i++) {
		SpawnGroup(eventspawns[i]);
	}
}
