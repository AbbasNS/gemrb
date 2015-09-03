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

#include "View.h"

#include "GUI/GUIScriptInterface.h"
#include "GUI/ScrollBar.h"
#include "Interface.h"
#include "Sprite2D.h"
#include "Video.h"

namespace GemRB {

View::DragOp::DragOp(View* v) : dragView(v)
{
}

View::DragOp::~DragOp() {
	dragView->CompleteDragOperation(*this);
}

View::View(const Region& frame)
	: frame(frame)
{
	scriptingRef = NULL;
	scrollbar = NULL;
	background = NULL;
	superView = NULL;

	dirty = true;
	visible = true;
	resizeFlags = RESIZE_NONE;
	flags = 0;
}

View::~View()
{
	DeleteScriptingRef();
	if (superView) {
		superView->RemoveSubview(this);
	}
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* view = *it;
		view->superView = NULL;
		delete view;
	}
	if (background) background->release();
}

void View::SetBackground(Sprite2D* bg)
{
	bg->acquire();
	if (background) background->release();
	background = bg;

	MarkDirty();
}

void View::MarkDirty()
{
	dirty = true;
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		(*it)->MarkDirty();
	}
}

bool View::NeedsDraw() const
{
	if (frame.Dimensions().IsEmpty()) return false;

	// subviews are drawn over when the superview is drawn, so always redraw when super needs it.
	if (superView && superView->IsAnimated()) return true;

	return (dirty || IsAnimated());
}

void View::DrawSubviews(bool drawBg)
{
	std::list<View*>::iterator it;
	for (it = subViews.begin(); it != subViews.end(); ++it) {
		View* v = *it;
		if (drawBg && !v->IsOpaque() && v->NeedsDraw()) {
			const Region& fromClip = v->Frame();
			DrawBackground(&fromClip);
		}
		v->Draw();
	}
}

void View::DrawBackground(const Region* rgn) const
{
	if (superView && !IsOpaque()) {
		if (rgn) {
			Region r = frame.Intersect(Region(ConvertPointToSuper(rgn->Origin()), rgn->Dimensions()));
			superView->DrawBackground(&r);
		} else {
			// already in super coordinates
			superView->DrawBackground(&frame);
		}
	}
	if (background) {
		Video* video = core->GetVideoDriver();
		if (rgn) {
			Region bgRgn = Region(background->XPos, background->YPos, background->Width, background->Height);
			Region intersect = rgn->Intersect(bgRgn);
			Point screenPt = ConvertPointToScreen(intersect.Origin());
			Region toClip(screenPt, intersect.Dimensions());
			video->BlitSprite( background, intersect, toClip);
		} else {
			Point dp = ConvertPointToScreen(Point(background->XPos, background->YPos));
			video->BlitSprite( background, dp.x, dp.y, true );
		}
	}
}

void View::Draw()
{
	if (!visible) return;

	Video* video = core->GetVideoDriver();
	const Region& clip = video->GetScreenClip();
	const Region& drawFrame = Region(ConvertPointToScreen(Point(0,0)), Dimensions());
	const Region& intersect = clip.Intersect(drawFrame);
	if (intersect.Dimensions().IsEmpty()) return; // outside the window/screen

	// clip drawing to the control bounds, then restore after drawing
	video->SetScreenClip(&intersect);
	// notify subclasses that drawing is about to happen. could pass the rects too, but no need ATM.
	WillDraw();

	bool drawBg = (background != NULL) || !IsOpaque();
	if (NeedsDraw()) {
		if (background) {
			DrawBackground(NULL);
			drawBg = false;
		}
		DrawSelf(drawFrame, intersect);
		dirty = false;
	}

	// always call draw on subviews because they can be dirty without us
	DrawSubviews(drawBg);
	// restore the screen clip
	video->SetScreenClip(&clip);
}

Point View::ConvertPointToSuper(const Point& p) const
{
	return p + Origin();
}

Point View::ConvertPointFromSuper(const Point& p) const
{
	return p - Origin();
}

Point View::ConvertPointToScreen(const Point& p) const
{
	// NULL superview is screen
	Point newP = ConvertPointToSuper(p);
	if (superView) {
		newP = superView->ConvertPointToScreen(newP);
	}
	return newP;
}

Point View::ConvertPointFromScreen(const Point& p) const
{
	// NULL superview is screen
	Point newP = ConvertPointFromSuper(p);
	if (superView) {
		newP = superView->ConvertPointFromScreen(newP);
	}
	return newP;
}

void View::AddSubviewInFrontOfView(View* front, const View* back)
{
	if (front == NULL) return;

	std::list<View*>::iterator it;
	it = std::find(subViews.begin(), subViews.end(), back);

	View* super = front->superView;
	if (super == this) {
		// already here, but may need to move the view
		std::list<View*>::iterator cur;
		cur = std::find(subViews.begin(), subViews.end(), front);
		subViews.splice(it, subViews, cur);
	} else {
		if (super != NULL) {
			front->superView->RemoveSubview(front);
		}
		subViews.insert(it, front);
	}

	front->superView = this;
	front->MarkDirty(); // must redraw the control now
	SubviewAdded(front, this);
	// FIXME: we probably shouldnt allow things in front of the scrollbar
}

View* View::RemoveSubview(const View* view)
{
	if (!view || view->superView != this) {
		return NULL;
	}

	std::list<View*>::iterator it;
	it = std::find(subViews.begin(), subViews.end(), view);
	assert(it != subViews.end());

	View* subView = *it;
	if (scrollbar && subView == scrollbar) {
		Size s = Dimensions();
		s.w -= scrollbar->Dimensions().w;
		SetFrameSize(s);
		scrollbar = NULL;
	}
	subViews.erase(it);
	const Region& viewFrame = subView->Frame();
	DrawBackground(&viewFrame);
	SubviewRemoved(subView, this);
	subView->superView = NULL;
	return subView;
}

void View::SubviewAdded(View* view, View* parent)
{
	if (superView) {
		superView->SubviewAdded(view, parent);
	}
}

void View::SubviewRemoved(View* view, View* parent)
{
	if (superView) {
		superView->SubviewRemoved(view, parent);
	}
}

bool View::EventHit(const Point& p) const
{
	if (!IsOpaque() && background) {
		return !background->IsPixelTransparent(p.x, p.y);
	}
	return true;
}

View* View::SubviewAt(const Point& p, bool ignoreTransparency, bool recursive)
{
	// iterate backwards because the backmost control would be drawn on top
	std::list<View*>::reverse_iterator it;
	for (it = subViews.rbegin(); it != subViews.rend(); ++it) {
		View* v = *it;
		const Region& viewFrame = v->Frame();
		Point subP = v->ConvertPointFromSuper(p);
		if (viewFrame.PointInside(p) && (ignoreTransparency || v->EventHit(subP))) {
			if (recursive) {
				View* subV = v->SubviewAt(subP, ignoreTransparency, recursive);
				v = (subV) ? subV : v;
			}
			return v;
		}
	}
	return NULL;
}

void View::SetFrame(const Region& r)
{
	SetFrameOrigin(r.Origin());
	SetFrameSize(r.Dimensions());
}

void View::SetFrameOrigin(const Point& p)
{
	frame.x = p.x;
	frame.y = p.y;
	MarkDirty();
}

void View::SetFrameSize(const Size& s)
{
	Size oldSize = frame.Dimensions();
	if (oldSize == s) return;

	frame.w = s.w;
	frame.h = s.h;

	SizeChanged(oldSize);

	if (resizeFlags&RESIZE_SUBVIEWS) {
		std::list<View*>::iterator it;
		for (it = subViews.begin(); it != subViews.end(); ++it) {
			View* subview = *it;
			Region newSubFrame = subview->Frame();
			newSubFrame.w += s.w - oldSize.w;
			newSubFrame.h += s.h - oldSize.h;
			subview->SetFrame(newSubFrame);
		}
	}
	MarkDirty();
}

void View::SetScrollBar(ScrollBar* sb)
{
	delete RemoveSubview(scrollbar);
	if (sb) {
		Size sbSize = sb->Dimensions();
		Point origin = ConvertPointFromSuper(sb->Origin());
		sb->SetFrame(Region(origin, sbSize));
		AddSubviewInFrontOfView(sb);
		scrollbar = sb;
		Size s = Dimensions();
		s.w += sbSize.w;
		SetFrameSize(s);
	}
}

bool View::SetFlags(int arg_flags, int opcode)
{
	ieDword newFlags = flags;
	switch (opcode) {
		case BM_SET:
			newFlags = arg_flags;  //set
			break;
		case BM_AND:
			newFlags &= arg_flags;
			break;
		case BM_OR:
			newFlags |= arg_flags; //turn on
			break;
		case BM_XOR:
			newFlags ^= arg_flags;
			break;
		case BM_NAND:
			newFlags &= ~arg_flags;//turn off
			break;
		default:
			return false;
	}
	flags = newFlags;
	MarkDirty();
	return true;
}

void View::SetTooltip(const String& string)
{
	tooltip = string;
	TrimString(tooltip); // for proper vertical alaignment
}

// View simpler either forwards events to concrete subclasses, or to its attached scrollbar
void View::OnMouseOver(const Point& p)
{
	if (superView) {
		superView->OnMouseOver(ConvertPointToSuper(p));
	}
}

void View::OnMouseDown(const Point& p, unsigned short button, unsigned short mod)
{
	if (scrollbar && (button == GEM_MB_SCRLUP || button == GEM_MB_SCRLDOWN)) {
		// forward to scrollbar
		scrollbar->OnMouseDown(scrollbar->ConvertPointFromSuper(p), button, mod);
	} else if (superView) {
		superView->OnMouseDown(ConvertPointToSuper(p), button, mod);
	}
}

void View::OnMouseUp(const Point& p, unsigned short button, unsigned short mod)
{
	if (superView) {
		superView->OnMouseUp(ConvertPointToSuper(p), button, mod);
	}
}

void View::OnMouseWheelScroll(short x, short y)
{
	if (scrollbar) {
		scrollbar->OnMouseWheelScroll( x, y );
	}
	if (superView) {
		superView->OnMouseWheelScroll( x, y );
	}
}

bool View::OnSpecialKeyPress(unsigned char key)
{
	if (scrollbar && (key == GEM_UP || key == GEM_DOWN)) {
		return scrollbar->OnSpecialKeyPress(key);
	}

	return false;
}

void View::AssignScriptingRef(ViewScriptingRef* ref)
{
	delete scriptingRef;
	scriptingRef = ref;
}

inline void View::DeleteScriptingRef()
{
	ScriptEngine::UnregisterScriptingRef(scriptingRef);
	AssignScriptingRef(NULL);
}

}
