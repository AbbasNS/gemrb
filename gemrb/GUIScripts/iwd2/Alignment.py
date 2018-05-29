# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation, alignment (GUICG3)
import GemRB
from GUIDefines import *
import CharOverview
import CommonTables

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	
	Class = GemRB.GetVar("Class")-1
	KitName = CommonTables.Classes.GetRowName(Class)

	AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	AlignmentWindow = GemRB.LoadWindow(3, "GUICG")
	CharOverview.PositionCharGenWin(AlignmentWindow)
	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText (CommonTables.Aligns.GetValue (i, 0))

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		if AlignmentOk.GetValue (KitName, CommonTables.Aligns.GetValue (i, 4)) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, AlignmentPress)
			Button.SetVarAssoc("Alignment", i+1)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.MakeEscape()

	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.MakeDefault()

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.Focus()
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")-1
	TextAreaControl.SetText (CommonTables.Aligns.GetValue (Alignment, 1))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	return

def NextPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetNextScript("CharGen5") #appearance
	return
