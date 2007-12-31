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
 * $Id$
 *
 */

#include <stdio.h>
#include "Interface.h"
#include "Actor.h"
#include "Effect.h"
#include "EffectQueue.h"
#include "SymbolMgr.h"
#include "Game.h"
#include "Map.h"

static int initialized = 0;
static EffectRef *effectnames = NULL;
static EffectRef effect_refs[MAX_EFFECTS];

static int opcodes_count = 0;

bool EffectQueue::match_ids(Actor *target, int table, ieDword value)
{
	if (value == 0) {
		return true;
	}

	int a, stat;

	switch (table) {
	case 2: //EA
		stat = IE_EA; break;
	case 3: //GENERAL
		stat = IE_GENERAL; break;
	case 4: //RACE
		stat = IE_RACE; break;
	case 5: //CLASS
		stat = IE_CLASS; break;
	case 6: //SPECIFIC
		stat = IE_SPECIFIC; break;
	case 7: //GENDER
		stat = IE_SEX; break;
	case 8: //ALIGNMENT
		stat = target->GetStat(IE_ALIGNMENT);
		a = value&15;
		if (a) {
			if (a != ( stat & 15 )) {
				return false;
			}
		}
		a = value & 0xf0;
		if (a) {
			if (a != ( stat & 0xf0 )) {
				return false;
			}
		}
		return true;
	default:
		return false;
	}
	if (target->GetStat(stat)==value) {
		return true;
	}
	return false;
}

static const bool fx_instant[MAX_TIMING_MODE]={true,true,true,false,false,false,false,false,true,true,true};

inline bool IsInstant(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_instant[timingmode];
}
//                                               0    1     2     3    4    5    6     7       8   9     10
static const bool fx_relative[MAX_TIMING_MODE]={true,false,false,true,true,true,false,false,false,false,false};

inline bool NeedPrepare(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_relative[timingmode];
}

#define INVALID  -1
#define PERMANENT 0
#define DELAYED   1
#define DURATION  2

static const int fx_prepared[MAX_TIMING_MODE]={DURATION,PERMANENT,PERMANENT,DELAYED, //0-3
DELAYED,DELAYED,DELAYED,DELAYED,PERMANENT,PERMANENT,PERMANENT};                //4-7

inline int IsPrepared(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return INVALID;
	return fx_prepared[timingmode];
}

//change the timing method after the effect triggered
static const ieByte fx_triggered[MAX_TIMING_MODE]={FX_DURATION_JUST_EXPIRED,FX_DURATION_INSTANT_PERMANENT,//0,1
FX_DURATION_INSTANT_WHILE_EQUIPPED,FX_DURATION_DELAY_LIMITED_PENDING,//2,3
FX_DURATION_AFTER_EXPIRES,FX_DURATION_PERMANENT_UNSAVED, //4,5
FX_DURATION_INSTANT_LIMITED,FX_DURATION_JUST_EXPIRED,FX_DURATION_PERMANENT_UNSAVED,//6,8
FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES,FX_DURATION_JUST_EXPIRED};//9,10

//change the timing method for effect that should trigger after this effect expired
static const ieDword fx_to_delayed[]={FX_DURATION_AFTER_EXPIRES,FX_DURATION_JUST_EXPIRED,
FX_DURATION_PERMANENT_UNSAVED,FX_DURATION_DELAY_LIMITED_PENDING,
FX_DURATION_AFTER_EXPIRES,FX_DURATION_PERMANENT_UNSAVED, //4,5
FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED,//6,8
FX_DURATION_JUST_EXPIRED,FX_DURATION_JUST_EXPIRED};//9,10

inline ieByte TriggeredEffect(ieByte timingmode)
{
	if (timingmode>=MAX_TIMING_MODE) return false;
	return fx_triggered[timingmode];
}

int compare_effects(const void *a, const void *b)
{
	return stricmp(((EffectRef *) a)->Name,((EffectRef *) b)->Name);
}

int find_effect(const void *a, const void *b)
{
	return stricmp((const char *) a,((const EffectRef *) b)->Name);
}

static EffectRef* FindEffect(const char* effectname)
{
	if (!effectname || !effectnames) {
		return NULL;
	}
	void *tmp = bsearch(effectname, effectnames, opcodes_count, sizeof(EffectRef), find_effect);
	if (!tmp) {
		printf( "Warning: Couldn't assign effect: %s\n", effectname );
	}
	return (EffectRef *) tmp;
}

static EffectRef fx_protection_from_display_string_ref={"Protection:String",NULL,-1};

//special effects without level check (but with damage dices precalculated)
static EffectRef diced_effects[] = {
	//core effects
	{"Damage",NULL,-1},
	{"CurrentHPModifier",NULL,-1},
	{"MaximumHPModifier",NULL,-1},
	//iwd effects
	{"BurningBlood",NULL,-1}, //iwd
	{"ColdDamage",NULL,-1},
	{"CrushingDamage",NULL,-1},
	{"VampiricTouch",NULL,-1},
	{"VitriolicSphere",NULL,-1},
	//pst effects
	{"TransferHP",NULL,-1},
{NULL,NULL,0} };

//special effects without level check (but with damage dices not precalculated)
static EffectRef diced_effects2[] = {
	{"BurningBlood2",NULL,-1}, //how/iwd2
	{"StaticCharge",NULL,-1}, //how/iwd2
	{"LichTouch",NULL,-1}, //how
{NULL,NULL,0} };

inline static void ResolveEffectRef(EffectRef &effect_reference)
{
	if (effect_reference.EffText==-1) {
		EffectRef* ref = FindEffect(effect_reference.Name);
		if (ref && ref->EffText>=0) {
			effect_reference.EffText = ref->EffText;
			return;
		}
		effect_reference.EffText = -2;
	}
}

bool Init_EffectQueue()
{
	int i;

	if (initialized) {
		return true;
	}
	TableMgr* efftextTable=NULL;
	SymbolMgr* effectsTable;
	memset( effect_refs, 0, sizeof( effect_refs ) );
	for(i=0;i<MAX_EFFECTS;i++) {
		effect_refs[i].EffText=-1;
	}

	initialized = 1;

	int effT = core->LoadTable( "efftext" );
	efftextTable = core->GetTable( effT );

	int eT = core->LoadSymbol( "effects" );
	if (eT < 0) {
		printMessage( "EffectQueue","A critical scripting file is missing!\n",LIGHT_RED );
		return false;
	}
	effectsTable = core->GetSymbol( eT );
	if (!effectsTable) {
		printMessage( "EffectQueue","A critical scripting file is damaged!\n",LIGHT_RED );
		return false;
	}

	for (i = 0; i < MAX_EFFECTS; i++) {
		const char* effectname = effectsTable->GetValue( i );
		if (efftextTable) {
			int row = efftextTable->GetRowCount();
			while (row--) {
				const char* ret = efftextTable->GetRowName( row );
				long val;
				if (valid_number( ret, val ) && (i == val) ) {
					effect_refs[i].EffText = atoi( efftextTable->QueryField( row, 1 ) );
				}
			}
		}

		EffectRef* poi = FindEffect( effectname );
		if (poi != NULL) {
			effect_refs[i].Function = poi->Function;
			effect_refs[i].Name = poi->Name;
			//reverse linking opcode number
			//using this unused field
			if ((poi->EffText!=-1) && effectname[0]!='*') {
				printf("Clashing opcodes FN: %d vs. %d, %s\n", i, poi->EffText, effectname);
				abort();
			}
			poi->EffText = i;
		}
		//printf("-------- FN: %d, %s\n", i, effectname);
	}
	core->DelSymbol( eT );
	if ( efftextTable ) core->DelTable( effT );

	//additional initialisations
	for (i=0;diced_effects[i].Name;i++) {
		ResolveEffectRef(diced_effects[i]);
	}
	for (i=0;diced_effects2[i].Name;i++) {
		ResolveEffectRef(diced_effects2[i]);
	}

	return true;
}

void EffectQueue_ReleaseMemory()
{
	if (effectnames) {
		free (effectnames);
	}
	opcodes_count = 0;
	effectnames = NULL;
}

void EffectQueue_RegisterOpcodes(int count, const EffectRef* opcodes)
{
	if (! effectnames) {
		effectnames = (EffectRef*) malloc( (count+1) * sizeof( EffectRef ) );
	} else {
		effectnames = (EffectRef*) realloc( effectnames, (opcodes_count + count + 1) * sizeof( EffectRef ) );
	}

	memcpy( effectnames + opcodes_count, opcodes, count * sizeof( EffectRef ));
	opcodes_count += count;
	effectnames[opcodes_count].Name = NULL;
	//if we merge two effect lists, then we need to sort their effect tables
	//actually, we might always want to sort this list, so there is no
	//need to do it manually (sorted table is needed if we use bsearch)
	qsort(effectnames, opcodes_count, sizeof(EffectRef), compare_effects);
}

EffectQueue::EffectQueue()
{
	Owner = NULL;
}

EffectQueue::~EffectQueue()
{
	std::list< Effect* >::iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		delete (*f);
		//delete( effects[i] );
	}
}

Effect *EffectQueue::CreateEffect(ieDword opcode, ieDword param1, ieDword param2, ieDword timing)
{
	if (opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if (!fx) {
		return NULL;
	}
	memset(fx,0,sizeof(Effect));
	fx->Target=FX_TARGET_SELF;
	fx->Opcode=opcode;
	//probability2 is the low number (by effectqueue 331)
	fx->Probability1=100;
	fx->Parameter1=param1;
	fx->Parameter2=param2;
	fx->TimingMode=timing;
	fx->PosX=0xffffffff;
	fx->PosY=0xffffffff;
	return fx;
}

ieDword EffectQueue::CountEffects(EffectRef &effect_reference, ieDword param1, ieDword param2, const char *resource) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return 0;
	}
	return CountEffects(effect_reference.EffText, param1, param2, resource);
}

void EffectQueue::ModifyEffectPoint(EffectRef &effect_reference, ieDword x, ieDword y)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return;
	}
	ModifyEffectPoint(effect_reference.EffText, x, y);
}

Effect *EffectQueue::CreateEffect(EffectRef &effect_reference, ieDword param1, ieDword param2, ieDword timing)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return CreateEffect(effect_reference.EffText, param1, param2, timing);
}

Effect *EffectQueue::CreateEffectCopy(Effect *oldfx, ieDword opcode, ieDword param1, ieDword param2)
{
	if (opcode==0xffffffff) {
		return NULL;
	}
	Effect *fx = new Effect();
	if (!fx) {
		return NULL;
	}
	memcpy(fx,oldfx,sizeof(Effect) );
	fx->Opcode=opcode;
	fx->Parameter1=param1;
	fx->Parameter2=param2;
	return fx;
}

Effect *EffectQueue::CreateEffectCopy(Effect *oldfx, EffectRef &effect_reference, ieDword param1, ieDword param2)
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return CreateEffectCopy(oldfx, effect_reference.EffText, param1, param2);
}

void EffectQueue::AddEffect(Effect* fx, bool insert)
{
	Effect* new_fx = new Effect;
	memcpy( new_fx, fx, sizeof( Effect ) );
	if (insert) {
		effects.insert( effects.begin(), new_fx );
	} else {
		effects.push_back( new_fx );
	}
}

bool EffectQueue::RemoveEffect(Effect* fx)
{
	int invariant_size = offsetof( Effect, random_value );

	for (std::list< Effect* >::iterator f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx2 = *f;

		//TODO:
		//equipped effects do not have point at removal
		if ( (fx==fx2) || !memcmp( fx, fx2, invariant_size)) {
			delete fx2;
			effects.erase( f );
			return true;
		}
	}
	return false;
}

//this is where we reapply all effects when loading a saved game
//The effects are already in the fxqueue of the target
void EffectQueue::ApplyAllEffects(Actor* target)
{
	//nah, we already been through this
	//ieDword random_value = core->Roll( 1, 100, 0 );

	std::list< Effect* >::iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		ApplyEffect( target, *f, 0 );
	}
	for ( f = effects.begin(); f != effects.end(); ) {
		if ((*f)->TimingMode==FX_DURATION_JUST_EXPIRED) {
			delete *f;
			effects.erase(f++);
		} else {
			f++;
		}
	}
}

int EffectQueue::AddEffect(Effect* fx, Actor* self, Actor* pretarget, Point &dest)
{
	int i;
	Game *game;
	Map *map;
	int flg;

	switch (fx->Target) {
	case FX_TARGET_ORIGINAL:
		fx->PosX=self->Pos.x;
		fx->PosY=self->Pos.y;
		flg = self->fxqueue.ApplyEffect( self, fx, 1 );
		if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
			self->fxqueue.AddEffect( fx, flg==FX_INSERT );
		}
		break;
	case FX_TARGET_SELF:
		fx->PosX=dest.x;
		fx->PosY=dest.y;
		flg = self->fxqueue.ApplyEffect( self, fx, 1 );
		if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
			self->fxqueue.AddEffect( fx, flg==FX_INSERT );
		}
		break;

	case FX_TARGET_PRESET:
		fx->PosX=pretarget->Pos.x;
		fx->PosY=pretarget->Pos.y;
		flg = self->fxqueue.ApplyEffect( pretarget, fx, 1 );
		if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
			pretarget->fxqueue.AddEffect( fx, flg==FX_INSERT );
		}
		break;

	case FX_TARGET_PARTY:
		game=core->GetGame();
		for (i = game->GetPartySize(true); i >= 0; i--) {
			Actor* actor = game->GetPC( i, true );
			fx->PosX=actor->Pos.x;
			fx->PosY=actor->Pos.y;
			flg = self->fxqueue.ApplyEffect( actor, fx, 1 );
			if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		flg = FX_APPLIED;
		break;

	case FX_TARGET_GLOBAL_INCL_PARTY:
		map=self->GetCurrentArea();
		for (i = map->GetActorCount(true)-1; i >= 0; i--) {
			Actor* actor = map->GetActor( i, true );
			fx->PosX=actor->Pos.x;
			fx->PosY=actor->Pos.y;
			flg = self->fxqueue.ApplyEffect( actor, fx, 1 );
			if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		flg = FX_APPLIED;
		break;

	case FX_TARGET_GLOBAL_EXCL_PARTY:
		map=self->GetCurrentArea();
		for (i = map->GetActorCount(false); i >= 0; i--) {
			Actor* actor = map->GetActor( i, false );
			fx->PosX=actor->Pos.x;
			fx->PosY=actor->Pos.y;
			flg = self->fxqueue.ApplyEffect( actor, fx, 1 );
			//GetActorCount can now return all nonparty critters
			//if (actor->InParty) continue;
			if (fx->TimingMode!=FX_DURATION_JUST_EXPIRED) {
				actor->fxqueue.AddEffect( fx, flg==FX_INSERT );
			}
		}
		flg = FX_APPLIED;
		break;

	case FX_TARGET_UNKNOWN:
	default:
		printf( "Unknown FX target type: %d\n", fx->Target);
		flg = FX_ABORT;
		break;
	}

	return flg;
}

//this is where effects from spells first get in touch with the target
//the effects are currently NOT in the target's fxqueue, those that stick
//will get copied (hence the fxqueue.AddEffect call)
void EffectQueue::AddAllEffects(Actor* target, Point &destination)
{
	// pre-roll dice for fx needing them and stow them in the effect
	ieDword random_value = core->Roll( 1, 100, 0 );

	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//handle resistances and saving throws here
		(*f)->random_value = random_value;
		//if applyeffect returns true, we stop adding the future effects
		//this is to simulate iwd2's on the fly spell resistance
		if(AddEffect(*f, Owner, target, destination)==FX_ABORT) {		
			break;
		}
	}
}

inline static bool IsDicedEffect(int opcode)
{
	int i;

	for(i=0;diced_effects[i].Name;i++) {
		if(diced_effects[i].EffText==opcode) {
			return true;
		}
	}
	return false;
}

inline static bool IsDicedEffect2(int opcode)
{
	int i;

	for(i=0;diced_effects2[i].Name;i++) {
		if(diced_effects2[i].EffText==opcode) {
			return true;
		}
	}
	return false;
}

//resisted effect based on level
inline bool check_level(Actor *target, Effect *fx)
{
	//skip non level based effects
	if (IsDicedEffect((int) fx->Opcode)) {
		fx->Parameter1 = DICE_ROLL((signed)fx->Parameter1);
		//this is a hack for PST style diced effects
		if (core->HasFeature(GF_SAVE_FOR_HALF) ) {
			if (memcmp(fx->Resource,"NEG",4) ) {
				fx->IsSaveForHalfDamage=1;
			}
		} else {
			if ((fx->Parameter2&3)==3) {
				fx->IsSaveForHalfDamage=1;
			}
		}
		return false;
	}
	if (IsDicedEffect2((int) fx->Opcode)) {
		return false;
	}
	ieDword level = (ieDword) target->GetXPLevel( true );
	if ((fx->DiceSides != 0 && fx->DiceThrown != 0) && (level > fx->DiceSides || level < fx->DiceThrown)) {
		return true;
	}
	return false;
}

inline bool check_probability(Effect* fx)
{
	//watch for this, probability1 is the high number
	//probability2 is the low number
	//random value is 1-100
	if (fx->random_value<=fx->Probability2 || fx->random_value>fx->Probability1) {
		return false;
	}
	return true;
}

//immunity effects
static EffectRef fx_level_immunity_ref={"Protection:Spelllevel",NULL,-1};
static EffectRef fx_opcode_immunity_ref={"Protection:Opcode",NULL,-1}; //bg2
static EffectRef fx_opcode_immunity2_ref={"Protection:Opcode2",NULL,-1};//iwd
static EffectRef fx_spell_immunity_ref={"Protection:Spell",NULL,-1}; //bg2
static EffectRef fx_spell_immunity2_ref={"Protection:Spell2",NULL,-1};//iwd
static EffectRef fx_school_immunity_ref={"Protection:School",NULL,-1};
static EffectRef fx_secondary_type_immunity_ref={"Protection:SecondaryType",NULL,-1};

//decrementing immunity effects
static EffectRef fx_level_immunity_dec_ref={"Protection:SpellLevelDec",NULL,-1};
static EffectRef fx_spell_immunity_dec_ref={"Protection:SpellDec",NULL,-1};
static EffectRef fx_school_immunity_dec_ref={"Protection:SchoolDec",NULL,-1};
static EffectRef fx_secondary_type_immunity_dec_ref={"Protection:SecondaryTypeDec",NULL,-1};

//bounce effects
static EffectRef fx_level_bounce_ref={"Bounce:SpellLevel",NULL,-1};
//static EffectRef fx_opcode_bounce_ref={"Bounce:Opcode",NULL,-1};
static EffectRef fx_spell_bounce_ref={"Bounce:Spell",NULL,-1};
static EffectRef fx_school_bounce_ref={"Bounce:School",NULL,-1};
static EffectRef fx_secondary_type_bounce_ref={"Bounce:SecondaryType",NULL,-1};

//decrementing bounce effects
static EffectRef fx_level_bounce_dec_ref={"Bounce:SpellLevelDec",NULL,-1};
static EffectRef fx_spell_bounce_dec_ref={"Bounce:SpellDec",NULL,-1};
static EffectRef fx_school_bounce_dec_ref={"Bounce:SchoolDec",NULL,-1};
static EffectRef fx_secondary_type_bounce_dec_ref={"Bounce:SecondaryTypeDec",NULL,-1};

//spelltrap (multiple decrementing immunity)
static EffectRef fx_spelltrap={"SpellTrap", NULL,-1};

//this is for whole spell immunity/bounce
inline static void DecreaseEffect(Effect *efx)
{
	efx->Parameter1--;
	if ((int) efx->Parameter1<1) {
		//don't remove effects directly!!!
		efx->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

static int check_type(Actor* actor, Effect* fx)
{
	//the protective effect (if any)
	Effect *efx;

	ieDword bounce = actor->GetStat(IE_BOUNCE);

	//immunity checks
/*opcode immunity is in the per opcode checks
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		return 0;
	}
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		return 0;
	}
*/
	//spell level immunity
	if (actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_ref, fx->Power, 0) ) {
		return 0;
	}

	//source immunity (spell name)
	//if source is unspecified, don't resist it
	if (fx->Source[0]) {
		if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity_ref, fx->Source) ) {
			return 0;
		}
		if (actor->fxqueue.HasEffectWithResource(fx_spell_immunity2_ref, fx->Source) ) {
			return 0;
		}
	}

	//primary type immunity (school)
	if (fx->PrimaryType) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_immunity_ref, fx->PrimaryType)) {
			return 0;
		}
	}

	//secondary type immunity (usage)
	if (fx->SecondaryType) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_ref, fx->SecondaryType) ) {
			return 0;
		}
	}

	//decrementing immunity checks
	//decrementing level immunity
	efx = actor->fxqueue.HasEffectWithParamPair(fx_level_immunity_dec_ref, fx->Power, 0);
	if (efx ) {
		DecreaseEffect(efx);
		return 0;
	}

	//decrementing spell immunity
	if (fx->Source[0]) {
		efx = actor->fxqueue.HasEffectWithResource(fx_spell_immunity_dec_ref, fx->Source);
		if (efx) {
			DecreaseEffect(efx);
			return 0;
		}
	}
	//decrementing primary type immunity (school)
	if (fx->PrimaryType) {
		efx = actor->fxqueue.HasEffectWithParam(fx_school_immunity_dec_ref, fx->PrimaryType);
		if (efx) {
			DecreaseEffect(efx);
			return 0;
		}
	}

	//decrementing secondary type immunity (usage)
	if (fx->SecondaryType) {
		efx = actor->fxqueue.HasEffectWithParam(fx_secondary_type_immunity_dec_ref, fx->SecondaryType);
		if (efx) {
			DecreaseEffect(efx);
			return 0;
		}
	}

	//spelltrap (absorb)
	//FIXME:
	//if the spelltrap effect already absorbed enough levels
	//but still didn't get removed, it will absorb levels it shouldn't
	//it will also absorb multiple spells in a single round
	efx=actor->fxqueue.HasEffectWithParamPair(fx_spelltrap, 0, fx->Power);
	if (efx) {
		//storing the absorbed spell level
		efx->Parameter3+=fx->Power;
		//instead of a single effect, they had to create an effect for each level
		//HOW DAMN LAME
		//if decrease needs the spell level, use fx->Power here
		actor->fxqueue.DecreaseParam1OfEffect(fx_spelltrap, 1);
		//efx->Parameter1--;
		return 0;
	}

	//bounce checks
	if ((bounce&BNC_LEVEL) && actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_ref, fx->Power, 0) ) {
		return 0;
	}

	if (fx->Source[0] && (bounce&BNC_RESOURCE) && actor->fxqueue.HasEffectWithResource(fx_spell_bounce_ref, fx->Source) ) {
		return -1;
	}

	if (fx->PrimaryType && (bounce&BNC_SCHOOL) ) {
		if (actor->fxqueue.HasEffectWithParam(fx_school_bounce_ref, fx->PrimaryType)) {
			return -1;
		}
	}

	if (fx->SecondaryType && (bounce&BNC_SECTYPE) ) {
		if (actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_ref, fx->SecondaryType)) {
			return -1;
		}
	}
	//decrementing bounce checks

	//level decrementing bounce check
	if ((bounce&BNC_LEVEL_DEC)) {
		efx=actor->fxqueue.HasEffectWithParamPair(fx_level_bounce_dec_ref, fx->Power, 0);
		if (efx) {
			DecreaseEffect(efx);
			return -1;
		}
	}

	if (fx->Source[0] && (bounce&BNC_RESOURCE_DEC)) {
		efx=actor->fxqueue.HasEffectWithResource(fx_spell_bounce_dec_ref, fx->Resource);
		if (efx) {
			DecreaseEffect(efx);
			return -1;
		}
	}

	if (fx->PrimaryType && (bounce&BNC_SCHOOL_DEC) ) {
		efx=actor->fxqueue.HasEffectWithParam(fx_school_bounce_dec_ref, fx->PrimaryType);
		if (efx) {
			DecreaseEffect(efx);
			return -1;
		}
	}

	if (fx->SecondaryType && (bounce&BNC_SECTYPE_DEC) ) {
		efx=actor->fxqueue.HasEffectWithParam(fx_secondary_type_bounce_dec_ref, fx->SecondaryType);
		if (efx) {
			DecreaseEffect(efx);
			return -1;
		}
	}

	return 1;
}

static bool check_resistance(Actor* actor, Effect* fx)
{
	//magic immunity
	ieDword val = actor->GetStat(IE_RESISTMAGIC);
	if (fx->random_value < val) {
		return true;
	}
	//opcode immunity
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity_ref, fx->Opcode) ) {
		return true;
	}
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_immunity2_ref, fx->Opcode) ) {
		return true;
	}

/* opcode bouncing isn't implemented?
	//opcode bouncing
	if (actor->fxqueue.HasEffectWithParam(fx_opcode_bounce_ref, fx->Opcode) ) {
		return false;
	}
*/

	//saving throws
	bool saved = false;
	ieDword j=1;
	for (int i=0;i<5;i++) {
		if (fx->SavingThrowType&j) {
			saved = actor->GetSavingThrow(i, fx->SavingThrowBonus);
			if (saved) {
				break;
			}
		}
	}
	if (saved) {
		if (fx->IsSaveForHalfDamage) {
			fx->Parameter1/=2;
		} else {
			return true;
		}
	}
	return false;
}

// this function is called two different ways
// when FirstApply is set, then the effect isn't stuck on the target
// this happens when a new effect comes in contact with the target.
// if the effect returns FX_DURATION_JUST_EXPIRED then it won't stick
// when first_apply is unset, the effect is already on the target
// this happens on load time too!
// returns true if the process should stop calling applyeffect anymore

int EffectQueue::ApplyEffect(Actor* target, Effect* fx, ieDword first_apply)
{
	if (!target) {
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
		return FX_ABORT;
	}
	//printf( "FX 0x%02x: %s(%d, %d)\n", fx->Opcode, effectnames[fx->Opcode].Name, fx->Parameter1, fx->Parameter2 );
	if (fx->Opcode >= MAX_EFFECTS) {
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
		return FX_NOT_APPLIED;
	}

	ieDword GameTime = core->GetGame()->GameTime;

	fx->FirstApply=first_apply;
	if (first_apply) {
		if ((fx->PosX==0xffffffff) && (fx->PosY==0xffffffff)) {
			fx->PosX = target->Pos.x;
			fx->PosY = target->Pos.y;
		}
		//the effect didn't pass the probability check
		if (!check_probability(fx) ) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			return FX_NOT_APPLIED;
		}

		//the effect didn't pass the target level check
		if (check_level(target, fx) ) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			return FX_NOT_APPLIED;
		}

		//the effect didn't pass the resistance check
		if(fx->Resistance == FX_CAN_RESIST_CAN_DISPEL ||
			fx->Resistance == FX_CAN_RESIST_NO_DISPEL) {
			if (check_resistance(target, fx) ) {
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				return FX_NOT_APPLIED;
			}
		}
		if (NeedPrepare((ieByte) fx->TimingMode) ) {
			//save delay for later
			fx->SecondaryDelay=fx->Duration;
			PrepareDuration(fx);
		}
	}
	//check if the effect has triggered or expired
	switch (IsPrepared((ieByte) fx->TimingMode) ) {
	case DELAYED:
		if (fx->Duration>GameTime) {
			return FX_NOT_APPLIED;
		}
		//effect triggered
		//delayed duration (3)
		if (NeedPrepare((ieByte) fx->TimingMode) ) {
			//prepare for delayed duration effects
			fx->Duration=fx->SecondaryDelay;
			PrepareDuration(fx);
		}
		fx->TimingMode=TriggeredEffect((ieByte) fx->TimingMode);
		break;
	case DURATION:
		if (fx->Duration<=GameTime) {
			fx->TimingMode=FX_DURATION_JUST_EXPIRED;
			//add a return here, if 0 duration effects shouldn't work
		}
		break;
	//permanent effect (so there is no warning)
	case PERMANENT:
		break;
	//this shouldn't happen
	default:
		abort();
	}

	EffectFunction fn = 0;
	if (fx->Opcode<MAX_EFFECTS) {
		fn = effect_refs[fx->Opcode].Function;
	}
	int res = FX_ABORT;
	if (fn) {    
		if ( target && first_apply ) {
			if (!target->fxqueue.HasEffectWithParamPair(fx_protection_from_display_string_ref, fx->Parameter1, 0) ) {
				core->DisplayStringName( effect_refs[fx->Opcode].EffText, 0xf0f0f0, 
					target, IE_STR_SOUND);
			}
		}
		
		res=fn( Owner?Owner:target, target, fx );

		//if there is no owner, we assume it is the target
		switch( res ) {
			case FX_APPLIED:
				//normal effect with duration
				break;
			case FX_NOT_APPLIED:
 				//instant effect, pending removal
				//for example, a damage effect
				fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				break;
			case FX_INSERT:
				//put this effect in the beginning of the queue
				//all known insert effects are 'permanent' too
				//that is the AC effect only
				//actually, permanent effects seem to be
				//inserted by the game engine too
			case FX_PERMANENT:
				//don't stick around if it was permanent
				//for example, a strength modifier effect
				if ((fx->TimingMode==FX_DURATION_INSTANT_PERMANENT) ) {
					fx->TimingMode=FX_DURATION_JUST_EXPIRED;
				}
				break;
			case FX_ABORT:
				break;
			default:
				abort();
		}
	} else {
		//effect not found, it is going to be discarded
		fx->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
	return res;
}

// looks for opcode with param2

#define MATCH_OPCODE() if((*f)->Opcode!=opcode) { continue; }

// useful for: remove equipped item
#define MATCH_SLOTCODE() if((*f)->InventorySlot!=slotcode) { continue; }

// useful for: remove projectile type
#define MATCH_PROJECTILE() if((*f)->Projectile!=projectile) { continue; }

#define MATCH_LIVE_FX() {ieDword tmp=(*f)->TimingMode; \
		if (tmp!=FX_DURATION_INSTANT_LIMITED && \
			tmp!=FX_DURATION_INSTANT_PERMANENT && \
			tmp!=FX_DURATION_INSTANT_WHILE_EQUIPPED && \
			tmp!=FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES) { \
			continue; \
		}}

#define MATCH_PARAM1() if((*f)->Parameter1!=param1) { continue; }
#define MATCH_PARAM2() if((*f)->Parameter2!=param2) { continue; }
#define MATCH_RESOURCE() if( strnicmp( (*f)->Resource, resource, 8) ) { continue; }
#define MATCH_SOURCE() if( strnicmp( (*f)->Source, Removed, 8) ) { continue; }
#define MATCH_TIMING() if ((*f)->TimingMode!=timing) { continue; }

//call this from an applied effect, after it returns, these effects
//will be killed along with it
void EffectQueue::RemoveAllEffects(ieDword opcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//removes all equipping effects that match slotcode
void EffectQueue::RemoveEquippingEffects(ieDwordSigned slotcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ((*f)->TimingMode!=FX_DURATION_INSTANT_WHILE_EQUIPPED) continue;
		MATCH_SLOTCODE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//removes all effects that match projectile
void EffectQueue::RemoveAllEffectsWithProjectile(ieDword projectile) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_PROJECTILE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell
void EffectQueue::RemoveAllEffects(const ieResRef Removed) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_LIVE_FX();
		MATCH_SOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//remove effects belonging to a given spell, but only if they match timing method x
void EffectQueue::RemoveAllEffects(const ieResRef Removed, ieDword timing) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_TIMING();
		MATCH_SOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//this will modify effect reference
void EffectQueue::RemoveAllEffects(EffectRef &effect_reference) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return;
	}
	RemoveAllEffects(effect_reference.EffText);
}

void EffectQueue::RemoveAllEffectsWithResource(ieDword opcode, const ieResRef resource) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

void EffectQueue::RemoveAllEffectsWithResource(EffectRef &effect_reference, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithResource(effect_reference.EffText, resource);
}

void EffectQueue::RemoveAllDetrimentalEffects(ieDword opcode, ieDword current) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		switch((*f)->Parameter2) {
		case 0:case 3:
			if (((signed) (*f)->Parameter1)>=0) continue;
			break;
		case 1:case 4:
			if (((signed) (*f)->Parameter1)>=(signed) current) continue;
			break;
		case 2:case 5:
			if (((signed) (*f)->Parameter1)>=100) continue;
			break;
		default:
			break;
		}
		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

void EffectQueue::RemoveAllEffectsWithParam(ieDword opcode, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
	}
}

//this function is called by FakeEffectExpiryCheck
//probably also called by rest
void EffectQueue::RemoveExpiredEffects(ieDword futuretime) const
{
	ieDword GameTime = core->GetGame()->GameTime;
	if (GameTime+futuretime<GameTime) {
		GameTime=0xffffffff;
	} else {
		GameTime+=futuretime;
	}

	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		//FIXME: how this method handles delayed effects???
		//it should remove them as well, i think
		if ( IsPrepared( (ieByte) ((*f)->TimingMode) )!=PERMANENT ) {
			if ( (*f)->Duration<=GameTime) {
				(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
			}
		}
	}
}

//this effect will expire all effects that are not truly permanent
//which i call permanent after death (iesdp calls it permanent after bonuses)
void EffectQueue::RemoveAllNonPermanentEffects() const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if( (*f)->TimingMode != FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES) {
			(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
		}
	}
}

//this will modify effect reference

void EffectQueue::RemoveAllDetrimentalEffects(EffectRef &effect_reference, ieDword current) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllDetrimentalEffects(effect_reference.EffText, current);
}

void EffectQueue::RemoveAllEffectsWithParam(EffectRef &effect_reference, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	RemoveAllEffectsWithParam(effect_reference.EffText, param2);
}

void EffectQueue::RemoveLevelEffects(ieDword level, ieDword Flags, ieDword match) const
{
	ieResRef Removed;

	Removed[0]=0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ( (*f)->Power>level) {
			continue;
		}

		if (Removed[0]) {
			MATCH_SOURCE();
		}
		if (Flags&RL_MATCHSCHOOL) {
			if ((*f)->PrimaryType!=match) {
				continue;
			}
		}
		if (Flags&RL_MATCHSECTYPE) {
			if ((*f)->SecondaryType!=match) {
				continue;
			}
		}
		//if dispellable was not set, or the effect is dispellable
		//then remove it
		if (Flags&RL_DISPELLABLE) {
			if (!(*f)->Resistance&FX_CAN_DISPEL) {
				continue;
			}
		}
		(*f)->TimingMode=FX_DURATION_JUST_EXPIRED;
		if (Flags&RL_REMOVEFIRST) {
			memcpy(Removed,(*f)->Source, sizeof(Removed));
		}
	}
}

Effect *EffectQueue::HasOpcode(ieDword opcode) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffect(EffectRef &effect_reference) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcode(effect_reference.EffText);
}

Effect *EffectQueue::HasOpcodeWithParam(ieDword opcode, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithParam(EffectRef &effect_reference, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcodeWithParam(effect_reference.EffText, param2);
}

//looks for opcode with pairs of parameters (useful for protection against creature, extra damage or extra thac0 against creature)
//generally an IDS targeting

Effect *EffectQueue::HasOpcodeWithParamPair(ieDword opcode, ieDword param1, ieDword param2) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_PARAM2();
		//0 is always accepted as first parameter
		if (param1) {
			MATCH_PARAM1();
		}

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithParamPair(EffectRef &effect_reference, ieDword param1, ieDword param2) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return NULL;
	}
	return HasOpcodeWithParamPair(effect_reference.EffText, param1, param2);
}

//this could be used for stoneskins and mirror images as well
void EffectQueue::DecreaseParam1OfEffect(ieDword opcode, ieDword amount) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		ieDword value = (*f)->Parameter1;
		if (value>amount) value-=amount;
		else value = 0;
		(*f)->Parameter1=value;
	}
}

void EffectQueue::DecreaseParam1OfEffect(EffectRef &effect_reference, ieDword amount) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return;
	}
	DecreaseParam1OfEffect(effect_reference.EffText, amount);
}


//this function does IDS targeting for effects (extra damage/thac0 against creature)
static const int ids_stats[7]={IE_EA, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, IE_SEX, IE_ALIGNMENT};

int EffectQueue::BonusAgainstCreature(ieDword opcode, Actor *actor) const
{
	int sum = 0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		ieDword ids = (*f)->Parameter2;
		if (ids<2 || ids>9) {
			ids=2;
		}
		ieDword param1 = actor->GetStat(ids_stats[ids-2]);
		if ( (*f)->Parameter1) {
			MATCH_PARAM1();
		}
		int val = (int) (*f)->Parameter3;
		if (!val) val = 2;
		sum += val;
	}
	return sum;
}

int EffectQueue::BonusAgainstCreature(EffectRef &effect_reference, Actor *actor) const
{
	ResolveEffectRef(effect_reference);
	if (effect_reference.EffText<0) {
		return 0;
	}
	return BonusAgainstCreature(effect_reference.EffText, actor);
}

bool EffectQueue::WeaponImmunity(ieDword opcode, int enchantment, ieDword weapontype) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		//
		int magic = (int) (*f)->Parameter1;
		ieDword mask = (*f)->Parameter3;
		ieDword value = (*f)->Parameter4;
		if (magic==0) {
			if (enchantment) continue;
		} else if (magic>0) {
			if (enchantment>magic) continue;
		}

		if ((weapontype&mask) != value) {
			continue;
		}
		return true;
	}
	return false;
}

static EffectRef fx_weapon_immunity_ref={"Protection:Weapons",NULL,-1};

bool EffectQueue::WeaponImmunity(int enchantment, ieDword weapontype) const
{
	ResolveEffectRef(fx_weapon_immunity_ref);
	if (fx_weapon_immunity_ref.EffText<0) {
		return 0;
	}
	return WeaponImmunity(fx_weapon_immunity_ref.EffText, enchantment, weapontype);
}

//useful for immunity vs spell, can't use item, etc.
Effect *EffectQueue::HasOpcodeWithResource(ieDword opcode, const ieResRef resource) const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		MATCH_LIVE_FX();
		MATCH_RESOURCE();

		return (*f);
	}
	return NULL;
}

Effect *EffectQueue::HasEffectWithResource(EffectRef &effect_reference, const ieResRef resource) const
{
	ResolveEffectRef(effect_reference);
	return HasOpcodeWithResource(effect_reference.EffText, resource);
}

bool EffectQueue::HasAnyDispellableEffect() const
{
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		if ((*f)->Resistance&FX_CAN_DISPEL) {
			return true;
		}
	}
	return false;
}

void EffectQueue::dump()
{
	printf( "EFFECT QUEUE:\n" );
	int i = 0;
	std::list< Effect* >::const_iterator f;
	for ( f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx = *f;
		if (fx) {
			char *Name = NULL;
			if (fx->Opcode < MAX_EFFECTS)
				Name = (char*) effect_refs[fx->Opcode].Name;

			printf( " %2d: 0x%02x: %s (%d, %d)\n", i++, fx->Opcode, Name, fx->Parameter1, fx->Parameter2 );
		}
	}
}
/*
Effect *EffectQueue::GetEffect(ieDword idx) const
{
	if (effects.size()<=idx) {
		return NULL;
	}
	return effects[idx];
}
*/

//returns true if the effect supports simplified duration
bool EffectQueue::HasDuration(Effect *fx)
{
	switch(fx->TimingMode) {
	case FX_DURATION_INSTANT_LIMITED: //simple duration
	case FX_DURATION_DELAY_LIMITED:   //delayed duration
	case FX_DURATION_DELAY_PERMANENT: //simple delayed
		return true;
	}
	return false;
}

static EffectRef fx_variable_ref={"Variable:StoreLocalVariable",NULL,-1};

//returns true if the effect must be saved
//variables are saved differently
bool EffectQueue::Persistent(Effect* fx)
{
	//we save this as variable
	if (fx->Opcode==(ieDword) ResolveEffect(fx_variable_ref)) {
		return false;
	}

	switch (fx->TimingMode) {
		//normal equipping fx of items
		case FX_DURATION_INSTANT_WHILE_EQUIPPED:
		//delayed effect not saved
		case FX_DURATION_DELAY_UNSAVED:
		//permanent effect not saved
		case FX_DURATION_PERMANENT_UNSAVED:
		//just expired effect
		case FX_DURATION_JUST_EXPIRED:
			return false;
	}
	return true;
}

//alter the color effect in case the item is equipped in the shield slot
void EffectQueue::HackColorEffects(Actor *Owner, Effect *fx)
{
	if (fx->InventorySlot!=Owner->inventory.GetShieldSlot()) return;

	unsigned int gradienttype = fx->Parameter2 & 0xF0;
	if (gradienttype == 0x10) {
		gradienttype = 0x20; // off-hand
		fx->Parameter2 &= ~0xF0;
		fx->Parameter2 |= gradienttype;
	}
}

//iterate through saved effects
const Effect *EffectQueue::GetNextSavedEffect(std::list< Effect* >::const_iterator f) const
{
	while(f!=effects.end()) {
		Effect *effect = *f;
		f++;
		if (Persistent(effect)) {
			return effect;
		}
	}
	return NULL;
}

Effect *EffectQueue::GetNextEffect(std::list< Effect* >::const_iterator f) const
{
	if (f!=effects.end()) return *f++;
	return NULL;
}

ieDword EffectQueue::CountEffects(ieDword opcode, ieDword param1, ieDword param2, const char *resource) const
{
	ieDword cnt = 0;

	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		if (param1!=0xffffffff)
			MATCH_PARAM1();
		if (param2!=0xffffffff)
			MATCH_PARAM2();
		if (resource) {
			MATCH_RESOURCE();
		}
		cnt++;
	}
	return cnt;
}

void EffectQueue::ModifyEffectPoint(ieDword opcode, ieDword x, ieDword y) const
{
	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		MATCH_OPCODE();
		(*f)->PosX=x;
		(*f)->PosY=y;
		(*f)->Parameter3=0;
		return;
	}
}

//count effects that get saved
ieDword EffectQueue::GetSavedEffectsCount() const
{
	ieDword cnt = 0;

	std::list< Effect* >::const_iterator f;

	for ( f = effects.begin(); f != effects.end(); f++ ) {
		Effect* fx = *f;
		if (Persistent(fx))
			cnt++;
	}
	return cnt;
}

void EffectQueue::TransformToDelay(ieDword &TimingMode)
{
	if (TimingMode<MAX_TIMING_MODE) {;
		TimingMode = fx_to_delayed[TimingMode];
	} else {
		TimingMode = FX_DURATION_JUST_EXPIRED;
	}
}

int EffectQueue::ResolveEffect(EffectRef &effect_reference)
{
	ResolveEffectRef(effect_reference);
	return effect_reference.EffText;
}

// this check goes for the whole effect block, not individual effects
// But it takes the first effect of the block for the common fields

//returns 1 if effect block applicable
//returns 0 if effect block disabled
//returns -1 if effect block bounced
int EffectQueue::CheckImmunity(Actor *target) const
{
	if (effects.size() ) {
		Effect* fx = *effects.begin();

		//projectile immunity
		if (target->ImmuneToProjectile(fx->Projectile)) return 0;
		//check level resistances
		//check specific spell immunity
		//check school/sectype immunity
		return check_type(target, fx);
	}
	return 0;
}

void EffectQueue::AffectAllInRange(Map *map, Point &pos, int idstype, int idsvalue,
		unsigned int range, Actor *except)
{
	int cnt = map->GetActorCount(true);
	while(cnt--) {
		Actor *actor = map->GetActor(cnt,true);
		if (except==actor) {
			continue;
		}
		//distance
		if (Distance(pos, actor)>range) {
			continue;
		}
		//ids targeting
		if (!match_ids(actor, idstype, idsvalue)) {
			continue;
		}
		//line of sight
		if (!map->IsVisible(actor->Pos, pos)) {
			continue;
		}
		AddAllEffects(actor, actor->Pos);
	}
}

