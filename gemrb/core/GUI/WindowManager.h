/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 */

#ifndef __GemRB__WindowManager__
#define __GemRB__WindowManager__

#include "EventMgr.h"
#include "Sprite2D.h"
#include "Video.h"

#include <deque>

namespace GemRB {

class Sprite2D;
class Window;

typedef std::deque<Window*> WindowList;

class WindowManager {
public:
	// Colors of modal window shadow
	// !!! Keep these synchronized with GUIDefines.py !!!
	enum ModalShadow {
		ShadowNone = 0,
		ShadowGray,
		ShadowBlack
	};

	static Holder<Sprite2D> CursorMouseUp;
	static Holder<Sprite2D> CursorMouseDown;

private:
	WindowList windows;
	WindowList closedWindows; // windows that have been closed. kept around temporarily in case they get reopened

	Region screen; // only a Region for convinience. we dont use x,y
	Window* modalWin; // FIXME: is a single pointer sufficient? can't we open another window from within a modal window?
	Window* gameWin;
	Window* hoverWin;
	ModalShadow modalShadow;

	EventMgr eventMgr;

	Video* video;
	VideoBuffer* cursorBuf;// buffer the cursors and tooltips are drawn to (layer 3)

	static int ToolTipDelay;
	static unsigned long TooltipTime;

private:
	bool IsOpenWindow(Window* win) const;
	Sprite2D* WinFrameEdge(int edge) const;

	inline void DrawCursor() const;
	inline void DrawTooltip() const;
	inline void DrawWindowFrame() const;

	Window* NextEventWindow(const Event& event, WindowList::const_iterator& current);
	bool DispatchEvent(const Event&);
	bool HotKey(const Event&);

public:
	WindowManager(Video* vid);
	~WindowManager();

	Window* MakeWindow(const Region& rgn);
	void CloseWindow(Window* win);

	bool OrderFront(Window* win);
	bool OrderBack(Window* win);
	bool OrderRelativeTo(Window* win, Window* win2, bool front);

	bool FocusWindow(Window* win);
	bool MakeModal(Window* win, ModalShadow Shadow = ShadowNone);
	bool IsPresentingModalWindow() const;

	void DrawWindows() const;
	void RedrawAll() const;

	Size ScreenSize() const { return screen.Dimensions(); }

	Sprite2D* GetScreenshot(Window* win) const;
	Window* GetGameWindow() const { return gameWin; }

	static WindowManager& DefaultWindowManager();
};


}

#endif /* defined(__GemRB__WindowManager__) */
