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

#include "WindowManager.h"

#include "GameData.h"
#include "Interface.h"
#include "ImageMgr.h"
#include "Window.h"

#include "defsounds.h"

#define WIN_IT(w) \
std::find(windows.begin(), windows.end(), w)

namespace GemRB {

int WindowManager::ToolTipDelay = 500;
unsigned long WindowManager::TooltipTime = 0;

Holder<Sprite2D> WindowManager::CursorMouseUp;
Holder<Sprite2D> WindowManager::CursorMouseDown;

WindowManager& WindowManager::DefaultWindowManager()
{
	static WindowManager wm(core->GetVideoDriver());
	return wm;
}

WindowManager::WindowManager(Video* vid)
{
	assert(vid);

	hoverWin = NULL;
	modalWin = NULL;

	modalShadow = ShadowNone;
	eventMgr.AddEventTap(new MethodCallback<WindowManager, const Event&, bool>(this, &WindowManager::DispatchEvent));

	EventMgr::EventCallback* cb = new MethodCallback<WindowManager, const Event&, bool>(this, &WindowManager::HotKey);
	eventMgr.RegisterHotKeyCallback(cb, GEM_TAB, 0);
	eventMgr.RegisterHotKeyCallback(cb, 'f', GEM_MOD_CTRL);

	screen = Region(Point(), vid->GetScreenSize());
	// FIXME: technically we should unset the current video event manager...
	vid->SetEventMgr(&eventMgr);

	gameWin = new Window(screen, *this);
	gameWin->SetFlags(Window::Borderless, OP_OR);
	gameWin->SetFrame(screen);

	cursorBuf = vid->CreateBuffer(screen);
	cursorBuf->SetColorKey(ColorGreen);

	// set the buffer that always gets cleared just in case anything
	// tries to draw
	vid->SetDrawingBuffer(cursorBuf);
	video = vid;
	// TODO: changing screen size should adjust window positions too
	// TODO: how do we get notified if the Video driver changes size?
}

WindowManager::~WindowManager()
{
	video->DestroyBuffer(cursorBuf);
	delete gameWin;
}

void WindowManager::RedrawAll() const
{
	for (size_t i=0; i < windows.size(); i++) {
		windows[i]->MarkDirty();
	}
}

bool WindowManager::IsOpenWindow(Window* win) const
{
	WindowList::const_iterator it = WIN_IT(win);
	return it != windows.end();
}

bool WindowManager::IsPresentingModalWindow() const
{
	return modalWin != NULL;
}

/** Show a Window in Modal Mode */
bool WindowManager::MakeModal(Window* win, ModalShadow Shadow)
{
	if (!IsOpenWindow(win) || IsPresentingModalWindow()) return false;

	FocusWindow( win );
	modalWin = win;

	modalShadow = Shadow;
	if (win->Flags()&Window::Borderless) {
		core->PlaySound(DS_WINDOW_OPEN);
	}

	return true;
}

/** Sets a Window on the Top */
bool WindowManager::FocusWindow(Window* win)
{
	if (!IsPresentingModalWindow() && OrderFront(win)) {
		win->SetDisabled(false);
		return true;
	}
	return false;
}

bool WindowManager::OrderFront(Window* win)
{
	return OrderRelativeTo(win, windows.front(), true);
}

bool WindowManager::OrderBack(Window* win)
{
	return OrderRelativeTo(win, windows.back(), false);
}

bool WindowManager::OrderRelativeTo(Window* win, Window* win2, bool front)
{
	if (windows.size() < 2 || win == win2) return false;

	WindowList::iterator it = WIN_IT(win), it2 = WIN_IT(win2);
	if (it == windows.end() || it2 == windows.end()) return false;

	windows.erase(it);
	// it2 may have become invalid after erase
	it2 = WIN_IT(win2);
	windows.insert((front) ? it2 : ++it2, win);
	return true;
}

Window* WindowManager::MakeWindow(const Region& rgn)
{
	Window* win = new Window(rgn, *this);
	windows.push_back(win);

	if (closedWindows.size() > 1) {
		// delete all but the most recent closed window
		// FIXME: this is intended for caching (in case the last window is reopened)
		// but currently there is no way to reopen a window (other than recreating it)
		// IMPORTANT: aside from caching, the last window probably is in a callback that resulted in this new window...
		WindowList::iterator it = closedWindows.begin();
		for (; it != --closedWindows.end();) {
			Window* win = *it;
			delete win;
			it = closedWindows.erase(it);
		}
	}

	return win;
}

//this function won't delete the window, just queue it for deletion
//it will be deleted when another window is opened
//regardless, the window deleted is inaccessible for gui scripts and
//other high level functions from now
// this is a caching mechanisim in case the window is reopened
void WindowManager::CloseWindow(Window* win)
{
	WindowList::iterator it = WIN_IT(win);
	if (it == windows.end()) return;

	if (win == modalWin) {
		if (win->Flags()&Window::Borderless) {
			core->PlaySound(DS_WINDOW_CLOSE);
		}

		modalWin = NULL;
	}

	bool isFront = it == windows.begin();
	it = windows.erase(it);
	if (it != windows.end()) {
		// the window beneath this must get redrawn
		(*it)->MarkDirty();
		if (isFront)
			(*it)->Focus();
	}
	closedWindows.push_back(win);

	win->DeleteScriptingRef();
	win->SetVisible(false);
	win->SetDisabled(true);
}

bool WindowManager::HotKey(const Event& event)
{
	if (event.type == Event::KeyDown) {
		switch (event.keyboard.keycode) {
			case GEM_TAB:
				TooltipTime -= ToolTipDelay;
				return bool(hoverWin);
			case 'f':
				video->ToggleFullscreenMode();
				return true;
			default:
				return false;
		}
	}
	return false;
}

#define HIT_TEST(e, w) \
((w)->Frame().PointInside(e.mouse.Pos()))

Window* WindowManager::NextEventWindow(const Event& event, WindowList::const_iterator& current)
{
	if (current == windows.end()) {
		// we already we through them all and returned gameWin or modalWin once. there is no target window after gameWin
		return NULL;
	}

	if (IsPresentingModalWindow()) {
		// modal win is always the target for all events no matter what
		// if the window shouldnt handle sreen events outside its bounds (ie negative coords etc)
		// then the Window class should be responsible for bounds checking

		// the NULL return is so that if this is called again after returning modalWindow there is no NextTarget
		current = windows.end(); // invalidate the iterator, no other target s possible.
		return modalWin;
	}

	while (current != windows.end()) {
		Window* win = *current++;
		if (win->IsVisible() && (!event.isScreen || HIT_TEST(event,win))) {
			// NOTE: we want to "target" the first window hit regardless of it being disabled or otherwise
			// we still need to update which window is under the mouse and block events from reaching the windows below
			return win;
		}
	}

	// we made it though with no takers...
	// send it to the game win
	return gameWin;
}

bool WindowManager::DispatchEvent(const Event& event)
{
	if (windows.empty()) return false;

	if (event.type == Event::MouseMove) {
		TooltipTime = GetTickCount();
	}

	WindowList::const_iterator it = windows.begin();
	while (Window* target = NextEventWindow(event, it)) {
		// disabled windows get no events, but should block them from going to windows below
		if (target->IsDisabled() || target->DispatchEvent(event)) {
			if (event.isScreen) {
				hoverWin = target;
			}
			return true;
		}
	}

	return false;
}

#undef HIT_TEST

void WindowManager::DrawCursor() const
{
	Holder<Sprite2D> cur(NULL);
	if (hoverWin) {
		cur = hoverWin->Cursor();
	}

	if (!cur) {
		// no cursor override
		cur = (eventMgr.MouseDown()) ? CursorMouseDown : CursorMouseUp;
	}
	assert(cur); // must have a cursor

	Point pos = eventMgr.MousePos();
	if (hoverWin && hoverWin->IsDisabled()) {
		// draw greayed cursor
		video->BlitGameSprite(cur.get(), pos.x, pos.y, BLIT_GREY, ColorGray, NULL, NULL, NULL);
	} else {
		// draw normal cursor
		video->BlitSprite(cur.get(), pos.x, pos.y);
	}
}

void WindowManager::DrawTooltip() const
{
	if (hoverWin && hoverWin->TooltipText().length() && TooltipTime && GetTickCount() >= TooltipTime + ToolTipDelay) {
		Point pos = eventMgr.MousePos();
		// TODO: Interface::DrawTooltip logic should be relocated to here (and possibly a Tooltip class)
		core->DrawTooltip(hoverWin->TooltipText(), pos);
	}
}

void WindowManager::DrawWindowFrame() const
{
	// the window buffers dont have room for the frame
	// we also only need to draw the frame *once* (even if it applies to multiple windows)
	// therefore, draw the frame on the cursor buffer (above everything else)
	// ... I'm not 100% certain this works for all use cases.
	// if it doesnt... i think it might be better to just forget about the window frames once the game is loaded

	video->SetScreenClip( NULL );

	Sprite2D* edge = WinFrameEdge(0); // left
	if (edge) {
		// we assume if one fails, they all do
		video->BlitSprite(edge, 0, 0);
		edge = WinFrameEdge(1); // right
		int sideW = edge->Width;
		video->BlitSprite(edge, screen.w - sideW, 0);
		edge = WinFrameEdge(2); // top
		video->BlitSprite(edge, sideW, 0);
		edge = WinFrameEdge(3); // bottom
		video->BlitSprite(edge, sideW, screen.h - edge->Height);
	}
}

void WindowManager::DrawWindows() const
{
	if (!windows.size()) {
		return;
	}

	// TODO: this can probably be done cleaner by drawing to cursorBuf
	static bool modalShield = false;
	if (modalWin) {
		if (!modalShield) {
			// only draw the shield layer once
			Color shieldColor = Color(); // clear
			if (modalShadow == ShadowGray) {
				shieldColor.a = 128;
			} else if (modalShadow == ShadowBlack) {
				shieldColor.a = 0xff;
			}
			video->DrawRect( screen, shieldColor );
			RedrawAll(); // wont actually have any effect until the modal window is dismissed.
			modalShield = true;
		}
		modalWin->Draw();
		return;
	}
	modalShield = false;

	// draw the game window now (beneath everything else); its not part of the windows collection
	gameWin->Draw();

	bool drawFrame = false;
	const Region& frontWinFrame = windows.front()->Frame();
	// we have to draw windows from the bottom up so the front window is drawn last
	WindowList::const_reverse_iterator rit = windows.rbegin();
	for (; rit != windows.rend(); ++rit) {
		Window* win = *rit;
		const Region& frame = win->Frame();

		// FYI... this only checks if the front window obscures... could be covered by another window too
		if (win != windows.front() && win->NeedsDraw()) {
			Region intersect = frontWinFrame.Intersect(frame);
			if (!intersect.Dimensions().IsEmpty()) {
				if (intersect == frame) {
					// this window is completely obscured by the front window
					// we dont have to bother drawing it because IE has no concept of translucent windows
					continue;
				}
			}
		}

		if (!drawFrame && !(win->Flags()&Window::Borderless) && (frame.w < screen.w || frame.h < screen.h)) {
			// the window requires us to draw the frame border (happens later, on the cursor buffer)
			drawFrame = true;
		}

		if (win->IsDisabled()) {
			if (win->NeedsDraw()) {
				// Important to only draw if the window itself is dirty
				// controls on greyed out windows shouldnt be updating anyway
				win->Draw();
				Color fill = { 0, 0, 0, 128 };
				video->DrawRect(frame, fill);
			}
		} else {
			win->Draw();
		}
	}

	cursorBuf->Clear(); // erase the last frame
	video->SetDrawingBuffer(cursorBuf);

	if (drawFrame) {
		DrawWindowFrame();
	}

	// tooltips and cursor are always last
	DrawCursor();
	DrawTooltip();
}

//copies a screenshot into a sprite
Sprite2D* WindowManager::GetScreenshot(Window* win) const
{
	Sprite2D* screenshot = NULL;
	if (win) { // we dont really care if we are managing the window
		// only a screen shot of passed win
		win->MarkDirty();
		win->Draw();
		screenshot = video->GetScreenshot( win->Frame() );
	} else {
		// we dont want cursors and tooltips in the shot
		cursorBuf->Clear();
		screenshot = video->GetScreenshot( screen );
	}

	return screenshot;
}

Sprite2D* WindowManager::WinFrameEdge(int edge) const
{
	std::string refstr = "STON";
	switch (screen.w) {
		case 800:
			refstr += "08";
			break;
		case 1024:
			refstr += "10";
			break;
	}
	switch (edge) {
		case 0:
			refstr += "L";
			break;
		case 1:
			refstr += "R";
			break;
		case 2:
			refstr += "T";
			break;
		case 3:
			refstr += "B";
			break;
	}

	typedef Holder<Sprite2D> FrameImage;
	static std::map<ResRef, FrameImage> frames;

	ResRef ref = refstr.c_str();
	Sprite2D* frame = NULL;
	if (frames.find(ref) != frames.end()) {
		frame = frames[ref].get();
	} else {
		ResourceHolder<ImageMgr> im(ref);
		if (im) {
			frame = im->GetSprite2D();
		}
		frames.insert(std::make_pair(ref, frame));
	}
	
	return frame;
}

#undef WIN_IT

}
