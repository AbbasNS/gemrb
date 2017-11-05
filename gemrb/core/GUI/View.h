/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2015 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __GemRB__View__
#define __GemRB__View__

#include "globals.h"
#include "GUI/EventMgr.h"
#include "Region.h"
#include "ScriptEngine.h"

#include <list>

#define DEBUG_VIEWS 0

namespace GemRB {

class Sprite2D;
class ViewScriptingRef;
class Window;

class GEM_EXPORT View {
public:
	// using Held so we can have polymorphic drag operations
	struct DragOp : public Held<DragOp> {
		View* dragView;
		
		DragOp(View* v);
		virtual ~DragOp();
	};
	
	enum AutoresizeFlags {
		// when a superview frame changes...
		ResizeNone = 0,
		ResizeTop = 1 << 0, // keep my top relative to my super
		ResizeBottom = 1 << 1, // keep my bottom relative to my super
		ResizeVertical = ResizeTop|ResizeBottom, // top+bottom effectively resizes me vertically
		ResizeLeft = 1 << 3, // keep my left relative to my super
		ResizeRight = 1 << 4, // keep my right relative to my super
		ResizeHorizontal = ResizeLeft|ResizeRight, // top+bottom effectively resizes me horizontaly
		ResizeAll = ResizeVertical|ResizeHorizontal, // resize me relative to my super
		
		// TODO: move these to TextContainer
		RESIZE_WIDTH = 1 << 27,		// resize the view horizontally if horizontal content exceeds width
		RESIZE_HEIGHT = 1 << 26	// resize the view vertically if vertical content exceeds width
	};
	
	enum ViewFlags {
		Invisible = 1 << 30,
		Disabled = 1 << 29,
		IgnoreEvents = 1 << 28
	};
private:
	Holder<Sprite2D> background;
	Holder<Sprite2D> cursor;
	std::list<ViewScriptingRef*> scriptingRefs;

	mutable bool dirty;

	// TODO: we could/should generalize this
	// MarkDirty could take a region, and more complicated views could potentially
	// save a lot of drawing time by only drawing their dirty portions (GameControl?)
	Regions dirtyBGRects;
	
	View* eventProxy;

protected:
	View* superView;
	// for convenience because we need to get this so much
	// all it is is a saved pointer returned from View::GetWindow and is updated in AddedToView
	Window* window;

	Region frame;
	std::list<View*> subViews;
	String tooltip;

	// Flags: top byte is reserved for View flags, subclasses may use the remaining bits however they want
	unsigned int flags;
	unsigned short autoresizeFlags; // these flags don't produce notifications

private:
	void DirtyBGRect(const Region&);
	void DrawBackground(const Region*) const;
	void DrawSubviews() const;
	void MarkDirty(const Region*);

	// TODO: to support partial redraws, we should change the clip parameter to a list of dirty rects
	// that have all been clipped to the video ScreenClip
	// subclasses can then use the list to efficiently redraw only those sections that are dirty
	virtual void DrawSelf(Region /*drawFrame*/, const Region& /*clip*/) {};

	void AddedToWindow(Window*);
	void AddedToView(View*);
	void RemovedFromView(View*);
	virtual void SubviewAdded(View* /*view*/, View* /*parent*/) {};
	virtual void SubviewRemoved(View* /*view*/, View* /*parent*/) {};

	// notifications
	virtual void FlagsChanged(unsigned int /*oldflags*/) {}
	virtual void SizeChanged(const Size&) {}
	virtual void OriginChanged(const Point&) {}
	virtual void WillDraw() {}
	virtual void DidDraw() {}
	
	virtual ViewScriptingRef* CreateScriptingRef(ScriptingId id, ResRef group);
	
protected:
	void ClearScriptingRefs();
	
	// TODO: recheck use of IgnoreEvents flag. It may not be needed anymore since we can just return false for those cases.
	// TODO: examine window event dispatch, all bubbling should be handled implicitly here
	// TODO: recheck drag/drop code. probably should return false sometimes.
	
	// these events make no sense to forward
	virtual void OnMouseEnter(const MouseEvent& /*me*/, const DragOp*) {}
	virtual void OnMouseLeave(const MouseEvent& /*me*/, const DragOp*) {}
	
	// default view implementation does nothing but ignore the event
	virtual bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnKeyRelease(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseOver(const MouseEvent& /*me*/) { return false; }
	virtual bool OnMouseDrag(const MouseEvent& /*me*/) { return false; }
	virtual bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/) { return false; }
	virtual bool OnMouseWheelScroll(const Point&) { return false; }

public:
#if DEBUG_VIEWS
	bool debuginfo;
#endif

	View(const Region& frame);
	virtual ~View();

	void Draw();

	void MarkDirty();
	virtual bool NeedsDraw() const;

	virtual bool IsAnimated() const { return false; }
	virtual bool IsOpaque() const;
	virtual bool HitTest(const Point& p) const;

	bool SetFlags(unsigned int arg_flags, int opcode);
	inline unsigned int Flags() const { return flags; }
	bool SetAutoResizeFlags(unsigned short arg_flags, int opcode);
	unsigned short AutoResizeFlags() const { return autoresizeFlags; }

	void SetVisible(bool vis) { SetFlags(Invisible, (vis) ? OP_NAND : OP_OR ); }
	bool IsVisible() const;
	void SetDisabled(bool disable) { SetFlags(Disabled, (disable) ? OP_OR : OP_NAND); }
	bool IsDisabled() const { return flags&Disabled; }
	virtual bool IsDisabledCursor() const { return IsDisabled(); }

	Region Frame() const { return frame; }
	Point Origin() const { return frame.Origin(); }
	Size Dimensions() const { return frame.Dimensions(); }
	void SetFrame(const Region& r);
	void SetFrameOrigin(const Point&);
	void SetFrameSize(const Size&);
	void SetBackground(Sprite2D*);

	// FIXME: I don't think I like this being virtual. Currently required because ScrollView is "overriding" this
	// we perhapps should instead have ScrollView implement SubviewAdded and move the view to its contentView there
	virtual void AddSubviewInFrontOfView(View*, const View* = NULL);
	View* RemoveSubview(const View*);
	View* RemoveFromSuperview();
	View* SubviewAt(const Point&, bool ignoreTransparency = false, bool recursive = false);
	Window* GetWindow() const;
	bool ContainsView(const View* view) const;

	Point ConvertPointToSuper(const Point&) const;
	Point ConvertPointFromSuper(const Point&) const;
	Point ConvertPointToWindow(const Point&) const;
	Point ConvertPointFromWindow(const Point&) const;
	Point ConvertPointToScreen(const Point&) const;
	Point ConvertPointFromScreen(const Point&) const;

	virtual bool CanLockFocus() const { return true; };
	virtual bool CanUnlockFocus() const { return true; };
	virtual bool TracksMouseDown() const { return false; }

	virtual Holder<DragOp> DragOperation() { return Holder<DragOp>(NULL); }
	virtual bool AcceptsDragOperation(const DragOp&) const { return false; }
	virtual void CompleteDragOperation(const DragOp&) {}

	virtual bool KeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);
	virtual bool KeyRelease(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/);

	virtual void MouseEnter(const MouseEvent& /*me*/, const DragOp*);
	virtual void MouseLeave(const MouseEvent& /*me*/, const DragOp*);
	virtual void MouseOver(const MouseEvent& /*me*/);
	virtual void MouseDrag(const MouseEvent& /*me*/);
	virtual void MouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/);
	virtual void MouseUp(const MouseEvent& /*me*/, unsigned short /*Mod*/);
	virtual void MouseWheelScroll(const Point&);

	void SetTooltip(const String& string);
	virtual String TooltipText() const { return tooltip; }
	/* override the standard cursors. default does not override (returns NULL). */
	virtual Sprite2D* Cursor() const { return cursor.get(); }
	void SetCursor(Sprite2D* c);
	void SetEventProxy(View* proxy) { eventProxy = proxy; }

	// GUIScripting
	const ViewScriptingRef* AssignScriptingRef(ScriptingId id, ResRef group);
	const ViewScriptingRef* GetScriptingRef() const;
};

}

#endif /* defined(__GemRB__View__) */
