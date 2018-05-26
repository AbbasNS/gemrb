/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUI/EventMgr.h"
#include "Interface.h"
#include "Video.h"

#include "globals.h"
#include "win32def.h"

namespace GemRB {

unsigned long EventMgr::DCDelay = 250;
unsigned long EventMgr::DPDelay = 250;
bool EventMgr::TouchInputEnabled = true;

EventMgr::buttonbits EventMgr::mouseButtonFlags;
EventMgr::buttonbits EventMgr::modKeys;
Point EventMgr::mousePos;
std::map<unsigned short, TouchEvent::Finger> EventMgr::fingerStates;

EventMgr::KeyMap EventMgr::HotKeys = KeyMap();
EventMgr::EventTaps EventMgr::Taps = EventTaps();

bool EventMgr::ModState(unsigned short mod)
{
	return (modKeys & buttonbits(mod)).any();
}

bool EventMgr::MouseButtonState(unsigned short btn)
{
	return (mouseButtonFlags & buttonbits(btn)).any();
}

bool EventMgr::MouseDown()
{
	return mouseButtonFlags.any();
}

bool EventMgr::FingerDown()
{
	return fingerStates.size() > 0;
}

void EventMgr::DispatchEvent(Event& e)
{
	Video* video = core->GetVideoDriver();

	e.time = GetTickCount();

	if (e.type == Event::TextInput) {
		if (e.text.text.length() == 0) {
			return;
		}
	} else if (e.EventMaskFromType(e.type) & Event::AllKeyMask) {
		// first check for hot key listeners
		static unsigned long lastKeyDown = 0;
		static unsigned char repeatCount = 0;
		static KeyboardKey repeatKey = 0;
		
		if (e.type == Event::KeyDown) {
			if (e.keyboard.keycode == repeatKey && e.time <= lastKeyDown + DPDelay) {
				repeatCount++;
			} else {
				repeatCount = 1;
			}
			repeatKey = e.keyboard.keycode;
			lastKeyDown = GetTickCount();
		}

		e.keyboard.repeats = repeatCount;

		int flags = e.mod << 16;
		flags |= e.keyboard.keycode;
		modKeys = e.mod;

		KeyMap::const_iterator hit = HotKeys.find(flags);
		if (hit != HotKeys.end()) {
			KeyMap::value_type::second_type list = hit->second;
			Holder<EventCallback> cb = hit->second.front();
			if ((*cb)(e)) {
				return;
			}
		}
	} else if (e.isScreen) {
		if (e.EventMaskFromType(e.type) & (Event::MouseUpMask | Event::MouseDownMask
										   | Event::TouchUp | Event::TouchDown)
		) {
			// WARNING: these are shared between mouse and touch
			// it is assumed we wont be using both simultaniously
			static unsigned long lastMouseDown = 0;
			static unsigned char repeatCount = 0;
			static EventButton repeatButton = 0;
			static Point repeatPos;

			ScreenEvent& se = (e.type == Event::MouseDown) ? static_cast<ScreenEvent&>(e.mouse) : static_cast<ScreenEvent&>(e.touch);
			EventButton btn = (e.type == Event::MouseDown) ? e.mouse.button : 0;

			if (e.type == Event::MouseDown || e.type == Event::TouchDown) {
				if (video->InTextInput())
					video->StopTextInput();

				if (btn == repeatButton
					&& e.time <= lastMouseDown + DCDelay
					&& repeatPos.isWithinRadius(5, se.Pos())
				) {
					repeatCount++;
				} else {
					repeatCount = 1;
				}
				repeatPos = se.Pos();
				repeatButton = btn;
				lastMouseDown = GetTickCount();
			}

			se.repeats = repeatCount;
		}

		if (e.EventMaskFromType(e.type) & (Event::AllMouseMask)) {
			// set/clear the appropriate buttons
			mouseButtonFlags = e.mouse.buttonStates;
			mousePos = e.mouse.Pos();
		} else { // touch
			TouchEvent& te = (e.type == Event::TouchGesture) ? static_cast<TouchEvent&>(e.gesture) : static_cast<TouchEvent&>(e.touch);
			for (int i = 0; i < FINGER_MAX; ++i) {
				if (i < te.numFingers) {
					fingerStates[i] = te.fingers[i];
				} else {
					fingerStates.erase(i);
				}
			}
		}
	} else {
		// TODO: implement controller events
		if (video->InTextInput())
			video->StopTextInput();
	}

	// no hot keys or their listeners refused the event...
	EventTaps tapsCopy = Taps; // copy this because its possible things get unregistered as a result of the event
	EventTaps::iterator it = tapsCopy.begin();
	for (; it != tapsCopy.end(); ++it) {
		const std::pair<Event::EventTypeMask, Holder<EventCallback> >& pair = it->second;
		if (pair.first & e.EventMaskFromType(e.type)) {
			// all matching taps get a copy of the event
			EventCallback* cb = pair.second.get();
			(*cb)(e);
		}
	}
}

bool EventMgr::RegisterHotKeyCallback(Holder<EventCallback> cb, KeyboardKey key, short mod)
{
	if (key < ' ') { // allowing certain non printables (eg 'F' keys)
		return false;
	}

	int flags = mod << 16;
	flags |= key;

	KeyMap::iterator it = HotKeys.find(flags);
	if (it != HotKeys.end()) {
		it->second.push_front(cb);
	} else {
		HotKeys.insert(std::make_pair(flags, std::list<Holder<EventCallback> >(1, cb)));
	}

	return true;
}

void EventMgr::UnRegisterHotKeyCallback(Holder<EventCallback> cb, KeyboardKey key, short mod)
{
	int flags = mod << 16;
	flags |= key;

	KeyMap::iterator it = HotKeys.find(flags);
	if (it != HotKeys.end()) {
		KeyMap::value_type::second_type::iterator cbit;
		cbit = std::find(it->second.begin(), it->second.end(), cb);
		if (cbit != it->second.end()) {
			cb = *cbit;
			it->second.erase(cbit);
		}
	}
}

EventMgr::TapMonitorId EventMgr::RegisterEventMonitor(Holder<EventCallback> cb, Event::EventTypeMask mask)
{
	static size_t id = 0;
	Taps[id] = std::make_pair(mask, cb);
	// return the "id" of the inserted tap so it could be unregistered later on
	return id++;
}

void EventMgr::UnRegisterEventMonitor(TapMonitorId monitor)
{
	Taps.erase(monitor);
}

Event EventMgr::CreateMouseBtnEvent(const Point& pos, EventButton btn, bool down, int mod)
{
	assert(btn);

	Event e = CreateMouseMotionEvent(pos, mod);

	if (down) {
		e.type = Event::MouseDown;
		e.mouse.buttonStates |= btn;
	} else {
		e.type = Event::MouseUp;
		e.mouse.buttonStates &= ~btn;
	}
	e.mouse.button = btn;

	return e;
}

Event EventMgr::CreateMouseMotionEvent(const Point& pos, int mod)
{
	Event e = {}; // initialize all members to 0
	e.mod = mod;
	e.type = Event::MouseMove;
	e.mouse.buttonStates = mouseButtonFlags.to_ulong();

	e.mouse.x = pos.x;
	e.mouse.y = pos.y;

	Point delta = MousePos() - pos;
	e.mouse.deltaX = delta.x;
	e.mouse.deltaY = delta.y;

	e.isScreen = true;

	return e;
}

Event EventMgr::CreateMouseWheelEvent(const Point& vec, int mod)
{
	Event e = CreateMouseMotionEvent(MousePos(), mod);
	e.type = Event::MouseScroll;
	e.mouse.deltaX = vec.x;
	e.mouse.deltaY = vec.y;
	return e;
}

Event EventMgr::CreateKeyEvent(KeyboardKey key, bool down, int mod)
{
	Event e = {}; // initialize all members to 0
	e.mod = mod;
	e.type = (down) ? Event::KeyDown : Event::KeyUp;
	e.keyboard.keycode = key;
	e.isScreen = false;

	KeyboardKey character = 0;
	if (key >= ' ' && key < GEM_LEFT) { // if printable
		// FIXME: need to translate the keycode for e.keyboard.character
		// probably need to lookup what encoding we are currently using
		character = key;
		if (mod & GEM_MOD_SHIFT) {
			character = toupper(character);
		}
	}
	e.keyboard.character = character;
	return e;
}

Event EventMgr::CreateTouchEvent(ScreenEvent fingers[], int numFingers, bool down, float pressure)
{
	if (numFingers > FINGER_MAX) {
		Log(ERROR, "EventManager", "cannot create a touch event with %d fingers; max is %d.", numFingers, FINGER_MAX);
		return Event();
	}

	Event e = {};
	e.isScreen = true;
	e.mod = 0;
	e.type = (down) ? Event::TouchDown : Event::TouchUp;

	for (int i = 0; i < numFingers; ++i) {
		// TODO: calculate the mid point of all fingers and assign to e.touch.pos
		// do the same for the delta using FingerState() to compare
		e.touch.fingers[i] = fingers[i];
	}

	e.touch.numFingers = numFingers;
	e.touch.pressure = pressure;

	return e;
}

Event EventMgr::CreateTouchGesture(const TouchEvent& touch, float rotation, float pinch)
{
	Event e = {};
	e.isScreen = true;
	e.mod = 0;
	e.type = Event::TouchGesture;
	e.touch = touch;
	e.gesture.dTheta = rotation;
	e.gesture.dDist = pinch;

	return e;
}

Event EventMgr::CreateTextEvent(const char* text)
{
	Event e = {};
	String* string = StringFromCString(text);
	if (string) {
		e = EventMgr::CreateTextEvent(*string);
	}
	delete string;

	return e;
}

Event EventMgr::CreateTextEvent(const String& text)
{
	Event e = {};
	e.type = Event::TextInput;
	e.text.text = text;

	return e;
}

}
