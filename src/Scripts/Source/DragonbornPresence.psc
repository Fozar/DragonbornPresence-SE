Scriptname DragonbornPresence

Function UpdatePresenceData(String currentPosition, String currentPlayerInfo) Global Native

Function SetGameLoaded() Global Native

String Function GetCurrentPosition(String currentWorldspaceName, String currentLocationName, String currentCellName) Global
  If(currentWorldspaceName)
    If(currentLocationName)
      If(currentWorldspaceName == currentLocationName)
        Return currentWorldspaceName
      Else
        Return currentWorldspaceName + ": " + currentLocationName
      EndIf
    Else
      Return currentWorldspaceName
    EndIf
  ElseIf(currentLocationName)
    If(currentCellName)
      If(currentLocationName == currentCellName)
        Return currentLocationName
      Else
        Return currentLocationName + ": " + currentCellName
      EndIf
    Else
      Return currentLocationName
    EndIf
  Else
    Return currentCellName
  EndIf
EndFunction

String Function GetPlayerInfo(Actor playerRef) Global
  Return playerRef.GetBaseObject().GetName() + " - " + playerRef.GetRace().GetName() + " (" + playerRef.GetLevel() + ")"
EndFunction
