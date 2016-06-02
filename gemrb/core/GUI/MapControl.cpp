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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "GUI/MapControl.h"

#include "win32def.h"
#include "ie_cursors.h"

#include "Game.h"
#include "GlobalTimer.h"
#include "Map.h"
#include "Sprite2D.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"

namespace GemRB {

#define MAP_NO_NOTES   0
#define MAP_VIEW_NOTES 1
#define MAP_SET_NOTE   2
#define MAP_REVEAL     3

typedef enum {black=0, gray, violet, green, orange, red, blue, darkblue, darkgreen} colorcode;

Color colors[]={
 { 0x00, 0x00, 0x00, 0xff }, //black
 { 0x60, 0x60, 0x60, 0xff }, //gray
 { 0xa0, 0x00, 0xa0, 0xff }, //violet
 { 0x00, 0xff, 0x00, 0xff }, //green
 { 0xff, 0xff, 0x00, 0xff }, //orange
 { 0xff, 0x00, 0x00, 0xff }, //red
 { 0x00, 0x00, 0xff, 0xff }, //blue
 { 0x00, 0x00, 0x80, 0xff }, //darkblue
 { 0x00, 0x80, 0x00, 0xff }  //darkgreen
};

MapControl::MapControl(const Region& frame)
	: Control(frame)
{
	ControlType = IE_GUI_MAP;

	LinkedLabel = NULL;
	ScrollX = 0;
	ScrollY = 0;
	NotePosX = 0;
	NotePosY = 0;
	MapWidth = MapHeight = ViewWidth = ViewHeight = 0;
	XCenter = YCenter = 0;
	lastMouseX = lastMouseY = 0;
	mouseIsDown = false;
	convertToGame = true;
	memset(Flag,0,sizeof(Flag) );

	// initialize var and event callback to no-ops
	VarName[0] = 0;
	ResetEventHandler( MapControlOnPress );
	ResetEventHandler( MapControlOnRightPress );

	MyMap = core->GetGame()->GetCurrentArea();
	if (MyMap && MyMap->SmallMap) {
		MapMOS = MyMap->SmallMap;
		MapMOS->acquire();
	} else
		MapMOS = NULL;
}

MapControl::~MapControl(void)
{
	if (MapMOS) {
		Sprite2D::FreeSprite(MapMOS);
	}
	for(int i=0;i<8;i++) {
		if (Flag[i]) {
			Sprite2D::FreeSprite(Flag[i]);
		}
	}
}

// Draw fog on the small bitmap
void MapControl::DrawFog(const Region& /*rgn*/)
{
/*
	Video *video = core->GetVideoDriver();
	Point p = rgn.Origin();

	// FIXME: this is ugly, the knowledge of Map and ExploredMask
	//   sizes should be in Map.cpp
	int w = MyMap->GetWidth() / 2;
	int h = MyMap->GetHeight() / 2;

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			p.x = (MAP_MULT * x), p.y = (MAP_MULT * y);
			bool visible = MyMap->IsVisible( p, true );
			if (!visible) {
				p.x = (MAP_DIV * x), p.y = (MAP_DIV * y);
				Region rgn = Region ( ConvertPointToScreen(p), Size(MAP_DIV, MAP_DIV) );
				video->DrawRect( rgn, colors[black] );
			}
		}
	}
*/
}

// To be called after changes in control's or screen geometry
void MapControl::Realize()
{
	/*
	// FIXME: ugly!! How to get area size in pixels?
	//Map *map = core->GetGame()->GetCurrentMap();
	//MapWidth = map->GetWidth();
	//MapHeight = map->GetHeight();

	if (MapMOS) {
		MapWidth = (short) MapMOS->Width;
		MapHeight = (short) MapMOS->Height;
	} else {
		MapWidth = 0;
		MapHeight = 0;
	}

	// FIXME: ugly hack! What is the actual viewport size?
	ViewWidth = (short) (core->Width * MAP_DIV / MAP_MULT);
	ViewHeight = (short) (core->Height * MAP_DIV / MAP_MULT);

	XCenter = (short) (frame.w - MapWidth ) / 2;
	YCenter = (short) (frame.h - MapHeight ) / 2;
	if (XCenter < 0) XCenter = 0;
	if (YCenter < 0) YCenter = 0;
	*/
}

void MapControl::UpdateState(unsigned int Sum)
{
	Value = Sum;
	MarkDirty();
}

/** Draws the Control on the Output Display */
void MapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
	Realize();

	Video* video = core->GetVideoDriver();
	if (MapMOS) {
		video->BlitSprite( MapMOS, 0, 0, &rgn );
	}

	if (core->FogOfWar&FOG_DRAWFOG)
		DrawFog(rgn);

	Region vp = core->GetGameControl()->Viewport();
	vp.w = ViewWidth;
	vp.h = ViewHeight;

	/*
	if ((vp.x + vp.w) >= MAP_TO_SCREENX( frame.w ))
		vp.w = MAP_TO_SCREENX( frame.w ) - vp.x;
	if ((vp.y + vp.h) >= MAP_TO_SCREENY( frame.h ))
		vp.h = MAP_TO_SCREENY( frame.h ) - vp.y;
*/
	video->DrawRect( vp, colors[green], false );

	// Draw PCs' ellipses
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC( i, true );
		if (MyMap->HasActor(actor) ) {
			video->DrawEllipse( actor->Pos.x, actor->Pos.y, 3, 2, actor->Selected ? colors[green] : colors[darkgreen] );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (Value!=MAP_NO_NOTES) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			Sprite2D *anim = Flag[mn.color&7];
			Point pos = mn.Pos;
			/*
			if (convertToGame) {
				vp.x = GAME_TO_SCREENX(mn.Pos.x);
				vp.y = GAME_TO_SCREENY(mn.Pos.y);
			} else { //pst style
				vp.x = MAP_TO_SCREENX(mn.Pos.x);
				vp.y = MAP_TO_SCREENY(mn.Pos.y);
				pos.x = pos.x * MAP_MULT / MAP_DIV;
				pos.y = pos.y * MAP_MULT / MAP_DIV;
			}
			 */

			//Skip unexplored map notes
			bool visible = MyMap->IsVisible( pos, true );
			if (!visible)
				continue;

			if (anim) {
				video->BlitSprite( anim, vp.x - anim->Width/2, vp.y - anim->Height/2, &rgn );
			} else {
				video->DrawEllipse( (short) vp.x, (short) vp.y, 6, 5, colors[mn.color&7] );
			}
		}
	}
}

/** Mouse Over Event */
void MapControl::OnMouseOver(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());
	if (mouseIsDown) {
		MarkDirty();
		ScrollX -= p.x - lastMouseX;
		ScrollY -= p.y - lastMouseY;

		if (ScrollX > MapWidth - frame.w)
			ScrollX = MapWidth - frame.w;
		if (ScrollY > MapHeight - frame.h)
			ScrollY = MapHeight - frame.h;
		if (ScrollX < 0)
			ScrollX = 0;
		if (ScrollY < 0)
			ScrollY = 0;
		ViewHandle(p.x, p.y);
	}

	lastMouseX = p.x;
	lastMouseY = p.y;

	// FIXME: implement cursor changing
	switch (Value) {
		case MAP_REVEAL: //for farsee effect
			//Owner->Cursor = IE_CURSOR_CAST;
			break;
		case MAP_SET_NOTE:
			//Owner->Cursor = IE_CURSOR_GRAB;
			break;
		default:
			//Owner->Cursor = IE_CURSOR_NORMAL;
			break;
	}

	if (Value == MAP_VIEW_NOTES || Value == MAP_SET_NOTE || Value == MAP_REVEAL) {
		Point mp;
		/*
		unsigned int dist;
		if (convertToGame) {
			mp.x = (short) SCREEN_TO_GAMEX(p.x);
			mp.y = (short) SCREEN_TO_GAMEY(p.y);
			dist = 100;
		} else {
			mp.x = (short) SCREEN_TO_MAPX(p.x);
			mp.y = (short) SCREEN_TO_MAPY(p.y);
			dist = 16;
		}
		int i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			if (Distance(mp, mn.Pos)<dist) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn.text );
				}
				NotePosX = mn.Pos.x;
				NotePosY = mn.Pos.y;
				return;
			}
		}
		*/
		NotePosX = mp.x;
		NotePosY = mp.y;
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( L"" );
	}
}

void MapControl::ClickHandle(unsigned short Button)
{
	core->GetDictionary()->SetAt( "MapControlX", NotePosX );
	core->GetDictionary()->SetAt( "MapControlY", NotePosY );
	switch(Button) {
		case GEM_MB_ACTION:
			RunEventHandler( MapControlOnPress );
			break;
		case GEM_MB_MENU:
			RunEventHandler( MapControlOnRightPress );
			break;
	}
}

void MapControl::ViewHandle(unsigned short x, unsigned short y)
{
	short xp = (short) (x - ViewWidth / 2);
	short yp = (short) (y - ViewHeight / 2);

	if (xp + ViewWidth > MapWidth) xp = MapWidth - ViewWidth;
	if (yp + ViewHeight > MapHeight) yp = MapHeight - ViewHeight;
	if (xp < 0) xp = 0;
	if (yp < 0) yp = 0;

	// clear any previously scheduled moves and then do it asap, so it works while paused
	Point p;//(xp * MAP_MULT / MAP_DIV, yp * MAP_MULT / MAP_DIV);
	core->timer->SetMoveViewPort( p, 0, false );
}

/** Mouse Button Down */
void MapControl::OnMouseDown(const Point& /*p*/, unsigned short /*Button*/, unsigned short /*Mod*/)
{
	mouseIsDown = true;
	/*
	Region vp = core->GetGameControl()->Viewport();
	vp.w = vp.x+ViewWidth*MAP_MULT/MAP_DIV;
	vp.h = vp.y+ViewHeight*MAP_MULT/MAP_DIV;
	ViewHandle(p.x, p.y);
	lastMouseX = p.x;
	lastMouseY = p.y;
	*/
}

/** Mouse Button Up */
void MapControl::OnMouseUp(const Point& /*p*/, unsigned short Button, unsigned short /*Mod*/)
{
	if (!mouseIsDown) {
		return;
	}

	if (Button&GEM_MB_ACTION&GEM_MB_DOUBLECLICK) {
		Owner->Close();
	}

	mouseIsDown = false;
	/*
	switch(Value) {
		case MAP_REVEAL:
			ViewHandle(p.x, p.y);
			NotePosX = (short) SCREEN_TO_MAPX(p.x) * MAP_MULT / MAP_DIV;
			NotePosY = (short) SCREEN_TO_MAPY(p.y) * MAP_MULT / MAP_DIV;
			ClickHandle(Button);
			return;
		case MAP_NO_NOTES:
			ViewHandle(p.x, p.y);
			return;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (Button == GEM_MB_ACTION) {
				ViewHandle(p.x, p.y);
			}
			ClickHandle(Button);
			return;
		default:
			return Control::OnMouseUp(p, Button, Mod);
	}
	*/
}

/** Special Key Press */
bool MapControl::OnKeyPress(const KeyboardEvent& Key, unsigned short mod)
{
	ieDword keyScrollSpd = 64;
	core->GetDictionary()->Lookup("Keyboard Scroll Speed", keyScrollSpd);
	switch (Key.keycode) {
		case GEM_LEFT:
			ScrollX -= keyScrollSpd;
			break;
		case GEM_UP:
			ScrollY -= keyScrollSpd;
			break;
		case GEM_RIGHT:
			ScrollX += keyScrollSpd;
			break;
		case GEM_DOWN:
			ScrollY += keyScrollSpd;
			break;
		default:
			return Control::OnKeyPress(Key, mod);
	}

	if (ScrollX > MapWidth - frame.w)
		ScrollX = MapWidth - frame.w;
	if (ScrollY > MapHeight - frame.h)
		ScrollY = MapHeight - frame.h;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
	MarkDirty();
	return true;
}

bool MapControl::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
		case IE_GUI_MAP_ON_PRESS:
			MapControlOnPress = handler;
			break;
		case IE_GUI_MAP_ON_RIGHT_PRESS:
			MapControlOnRightPress = handler;
			break;
		default:
			return false;
	}

	return true;
}

}
