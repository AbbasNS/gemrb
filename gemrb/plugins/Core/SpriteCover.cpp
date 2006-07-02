/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SpriteCover.cpp,v 1.2 2006/07/02 11:46:58 wjpalenstijn Exp $
 *
 */

#include "SpriteCover.h"
#include "Interface.h"
#include "Video.h"

SpriteCover::SpriteCover()
{
	pixels = 0;
}

SpriteCover::~SpriteCover()
{
	core->GetVideoDriver()->DestroySpriteCover(this);
}

bool SpriteCover::Covers(int x, int y, int xpos, int ypos,
						 int width, int height)
{
	// if basepoint changed, no longer valid
	if (x != worldx || y != worldy) return false;

	// top-left not covered
	if (xpos > XPos || ypos > YPos) return false;

	// bottom-right not covered
	if (width-xpos > Width-XPos || height-ypos > Height-YPos) return false;

	return true;
}
