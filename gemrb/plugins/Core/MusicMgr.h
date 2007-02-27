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

#ifndef MUSICMGR_H
#define MUSICMGR_H

#include "Plugin.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT MusicMgr : public Plugin {
public: 
	MusicMgr();
	virtual ~MusicMgr();
	/** Ends the Current PlayList Execution */
	virtual void End(void) = 0;
	virtual void HardEnd(void) = 0;
	/** Start the PlayList Music Execution */
	virtual void Start(void) = 0;
	/** Initializes the PlayList Manager */
	virtual bool Init();
	/** Loads a PlayList for playing */
	virtual bool OpenPlaylist(const char* name) = 0;
	/** Switches the current PlayList while playing the current one */
	virtual void SwitchPlayList(const char* name, bool Hard) = 0;
	/** Plays the Next Entry */
	virtual void PlayNext() = 0;
};

#endif
