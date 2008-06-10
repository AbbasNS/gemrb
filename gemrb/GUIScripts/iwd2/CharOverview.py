# Some small attempt to re-use code in character generation.
# Attempting to emulate lynx's loop method from BG2 GUI

# Should be called CharGenCommon for continuity but I started
# this with something else in mind and then changed it
# - Fyorl

# CharOverview.py (GUICG)

import GemRB
from GUIDefines import *

CharGenWindow = 0
TextAreaControl = 0
StartOverWindow = 0
PortraitButton = 0
StepButtons = {}
PersistButtons = {}
Steps = ['Gender', 'Race', 'Class', 'Alignment', 'Abilities', 'Skills', 'Appearance', 'Name']
GlobalStep = 0

### Utility functions
def AddText(strref, row = False):
	if row: return GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, strref, row);
	return GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, strref)
### End utility functions

def UpdateOverview(CurrentStep):
	global CharGenWindow, TextAreaControl, StartOverWindow, PortraitButton
	global StepButtons, Steps, PersistButtons, GlobalStep
	
	GlobalStep = CurrentStep
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	CharGenWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame(CharGenWindow)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	
	# Handle portrait
	PortraitName = GemRB.GetToken('LargePortrait')
	if PortraitName != '' and CurrentStep > 1:
		GemRB.SetButtonPicture(CharGenWindow, PortraitButton, PortraitName, 'NOPORTLG')
	
	# Handle step buttons
	TextLookup = [11956, 11957, 11959, 11958, 11960, 11983, 11961, 11963]
	for i, Step in enumerate(Steps):
		StepButtons[Step] = GemRB.GetControl(CharGenWindow, i)
		GemRB.SetText(CharGenWindow, StepButtons[Step], TextLookup[i])
		State = IE_GUI_BUTTON_DISABLED
		if CurrentStep - 1 == i:
			State = IE_GUI_BUTTON_ENABLED
			GemRB.SetButtonFlags(CharGenWindow, StepButtons[Step], IE_GUI_BUTTON_DEFAULT, OP_OR)
			GemRB.SetEvent(CharGenWindow, StepButtons[Step], IE_GUI_BUTTON_ON_PRESS, 'NextPress')
		GemRB.SetButtonState(CharGenWindow, StepButtons[Step], State)
	
	# Handle (not so) persistent buttons
	# This array handles all the default values for the buttons
	# Exceptions are handled within the loop
	ControlLookup = {
		'Bio': [16, 18003, 0, 0],
		'Import': [13, 13955, 0, 0],
		'Back': [11, 15416, 1, 'BackPress'],
		'Next': [8, 28210, 0, 0],
		'Start': [15, 36788, 1, 'StartOver']
	}
	States = [IE_GUI_BUTTON_DISABLED, IE_GUI_BUTTON_ENABLED]
	for Key in ControlLookup:
		PersistButtons[Key] = GemRB.GetControl(CharGenWindow, ControlLookup[Key][0])
		Text = ControlLookup[Key][1]
		State = States[ControlLookup[Key][2]]
		Event = ControlLookup[Key][3]
		
		if Key == 'Bio' and CurrentStep == 9:
			State = States[1]
			Event = 'BioPress'
		
		if Key == 'Import' and CurrentStep == 1:
			State = States[1]
			Event = 'ImportPress'
		
		if Key == 'Back' and CurrentStep == 1:
			State = States[0]
			Event = 0
		
		if Key == 'Next' and CurrentStep == 9:
			Text = 11962
			State = 1
			Event = 'NextPress'
		
		if Key == 'Start' and CurrentStep == 1:
			Text = 13727
			Event = 'CancelPress'
		
		GemRB.SetText(CharGenWindow, PersistButtons[Key], Text)
		GemRB.SetButtonState(CharGenWindow, PersistButtons[Key], State)
		
		if Event:
			GemRB.SetEvent(CharGenWindow, PersistButtons[Key], IE_GUI_BUTTON_ON_PRESS, Event)
	
	# Handle character overview information
	TextAreaControl = GemRB.GetControl(CharGenWindow, 9)
	Tables = []
	for tbl in ['races', 'classes', 'aligns', 'ability', 'skillsta', 'skills', 'featreq', 'feats']:
		Tables.append(GemRB.LoadTable(tbl))
	
	if GemRB.GetVar('Gender') > 0:
		if GemRB.GetToken('CHARNAME') == '':
			GemRB.SetText(CharGenWindow, TextAreaControl, 12135)
		else:
			GemRB.SetText(CharGenWindow, TextAreaControl, 1047)
			AddText(': ' + GemRB.GetToken('CHARNAME'))
			AddText(12135, -1)
		AddText(': ')
		strref = 1049 + GemRB.GetVar('Gender')
		AddText(strref)
	
	if GemRB.GetVar('Race') > 0:
		AddText(1048, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[0], GemRB.FindTableValue(Tables[0], 3, GemRB.GetVar('Race')), 2))
	
	if GemRB.GetVar('Class') > 0:
		AddText(11959, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[1], GemRB.GetVar('Class') - 1, 0))
	
	if GemRB.GetVar('Alignment') > 0:
		AddText(11958, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[2], GemRB.GetVar('Alignment') - 1, 0))
	
	if GemRB.GetVar('Ability 0') > 0:
		AddText('\n[color=FFFF00]', -1)
		AddText(17088)
		AddText('[/color]')
		for i in range(0, 6):
			strref = GemRB.GetTableValue(Tables[3], i, 2)
			AddText(strref, -1)
			abl = GemRB.GetVar('Ability ' + str(i))
			AddText(': %d (%+d)' % (abl, abl / 2 - 5))
	
	if CurrentStep > 6:
		AddText('\n[color=FFFF00]', -1)
		AddText(11983)
		AddText('[/color]')
		
		ClassColumn = GemRB.GetTableValue(Tables[1], GemRB.GetVar('Class') - 1, 3, 1) # Finds base class row id
		if ClassColumn < 1: ClassColumn = GemRB.GetVar('Class') - 1; # If 0 then already a base class so need actual row
		else: ClassColumn -= 1; # 'CLASS' column in classes.2da is out by 1 for some reason
		ClassColumn += 4 # There are 4 columns before the classes in skills.2da
		# At the moment only cleric kits get skill bonuses but their column names in skills.2da don't match up
		# to their kit names. All classes aren't covered in skills.2da either which is why I have to resort
		# to calculating the base class. This isn't ideal. Recommend a new 2DA be created with *all* classes
		# as rows and skills as columns. Something like SKILCLAS.2DA
		
		### Cleric kit hack:
		if GemRB.GetVar('Class') in range(27, 36):
			ClassColumn = GemRB.GetVar('Class') - 12
		
		RaceName = GemRB.GetTableRowName(Tables[0], GemRB.FindTableValue(Tables[0], 3, GemRB.GetVar('Race')))
		SkillColumn = GemRB.GetTableValue(Tables[0], RaceName, 'SKILL_COLUMN', 1) + 1
		Lookup = {'STR': 0, 'DEX': 1, 'CON': 2, 'INT': 3, 'WIS': 4, 'CHA': 5} # Probably a better way to do this
		for i in range(GemRB.GetTableRowCount(Tables[4])):
			SkillName = GemRB.GetTableRowName(Tables[5], i)
			Abl = GemRB.GetTableValue(Tables[4], i, 1, 0)
			Ranks = GemRB.GetVar('Skill ' + str(i))
			value = Ranks
			value += (GemRB.GetVar('Ability ' + str(Lookup[Abl])) / 2 - 5)
			value += GemRB.GetTableValue(Tables[5], i, SkillColumn, 1)
			value += GemRB.GetTableValue(Tables[5], i, ClassColumn, 1)
			
			untrained = GemRB.GetTableValue(Tables[5], i, 3, 1)
			if not untrained and Ranks < 1:
				value = 0
			
			if value:
				strref = GemRB.GetTableValue(Tables[5], i, 1)
				AddText(strref, -1)
				strn = ': ' + str(value)
				if value != Ranks: strn += ' (' + str(Ranks) + ')'
				AddText(strn)

		AddText('\n[color=FFFF00]', -1)
		AddText(36310)
		AddText('[/color]')
		
		for i in range(GemRB.GetTableRowCount(Tables[6])):
			value = GemRB.GetVar('Feat ' + str(i))
			if value:
				strref = GemRB.GetTableValue(Tables[7], i, 1)
				AddText(strref, -1)
				if value > 1: AddText(': ' + str(value))

	# Cleanup tables
	for tbl in Tables:
		GemRB.UnloadTable(tbl)
	
	# Handle StartOverWindow
	StartOverWindow = GemRB.LoadWindow(53)
	
	YesButton = GemRB.GetControl(StartOverWindow, 0)
	GemRB.SetText(StartOverWindow, YesButton, 13912)
	GemRB.SetEvent(StartOverWindow, YesButton, IE_GUI_BUTTON_ON_PRESS, 'RestartGen')
	
	NoButton = GemRB.GetControl(StartOverWindow, 1)
	GemRB.SetText(StartOverWindow, NoButton, 13913)
	GemRB.SetEvent(StartOverWindow, NoButton, IE_GUI_BUTTON_ON_PRESS, 'NoExitPress')
	
	TextAreaControl = GemRB.GetControl(StartOverWindow, 2)
	GemRB.SetText(StartOverWindow, TextAreaControl, 40275)
	
	# And we're done, w00t!
	GemRB.SetVisible(CharGenWindow, 1)
	return

def NextPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript(Steps[GlobalStep - 1])
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript('SPPartyFormation')
	return

def StartOver():
	GemRB.SetVisible(StartOverWindow, 1)
	return

def NoExitPress():
	GemRB.SetVisible(StartOverWindow, 0)
	GemRB.SetVisible(CharGenWindow, 1)
	return

def ImportPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetToken('NextScript', 'CharGen')
	GemRB.SetNextScript('ImportFile')
	return

def RestartGen():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript('CharGen')
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	
	# Need to clear relevant variables
	if GlobalStep == 2: GemRB.SetVar('Gender', 0);
	elif GlobalStep == 3: GemRB.SetVar('Race', 0);
	elif GlobalStep == 4: GemRB.SetVar('Class', 0);
	elif GlobalStep == 5: GemRB.SetVar('Alignment', 0);
	elif GlobalStep == 6:
		for i in range(0, 6): GemRB.SetVar('Ability ' + str(i), 0);
	elif GlobalStep == 9: GemRB.SetToken('CHARNAME', '')
	
	ScrName = 'CharGen' + str(GlobalStep - 1)
	if GlobalStep == 2: ScrName = 'CharGen';
	GemRB.SetNextScript(ScrName)
	return
# Some small attempt to re-use code in character generation.
# Attempting to emulate lynx's loop method from BG2 GUI

# Should be called CharGenCommon for continuity but I started
# this with something else in mind and then changed it
# - Fyorl

# CharOverview.py (GUICG)

import GemRB
from GUIDefines import *

CharGenWindow = 0
TextAreaControl = 0
StartOverWindow = 0
PortraitButton = 0
StepButtons = {}
PersistButtons = {}
Steps = ['Gender', 'Race', 'Class', 'Alignment', 'Abilities', 'Skills', 'Appearance', 'Name']
GlobalStep = 0

### Utility functions
def AddText(strref, row = False):
	if row: return GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, strref, row);
	return GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, strref)
### End utility functions

def UpdateOverview(CurrentStep):
	global CharGenWindow, TextAreaControl, StartOverWindow, PortraitButton
	global StepButtons, Steps, PersistButtons, GlobalStep
	
	GlobalStep = CurrentStep
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	CharGenWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame(CharGenWindow)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	
	# Handle portrait
	PortraitName = GemRB.GetToken('LargePortrait')
	if PortraitName != '' and CurrentStep > 1:
		GemRB.SetButtonPicture(CharGenWindow, PortraitButton, PortraitName, 'NOPORTLG')
	
	# Handle step buttons
	TextLookup = [11956, 11957, 11959, 11958, 11960, 11983, 11961, 11963]
	for i, Step in enumerate(Steps):
		StepButtons[Step] = GemRB.GetControl(CharGenWindow, i)
		GemRB.SetText(CharGenWindow, StepButtons[Step], TextLookup[i])
		State = IE_GUI_BUTTON_DISABLED
		if CurrentStep - 1 == i:
			State = IE_GUI_BUTTON_ENABLED
			GemRB.SetButtonFlags(CharGenWindow, StepButtons[Step], IE_GUI_BUTTON_DEFAULT, OP_OR)
			GemRB.SetEvent(CharGenWindow, StepButtons[Step], IE_GUI_BUTTON_ON_PRESS, 'NextPress')
		GemRB.SetButtonState(CharGenWindow, StepButtons[Step], State)
	
	# Handle (not so) persistent buttons
	# This array handles all the default values for the buttons
	# Exceptions are handled within the loop
	ControlLookup = {
		'Bio': [16, 18003, 0, 0],
		'Import': [13, 13955, 0, 0],
		'Back': [11, 15416, 1, 'BackPress'],
		'Next': [8, 28210, 0, 0],
		'Start': [15, 36788, 1, 'StartOver']
	}
	States = [IE_GUI_BUTTON_DISABLED, IE_GUI_BUTTON_ENABLED]
	for Key in ControlLookup:
		PersistButtons[Key] = GemRB.GetControl(CharGenWindow, ControlLookup[Key][0])
		Text = ControlLookup[Key][1]
		State = States[ControlLookup[Key][2]]
		Event = ControlLookup[Key][3]
		
		if Key == 'Bio' and CurrentStep == 9:
			State = States[1]
			Event = 'BioPress'
		
		if Key == 'Import' and CurrentStep == 1:
			State = States[1]
			Event = 'ImportPress'
		
		if Key == 'Back' and CurrentStep == 1:
			State = States[0]
			Event = 0
		
		if Key == 'Next' and CurrentStep == 9:
			Text = 11962
			State = 1
			Event = 'NextPress'
		
		if Key == 'Start' and CurrentStep == 1:
			Text = 13727
			Event = 'CancelPress'
		
		GemRB.SetText(CharGenWindow, PersistButtons[Key], Text)
		GemRB.SetButtonState(CharGenWindow, PersistButtons[Key], State)
		
		if Event:
			GemRB.SetEvent(CharGenWindow, PersistButtons[Key], IE_GUI_BUTTON_ON_PRESS, Event)
	
	# Handle character overview information
	TextAreaControl = GemRB.GetControl(CharGenWindow, 9)
	Tables = []
	for tbl in ['races', 'classes', 'aligns', 'ability', 'skillsta', 'skills', 'featreq', 'feats']:
		Tables.append(GemRB.LoadTable(tbl))
	
	if GemRB.GetVar('Gender') > 0:
		if GemRB.GetToken('CHARNAME') == '':
			GemRB.SetText(CharGenWindow, TextAreaControl, 12135)
		else:
			GemRB.SetText(CharGenWindow, TextAreaControl, 1047)
			AddText(': ' + GemRB.GetToken('CHARNAME'))
			AddText(12135, -1)
		AddText(': ')
		strref = 1049 + GemRB.GetVar('Gender')
		AddText(strref)
	
	if GemRB.GetVar('Race') > 0:
		AddText(1048, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[0], GemRB.FindTableValue(Tables[0], 3, GemRB.GetVar('Race')), 2))
	
	if GemRB.GetVar('Class') > 0:
		AddText(11959, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[1], GemRB.GetVar('Class') - 1, 0))
	
	if GemRB.GetVar('Alignment') > 0:
		AddText(11958, -1)
		AddText(': ')
		AddText(GemRB.GetTableValue(Tables[2], GemRB.GetVar('Alignment') - 1, 0))
	
	if GemRB.GetVar('Ability 0') > 0:
		AddText('\n[color=FFFF00]', -1)
		AddText(17088)
		AddText('[/color]')
		for i in range(0, 6):
			strref = GemRB.GetTableValue(Tables[3], i, 2)
			AddText(strref, -1)
			abl = GemRB.GetVar('Ability ' + str(i))
			AddText(': %d (%+d)' % (abl, abl / 2 - 5))
	
	if CurrentStep > 6:
		AddText('\n[color=FFFF00]', -1)
		AddText(11983)
		AddText('[/color]')
		
		ClassColumn = GemRB.GetTableValue(Tables[1], GemRB.GetVar('Class') - 1, 3, 1) # Finds base class row id
		if ClassColumn < 1: ClassColumn = GemRB.GetVar('Class') - 1; # If 0 then already a base class so need actual row
		else: ClassColumn -= 1; # 'CLASS' column in classes.2da is out by 1 for some reason
		ClassColumn += 4 # There are 4 columns before the classes in skills.2da
		# At the moment only cleric kits get skill bonuses but their column names in skills.2da don't match up
		# to their kit names. All classes aren't covered in skills.2da either which is why I have to resort
		# to calculating the base class. This isn't ideal. Recommend a new 2DA be created with *all* classes
		# as rows and skills as columns. Something like SKILCLAS.2DA
		
		### Cleric kit hack:
		if GemRB.GetVar('Class') in range(27, 36):
			ClassColumn = GemRB.GetVar('Class') - 12
		
		RaceName = GemRB.GetTableRowName(Tables[0], GemRB.FindTableValue(Tables[0], 3, GemRB.GetVar('Race')))
		SkillColumn = GemRB.GetTableValue(Tables[0], RaceName, 'SKILL_COLUMN', 1) + 1
		Lookup = {'STR': 0, 'DEX': 1, 'CON': 2, 'INT': 3, 'WIS': 4, 'CHA': 5} # Probably a better way to do this
		for i in range(GemRB.GetTableRowCount(Tables[4])):
			SkillName = GemRB.GetTableRowName(Tables[5], i)
			Abl = GemRB.GetTableValue(Tables[4], i, 1, 0)
			Ranks = GemRB.GetVar('Skill ' + str(i))
			value = Ranks
			value += (GemRB.GetVar('Ability ' + str(Lookup[Abl])) / 2 - 5)
			value += GemRB.GetTableValue(Tables[5], i, SkillColumn, 1)
			value += GemRB.GetTableValue(Tables[5], i, ClassColumn, 1)
			
			untrained = GemRB.GetTableValue(Tables[5], i, 3, 1)
			if not untrained and Ranks < 1:
				value = 0
			
			if value:
				strref = GemRB.GetTableValue(Tables[5], i, 1)
				AddText(strref, -1)
				strn = ': ' + str(value)
				if value != Ranks: strn += ' (' + str(Ranks) + ')'
				AddText(strn)

		AddText('\n[color=FFFF00]', -1)
		AddText(36310)
		AddText('[/color]')
		
		for i in range(GemRB.GetTableRowCount(Tables[6])):
			value = GemRB.GetVar('Feat ' + str(i))
			if value:
				strref = GemRB.GetTableValue(Tables[7], i, 1)
				AddText(strref, -1)
				if value > 1: AddText(': ' + str(value))

	# Cleanup tables
	for tbl in Tables:
		GemRB.UnloadTable(tbl)
	
	# Handle StartOverWindow
	StartOverWindow = GemRB.LoadWindow(53)
	
	YesButton = GemRB.GetControl(StartOverWindow, 0)
	GemRB.SetText(StartOverWindow, YesButton, 13912)
	GemRB.SetEvent(StartOverWindow, YesButton, IE_GUI_BUTTON_ON_PRESS, 'RestartGen')
	
	NoButton = GemRB.GetControl(StartOverWindow, 1)
	GemRB.SetText(StartOverWindow, NoButton, 13913)
	GemRB.SetEvent(StartOverWindow, NoButton, IE_GUI_BUTTON_ON_PRESS, 'NoExitPress')
	
	TextAreaControl = GemRB.GetControl(StartOverWindow, 2)
	GemRB.SetText(StartOverWindow, TextAreaControl, 40275)
	
	# And we're done, w00t!
	GemRB.SetVisible(CharGenWindow, 1)
	return

def NextPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript(Steps[GlobalStep - 1])
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript('SPPartyFormation')
	return

def StartOver():
	GemRB.SetVisible(StartOverWindow, 1)
	return

def NoExitPress():
	GemRB.SetVisible(StartOverWindow, 0)
	GemRB.SetVisible(CharGenWindow, 1)
	return

def ImportPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetToken('NextScript', 'CharGen')
	GemRB.SetNextScript('ImportFile')
	return

def RestartGen():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript('CharGen')
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	
	# Need to clear relevant variables
	if GlobalStep == 2: GemRB.SetVar('Gender', 0);
	elif GlobalStep == 3: GemRB.SetVar('Race', 0);
	elif GlobalStep == 4: GemRB.SetVar('Class', 0);
	elif GlobalStep == 5: GemRB.SetVar('Alignment', 0);
	elif GlobalStep == 6:
		for i in range(0, 6): GemRB.SetVar('Ability ' + str(i), 0);
	elif GlobalStep == 9: GemRB.SetToken('CHARNAME', '')
	
	ScrName = 'CharGen' + str(GlobalStep - 1)
	if GlobalStep == 2: ScrName = 'CharGen';
	GemRB.SetNextScript(ScrName)
	return