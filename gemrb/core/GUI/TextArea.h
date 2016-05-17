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

#ifndef TEXTAREA_H
#define TEXTAREA_H

#include "GUI/Control.h"
#include "GUI/ScrollBar.h"
#include "GUI/TextSystem/Font.h"
#include "GUI/TextSystem/GemMarkup.h"
#include "GUI/TextSystem/TextContainer.h"

#include "RGBAColor.h"
#include "exports.h"

#include <vector>

namespace GemRB {

// Keep these synchronized with GUIDefines.py
// 0x05 is the control type of TextArea
#define IE_GUI_TEXTAREA_ON_CHANGE   0x05000000 // text change event (keyboard, etc)
#define IE_GUI_TEXTAREA_ON_SELECT	0x05000001 // selection event such as dialog or a listbox

// TextArea flags, keep these in sync too
// the control type is intentionally left out
#define IE_GUI_TEXTAREA_AUTOSCROLL   1
#define IE_GUI_TEXTAREA_HISTORY      2	// message window
#define IE_GUI_TEXTAREA_EDITABLE     4

typedef std::pair<int, String> SelectOption;

/**
 * @class TextArea
 * Widget capable of displaying long paragraphs of text.
 * It is usually scrolled with a ScrollBar widget
 */

class GEM_EXPORT TextArea : public Control {
protected:
	/** Draws the Control on the Output Display */
	void DrawSelf(Region drawFrame, const Region& clip);
	void SizeChanged(const Size& /*oldSize*/);

public:
	TextArea(const Region& frame, Font* text);
	TextArea(const Region& frame, Font* text, Font* caps,
			 Color hitextcolor, Color initcolor, Color lowtextcolor);
	~TextArea(void);

	bool IsAnimated() const { return animationEnd; }
	bool IsOpaque() const { return false; }

	/** Sets the Actual Text */
	void SetText(const String& text);
	/** Clears the textarea */
	void ClearText();
	void ClearHover();
	/** Appends a String to the current Text */
	void AppendText(const String& text);
	/** Inserts a String into the current Text at pos */
	// int InsertText(const char* text, int pos);

	/** Per Pixel scrolling */
	void ScrollToY(int y, Control* sender = NULL, ieDword duration = 0);

	/** Returns total height of the text */
	int GetRowHeight() const;
	/** Set Starting Row */
	void SetRow(int row);
	int RowCount() { return rows; }
	void SetSelectOptions(const std::vector<SelectOption>&, bool numbered,
						  const Color* color, const Color* hiColor, const Color* selColor);
	/** Set Selectable */
	void SetSelectable(bool val);
	void SetAnimPicture(Sprite2D* Picture);

	/** Returns the selected text */
	String QueryText() const;
	/** Marks textarea for redraw with a new value */
	void UpdateState(unsigned int optIdx);
	void SetScrollBar(ScrollBar* sb);

private: // Private attributes
	// dialog and listbox handling
	typedef std::pair<int, TextContainer*> OptionSpan;
	std::vector<OptionSpan> OptSpans;
	TextContainer* hoverSpan, *selectedSpan;
	const Content* dialogBeginNode;
	// dialog options container
	TextContainer* selectOptions;
	// standard text display container
	TextContainer* textContainer;
	ScrollBar* scrollbar; // temporary; will be superceeded by ScrollView

	struct AnimationPoint {
		// TODO: we cant currently scroll the x axis
		// if that happens we should upgrade this to Point
		int y;
		unsigned long time;

		AnimationPoint() : y(0), time(0) {}
		AnimationPoint(int y, unsigned long t) : y(y), time(t) {}

		operator bool() const {
			return (time > 0);
		}
	};
	AnimationPoint animationBegin, animationEnd;

	int TextYPos;
	int rows;

	/** Fonts */
	Font* finit, * ftext;
	GemMarkupParser parser;

	/** OnChange Scripted Event Function Name */
	ControlEventHandler TextAreaOnChange;
	ControlEventHandler TextAreaOnSelect;

	enum PALETTE_TYPE {
		PALETTE_NORMAL = 0,	// standard text color
		PALETTE_OPTIONS,	// normal palette for selectable options (dialog/listbox)
		PALETTE_HOVER,		// palette for hovering options (dialog/listbox)
		PALETTE_SELECTED,	// selected list box/dialog option.
		PALETTE_INITIALS,	// palette for finit. used only is some cases.

		PALETTE_TYPE_COUNT
	};
	Palette* palettes[PALETTE_TYPE_COUNT];
	Palette* palette; // shortcut for palettes[PALETTE_NORMAL]

private: //internal functions
	void Init();
	void SetPalette(const Color*, PALETTE_TYPE);

	void UpdateRowCount(int h);
	void UpdateScrollbar();
	void UpdateTextLayout();

	int ContentHeight() const;

public: //Events
	/** Key Press Event */
	bool OnKeyPress(const KeyboardEvent& Key, unsigned short Mod);
	/** Mousewheel scroll */
	void OnMouseWheelScroll(short x, short y);
	/** Mouse Over Event */
	void OnMouseOver(const Point&);
	/** Mouse Button Up */
	void OnMouseUp(const Point&, unsigned short Button, unsigned short Mod);
	void OnMouseLeave(const Point&, const DragOp*);
	/** Set handler for specified event */
	bool SetEvent(int eventType, ControlEventHandler handler);
	void SetFocus();

	void ClearSelectOptions();
};

}

#endif
