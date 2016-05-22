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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "ScrollBar.h"

#include "win32def.h"

#include "Interface.h"
#include "Variables.h"
#include "Sprite2D.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"

namespace GemRB {

ScrollBar::ScrollBar(const Region& frame, Sprite2D* images[IE_SCROLLBAR_IMAGE_COUNT])
	: Control(frame)
{
	ControlType = IE_GUI_SCROLLBAR;
	Pos = 0;
	Value = 0;
	State = 0;
	SliderYPos = 0;
	ScrollDelta = 1;
	ResetEventHandler( ScrollBarOnChange );
	textarea = NULL;

	for(int i=0; i < IE_SCROLLBAR_IMAGE_COUNT; i++) {
		Frames[i] = images[i];
		assert(Frames[i]);
	}
	SliderRange = frame.h
		- GetFrameHeight(IE_GUI_SCROLLBAR_SLIDER)
		- GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED)
		- GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED);
	if (SliderRange <= 0) {
		// must have a positive value, or we wont be able to scroll anywhere but the top
		SliderRange = 1;
	}
}

ScrollBar::~ScrollBar(void)
{
	for(int i=0; i < IE_SCROLLBAR_IMAGE_COUNT; i++) {
		Sprite2D::FreeSprite(Frames[i]);
	}
}

int ScrollBar::GetFrameHeight(int frame) const
{
	return Frames[frame]->Height;
}

double ScrollBar::GetStep()
{
	double stepPx = 0.0;
	if (Value){
		stepPx = (double)SliderRange / (double)Value;
	}
	return stepPx;
}

/** Sets a new position, relays the change to an associated textarea and calls
	any existing GUI OnChange callback */
void ScrollBar::SetPos(ieDword NewPos)
{
	if (NewPos > Value) NewPos = Value;

	if (( State & SLIDER_GRAB ) == 0){
		// set the slider to the exact y for NewPos.
		// if the slider is grabbed dont set position! otherwise you will get a flicker as it bounces between exact positioning and arbitrary
		SliderYPos = (unsigned short) (NewPos * GetStep());
	}
	if (Pos && ( Pos == NewPos )) {
		return;
	}

	MarkDirty();
	Pos = (ieWord) NewPos;
	if (textarea) {
		textarea->SetRow( Pos );
	}
	if (VarName[0] != 0) {
		core->GetDictionary()->SetAt( VarName, Pos );
	}
	RunEventHandler( ScrollBarOnChange );
}

/** Sets the Pos for a given y pixel coordinate (control coordinates) */
void ScrollBar::SetPosForY(short y)
{
	double stepPx = GetStep();
	if (y && stepPx && Value > 0) {// if the value is 0 we are simultaneously at both the top and bottom so there is nothing to do
		// clamp the value
		y -= (frame.h - SliderRange) / 2;
		if (y < 0) y = 0;
		else if (y > SliderRange) y = SliderRange;

		unsigned short NewPos = (unsigned short)(y / stepPx);
		if (Pos != NewPos) {
			SetPos(NewPos);
		} else {
			MarkDirty();
		}
		SliderYPos = y;
	} else {
		// top is our default position
		SetPos(0);
		SliderYPos = 0;
	}
}

/** Refreshes the ScrollBar according to a guiscript variable */
void ScrollBar::UpdateState(unsigned int Sum)
{
	SetPos( Sum );
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollUp()
{
	SetPos(Pos >= ScrollDelta ? Pos - ScrollDelta : 0);
}

/** SDL < 1.3 Mousewheel support */
void ScrollBar::ScrollDown()
{
	SetPos(Pos + ScrollDelta);
}

bool ScrollBar::IsOpaque() const
{
	/*
	 IWD2 scrollbars have a transparent trough and we dont have a good way to know about such things.
	 returning false for now.
	return (Frames[IE_GUI_SCROLLBAR_TROUGH]->Width >= Frames[IE_GUI_SCROLLBAR_SLIDER]->Width);
	 */
	return false;
}

/** Draws the ScrollBar control */
void ScrollBar::DrawSelf(Region drawFrame, const Region& /*clip*/)
{
	Video *video=core->GetVideoDriver();
	int upMy = GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED);
	int doMy = GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED);
	unsigned int domy = (frame.h - doMy);

	//draw the up button
	if (( State & UP_PRESS ) != 0) {
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_PRESSED], drawFrame.x, drawFrame.y, &drawFrame );
	} else {
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_UP_UNPRESSED], drawFrame.x, drawFrame.y, &drawFrame );
	}
	int maxy = drawFrame.y + drawFrame.h - GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED);
	int stepy = GetFrameHeight(IE_GUI_SCROLLBAR_TROUGH);
	// some "scrollbars" are sized to just show the up and down buttons
	// we must skip the trough (and slider) in those cases
	if (maxy >= stepy) {
		// draw the trough
		if (stepy) {
			Region rgn( drawFrame.x, drawFrame.y + upMy, drawFrame.w, domy - upMy);
			for (int dy = drawFrame.y + upMy; dy < maxy; dy += stepy) {
				//TROUGH surely exists if it has a nonzero height
				video->BlitSprite( Frames[IE_GUI_SCROLLBAR_TROUGH],
					drawFrame.x + Frames[IE_GUI_SCROLLBAR_TROUGH]->XPos + ( ( frame.w - Frames[IE_GUI_SCROLLBAR_TROUGH]->Width - 1 ) / 2 ),
					dy + Frames[IE_GUI_SCROLLBAR_TROUGH]->YPos, &rgn );
			}
		}
		// draw the slider
		short slx = ((frame.w - Frames[IE_GUI_SCROLLBAR_SLIDER]->Width - 1) / 2 );
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_SLIDER],
						  drawFrame.x + slx + Frames[IE_GUI_SCROLLBAR_SLIDER]->XPos,
						  drawFrame.y + Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos + upMy + SliderYPos, &drawFrame );
	}
	//draw the down button
	if (( State & DOWN_PRESS ) != 0) {
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_PRESSED], drawFrame.x, maxy, &drawFrame );
	} else {
		video->BlitSprite( Frames[IE_GUI_SCROLLBAR_DOWN_UNPRESSED], drawFrame.x, maxy, &drawFrame );
	}
}

/** Mouse Button Down */
void ScrollBar::OnMouseDown(const Point& p, unsigned short Button, unsigned short /*Mod*/)
{
	//removing the double click flag, use a more sophisticated method
	//if it is needed later
	Button&=GEM_MB_NORMAL;
	if (p.y <= GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED) ) {
		State |= UP_PRESS;
		ScrollUp();
		return;
	}
	if (p.y >= frame.h - GetFrameHeight(IE_GUI_SCROLLBAR_DOWN_UNPRESSED)) {
		State |= DOWN_PRESS;
		ScrollDown();
		return;
	}
	// if we made it this far we will jump the nib to y and "grab" it
	// this way we only need to click once to jump+scroll
	State |= SLIDER_GRAB;
	ieWord sliderPos = SliderYPos + GetFrameHeight(IE_GUI_SCROLLBAR_UP_UNPRESSED);
	if (p.y >= sliderPos && p.y <= sliderPos + GetFrameHeight(IE_GUI_SCROLLBAR_SLIDER)) {
		// FIXME: hack. we shouldnt mess with the sprite position should we?
		Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos = p.y - sliderPos - GetFrameHeight(IE_GUI_SCROLLBAR_SLIDER)/2;
		return;
	}
	SetPosForY(p.y);
}

/** Mouse Button Up */
void ScrollBar::OnMouseUp(const Point&, unsigned short /*Button*/, unsigned short /*Mod*/)
{
	MarkDirty();
	State = 0;
	Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos = 0; //this is to clear any offset incurred by grabbing the slider

}

/** Mousewheel scroll */
void ScrollBar::OnMouseWheelScroll(short /*x*/, short y)
{
	if ( State == 0 ){//dont allow mousewheel to do anything if the slider is being interacted with already.
		unsigned short fauxY = SliderYPos;
		if ((short)fauxY + y <= 0) fauxY = 0;
		else fauxY += y;
		SetPosForY(fauxY);
	}
}

/** Mouse Over Event */
void ScrollBar::OnMouseOver(const Point& p)
{
	if (State&SLIDER_GRAB) {
		SetPosForY(p.y - Frames[IE_GUI_SCROLLBAR_SLIDER]->YPos);
	}
}

/** Sets the Maximum Value of the ScrollBar */
void ScrollBar::SetMax(unsigned short Max)
{
	Value = Max;
	if (Max == 0) {
		SetPos( 0 );
	} else if (Pos > Max){
		SetPos( Max );
	}
}

/** Sets the ScrollBarOnChange event (guiscript callback) */
bool ScrollBar::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
	case IE_GUI_SCROLLBAR_ON_CHANGE:
		ScrollBarOnChange = handler;
		break;
	default:
		return false;
	}

	return true;
}

}
