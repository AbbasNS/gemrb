# -*-python-*-
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/GUIINV.py,v 1.16 2006/08/15 15:33:29 avenger_teambg Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import string
import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_slots import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

InventoryWindow = None
ItemInfoWindow = None
ItemAmountWindow = None
ItemIdentifyWindow = None
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

def OpenInventoryWindow ():
	global InventoryWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow
	
	if CloseOtherWindow (OpenInventoryWindow):
		GemRB.UnloadWindow (InventoryWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

		InventoryWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIINV", 640, 480)
	InventoryWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", InventoryWindow)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenInventoryWindow")
	GemRB.SetWindowFrame (OptionsWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)

	#Ground Item
	for i in range (5):
		Button = GemRB.GetControl (Window, i+68)
		GemRB.SetVarAssoc (Window, Button, "GroundItemButton", i)
		GemRB.SetButtonFont (Window, Button, "NUMBER")
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,128,128,255,64,0,1)

	#ground items scrollbar
	ScrollBar = GemRB.GetControl (Window, 66)
	GemRB.SetEvent (Window, ScrollBar, IE_GUI_SCROLLBAR_ON_CHANGE, "RefreshInventoryWindow")

	#major & minor clothing color
	Button = GemRB.GetControl (Window, 62)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	GemRB.SetTooltip (Window, Button, 12007)

	Button = GemRB.GetControl (Window, 63)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	GemRB.SetTooltip (Window, Button, 12008)

	# portrait
	Button = GemRB.GetControl (Window, 50)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnAutoEquip")

	# encumbrance
	Label = GemRB.CreateLabel (Window, 0x10000043, 5,385,60,15,"NUMBER","0:",IE_FONT_ALIGN_LEFT|IE_FONT_ALIGN_TOP)
	Label = GemRB.CreateLabel (Window, 0x10000044, 5,455,80,15,"NUMBER","0:",IE_FONT_ALIGN_RIGHT|IE_FONT_ALIGN_TOP)

	# armor class
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetTooltip (Window, Label, 4197)

	# hp current
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetTooltip (Window, Label, 4198)

	# hp max
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetTooltip (Window, Label, 4199)

	#info label, game paused, etc
	Label = GemRB.GetControl (Window, 0x1000003f)
	GemRB.SetText (Window, Label, "")

	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for slot in range (SlotCount):
		SlotType = GemRB.GetSlotType (slot+1)
		if SlotType["ID"]:
			Button = GemRB.GetControl (Window, SlotType["ID"])
			GemRB.SetVarAssoc (Window, Button, "ItemButton", slot+1)
			GemRB.SetButtonFont (Window, Button, "NUMBER")
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_TOP | IE_GUI_BUTTON_PICTURE, OP_OR)
			GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,128,128,255,64,0,1)

	GemRB.SetVar ("TopIndex", 0)
	SetSelectionChangeHandler (UpdateInventoryWindow)
	UpdateInventoryWindow ()
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 3)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def ColorDonePress():
	pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.UnloadWindow (ColorPicker)
	GemRB.SetVisible (InventoryWindow,1)
	ColorTable = GemRB.LoadTable ("clowncol")
	PickedColor=GemRB.GetTableValue (ColorTable, ColorIndex, GemRB.GetVar ("Selected"))
	GemRB.UnloadTable (ColorTable)
	if ColorIndex==2:
		GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)
		UpdateInventoryWindow ()
		return
	GemRB.SetPlayerStat (pc, IE_MINOR_COLOR, PickedColor)
	UpdateInventoryWindow ()
	return

def MajorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 2
	PickedColor = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1)
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	pc = GemRB.GameGetSelectedPCSingle ()
	ColorIndex = 3
	PickedColor = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1)
	GetColor()
	return

def GetColor():
	global ColorPicker

	ColorTable = GemRB.LoadTable ("clowncol")
	GemRB.SetVisible (InventoryWindow,2) #darken it
	ColorPicker=GemRB.LoadWindow (3)
	GemRB.SetVar ("Selected",-1)
	for i in range (34):
		Button = GemRB.GetControl (ColorPicker, i)
		GemRB.SetButtonState (ColorPicker, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags (ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range (34):
		MyColor = GemRB.GetTableValue (ColorTable, ColorIndex, i)
		if MyColor == "*":
			break
		Button = GemRB.GetControl (ColorPicker, i)
		GemRB.SetButtonBAM (ColorPicker, Button, "COLGRAD", 2, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar ("Selected",i)
			Selected = i
		GemRB.SetButtonState (ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc (ColorPicker, Button, "Selected",i)
		GemRB.SetEvent (ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "ColorDonePress")
	GemRB.UnloadTable (ColorTable)
	GemRB.SetVisible (ColorPicker,1)
	return

#complete update
def UpdateInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Container = GemRB.GetContainer (pc, 1)
	ScrollBar = GemRB.GetControl (Window, 66)
	Count = Container['ItemCount']
	if Count<1:
		Count=1
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", Count)
	RefreshInventoryWindow ()
	# populate inventory slot controls
	SlotCount = GemRB.GetSlotType (-1)["Count"]
	for i in range (SlotCount):
		UpdateSlot (pc, i)
	return

#partial update without altering TopIndex
def RefreshInventoryWindow ():
	Window = InventoryWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	# name
	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetText (Window, Label, GemRB.GetPlayerName (pc, 0))

	# portrait
	Button = GemRB.GetControl (Window, 50)
	Color1 = GemRB.GetPlayerStat (pc, IE_METAL_COLOR)
	Color2 = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR)
	Color3 = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR)
	Color4 = GemRB.GetPlayerStat (pc, IE_SKIN_COLOR)
	Color5 = GemRB.GetPlayerStat (pc, IE_LEATHER_COLOR)
	Color6 = GemRB.GetPlayerStat (pc, IE_ARMOR_COLOR)
	Color7 = GemRB.GetPlayerStat (pc, IE_HAIR_COLOR)
	GemRB.SetButtonPLT (Window, Button, GetActorPaperDoll (pc),
		Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 0)

	PortraitTable = GemRB.LoadTable ("PDOLLS")
	anim_id = GemRB.GetPlayerStat (pc, IE_ANIMATION_ID)
	row = "0x%04X" %anim_id
	size = GemRB.GetTableValue (PortraitTable, row, "SIZE")

	# Weapon
	slot_item = GemRB.GetSlotItem (pc, GemRB.GetEquippedQuickSlot(pc) )
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			GemRB.SetButtonPLT(Window, Button, "WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 1)

	# Shield
	slot_item = GemRB.GetSlotItem (pc, 3)
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			if (GemRB.CanUseItemType(item['Type'], SLOT_WEAPON)):
				# off-hand weapon
				GemRB.SetButtonPLT(Window, Button, "WP" + size + item['AnimationType'] + "OIN", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 2)
			else:
				# shield
				GemRB.SetButtonPLT(Window, Button, "WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 2)
	# Helmet
	slot_item = GemRB.GetSlotItem (pc, 1)
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		if (item['AnimationType'] != ''):
			GemRB.SetButtonPLT(Window, Button, "WP" + size + item['AnimationType'] + "INV", Color1, Color2, Color3, Color4, Color5, Color6, Color7, 0, 3)

	# encumbrance
	SetEncumbranceLabels( Window, 0x10000043, 0x10000044, pc)

	# armor class
	ac = GemRB.GetPlayerStat (pc, IE_ARMORCLASS)
	Label = GemRB.GetControl (Window, 0x10000038)
	GemRB.SetText (Window, Label, str (ac))

	# hp current
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	Label = GemRB.GetControl (Window, 0x10000039)
	GemRB.SetText (Window, Label, str (hp))

	# hp max
	hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	Label = GemRB.GetControl (Window, 0x1000003a)
	GemRB.SetText (Window, Label, str (hpmax))

	# party gold
	Label = GemRB.GetControl (Window, 0x10000040)
	GemRB.SetText (Window, Label, str (GemRB.GameGetPartyGold ()))

	# class
	ClassTitle = GetActorClassTitle (pc)
	Label = GemRB.GetControl (Window, 0x10000042)
	GemRB.SetText (Window, Label, ClassTitle)

	Button = GemRB.GetControl (Window, 62)
	Color = GemRB.GetPlayerStat (pc, IE_MAJOR_COLOR, 1)
	GemRB.SetButtonBAM (Window, Button, "COLGRAD", 1, 0, Color)

	Button = GemRB.GetControl (Window, 63)
	Color = GemRB.GetPlayerStat (pc, IE_MINOR_COLOR, 1)
	GemRB.SetButtonBAM (Window, Button, "COLGRAD", 1, 0, Color)

	# update ground inventory slots
	Container = GemRB.GetContainer(pc, 1)
	TopIndex = GemRB.GetVar ("TopIndex")
	for i in range (5):
		Button = GemRB.GetControl (Window, i+68)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItemGround")
		Slot = GemRB.GetContainerItem (pc, i+TopIndex)
		if Slot != None:
			item = GemRB.GetItem (Slot['ItemResRef'])
			identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
			GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if not identified or item["ItemNameIdentified"] == -1:
				GemRB.SetTooltip (Window, Button, item["ItemName"])
				GemRB.EnableButtonBorder (Window, Button, 0, 1)
			else:
				GemRB.SetTooltip (Window, Button, item["ItemNameIdentified"])
				GemRB.EnableButtonBorder (Window, Button, 0, 0)

			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItemGround")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenGroundItemInfoWindow")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenGroundItemAmountWindow")
		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetTooltip (Window, Button, 12011)
			GemRB.EnableButtonBorder (Window, Button, 0, 0)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")

	#making window visible/shaded depending on the pc's state
	GemRB.SetVisible (Window, 1)
	return

def UpdateSlot (pc, slot):

	Window = InventoryWindow
	SlotType = GemRB.GetSlotType (slot+1, pc)
	if not SlotType["ID"]:
		return

	if GemRB.IsDraggingItem():
		#get dragged item
		drag_item = GemRB.GetSlotItem(0,0)
		drag_item = GemRB.GetItem (drag_item["ItemResRef"])
		itemtype = drag_item["Type"]
	else:
		itemtype = -1

	Button = GemRB.GetControl (Window, SlotType["ID"])
	slot_item = GemRB.GetSlotItem (pc, slot+1)

	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDragItem")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	if slot_item:
		item = GemRB.GetItem (slot_item["ItemResRef"])
		identified = slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED

		GemRB.SetItemIcon (Window, Button, slot_item["ItemResRef"])
		if item["StackAmount"] > 1:
			GemRB.SetText (Window, Button, str (slot_item["Usages0"]))
		else:
			GemRB.SetText (Window, Button, "")

		if not identified or item["ItemNameIdentified"] == -1:
			GemRB.SetTooltip (Window, Button, item["ItemName"])
			GemRB.EnableButtonBorder (Window, Button, 0, 1)
		else:
			GemRB.SetTooltip (Window, Button, item["ItemNameIdentified"])
			GemRB.EnableButtonBorder (Window, Button, 0, 0)

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnDragItem")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenItemInfoWindow")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "OpenItemAmountWindow")
	else:

		if SlotType["ResRef"]=="*":
			GemRB.SetButtonBAM (Window, Button, "",0,0,0)
			itemtype = -1
		elif SlotType["ResRef"]=="":
			GemRB.SetButtonBAM (Window, Button, "",0,0,0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			itemtype = -1
		else:
			GemRB.SetButtonBAM (Window, Button, SlotType["ResRef"],0,0,0)
		GemRB.SetText (Window, Button, "")
		GemRB.SetTooltip (Window, Button, SlotType["Tip"])
		GemRB.EnableButtonBorder (Window, Button, 0, 0)

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")

	if itemtype>=0 and GemRB.CanUseItemType(itemtype, SlotType["Type"]):
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SECOND)
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)

	if slot_item and (GemRB.GetEquippedQuickSlot (pc)==slot+1):
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_SECOND)

	return

def OnDragItemGround ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("GroundItemButton")+GemRB.GetVar("TopIndex")
	if not GemRB.IsDraggingItem ():
		slot_item = GemRB.GetContainerItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 1) #container
	else:
		GemRB.DropDraggedItem (pc, -2) #dropping on ground

	UpdateInventoryWindow ()
	return

def OnAutoEquip ():
	if not GemRB.IsDraggingItem ():
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	GemRB.DropDraggedItem (pc, -1)

	if GemRB.IsDraggingItem ():
		GemRB.PlaySound("GAM_47")  #failed equip

	UpdateInventoryWindow ()
	return

def OnDragItem ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("ItemButton")
	if not GemRB.IsDraggingItem ():
		slot_item = GemRB.GetSlotItem (pc, slot)
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"], 0, 0)
	else:
		GemRB.DropDraggedItem (pc, slot)
		if GemRB.IsDraggingItem ():
			GemRB.PlaySound("GAM_47")  #failed equip

	UpdateInventoryWindow ()
	return

def OnDropItemToPC ():
	pc = GemRB.GetVar ("PressedPortrait") + 1

	GemRB.DropDraggedItem (pc, -3)
	if GemRB.IsDraggingItem ():
		GemRB.PlaySound("GAM_47")  #failed equip
	UpdateInventoryWindow ()
	return

def IdentifyUseSpell ():
	return

def IdentifyUseScroll ():
	return

def CloseIdentifyItemWindow ():
	GemRB.UnloadWindow (ItemIdentifyWindow)
	return

def IdentifyItemWindow ():
	global ItemIdentifyWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	ItemIdentifyWindow = Window = GemRB.LoadWindow (9)
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17105)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 17106)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyUseSpell")
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseIdentifyItemWindow")
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 19394)
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseItemInfoWindow ():
	GemRB.UnloadWindow (ItemInfoWindow)
	return

def DisplayItem (itemresref, type):
	global ItemInfoWindow

	item = GemRB.GetItem (itemresref)
	ItemInfoWindow = Window = GemRB.LoadWindow (5)

	#item icon
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
	GemRB.SetItemIcon (Window, Button, itemresref,0)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	#middle button
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 11973)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseItemInfoWindow")

	#textarea
	Text = GemRB.GetControl (Window, 5)
	if (type&2):
		text = item["ItemDesc"]
	else:
		text = item["ItemDescIdentified"]
	GemRB.SetText (Window, Text, text)

	Button = GemRB.GetControl (Window, 6)
	#left button
	Button = GemRB.GetControl(Window, 8)

	if type&2:
		GemRB.SetText (Window, Button, 14133)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "IdentifyItemWindow")
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	#description icon
	#Button = GemRB.GetControl (Window, 7)
	#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
	#GemRB.SetItemIcon (Window, Button, itemresref,2)

	#right button
	Button = GemRB.GetControl(Window, 9)
	drink = (type&1) and (item["Function"]&1)
	read = (type&1) and (item["Function"]&2)
	container = (type&1) and (item["Function"]&4)
	dialog = (type&1) and (item["Dialog"]!="")
	if drink:
		GemRB.SetText (Window, Button, 19392)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DrinkItemWindow")
	elif read:
		GemRB.SetText (Window, Button, 17104)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ReadItemWindow")
	elif container:
		GemRB.SetText (Window, Button, 14133)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenItemWindow")
	elif dialog:
		GemRB.SetText (Window, Button, item["DialogName"])
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "DialogItemWindow")
	else:
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	Text = GemRB.GetControl (Window, 0x1000000b)
	if (type&2):
		text = item["ItemName"]
	else:
		text = item["ItemNameIdentified"]
	GemRB.SetText (Window, Text, text)

	GemRB.ShowModal(ItemInfoWindow, MODAL_SHADOW_GRAY)
	return

def OpenItemInfoWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar ("ItemButton")
	slot_item = GemRB.GetSlotItem (pc, slot)

	if slot_item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 1
	else:
		value = 3
	DisplayItem(slot_item["ItemResRef"], value)
	return

def OpenGroundItemInfoWindow ():
	global ItemInfoWindow

	pc = GemRB.GameGetSelectedPCSingle ()

	slot = GemRB.GetVar("TopIndex")+GemRB.GetVar("GroundItemButton")
	print "OpenGroundItemInfo", slot
	slot_item = GemRB.GetContainerItem (pc, slot)
	if item["Flags"] & IE_INV_ITEM_IDENTIFIED:
		value = 0
	else:
		value = 2
	DisplayItem(slot_item["ItemResRef"], value)  #the ground items are only displayable
	return

###################################################
# End of file GUIINV.py
