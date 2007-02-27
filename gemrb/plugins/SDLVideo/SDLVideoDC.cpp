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
 * $Id$
 *
 */

#include "SDLVideoDC.h"
#include "SDLVideoDriver.h"

SDLVideoDC::SDLVideoDC(void)
{
}

SDLVideoDC::~SDLVideoDC(void)
{
}

void* SDLVideoDC::Create(void)
{
	return new SDLVideoDriver();
}

const char* SDLVideoDC::ClassName(void)
{
	return "SDLVideo";
}

SClass_ID SDLVideoDC::SuperClassID(void)
{
	return IE_VIDEO_CLASS_ID;
}

Class_ID SDLVideoDC::ClassID(void)
{
	return Class_ID( 0x67402f81, 0x4b9217d6 );
}
