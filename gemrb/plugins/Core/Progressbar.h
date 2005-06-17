/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Progressbar.h,v 1.7 2005/06/17 19:33:06 avenger_teambg Exp $
 *
 */

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "Control.h"
#include "Sprite2D.h"
#include "Animation.h"

// !!! Keep in sync with GUIDefines.py !!!
#define IE_GUI_PROGRESS_END_REACHED  0x01000000


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Progressbar : public Control  {
public: 
	Progressbar(unsigned short KnobStepsCount, bool Clear = false);
	~Progressbar();
	/** Draws the Control on the Output Display */
	void Draw(unsigned short x, unsigned short y);
	/** Returns the actual Progressbar Position */
	unsigned int GetPosition();
	/** Sets the actual Progressbar Position trimming to the Max and Min Values */
	void SetPosition(unsigned int pos);
	/** Sets the background images */
	void SetImage(Sprite2D * img, Sprite2D * img2);
	/** Sets a bam resource for progressbar */
	void SetAnimation(Animation *arg);
	/** Sets a mos resource for progressbar cap */
	void SetBarCap(Sprite2D *img3);
	/** Sets the mos coordinates for the progressbar filler mos/cap */
	void SetSliderPos(int x, int y, int x2, int y2);
	/** Dummy function */
	int SetText(const char * string, int pos = 0);
	/** Redraws a progressbar which is associated with VariableName */
	void RedrawProgressbar(char *VariableName, int Sum);
	/** Set handler for specified event */
	bool SetEvent(int eventType, EventHandler handler);

private: // Private attributes
	/** BackGround Images. If smaller than the Control Size, the image will be tiled. */
	Sprite2D * BackGround;
	Sprite2D * BackGround2; //mos resource for the filling of the bar 
	/** Knob Steps Count */
	unsigned int KnobStepsCount;
	int KnobXPos, KnobYPos; //relative coordinates for Background2
	int CapXPos, CapYPos; //relative coordinates for PBarCap
	/** If true, on deletion the Progressbar will destroy the associated images */
	bool Clear;
	/** The bam cycle whose frames work as a progressbar (animated progressbar) */
	Animation *PBarAnim;
	/** The most for the progressbar cap (linear progressbar) */
	Sprite2D *PBarCap;
public:
/** EndReached Scripted Event Function Name */
	EventHandler EndReached;
};

#endif
