# Credits and notices

## Original work

**Coalition VON** and the **Coalition TeamSpeak Plugin** by CoalitionArma
- Source: https://github.com/CoalitionArma/Coalition-VON
- Workshop: Coalition VON, item 6624E618B9058265, by SalamiBoiDev
- License: Arma Public License (APL), https://www.bohemia.net/community/licenses/arma-public-license

Authors, from the upstream git history and Workshop page:
- Salamiboio / SalamiBoiDev (author)
- Troy / the tanaKa (contributor)
- Njpatman (contributor)
- Björn Dahlgren (contributor)

This fork is not made, endorsed or supported by CoalitionArma. Report problems with Bifrost VON
to Task Force Halberd, not to the Coalition team.

## This fork

**Bifrost VON** by Task Force Halberd (25thplaymaking), forked from upstream commit `9517a39`
(Coalition VON 3.0.7 on the Workshop, plugin 3.0.1). Addon GUID `FE63A4168A0D582D`. It replaces
Coalition VON on a server: do not load both.

### Changes from the original
- Addon renamed to Bifrost VON with its own GUID; license and credit files added.
- TeamSpeak plugin: left and right radio ears now follow TeamSpeak's speaker layout, so they
  land in the correct ear on every playback device. Plugin renamed and versioned 3.1.0; the DLL
  file name is unchanged so it replaces the Coalition plugin instead of running beside it.
- Game Master and group-tuning null errors seen on the server fixed.
- Radio ear and volume settings survive death and come back on the right radio after respawning.
- Radios accept frequencies in 5 kHz steps (e.g. 31120, 31130).
- Vehicle radios fill themselves with the crew's encryption when they get in.
- Each radio can monitor a second channel in its own ear.
- The radio pop-up shows who is talking when a call comes in.
- Radio signal is weakened by terrain between sender and receiver.
- Voices are slightly quieter behind the talker; handheld radios can go on speaker.
- Plugin versions 3.0.1 and newer are accepted, with a hint to update older plugins.
- In-game hint and keybinding titles say "Bifrost VON".
- Radio keypads and displays rebuilt to work like the real radios (presets, OPT and PGM menus,
  backlight, volume keys, four-line display), net names on channels, and a new radio pop-up.
- Transmit power LOW / MED / HIGH on every radio, changing how far calls reach.
- TX / RX marker and signal-strength bars on the radio displays.
- Terrain now takes a share of a radio's range, a 0.3 s release tail on radio keys, a radio key spam lock,
  and unlimited radio range for Game Masters in the editor. Ideas from Lima Charlie's documentation;
  no Lima Charlie code or files are used (its repository has no licence).
- A join status notice (plugin, TeamSpeak, VON channel, mic and sound), with the plugin reporting that
  state, and trees, people, fences and thin props no longer muffling voices. Also ideas from Lima Charlie.
- A smaller radio pop-up (TX / RX, ear tag, long names cut to fit), the radio models' missing metal
  surface material added, and missing drone props removed from the test world.

See [docs/BIFROST_CHANGES.md](docs/BIFROST_CHANGES.md) for details and the TFAR comparison.

## Workshop description block

Paste this at the end of the Workshop description on every release:

> Bifrost VON is a fork of Coalition VON by CoalitionArma (SalamiBoiDev, the tanaKa and
> contributors), used under the Arma Public License (APL). Original: Workshop item
> 6624E618B9058265, https://github.com/CoalitionArma/Coalition-VON. Not endorsed by
> CoalitionArma. Non-commercial, Arma only.

## Ideas credited

- **Lima Charlie** by IPS-Keystone, https://github.com/IPS-Keystone/Lima-Charlie: the terrain range
  model, release tail, key spam lock, Game Master radio range, join status notice and what voices pass
  without muffling follow behaviour its documentation
  describes. Our own code; nothing copied.
- **TFAR** and **ACRE2**: features re-implemented from how they behave in game; no code or files used.

## Third-party code
- cJSON, Copyright (c) 2009-2017 Dave Gamble and cJSON contributors, MIT License (text kept
  in `CoalitionTeamspeakPlugin/include/cJSON.h` and `cJSON.c`).
- TeamSpeak 3 Plugin SDK headers and documentation, Copyright (c) TeamSpeak Systems GmbH.
