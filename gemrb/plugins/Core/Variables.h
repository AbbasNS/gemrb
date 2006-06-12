/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 |Avenger|
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Variables.h,v 1.24 2006/06/12 18:05:32 avenger_teambg Exp $
 *
 */

#if !defined(VARIABLES_H)
#define VARIABLES_H

#include <ctype.h>
#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../../includes/SClassID.h"

/////////////////////////////////////////////////////////////////////////////
// Variables<ieDword, VALUE>

#ifndef ReleaseFun
typedef void (*ReleaseFun)(void *);
#endif

#define GEM_VARIABLES_INT      0
#define GEM_VARIABLES_STRING   1

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Variables {
protected:
	// Association
	struct MyAssoc {
		MyAssoc* pNext;
		char* key;
		union {
			ieDword nValue;
			char* sValue;
		} Value;
		unsigned long nHashValue;
	};
	struct MemBlock {
		MemBlock* pNext;
	};
public:
	// Construction
	Variables(int nBlockSize = 10, int nHashTableSize = 2049);

	// Attributes
	//sets the way we handle keys, no parsing for .ini file entries, parsing for game variables
	//you should set this only on an empty mapping
	inline int ParseKey(int arg)
	{
		MYASSERT( m_nCount == 0 );
		m_lParseKey = ( arg > 0 );
		return 0;
	}
	//sets the way we handle values
	inline void SetType(int type)
	{
		m_type = type;
	}
	inline int GetCount() const
	{
		return m_nCount;
	}
	inline bool IsEmpty() const
	{
		return m_nCount == 0;
	}

	// Lookup
	int GetValueLength(const char* key) const;
	bool Lookup(const char* key, char* dest, int MaxLength) const;
	bool Lookup(const char* key, ieDword& rValue) const;
	bool Lookup(const char* key, char*& dest) const;

	// Operations
	void SetAtCopy(const char* key, const char* newValue);
	void SetAt(const char* key, const char* newValue);
	void SetAt(const char* key, ieDword newValue);
	void RemoveAll(ReleaseFun fun);
	void InitHashTable(unsigned int hashSize, bool bAllocNow = true);

	POSITION GetNextAssoc(POSITION rNextPosition, const char*& rKey,
		ieDword& rValue) const;
	// Implementation
protected:
	Variables::MyAssoc** m_pHashTable;
	unsigned int m_nHashTableSize;
	bool m_lParseKey;
	int m_nCount;
	Variables::MyAssoc* m_pFreeList;
	MemBlock* m_pBlocks;
	int m_nBlockSize;
	int m_type; //could be string or ieDword 

	Variables::MyAssoc* NewAssoc(const char* key);
	void FreeAssoc(Variables::MyAssoc*);
	Variables::MyAssoc* GetAssocAt(const char*, unsigned int&) const;
	inline bool MyCopyKey(char*& dest, const char* key) const;
	inline unsigned int MyHashKey(const char*) const;

public:
	~Variables();
};

#endif // !defined(VARIABLES_H)
