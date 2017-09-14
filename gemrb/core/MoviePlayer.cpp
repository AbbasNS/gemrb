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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "MoviePlayer.h"

#include "GUI/Label.h"
#include "Interface.h"

namespace GemRB {

const TypeID MoviePlayer::ID = { "MoviePlayer" };

MoviePlayer::MoviePlayer(void)
{
	framePos = 0;
	subtitles = NULL;
}

MoviePlayer::~MoviePlayer(void)
{
	Stop();
	delete subtitles;
}

void MoviePlayer::SetSubtitles(SubtitleSet* subs)
{
	delete subtitles;
	subtitles = subs;
}

void MoviePlayer::Play(Window* win)
{
	assert(win);
	Video* video = core->GetVideoDriver();

	MoviePlayerControls* mpc = new MoviePlayerControls(*this);
	mpc->SetFrameSize(win->Dimensions());
	win->AddSubviewInFrontOfView(mpc);

	// center over win
	const Region& winFrame = win->Frame();
	const Size& size = Dimensions();
	Point center(winFrame.w/2 - size.w/2, winFrame.h/2 - size.h/2);
	center = center + winFrame.Origin();
	VideoBuffer* subBuf, *vb = video->CreateBuffer(Region(center, size), movieFormat);
	
	if (subtitles) {
		// FIXME: arbitrary frame of my choosing, not sure there is a better method
		// this hould probably at least be sized according to line height
		Region subFrame(0, winFrame.h - center.y, winFrame.w, center.y);
		subBuf = video->CreateBuffer(subFrame);
	}

	// currently, our MoviePlayer implementation takes over the entire screen
	// not only that but the Play method blocks until movie is done/stopped.
	win->Focus(); // we bypass the WindowManager for drawing, but for event handling we need this
	isPlaying = true;
	do {
		// taking over the application runloop...
		
		// we could draw all the windows if we wanted to be able to have videos that aren't fullscreen
		// However, since we completely block the normal game loop we will bypass WindowManager drawing
		//WindowManager::DefaultWindowManager().DrawWindows();
		
		// first draw the window for play controls/subtitles
		//win->Draw();
		if (subtitles) {
			assert(subBuf);
			// we purposely draw on the window, which may be larger than the video
			subBuf->Clear();
			video->PushDrawingBuffer(subBuf);
			subtitles->RenderInBuffer(*subBuf, framePos);
		}
		
		video->PushDrawingBuffer(vb);
		if (DecodeFrame(*vb) == false) {
			Stop(); // error / end
		}
		// TODO: pass movie fps (and remove the cap from within the movie decoders)
	} while ((video->SwapBuffers(0) == GEM_OK) && isPlaying);

	delete win->View::RemoveSubview(mpc);
	video->DestroyBuffer(vb);
	
	if (subtitles)
		video->DestroyBuffer(subBuf);
}

void MoviePlayer::Stop()
{
	isPlaying = false;
}

}
