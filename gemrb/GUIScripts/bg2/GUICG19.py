#character generation, sounds (GUICG19)
import GemRB

VoiceList = 0
CharSoundWindow = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharSoundWindow=GemRB.LoadWindow(19)

	VoiceList = GemRB.GetControl (CharSoundWindow, 45)
	GemRB.SetTextAreaFlags (CharSoundWindow, VoiceList, IE_GUI_TEXTAREA_SELECTABLE)
	if GemRB.GetVar ("Gender")==1:
		GemRB.SetVar ("Selected", 4)
	else:
		GemRB.SetVar ("Selected", 0)

	GemRB.SetVarAssoc (CharSoundWindow, VoiceList, "Selected", 0)
	RowCount=GemRB.GetCharSounds(CharSoundWindow, VoiceList)

	PlayButton = GemRB.GetControl (CharSoundWindow, 47)
	GemRB.SetButtonState (CharSoundWindow, PlayButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent (CharSoundWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetText (CharSoundWindow, PlayButton, 17318)

	TextArea = GemRB.GetControl (CharSoundWindow, 50)
	GemRB.SetText (CharSoundWindow, TextArea, 11315)

	BackButton = GemRB.GetControl(CharSoundWindow,10)
	GemRB.SetText(CharSoundWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(CharSoundWindow,0)
	GemRB.SetText(CharSoundWindow,DoneButton,11973)
	GemRB.SetButtonFlags(CharSoundWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent(CharSoundWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(CharSoundWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(CharSoundWindow,1)
	return

def PlayPress():
	global CharSoundWindow

	CharSound = GemRB.QueryText(CharSoundWindow, VoiceList)
	GemRB.PlaySound (CharSound+"a")
	return

def BackPress():
	global CharSoundWindow

	GemRB.UnloadWindow(CharSoundWindow)
	GemRB.SetNextScript("GUICG13") 
	return

def NextPress():
	global CharSoundWindow

	CharSound = GemRB.QueryText (CharSoundWindow, VoiceList)
	GemRB.SetToken ("CharSound", CharSound)
	GemRB.UnloadWindow(CharSoundWindow)
	GemRB.SetNextScript("CharGen8") #name
	return
