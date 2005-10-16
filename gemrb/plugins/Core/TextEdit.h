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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TextEdit.h,v 1.14 2005/10/16 21:36:46 edheldil Exp $
 *
 */

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "Control.h"
#include "../../includes/RGBAColor.h"
#include "Font.h"

// !!! Keep these synchronized with GUIDefines.py
#define IE_GUI_EDIT_ON_CHANGE      0x03000000
#define IE_GUI_EDIT_ON_DONE        0x03000001
#define IE_GUI_EDIT_ON_CANCEL      0x03000002

//this is stored in 'Value' of Control class
#define IE_GUI_EDIT_NUMBER         1

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TextEdit : public Control {
public:
	TextEdit(unsigned short maxLength);
	~TextEdit(void);
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Set Font */
	void SetFont(Font* f);
	/** Set Cursor */
	void SetCursor(Sprite2D* cur);
	/** Set BackGround */
	void SetBackGround(Sprite2D* back);
	/** Sets the Text of the current control */
	int SetText(const char* string, int pos = 0);
	/** Sets the Text of the current control */
	const char* QueryText();
	/** Sets the buffer length */
	void SetBufferLength(int buflen);
private:
	/** Text Editing Cursor Sprite */
	Sprite2D* Cursor;
	/** Text Font */
	Font* font;
	/** Background */
	Sprite2D* Back;
	/** Max Edit Text Length */
	unsigned short max;
	/** Text Buffer */
	unsigned char* Buffer;
	/** Cursor Position */
	unsigned short CurPos;
	/** Color Palette */
	Color* palette;
public: //Events
	/** Key Press Event */
	void OnKeyPress(unsigned char Key, unsigned short Mod);
	/** Special Key Press */
	void OnSpecialKeyPress(unsigned char Key);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);
	/** OnChange Scripted Event Function Name */
	EventHandler EditOnChange;
	EventHandler EditOnDone;
	EventHandler EditOnCancel;
};

#endif
