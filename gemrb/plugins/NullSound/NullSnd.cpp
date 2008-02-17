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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "NullSnd.h"
#include "../Core/AmbientMgr.h"

NullSnd::NullSnd(void)
{
	XPos = 0;
	YPos = 0;
	ambim = new AmbientMgr();
}

NullSnd::~NullSnd(void)
{
	delete ambim;
}

bool NullSnd::Init(void)
{
	return true;
}

unsigned int NullSnd::Play(const char*, int, int, unsigned int)
{
	return 1000; //Returning 1 Second Length
}

int NullSnd::StreamFile(const char*)
{
	return 0;
}

bool NullSnd::Stop()
{
	return true;
}

bool NullSnd::Play()
{
	return true;
}

void NullSnd::ResetMusics()
{
}

bool NullSnd::CanPlay()
{
	return false;
}

bool NullSnd::IsSpeaking()
{
	return false;
}

void NullSnd::UpdateListenerPos(int x, int y)
{
	XPos = x;
	YPos = y;
}

void NullSnd::GetListenerPos(int& x, int& y)
{
	x = XPos;
	y = YPos;
}

int NullSnd::SetupNewStream(ieWord, ieWord, ieWord, ieWord, bool, bool)
{
	return -1;
}

int NullSnd::QueueAmbient(int, const char*)
{
	return -1;
}

bool NullSnd::ReleaseStream(int, bool)
{
	return true;
}

void NullSnd::SetAmbientStreamVolume(int, int)
{

}

void NullSnd::QueueBuffer(int, unsigned short, int, short*, int, int)
{

}
