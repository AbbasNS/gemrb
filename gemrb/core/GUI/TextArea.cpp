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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TextArea.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Variables.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

#define EDGE_PADDING 3

namespace GemRB {

TextArea::TextArea(const Region& frame, Font* text, Window* win)
	: Control(frame, win), ftext(text), palettes()
{
	palette = text->GetPalette().get();
	finit = ftext;
	Init();
}

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color textcolor, Color initcolor, Color lowtextcolor,
                   Window* win)
	: Control(frame, win), ftext(text), palettes()
{
	palettes[PALETTE_NORMAL] = new Palette( textcolor, lowtextcolor );
	palette = palettes[PALETTE_NORMAL];

	// quick font optimization (prevents creating unnecessary cap spans)
	finit = (caps != ftext) ? caps : ftext;

	// in case a bad or missing font was specified, use an obvious fallback
	if (!finit) {
		Log(ERROR, "TextArea", "Tried to use missing font, resorting to a fallback!");
		finit = core->GetTextFont();
		ftext = finit;
	}

	if (finit->Baseline < ftext->LineHeight) {
		// FIXME: initcolor is only used for *some* initial fonts
		// this is a hack to workaround the INITIALS font getting its palette set
		// do we have another (more sane) way to tell if a font needs this palette? (something in the BAM?)
		SetPalette(&initcolor, PALETTE_INITIALS);
	} else {
		palettes[PALETTE_INITIALS] = finit->GetPalette().get();
	}

	parser.ResetAttributes(text, palette, finit, palettes[PALETTE_INITIALS]);
	Init();
}

void TextArea::Init()
{
	ControlType = IE_GUI_TEXTAREA;
	rows = 0;
	TextYPos = 0;
	strncpy(VarName, "Selected", sizeof(VarName));

	selectOptions = NULL;
	textContainer = NULL;
	scrollbar = NULL;

	// initialize the Text containers
	SetScrollBar(NULL);
	ClearSelectOptions();
	ClearText();
	SetAnimPicture(NULL);
}

TextArea::~TextArea(void)
{
	for (int i=0; i < PALETTE_TYPE_COUNT; i++) {
		gamedata->FreePalette( palettes[i] );
	}
}

void TextArea::DrawSelf(Region drawFrame, const Region& /*clip*/)
{
	if (animationEnd) {
		if (TextYPos > textContainer->Dimensions().h) {
			// the text is offscreen, this happens with chapter text
			ScrollToY(TextYPos); // reset animation values
		} else {
			// update animation for this next draw cycle
			unsigned long curTime = GetTickCount();
			if (animationEnd.time > curTime) {
				//double animProgress = curTime / animationEnd;
				int deltaY = animationEnd.y - animationBegin.y;
				unsigned long deltaT = animationEnd.time - animationBegin.time;
				int y = deltaY * ((double)(curTime - animationBegin.time) / deltaT);
				TextYPos = animationBegin.y + y;
				UpdateTextLayout();
			} else {
				UpdateScrollbar();
				int tmp = animationEnd.y; // FIXME: sidestepping a rounding issue (probably in Scrollbar)
				ScrollToY(animationEnd.y);
				TextYPos = tmp;
			}
		}
	}

	if (AnimPicture) {
		// speaker portrait
		core->GetVideoDriver()->BlitSprite(AnimPicture, drawFrame.x, drawFrame.y + EDGE_PADDING);
	}
}

void TextArea::SizeChanged(const Size& /*oldSize*/)
{
	// TODO: subview resizing should be able to be handled generically by View
	UpdateTextLayout();
}

void TextArea::SetAnimPicture(Sprite2D* pic)
{
	Control::SetAnimPicture(pic);
	UpdateTextLayout();
}

void TextArea::UpdateTextLayout()
{
	Region tf = Region(Point(EDGE_PADDING, -TextYPos), Dimensions());
	if (AnimPicture) {
		// shrink and shift the container to accommodate the image
		tf.x += AnimPicture->Width;
		tf.w -= frame.x;
	}

	// pad on both edges
	tf.w -= EDGE_PADDING * 2;

	if (textContainer) {
		textContainer->SetFrame(tf);
	}
	if (selectOptions) {
		// FIXME: implement this
		//tf.y = textContainer->Dimensions().h;
		//selectOptions->SetFrame(tf);
	}
}

void TextArea::UpdateRowCount(int h)
{
	int rowHeight = GetRowHeight();
	rows = (h + rowHeight - 1) / rowHeight; // round up
}

void TextArea::UpdateScrollbar()
{
	if (scrollbar == NULL) return;

	int textHeight = ContentHeight();
	Region nodeBounds;
	if (dialogBeginNode) {
		// possibly add some phony height to allow dialogBeginNode to the top when the scrollbar is at the bottom
		// add the height of a newline too so that there is a space
		nodeBounds = textContainer->BoundingBoxForContent(dialogBeginNode);
		Size selectFrame = selectOptions->Dimensions();
		// page = blank line + dialog node + blank line + select options
		int pageH = ftext->LineHeight + nodeBounds.h + selectFrame.h;
		if (pageH < frame.h) {
			// if the node isnt a full page by itself we need to fake it
			textHeight += frame.h - pageH;
		}
	}
	int rowHeight = GetRowHeight();
	int newRows = (textHeight + rowHeight - 1) / rowHeight; // round up
	if (newRows != rows) {
		UpdateRowCount(textHeight);
		ieWord visibleRows = (frame.h / GetRowHeight());
		ieWord sbMax = (rows > visibleRows) ? (rows - visibleRows) : 0;
		scrollbar->SetValueRange(0, sbMax);
	}

	if (flags&IE_GUI_TEXTAREA_AUTOSCROLL
		&& dialogBeginNode) {
		// now scroll dialogBeginNode to the top less a blank line
		ScrollToY(nodeBounds.y - ftext->LineHeight);
	}
}

void TextArea::SetScrollBar(ScrollBar* sb)
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

		sb->textarea = this;
		sb->SetScrollAmount(GetRowHeight());
	}

	// we need to update the ScrollBar position based around TextYPos
	UpdateScrollbar();
	if (flags&IE_GUI_TEXTAREA_AUTOSCROLL) {
		int bottom = ContentHeight() - frame.h;
		if (bottom > 0)
			ScrollToY(bottom); // no animation for this one
	} else {
		// seems silly to call ScrollToY() with the current position,
		// but it is to update the scrollbar so not a mistake
		ScrollToY(TextYPos);
	}
}

/** Sets the Actual Text */
void TextArea::SetText(const String& text)
{
	ClearText();
	AppendText(text);
}

void TextArea::SetPalette(const Color* color, PALETTE_TYPE idx)
{
	assert(idx < PALETTE_TYPE_COUNT);
	if (color) {
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = new Palette( *color, ColorBlack );
	} else if (idx > PALETTE_NORMAL) {
		// default to normal
		gamedata->FreePalette(palettes[idx]);
		palettes[idx] = palettes[PALETTE_NORMAL];
		palettes[idx]->acquire();
	}
}

void TextArea::AppendText(const String& text)
{
	if (flags&IE_GUI_TEXTAREA_HISTORY) {
		int heightLimit = (ftext->LineHeight * 100); // 100 lines of content
		// start trimming content from the top until we are under the limit.
		Size frame = textContainer->Dimensions();
		int currHeight = frame.h;
		if (currHeight > heightLimit) {
			Region exclusion(Point(), Size(frame.w, currHeight - heightLimit));
			textContainer->DeleteContentsInRect(exclusion);
		}
	}

	size_t tagPos = text.find_first_of('[');
	if (tagPos != String::npos) {
		parser.ParseMarkupStringIntoContainer(text, *textContainer);
	} else if (text.length()) {
		if (finit != ftext) {
			// append cap spans
			size_t textpos = text.find_first_not_of(WHITESPACE_STRING);
			if (textpos != String::npos) {
				// first append the white space as its own span
				textContainer->AppendText(text.substr(0, textpos));

				// we must create and append this span here (instead of using AppendText),
				// because the original data files for the DC font specifies a line height of 13
				// that would cause overlap when the lines wrap beneath the DC if we didnt specify the correct size
				Size s = finit->GetGlyph(text[textpos]).size;
				if (s.h > ftext->LineHeight) {
					// pad this only if it is "real" (it is higher than the other text).
					// some text areas have a "cap" font assigned in the CHU that differs from ftext, but isnt meant to be a cap
					// see BG2 chargen
					s.w += EDGE_PADDING;
				}
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, palettes[PALETTE_INITIALS], &s);
				textContainer->AppendContent(dc);
				textpos++;
				// FIXME: assuming we have more text!
				// FIXME: as this is currently implemented, the cap is *not* considered part of the word,
				// there is potential wrapping errors (BG2 char gen).
				// we could solve this by wrapping the cap and the letters remaining letters of the word into their own TextContainer
			} else {
				textpos = 0;
			}
			textContainer->AppendText(text.substr(textpos));
		} else {
			textContainer->AppendText(text);
		}
	}

	if (scrollbar) {
		UpdateScrollbar();
		if (flags&IE_GUI_TEXTAREA_AUTOSCROLL && !selectOptions)
		{
			// scroll to the bottom
			int bottom = ContentHeight() - frame.h;
			if (bottom > 0)
				ScrollToY(bottom, NULL, 500); // animated scroll
		}
	} else {
		UpdateRowCount(ContentHeight());
	}
	MarkDirty();
}
/*
int TextArea::InsertText(const char* text, int pos)
{
	// TODO: actually implement this
	AppendText(text);
	return pos;
}
*/
/** Key Press Event */
bool TextArea::OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/)
{
	if (flags & IE_GUI_TEXTAREA_EDITABLE) {
		if (Key.character) {
			MarkDirty();
			// TODO: implement this! currently does nothing
			size_t CurPos = 0, len = 0;
			switch (Key.keycode) {
				case GEM_HOME:
					CurPos = 0;
					break;
				case GEM_UP:
					break;
				case GEM_DOWN:
					break;
				case GEM_END:
					break;
				case GEM_LEFT:
					if (CurPos > 0) {
						CurPos--;
					} else {

					}
					break;
				case GEM_RIGHT:
					if (CurPos < len) {
						CurPos++;
					} else {

					}
					break;
				case GEM_DELETE:
					if (CurPos>=len) {
						break;
					}
					break;
				case GEM_BACKSP:
					if (CurPos != 0) {
						if (len<1) {
							break;
						}
						CurPos--;
					} else {

					}
					break;
				case GEM_RETURN:
					//add an empty line after CurLine
					// TODO: implement this
					//copy the text after the cursor into the new line

					//truncate the current line
					
					//move cursor to next line beginning
					CurPos=0;
					break;
			}

			PerformAction(Action::Change);
		}
		return true;
	}

	if (( Key.character < '1' ) || ( Key.character > '9' ))
		return false;

	MarkDirty();

	unsigned int lookupIdx = Key.character - '1';
	if (lookupIdx < OptSpans.size()) {
		UpdateState(lookupIdx);
	}
	return true;
}

int TextArea::GetRowHeight() const
{
	return ftext->LineHeight;
}

/** Will scroll y pixels. sender is the control requesting the scroll (ie the scrollbar) */
void TextArea::ScrollToY(int y, Control* sender, ieDword duration)
{
	// set up animation if required
	if  (duration) {
		// HACK: notice the values arent clamped here.
		// this is sort of a hack we allow for chapter text so it can begin and end out of sight
		unsigned long startTime = GetTickCount();
		animationBegin = AnimationPoint(TextYPos, startTime);
		animationEnd = AnimationPoint(y, startTime + duration);
		return;
	} else if (animationEnd) {
		// cancel the existing animation (if any)
		animationBegin = AnimationPoint();
		animationEnd = AnimationPoint();
	}

	if (scrollbar && sender != scrollbar) {
		scrollbar->SetValue(y);
		// sb->SetPos will recall this method so we dont need to do more... yet.
	} else if (scrollbar) {
		// our scrollbar has set position for us
		TextYPos = y;
		UpdateTextLayout();
	} else {
		// no scrollbar. need to call SetRow myself.
		// SetRow will set TextYPos.
		SetRow( y / ftext->LineHeight );
	}
}

/** Set Starting Row */
void TextArea::SetRow(int row)
{
	if (row <= rows) {
		TextYPos = row * GetRowHeight();
		UpdateTextLayout();
	}
}

/** Mousewheel scroll */
/** This method is key to touchscreen scrolling */
void TextArea::OnMouseWheelScroll(const Point& delta)
{
	// we allow scrolling to cancel the animation only if there is a scrollbar
	// otherwise it is "Chapter Text" behavior
	if (!animationEnd || scrollbar){
		unsigned long fauxY = TextYPos;
		if ((long)fauxY + delta.y <= 0) fauxY = 0;
		else fauxY += delta.y;
		ScrollToY((int)fauxY);
	}
}

/** Mouse Over Event */
void TextArea::OnMouseOver(const MouseEvent& me)
{
	if (!selectOptions)
		return;

	Point p = ConvertPointFromScreen(me.Pos());
	TextContainer* span = NULL;
	if (selectOptions) {
		Point subp = p;
		subp.x -= (AnimPicture) ? AnimPicture->Width + EDGE_PADDING : 0;
		subp.y -= textContainer->Frame().h - TextYPos;
		// container only has text, so...
		span = dynamic_cast<TextContainer*>(selectOptions->ContentAtPoint(p));
	}

	if (hoverSpan || span)
		MarkDirty();

	ClearHover();
	if (span) {
		hoverSpan = span;
		hoverSpan->SetPalette(palettes[PALETTE_HOVER]);
	}
}

/** Mouse Button Up */
void TextArea::OnMouseUp(const MouseEvent& me, unsigned short Mod)
{
	if (!hoverSpan)
		return View::OnMouseUp(me, Mod);

	if (hoverSpan) { // select the item under the mouse
		int optIdx = 0;
		std::vector<OptionSpan>::const_iterator it;
		for (it = OptSpans.begin(); it != OptSpans.end(); ++it) {
			if( it->second == hoverSpan ) {
				break;
			}
			optIdx++;
		}
		UpdateState(optIdx);
	}
}

void TextArea::OnMouseLeave(const MouseEvent& /*me*/, const DragOp*)
{
	ClearHover();
}

void TextArea::UpdateState(unsigned int optIdx)
{
	if (!VarName[0] || optIdx >= OptSpans.size()) {
		return;
	}
	if (!selectOptions) {
		// no selectable options present
		// set state to safe and return
		ClearSelectOptions();
		return;
	}

	// always run the TextAreaOnSelect handler even if the value hasnt changed
	// the *context* of the value can change (dialog) and the handler will want to know 
	SetValue( OptSpans[optIdx].first );

	// this can be called from elsewhere (GUIScript), so we need to make sure we update the selected span
	TextContainer* optspan = OptSpans[optIdx].second;
	if (selectedSpan && selectedSpan != optspan) {
		// reset the previous selection
		selectedSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		MarkDirty();
	}
	selectedSpan = optspan;
	selectedSpan->SetPalette(palettes[PALETTE_SELECTED]);

	PerformAction(Action::Select);
}

int TextArea::ContentHeight() const
{
	int cHeight = 0;
	cHeight += (textContainer) ? textContainer->Dimensions().h : 0;
	cHeight += (selectOptions) ? selectOptions->Dimensions().h : 0;
	return cHeight;
}

String TextArea::QueryText() const
{
	if (selectedSpan) {
		return selectedSpan->Text();
	} else if (OptSpans.size()) {
		String options;
		for (size_t i = 0; i < OptSpans.size(); i++) {
			options.append(OptSpans[i].second->Text());
			options.append(L"\n");
		}
		return options;
	}
	return textContainer->Text();
}

void TextArea::ClearSelectOptions()
{
	OptSpans.clear();
	delete RemoveSubview(selectOptions);
	dialogBeginNode = NULL;
	selectOptions = NULL;
	selectedSpan = NULL;
	hoverSpan = NULL;

	UpdateScrollbar();
}

void TextArea::SetSelectOptions(const std::vector<SelectOption>& opts, bool numbered,
								const Color* color, const Color* hiColor, const Color* selColor)
{
	SetPalette(color, PALETTE_OPTIONS);
	SetPalette(hiColor, PALETTE_HOVER);
	SetPalette(selColor, PALETTE_SELECTED);

	ClearSelectOptions(); // deletes previous options

	Region optFrame = textContainer->Frame();
	//optFrame.w -= (AnimPicture) ? AnimPicture->Width : 0;
	optFrame.y += optFrame.h;
	optFrame.h = 0; // will dynamically size itself

	selectOptions = new TextContainer(optFrame, ftext, palettes[PALETTE_SELECTED]);

	Size flexFrame(-1, 0); // flex frame for hanging indent after optnum
	ContentContainer::ContentList::const_reverse_iterator it = textContainer->Contents().rbegin();
	if (it != textContainer->Contents().rend()) {
		dialogBeginNode = *it; // need to get the last node *before* we append anything
		selectOptions->AppendText(L"\n"); // always want a gap between text and select options for dialog
	}
	for (size_t i = 0; i < opts.size(); i++) {
		TextContainer* selOption = new TextContainer(optFrame, ftext, palettes[PALETTE_OPTIONS]);
		if (numbered) {
			wchar_t optNum[6];
			swprintf(optNum, sizeof(optNum)/sizeof(optNum[0]), L"%d. - ", i+1);
			// TODO: as per the original PALETTE_SELECTED should be updated to the PC color (same color their name is rendered in)
			// but that should probably actually be done by the dialog handler, not here.
			selOption->AppendContent(new TextSpan(optNum, NULL, palettes[PALETTE_SELECTED]));
		}
		selOption->AppendContent(new TextSpan(opts[i].second, NULL, NULL, &flexFrame));

		OptSpans.push_back(std::make_pair(opts[i].first, selOption));

		//selectOptions->AppendContent(selOption); // container owns the option
		if (EventMgr::TouchInputEnabled) {
			// now add a newline for keeping the options spaced out (for touch screens)
			selectOptions->AppendText(L"\n");
		}
	}
	assert(textContainer);

	AddSubviewInFrontOfView(selectOptions);
	UpdateScrollbar();
	MarkDirty();
}

void TextArea::ClearHover()
{
	if (hoverSpan) {
		if (hoverSpan == selectedSpan) {
			hoverSpan->SetPalette(palettes[PALETTE_SELECTED]);
		} else {
			// reset the old hover span
			hoverSpan->SetPalette(palettes[PALETTE_OPTIONS]);
		}
		hoverSpan = NULL;
	}
}

void TextArea::ClearText()
{
	ClearHover();
	delete RemoveSubview(textContainer);

	parser.Reset(); // reset in case any tags were left open from before
	textContainer = new TextContainer(Region(Point(), Size(frame.w, 0)), ftext, palette);
	AddSubviewInFrontOfView(textContainer);

	UpdateTextLayout();
	ScrollToY(0); // reset text position to top
	UpdateScrollbar();
}

void TextArea::SetFocus()
{
	Control::SetFocus();
	if (IsFocused() && flags & IE_GUI_TEXTAREA_EDITABLE) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

}
