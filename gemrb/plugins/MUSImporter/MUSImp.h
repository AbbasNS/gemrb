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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MUSImporter/MUSImp.h,v 1.11 2004/01/02 15:51:14 balrog994 Exp $
 *
 */

#ifndef MUSIMP_H
#define MUSIMP_H

#include "../Core/MusicMgr.h"
#include "../Core/FileStream.h"
#include <stdio.h>

/**MUS PlayList Importer
  *@author GemRB Developement Team
  */

typedef struct PLString {
	char PLFile[10];
	char PLLoop[10];
	char PLTag[10];
	char PLEnd[10];
	unsigned long soundID;
} PLString;
  
class MUSImp : public MusicMgr
{
private:
	bool Initialized;
	bool Playing;
	char PLName[32];
	int PLpos, PLnext;
	FileStream * str;
	std::vector<PLString> playlist;
	unsigned long lastSound;
private:
	void PlayMusic(int pos);
	void PlayMusic(char *name);
public: 
	MUSImp();
	~MUSImp();
	/** Loads a PlayList for playing */
	bool OpenPlaylist(const char * name);
	/** Initializes the PlayList Manager */
	bool Init();
	/** Switches the current PlayList while playing the current one */
	void SwitchPlayList(const char * name, bool Hard);
	/** Ends the Current PlayList Execution */
	void End();
	void HardEnd();
	/** Start the PlayList Music Execution */
	void Start();
	/** Plays the Next Entry */
	void PlayNext();
public:
	void release(void)
	{
		delete this;
	}
};

#endif
