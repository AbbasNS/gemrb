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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/StoreMgr.h,v 1.5 2005/11/08 22:59:05 edheldil Exp $
 *
 */

/**
 * @file StoreMgr.h
 * Declares StoreMgr class, loader for Store objects
 * @author The GemRB Project
 */


#ifndef STOREMGR_H
#define STOREMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Store.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @class StoreMgr
 * Abstract loader for Store objects
 */

class GEM_EXPORT StoreMgr : public Plugin {
public:
	StoreMgr(void);
	virtual ~StoreMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual Store* GetStore(Store *s) = 0;

	virtual int GetStoredFileSize(Store *s) = 0;
	virtual int PutStore(DataStream* stream, Store *s) = 0;
};

#endif
