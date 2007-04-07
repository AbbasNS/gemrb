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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"
#include "ResourceMgr.h"
#include "SoundMgr.h"
#include "Video.h"
#include "Game.h"

#define ILLEGAL 0         //
#define ONE 1             //hold
#define TWO 2             //onset + hold
#define THREE 3           //onset + hold + release
#define DOUBLE 4          //has twin (pst)
#define FIVE 8            //five faces (orientation)
#define NINE 16           //nine faces (orientation)

#define MAX_CYCLE_TYPE 16
//  0       1   2    3        4         5       6           7
static ieByte ctypes[MAX_CYCLE_TYPE]=
{ ILLEGAL, ONE, TWO, THREE, TWO|DOUBLE, ONE|FIVE, THREE|DOUBLE, ILLEGAL,
// 8       9   10           11       12       13      14       15
	ILLEGAL,ONE|NINE, TWO|FIVE, ILLEGAL, ILLEGAL, ILLEGAL, ILLEGAL,THREE|FIVE,
};

static ieByte SixteenToNine[3*MAX_ORIENT]={
	0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1,
	9,10,11,12,13,14,15,16,17,16,15,14,13,12,11,10,
 18,19,20,21,22,23,24,25,26,25,24,23,22,21,20,19
};
static ieByte SixteenToFive[3*MAX_ORIENT]={
	0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 3, 3, 2, 2, 1, 1,
	5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 8, 8, 7, 7, 6, 6,
 10,10,11,11,12,12,13,13,14,14,13,13,12,12,11,11
};

ScriptedAnimation::ScriptedAnimation()
{
	Init();
}

void ScriptedAnimation::Init()
{
	cover = NULL;
	memset(anims,0,sizeof(anims));
	palette = NULL;
	sounds[0][0] = 0;
	sounds[1][0] = 0;
	sounds[2][0] = 0;

	Transparency = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = 15;
	FaceTarget = 0;
	Orientation = 0;
	dither = 0;
	Duration = 0xffffffff;
	justCreated = true;
	PaletteName[0]=0;
	twin = NULL;
	Phase = P_NOTINITED;
}

void ScriptedAnimation::Override(ScriptedAnimation *templ)
{
	Transparency = templ->Transparency;
	SequenceFlags = templ->SequenceFlags;
	XPos = templ->XPos;
	YPos = templ->YPos;
	ZPos = templ->ZPos;
	FrameRate = templ->FrameRate;
	FaceTarget = templ->FaceTarget;
	for (unsigned int i=0;i<3;i++) {
		memcpy(sounds[i],templ->sounds[i],sizeof(ieResRef));
	}
	if (templ->Duration!=0xffffffff) {
		SetDefaultDuration(templ->Duration);
	}
	if (templ->PaletteName[0]) {
		SetFullPalette(templ->PaletteName);
	}
}

//prepare the animation before doing anything
void ScriptedAnimation::PrepareAnimation(Animation *anim, ieDword Transparency)
{
	if (Transparency&IE_VVC_MIRRORX) {
		anim->MirrorAnimation();
	}
	if (Transparency&IE_VVC_MIRRORY) {
		anim->MirrorAnimationVert();
	}
	
	//make this the last if possible, because of the return
	if (Transparency&IE_VVC_BLENDED) {
		GetPaletteCopy();
		if (!palette)
			return;
		if (!palette->alpha) {
			palette->CreateShadedAlphaChannel();
		}
	}
}

/* Creating animation from BAM */
void ScriptedAnimation::LoadAnimationFactory(AnimationFactory *af, int gettwin)
{
	//getcycle returns NULL if there is no such cycle
	//special case, PST double animations

	memcpy(ResName, af->ResRef, sizeof(ResName) );
	unsigned int cCount=af->GetCycleCount();
	if (cCount>=MAX_CYCLE_TYPE) {
		cCount=1;
	}

	int type = ctypes[cCount];
	switch(gettwin) {
	case 2:
		if (type==TWO) {
			type=ONE|DOUBLE;
		}
		gettwin=0;
		break;
	case 1:
		type=ONE|DOUBLE;
		break;
	}

	if (type==ILLEGAL) {
		cCount=1;
		type=ONE;
	}
	else if (type&DOUBLE) {
		cCount/=2;
	}

	//these fields mark that the anim cycles should 'follow' the target's orientation
	if (type&FIVE) {
		FaceTarget = 5;
		cCount = MAX_ORIENT*(type&3);
	} else if (type&NINE) {
		FaceTarget = 9;
		cCount = MAX_ORIENT*(type&3);
	} else {
		FaceTarget = 0;
	}

	palette = NULL;

	for(unsigned int i=0;i<cCount;i++) {
		bool mirror = false;
		int c = i;
		int p = i;
		if (type&DOUBLE) {
			c<<=1;
			if (gettwin) c++;
		} else if (type&FIVE) {
			c=SixteenToFive[c];
			if ((i&15)>=5) mirror = true;
		} else if (type&NINE) {
			c=SixteenToNine[c];
			if ((i&15)>=9) mirror = true;
		} else p*=MAX_ORIENT;
	 
		anims[p] = af->GetCycle( (ieByte) c );
		if (anims[p]) {
			anims[p]->pos=0;
			if (mirror) {
				anims[p]->MirrorAnimation();
			}
			anims[p]->gameAnimation=true;
		}
	}

	for (unsigned int o = 0; o<MAX_ORIENT; o++)
	{
		unsigned int p_hold = P_HOLD*MAX_ORIENT+o;
		unsigned int p_onset = P_ONSET*MAX_ORIENT+o;
		unsigned int p_release = P_RELEASE*MAX_ORIENT+o;
		//if there is no hold anim, move the onset anim there
		if (!anims[p_hold]) {
			anims[p_hold]=anims[p_onset];
			anims[p_onset]=NULL;
		}
		//onset and release phases are to be played only once
		if (anims[p_onset])
			anims[p_onset]->Flags |= S_ANI_PLAYONCE;
		if (anims[p_release])
			anims[p_release]->Flags |= S_ANI_PLAYONCE;
	}
	//we are getting a twin, no need of going further,
	//if there is any more common initialisation, it should
	//go above this point
	if (gettwin) {
		return;
	}
	if (type&DOUBLE) {
		twin = new ScriptedAnimation();
		twin->LoadAnimationFactory(af, 1);
	}
	SetPhase(P_ONSET);
}

/* Creating animation from VVC */
ScriptedAnimation::ScriptedAnimation(DataStream* stream, bool autoFree)
{
	Init();
	if (!stream) {
		return;
	}

	char Signature[8];

	stream->Read( Signature, 8);
	if (strncmp( Signature, "VVC V1.0", 8 ) != 0) {
		printf( "Not a valid VVC File\n" );
		if (autoFree)
			delete( stream );
		return;
	}
	ieResRef Anim1ResRef;  
	ieDword seq1, seq2, seq3;
	stream->ReadResRef( Anim1ResRef );
	//there is no proof it is a second resref
	//stream->ReadResRef( Anim2ResRef );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS );
	ieDword tmp;
	stream->ReadDword( &tmp );
	XPos = (signed) tmp;
	stream->ReadDword( &tmp );  //this affects visibility
	ZPos = (signed) tmp;
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &FrameRate );
	stream->ReadDword( &FaceTarget );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->ReadDword( &tmp );  //this doesn't affect visibility
	YPos = (signed) tmp;
	stream->Seek( 12, GEM_CURRENT_POS );
	stream->ReadDword( &Duration );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &seq1 );
	if (seq1>0) seq1--; //hack but apparently it works this way
	stream->ReadDword( &seq2 );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( sounds[P_ONSET] );
	stream->ReadResRef( sounds[P_HOLD] );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadDword( &seq3 );
	stream->ReadResRef( sounds[P_RELEASE] );

	if (SequenceFlags&IE_VVC_BAM) {
		AnimationFactory* af = ( AnimationFactory* )
			core->GetResourceMgr()->GetFactoryResource( Anim1ResRef, IE_BAM_CLASS_ID );
		//no idea about vvc phases, i think they got no endphase?
		//they certainly got onset and hold phases
		anims[P_ONSET] = af->GetCycle( ( unsigned char ) seq1 );
		if (anims[P_ONSET]) {
			PrepareAnimation(anims[P_ONSET], Transparency);
			//creature anims may start at random position, vvcs always start on 0
			anims[P_ONSET]->pos=0;
			//vvcs are always paused
			anims[P_ONSET]->gameAnimation=true;
			anims[P_ONSET]->Flags |= S_ANI_PLAYONCE;
		}

		anims[P_HOLD] = af->GetCycle( ( unsigned char ) seq2 );  
		if (anims[P_HOLD]) {
			PrepareAnimation(anims[P_HOLD], Transparency);

			anims[P_HOLD]->pos=0;
			anims[P_HOLD]->gameAnimation=true;
			if (!(SequenceFlags&IE_VVC_LOOP) ) {
				anims[P_HOLD]->Flags |= S_ANI_PLAYONCE;
			}
		}

		anims[P_RELEASE] = af->GetCycle( ( unsigned char ) seq3 );  
		if (anims[P_RELEASE]) {
			PrepareAnimation(anims[P_RELEASE], Transparency);

			anims[P_RELEASE]->pos=0;
			anims[P_RELEASE]->gameAnimation=true;
			anims[P_RELEASE]->Flags |= S_ANI_PLAYONCE;
		}
	}

	//copying resource name to the object, so it could be referenced by it
	//used by immunity/remove specific animation
	//this is better done in Interface::GetScriptedAnimation
	//where the original resref is more readily available
	/*
	memcpy(ResName, stream->filename, 8);
	for(int i=0;i<8;i++) {
		if (ResName[i]=='.') ResName[i]=0;
	}
	*/
	SetPhase(P_ONSET);
	PaletteName[0]=0;

	if (autoFree) {
		delete( stream );
	}
}

ScriptedAnimation::~ScriptedAnimation(void)
{
	for(unsigned int i=0;i<3*MAX_ORIENT;i++) {
		if (anims[i]) {
			delete( anims[i] );
		}		
	}
	core->FreePalette(palette, PaletteName);

	if (cover) {
		SetSpriteCover(NULL);
	}
	if (twin) {
		delete twin;
	}
}

void ScriptedAnimation::SetPhase(int arg)
{
	if (arg>=P_ONSET && arg<=P_RELEASE) {
		Phase = arg;
	}
	SetSpriteCover(NULL);
	if (twin) {
		twin->SetPhase(Phase);
	}
}

void ScriptedAnimation::SetSound(int arg, const ieResRef sound)
{
	if (arg>=P_ONSET && arg<=P_RELEASE) {
		memcpy(sounds[arg],sound,sizeof(sound));
	}
	//no need to call the twin
}

void ScriptedAnimation::PlayOnce()
{
	SequenceFlags&=~IE_VVC_LOOP;
	for (unsigned int i=0;i<3*MAX_ORIENT;i++) {
		if (anims[i]) {
			anims[i]->Flags |= S_ANI_PLAYONCE;
		}
	}
	if (twin) {
		twin->PlayOnce();
	}
}

void ScriptedAnimation::SetFullPalette(const ieResRef PaletteResRef)
{
	core->FreePalette(palette, PaletteName);
	palette=core->GetPalette(PaletteResRef);
	memcpy(PaletteName, PaletteResRef, sizeof(PaletteName) );
	if (twin) {
		twin->SetFullPalette(PaletteResRef);
	}
}

void ScriptedAnimation::SetFullPalette(int idx)
{
	ieResRef PaletteResRef;

	//make sure this field is zero terminated, or strlwr will run rampant!!!
	snprintf(PaletteResRef,sizeof(PaletteResRef),"%.7s%d",ResName, idx);
	strnlwrcpy(PaletteResRef,PaletteResRef,8);
	SetFullPalette(PaletteResRef);
	//no need to call twin
}

#define PALSIZE 12
static Color NewPal[PALSIZE];

void ScriptedAnimation::SetPalette(int gradient, int start)
{
	//get a palette
	GetPaletteCopy();
	if (!palette)
		return;
	//default start
	if (start==-1) {
		start=4;
	}
	core->GetPalette( gradient&255, PALSIZE, NewPal );

	memcpy( &palette->col[start], NewPal, PALSIZE*sizeof( Color ) );
	if (twin) {
		twin->SetPalette(gradient, start);
	}
}

ieDword ScriptedAnimation::GetSequenceDuration(ieDword multiplier)
{
	//P_HOLD * MAX_ORIENT == MAX_ORIENT
	return anims[P_HOLD*MAX_ORIENT]->GetFrameCount()*multiplier/FrameRate;
}

void ScriptedAnimation::SetDefaultDuration(ieDword duration)
{
	if (Duration==0xffffffff) {
		Duration = duration;
	}
	if (twin) {
		twin->Duration=Duration;
	}
}

void ScriptedAnimation::SetOrientation(int orientation)
{
	if (orientation==-1) {
		return;
	}
	Orientation=orientation;
	if (twin) {
		twin->Orientation=Orientation;
	}
}

bool ScriptedAnimation::HandlePhase(Sprite2D *&frame)
{
	if (justCreated) {
		if (Phase == P_NOTINITED) {
			printMessage("ScriptedAnimation", "Not fully initialised VVC!", LIGHT_RED);
			return true;
		}
		justCreated = false;
		if (Duration!=0xffffffff) {
			Duration += core->GetGame()->Ticks;
		}

		if (!anims[P_ONSET*MAX_ORIENT+Orientation]) {
			Phase = P_HOLD;
		}
retry:
		if (sounds[Phase][0] != 0) {
			core->GetSoundMgr()->Play( sounds[Phase] );
		}
	}

	if (!anims[Phase*MAX_ORIENT+Orientation]) {
		if (Phase>=P_RELEASE) {
			return true;
		}
		Phase++;
		goto retry;
	}
	frame = anims[Phase*MAX_ORIENT+Orientation]->NextFrame();

	//explicit duration
	if (Phase==P_HOLD) {
		if (core->GetGame()->Ticks>Duration) {
			Phase++;
			goto retry;
		}
	}
	//automatically slip from onset to hold to release
	if (!frame || anims[Phase*MAX_ORIENT+Orientation]->endReached) {
		if (Phase>=P_RELEASE) {
			return true;
		}
		Phase++;
		goto retry;
	}
	return false;
}

//it is not sure if we need tint at all
bool ScriptedAnimation::Draw(Region &screen, Point &Pos, Color &p_tint, Map *area, int p_dither, int orientation)
{
	if (FaceTarget) {
		SetOrientation(orientation);
	}

	// not sure
	if (twin) {
		twin->Draw(screen, Pos, p_tint, area, p_dither, -1);
	}

	Video *video = core->GetVideoDriver();

	Sprite2D* frame;

	if (HandlePhase(frame)) {
		return true;
	}

	ieDword flag = BLIT_TRANSSHADOW;
	//transferring flags to SDLdriver, this will have to be consolidated later

	if (Transparency & IE_VVC_TRANSPARENT) {
		flag |= BLIT_HALFTRANS;
	}

	Color tint = {0,0,0,0};

	//darken, greyscale, red tint are probably not needed if the global tint works
	//these are used in the original engine to implement weather/daylight effects
	//on the other hand

	if (Transparency & IE_VVC_GREYSCALE) {
		flag |= BLIT_GREY;
	}

	if (Transparency & IE_VVC_RED_TINT) {
		flag |= BLIT_RED;
	}

	if ((Transparency & IE_VVC_TINT)==IE_VVC_TINT) {
		flag |= BLIT_TINTED;
		tint = p_tint;
	}

	int cx = Pos.x + XPos;
	int cy = Pos.y - ZPos + YPos;

	if( SequenceFlags&IE_VVC_NOCOVER) {
		if (cover) SetSpriteCover(NULL);
	} else {
		if (!cover || (dither!=p_dither) || (!cover->Covers(cx, cy, frame->XPos, frame->YPos, frame->Width, frame->Height)) ) {
			dither = p_dither;
			Animation *anim = anims[Phase*MAX_ORIENT+Orientation];
			SetSpriteCover(area->BuildSpriteCover(cx, cy, -anim->animArea.x, 
			-anim->animArea.y, anim->animArea.w, anim->animArea.h, p_dither) );
		}
		assert(cover->Covers(cx, cy, frame->XPos, frame->YPos, frame->Width, frame->Height));
	}

	video->BlitGameSprite( frame, cx + screen.x, cy + screen.y, flag, tint, cover, palette, &screen);
	return false;
}

void ScriptedAnimation::SetBlend()
{
	Transparency |= IE_VVC_BLENDED;

	for(unsigned int i=0;i<3*MAX_ORIENT;i++) {
		if (anims[i]) {
			//don't call mirror again
			PrepareAnimation(anims[i], IE_VVC_BLENDED);
		}
	}
	if (twin) {
		twin->SetBlend();
	}
}

void ScriptedAnimation::GetPaletteCopy()
{
	if (palette)
		return;
	for (unsigned int i=0;i<3*MAX_ORIENT;i++) {
		if (anims[i]) {
			Sprite2D* spr = anims[i]->GetFrame(0);
			if (spr) {
				palette = core->GetVideoDriver()->GetPalette(spr)->Copy();
			}
		}
	}
}

void ScriptedAnimation::AlterPalette(const RGBModifier& mod)
{
	GetPaletteCopy();
	if (!palette)
		return;
	palette->SetupCompleteRGBModification(palette,mod);
	if (twin) {
		twin->AlterPalette(mod);
	}
}

ScriptedAnimation *ScriptedAnimation::DetachTwin()
{
	if (!twin) {
		return NULL;
	}
	ScriptedAnimation * ret = twin;
	ret->YPos+=ret->ZPos+1;
	ret->ZPos=-1;
	twin=NULL;
	return ret;
}
