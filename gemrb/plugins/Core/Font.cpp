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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Font.cpp,v 1.49 2006/08/07 22:25:09 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Font.h"
#include "Interface.h"
#include "Video.h"
#include "Palette.h"
#include <cassert>

unsigned int lastX = 0;

#define PARAGRAPH_START_X 5;

inline size_t mystrlen(const char* string)
{
	if (!string) {
		return ( size_t ) 0;
	}
	const char* tmp = string;
	size_t count = 0;
	while (*tmp != 0) {
		if (( ( unsigned char ) * tmp ) >= 0xf0) {
			tmp += 3;
			count += 3;
		}
		count++;
		tmp++;
	}
	return count;
}

Font::Font(int w, int h, Palette* pal, bool cK, int index)
{
	lastX = 0;
	count = 0;
	FirstChar = 0;
	void* pixels = malloc( w* h );
	memset( xPos, 0, sizeof( xPos) );
	memset( yPos, 0, sizeof( yPos) );
	sprBuffer = core->GetVideoDriver()->CreateSprite8( w, h, 8, pixels, pal ? pal->col : 0, cK, index );
	pal->IncRef();
	palette = pal;
	maxHeight = h;
}

Font::~Font(void)
{
	Video *video = core->GetVideoDriver();
	core->FreePalette( palette );
	video->FreeSprite( sprBuffer );
}

void Font::AddChar(void* spr, int w, int h, short xPos, short yPos)
{
	if (!spr) {
		size[count].x = 0;
		size[count].y = 0;
		size[count].w = 0;
		size[count].h = 0;
		this->xPos[count] = 0;
		this->yPos[count] = 0;
		count++;
		return;
	}
	unsigned char * startPtr = ( unsigned char * ) sprBuffer->pixels;
	unsigned char * currPtr;
	unsigned char * srcPtr = ( unsigned char * ) spr;
	for (int y = 0; y < h; y++) {
		currPtr = startPtr + ( y * sprBuffer->Width ) + lastX;
		memcpy( currPtr, srcPtr, w );
		srcPtr += w;
	}
	size[count].x = lastX;
	size[count].y = 0;
	size[count].w = w;
	size[count].h = h;
	this->xPos[count] = xPos;
	this->yPos[count] = yPos;
	count++;
	lastX += w;
}

void Font::PrintFromLine(int startrow, Region rgn, const unsigned char* string,
	Palette* hicolor, unsigned char Alignment, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor)
{
	unsigned int psx = PARAGRAPH_START_X;
	Palette *pal = hicolor;
	if (!pal) {
		pal = palette;
	}
	if (startrow || (initials==this) ) {
		initials = NULL;
	}

	Video* video = core->GetVideoDriver();
	video->SetPalette( sprBuffer, pal );
	size_t len = strlen( ( char* ) string );
	char* tmp = ( char* ) malloc( len + 1 );
	memcpy( tmp, ( char * ) string, len + 1 );
	SetupString( tmp, rgn.w, NoColor );
	int ystep = 0;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		for (size_t i = 0; i < len; i++) {
			int height = yPos[( unsigned char ) tmp[i] - 1];
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = size[1].h;
	}
	if (!ystep) ystep = maxHeight;
	int x = psx, y = ystep;
	int w = CalcStringWidth( tmp, NoColor );
	if (Alignment & IE_FONT_ALIGN_CENTER) {
		x = ( rgn.w - w) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		x = ( rgn.w - w );
	}
	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 1;
		for (size_t i = 0; i <= len; i++) {
			if (( tmp[i] == 0 ) || ( tmp[i] == '\n' ))
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h );
	}
	int row = 0;
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) tmp[i] ) == '[' && !NoColor) {
			i++;
			char tag[256];
			int k = 0;
			for (k = 0; k < 256; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}
			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				Color c = {r,g, b, 0}, back = {0, 0, 0, 0};
				Palette* newPal = core->CreatePalette( c, back );
				video->SetPalette( sprBuffer, newPal );
				core->FreePalette( newPal );
			} else {
				if (stricmp( tag, "/color" ) == 0) {
					video->SetPalette( sprBuffer, pal );
				} else {
					if (stricmp( "p", tag ) == 0) {
						psx = x;
					} else {
						if (stricmp( "/p", tag ) == 0) {
							psx = PARAGRAPH_START_X;
						}
					}
				}
			}
			continue;
		}
		if (row < startrow) {
			if (tmp[i] == 0) {
				row++;
			}
			continue;
		}
		if (( tmp[i] == 0 ) || ( tmp[i] == '\n' )) {
			y += ystep;
			x = psx;
			int w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		unsigned char currChar = ( unsigned char ) tmp[i] - 1;
		if (initials) {
			x = initials->PrintInitial( x, y, rgn, currChar );
			initials = NULL;
			continue;
		}
		video->BlitSpriteRegion( sprBuffer, size[currChar],
			x + rgn.x, y + rgn.y - yPos[currChar], true, &rgn );
		if (cursor && ( i == curpos )) {
			video->BlitSprite( cursor, x + rgn.x,
				y + rgn.y, true, &rgn );
		}
		x += size[currChar].w;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x,
			y + rgn.y, true, &rgn );
	}
	free( tmp );
}

void Font::Print(Region rgn, const unsigned char* string, Palette* hicolor,
	unsigned char Alignment, bool anchor, Font* initials,
	Sprite2D* cursor, unsigned int curpos, bool NoColor)
{
	unsigned int psx = PARAGRAPH_START_X;
	Palette* pal = hicolor;
	if (!pal) {
		pal = palette;
	}
	if (initials==this) {
		initials = NULL;
	}

	Video* video = core->GetVideoDriver();
	video->SetPalette( sprBuffer, pal );
	size_t len = strlen( ( char* ) string );
	char* tmp = ( char* ) malloc( len + 1 );
	memcpy( tmp, ( char * ) string, len + 1 );
	SetupString( tmp, rgn.w, NoColor );
	int ystep = 0;
	if (Alignment & IE_FONT_SINGLE_LINE) {
		
		for (size_t i = 0; i < len; i++) {
			if (tmp[i] == 0) continue;
			int height = yPos[( unsigned char ) tmp[i] - 1];
			if (ystep < height)
				ystep = height;
		}
	} else {
		ystep = size[1].h;
	}
	if (!ystep) ystep = maxHeight;
	int x = psx, y = ystep;
	if (Alignment & IE_FONT_ALIGN_CENTER) {
		int w = CalcStringWidth( tmp, NoColor );
		x = ( rgn.w - w ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
		int w = CalcStringWidth( tmp, NoColor );
		x = ( rgn.w - w );
	}

	if (Alignment & IE_FONT_ALIGN_MIDDLE) {
		int h = 0;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h ) / 2;
	} else if (Alignment & IE_FONT_ALIGN_BOTTOM) {
		int h = 1;
		for (size_t i = 0; i <= len; i++) {
			if (tmp[i] == 0)
				h++;
		}
		h = h * ystep;
		y += ( rgn.h - h );
	} else if (Alignment & IE_FONT_ALIGN_TOP) {
		y += 5;
	}
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) tmp[i] ) == '[' && !NoColor) {
			i++;
			if (i>=len)
				break;
			char tag[256];
			for (int k = 0; k < 256; k++) {
				if (tmp[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = tmp[i++];
			}
			if (strnicmp( tag, "color=", 6 ) == 0) {
				unsigned int r,g,b;
				if (sscanf( tag, "color=%02X%02X%02X", &r, &g, &b ) != 3)
					continue;
				Color c = {r,g, b, 0}, back = {0, 0, 0, 0};
				Palette* newPal = core->CreatePalette( c, back );
				video->SetPalette( sprBuffer, newPal );
				core->FreePalette( newPal );
			} else {
				if (stricmp( tag, "/color" ) == 0) {
					video->SetPalette( sprBuffer, pal );
				} else {
					if (stricmp( "p", tag ) == 0) {
						psx = x;
					} else {
						if (stricmp( "/p", tag ) == 0) {
							psx = PARAGRAPH_START_X;
						}
					}
				}
			}
			continue;
		}
		if (tmp[i] == 0) {
			y += ystep;
			x = psx;
			int w = CalcStringWidth( &tmp[i + 1], NoColor );
			if (Alignment & IE_FONT_ALIGN_CENTER) {
				x = ( rgn.w - w ) / 2;
			} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
				x = ( rgn.w - w );
			}
			continue;
		}
		unsigned char currChar = ( unsigned char ) tmp[i] - 1;
		if (initials) {
			x = initials->PrintInitial( x, y, rgn, currChar );
			initials = NULL;
			continue;
		}
		video->BlitSpriteRegion( sprBuffer, size[currChar],
			x + rgn.x, y + rgn.y - yPos[currChar],
			anchor, &rgn );
		if (cursor && ( curpos == i ))
			video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &rgn );
		x += size[currChar].w;
	}
	if (cursor && ( curpos == len )) {
		video->BlitSprite( cursor, x + rgn.x, y + rgn.y, anchor, &rgn );
	}
	free( tmp );
}

int Font::PrintInitial(int x, int y, Region &rgn, unsigned char currChar)
{
	Video *video = core->GetVideoDriver();
	video->BlitSpriteRegion( sprBuffer, size[currChar],
		x + rgn.x, y + rgn.y - yPos[currChar], true, &rgn );
	x += size[currChar].w;
	return x;
}

int Font::CalcStringWidth(const char* string, bool NoColor)
{
	size_t ret = 0, len = strlen( string );
	for (size_t i = 0; i < len; i++) {
		if (( ( unsigned char ) string[i] ) == '[' && !NoColor) {
			i++;
			if (i>=len)
				break;
			char tag[256];
			int k = 0;
			for (k = 0; k < 256; k++) {
				if (string[i] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = string[i++];
			}
			continue;
		}
		ret += size[( unsigned char ) string[i] - 1].w;
	}
	return ( int ) ret;
}

void Font::SetupString(char* string, unsigned int width, bool NoColor)
{
	size_t len = strlen( string );
	unsigned int psx = PARAGRAPH_START_X;
	int lastpos = 0;
	unsigned int x = psx, wx = 0;
	bool endword = false;
	for (size_t pos = 0; pos < len; pos++) {
		if (x + wx > width) {
			if (!endword && ( x == psx ))
				lastpos = ( int ) pos;
			else
				string[lastpos] = 0;
			x = psx;
		}
		if (string[pos] == 0) {
			continue;
		}
		endword = false;
		if (string[pos] == '\r')
			string[pos] = ' ';
		if (string[pos] == '\n') {
			string[pos] = 0;
			x = psx;
			wx = 0;
			lastpos = ( int ) pos;
			endword = true;
			continue;
		}
		if (( ( unsigned char ) string[pos] ) == '[' && !NoColor) {
			pos++;
			if (pos>=len)
				break;
			char tag[256];
			int k = 0;
			for (k = 0; k < 256; k++) {
				if (string[pos] == ']') {
					tag[k] = 0;
					break;
				}
				tag[k] = string[pos++];
			}
			if (stricmp( "p", tag ) == 0) {
				psx = x;
			} else {
				if (stricmp( "/p", tag ) == 0) {
					psx = PARAGRAPH_START_X;
				}
			}
			continue;
		}

		if (string[pos] && string[pos] != ' ') 
			string[pos] -= FirstChar;

		wx += size[( unsigned char ) string[pos] - 1].w;
		if (( string[pos] == ' ' ) || ( string[pos] == '-' )) {
			x += wx;
			wx = 0;
			lastpos = ( int ) pos;
			endword = true;
		}
	}
}

Palette* Font::GetPalette()
{
	assert(palette);
	palette->IncRef();
	return palette;
}

void Font::SetPalette(Palette* pal)
{
	if (palette) palette->Release();
	pal->IncRef();
	palette = pal;
}

void Font::SetFirstChar( unsigned char first)
{
	FirstChar = first;
}
