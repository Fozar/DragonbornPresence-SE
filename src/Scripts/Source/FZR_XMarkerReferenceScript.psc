Scriptname FZR_XMarkerReferenceScript Extends ObjectReference

Actor property playerRef Auto
FZR_PlayerReferenceLoadScript Property playerRefScript Auto

Event OnCellDetach()
    Utility.Wait(0.1)
    MoveTo(playerRef)
    playerRefScript.UpdatePosition()
EndEvent
