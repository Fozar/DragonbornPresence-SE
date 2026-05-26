# Changelog

## 2.5.0 — 2026-05-26

Crafting activities are now shown in the presence. Opening a smithing forge, workbench, or any other crafting station shows `"Smithing"`; an alchemy lab shows `"Brewing"`; an enchanting table shows `"Enchanting"`. The suffix reverts to the active quest as soon as the crafting menu closes. Combat still takes priority over crafting if both are active simultaneously. Can be disabled with `"show_crafting": false` in the config file. The three locale strings (`crafting_smithing`, `crafting_brewing`, `crafting_enchanting`) are customisable.

## 2.4.0 — 2026-05-23

NPC dialogue is now shown in the presence. When you open a conversation the quest suffix is replaced with "Talking to <NPC name>" (e.g. `Skyrim: Whiterun · Talking to Farengar Secret-Fire`). The suffix reverts to the active quest as soon as the dialogue menu closes. Can be disabled with `"show_dialogue": false` in the config file. The locale string `talking_to` is customisable — use `{name}` as the placeholder; SOV languages can write e.g. `"{name}と会話中"`.

## 2.3.0 — 2026-05-23

Added a config file (`Data\SKSE\Plugins\DragonbornPresenceConfig.json`) to control what is shown in the presence. Each of the four elements — location, active quest, combat, and character info — can be toggled independently by setting its key to `true` or `false`. All are enabled by default. The file is installed automatically by the mod manager; manual installers will find it in the archive alongside the DLL.

## 2.2.0 — 2026-05-20

FOMOD installer included — mod managers will now ask you to pick a language during installation
Built-in translations for English, Russian, German, French, Spanish, Italian, Polish, Chinese (Simplified), Japanese, Korean, and Portuguese (Brazilian)
Combat presence now correctly reads as "In combat with Alduin" — the enemy name is placed inside the phrase rather than just appended, so each language can put the name where its grammar expects it
Added a separate display string for the rare moment when you are in combat but the game has not yet resolved the enemy name ("In combat" instead of nothing)

## 2.1.1 — 2026-05-20

discord_game_sdk.dll moved from the Skyrim root folder into Data\SKSE\Plugins where it belongs — mod managers now track and uninstall it cleanly
If you have an old discord_game_sdk.dll sitting next to SkyrimSE.exe, you can delete it
Fixed combat presence sometimes not clearing after combat ended

## 2.1.0 — 2026-05-15

Combat is now shown in Discord: while fighting, the active quest is replaced by the enemy's name (e.g. Skyrim: Helgen · In combat with Alduin)
Presence reverts to the quest name as soon as combat ends

## 2.0.1 — 2026-05-13

Active quest name now appears in Discord alongside your location (e.g. Skyrim: Whiterun · Bleak Falls Barrow)
Fixed a crash on startup when discord_game_sdk.dll was missing from the installation
Fixed presence sometimes freezing for several seconds during location transitions
Fixed a ghost "Loading" screen flash that appeared right after the game launched
Locale file format changed from .txt to .json — if you had a custom locale file, rename it and wrap the values in standard JSON
