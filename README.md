# DragonbornPresence SE

An SKSE plugin for **Skyrim Special Edition** that displays your current in-game state as Discord Rich Presence.

While you play, Discord shows your character's name, race, level, current location, active quest, and current combat — and updates automatically as you move through the world.

---

## Features

- Current location displayed in Discord (worldspace + named location)
- Character info: name, race, and level
- Active quest name shown alongside the location
- Combat state — shows `In combat with <enemy name>` while in combat, replacing the quest suffix
- NPC dialogue — shows `Talking to <NPC name>` while in conversation, replacing the combat/quest suffix
- Session timer showing how long you've been playing
- State-aware presence: Main Menu, Character Creation, Loading, and In-Game are all handled separately
- Configurable — location, quest, combat, and character info can each be toggled via a JSON file
- Localization support — English labels can be replaced via a JSON file
- Graceful degradation if Discord is not running

---

## Requirements

- Skyrim Special Edition or Anniversary Edition (any runtime — uses Address Library)
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
    "show_player_info": true
}
```

- `show_location` — current location (e.g. `Skyrim: Riverwood`)
- `show_quest` — active quest name shown as a suffix after the location
- `show_combat` — enemy name shown during combat, replacing the quest suffix
- `show_dialogue` — NPC name shown during conversation, replacing the combat/quest suffix
- `show_player_info` — character name, race, and level shown in the details line

If the file is missing or a key is absent, that feature defaults to `true`. An invalid value for a key is silently ignored and the default is kept.

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
    "talking_to": "Talking to {name}"
}
```

- `combat_fighting` — shown when an enemy name is known. `{name}` is replaced with the enemy's name at runtime (e.g. `"In combat with Alduin"`). Place it anywhere in the string; SOV languages can write `"{name}と戦闘中"`.
- `combat_no_target` — shown when in combat but no enemy name is available (rare transient state). Falls back to `"In combat"` if the key is absent.
- `talking_to` — shown while in conversation with an NPC. `{name}` is replaced with the NPC's name (e.g. `"Talking to Farengar Secret-Fire"`). Flexible placement: `"{name}と会話中"` works for SOV languages.

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

On the first configure CMake downloads [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) v3.7.0, [Discord Game SDK 3.2.1](https://discord.com/developers/docs/game-sdk/sdk-starter-guide), and several header-only libraries — this takes a minute or two. Subsequent builds use the cache and are fast.

The post-build step produces `DragonbornPresence.zip` in the build directory with the full install layout ready to drop into the game.

---

## Architecture

Pure C++ DLL — no `.esp`, no Papyrus scripts.

**`main.cpp`** — Plugin entry point (`SKSEPluginLoad`). Sets up spdlog logging, reads the locale file, registers the SKSE messaging listener. Reacts to `kDataLoaded` (registers event sinks) and `kNewGame`/`kPostLoadGame` (forces presence refresh).

**`DragonbornPresence.cpp`** — All state and Discord logic.

- **State machine** (`enum class State`): `Loading → MainMenu → Playing / EditingCharacter`. Transitions via `TransitionTo()`.
- **`MenuEventSink`** — listens to `MenuOpenCloseEvent`. Maps `"Main Menu"`, `"Loading Menu"`, `"RaceSex Menu"`, and `"Journal Menu"` to state transitions or presence refreshes.
- **`LocationChangeSink`** — listens to `TESActorLocationChangeEvent`; calls `RefreshPosition()` when the player changes named location.
- **`CellLoadSink`** — listens to `TESCellFullyLoadedEvent`; calls `RefreshPosition()` when the player's cell finishes loading.
- **`QuestStageSink`** / **`QuestStartStopSink`** — listen to quest stage and start/stop events; refresh presence via an SKSE task so the update runs on the game thread.
- **`CombatSink`** — listens to `TESCombatEvent` filtered to the player. On `kCombat` stores `"In combat with <target name>"` in `g_combatTarget` and triggers a refresh; on `kNone` clears it. While `g_combatTarget` is set it replaces the quest suffix in the presence state line.
- **`BuildPosition()`** — traverses the `parentLoc` chain to find the first named ancestor. Returns `"Worldspace: Location"` for exterior, `"Location"` for interior, cell name as last resort. Caches the last non-empty result to handle null pointers during location boundary crossing.
- **`BuildActiveQuest()`** — scans displayed quest objectives and returns the name of the highest-priority active quest. Appended to the location string with a `·` separator when not in combat.
- **`BuildPlayerInfo()`** — returns `"Name - Race (Level)"`.
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
| [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) | v3.7.0 | CMake FetchContent |
| [Discord Game SDK](https://discord.com/developers/docs/game-sdk/sdk-starter-guide) | 3.2.1 | CMake download |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | CMake FetchContent |
| fmt | 10.2.1 | CMake FetchContent |
| spdlog | v1.13.0 | CMake FetchContent |

---

## License

[GPL-3.0](LICENSE)
