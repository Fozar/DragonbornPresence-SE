#include "tinyfsm.hpp"
#include "DragonbornPresence.h"
#include "skse64/GameMenus.h"
#include "AdditionalFunctions.h"
#include <ctime>


bool is_user_connected = true;

int64_t start_time;

const char * application_id = "565627104608256015";
const char * steam_appid = "72850";

const char * current_player_info;

const char * current_position;

std::map<const char*, std::string> locales
{
  {
    {"MainMenu", "Main menu"}, {"EditingCharacter", "Editing character"}
  }
};


#pragma region State Machine

#pragma region Event Declaration
struct StatusEvent : tinyfsm::Event {
};

struct GoToMainMenu : StatusEvent {
};
struct GoToLoading : StatusEvent {
};
struct GoToEditingCharacter : StatusEvent {
};
struct GoToPlaying : StatusEvent {
};
#pragma endregion

#pragma region State Machine Class Declaration
struct DiscordState : tinyfsm::Fsm<DiscordState> {
  static void react(tinyfsm::Event const &) {
  };

  virtual void react(GoToMainMenu const &) {
  };

  virtual void react(GoToLoading const &) {
  };

  virtual void react(GoToEditingCharacter const &) {
  };

  virtual void react(GoToPlaying const &) {
  };

  virtual void entry() {
  };

  virtual void exit() {
  };
};
#pragma endregion

#pragma region States Declaration
struct MainMenuState : DiscordState {
  void entry() override;
  void react(GoToLoading const &) override;
};

struct LoadingState : DiscordState {
  void entry() override;
  void react(GoToMainMenu const &) override;
  void react(GoToEditingCharacter const &) override;
  void react(GoToPlaying const &) override;
};

struct EditingCharacterState : DiscordState {
  void entry() override;
  void react(GoToPlaying const &) override;
};

struct PlayingState : DiscordState {
  void entry() override;
  void react(GoToEditingCharacter const &) override;
  void react(GoToLoading const &) override;
  void react(GoToMainMenu const &) override;
};

FSM_INITIAL_STATE(DiscordState, LoadingState)
#pragma endregion

#pragma region Actions and Event Reactions Implemention
void MainMenuState::entry() {
  _MESSAGE("Main Menu State");
  dragonborn_presence_namespace::UpdatePresence(
    locales.find("MainMenu")->second.c_str(), nullptr);
}

void MainMenuState::react(GoToLoading const &) { transit<LoadingState>(); }

void LoadingState::entry() { _MESSAGE("Loading State"); }

void LoadingState::react(GoToMainMenu const &) { transit<MainMenuState>(); }

void LoadingState::react(GoToEditingCharacter const &) {
  transit<EditingCharacterState>();
}

void LoadingState::react(GoToPlaying const &) { transit<PlayingState>(); }

void EditingCharacterState::entry() {
  _MESSAGE("Editing Character State");
  dragonborn_presence_namespace::UpdatePresence(
    locales.find("EditingCharacter")->second.c_str(), nullptr);
}

void EditingCharacterState::react(GoToPlaying const &) {
  transit<PlayingState>();
}

void PlayingState::entry() {
  _MESSAGE("Playing State");
  dragonborn_presence_namespace::UpdatePresence(current_position,
                                                current_player_info);
}

void PlayingState::react(GoToEditingCharacter const &) {
  transit<EditingCharacterState>();
}

void PlayingState::react(GoToLoading const &) { transit<LoadingState>(); }

void PlayingState::react(GoToMainMenu const &) { transit<MainMenuState>(); }
#pragma endregion

#pragma endregion

namespace dragonborn_presence_namespace {

  void SetLocale() {
    FILE * file = nullptr;
    const auto err = fopen_s(&file,
                             R"(Data\SKSE\Plugins\DragonbornPresenceLocale.txt)",
                             "r");

    if (err == 2 || file == nullptr) // Missing file.
      return;

    if (err != 0) // Unknown error code.
      return;

    char buffer[1024];
    auto index = 0;
    while (fgets(buffer, 1024, file) != nullptr) {
      std::string line = buffer;
      while (!line.empty() && (line[line.length() - 1] == '\r' || line[line.
        length() - 1] == '\n'))
        line = line.substr(0, line.length() - 1);

      if (line.empty())
        continue;

      if (line[0] == ';')
        continue;

      if (index == 0) { locales.find("MainMenu")->second = line; } else if (
        index == 1) { locales.find("EditingCharacter")->second = line; }

      index++;
    }

    fclose(file);
  }

#pragma region Discord Callbacks

  //Calls when Discord RPC is successfully connected to user.
  void HandleDiscordReady(const DiscordUser * connected_user) {
    _MESSAGE("Discord RPC: connected to user %s#%s - %s",
             connected_user->username, connected_user->discriminator,
             connected_user->userId);
    is_user_connected = true;
  }

  //Calls when discord reported an error.
  void HandleDiscordError(const int error_code, const char * message) {
    _MESSAGE("Discord RPC: an error occured (%d: %s)", error_code, message);
  }


  //Calls when Discord RPC is disconnected from user.
  void HandleDiscordDisconnected(const int error_code, const char * message) {
    _MESSAGE("Discord RPC: disconnected (%d: %s)", error_code, message);
    is_user_connected = false;
  }

#pragma endregion

#pragma region Discord Functions
  //Initializes Discord RPC. Sets the timer, counting down the time from the
  //start of the game. Assigns functions to be called when callbacks. 
  void InitDiscord() {
    start_time = time(nullptr);
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = HandleDiscordReady;
    handlers.errored = HandleDiscordError;
    handlers.disconnected = HandleDiscordDisconnected;

    Discord_Initialize(application_id, &handlers, 1, steam_appid);
    _MESSAGE("Discord RPC Init OK.");
  }

  //Updates rich presence in Discord.
  void UpdatePresence(const char * current_state,
                      const char * current_details) {
    if (is_user_connected) {
      DiscordRichPresence discord_presence;
      memset(&discord_presence, 0, sizeof(discord_presence));
      discord_presence.state = current_state;
      discord_presence.details = current_details;
      discord_presence.startTimestamp = start_time;
      discord_presence.largeImageKey = "skyrim_logo";
      discord_presence.largeImageText = "The Elder Scrolls V: Skyrim";
      Discord_UpdatePresence(&discord_presence);

      Discord_RunCallbacks();

      _MESSAGE("Updated presence.");
    }
  }
#pragma endregion

#pragma region ReceiveEvent

  EventResult DiscordMenuEventHandler::ReceiveEvent(
    MenuOpenCloseEvent * evn,
    EventDispatcher<MenuOpenCloseEvent> * dispatcher) {
    if (evn->opening && evn->menuName == BSFixedString("Main Menu")) {
      DiscordState::dispatch(GoToMainMenu());
    } else if (evn->menuName == BSFixedString("Loading Menu")) {
      if (evn->opening)
        DiscordState::dispatch(GoToLoading());
      else
        DiscordState::dispatch(GoToPlaying());
    } else if (evn->menuName == BSFixedString("RaceSex Menu")) {
      if (evn->opening)
        DiscordState::dispatch(GoToEditingCharacter());
      else
        DiscordState::dispatch(GoToPlaying());
    }

    return kEvent_Continue;
  }
#pragma endregion

#pragma region Event Handlers
  DiscordMenuEventHandler g_discordMenuEventHandler;
#pragma endregion

#pragma region Event registration
  void RegisterGameEventHandlers() {
    _MESSAGE("Registering game event handlers...");

    DiscordState::start();

    auto mm = MenuManager::GetSingleton();
    if (mm) {
      mm->MenuOpenCloseEventDispatcher()->AddEventSink(
        &g_discordMenuEventHandler);
    } else { _MESSAGE("Failed to register SKSE menuEventHandler!"); }
  }
#pragma endregion

#pragma region Papyrus functions
  void UpdatePresenceData(StaticFunctionTag * base, BSFixedString new_position,
                          BSFixedString new_player_info) {
    _MESSAGE("CURRENT POSITION: %s", new_position.data);
    current_position = is_valid_utf8(new_position.data)
                         ? new_position.data
                         : Cp1251ToUtf8(new_position.data);
    current_player_info = is_valid_utf8(new_player_info.data)
                            ? new_player_info.data
                            : Cp1251ToUtf8(new_player_info.data);
    if (DiscordState::is_in_state<PlayingState>()) {
      UpdatePresence(current_position, current_player_info);
    }
  }

  void SetGameLoaded(StaticFunctionTag * base) {
    DiscordState::dispatch(GoToPlaying());
	  UpdatePresence(current_position, current_player_info);
  }


  bool RegisterFuncs(VMClassRegistry * registry) {
    registry->RegisterFunction(
      new NativeFunction2<StaticFunctionTag, void, BSFixedString, BSFixedString>
      ("UpdatePresenceData", "DragonbornPresence", UpdatePresenceData,
       registry));

    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>(
      "SetGameLoaded", "DragonbornPresence", SetGameLoaded, registry));

    InitDiscord();

    return true;
  }
#pragma endregion
}
