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

#include "../../includes/win32def.h"
#include "EventMgr.h"
#include "Interface.h"
#include "Video.h"

EventMgr::EventMgr(void)
{
	last_win_focused = NULL;
	// Last window we were over. Used to determine MouseEnter and MouseLeave events
	last_win_over = NULL;
	MButtons = 0;
}

EventMgr::~EventMgr(void)
{
}

void EventMgr::SetOnTop(int Index)
{
	std::vector< int>::iterator t;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		if (( *t ) == Index) {
			topwin.erase( t );
			break;
		}
	}
	if (topwin.size() != 0) {
			topwin.insert( topwin.begin(), Index );
	} else {
		topwin.push_back( Index );
	}
}

void EventMgr::SetDefaultFocus(Window *win)
{
	if (!last_win_focused) {
		last_win_focused = win;
		last_win_focused->SetFocused(last_win_focused->GetControl(0));
	}
	last_win_over = NULL;
}

/** Adds a Window to the Event Manager */
void EventMgr::AddWindow(Window* win)
{
	unsigned int i;

	if (win == NULL) {
		return;
	}
	bool found = false;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == win) {
			goto ok;
		}
		if(windows[i]==NULL) {
			windows[i] = win;
ok:
			SetOnTop( i );
			found = true;
			break;
		}
	}
	if (!found) {
		windows.push_back( win );
		if (windows.size() == 1)
			topwin.push_back( 0 );
		else
			SetOnTop( ( int ) windows.size() - 1 );
	}
	SetDefaultFocus(win);
}
/** Frees and Removes all the Windows in the Array */
void EventMgr::Clear()
{
	topwin.clear();
	windows.clear();
	last_win_focused = NULL;
	last_win_over = NULL;
}

/** Remove a Window from the array */
void EventMgr::DelWindow(Window *win)
//unsigned short WindowID, const char *WindowPack)
{
	if (last_win_focused == win) {
		last_win_focused = NULL;
	}
	if (last_win_over == win) {
		last_win_over = NULL;
	}

	if (windows.size() == 0) {
		return;
	}
	int pos = -1;
	std::vector< Window*>::iterator m;
	for (m = windows.begin(); m != windows.end(); ++m) {
		pos++;
		if ( (*m) == win) {
			(*m) = NULL;
			std::vector< int>::iterator t;
			for (t = topwin.begin(); t != topwin.end(); ++t) {
				if ( (*t) == pos) {
					topwin.erase( t );
					return;
				}
			}
			printMessage("EventManager","Couldn't find window",YELLOW);
		}
	}
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseMove(unsigned short x, unsigned short y)
{
	if (windows.size() == 0) {
		return;
	}
	if (!last_win_focused) {
		return;
	}
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m += ( *t );
		Window *win = *m;
		if (win == NULL)
			continue;
		if (!win->Visible)
			continue;
		if (( win->XPos <= x ) && ( win->YPos <= y )) {
			//Maybe we are on the window, let's check
			if (( win->XPos + win->Width >= x ) &&
				( win->YPos + win->Height >= y )) {
				//Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control* ctrl = win->GetControl( x, y, true );
				//look for the low priority flagged controls (mostly static labels)
				if (ctrl != NULL) {
					ctrl = win->GetControl( x, y, false );
				}
				if (ctrl != win->GetOver()) {
					// Remove tooltip if mouse moved to different control
					core->DisplayTooltip( 0, 0, NULL );
					last_win_focused->OnMouseLeave( x, y );
					last_win_over = win;
					win->OnMouseEnter( x, y, ctrl );
				}
				if (ctrl != NULL) {
					win->OnMouseOver( x, y );
				}
				core->GetVideoDriver()->SetCursor( core->Cursors[win->Cursor], core->Cursors[win->Cursor ^ 1] );
				return;
			}
		}
		//stop going further
		if (( *m )->Visible == WINDOW_FRONT)
			break;
	}
	core->DisplayTooltip( 0, 0, NULL );
}

/** BroadCast Mouse Move Event */
void EventMgr::MouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	std::vector< int>::iterator t;
	std::vector< Window*>::iterator m;
	MButtons |= Button;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		m = windows.begin();
		m += ( *t );
		if (( *m ) == NULL)
			continue;
		if (!( *m )->Visible)
			continue;
		if (( ( *m )->XPos <= x ) && ( ( *m )->YPos <= y )) {
			//Maybe we are on the window, let's check
			if (( ( *m )->XPos + ( *m )->Width >= x ) &&
				( ( *m )->YPos + ( *m )->Height >= y )) {
				//Yes, we are on the Window
				//Let's check if we have a Control under the Mouse Pointer
				Control* ctrl = ( *m )->GetControl( x, y, true );
				if (!ctrl) {
					ctrl = ( *m )->GetControl( x, y, false);
				}
				//printf( "dn: ctrl: %p\n", ctrl );
				last_win_focused = *m;
				if (ctrl != NULL) {
					last_win_focused->SetFocused( ctrl );
					ctrl->OnMouseDown( x - last_win_focused->XPos - ctrl->XPos, y - last_win_focused->YPos - ctrl->YPos, Button, Mod );
				}
				return;
			}
		}
		if (( *m )->Visible == WINDOW_FRONT) //stop looking further
			break;
	}
	if (last_win_focused) {
		last_win_focused->SetFocused(NULL);
	}
}
/** BroadCast Mouse Up Event */
void EventMgr::MouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	MButtons &= ~Button;
	//printf( "up lastF: %p\n", lastF );
	if (last_win_focused == NULL) return;
	Control *last_ctrl_focused = last_win_focused->GetFocus();
	if (last_ctrl_focused == NULL) return;
	last_ctrl_focused->OnMouseUp( x - last_win_focused->XPos - last_ctrl_focused->XPos,
		y - last_win_focused->YPos - last_ctrl_focused->YPos, Button, Mod );
}

/** BroadCast Mouse Idle Event */
void EventMgr::MouseIdle(unsigned long /*time*/)
{
	if (last_win_over == NULL) return;
	Control *ctrl = last_win_over->GetOver();
	if (ctrl == NULL) return;
	ctrl->DisplayTooltip();
}

/** BroadCast Key Press Event */
void EventMgr::KeyPress(unsigned char Key, unsigned short Mod)
{
	if (last_win_focused == NULL) return;
	Control *ctrl = last_win_focused->GetFocus();
	if (ctrl == NULL) return;
	ctrl->OnKeyPress( Key, Mod );
}
/** BroadCast Key Release Event */
void EventMgr::KeyRelease(unsigned char Key, unsigned short Mod)
{
	if (last_win_focused == NULL) return;
	Control *ctrl = last_win_focused->GetFocus();
	if (ctrl == NULL) return;
	ctrl->OnKeyRelease( Key, Mod );
}

/** Special Key Press Event */
void EventMgr::OnSpecialKeyPress(unsigned char Key)
{
	if (!last_win_focused) {
		return;
	}
	Control *ctrl = NULL;

	//the default control will get only GEM_RETURN
	if (Key == GEM_RETURN) {
		ctrl = last_win_focused->GetDefaultControl();
	}
	//if there was no default button set, then the current focus will get it
	if (!ctrl) {
		ctrl = last_win_focused->GetFocus();
	}
	if (ctrl) {
		switch (ctrl->ControlType) {
			//buttons will receive only GEM_RETURN
			case IE_GUI_BUTTON:
				if (Key != GEM_RETURN) {
					return;
				}
				break;
			case IE_GUI_GAMECONTROL:
				//gamecontrols will receive all special keys
				break;
			case IE_GUI_EDIT:
				//editboxes will receive all special keys
				break;
			default:
				//other controls don't receive any
				return;
		}
		ctrl->OnSpecialKeyPress( Key );
	}
}

void EventMgr::SetFocused(Window *win, Control *ctrl)
{
	last_win_focused = win;
	last_win_focused->SetFocused(ctrl);
	//this is to refresh changing mouse cursors should the focus change)
	int x,y;
	core->GetVideoDriver()->GetMousePos(x,y);
	MouseMove((unsigned short) x, (unsigned short) y);
}
