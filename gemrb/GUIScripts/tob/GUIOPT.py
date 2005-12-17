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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIOPT.py,v 1.13 2005/12/17 10:23:07 avenger_teambg Exp $

# GUIOPT.py - scripts to control options windows mostly from GUIOPT winpack
# Ingame options

###################################################
import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
import GUICommonWindows
from GUISAVE import *
from GUICommonWindows import *

###################################################
GameOptionsWindow = None
PortraitWindow = None
OldPortraitWindow = None
HelpTextArea = None

LoadMsgWindow = None
QuitMsgWindow = None

###################################################
def CloseOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow
	
	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVisible (0,1)
	GemRB.UnhideGUI ()
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	OldPortraitWindow = None
	return

###################################################
def OpenOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow

	CloseOtherWindow (CloseOptionsWindow)

	hideflag = GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIOPT", 640, 480)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "CloseOptionsWindow")
	GemRB.SetWindowFrame (OptionsWindow)

	# Return to Game
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 10308)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseOptionsWindow")	

	# Quit Game
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetText (Window, Button, 13731)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# Load Game
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 13729)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Save Game
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 13730)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSaveMsgWindow")

	# Video Options
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 17162)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")

	# Audio Options
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 17164)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")

	# Gameplay Options
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 17165)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")

	# game version, e.g. v1.1.0000
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, GEMRB_VERSION)
	
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	#GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

	
###################################################

def CloseVideoOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)
	GameOptionsWindow = None
	OpenOptionsWindow ()


def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
	
	GameOptionsWindow = Window = GemRB.LoadWindow (6)

	HelpTextArea = OptHelpText ('VideoOptions', Window, 33, 18038)

	OptDone ('VideoOptions', Window, 21)
	OptCancel ('VideoOptions', Window, 32)

	OptSlider ('Brightness', Window, 3, 'Brightness Correction', 0)
	OptSlider ('Contrast', Window, 22, 'Gamma Correction', 0)

	OptRadio ('BPP', Window, 5, 37, 'BitsPerPixel', 16)
	OptRadio ('BPP', Window, 6, 37, 'BitsPerPixel', 24)
	OptRadio ('BPP', Window, 7, 37, 'BitsPerPixel', 32)
	OptCheckbox ('FullScreen', Window, 9, 38, 'Full Screen', 1)

	OptCheckbox ('TransShadow', Window, 51, 50, 'Translucent Shadows', 1)
	OptCheckbox ('SoftMirrBlt', Window, 40, 44, 'SoftMirrorBlt' ,1)
	OptCheckbox ('SoftTransBlt', Window, 41, 46, 'SoftSrcKeyBlt' ,1)
	OptCheckbox ('SoftStandBlt', Window, 42, 48, 'SoftBltFast' ,1)

	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)
	

def DisplayHelpFullScreen ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18000)

def DisplayHelpBPP ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17205)

def DisplayHelpBrightness ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17203)

def DisplayHelpContrast ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17204)

def DisplayHelpSoftMirrBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18004)

def DisplayHelpSoftTransBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18006)

def DisplayHelpSoftStandBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18007)

def DisplayHelpTransShadow ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 20620)


###################################################

def CloseAudioOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)
	GameOptionsWindow = None
	OpenOptionsWindow ()


def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (7)

	HelpTextArea = OptHelpText ('AudioOptions', Window, 14, 18040)

	OptDone ('AudioOptions', Window, 24)
	OptCancel ('AudioOptions', Window, 25)
	OptButton ('CharacterSounds', Window, 13, 17778)

	OptSlider ('AmbientVolume', Window, 1, 'Volume Ambients', 10)
	OptSlider ('SoundFXVolume', Window, 2, 'Volume SFX', 10)
	OptSlider ('VoiceVolume', Window, 3, 'Volume Voices', 10)
	OptSlider ('MusicVolume', Window, 4, 'Volume Music', 10)
	OptSlider ('MovieVolume', Window, 22, 'Volume Movie', 10)
	
	OptCheckbox ('CreativeEAX', Window, 26, 28, 'Environmental Audio', 1)
	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)
	

def DisplayHelpAmbientVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18008)
	GemRB.UpdateAmbientsVolume()
	
def DisplayHelpSoundFXVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18009)

def DisplayHelpVoiceVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18010)

def DisplayHelpMusicVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18011)
	GemRB.UpdateMusicVolume()

def DisplayHelpMovieVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18012)

def DisplayHelpCreativeEAX ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18022)


###################################################

def CloseGameplayOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)
	GameOptionsWindow = None
	OpenOptionsWindow ()


def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	#gameplayoptions
	GameOptionsWindow = Window = GemRB.LoadWindow (8)
	

	HelpTextArea = OptHelpText ('GameplayOptions', Window, 40, 18042)

	OptDone ('GameplayOptions', Window, 7)
	OptCancel ('GameplayOptions', Window, 20)

	OptSlider ('TooltipDelay', Window, 1, 'Tooltips', 0)
	OptSlider ('MouseScrollingSpeed', Window, 2, 'Mouse Scroll Speed', 0)
	OptSlider ('KeyboardScrollingSpeed', Window, 3, 'Keyboard Scroll Speed', 0)
	OptSlider ('Difficulty', Window, 12, 'Difficulty Level', 0)

	OptCheckbox ('DitherAlways', Window, 14, 25, 'Always Dither', 1)
	OptCheckbox ('Gore', Window, 19, 27, 'Gore', 1)
	OptCheckbox ('Infravision', Window, 42, 44, 'Infravision', 1)
	OptCheckbox ('Weather', Window, 47, 46, 'Weather', 1)
	OptCheckbox ('RestUntilHealed', Window, 50, 48, 'Heal Party on Rest', 1)

	OptButton ('FeedbackOptions', Window, 5, 17163)
	OptButton ('AutopauseOptions', Window, 6, 17166)
	OptButton ('HotkeyOptions', Window, 51, 816)

	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)


def DisplayHelpTooltipDelay ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18017)

def DisplayHelpMouseScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18018)

def DisplayHelpKeyboardScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18019)

def DisplayHelpDifficulty ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18020)

def DisplayHelpDitherAlways ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18021)

def DisplayHelpGore ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18023)

def DisplayHelpInfravision ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 11797)

def DisplayHelpWeather ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 20619)

def DisplayHelpRestUntilHealed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 2242)

###################################################

def CloseFeedbackOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()


def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global GameOptionsWindow, HelpTextArea
	
	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
	
	#feedback
	GameOptionsWindow = Window = GemRB.LoadWindow (9)

	HelpTextArea = OptHelpText ('FeedbackOptions', Window, 28, 18043)

	OptDone ('FeedbackOptions', Window, 26)
	OptCancel ('FeedbackOptions', Window, 27)

	OptSlider ('MarkerFeedback', Window, 8, 'GUI Feedback Level', 1)
	OptSlider ('LocatorFeedback', Window, 9, 'Locator Feedback Level', 1)

	OptCheckbox ('ToHitRolls', Window, 10, 32, 'Rolls', 1)
	OptCheckbox ('CombatInfo', Window, 11, 33, 'Combat Info', 1)
	OptCheckbox ('Actions', Window, 12, 34, 'Actions', 1)
	OptCheckbox ('States', Window, 13, 35, 'State Changes', 1)
	OptCheckbox ('Selection', Window, 14, 36, 'Selection Text', 1)
	OptCheckbox ('Miscellaneous', Window, 15, 37, 'Miscellaneous Text', 1)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	

def DisplayHelpMarkerFeedback ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18024)

def DisplayHelpLocatorFeedback ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18025)

def DisplayHelpToHitRolls ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18026)

def DisplayHelpCombatInfo ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18027)

def DisplayHelpActions ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18028)

def DisplayHelpStates ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18029)

def DisplayHelpSelection ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18030)

def DisplayHelpMiscellaneous ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18031)


###################################################

def CloseAutopauseOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()


def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global GameOptionsWindow, HelpTextArea
	
	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
	
	GameOptionsWindow = Window = GemRB.LoadWindow (10)

	HelpTextArea = OptHelpText ('AutopauseOptions', Window, 15, 18044)

	OptDone ('AutopauseOptions', Window, 11)
	OptCancel ('AutopauseOptions', Window, 14)

	OptCheckbox ('CharacterHit', Window, 1, 17, 'Auto Pause Status', 1)
	OptCheckbox ('CharacterInjured', Window, 2, 18, 'Auto Pause Status', 2)
	OptCheckbox ('CharacterDead', Window, 3, 19, 'Auto Pause Status', 4)
	OptCheckbox ('CharacterAttacked', Window, 4, 20, 'Auto Pause Status', 8)
	OptCheckbox ('WeaponUnusable', Window, 5, 21, 'Auto Pause Status', 16)
	OptCheckbox ('TargetGone', Window, 13, 22, 'Auto Pause Status', 32)
	OptCheckbox ('EndOfRound', Window, 25, 24, 'Auto Pause Status', 64)
	OptCheckbox ('EnemySighted', Window, 26, 27, 'Auto Pause Status', 128)
	OptCheckbox ('SpellCast', Window, 34, 30, 'Auto Pause Status', 256)
	OptCheckbox ('TrapFound', Window, 31, 33, 'Auto Pause Status', 512)
	OptCheckbox ('CenterOnActor', Window, 31, 33, 'Auto Pause Center', 1)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	

def DisplayHelpCharacterHit ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18032)

def DisplayHelpCharacterInjured ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18033)

def DisplayHelpCharacterDead ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18034)

def DisplayHelpCharacterAttacked ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18035)

def DisplayHelpWeaponUnusable ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18036)

def DisplayHelpTargetGone ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18037)

def DisplayHelpEndOfRound ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 10640)

def DisplayHelpEnemySighted ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 23514)

def DisplayHelpSpellCast ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 58171)

def DisplayHelpTrapFound ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31872)

def DisplayHelpCenterOnActor ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 10571)

###################################################

def CloseCharacterSoundsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()


def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""
	global GameOptionsWindow, HelpTextArea
	
	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
	
	GameOptionsWindow = Window = GemRB.LoadWindow (12)

	HelpTextArea = OptHelpText ('CharacterSounds', Window, 16, 18041)

	OptDone ('AutopauseOptions', Window, 24)
	OptCancel ('AutopauseOptions', Window, 25)

	OptCheckbox ('Subtitles', Window, 5, 20, 'Subtitles', 1)
	OptCheckbox ('AttackSounds', Window, 6, 18, 'Attack Sounds', 1)
	OptCheckbox ('Footsteps', Window, 7, 19, 'Footsteps', 1)
	OptRadio ('CommandSounds', Window, 8, 21, 'Command Sounds Frequency', 1)
	OptRadio ('CommandSounds', Window, 9, 21, 'Command Sounds Frequency', 2)
	OptRadio ('CommandSounds', Window, 10, 21, 'Command Sounds Frequency', 3)
	OptRadio ('SelectionSounds', Window, 58, 57, 'Selection Sounds Frequency', 1)
	OptRadio ('SelectionSounds', Window, 59, 57, 'Selection Sounds Frequency', 2)
	OptRadio ('SelectionSounds', Window, 60, 57, 'Selection Sounds Frequency', 3)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	
def DisplayHelpSubtitles ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18015)

def DisplayHelpAttackSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18013)

def DisplayHelpFootsteps ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18014)

def DisplayHelpCommandSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18016)

def DisplayHelpSelectionSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 11352)

###################################################

def OpenSaveMsgWindow ():
	#CloseOptionsWindow ()
	GemRB.SetVar("QuitAfterSave",0)
	OpenSaveWindow ()
	#save the game without quitting
	return

###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:		
		return
		
	LoadMsgWindow = Window = GemRB.LoadWindow (4)
	
	# Load
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15590)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseLoadMsgWindow")

	# Loading a game will destroy ...
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 19531)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def LoadGamePress ():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return

#save game AND quit
def SaveGamePress():
	#CloseOptionsWindow ()
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenSaveWindow ()	
	#GemRB.QuitGame ()
	#GemRB.SetNextScript ("Start")
	return

def QuitGamePress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:		
		return
		
	QuitMsgWindow = Window = GemRB.LoadWindow (5)
	
	# Save
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15589)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")

	# Quit Game
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 15417)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "QuitGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseQuitMsgWindow")

	# The game has not been saved ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 16456) 

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

###################################################

key_list = [
	('GemRB', None),
	('Grab pointer', '^G'),
	('Toggle fullscreen', '^F'),
	('Enable cheats', '^T'),
	('', None),
	
	('IE', None),
	('Open Inventory', 'I'),
	('Open Priest Spells', 'P'),
	('Open Mage Spells', 'S'),
	('Pause Game', 'SPC'),
	('Select Weapon', ''),
	('', None),
	]

###################################################
###################################################

# These functions help to setup controls found
# in Video, Audio, Gameplay, Feedback and Autopause
# options windows

# These controls are usually made from an active
# control (button, slider ...) and a label


def OptSlider (name, window, slider_id, variable, value):
	"""Standard slider for option windows"""
	slider = GemRB.GetControl (window, slider_id)
	GemRB.SetVarAssoc (window, slider, variable, value)
	GemRB.SetEvent (window, slider, IE_GUI_SLIDER_ON_CHANGE, "DisplayHelp" + name)
	return slider


def OptRadio (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)

	return button

def OptCheckbox (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)

	return button

def OptButton (name, window, button_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)	
	GemRB.SetText (window, button, label_strref)

def OptDone (name, window, button_id):
	"""Standard `Done' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 11973) # Done
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)	

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 13727) # Cancel
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)	

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = GemRB.GetControl (window, text_id)
	GemRB.SetText (window, text, text_strref)
	return text


###################################################
# End of file GUIOPT.py
