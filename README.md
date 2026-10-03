# DragonbornPresence SE

An SKSE plugin for **Skyrim Special Edition** that displays your current in-game state as Discord Rich Presence.

While you play, Discord shows your character's name, race, level, current location, in-game time and weather, active quest, and what you're doing right now — fighting, talking, trading, crafting, reading, sneaking, riding, sleeping — and updates automatically as you move through the world.

---

## Features

- Current location displayed in Discord (worldspace + named location)
- In-game clock — e.g. `Skyrim: Whiterun · 14:30` (rounded to half an hour)
- Weather — 🌧 while raining, ❄ while snowing (exteriors only)
- Character info: name, race, and level
- Active quest name shown alongside the location
- Combat state — shows `In combat with <enemy name>` while in combat, replacing the quest suffix
- NPC dialogue — shows `Talking to <NPC name>` while in conversation
- Crafting — shows `Smithing`, `Brewing`, or `Enchanting` while at a crafting station
- Activities — `Reading <book>`, `Trading with <merchant>`, `Pickpocketing <victim>`, `Picking a lock`, `Training`, `Sleeping`, `Waiting`
- Movement — `Sneaking`, `Swimming`, or `Riding <horse name>` when nothing more important is happening
- Bounty — `Wanted: <gold>` in the details line while any hold has a bounty on you
- Death — shows `Dead` when you die
- Session timer showing how long you've been playing
- State-aware presence: Main Menu, Character Creation, Loading, and In-Game are all handled separately
- Configurable — every element can be toggled independently via a JSON file
- Localization support — 11 languages included, all labels replaceable via a JSON file
- Graceful degradation if Discord is not running

---

## Requirements

- Skyrim Special Edition or Anniversary Edition, including the 1.7.x update (any runtime — uses Address Library)
- [SKSE64](https://skse.silverlock.org/) matching your runtime

---

## Installation

**Mod manager (recommended):** Install the archive normally. The FOMOD installer will prompt you to pick a language. The config file is installed automatically.

**Manual:** Extract the archive, copy `SKSE\` into your Skyrim `Data\` directory, then copy the locale file for your language from `locales\<lang>\DragonbornPresenceLocale.json` to `Data\SKSE\Plugins\`.

Launch the game through SKSE. Discord must be running before or alongside the game — if it is not running the plugin loads normally and presence is simply disabled.

---

## Configuration

Edit `Data\SKSE\Plugins\DragonbornPresenceConfig.json` to control what appears in the presence:

```json
{
    "show_location": true,
    "show_quest": true,
    "show_combat": true,
    "show_dialogue": true,
    "show_crafting": true,
    "show_player_info": true,
    "show_menus": true,
    "show_movement": true,
    "show_time": true,
    "show_weather": true,
    "show_bounty": true,
    "show_death": true
}
```

- `show_location` — current location (e.g. `Skyrim: Riverwood`)
- `show_quest` — active quest name shown as a suffix after the location
- `show_combat` — enemy name shown during combat, replacing the quest suffix
- `show_dialogue` — NPC name shown during conversation, replacing the combat/quest suffix
- `show_crafting` — activity shown while at a crafting station (`Smithing`, `Brewing`, or `Enchanting`), replacing the quest suffix
- `show_player_info` — character name, race, and level shown in the details line
- `show_menus` — activity shown while reading a book, trading, pickpocketing, lockpicking, training, sleeping, or waiting
- `show_movement` — `Sneaking` / `Swimming` / `Riding <horse>` when no higher-priority activity is shown
- `show_time` — in-game clock after the location
- `show_weather` — rain/snow marker next to the clock (exteriors only)
- `show_bounty` — `Wanted: <gold>` in the details line when your total bounty is above zero
- `show_death` — `Dead` when your character dies

If the file is missing or a key is absent, that feature defaults to `true`. An invalid value for a key is silently ignored and the default is kept.

**Suffix priority** (only one is shown at a time): death > dialogue > combat > crafting > menu activity > movement > active quest.

---

## Localization

The archive includes locale files for 11 languages. The FOMOD installer lets you pick one during installation:

| Code | Language |
|------|----------|
| `en` | English |
| `ru` | Russian / Русский |
| `de` | German / Deutsch |
| `fr` | French / Français |
| `es` | Spanish / Español |
| `it` | Italian / Italiano |
| `pl` | Polish / Polski |
| `zh` | Chinese (Simplified) / 中文 |
| `ja` | Japanese / 日本語 |
| `ko` | Korean / 한국어 |
| `pt` | Portuguese (BR) / Português |

You can also edit `Data\SKSE\Plugins\DragonbornPresenceLocale.json` directly at any time:

```json
{
    "main_menu": "Main menu",
    "editing_character": "Editing character",
    "combat_fighting": "In combat with {name}",
    "combat_no_target": "In combat",
    "talking_to": "Talking to {name}",
    "crafting_smithing": "Smithing",
    "crafting_brewing": "Brewing",
    "crafting_enchanting": "Enchanting",
    "crafting_other": "Crafting",
    "reading": "Reading {name}",
    "trading": "Trading with {name}",
    "pickpocketing": "Pickpocketing {name}",
    "lockpicking": "Picking a lock",
    "training": "Training",
    "waiting": "Waiting",
    "sleeping": "Sleeping",
    "dead": "Dead",
    "sneaking": "Sneaking",
    "swimming": "Swimming",
    "riding": "Riding {name}",
    "riding_no_name": "On horseback",
    "wanted": "Wanted: {gold}",
    "weather_rain": "🌧",
    "weather_snow": "❄"
}
```

All `{name}` keys replace the placeholder at runtime; position is flexible so SOV languages can write e.g. `"{name}と戦闘中"`.

- `combat_fighting` — shown when an enemy name is known (e.g. `"In combat with Alduin"`).
- `combat_no_target` — shown when in combat but no enemy name is available (rare transient state).
- `talking_to` — shown while in conversation with an NPC.
- `crafting_smithing` / `crafting_brewing` / `crafting_enchanting` — smithing stations / alchemy lab / enchanting table.
- `crafting_other` — any other recipe station (cooking pots, Hearthfire building tables, etc.).
- `reading` — `{name}` is the book's title.
- `trading` / `pickpocketing` — `{name}` is the merchant's / victim's name.
- `lockpicking`, `training`, `waiting`, `sleeping`, `dead` — plain labels.
- `sneaking` / `swimming` — plain labels shown while sneaking or swimming.
- `riding` — `{name}` is the mount's name; `riding_no_name` is used when the mount has no name.
- `wanted` — `{gold}` is replaced with your total bounty across all holds.
- `weather_rain` / `weather_snow` — shown next to the in-game clock; default to the 🌧 / ❄ emoji, replace with text if you prefer.

If the file is missing or any key is absent, English defaults are used.

---

## Building from Source

### Requirements

- CMake 3.28 or later
- Visual Studio 2022 with the **Desktop development with C++** workload
- Internet access for the first CMake configure

### Steps

```bash
git clone https://github.com/your-repo/DragonbornPresence-SE.git
cd DragonbornPresence-SE
```

Open `CMakeLists.txt` in CLion, or configure from the command line:

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

On the first configure CMake downloads [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) v10.1.0 (alandtse fork — compiled from source, ~15 min the first time), [Discord Game SDK 3.2.1](https://discord.com/developers/docs/game-sdk/sdk-starter-guide), and several other libraries. Subsequent builds use the cache and are fast.

The post-build step produces `DragonbornPresence.zip` in the build directory with the full install layout ready to drop into the game.

---

## Architecture

Pure C++ DLL — no `.esp`, no Papyrus scripts.

**`main.cpp`** — Plugin entry point (`SKSEPluginLoad`). Sets up spdlog logging, reads the locale file, registers the SKSE messaging listener. Reacts to `kDataLoaded` (registers event sinks) and `kNewGame`/`kPostLoadGame` (forces presence refresh).

**`DragonbornPresence.cpp`** — All state and Discord logic.

- **State machine** (`enum class State`): `Loading → MainMenu → Playing / EditingCharacter`. Transitions via `TransitionTo()`.
- **`MenuEventSink`** — listens to `MenuOpenCloseEvent` and dispatches to per-menu handlers: state transitions (`Main Menu`, `Loading Menu`, `RaceSex Menu`), dialogue, crafting, book reading, bartering, pickpocketing (`ContainerMenu` in pickpocket mode only), lockpicking, training, and the sleep/wait menu.
- **`LocationChangeSink`** — listens to `TESActorLocationChangeEvent`; calls `RefreshPosition()` when the player changes named location.
- **`CellLoadSink`** — listens to `TESCellFullyLoadedEvent`; calls `RefreshPosition()` when the player's cell finishes loading.
- **`QuestStageSink`** / **`QuestStartStopSink`** — listen to quest stage and start/stop events; refresh presence via an SKSE task so the update runs on the game thread.
- **`CombatSink`** — listens to `TESCombatEvent` filtered to the player. On `kCombat` stores `"In combat with <target name>"` in `g_combatTarget` and triggers a refresh; on `kNone` clears it.
- **`DeathSink`** — listens to `TESDeathEvent`; shows `Dead` when the player dies, cleared on load.
- **`SleepStartSink`** / **`SleepStopSink`** — listen to `TESSleepStartEvent` / `TESSleepStopEvent` for the `Sleeping` label.
- **`PollGameState()`** — game-thread task run every ~2 s by the callback thread; detects changes in sneaking/swimming/riding, in-game time, weather, and bounty (none of which have engine events) and refreshes only when the composite state changes.
- **`BuildPosition()`** — traverses the `parentLoc` chain to find the first named ancestor. Returns `"Worldspace: Location"` for exterior, `"Location"` for interior, cell name as last resort. Caches the last non-empty result to handle null pointers during location boundary crossing.
- **`BuildActiveQuest()`** — scans displayed quest objectives and returns the name of the highest-priority active quest.
- **`BuildPlayerInfo()`** — returns `"Name - Race (Level)"`; the details line also carries the bounty when above zero.
- **`BuildTimeWeather()`** — in-game clock (floored to 30 minutes) plus rain/snow marker (exteriors only).
- **`SendPresence()`** — sends a `discord::Activity` to the Discord Game SDK.
- **`StartCallbackThread()`** — background thread that posts one `SKSE::GetTaskInterface()->AddTask` per 100 ms to call `g_core->RunCallbacks()` on the game thread. Keeps Discord IPC processing off the hot path without per-frame overhead.
- **`DeferredRefresh(int ticks)`** — self-rescheduling SKSE task used after `EditingCharacter → Playing` to wait ~10 frames for the engine to commit the new character name.

**`AdditionalFunctions.cpp`** — `Cp1251ToUtf8`, `IsValidUtf8`. Used by `SafeStr()` to handle Russian locale game data.

**`discord_loader.cpp`** — `__pfnDliNotifyHook2` delay-load hook. Intercepts `discord_game_sdk.dll` load and resolves it from the plugin's own directory instead of the system search path.

### Log file

`%USERPROFILE%\Documents\My Games\Skyrim Special Edition\SKSE\DragonbornPresence.log`

---

## Dependencies (bundled or auto-fetched)

| Dependency | Version | How |
|---|---|---|
| [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) (alandtse fork) | v10.1.0 | CMake FetchContent |
| [Discord Game SDK](https://discord.com/developers/docs/game-sdk/sdk-starter-guide) | 3.2.1 | CMake download |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.12.0 | CMake FetchContent |
| fmt | 12.1.0 | CMake FetchContent |
| spdlog | v1.16.0 | CMake FetchContent |
| [DirectXTK](https://github.com/microsoft/DirectXTK) | oct2025 | CMake FetchContent (headers only, required by CommonLib) |

---

## License

[GPL-3.0](LICENSE)
