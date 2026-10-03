#include "DragonbornPresence.h"
#include "AdditionalFunctions.h"
#include "discord.h"
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace DragonbornPresence {

namespace {

constexpr discord::ClientId kAppId          = 565627104608256015LL;
constexpr const char*       kLargeImageKey  = "skyrim_logo";
constexpr const char*       kLargeImageText = "The Elder Scrolls V: Skyrim";
constexpr const char*       kSeparator      = " \xC2\xB7 ";  // · (U+00B7)
constexpr std::size_t       kMaxFieldBytes  = 127;  // discord::Activity copies into char[128]
constexpr auto              kMinSendInterval = std::chrono::seconds(4);  // Discord: 5 updates / 20 s

enum class State { Loading, MainMenu, EditingCharacter, Playing };

// Written on the game thread; read by event sinks that may run elsewhere.
std::atomic<State> g_state{State::Loading};
discord::Core* g_core              = nullptr;
int64_t        g_startTime         = 0;
std::unordered_map<std::string, std::string> g_locale = {
    {"main_menu",           "Main menu"},
    {"editing_character",   "Editing character"},
    {"combat_fighting",     "In combat with {name}"},
    {"combat_no_target",    "In combat"},
    {"talking_to",          "Talking to {name}"},
    {"crafting_smithing",   "Smithing"},
    {"crafting_brewing",    "Brewing"},
    {"crafting_enchanting", "Enchanting"},
    {"crafting_other",      "Crafting"},
    {"reading",             "Reading {name}"},
    {"trading",             "Trading with {name}"},
    {"pickpocketing",       "Pickpocketing {name}"},
    {"lockpicking",         "Picking a lock"},
    {"training",            "Training"},
    {"waiting",             "Waiting"},
    {"sleeping",            "Sleeping"},
    {"dead",                "Dead"},
    {"sneaking",            "Sneaking"},
    {"swimming",            "Swimming"},
    {"riding",              "Riding {name}"},
    {"riding_no_name",      "On horseback"},
    {"wanted",              "Wanted: {gold}"},
    {"weather_rain",        "\xF0\x9F\x8C\xA7"},   // 🌧
    {"weather_snow",        "\xE2\x9D\x84"},       // ❄
};

// Presence sub-state. All mutated on the game thread (event sinks + AddTask).
std::string g_lastPosition;     // last non-empty location (engine nulls during transitions)
std::string g_combatTarget;     // "In combat with X" while player is in combat
std::string g_dialogueSpeaker;  // "Talking to X" while Dialogue Menu is open
std::string g_craftingActivity; // "Smithing"/"Brewing"/… while Crafting Menu is open
std::string g_menuActivity;     // reading/trading/pickpocketing/lockpicking/training/waiting/sleeping
std::string g_pollSignature;    // movement|time-weather|bounty snapshot for change detection
bool        g_isDead = false;   // player died (cleared on load/new game)

struct Config {
    bool show_location    = true;
    bool show_quest       = true;
    bool show_combat      = true;
    bool show_dialogue    = true;
    bool show_crafting    = true;
    bool show_player_info = true;
    bool show_menus       = true;   // book/barter/pickpocket/lockpick/training/sleep/wait
    bool show_movement    = true;   // sneaking/swimming/riding
    bool show_time        = true;   // in-game clock
    bool show_weather     = true;   // rain/snow (exterior only)
    bool show_bounty      = true;   // total crime gold
    bool show_death       = true;
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

// Replaces placeholder in tmpl with value; appends " " + value if no placeholder
// (legacy fallback for locale files written before the placeholder existed).
static std::string FormatPlaceholder(const std::string& tmpl, std::string_view placeholder,
                                     const std::string& value) {
    auto pos = tmpl.find(placeholder);
    if (pos == std::string::npos)
        return tmpl + " " + value;
    std::string result = tmpl;
    result.replace(pos, placeholder.size(), value);
    return result;
}

static std::string FormatWithName(const std::string& tmpl, const std::string& name) {
    return FormatPlaceholder(tmpl, "{name}", name);
}

static std::string Join(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    return a + kSeparator + b;
}

// Cuts s to at most maxBytes without splitting a UTF-8 sequence and marks the cut
// with "…". The Discord SDK truncates blindly at 127 bytes, which can leave a broken
// trailing character for Cyrillic/CJK text.
static void TruncateUtf8(std::string& s, std::size_t maxBytes) {
    if (s.size() <= maxBytes) return;
    constexpr std::string_view kEllipsis = "\xE2\x80\xA6";  // … (U+2026)
    std::size_t n = maxBytes - kEllipsis.size();
    while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) --n;
    s.resize(n);
    s += kEllipsis;
}

// Opens a file from Data\SKSE\Plugins: relative to the working directory first (the
// game root), then next to the DLL in case the game was started from elsewhere.
static std::ifstream OpenPluginFile(std::wstring_view fileName) {
    std::ifstream file(std::filesystem::path(L"Data\\SKSE\\Plugins") / fileName);
    if (file) return file;
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), buf, MAX_PATH);
    return std::ifstream(std::filesystem::path(buf).parent_path() / fileName);
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

    // Accessor handles the base shifts at AE 1.6.629 and 1.7.99.
    auto& objectives = player->GetPlayerRuntimeData().objectives;

    // Prefer quests the player marked active in the journal; break ties by priority.
    RE::TESQuest* best = nullptr;
    for (const auto& instObj : objectives) {
        if (instObj.InstanceState != RE::QUEST_OBJECTIVE_STATE::kDisplayed) continue;
        auto* quest = instObj.Objective ? instObj.Objective->ownerQuest : nullptr;
        if (!quest) continue;
        if (!best ||
            (quest->IsActive() && !best->IsActive()) ||
            (quest->IsActive() == best->IsActive() && quest->data.priority > best->data.priority))
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

// Sneaking / swimming / riding — polled, no dedicated engine events.
static std::string BuildMovement(RE::PlayerCharacter* player) {
    if (!g_config.show_movement) return "";
    if (auto* st = player->AsActorState(); st && st->IsSwimming())
        return Locale("swimming");
    if (player->IsSneaking())
        return Locale("sneaking");
    if (player->IsOnMount()) {
        RE::NiPointer<RE::Actor> mount;
        std::string name;
        if (player->GetMount(mount) && mount)
            name = SafeStr(mount->GetName());
        return name.empty() ? Locale("riding_no_name")
                            : FormatWithName(Locale("riding"), name);
    }
    return "";
}

// In-game clock (floored to 30 minutes so presence doesn't churn) + rain/snow marker.
static std::string BuildTimeWeather(RE::PlayerCharacter* player) {
    std::string result;
    if (g_config.show_time) {
        if (auto* cal = RE::Calendar::GetSingleton()) {
            float hour = cal->GetHour();
            int   hh   = static_cast<int>(hour) % 24;
            int   mm   = (hour - static_cast<int>(hour)) >= 0.5f ? 30 : 0;
            char  buf[8];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);
            result = buf;
        }
    }
    if (g_config.show_weather) {
        auto* cell = player->GetParentCell();
        if (!cell || !cell->IsInteriorCell()) {  // no weather indoors
            if (auto* sky = RE::Sky::GetSingleton()) {
                std::string w;
                if (sky->IsRaining())      w = Locale("weather_rain");
                else if (sky->IsSnowing()) w = Locale("weather_snow");
                if (!w.empty())
                    result += result.empty() ? w : " " + w;
            }
        }
    }
    return result;
}

// Total bounty across all crime-tracking factions (holds).
static int GetTotalBounty() {
    if (!g_config.show_bounty) return 0;
    auto* dh = RE::TESDataHandler::GetSingleton();
    if (!dh) return 0;
    int total = 0;
    for (auto* faction : dh->GetFormArray<RE::TESFaction>()) {
        if (!faction || !(faction->data.flags & RE::FACTION_DATA::Flag::kTrackCrime))
            continue;
        if (auto gold = faction->GetCrimeGold(); gold > 0)
            total += gold;
    }
    return total;
}

// Re-reads the player's combat state into g_combatTarget; returns true if it changed.
// Polled as well as event-driven: no event fires for the player when their target
// dies or they switch to another enemy.
static bool UpdateCombatTarget(RE::PlayerCharacter* player) {
    std::string next;
    if (g_config.show_combat && player->IsInCombat()) {
        if (auto target = player->GetActorRuntimeData().currentCombatTarget.get()) {
            std::string name = SafeStr(target->GetName());
            next = name.empty() ? Locale("combat_no_target")
                                : FormatWithName(Locale("combat_fighting"), name);
        } else {
            // currentCombatTarget is briefly null while switching targets — keep existing name
            next = g_combatTarget.empty() ? Locale("combat_no_target") : g_combatTarget;
        }
    }
    if (next == g_combatTarget) return false;
    g_combatTarget = std::move(next);
    SKSE::log::info("Combat: '{}'", g_combatTarget);
    return true;
}

// ---- Discord dispatch -------------------------------------------------------
// Updates are deduplicated and spaced kMinSendInterval apart so bursts (combat
// events, fast-forwarded time while waiting) don't hit Discord's rate limit and
// drop the final state. FlushPresence() runs every 100 ms and sends the latest
// queued update once the interval has passed.

struct Presence { std::string state, details; };
std::optional<Presence>               g_pendingPresence;
Presence                              g_sentPresence;
std::chrono::steady_clock::time_point g_lastSend{};

static void FlushPresence() {
    if (!g_core || !g_pendingPresence) return;
    const auto now = std::chrono::steady_clock::now();
    if (now - g_lastSend < kMinSendInterval) return;

    g_sentPresence = std::move(*g_pendingPresence);
    g_pendingPresence.reset();
    g_lastSend = now;

    discord::Activity activity{};
    if (!g_sentPresence.state.empty())   activity.SetState(g_sentPresence.state.c_str());
    if (!g_sentPresence.details.empty()) activity.SetDetails(g_sentPresence.details.c_str());
    activity.GetTimestamps().SetStart(g_startTime);
    activity.GetAssets().SetLargeImage(kLargeImageKey);
    activity.GetAssets().SetLargeText(kLargeImageText);

    g_core->ActivityManager().UpdateActivity(activity, [](discord::Result r) {
        if (r != discord::Result::Ok) {
            SKSE::log::error("Discord: UpdateActivity failed (result={})", static_cast<int>(r));
            g_sentPresence = {};  // don't let dedup suppress a retry of the same text
        } else {
            SKSE::log::info("Presence updated.");
        }
    });
}

void SendPresence(std::string state, std::string details) {
    if (!g_core) return;
    TruncateUtf8(state, kMaxFieldBytes);
    TruncateUtf8(details, kMaxFieldBytes);
    if (state == g_sentPresence.state && details == g_sentPresence.details) {
        g_pendingPresence.reset();  // a queued update would only overwrite this with stale text
        return;
    }
    g_pendingPresence = Presence{std::move(state), std::move(details)};
    FlushPresence();
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

    std::string movement    = BuildMovement(player);
    std::string timeWeather = BuildTimeWeather(player);
    int         bounty      = GetTotalBounty();
    // Snapshot for the poller: refresh only fires when this composite changes.
    g_pollSignature = movement + '\x1F' + timeWeather + '\x1F' + std::to_string(bounty);

    std::string suffix;
    if (g_isDead && g_config.show_death)
        suffix = Locale("dead");
    else if (!g_dialogueSpeaker.empty() && g_config.show_dialogue)
        suffix = g_dialogueSpeaker;
    else if (!g_combatTarget.empty() && g_config.show_combat)
        suffix = g_combatTarget;
    else if (!g_craftingActivity.empty() && g_config.show_crafting)
        suffix = g_craftingActivity;
    else if (!g_menuActivity.empty() && g_config.show_menus)
        suffix = g_menuActivity;
    else if (!movement.empty())
        suffix = movement;
    else if (g_config.show_quest)
        suffix = BuildActiveQuest(player);

    std::string state = Join(display, suffix);
    // Time/weather is the least important part — drop it rather than truncate the rest.
    if (std::string full = Join(state, timeWeather); full.size() <= kMaxFieldBytes || state.empty())
        state = std::move(full);

    std::string details = g_config.show_player_info ? BuildPlayerInfo(player) : "";
    if (bounty > 0)
        details = Join(details, FormatPlaceholder(Locale("wanted"), "{gold}", std::to_string(bounty)));

    SKSE::log::info("[{}] player='{}' location='{}' suffix='{}' extra='{}'{}",
        trigger ? trigger : "refresh", details, display, suffix, timeWeather,
        fallback ? " [fallback]" : "");
    SendPresence(std::move(state), std::move(details));
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
    State prev = g_state.exchange(next);
    switch (next) {
    case State::MainMenu:
        SKSE::log::info("State -> MainMenu");
        SendPresence(Locale("main_menu"), {});
        break;
    case State::EditingCharacter:
        SKSE::log::info("State -> EditingCharacter");
        SendPresence(Locale("editing_character"), {});
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
        g_dialogueSpeaker.clear();
        g_craftingActivity.clear();
        g_menuActivity.clear();
        g_lastPosition.clear();  // may belong to another save after this load
        g_isDead = false;
        break;
    }
}

// Clears g_menuActivity (set by book/barter/pickpocket/lockpick/training/wait/sleep)
// and refreshes if it was showing.
static void ClearMenuActivity(const char* trigger) {
    if (g_menuActivity.empty()) return;
    g_menuActivity.clear();
    if (g_state == State::Playing) RefreshPosition(trigger);
}

// ---- Menu handlers (called from MenuEventSink::ProcessEvent) --------------

static void OnDialogueMenu(bool opening) {
    if (opening && g_config.show_dialogue) {
        // Deferred one frame so MenuTopicManager::speaker is populated.
        SKSE::GetTaskInterface()->AddTask([]() {
            if (!g_config.show_dialogue || g_state != State::Playing) return;
            auto* mtm = RE::MenuTopicManager::GetSingleton();
            if (mtm) {
                if (auto ref = mtm->speaker.get()) {
                    std::string name = SafeStr(ref->GetName());
                    if (!name.empty())
                        g_dialogueSpeaker = FormatWithName(Locale("talking_to"), name);
                }
            }
            RefreshPosition("dialogue-open");
        });
    } else if (!opening) {
        g_dialogueSpeaker.clear();
        if (g_state == State::Playing)
            RefreshPosition("dialogue-close");
    }
}

static void OnCraftingMenu(bool opening) {
    if (!g_config.show_crafting) return;
    if (opening) {
        // subMenu and furniture are set after the open event — defer one frame.
        SKSE::GetTaskInterface()->AddTask([]() {
            if (!g_config.show_crafting || g_state != State::Playing) return;
            std::string activity = Locale("crafting_smithing");
            if (auto* ui = RE::UI::GetSingleton()) {
                if (auto gptr = ui->GetMenu<RE::CraftingMenu>()) {
                    auto* cm  = static_cast<RE::CraftingMenu*>(gptr.get());
                    auto* sub = cm->GetCraftingSubMenu();
                    // sub->furniture can be a TESObjectREFR* (SmithingMenu) or a
                    // TESFurniture* (AlchemyMenu, EnchantConstructMenu) depending on
                    // the subclass. Check the actual form type to handle both cases.
                    RE::TESFurniture* furn = nullptr;
                    if (sub && sub->furniture) {
                        auto* form = reinterpret_cast<RE::TESForm*>(sub->furniture);
                        if (form->GetFormType() == RE::FormType::Reference) {
                            auto* base = static_cast<RE::TESObjectREFR*>(form)->GetBaseObject();
                            furn = base ? base->As<RE::TESFurniture>() : nullptr;
                        } else {
                            furn = form->As<RE::TESFurniture>();
                        }
                    }
                    if (furn) {
                        using BT = RE::TESFurniture::WorkBenchData::BenchType;
                        const auto bench = furn->workBenchData.benchType.get();
                        switch (bench) {
                        case BT::kAlchemy:
                        case BT::kAlchemyExperiment:
                            activity = Locale("crafting_brewing");    break;
                        case BT::kEnchanting:
                        case BT::kEnchantingExperiment:
                            activity = Locale("crafting_enchanting"); break;
                        case BT::kCreateObject:
                            // Forges share kCreateObject with smelters, tanning racks and
                            // cooking pots — only the workbench keyword tells them apart.
                            if (!furn->HasKeywordString("CraftingSmithingForge") &&
                                !furn->HasKeywordString("CraftingSmithingSkyforge"))
                                activity = Locale("crafting_other");
                            break;
                        default: break;  // kSmithingWeapon, kSmithingArmor
                        }
                        SKSE::log::info("Crafting: benchType={}", static_cast<int>(bench));
                    }
                }
            }
            g_craftingActivity = activity;
            SKSE::log::info("Menu: 'Crafting Menu' open -> crafting='{}'", g_craftingActivity);
            RefreshPosition("crafting-open");
        });
    } else {
        SKSE::log::info("Menu: 'Crafting Menu' close");
        g_craftingActivity.clear();
        if (g_state == State::Playing) RefreshPosition("crafting-close");
    }
}

static void OnBookMenu(bool opening) {
    if (!g_config.show_menus) return;
    if (opening) {
        // Target form is set after the open event — defer one frame.
        SKSE::GetTaskInterface()->AddTask([]() {
            if (!g_config.show_menus || g_state != State::Playing) return;
            std::string name;
            if (auto* book = RE::BookMenu::GetTargetForm())
                name = SafeStr(book->GetName());
            if (!name.empty()) {
                g_menuActivity = FormatWithName(Locale("reading"), name);
                RefreshPosition("book-open");
            }
        });
    } else {
        ClearMenuActivity("book-close");
    }
}

static void OnBarterMenu(bool opening) {
    if (!g_config.show_menus) return;
    if (opening) {
        SKSE::GetTaskInterface()->AddTask([]() {
            if (!g_config.show_menus || g_state != State::Playing) return;
            std::string name;
            if (auto ref = RE::TESObjectREFR::LookupByHandle(RE::BarterMenu::GetTargetRefHandle()))
                name = SafeStr(ref->GetName());
            if (!name.empty()) {
                g_menuActivity = FormatWithName(Locale("trading"), name);
                RefreshPosition("barter-open");
            }
        });
    } else {
        ClearMenuActivity("barter-close");
    }
}

static void OnContainerMenu(bool opening) {
    if (!g_config.show_menus) return;
    if (opening) {
        SKSE::GetTaskInterface()->AddTask([]() {
            if (!g_config.show_menus || g_state != State::Playing) return;
            auto* ui = RE::UI::GetSingleton();
            auto  cm = ui ? ui->GetMenu<RE::ContainerMenu>() : nullptr;
            // Only pickpocketing is presence-worthy; plain looting is too noisy.
            if (!cm || cm->GetContainerMode() != RE::ContainerMenu::ContainerMode::kPickpocket)
                return;
            std::string name;
            if (auto ref = RE::TESObjectREFR::LookupByHandle(RE::ContainerMenu::GetTargetRefHandle()))
                name = SafeStr(ref->GetName());
            if (!name.empty()) {
                g_menuActivity = FormatWithName(Locale("pickpocketing"), name);
                RefreshPosition("pickpocket-open");
            }
        });
    } else {
        ClearMenuActivity("container-close");
    }
}

// Lockpicking / Training / Sleep-Wait — static text, no target lookup needed.
static void OnSimpleActivityMenu(bool opening, const char* localeKey, const char* trigger) {
    if (!g_config.show_menus) return;
    if (opening) {
        if (g_state == State::Playing) {
            g_menuActivity = Locale(localeKey);
            RefreshPosition(trigger);
        }
    } else {
        ClearMenuActivity(trigger);
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
        } else if (menu == RE::DialogueMenu::MENU_NAME) {
            OnDialogueMenu(opening);
        } else if (menu == "Journal Menu") {
            if (!opening && g_state == State::Playing)
                RefreshPosition("journal-close");
        } else if (menu == RE::CraftingMenu::MENU_NAME) {
            OnCraftingMenu(opening);
        } else if (menu == RE::BookMenu::MENU_NAME) {
            OnBookMenu(opening);
        } else if (menu == RE::BarterMenu::MENU_NAME) {
            OnBarterMenu(opening);
        } else if (menu == RE::ContainerMenu::MENU_NAME) {
            OnContainerMenu(opening);
        } else if (menu == RE::LockpickingMenu::MENU_NAME) {
            OnSimpleActivityMenu(opening, "lockpicking", opening ? "lockpick-open" : "lockpick-close");
        } else if (menu == RE::TrainingMenu::MENU_NAME) {
            OnSimpleActivityMenu(opening, "training", opening ? "training-open" : "training-close");
        } else if (menu == RE::SleepWaitMenu::MENU_NAME) {
            OnSimpleActivityMenu(opening, "waiting", opening ? "wait-open" : "wait-close");
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
            SKSE::GetTaskInterface()->AddTask([]() {
                if (g_state == State::Playing) RefreshPosition("location-change");
            });
        return RE::BSEventNotifyControl::kContinue;
    }
};

class CellLoadSink : public RE::BSTEventSink<RE::TESCellFullyLoadedEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESCellFullyLoadedEvent* ev,
        RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override
    {
        if (!ev || g_state != State::Playing) return RE::BSEventNotifyControl::kContinue;
        SKSE::GetTaskInterface()->AddTask([cell = ev->cell]() {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (g_state == State::Playing && player && player->GetParentCell() == cell)
                RefreshPosition("cell-loaded");
        });
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
        // combat. The task below only refreshes when the combat text actually changes.
        bool mayEndCombat = ev->newState.get() == RE::ACTOR_COMBAT_STATE::kNone;
        if (!involvesPlayer && !mayEndCombat)
            return RE::BSEventNotifyControl::kContinue;

        // Read game-object state on the game thread via AddTask.
        SKSE::GetTaskInterface()->AddTask([]() {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || g_state != State::Playing) return;
            if (UpdateCombatTarget(player))
                RefreshPosition("combat");
        });
        return RE::BSEventNotifyControl::kContinue;
    }
};

class DeathSink : public RE::BSTEventSink<RE::TESDeathEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESDeathEvent* ev,
        RE::BSTEventSource<RE::TESDeathEvent>*) override
    {
        if (!ev || !g_config.show_death) return RE::BSEventNotifyControl::kContinue;
        if (ev->actorDying && ev->actorDying->IsPlayerRef()) {
            SKSE::GetTaskInterface()->AddTask([]() {
                if (g_state != State::Playing || g_isDead) return;
                g_isDead = true;
                RefreshPosition("death");
            });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

// TESSleepStartEvent is only forward-declared in CommonLibSSE — fine, the sink
// never dereferences the event.
class SleepStartSink : public RE::BSTEventSink<RE::TESSleepStartEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESSleepStartEvent*,
        RE::BSTEventSource<RE::TESSleepStartEvent>*) override
    {
        if (g_config.show_menus) {
            SKSE::GetTaskInterface()->AddTask([]() {
                if (!g_config.show_menus || g_state != State::Playing) return;
                g_menuActivity = Locale("sleeping");
                RefreshPosition("sleep-start");
            });
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};

class SleepStopSink : public RE::BSTEventSink<RE::TESSleepStopEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::TESSleepStopEvent*,
        RE::BSTEventSource<RE::TESSleepStopEvent>*) override
    {
        SKSE::GetTaskInterface()->AddTask([]() { ClearMenuActivity("sleep-stop"); });
        return RE::BSEventNotifyControl::kContinue;
    }
};

MenuEventSink      g_menuSink;
LocationChangeSink g_locationSink;
CellLoadSink       g_cellSink;
QuestStageSink     g_questStageSink;
QuestStartStopSink g_questStartStopSink;
CombatSink         g_combatSink;
DeathSink          g_deathSink;
SleepStartSink     g_sleepStartSink;
SleepStopSink      g_sleepStopSink;

// Detects changes in polled state (movement/time/weather/bounty/combat target) that
// have no reliable engine events. Runs on the game thread every ~2 s.
static void PollGameState() {
    if (g_state != State::Playing) return;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    const bool combatChanged = UpdateCombatTarget(player);
    std::string sig = BuildMovement(player) + '\x1F' + BuildTimeWeather(player)
                    + '\x1F' + std::to_string(GetTotalBounty());
    if (combatChanged || sig != g_pollSignature)
        RefreshPosition("poll");  // updates g_pollSignature itself
}

std::atomic<bool> g_callbackThreadRunning{false};

static void StartCallbackThread() {
    if (g_callbackThreadRunning.exchange(true)) return;
    std::thread([]() {
        int tick = 0;
        while (g_callbackThreadRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            const bool poll = (++tick % 20) == 0;  // every ~2 s
            SKSE::GetTaskInterface()->AddTask([poll]() {
                if (g_core) g_core->RunCallbacks();
                if (poll) PollGameState();
                FlushPresence();
            });
        }
    }).detach();
}

void InitDiscord() {
    g_startTime = static_cast<int64_t>(std::time(nullptr));
    discord::Result result;
    try {
        // NoRequireDiscord: with Default the SDK closes the game when Discord isn't running.
        result = discord::Core::Create(kAppId, DiscordCreateFlags_NoRequireDiscord, &g_core);
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
    auto file = OpenPluginFile(L"DragonbornPresenceConfig.json");
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
        try_bool("show_dialogue",    g_config.show_dialogue);
        try_bool("show_crafting",    g_config.show_crafting);
        try_bool("show_player_info", g_config.show_player_info);
        try_bool("show_menus",       g_config.show_menus);
        try_bool("show_movement",    g_config.show_movement);
        try_bool("show_time",        g_config.show_time);
        try_bool("show_weather",     g_config.show_weather);
        try_bool("show_bounty",      g_config.show_bounty);
        try_bool("show_death",       g_config.show_death);
    } catch (const nlohmann::json::exception& e) {
        SKSE::log::error("Failed to parse config JSON: {}", e.what());
    }
    SKSE::log::info("Config: location={} quest={} combat={} dialogue={} crafting={} player_info={} "
        "menus={} movement={} time={} weather={} bounty={} death={}",
        g_config.show_location, g_config.show_quest, g_config.show_combat,
        g_config.show_dialogue, g_config.show_crafting, g_config.show_player_info,
        g_config.show_menus, g_config.show_movement, g_config.show_time,
        g_config.show_weather, g_config.show_bounty, g_config.show_death);
}

void SetLocale() {
    auto file = OpenPluginFile(L"DragonbornPresenceLocale.json");
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
        src->AddEventSink<RE::TESDeathEvent>(&g_deathSink);
        src->AddEventSink<RE::TESSleepStartEvent>(&g_sleepStartSink);
        src->AddEventSink<RE::TESSleepStopEvent>(&g_sleepStopSink);
    } else {
        SKSE::log::error("Failed to get RE::ScriptEventSourceHolder singleton.");
    }
}

void OnGameLoaded() {
    // kPostLoadGame / kNewGame fires on the main thread after the engine
    // has fully committed all save data — call RefreshPosition directly.
    SKSE::log::info("Game loaded — forcing presence refresh");
    g_state  = State::Playing;
    g_isDead = false;
    RefreshPosition("game-loaded");
}

} // namespace DragonbornPresence
