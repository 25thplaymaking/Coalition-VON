# Bifrost VON: what this fork changes

Forked from Coalition VON at upstream commit `9517a39` (Workshop 3.0.7, plugin 3.0.1). Credits and
license: [NOTICE.md](../NOTICE.md), [LICENSE.md](../LICENSE.md).

## The night of 2026-11-06: what actually crashed

- **The server crash (20:26 ET) was not in VON.** The dedicated server died inside the *Ambient
  Civilians* mod (`AC_VehicleManager.SpawnVehicle`) while it spawned a covered Ural truck: an engine
  crash, reported to Bohemia (crash GUID `0dabd044-22c7-4307-9fa1-b2161d37ad2d`). Every client was then
  kicked with "replication timeout". The same truck spawned fine in the two earlier sessions that day,
  so it is intermittent.
- **VON did log errors that night**, fixed here:
  - Server, when the Game Master editor opened on a death: a null error in the VON radio entry
    (`SCR_VONMenu.GetKnownChannel`). The call is gone; the channel text comes from the CVON radio.
  - Server, when a player joins a group: null errors in the frequency tuning when a radio had been
    dropped or deleted. Guarded.
- **Client error spam (75 per session) comes from ACE Radio**, not VON: ACE Radio's radial-menu code
  looks for a text field the CVON radio layout doesn't have. ACE Radio and VON both replace the radio
  system and nothing on the server depends on ACE Radio Dev (`65AD7C75826B46C6`). Recommended: remove
  it from the server's mod list.

## Fixes

| Problem | Fix |
|---|---|
| Radio ears wrong on many sound setups | The plugin wrote "left" and "right" to the first two output channels and never told TeamSpeak which channels it filled. It now follows TeamSpeak's speaker layout: left speakers get the left ear, right speakers the right, centre the mix, subwoofer nothing. Proven by `CoalitionTeamspeakPlugin/tests/test_ear_routing.c` (the original plugin fails 5 of its 10 checks). |
| Same net on two radios only played in one ear | Every matching radio counts; their ears add up. |
| Radio settings never came back after respawning | They were saved against the new character when control changed, which carries none of the old radios, so every death wiped them. Now saved from the character being left. |
| Ears and volumes came back on the wrong radio after respawning | They were restored by list position before the list was rotated, and the plugin file was written before the restore. Now matched by radio type ("AN/PRC-152 #1"), restored after rotation, then written. The server no longer sends its own, differently ordered, copy. |
| Ears set with the ear keybinds or by wrapping in the radio menu weren't saved for respawn | Saved every time. |
| Commo-card frequencies like 31120, 31130 refused | Radios accept 5 kHz steps (were 25 kHz). Per radio prefab: `m_iMinFrequency`, `m_iMaxFrequency`, `m_iFrequencyStep`. |
| Plugin version had to match exactly | Any plugin from 3.0.1 works; players on an older plugin get a one-time hint to install the Bifrost plugin. |

## New features

- **Monitor channel (hear two nets at once).** Radio menu, pages "Monitor" and "Monitor Ear": pick a
  second channel of the same radio and its ear. Traffic on it plays together with the main channel, so
  e.g. platoon net in the left ear and company net in the right on one radio. Transmitting stays on the
  main channel. Saved for respawn when the new radio has that frequency. Works with the old plugin too.
- **Vehicle radios fill themselves (crew comms style).** Getting into a crew seat puts the vehicle radio
  on your encryption and channels, unless someone else in a crew seat is already on its key. The
  manual "Fill Vehicle Radio With Your Encryption" keybind still works.
- **Caller name on the radio pop-up.** When a call comes in, the radio pop-up opens on the radio that
  hears it (channel, frequency, ear) with the talker's name on a new line. It yields for 1.5 s after
  you use your own radio keys; your own transmission always wins.
- **Terrain interference.** Hills and ridges between two radios weaken the call (more static, then
  nothing), like TFAR. High ground clears them.
- **Voice directivity.** A voice is up to 5 dB quieter behind the talker than in front.
- **Handheld radio on speaker.** "Toggle Radio Speaker" (Left Ctrl + End) also works on your active
  handheld radio when you have no vehicle radio or manpack, so people nearby hear the net.

### Mission / server settings (on `CVON_VONGameModeComponent`)

| Setting | Default | Meaning |
|---|---|---|
| `m_bAutoFillVehicleRadios` | on | Vehicle radios take the crew's key when they get in |
| `m_bShowRadioCallerName` | on | Incoming-call pop-up with the talker's name |
| `m_fTerrainInterference` | 7 | Extra signal metres per metre of terrain in the way; 0 = off |
| `m_bHandheldSpeakers` | on | Handheld radios can go on speaker |

## TFAR comparison

"Has" = Coalition VON already did it. Features are re-implemented from how they behave in game; no TFAR
code is used.

| TFAR feature | Coalition VON | Bifrost VON |
|---|---|---|
| Short- and long-range radios, channel presets | Has | |
| Per-radio volume and ear | Has, but buggy (above) | Fixed |
| Additional channel with its own ear | No | Added (Monitor) |
| Encryption per side, shared codes, using an enemy radio | Has | |
| Vehicle radio and intercom | Has | Auto key fill added |
| Radio on speakers | Manpack and vehicle only | Handhelds added |
| Terrain interference / high-ground advantage | No | Added |
| Directional voice | No | Added |
| Whisper / normal / yell | Has (5 levels) | |
| Muffling inside buildings and vehicles | Has | |
| Signal quality as static | Has | |
| Underwater: radios dead, voices muffled | No | **Not ported** (needs an in-game check of Reforger's water API) |
| Separate crew and passenger intercom channels | Passengers on/off only | **Not ported** |
| Diver radio | No | **Not ported** (no diving gear in our modset) |

## What was tested, and what wasn't

- Plugin builds (MSBuild, Release x64). Ear-routing test: 10/10 (original: 5/10).
- Mod compiles in Workbench 1.9.0.37 with zero errors.
- Play mode on `VONTestBasic` with the Game Master editor open: no script errors.
- Muted off-screen game self-test (48 checks, all passed): plugin version rules, defaults, 5 kHz
  frequencies, monitor channel routing rules, ear cycling, respawn keys, settings kept through death,
  voice directivity, terrain sampling, and the incoming-call pop-up text.
- **Not tested here (needs players and TeamSpeak):** hearing the monitored net, the pop-up during a real
  call, auto key fill with real crews, handheld speaker heard by others, terrain static in a real call.

## Installing the plugin

Members install `Bifrost_VON_TeamSpeak_Plugin_3.1.0.ts3_plugin` (double-click). It keeps the file name
`CRF_Teamspeak_Plugin_Main.dll`, so it replaces the Coalition plugin instead of running beside it (two
copies would process every voice twice). The DLL in `src/x64/Release` is still the upstream build.
