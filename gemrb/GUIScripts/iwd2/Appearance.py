#character generation, color (GUICG13)
import GemRB

global IE_ANIM_ID
ColorTable = 0
HairTable = 0
SkinTable = 0
ColorWindow = 0
ColorPicker = 0
DoneButton = 0
ColorIndex = 0
PickedColor = 0
HairButton = 0
SkinButton = 0
MajorButton = 0
MinorButton = 0
Color1 = 0
Color2 = 0
Color3 = 0
Color4 = 0
PDollButton = 0
IE_ANIM_ID = 206

def RefreshPDoll():
	PDollTable = GemRB.LoadTable("avatars")
	AnimID=0x6000
	table=GemRB.LoadTable("avprefr")
	AnimID=AnimID+GemRB.GetTableValue(table,GemRB.GetVar("BaseRace"),0)
	table=GemRB.LoadTable("avprefc")
	AnimID=AnimID+GemRB.GetTableValue(table,GemRB.GetVar("BaseClass"),0)
	table=GemRB.LoadTable("avprefg")
	AnimID=AnimID+GemRB.GetTableValue(table, GemRB.GetVar("Gender"),0)
	print "Anim ID:", hex(AnimID)
	ResRef=GemRB.GetTableValue(PDollTable,hex(AnimID), "AT_1")+"G1"
	GemRB.SetButtonBAM(ColorWindow, PDollButton, ResRef,10,0,0)
	GemRB.SetButtonFlags(ColorWindow, PDollButton, IE_GUI_BUTTON_ANIMATED,OP_OR)
	return

def OnLoad():
	global ColorWindow, DoneButton, PDollButton
	global HairTable, SkinTable, ColorTable
	global HairButton, SkinButton, MajorButton, MinorButton
	global Color1, Color2, Color3, Color4
	
	GemRB.LoadWindowPack("GUICG")
	ColorWindow=GemRB.LoadWindow(13)

	RaceTable = GemRB.LoadTable("races")
	Race = GemRB.GetVar("Race")-1
	HairTable = GemRB.LoadTable(GemRB.GetTableValue(RaceTable, Race, 5))
	SkinTable = GemRB.LoadTable(GemRB.GetTableValue(RaceTable, Race, 6))
	ColorTable = GemRB.LoadTable("clowncol")

	#set these colors to some default
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitIndex = GemRB.GetVar("PortraitIndex")
	Color1=GemRB.GetTableValue(PortraitTable,PortraitIndex,1)
	Color2=GemRB.GetTableValue(PortraitTable,PortraitIndex,2)
	Color3=GemRB.GetTableValue(PortraitTable,PortraitIndex,3)
	Color4=GemRB.GetTableValue(PortraitTable,PortraitIndex,4)
	PDollButton = GemRB.GetControl(ColorWindow, 1)
	GemRB.SetButtonFlags(ColorWindow, PDollButton, IE_GUI_BUTTON_PICTURE,OP_OR)

	HairButton = GemRB.GetControl(ColorWindow, 2)
	GemRB.SetButtonFlags(ColorWindow, HairButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, HairButton, IE_GUI_BUTTON_ON_PRESS,"HairPress")
	GemRB.SetButtonBAM(ColorWindow, HairButton, "COLGRAD", 1, 0, Color1)

	SkinButton = GemRB.GetControl(ColorWindow, 3)
	GemRB.SetButtonFlags(ColorWindow, SkinButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, SkinButton, IE_GUI_BUTTON_ON_PRESS,"SkinPress")
	GemRB.SetButtonBAM(ColorWindow, SkinButton, "COLGRAD", 1, 0, Color2)

	MajorButton = GemRB.GetControl(ColorWindow, 5)
	GemRB.SetButtonFlags(ColorWindow, MajorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, MajorButton, IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	GemRB.SetButtonBAM(ColorWindow, MajorButton, "COLGRAD", 1, 0, Color3)

	MinorButton = GemRB.GetControl(ColorWindow, 4)
	GemRB.SetButtonFlags(ColorWindow, MinorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, MinorButton, IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	GemRB.SetButtonBAM(ColorWindow, MinorButton, "COLGRAD", 1, 0, Color4)

	BackButton = GemRB.GetControl(ColorWindow,13)
	GemRB.SetText(ColorWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ColorWindow,0)
	GemRB.SetText(ColorWindow,DoneButton,11973)
        GemRB.SetButtonFlags(ColorWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent(ColorWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ColorWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RefreshPDoll()
	GemRB.SetVisible(ColorWindow,1)
	return

def DonePress():
	global Color1, Color2, Color3, Color4
	GemRB.UnloadWindow(ColorPicker)
	ColorWindow=GemRB.LoadWindow(13)
	GemRB.SetVisible(ColorWindow,1)
	
	if ColorIndex==0:
		PickedColor=GemRB.GetTableValue(HairTable, GemRB.GetVar("Selected"),0)
		Color1=PickedColor
		GemRB.SetButtonBAM(ColorWindow, HairButton, "COLGRAD", 1, 0, Color1)
		RefreshPDoll()
		return
	if ColorIndex==1:
		PickedColor=GemRB.GetTableValue(SkinTable, GemRB.GetVar("Selected"),0)
		Color2=PickedColor
		GemRB.SetButtonBAM(ColorWindow, SkinButton, "COLGRAD", 1, 0, Color2)
		RefreshPDoll()
		return
	if ColorIndex==2:
		PickedColor=GemRB.GetTableValue(ColorTable, 0, GemRB.GetVar("Selected"))
		Color3=PickedColor
		GemRB.SetButtonBAM(ColorWindow, MajorButton, "COLGRAD", 1, 0, Color3)
		RefreshPDoll()
		return

	PickedColor=GemRB.GetTableValue(ColorTable, 1, GemRB.GetVar("Selected"))
	Color4=PickedColor
	GemRB.SetButtonBAM(ColorWindow, MinorButton, "COLGRAD", 1, 0, Color4)
	RefreshPDoll()
	return

def GetColor():
	global ColorPicker

	ColorPicker=GemRB.LoadWindow(14)
	GemRB.SetVar("Selected",-1)
	for i in range(0,33):
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	m = 33
	if ColorIndex==0:
		m=GemRB.GetTableRowCount(HairTable)
		t=HairTable
	if ColorIndex==1:
		m=GemRB.GetTableRowCount(SkinTable)
		t=SkinTable
	for i in range(0,m):
		if ColorIndex<2:
			MyColor=GemRB.GetTableValue(t,i,0)
		else:
			MyColor=GemRB.GetTableValue(ColorTable, ColorIndex-2, i)
			print i, MyColor
		if MyColor == "*":
			break
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonBAM(ColorPicker, Button, "COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
			Selected = i
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc(ColorPicker, Button, "Selected",i)
		GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	
	Button = GemRB.GetControl(ColorPicker, 33)
	#default button
	GemRB.SetVarAssoc(ColorPicker, Button, "Selected", 0)
	GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetVisible(ColorPicker,1)
	return

def HairPress():
	global ColorIndex, PickedColor

#	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 0
	PickedColor = Color1
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

#	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 1
	PickedColor = Color2
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

#	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 2
	PickedColor = Color3
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

#	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 3
	PickedColor = Color4
	GetColor()
	return

def BackPress():
	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
        GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVar("Color1",Color1)
	GemRB.SetVar("Color2",Color2)
	GemRB.SetVar("Color3",Color3)
	GemRB.SetVar("Color4",Color4)
	GemRB.UnloadTable(HairTable)
	GemRB.UnloadTable(SkinTable)
	GemRB.UnloadTable(ColorTable)
	GemRB.SetNextScript("CSound") #character sound
	return
