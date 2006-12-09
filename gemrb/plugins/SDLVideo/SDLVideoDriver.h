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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SDLVideo/SDLVideoDriver.h,v 1.70 2006/12/09 15:00:26 avenger_teambg Exp $
 *
 */

#ifndef SDLVIDEODRIVER_H
#define SDLVIDEODRIVER_H

#include "../Core/Video.h"

#include <SDL.h>


class SDLVideoDriver : public Video {
private:
	SDL_Surface* disp;
	SDL_Surface* backBuf;
	SDL_Surface* extra;
	std::vector< Region> upd;//Regions of the Screen to Update in the next SwapBuffer operation.
	Region Viewport;
	Sprite2D* Cursor[3];
	SDL_Rect CursorPos;
	unsigned short CursorIndex;
	Color fadeColor;
	unsigned long lastTime;
	unsigned long lastMouseTime;
	SDL_Event event; /* Event structure */
	//subtitle specific variables
	Font *subtitlefont;
	Palette *subtitlepal;
	Region subtitleregion;
	char *subtitletext;
	ieDword subtitlestrref;
public:
	SDLVideoDriver(void);
	~SDLVideoDriver(void);
	int Init(void);
	int CreateDisplay(int width, int height, int bpp, bool fullscreen);
	void SetDisplayTitle(char* title, char* icon);
	VideoModes GetVideoModes(bool fullscreen = false);
	bool TestVideoMode(VideoMode& vm);
	bool ToggleFullscreenMode();
	int SwapBuffers(void);
	bool ToggleGrabInput();
	short GetWidth() { return ( disp ? disp->w : 0 ); };
	short GetHeight() { return ( disp ? disp->h : 0 ); };

	void InitSpriteCover(SpriteCover* sc);
	void AddPolygonToSpriteCover(SpriteCover* sc,Wall_Polygon* poly,int flags);
	void DestroySpriteCover(SpriteCover* sc);

	void GetMousePos(int &x, int &y);
	void MouseMovement(int x, int y);
	void MoveMouse(unsigned int x, unsigned int y);
	Sprite2D* CreateSprite(int w, int h, int bpp, ieDword rMask,
		ieDword gMask, ieDword bMask, ieDword aMask, void* pixels,
		bool cK = false, int index = 0);
	Sprite2D* CreateSprite8(int w, int h, int bpp, void* pixels,
		void* palette, bool cK = false, int index = 0);
	Sprite2D* CreateSpriteBAM8(int w, int h, bool RLE, void* pixeldata,
		unsigned int datasize, Palette* palette, int transindex);
	void FreeSprite(Sprite2D* &spr);
	void BlitSprite(Sprite2D* spr, int x, int y, bool anchor = false,
		Region* clip = NULL);
	void BlitSpriteHalfTrans(Sprite2D* spr, int x, int y,
		bool anchor = false, Region* clip = NULL);
	void BlitSpriteRegion(Sprite2D* spr, Region& size, int x, int y,
		bool anchor = true, Region* clip = NULL);
	void BlitGameSprite(Sprite2D* spr, int x, int y,
		unsigned int flags, Color tint,
		SpriteCover* cover, Palette *palette = NULL,
		Region* clip = NULL);
	void SetCursor(Sprite2D* up, Sprite2D* down);
	void SetDragCursor(Sprite2D* drag);
	Sprite2D* GetScreenshot( Region r );
	Region GetViewport(void);
	void SetViewport(int x, int y, unsigned int w, unsigned int h);
	void MoveViewportTo(int x, int y, bool center);
	void ConvertToVideoFormat(Sprite2D* sprite);
	/** No descriptions */
	void SetPalette(Sprite2D* spr, Palette* pal);
	/** This function Draws the Border of a Rectangle as described by the Region parameter. The Color used to draw the rectangle is passes via the Color parameter. */
	void DrawRect(Region& rgn, Color& color, bool fill = true, bool clipped = false);
	void DrawRectSprite(Region& rgn, Color& color, Sprite2D* sprite);
	/** This functions Draws a Circle */
	void SetPixel(short x, short y, Color& color, bool clipped = true);
	void GetPixel(short x, short y, Color* color);
	void DrawCircle(short cx, short cy, unsigned short r, Color& color, bool clipped = true);
	/** This functions Draws an Ellipse */
	void DrawEllipse(short cx, short cy, unsigned short xr, unsigned short yr,
		Color& color, bool clipped = true);
	/** This function Draws a Polygon on the Screen */
	void DrawPolyline(Gem_Polygon* poly, Color& color, bool fill = false);
	inline void DrawHLine(short x1, short y, short x2, Color& color, bool clipped = false);
	inline void DrawVLine(short x, short y1, short y2, Color& color, bool clipped = false);
	inline void DrawLine(short x1, short y1, short x2, short y2, Color& color, bool clipped = false);
	/** Blits a Sprite filling the Region */
	void BlitTiled(Region rgn, Sprite2D* img, bool anchor = false);
	/** Send a Quit Signal to the Event Queue */
	bool Quit();
	/** Get the Palette of a Sprite */
	Palette* GetPalette(Sprite2D* spr);
	/** Flips sprite vertically */
	Sprite2D *MirrorSpriteVertical(Sprite2D *sprite, bool MirrorAnchor);
	/** Flips sprite horizontally */
	Sprite2D *MirrorSpriteHorizontal(Sprite2D *sprite, bool MirrorAnchor);
	/** Creates sprite with alpha channel */
	void CreateAlpha(Sprite2D *sprite);
	/** Set Clip Rect */
	void SetClipRect(Region* clip);
	/** Convers a Screen Coordinate to a Game Coordinate */

	void ConvertToGame(short& x, short& y)
	{
		x += Viewport.x;
		y += Viewport.y;
	}

	void SetFadeColor(int r, int g, int b);
	void SetFadePercent(int percent);
	void InitMovieScreen(int &w, int &h);
	void SetMovieFont(Font *stfont, Palette *pal);
	void showFrame(unsigned char* buf, unsigned int bufw,
	unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
	unsigned int h, unsigned int dstx, unsigned int dsty, int truecolor,
	unsigned char *palette, ieDword strRef);
	int PollMovieEvents();
private:
	void DrawMovieSubtitle(ieDword strRef);

public:
	Color SpriteGetPixel (Sprite2D* sprite, unsigned short x, unsigned short y);
	Color SpriteGetPixelSum (Sprite2D* sprite, unsigned short xbase, unsigned short ybase, unsigned int ratio);
	bool IsSpritePixelTransparent (Sprite2D* sprite, unsigned short x, unsigned short y);
	Sprite2D* SpriteScaleDown( Sprite2D* sprite, unsigned int ratio );
	void SetGamma(int brightness, int contrast);
	void release(void)
	{
		delete this;
	}
};

#endif
