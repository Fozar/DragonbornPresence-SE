#include "DragonbornPresence.h"
#include "AdditionalFunctions.h"
#include "discord.h"
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

namespace DragonbornPresence {

namespace {

constexpr discord::ClientId kAppId          = 565627104608256015LL;
constexpr const char*       kLargeImageKey  = "skyrim_logo";
constexpr const char*       kLargeImageText = "The Elder Scrolls V: Skyrim";

enum class State { Loading, MainMenu, EditingCharacter, Playing };

State          g_state             = State::Loading;
discord::Core* g_core              = nullptr;
int64_t        g_startTime         = 0;
std::unordered_map<std::string, std::string> g_locale = {
    {"main_menu",         "Main menu"},
    {"editing_character", "Editing character"},
    {"combat_fighting",   "In combat with {name}"},
    {"combat_no_target",  "In combat"},
};
std::string    g_lastPosition;
std::string    g_combatTarget;

struct Config {
    bool show_location    = true;
    bool show_quest       = true;
    bool show_combat      = true;
    bool show_player_info = true;
} g_config;

static std::string SafeStr(const char* s) {
    if (!s || *s == '\0') return "";
    return IsValidUtf8(s) ? std::string(s) : Cp1251ToUtf8(s);
}

static const std::string& Locale(const std::string& key) {
    static const std::string kFallback;
    auto it = g_locale.find(key);
    return it != g_locale.end() ? it->second : kFallback;
}

// Formats the combat string for a known enemy name.
// If the template contains {name}, replaces it; otherwise appends " " + name (legacy fallback).
static std::string FormatCombat(const std::string& tmpl, const std::string& name) {
    constexpr std::string_view kPlaceholder = "{name}";
    auto pos = tmpl.find(kPlaceholder);
    if (pos == std::string::npos)
        return tmpl + " " + name;
    std::string result = tmpl;
    result.replace(pos, kPlaceholder.size(), name);
    return result;
}

static std::string BuildPosition(RE::PlayerCharacter* player) {
    auto* ws   = player->GetWorldspace();
    auto* loc  = player->GetCurrentLocation();
    auto* cell = player->GetParentCell();

    std::string locName;
    for (auto* l = loc; l && locName.empty(); l = l->parentLoc)
        locName = SafeStr(l->GetName());

    std::string wsName   = ws   ? SafeStr(ws->GetName())   : "";
    std::string cellName = cell ? SafeStr(cell->GetName()) : "";

if (!wsName.empty())
        return (!locName.empty() && locName != wsName) ? wsName + ": " + locName : wsName;
    if (!locName.empty()) return locName;
    return cellName;
}

static std::string BuildActiveQuest(RE::PlayerCharacter* player) {
    if (REL::Module::IsVR()) return "";  // objectives not mapped for VR

    // objectives sits at 0x580 from PlayerCharacter* in SE/older AE,
    // shifted +8 to 0x588 for AE 1.6.629+ (PLAYER_RUNTIME_DATA base moves from 0x3D8 → 0x3E0)
    auto& objectives = REL::RelocateMemberIfNewer<RE::BSTArray<RE::BGSInstancedQuestObjective>>(
        SKSE::RUNTIME_SSE_1_6_629, player, 0x580, 0x588);

    RE::TESQuest* best = nullptr;
    for (const auto& instObj : objectives) {
        if (instObj.InstanceState != RE::QUEST_OBJECTIVE_STATE::kDisplayed) continue;
        auto* quest = instObj.Objective ? instObj.Objective->ownerQuest : nullptr;
        if (!quest) continue;
        if (!best || quest->data.priority > best->data.priority)
            best = quest;
    }
    return best ? SafeStr(best->GetFullName()) : "";
}

static std::string BuildPlayerInfo(RE::PlayerCharacter* player) {
    auto* race = player->GetRace();
    // GetName() on the actor returns the display name (updated after char edit);
    // GetActorBase()->GetName() can lag behind by several frames.
    std::string name     = SafeStr(player->GetName());
    std::string raceName = race ? SafeStr(race->GetName()) : "";
    return name + " - " + raceName + " (" + std::to_string(player->GetLevel()) + ")";
}

void SendPresence(const char* state, const char* details) {
    if (!g_core) return;

    discord::Activity activity{};
    if (state   && *state)   activity.SetState(state);
    if (details && *details) activity.SetDetails(details);
    activity.GetTimestamps().SetStart(g_startTime);
    activity.GetAssets().SetLargeImage(kLargeImageKey);
    activity.GetAssets().SetLargeText(kLargeImageText);

    g_core->ActivityManager().UpdateActivity(activity, [](discord::Result r) {
        if (r != discord::Result::Ok)
            SKSE::log::error("Discord: UpdateActivity failed (result={})", static_cast<int>(r));
        else
            SKSE::log::info("Presence updated.");
    });
}

void RefreshPosition(const char* trigger = nullptr) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    std::string position;
    if (g_config.show_location) {
        position = BuildPosition(player);
        if (!position.empty()) g_lastPosition = position;
    }

    const bool fallback = g_config.show_location && position.empty() && !g_lastPosition.empty();
    const std::string& display = fallback ? g_lastPosition : position;
    std::string state = display;

    std::string suffix;
    if (!g_combatTarget.empty() && g_config.show_combat)
        suffix = g_combatTarget;
    else if (g_config.show_quest)
        suffix = BuildActiveQuest(player);

    if (!suffix.empty()) {
        if (!state.empty())
            state += " \xC2\xB7 " + suffix;  // · (U+00B7)
        else
            state = suffix;
    }

    std::string playerInfo = g_config.show_player_info ? BuildPlayerInfo(player) : "";

    SKSE::log::info("[{}] player='{}' location='{}' suffix='{}'{}",
        trigger ? trigger : "refresh", playerInfo, display, suffix,
        fallback ? " [fallback]" : "");
    SendPresence(state.c_str(), playerInfo.c_str());
}

void DeferredRefresh(int ticks) {
    SKSE::GetTaskInterface()->AddTask([ticks]() {
        if (ticks > 1)
            DeferredRefresh(ticks - 1);
        else if (g_state == State::Playing)
            RefreshPosition("post-charcreate");
    });
}

void TransitionTo(State next) {
    State prev = g_state;
    g_state = next;
    switch (g_state) {
    case State::MainMenu:
        SKSE::log::info("State -> MainMenu");
        SendPresence(Locale("main_menu").c_str(), nullptr);
        break;
    case State::EditingCharacter:
        SKSE::log::info("State -> EditingCharacter");
        SendPresence(Locale("editing_character").c_str(), nullptr);
        break;
    case State::Playing:
        SKSE::log::info("State -> Playing");
        if (prev == State::EditingCharacter) {
            // Skyrim commits new name/race to actor base after the menu-close
            // event fires — defer 10 ticks (~167ms at 60fps) to let it finish.
            DeferredRefresh(10);
        } else {
            RefreshPosition("state-change");
        }
        break;
    case State::Loading:
        SKSE::log::info("State -> Loading");
        g_combatTarget.clear();
        break;
    }
}

class MenuEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* ev,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
    {
        if (!ev) return RE::BSEventNotifyControl::kContinue;

        const bool  opening = ev->opening;
        const auto& menu    = ev->menuName;

        if (menu == "Main Menu") {
            SKSE::log::info("Menu: '{}' {}", menu.c_str(), opening ? "open" : "close");
            if (opening) TransitionTo(State::MainMenu);
        } else if (menu == "Loading Menu") {
            SKSE::log::info("Menu: '{}' {}", menu.c_str(), opening ? "open" : "close");
            if (opening) {
                TransitionTo(State::Loading);
            } else if (g_state == State::Loading) {
                TransitionTo(State::Playing);
            }
        } else if (menu == "RaceSex Menu") {
            SKSE::log::info("Menu: '{}' {}", menu.c_str(), opening ? "open" : "close");
            TransitionTo(opening ? State::EditingCharacter : State::Playing);
        } else if (menu == "Journal Menu") {
            if (!opening && g_state == State::Playing)
                RefreshPosition("journal-close");
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

class LocationChangeSink : public RE::BSTEventSink<RE::TESActorLocationChangeEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESActorLocationChangeEvent* ev,
        RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override
    {
        if (!ev) return RE::BSEventNotifyControl::kContinue;
        if (g_state == State::Playing &&
            ev->actor.get() == RE::PlayerCharacter::GetSingleton())
            RefreshPosition("location-change");
        return RE::BSEventNotifyControl::kContinue;
    }
};

class CellLoadSink : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESCellFullyLoadedEvent* ev,
        RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override
    {
        if (!ev) return RE::BSEventNotifyControl::kContinue;
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (g_state == State::Playing && player && player->GetParentCell() == ev->cell)
            RefreshPosition("cell-loaded");
        return RE::BSEventNotifyControl::kContinue;
    }
};

class QuestStageSink : public RE::BSTEventSink<RE::TESQuestStageEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESQuestStageEvent* ev,
        RE::BSTEventSource<RE::TESQuestStageEvent>*) override
    {
        if (!ev || !g_config.show_quest) return RE::BSEventNotifyControl::kContinue;
        if (g_state == State::Playing)
            SKSE::GetTaskInterface()->AddTask([]() { RefreshPosition("quest-stage"); });
        return RE::BSEventNotifyControl::kContinue;
    }
};

class QuestStartStopSink : public RE::BSTEventSink<RE::TESQuestStartStopEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESQuestStartStopEvent* ev,
        RE::BSTEventSource<RE::TESQuestStartStopEvent>*) override
    {
        if (!ev || !g_config.show_quest) return RE::BSEventNotifyControl::kContinue;
        if (g_state == State::Playing)
            SKSE::GetTaskInterface()->AddTask([]() { RefreshPosition("quest-startstop"); });
        return RE::BSEventNotifyControl::kContinue;
    }
};

class CombatSink : public RE::BSTEventSink<RE::TESCombatEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESCombatEvent* ev,
        RE::BSTEventSource<RE::TESCombatEvent>*) override
    {
        if (!ev || !g_config.show_combat) return RE::BSEventNotifyControl::kContinue;
        if (g_state != State::Playing) return RE::BSEventNotifyControl::kContinue;

        // TESCombatEvent fires for the NPC side too (actor=NPC, targetActor=player),
        // so check IsPlayerRef() on both sides rather than comparing pointers.
        bool actorIsPlayer  = ev->actor       && ev->actor->IsPlayerRef();
        bool targetIsPlayer = ev->targetActor && ev->targetActor->IsPlayerRef();
        bool involvesPlayer = actorIsPlayer || targetIsPlayer;
        // kNone events not involving the player may signal that the player also exited
        // combat — only trigger if we're currently showing combat to avoid spam.
        bool mayEndCombat = ev->newState.get() == RE::ACTOR_COMBAT_STATE::kNone
                            && !g_combatTarget.empty();
        SKSE::log::info("TESCombatEvent: state={} actorIsPlayer={} targetIsPlayer={}",
            static_cast<int>(ev->newState.get()), actorIsPlayer, targetIsPlayer);
        if (!involvesPlayer && !mayEndCombat)
            return RE::BSEventNotifyControl::kContinue;

        // Read game-object state on the game thread via AddTask.
        SKSE::GetTaskInterface()->AddTask([]() {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || g_state != State::Playing) return;

            if (player->IsInCombat()) {
                if (auto target = player->GetActorRuntimeData().currentCombatTarget.get()) {
                    std::string name = SafeStr(target->GetName());
                    g_combatTarget = name.empty() ? Locale("combat_no_target")
                                                  : FormatCombat(Locale("combat_fighting"), name);
                } else if (g_combatTarget.empty()) {
                    g_combatTarget = Locale("combat_no_target");
                }
                // currentCombatTarget temporarily null — keep existing name
            } else {
                g_combatTarget.clear();
            }

            SKSE::log::info("Combat: '{}'", g_combatTarget);
            RefreshPosition("combat");
        });
        return RE::BSEventNotifyControl::kContinue;
    }
};

MenuEventSink      g_menuSink;
LocationChangeSink g_locationSink;
CellLoadSink       g_cellSink;
QuestStageSink     g_questStageSink;
QuestStartStopSink g_questStartStopSink;
CombatSink         g_combatSink;

std::atomic<bool> g_callbackThreadRunning{false};

static void StartCallbackThread() {
    if (g_callbackThreadRunning.exchange(true)) return;
    std::thread([]() {
        while (g_callbackThreadRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            SKSE::GetTaskInterface()->AddTask([]() {
                if (g_core) g_core->RunCallbacks();
            });
        }
    }).detach();
}

void InitDiscord() {
    g_startTime = static_cast<int64_t>(std::time(nullptr));
    discord::Result result;
    try {
        result = discord::Core::Create(kAppId, DiscordCreateFlags_Default, &g_core);
    } catch (...) {
        // discord_game_sdk.dll not found — delay-load threw; run without presence.
        SKSE::log::warn("Discord: discord_game_sdk.dll not found — presence disabled.");
        g_core = nullptr;
        return;
    }
    if (result != discord::Result::Ok) {
        SKSE::log::error("Discord: failed to initialize (result={})", static_cast<int>(result));
        g_core = nullptr;
        return;
    }
    g_core->SetLogHook(discord::LogLevel::Warn, [](discord::LogLevel, const char* msg) {
        SKSE::log::warn("Discord: {}", msg);
    });
    SKSE::log::info("Discord Game SDK initialized.");
    StartCallbackThread();
}

} // anonymous namespace

void LoadConfig() {
    std::ifstream file(R"(Data\SKSE\Plugins\DragonbornPresenceConfig.json)");
    if (!file) return;
    try {
        auto j = nlohmann::json::parse(file);
        auto try_bool = [&](const char* key, bool& field) {
            if (auto it = j.find(key); it != j.end() && it->is_boolean())
                field = it->get<bool>();
        };
        try_bool("show_location",    g_config.show_location);
        try_bool("show_quest",       g_config.show_quest);
        try_bool("show_combat",      g_config.show_combat);
        try_bool("show_player_info", g_config.show_player_info);
    } catch (const nlohmann::json::exception& e) {
        SKSE::log::error("Failed to parse config JSON: {}", e.what());
    }
    SKSE::log::info("Config: location={} quest={} combat={} player_info={}",
        g_config.show_location, g_config.show_quest, g_config.show_combat, g_config.show_player_info);
}

void SetLocale() {
    std::ifstream file(R"(Data\SKSE\Plugins\DragonbornPresenceLocale.json)");
    if (!file) return;

    try {
        auto j = nlohmann::json::parse(file);
        for (auto& [key, val] : j.items())
            if (val.is_string()) g_locale[key] = val.get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        SKSE::log::error("Failed to parse locale JSON: {}", e.what());
    }
}

void RegisterGameEventHandlers() {
    SKSE::log::info("Registering game event handlers...");
    InitDiscord();
    // Plugin loads before Main Menu appears, but the open-event may still
    // race. Start in MainMenu state so Discord shows something right away.
    TransitionTo(State::MainMenu);

    auto* ui = RE::UI::GetSingleton();
    if (ui) {
        ui->AddEventSink<RE::MenuOpenCloseEvent>(&g_menuSink);
    } else {
        SKSE::log::error("Failed to get RE::UI singleton — menu events won't be tracked.");
    }

    auto* src = RE::ScriptEventSourceHolder::GetSingleton();
    if (src) {
        src->AddEventSink<RE::TESActorLocationChangeEvent>(&g_locationSink);
        src->AddEventSink<RE::TESCellFullyLoadedEvent>(&g_cellSink);
        src->AddEventSink<RE::TESQuestStageEvent>(&g_questStageSink);
        src->AddEventSink<RE::TESQuestStartStopEvent>(&g_questStartStopSink);
        src->AddEventSink<RE::TESCombatEvent>(&g_combatSink);
    } else {
        SKSE::log::error("Failed to get RE::ScriptEventSourceHolder singleton.");
    }
}

void OnGameLoaded() {
    // kPostLoadGame / kNewGame fires on the main thread after the engine
    // has fully committed all save data — call RefreshPosition directly.
    SKSE::log::info("Game loaded — forcing presence refresh");
    g_state = State::Playing;
    RefreshPosition("game-loaded");
}

} // namespace DragonbornPresence
