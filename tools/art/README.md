# Art for the mod page

SVG background + an HTML Discord-style profile card, drawn by `art.html` and rendered to PNG with headless Edge/Chrome.
Main colour: Discord blurple `#5865f2`. The card avatar is the mod logo (`logo.html` → `docs/media/logo.png`); `logo.png` here is the old flat logo, unused.

- `art.html#cover` — cover, 1920×1080:
  `msedge --headless=new --hide-scrollbars --window-size=1920,1080 --virtual-time-budget=8000 --allow-file-access-from-files --screenshot=cover.png "file:///…/art.html#cover"`
- `art.html#banner` — Nexus header banner, 1300×372 (`--window-size=1300,372`; add `--force-device-scale-factor=2` for 2600×744).

`--allow-file-access-from-files` is needed for the local images. Fonts (Cinzel, Cormorant Garamond, Noto Sans) come from
Google Fonts; without network the page falls back to Georgia / Segoe UI.
Rendered results: `docs/media/cover.png`, `docs/media/banner-1300x372.png` (+ `-2600x744`).
The card's presence images are the plugin's own icons from `../../assets/icons` (render those first).
- `logo.html` — mod logo for the top of the Nexus description (chat bubble with the presence-icon sky and the
  game's dragon), transparent 512×512 → `docs/media/logo.png`: same command with `--window-size=512,512
  --default-background-color=00000000` (`logo.html#<size>` for another size).
- `promo-icons.html` — 1920×1080 mod page image for the icons (sample cards + a selection of icons) → `docs/media/promo-icons.png`,
  same command as the cover.

# Discord presence icons (`presence/`)

`assets/icons/<key>.png` (512×512) — large image = location, small image = activity. The plugin
links them by raw GitHub URL, so a key is a public path: **never rename or delete one**, only add
(bump `?v=N` in the plugin's URL when an image changes, Discord caches by URL).

- `presence.html` — the generator, same night-sky style as the cover. Key → glyph tables `LARGE` / `SMALL`.
  `presence.html#sheet` shows all icons, `#<key>` renders one.
- `glyphs.js` — vector map markers (hold emblems, location types), the Skyrim logo and the sneak eye,
  taken unchanged from the game's `map.swf` / `statsmenu.swf` / `hudmenu.swf` by `extract.py`
  (`python extract.py "<Skyrim SE>\Data" [ffdec.jar] [java]`; needs `lz4` and JPEXS FFDec).
- `achievements.js` — Steam achievement icons traced to vectors by `achievements.py "<Steam>"`
  (needs `pillow numpy scipy potracer`). Only icon art, achievements themselves are not shown.
- `drawn.js` — hand-drawn glyphs for what the game has no icon for: activities, Sovngarde,
  Blackreach, Soul Cairn, Apocrypha, inn.
- `render.sh [key...]` — renders all (or the given) keys with headless Edge into `assets/icons/`,
  then `optimize.py` palettes them (Pillow; set `PYTHON=` if `python` isn't the right one).
