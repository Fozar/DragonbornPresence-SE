Scriptname FZR_PlayerReferenceLoadScript extends ReferenceAlias

Import DragonbornPresence

String currentCellName
String currentLocationName
String currentWorldspaceName
String currentPosition

Function UpdatePosition()
  Actor playerRef = GetActorReference()
  currentCellName = playerRef.GetParentCell().GetName()
  currentLocationName = playerRef.GetCurrentLocation().GetName()
  currentWorldspaceName = playerRef.GetWorldSpace().GetName()
  currentPosition = GetCurrentPosition(currentWorldspaceName as String, currentLocationName as String, currentCellName as String)
  UpdatePresenceData(currentPosition as String, GetPlayerInfo(playerRef) as String)
EndFunction

Event OnPlayerLoadGame()
    Utility.Wait(0.1)
    UpdatePosition()
    SetGameLoaded()
EndEvent
