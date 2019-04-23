#pragma once
#include "skse64/GameEvents.h"
#include "skse64/PapyrusNativeFunctions.h"
#include "discord_rpc.h"


namespace dragonborn_presence_namespace {

  void SetLocale();

#pragma region Discord Callbacks
  void HandleDiscordReady(const DiscordUser * connected_user);

  void HandleDiscordError(const int error_code, const char * message);

  void HandleDiscordDisconnected(const int error_code, const char * message);
#pragma endregion

#pragma region Discord Functions
  void InitDiscord();
  void UpdatePresence(const char * current_state, const char * current_details);
#pragma endregion

#pragma region Handler classes
  class DiscordMenuEventHandler : public BSTEventSink<MenuOpenCloseEvent> {
  public:
    EventResult ReceiveEvent(MenuOpenCloseEvent * evn,
                             EventDispatcher<MenuOpenCloseEvent> * dispatcher);
  };
#pragma endregion

#pragma region Event Handlers
  extern DiscordMenuEventHandler g_discordMenuEventHandler;

  void RegisterGameEventHandlers();
#pragma endregion

#pragma region Papyrus functions
  void UpdatePresenceData(StaticFunctionTag * base, BSFixedString new_position,
                          BSFixedString new_player_info);

  void SetGameLoaded(StaticFunctionTag * base);

  bool RegisterFuncs(VMClassRegistry * registry);
#pragma endregion
}
