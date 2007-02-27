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

#include "WMPImpCD.h"
#include "WMPImp.h"

WMPImpCD::WMPImpCD(void)
{
}

WMPImpCD::~WMPImpCD(void)
{
}

void* WMPImpCD::Create(void)
{
	return new WMPImp();
}

const char* WMPImpCD::ClassName(void)
{
	return "WMPImp";
}

SClass_ID WMPImpCD::SuperClassID(void)
{
	// FIXME?????
	return IE_WMP_CLASS_ID;
}

Class_ID WMPImpCD::ClassID(void)
{
	// FIXME?????
	return Class_ID( 0x088a4123, 0xbfc417de );
}
