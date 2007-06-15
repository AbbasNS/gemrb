/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005-2006 The GemRB Project
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
 * $Id$
 */

#ifndef PALETTE_H
#define PALETTE_H

#include <cassert>

#include "../../includes/RGBAColor.h"
#include "../../includes/ie_types.h"


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

enum PaletteType {
	PAL_MAIN,
	PAL_WEAPON,
	PAL_OFFHAND,
	PAL_HELMET
};

struct RGBModifier {
	Color rgb;
	int speed;
	int phase;
	enum Type {
		NONE,
		ADD,
		TINT,
		BRIGHTEN
	} type;
};


class GEM_EXPORT Palette {
public:
	Palette(Color* colours, bool alpha_=false) {
		for (int i = 0; i < 256; ++i) {
			col[i] = colours[i];
		}
		alpha = alpha_;
		refcount = 1;
		named = false;
	}
	Palette() {
		alpha = false;
		refcount = 1;
		named = false;
	}
	~Palette() { }

	Color col[256]; //< RGB or RGBA 8 bit palette
	bool alpha; //< true if this is a RGBA palette
	bool named; //< true if the palette comes from a bmp and cached

	void IncRef() {
		refcount++;
	}

	void Release() {
		assert(refcount > 0);
		if (!--refcount)
			delete this;
	}

	bool IsShared() const {
		return (refcount > 1);
	}

	void CreateShadedAlphaChannel();

	void SetupPaperdollColours(const ieDword* Colors, unsigned int type);
	void SetupRGBModification(const Palette* src, const RGBModifier* mods,
		unsigned int type);
	void SetupGlobalRGBModification(const Palette* src,
		const RGBModifier& mod);

	Palette* Copy();

private:
	unsigned int refcount;

};

#endif
