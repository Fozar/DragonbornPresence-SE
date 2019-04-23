Scriptname FZR_TrackInSameCell Extends ActiveMagicEffect
{ability that will be activated when player is not in the same cell as invisibleObject}

Actor Property playerRef Auto
ObjectReference Property invisibleObject Auto
FZR_PlayerReferenceLoadScript Property playerRefScript Auto

Event OnEffectStart(Actor akTarget, Actor akCaster)
    Utility.Wait(0.1)
    invisibleObject.MoveTo(playerRef)
    playerRefScript.UpdatePosition()
EndEvent
