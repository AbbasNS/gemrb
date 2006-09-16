/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ProjectileMgr.h,v 1.1 2006/09/16 13:30:15 avenger_teambg Exp $
 *
 */

#ifndef PROJECTILEMGR_H
#define PROJECTILEMGR_H

#include "Plugin.h"
#include "Projectile.h"
#include "DataStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ProjectileMgr : public Plugin {
public:
	ProjectileMgr(void);
	virtual ~ProjectileMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual Projectile* GetProjectile( Projectile *) = 0;
};

#endif
