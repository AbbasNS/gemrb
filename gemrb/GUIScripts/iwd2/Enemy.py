#character generation, racial enemy (GUICG15)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
RaceCount = 0
TopIndex = 0

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(0, 11):
		Button = GemRB.GetControl(RaceWindow,i+22)
		Val = GemRB.GetTableValue(RaceTable, i+TopIndex,0)
		if Val==0:
			GemRB.SetText(RaceWindow, Button, "")
			GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_DISABLED)
		else:
			GemRB.SetText(RaceWindow,Button, Val)
			GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
			GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"RacePress")
			GemRB.SetVarAssoc(RaceWindow,Button,"HateRace",GemRB.GetTableValue(RaceTable,i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, RaceCount, TopIndex
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	RaceWindow = GemRB.LoadWindow(15)

	Class = GemRB.GetVar("BaseClass")
	TmpTable = GemRB.LoadTable("clskills")
	TableName = GemRB.GetTableValue(TmpTable, Class, 0)
	if TableName == "*":
		GemRB.SetNextScript("Skills")
		return
	RaceTable = GemRB.LoadTable(TableName)
	RaceCount = GemRB.GetTableRowCount(RaceTable)-11
	if RaceCount<0:
		RaceCount=0

	for i in range(0,11):
		Button = GemRB.GetControl(RaceWindow,i+22)
		GemRB.SetButtonFlags(RaceWindow,Button,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	BackButton = GemRB.GetControl(RaceWindow,10)
	GemRB.SetText(RaceWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(RaceWindow,11)
	GemRB.SetText(RaceWindow,DoneButton,11973)
	GemRB.SetButtonFlags(RaceWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl = GemRB.GetControl(RaceWindow, 8)
	GemRB.SetText(RaceWindow,TextAreaControl,17256)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = GemRB.GetControl(RaceWindow, 1)
	GemRB.SetVarAssoc(RaceWindow, ScrollBarControl, "TopIndex",RaceCount)
	GemRB.SetEvent(RaceWindow, ScrollBarControl, IE_GUI_SCROLLBAR_ON_CHANGE, "DisplayRaces")

	GemRB.SetEvent(RaceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(RaceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(RaceWindow,1)
	DisplayRaces()
	return

def RacePress():
	Race = GemRB.GetVar("HateRace")
 	Row = GemRB.FindTableValue(RaceTable,1, Race)
	GemRB.SetText(RaceWindow,TextAreaControl, GemRB.GetTableValue(RaceTable, Row, 2) )
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetVar("HateRace",0)  #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("Skills")
	return
