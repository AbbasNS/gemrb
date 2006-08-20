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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/PNGImporter/PNGImp.h,v 1.2 2006/08/20 20:19:06 wjpalenstijn Exp $
 *
 */

#ifndef PNGIMP_H
#define PNGIMP_H

#include "../Core/ImageMgr.h"

struct PNGInternal;

class PNGImp : public ImageMgr {
private:
	DataStream* str;
	bool autoFree;
	PNGInternal* inf;

	ieDword Width, Height;
	bool hasPalette;
public:
	PNGImp(void);
	~PNGImp(void);
	void Close();
	bool Open(DataStream* stream, bool autoFree = true, bool convert = false);
	bool OpenFromImage(Sprite2D* sprite, bool autoFree = true);
	Sprite2D* GetImage();
	void PutImage(DataStream *output);
	/** No descriptions */
	void GetPalette(int index, int colors, Color* pal);
	/** Searchmap only */
	void SetPixelIndex(unsigned int x, unsigned int y, unsigned int /*idx*/)
	{
		if(x>=Width || y>=Height) {
			return;
		}
    //add png code here
	}
	unsigned int GetPixelIndex(unsigned int x, unsigned int y)
	{
		if(x>=Width || y>=Height) {
			return 0;
		}
    //add png code here
		return 0;
	}
	/** Gets a Pixel from the Image */
	Color GetPixel(unsigned int x, unsigned int y)
	{
		Color ret = {0,0,0,0};

		if(x>=Width || y>=Height) {
			return ret;
		}
    //add png code here (this part may be optional)
		return ret;
	}
	int GetWidth() { return (int) Width; }
	int GetHeight() { return (int) Height; }
public:
	void release(void)
	{
		delete this;
	}
	int GetCycleCount()
	{
		return 1;
	}
};

#endif
