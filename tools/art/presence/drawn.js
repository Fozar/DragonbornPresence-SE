// Hand-drawn glyphs for what the game has no icon for (activities, special worlds, inns).
// 100x100 box. Base fill is var(--fg); class "d" = dark detail (var(--bg)),
// "ws" / "ds" = light / dark stroke. Kept in the same flat style as the map markers.
const mirror = (s) => `${s}<g transform="translate(100 0) scale(-1 1)">${s}</g>`;
const sword = `<path d="M50 6 L55 15 V60 H45 V15Z"/><path class="d" d="M49 17h2v41h-2z"/>
  <rect x="35" y="60" width="30" height="6" rx="3"/><rect x="47" y="66" width="6" height="16" rx="2"/><circle cx="50" cy="87" r="5"/>`;

const DRAWN_GLYPHS = {
  // ---- activities (small image) ----
  Combat: `<g class="outline"><g transform="rotate(-40 50 50)">${sword}</g><g transform="rotate(40 50 50)">${sword}</g></g>`,
  Dialogue: `<path d="M18 22h64a7 7 0 0 1 7 7v34a7 7 0 0 1-7 7H46L28 86V70H18a7 7 0 0 1-7-7V29a7 7 0 0 1 7-7Z"/>
    <circle class="d" cx="33" cy="46" r="5"/><circle class="d" cx="50" cy="46" r="5"/><circle class="d" cx="67" cy="46" r="5"/>`,
  Smithing: `<g class="outline"><path d="M8 50 H74 C74 58 66 63 57 63 V70 H67 V82 H25 V70 H35 V63 C22 63 12 58 8 50Z"/>
    <path d="M74 50 H94 C90 56 82 59 74 59Z"/>
    <g transform="rotate(-38 58 30)"><rect x="56" y="24" width="7" height="30" rx="2"/><rect x="43" y="12" width="33" height="15" rx="2"/></g></g>`,
  Brewing: `<path d="M40 12h20v6h-4v19l22 35a8 8 0 0 1-7 12H29a8 8 0 0 1-7-12l22-35V18h-4Z"/>
    <path class="d" d="M31 63h38l6 10a3 3 0 0 1-3 4H28a3 3 0 0 1-3-4Z"/><circle cx="44" cy="70" r="3"/><circle cx="56" cy="72" r="2"/>`,
  Enchanting: `<path d="M50 8 L58 42 L92 50 L58 58 L50 92 L42 58 L8 50 L42 42Z"/><path d="M78 12l3 9 9 3-9 3-3 9-3-9-9-3 9-3Z"/>
    <path d="M22 66l2 6 6 2-6 2-2 6-2-6-6-2 6-2Z"/>`,
  Crafting: `<rect x="14" y="36" width="72" height="9" rx="4.5"/><path d="M20 45 H80 V58 C80 74 67 84 50 84 C33 84 20 74 20 58Z"/>
    <path class="d" d="M28 52 H72 V58 C72 70 62 76 50 76 C38 76 28 70 28 58Z"/>
    <path class="ws" stroke-width="5" stroke-linecap="round" d="M38 28 C34 22 42 18 38 10 M50 28 C46 22 54 18 50 10 M62 28 C58 22 66 18 62 10"/>`,
  Reading: `<path d="M50 26 C40 18 24 18 10 22v56c14-4 28-4 40 4 12-8 26-8 40-4V22c-14-4-30-4-40 4Z"/>
    <path class="d" d="M48 28v50h4V28Z"/><path class="ds" stroke-width="3" stroke-linecap="round" d="M20 36h20M20 46h20M20 56h16M60 36h20M60 46h20M60 56h16"/>`,
  Trading: `<g class="outline"><ellipse cx="38" cy="74" rx="26" ry="10"/><ellipse cx="38" cy="64" rx="26" ry="10"/><ellipse cx="38" cy="54" rx="26" ry="10"/>
    <circle cx="68" cy="36" r="22"/></g><circle class="d" cx="68" cy="36" r="15"/><circle cx="68" cy="36" r="11"/>`,
  // Coin purse: gathered neck, drawstring with a dangling tie, two folds.
  Pickpocketing: `<path d="M38 40 C22 50 14 62 14 73 C14 87 30 93 50 93 C70 93 86 87 86 73 C86 62 78 50 62 40Z"/>
    <path d="M37 40 C35 33 30 26 24 21 C33 21 39 25 43 29 C44 22 47 17 50 12 C53 17 56 22 57 29 C61 25 67 21 76 21 C70 26 65 33 63 40Z"/>
    <rect class="d" x="34" y="37" width="32" height="7" rx="3.5"/>
    <path class="ds" stroke-width="3.5" stroke-linecap="round" d="M37 56 C31 64 31 76 36 84 M63 62 C68 68 69 76 65 84"/>`,
  Lockpicking: `<path d="M28 46V34a22 22 0 0 1 44 0v12h-10V34a12 12 0 0 0-24 0v12Z"/><rect x="20" y="46" width="60" height="42" rx="7"/>
    <circle class="d" cx="50" cy="63" r="7"/><path class="d" d="M46 66h8l3 13H43Z"/>`,
  Training: `<path d="M50 10 L86 46 L74 58 L50 34 L26 58 L14 46Z"/><path d="M50 42 L86 78 L74 90 L50 66 L26 90 L14 78Z"/>`,
  Waiting: `<rect x="22" y="8" width="56" height="9" rx="3"/><rect x="22" y="83" width="56" height="9" rx="3"/>
    <path d="M28 17 H72 C72 38 57 45 55 50 C57 55 72 62 72 83 H28 C28 62 43 55 45 50 C43 45 28 38 28 17Z"/>
    <path class="d" d="M35 22 H65 C64 37 52 43 51 50 C52 57 64 63 65 78 H35 C36 63 48 57 49 50 C48 43 36 37 35 22Z"/>
    <path d="M40 28 H60 C58 36 52 40 50 45 C48 40 42 36 40 28Z M37 78 C39 68 47 63 50 60 C53 63 61 68 63 78Z"/>`,
  Sleeping: `<path d="M56 12 A38 38 0 1 0 88 64 A30 30 0 1 1 56 12Z"/>
    <path class="ws" stroke-width="5" stroke-linejoin="round" stroke-linecap="round" d="M66 20h14l-14 14h14"/>`,
  Dead: `<path d="M50 10c-21 0-36 15-36 34 0 12 6 21 15 25v15h42V69c9-4 15-13 15-25 0-19-15-34-36-34Z"/>
    <circle class="d" cx="35" cy="46" r="10"/><circle class="d" cx="65" cy="46" r="10"/><path class="d" d="M50 57l-6 10h12Z"/>
    <path class="ds" stroke-width="3.5" d="M41 73v11M50 73v11M59 73v11"/>`,
  Swimming: `<circle cx="62" cy="24" r="10"/><path d="M20 50 L44 38 L62 46 L54 54 L44 50 L32 56Z"/>
    <path class="ws" stroke-width="7" stroke-linecap="round" d="M10 66 q10-9 20 0 t20 0 t20 0 t20 0 M10 84 q10-9 20 0 t20 0 t20 0 t20 0"/>`,

  // ---- special worlds and interiors (large image) ----
  // Hall of Valor: central spire with the round window, stepped wings with horned gable ends.
  Sovngarde: mirror(`<path d="M50 6 L58 30 V82 H50Z"/><path d="M58 40 H68 V82 H58Z"/><path d="M68 54 H90 V82 H68Z"/>
      <path d="M64 54 L92 44 L94 54Z"/><path d="M88 46 C90 40 94 36 98 34 C96 40 96 46 94 52Z"/>
      <path d="M58 34 L72 28 C74 26 76 22 80 20 C78 26 78 32 76 38 L68 42 H58Z"/>
      <rect x="4" y="82" width="46" height="7"/>
      <rect class="d" x="52.5" y="36" width="3" height="20" rx="1.5"/><rect class="d" x="61" y="48" width="3" height="16" rx="1.5"/>
      <rect class="d" x="73" y="62" width="3" height="12" rx="1.5"/><rect class="d" x="80" y="62" width="3" height="12" rx="1.5"/>`)
    + `<circle class="d" cx="50" cy="22" r="4"/><path class="d" d="M45 82 V72 a5 5 0 0 1 10 0 V82Z"/>`,
  // Blackreach: the hanging Dwemer sun orb.
  Blackreach: `<rect x="47" y="4" width="6" height="30"/><circle cx="50" cy="52" r="20"/><circle class="d" cx="50" cy="52" r="13"/><circle cx="50" cy="52" r="8"/>
    <path class="ws" stroke-width="5" stroke-linecap="round" d="M50 82v10M78 52h10M12 52h10M70 72l7 7M23 25l7 7M70 32l7-7M23 79l7-7"/>`,
  // Soul Cairn: the tilted soul crystal floating in its aura over the ruined towers.
  SoulCairn: `<path class="ws" stroke-width="3" stroke-linecap="round" d="${[0, 40, 80, 120, 240, 280, 320].map((a) => {
      const r = (a - 90) * Math.PI / 180, c = Math.cos(r), s = Math.sin(r), r0 = a % 80 ? 30 : 33;
      return `M${50 + c * r0} ${36 + s * r0} L${50 + c * (r0 + 7)} ${36 + s * (r0 + 7)}`; }).join(' ')}"/>
    <g transform="rotate(18 50 36)"><path d="M50 6 L64 18 L66 44 L52 64 L38 52 L36 22Z"/>
      <path class="ds" stroke-width="2.5" stroke-linejoin="round" d="M36 22 L54 28 L64 18 M54 28 L52 64 M54 28 L66 44"/></g>
    <path d="M4 92 V80 L7 76 L10 79 V70 L13 66 L16 70 V92Z M22 92 V82 L25 78 L28 81 V92Z M34 92 V72 L38 68 L40 72 L43 70 V92Z
      M56 92 V78 L59 75 L62 79 V92Z M68 92 V70 L71 66 L74 70 L77 68 V92Z M84 92 V76 L88 72 L91 76 V92Z"/>
    <rect x="2" y="88" width="96" height="6"/><rect class="d" x="37" y="76" width="3" height="5"/><rect class="d" x="71" y="74" width="3" height="5"/>`,
  // Apocrypha: Hermaeus Mora — one-eyed tentacle mass over an open Black Book, small eyes around.
  Apocrypha: `<path class="ws" stroke-width="6" stroke-linecap="round" d="M42 26 C32 18 30 10 37 3 M58 26 C68 18 70 10 63 3
      M36 34 C22 30 14 22 6 25 M64 34 C78 30 86 22 94 25 M34 40 C16 44 8 58 14 72 M66 40 C84 44 92 58 86 72
      M44 82 C41 90 48 94 44 99 M56 82 C59 90 52 94 56 99"/>
    <ellipse cx="50" cy="33" rx="19" ry="13"/><circle class="d" cx="50" cy="34" r="7.5"/><circle cx="50" cy="34" r="3"/>
    <path d="M50 58 C40 52 26 52 13 56 V82 C26 78 40 78 50 84 C60 78 74 78 87 82 V56 C74 52 60 52 50 58Z"/>
    <path class="ds" stroke-width="2.5" d="M50 59 V83"/>
    <circle class="ds" stroke-width="2.5" cx="31" cy="68" r="7"/><path class="ds" stroke-width="2.5" stroke-linejoin="round" d="M69 61 L76 74 H62Z"/>
    ${[[20, 14], [80, 14], [12, 42], [88, 42], [24, 92], [76, 92]].map(([x, y]) =>
      `<circle cx="${x}" cy="${y}" r="4"/><circle class="d" cx="${x}" cy="${y}" r="1.8"/>`).join('')}`,
  Inn: `<path d="M22 30h42v50a6 6 0 0 1-6 6H28a6 6 0 0 1-6-6Z"/><path d="M64 40h8a11 11 0 0 1 11 11v10a11 11 0 0 1-11 11h-8v-9h7a3 3 0 0 0 3-3v-10a3 3 0 0 0-3-3h-7Z"/>
    <path d="M19 32c0-9 7-13 13-11 2-7 13-9 17-2 7-4 18 0 18 9v6H19Z"/>
    <path class="ds" stroke-width="3.5" d="M33 44v32M43 44v32M53 44v32"/>`,
};
