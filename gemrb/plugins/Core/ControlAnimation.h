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

#ifndef CONTROLANIMATIONS_H
#define CONTROLANIMATIONS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>

#include "../../includes/RGBAColor.h"
#include "AnimationFactory.h"
#include "Control.h"
#include "Sprite2D.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif


class GEM_EXPORT ControlAnimation {
private:
	AnimationFactory* bam;
	Control* control;
	unsigned int cycle;
	unsigned int frame;
	unsigned int anim_phase;
public:
	ControlAnimation(Control* ctl, const ieResRef ResRef, int Cycle = 0);
	~ControlAnimation(void);
	void UpdateAnimation();
	//report if the current resource is the same as descripted by the params
	bool SameResource(const ieResRef ResRef, int Cycle);
};

#endif
