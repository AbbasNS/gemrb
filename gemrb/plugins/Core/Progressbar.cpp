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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Progressbar.cpp,v 1.14 2005/03/18 20:01:20 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Progressbar.h"
#include "Interface.h"

Progressbar::Progressbar( unsigned short KnobStepsCount, bool Clear)
{
	BackGround = NULL;
	BackGround2 = NULL;
	this->Clear = Clear;
	this->KnobStepsCount = KnobStepsCount;
	PBarAnim = NULL;
	PBarCap = NULL;
	KnobXPos = KnobYPos = 0;
	CapXPos = CapYPos = 0;
	ResetEventHandler( EndReached );
}

Progressbar::~Progressbar()
{
	if (!Clear) {
		return;
	}
	if (BackGround) {
		core->GetVideoDriver()->FreeSprite( BackGround );
	}
	if (BackGround2) {
		core->GetVideoDriver()->FreeSprite( BackGround2 );
	}
	if (PBarAnim) {
		delete( PBarAnim );
	}
	if (PBarCap) {
		core->GetVideoDriver()->FreeSprite( PBarCap );
	}
}

/** Draws the Control on the Output Display */
void Progressbar::Draw(unsigned short x, unsigned short y)
{
	//it is unlikely that a floating window is above us, but...
	if (!Changed && !((Window*)Owner)->Floating) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	Sprite2D *bcksprite;

	if((Value >= 100) && KnobStepsCount && BackGround2) {
		bcksprite=BackGround2; //animated progbar end stage
	}
	else {
		bcksprite=BackGround;
	}
	if (bcksprite) {
		Region r( x + XPos, y + YPos, Width, Height );
		core->GetVideoDriver()->BlitSprite( bcksprite,
			x + XPos, y + YPos, true, &r );
		if( bcksprite==BackGround2) {
			return; //done for animated progbar
		}
	}

	unsigned int Count;

	if(!KnobStepsCount) {
		//linear progressbar (pst, iwd)
		int w = BackGround2->Width;
		int h = BackGround2->Height;
		//this is the PST/IWD specific part
		Count = Value*w/100;
		Region r( x + XPos + KnobXPos, y + YPos + KnobYPos, Count, h );
		core->GetVideoDriver()->BlitSprite( BackGround2, 
			r.x, r.y, true, &r );
		core->GetVideoDriver()->BlitSprite( PBarCap,
			x+XPos+CapXPos+Count, y+YPos+CapYPos );
		return;
	}

	//animated progressbar (bg2)
	Count=Value*KnobStepsCount/100;
	for(unsigned int i=0; i<Count ;i++ ) {
		Sprite2D *Knob = PBarAnim->GetFrame(i);
		core->GetVideoDriver()->BlitSprite( Knob, x , y , false );
	}
}

/** Returns the actual Progressbar Position */
unsigned int Progressbar::GetPosition()
{
	return Value;
}

/** Sets the actual Progressbar Position trimming to the Max and Min Values */
void Progressbar::SetPosition(unsigned int pos)
{
	if(pos>100) pos=100;
	if (Value == pos)
		return;
	Value = pos;
	Changed = true;
}

void Progressbar::RedrawProgressbar(char* VariableName, int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	SetPosition((unsigned int) Sum);
	if((Value==100) && Changed)
		RunEventHandler( EndReached );
}

/** Sets the selected image */
void Progressbar::SetImage(Sprite2D* img, Sprite2D* img2)
{
	if (BackGround && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround );
	BackGround = img;
	if (BackGround2 && Clear)
		core->GetVideoDriver()->FreeSprite( BackGround2 );
	BackGround2 = img2;
	Changed = true;
}

void Progressbar::SetBarCap(Sprite2D* img3)
{
	PBarCap = img3;
}

void Progressbar::SetAnimation(Animation *arg)
{
	PBarAnim = arg;
}

void Progressbar::SetSliderPos(int x, int y, int x2, int y2)
{
	KnobXPos=x;
	KnobYPos=y;
	CapXPos=x2;
	CapYPos=y2;
}

/* dummy virtual function */
int Progressbar::SetText(const char* /*string*/, int /*pos*/)
{
	return 0;
}

bool Progressbar::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_PROGRESS_END_REACHED:
		SetEventHandler( EndReached, handler );
		break;
	default:
		return Control::SetEvent( eventType, handler );
	}

	return true;
}
