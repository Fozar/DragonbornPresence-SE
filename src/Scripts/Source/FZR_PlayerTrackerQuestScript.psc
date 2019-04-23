ScriptName FZR_PlayerTrackerQuestScript Extends Quest

Import DragonbornPresence
FZR_PlayerReferenceLoadScript Property playerRefScript Auto

Event OnInit()
	Utility.Wait(0.1)
	playerRefScript.UpdatePosition()
	SetGameLoaded()
EndEvent
