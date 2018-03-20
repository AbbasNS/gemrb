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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Video.h
 * Declares Video, base class for video output plugins.
 * @author The GemRB Project
 */

#ifndef VIDEO_H
#define VIDEO_H

#include "globals.h"

#include "Plugin.h"
#include "Polygon.h"
#include "ScriptedAnimation.h"

#include <deque>
#include <algorithm>

namespace GemRB {

class EventMgr;
class Font;
class Palette;
class SpriteCover;

// Note: not all these flags make sense together. Specifically:
// NOSHADOW overrides TRANSSHADOW, and BLIT_GREY overrides BLIT_SEPIA
enum SpriteBlitFlags {
	BLIT_HALFTRANS = IE_VVC_TRANSPARENT, // 2
	BLIT_BLENDED = IE_VVC_BLENDED, // 8; not implemented in SDLVideo yet
	BLIT_MIRRORX = IE_VVC_MIRRORX, // 0x10
	BLIT_MIRRORY = IE_VVC_MIRRORY, // 0x20
	BLIT_NOSHADOW = 0x1000,
	BLIT_TRANSSHADOW = 0x2000,
	BLIT_TINTED = 0x00010000, // IE_VVC_TINT = 0x00030000
	BLIT_GREY = IE_VVC_GREYSCALE, // 0x80000; timestop palette
	BLIT_SEPIA = IE_VVC_SEPIA, // 0x02000000; dream scene palette
	BLIT_DARK = IE_VVC_DARKEN, // 0x00100000; not implemented in SDLVideo yet
	BLIT_GLOW = IE_VVC_GLOWING // 0x00200000; not implemented in SDLVideo yet
	// Note: bits 29,30,31 are used by SDLVideo internally
};

// !!! Keep this synchronized with GUIDefines.py !!!
// used for calculating the tooltip delay limit and the real tooltip delay
#define TOOLTIP_DELAY_FACTOR 250

class GEM_EXPORT VideoBuffer {
protected:
	Region rect;
	
public:
	VideoBuffer(const Region& r) : rect(r) {}
	virtual ~VideoBuffer() {}
	
	::GemRB::Size Size() const { return rect.Dimensions(); }
	Point Origin() const { return rect.Origin(); }
	Region Rect() const  { return rect; }
	
	void SetOrigin(const Point& p) { rect.x = p.x, rect.y = p.y; }

	virtual void Clear() = 0;
	// CopyPixels takes at least one void* buffer with implied pitch of Region.w, otherwise alternating pairs of buffers and their coresponding pitches
	virtual void CopyPixels(const Region& bufDest, const void* pixelBuf, const int* pitch = NULL, ...) = 0;
	
	virtual bool RenderOnDisplay(void* display) const = 0;
};

/**
 * @class Video
 * Base class for video output plugins.
 */

class GEM_EXPORT Video : public Plugin {
public:
	static const TypeID ID;

	enum BufferFormat {
		DISPLAY, // whatever format the video driver thinks is best for the display
		RGBPAL8,	// 8 bit palettized
		RGB565, // 16 bit RGB (truecolor)
		RGBA8888, // Standard 8 bits per channel with alpha
		YV12    // YUV format for BIK videos
	};

protected:
	unsigned long lastTime;
	EventMgr* EvntManager;
	Region screenClip;
	Size screenSize;
	int bpp;
	bool fullscreen;

	unsigned char Gamma10toGamma22[256];
	unsigned char Gamma22toGamma10[256];

	typedef std::deque<VideoBuffer*> VideoBuffers;

	// collection of all existing video buffers
	VideoBuffers buffers;
	// collection built by calls to PushDrawingBuffer() and cleared after SwapBuffers()
	// the collection is iterated and drawn in order during SwapBuffers()
	// Note: we can add the same buffer more than once to drawingBuffers!
	VideoBuffers drawingBuffers;
	// the current top of drawingBuffers that draw operations occur on
	VideoBuffer* drawingBuffer;

	Region ClippedDrawingRect(const Region& target, const Region* clip = NULL) const;
	virtual void Wait(unsigned long) = 0;

private:
	virtual VideoBuffer* NewVideoBuffer(const Region&, BufferFormat)=0;
	virtual void SwapBuffers(VideoBuffers&)=0;
	virtual int PollEvents() = 0;
	virtual int CreateDriverDisplay(const Size& s, int bpp, const char* title) = 0;

public:
	Video(void);
	virtual ~Video(void);

	virtual int Init(void) = 0;
	int CreateDisplay(const Size&, int bpp, bool fullscreen, const char* title);
	/** Toggles GemRB between fullscreen and windowed mode. */
	bool ToggleFullscreenMode();
	virtual bool SetFullscreenMode(bool set) = 0;
	bool GetFullscreenMode() const;
	/** Swaps displayed and back buffers */
	int SwapBuffers(unsigned int fpscap = 30);
	VideoBuffer* CreateBuffer(const Region&, BufferFormat = DISPLAY);
	void DestroyBuffer(VideoBuffer*);
	void PushDrawingBuffer(VideoBuffer*);
	void PopDrawingBuffer();
	/** Grabs and releases mouse cursor within GemRB window */
	virtual bool ToggleGrabInput() = 0;
	const Size& GetScreenSize() { return screenSize; }

	/** Displays or hides a virtual (software) keyboard*/
	virtual void ShowSoftKeyboard() = 0;
	virtual void HideSoftKeyboard() = 0;

	virtual bool TouchInputEnabled() = 0;

	virtual Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0) = 0;
	virtual Sprite2D* CreateSprite8(int w, int h, void* pixels,
									Palette* palette, bool cK = false, int index = 0) = 0;
	virtual Sprite2D* CreatePalettedSprite(int w, int h, int bpp, void* pixels,
										   Color* palette, bool cK = false, int index = 0) = 0;
	virtual bool SupportsBAMSprites() { return false; }

	virtual void BlitTile(const Sprite2D* spr, const Sprite2D* mask, int x, int y,
						  const Region* clip, unsigned int flags) = 0;
	void BlitSprite(const Sprite2D* spr, int x, int y,
					const Region* clip = NULL);
	virtual void BlitSprite(const Sprite2D* spr, const Region& src, Region dst) = 0;

	// Note: Tint cannot be constified, because it is modified locally
	// not a pretty interface :)
	virtual void BlitGameSprite(const Sprite2D* spr, int x, int y,
								unsigned int flags, Color tint,
								SpriteCover* cover,
								const Region* clip = NULL) = 0;

	void BlitGameSpriteWithPalette(Sprite2D* spr, Palette* pal, int x, int y,
				   unsigned int flags, Color tint,
				   SpriteCover* cover,
				   const Region* clip = NULL);

	/** Return GemRB window screenshot.
	 * It's generated from the momentary back buffer */
	virtual Sprite2D* GetScreenshot( Region r ) = 0;
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	virtual void DrawRect(const Region& rgn, const Color& color, bool fill = true) = 0;

	virtual void DrawPoint(const Point&, const Color& color) = 0;
	/** Draws a circle */
	virtual void DrawCircle(const Point& origin, unsigned short r, const Color& color) = 0;
	/** Draws an Ellipse Segment */
	virtual void DrawEllipseSegment(const Point& origin, unsigned short xr, unsigned short yr, const Color& color,
									double anglefrom, double angleto, bool drawlines = true) = 0;
	/** Draws an ellipse */
	virtual void DrawEllipse(const Point& origin, unsigned short xr, unsigned short yr, const Color& color) = 0;
	/** Draws a polygon on the screen */
	virtual void DrawPolygon(Gem_Polygon* poly, const Point& origin, const Color& color, bool fill = false) = 0;
	/** Draws a line segment */
	virtual void DrawLine(const Point& p1, const Point& p2, const Color& color) = 0;
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, const Sprite2D* img);
	/** Sets Event Manager */
	void SetEventMgr(EventMgr* evnt);
	/** Flips sprite, returns new sprite */
	Sprite2D *MirrorSprite(const Sprite2D *sprite, unsigned int flags, bool MirrorAnchor);
	/** Duplicates and transforms sprite to have an alpha channel */
	Sprite2D* CreateAlpha(const Sprite2D *sprite);

	/** Sets Clip Rectangle */
	void SetScreenClip(const Region* clip);
	/** Gets Clip Rectangle */
	const Region& GetScreenClip() { return screenClip; }
	virtual void SetGamma(int brightness, int contrast) = 0;

	/** Scales down a sprite by a ratio */
	Sprite2D* SpriteScaleDown( const Sprite2D* sprite, unsigned int ratio );
	/** Creates an ellipse or circle shaped sprite with various intensity
	 *  for projectile light spots */
	Sprite2D* CreateLight(int radius, int intensity);

	Color SpriteGetPixelSum (const Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio);
};

}

#endif
