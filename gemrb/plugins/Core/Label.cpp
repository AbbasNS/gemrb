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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Label.cpp,v 1.33 2005/11/12 19:31:51 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Label.h"
#include "Interface.h"

Label::Label(Font* font)
{
	this->font = font;
	Buffer = NULL;
	useRGB = false;
	ResetEventHandler( LabelOnPress );

	Alignment = IE_FONT_ALIGN_CENTER;
	palette = NULL;
}
Label::~Label()
{
	core->GetVideoDriver()->FreePalette( palette );
	if (Buffer) {
		free( Buffer );
	}
}
/** Draws the Control on the Output Display */
void Label::Draw(unsigned short x, unsigned short y)
{
	if (!Changed && !(((Window*)Owner)->Flags&WF_FLOAT)) {
		return;
	}
	Changed = false;
	if (XPos == 65535) {
		return;
	}
	if (font && Buffer) {
		font->Print( Region( this->XPos + x, this->YPos + y,
			this->Width, this->Height ), ( unsigned char * ) Buffer,
			useRGB?palette:NULL, Alignment | IE_FONT_ALIGN_MIDDLE |
			IE_FONT_SINGLE_LINE, true );
	}
}
/** This function sets the actual Label Text */
int Label::SetText(const char* string, int /*pos*/)
{
	if (Buffer )
		free( Buffer );
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			Buffer = (char *) malloc( 64 );
			strnlwrcpy( Buffer, string, 63 );
		}
		else {
			Buffer = strdup( string );
		}
	}
	else {
		Buffer = strdup( string );
	}
	if (!palette) {
		Color white = {0xff, 0xff, 0xff, 0x00}, black = {0x00, 0x00, 0x00, 0x00};
		SetColor(white, black);
	}
	if (Owner) {
		( ( Window * ) Owner )->Invalidate();
	}
	return 0;
}
/** Sets the Foreground Font Color */
void Label::SetColor(Color col, Color bac)
{
	core->GetVideoDriver()->FreePalette( palette );
	palette = core->GetVideoDriver()->CreatePalette( col, bac );
	Changed = true;
}

void Label::SetAlignment(unsigned char Alignment)
{
	if (Alignment > IE_FONT_ALIGN_RIGHT) {
		return;
	}
	this->Alignment = Alignment;
	if (Alignment == IE_FONT_ALIGN_CENTER) {
		if (core->HasFeature( GF_LOWER_LABEL_TEXT )) {
			strlwr( Buffer );
		}
	}
	Changed = true;
}

void Label::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char /*Button*/, unsigned short /*Mod*/)
{
	printf( "Label::OnMouseUp\n" );
	if (( x <= Width ) && ( y <= Height )) {
		if (VarName[0] != 0) {
			core->GetDictionary()->SetAt( VarName, Value );
		}
		if(LabelOnPress[0]) {
			RunEventHandler( LabelOnPress );
		}
	}
}

bool Label::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_LABEL_ON_PRESS:
		SetEventHandler( LabelOnPress, handler );
		break;
	default:
		return Control::SetEvent( eventType, handler );
	}

	return true;
}

/** Simply returns the pointer to the text, don't modify it! */
const char* Label::QueryText()
{
	return ( const char * ) Buffer;
}
