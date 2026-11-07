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
- **Terrain.** Hills and ridges between two radios shrink the sender's range, so a call behind a ridge
  gets more static, then nothing. High ground clears them. (Reworked later; see "Ideas from Lima Charlie".)
- **Voice directivity.** A voice is up to 5 dB quieter behind the talker than in front.
- **Handheld radio on speaker.** "Toggle Radio Speaker" (Left Ctrl + End) also works on your active
  handheld radio when you have no vehicle radio or manpack, so people nearby hear the net.

### Radio keypads and displays work like the real radios

Each radio keeps its own picture; its keys and screen were rebuilt (`CVON_RadioMenu.c` and the five
`UI/layouts/Radios/*` layouts).

- **Display.** Up to four lines inside the radio's own screen: preset number and CT/PT (crypto or plain
  text) with the volume, the net name, the frequency in MHz (`31.120`), and the ear or monitored channel.
  The PRC-152 and R-168 screens are too small for the fourth line, so messages use the name line there.
  Messages: VOLUME n, STORED, SAVED, INVALID FREQ, NOT AVAILABLE. A pressed key darkens for a moment.
- **Harris keypads (AN/PRC-152, -117, -150), per their key labels:** PRE + / - step presets (past the
  last one makes a new one, as before). VOL + / - set the volume. 7 OPT opens the options; 8 PGM types a
  new frequency for the preset; 2 LT switches the backlight. 6 and 9 (the arrow icons) and the left and
  right arrows move inside menus. In the options, ENT edits an entry and ENT again saves it; CLR backs
  out without saving. 1 CALL, 3 MODE, 4 SQL and 5 ZERO show NOT AVAILABLE (nothing to drive in VON).
  The PRC-152 picture has no volume keys, so its left and right arrows set the volume on the home screen.
- **R-168 and R-187:** + / - (the up and down arrows on the R-187) step channels; the bottom-row keys
  (R-168) and top soft keys (R-187) open the options; the red / green keys clear and store; digits type a
  frequency; the R-168 volume knob (left half down, right half up) and the R-187 side keys set volume.
- **Options (OPT):** volume, transmit power, ear, monitor channel, monitor ear, backlight, clock offset
  (shown as the radio's time, HH:MM).
- **Transmit power LOW / MED / HIGH.** Sets how far your calls reach: HIGH is the radio's full range (as
  before, and the default), MED half, LOW a quarter (PRC-152: 5 km, 2.5 km, 1.3 km). The OPT entry shows
  the range for each step, and the home screen shows H, M or L like the real Harris displays. Kept through
  respawn with the other radio settings. The server uses the sender's power when it works out who hears
  the call. Per radio prefab: `m_fMediumPowerRange` (0.5) and `m_fLowPowerRange` (0.25). Range grows
  roughly with the square root of power, so a quarter of the range is about a sixteenth of the power.
- **TX / RX marker and signal bars.** Five bars on the status line (on the R-187, a meter under the
  frequency). While someone talks on a channel the radio hears, it shows R and the bars give the signal
  strength: distance against the sender's range, minus terrain in the way, the same value that adds static
  to the call. While you transmit on that radio it shows T and the bars show your power setting
  (LOW 2, MED 3, HIGH 5). Unlit bars stay faintly visible, like LCD segments.
- **Keyboard:** the number pad types a frequency straight away; Backspace works like CLR.
- **Help overlay:** labels now name the new functions, with a short legend for the function keys.
- **Host fix:** on the machine hosting a game, the power knob used to flip twice and do nothing; it now
  switches once.
- **Sources.** PRE +/- presets, CLR backing out without saving, ENT saving, 7 OPT and 8 PGM come from
  public PRC-152 guides (ACRE2 and RHS documentation, used for behaviour only). VOL, LT, SQL, CALL,
  MODE and ZERO are read off the key labels drawn on the radios. No manual for the R-168 or R-187 keypads
  was found, so their keys keep the original Coalition mapping.
- **Net names.** Channels show the net's name from the frequency config: a group's new `m_sNetName`
  ("1 PLT NET") or, if empty, its first group name (long-range nets get "LR").

### New radio pop-up

- **New radio pop-up.** Replaces the small box clipped at the left edge: radio name, a coloured state
  (TRANSMITTING amber, RECEIVING green), channel, frequency and net name, the caller's name, and the ear
  and monitor. Layout: `UI/layouts/HUD/VON/BVON_RadioPopup.layout`. The radial menu keeps its entries.
- The pop-up and the display rework are original. TFAR and ACRE were looked at for what works (real key
  functions, channel names on the display) but none of their files are used: TFAR is APL-SA and ACRE2 is
  GPLv3, which can't be mixed into this APL mod.

### Mission / server settings (on `CVON_VONGameModeComponent`)

| Setting | Default | Meaning |
|---|---|---|
| `m_bAutoFillVehicleRadios` | on | Vehicle radios take the crew's key when they get in |
| `m_bShowRadioCallerName` | on | Incoming-call pop-up with the talker's name |
| `m_fTerrainEffect` | 1 | Scales every hill's height for the terrain model (2 = a 30 m hill counts as 60 m); 0 = off |
| `m_bRadioKeySpamLock` | on | More than 4 radio key presses in 4 s locks the radio keys for 2 s |
| `m_bGameMasterUnlimitedRadio` | on | Game Masters with the editor open transmit and receive radio with no range or terrain limit |
| `m_bHandheldSpeakers` | on | Handheld radios can go on speaker |

### Ideas from Lima Charlie

[Lima Charlie](https://github.com/IPS-Keystone/Lima-Charlie) (IPS-Keystone) is another TeamSpeak radio
mod for Reforger. Its repository has no licence, so none of its code, sounds or files are used. These are
our own implementations of behaviour its release notes and documentation describe:

- **Terrain takes a share of the range.** Replaces the TFAR-style "extra metres per metre of hill". The
  ground is sampled every 25 m along the line between the two radios. Each hill above that line takes a
  share of the sender's range that grows with its height (at most 80% for one hill), and several hills
  each take their share of what is left (at most 90% together). A ridge costs a 5 km handheld and a 20 km
  set the same share, so long range sets still reach far behind it. Examples: a 30 m hill takes 22%, a
  60 m hill 37%, a 120 m hill 57%, a 30 m and a 60 m hill together 51%.
- **Release tail.** A radio call ends 0.3 s after you let go of the key, so the end of your last word still
  goes out. Pressing the same radio's key again inside that 0.3 s carries on the same call, so a stutter on
  the key doesn't send a new pair of beeps.
- **Radio key spam lock.** More than four radio key presses in four seconds locks the radio keys for two
  seconds, with a deny sound and a hint.
- **Game Master radio.** With the editor open, a Game Master's calls reach everyone on the net and every
  call on their radios comes through clear, wherever the camera or body is.
- **Checked, no change needed:** Lima Charlie found Reforger stops writing its files when alt-tabbed. Ours
  kept writing its heartbeat in a muted test both unfocused and minimized, so alt-tabbing does not drop you
  out of the TeamSpeak channel (tested in a local game, not on a multiplayer client).

Not taken (yet), best first: room-by-room voice muffling and room echo, trees and thin props not muffling
voices, a join-time status notice (plugin, TeamSpeak, mic, channel), half duplex per radio, beeps only
inside 90% of range, enemy AI turning toward voices, coloured net names.

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
| Transmit power setting (ACRE, RHS) | No | Added (LOW / MED / HIGH) |
| Signal strength on the radio | No | Added (RX bars, TX power bars) |
| Diver radio | No | **Not ported** (no diving gear in our modset) |

## What was tested, and what wasn't

- Plugin builds (MSBuild, Release x64). Ear-routing test: 10/10 (original: 5/10).
- Mod compiles in Workbench 1.9.0.37 with zero errors.
- Play mode on `VONTestBasic` with the Game Master editor open: no script errors.
- Muted off-screen game self-test (48 checks, all passed): plugin version rules, defaults, 5 kHz
  frequencies, monitor channel routing rules, ear cycling, respawn keys, settings kept through death,
  voice directivity, terrain sampling, and the incoming-call pop-up text.
- Keypad, display, TX power, signal bars and the Lima Charlie ideas: muted off-screen self-test, 174 checks,
  all passed (every PRC-152 key on
  the home, OPT, edit and PGM screens, number pad and Backspace, power, the frequency reaching the server,
  TX power LOW / MED / HIGH and the range each gives, the R / T marker and bars for a call and a transmission,
  terrain shares for one and several hills, the key spam lock, the release tail, Game Master range with the
  editor closed and open,
  and the VOL keys, options and help legend on the other four radios). Screenshots of every radio's home
  screen, help overlay and backlight, plus the PRC-152 OPT and PGM screens, checked by eye.
- **Not tested here (needs players and TeamSpeak):** hearing the monitored net, the pop-up during a real
  call, auto key fill with real crews, handheld speaker heard by others, terrain static in a real call.

## Installing the plugin

Members install `Bifrost_VON_TeamSpeak_Plugin_3.1.0.ts3_plugin` (double-click). It keeps the file name
`CRF_Teamspeak_Plugin_Main.dll`, so it replaces the Coalition plugin instead of running beside it (two
copies would process every voice twice). The DLL in `src/x64/Release` is still the upstream build.
