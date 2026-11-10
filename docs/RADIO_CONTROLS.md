# Bifrost VON radio control ledger (researched 2026-11-07)

Tags: CONFIRMED = read in a primary or near-primary document (source named). INFERRED = reasoned, secondary or from memory; verify before shipping.
Gaps are stated openly. No control is invented. No manual prints knob angles, so every angle is ESTIMATED.

Sources (short names)
- H117G = Harris AN/PRC-117G Field Reference Guide Rev K 2014 (10515-0319-4100), http://www.radiomanual.info/schemi/Surplus_NATO/AN-PRC-117G_Harris_user_2014.pdf (read locally)
- H150 = Harris AN/PRC-150(C) Operation Manual 10515-0103-4100 Rev G, https://w2hx.com/x/Harris/RF-5800/Docs/10515-0103-4100%20PRC-150%20Operation%20Manual%20-%20June%202007%20Rev%20G.pdf (Table 3-3, 3.5.1, 3.7, 3.15.12, 3.15.13)
- DS152A = L3Harris PRC-152A datasheet, https://www.l3harris.com/sites/default/files/2020-07/cs_tc_datasheet_falcon-iii-an-prc-152a-wideband-networking-handheld-radio.pdf
- TRI = Longview TRI AN/PRC-152 replica write-up (secondary), https://longview.be/tri-anprc-152-20222023-model.html
- ACRE2 = https://acre2.idi-systems.com/wiki/radios/an-prc-152 (game behaviour only)
- AZ = Azart operator memo, https://www.cryptomuseum.com/radio/azart/files/Azart_manual_RU.pdf (pp. 9, 12, 13 rendered and read) and https://www.cryptomuseum.com/radio/azart/index.htm

---
## 1. AN/PRC-117G (Falcon III manpack)

One rotary switch (Harris calls it the Cipher Switch). In-game picture matches reality.

| Control | Positions | Behaviour | Tag |
|---|---|---|---|
| Cipher / function switch | OFF, CT, PT, LD, Z | OFF: pull-to-turn to enter or leave, radio off. CT: cipher text (INVALID CRYPTO MODE if no TEK). PT: plain text. LD: radio off-line for key fill or software install. Z: pull-to-turn, panic zeroize (all crypto fills deleted, presets reset to factory) | Functions CONFIRMED (H117G p.18, Panic Zeroize). Panel order from a Polish training page https://pawel202685.github.io/psychic-octo-invention/ : INFERRED |
| Normal use | CT (secure) or PT (clear) | | CONFIRMED |
| Angle | not published | ESTIMATE 5 positions, about 45 degrees apart | ESTIMATED |
| Power | the OFF position of the knob. No power button | | CONFIRMED |
| Volume | KEYS only: VOL + / VOL - ("Not lockable"). No volume knob | Meter replaces BAT meter for 4 s after each press | CONFIRMED (H117G pp.19-20) |
| PRE +/- | keys, step presets | | CONFIRMED |
| Keypad legends | 0 NEXT, 1 CALL, 2 LT, 3 MODE, 4 SQL, 5 ZERO, 7 OPT, 8 PGM, CLR, ENT, arrows | LT = Keypad/Display backlight menu. ZERO key = zeroize menus (separate from the Z knob position) | CONFIRMED |

Lighting
- LT opens a Display Light menu: LIGHT MODE = MOMENTARY or NONE, LIGHT INTENSITY, SCREEN CONTRAST. It is a menu, not a plain on/off latch. The guide says LT controls the "Keypad/Display backlight", so keypad and display light together. CONFIRMED (H117G).
- Backlight colour: not stated. Photos show a pale green backlit LCD, dark text, NVG-friendly. INFERRED.
- Keys lighting individually on press: not described anywhere. INFERRED no.
- Display content: top-left T (transmitting) / R (receiving), blinking R = radio silence; BAT meter (VOL meter for 4 s on volume press); cipher type PT/CT/LD; RX/TX power bar graph (TX power and emanated power during TX, signal level during RX). CONFIRMED (H117G front panel display figure).
- Separate LEDs: none described. INFERRED none.
- Physical movement on TX: none. Display letter and bar graph only.

---
## 2. AN/PRC-150(C) (Falcon II HF manpack)

| Control | Positions | Behaviour | Tag |
|---|---|---|---|
| Function switch | OFF, PT, CT, CC, LD, RV (also marked A), Z. Manual lists them in that order but does not state dial order. The in-game order OFF, PT, CC, CT, RV, LD, Z is plausible | OFF: off. PT: plain text voice/data. CT: Type 1 cipher (ANDVT, KG-84, Vinson). CC: Citadel cover, depot-controlled. LD: load Type 1 keys. RV: receive variable, OTAR rekey. Z: zeroize, pull-to-turn. Z set with no power zeroizes at next power-up | Functions CONFIRMED (H150 Table 3-3 item 8, 3.7). Dial order INFERRED |
| Normal use | PT, CC or CT | | CONFIRMED (3.5.1) |
| Angle | not published | ESTIMATE 7 positions, about 40 to 45 degrees apart. Only Z needs a pull | ESTIMATED |
| Power | knob: OFF to PT/CC/CT powers on. No button | | CONFIRMED |
| Volume | KEY: VOL +/- on the KDU. No volume knob | VOL and bar graph replace BAT briefly, then revert. Not adjustable in program mode. Radio silence drops volume to minimum | CONFIRMED (3.15.13) |
| Keypad | circular-arrow NEXT, CALL, LT, MODE, SQ, ZERO, OPT, PGM, CLR, ENT, VOL +/-, NET or PRE +/-, arrows | CALL differs per mode (FIX/ALE/HOP/3G). MODE changes FIX/ALE/HOP/3G/3G+. SQ toggles squelch. The manual table says the ZERO position is "Not used" | CONFIRMED |

Lighting
- LT opens the backlight menu: brightness 0 to 7, contrast 20 to 100 percent in 5 percent steps, LIGHT OPERATION = ON, OFF or MOMENTARY. MOMENTARY: backlight off after 10 s without a keypress. ON only with external DC or a single battery on J10. CONFIRMED (3.15.12).
- Radio silence sets light intensity to zero and volume to minimum. CONFIRMED.
- BAT meter flashes when battery critically low. R in upper-left flashes under radio silence. CONFIRMED.
- Colour of display/keys, key-press lighting: not in the manual. INFERRED same KDU family as 117G, one backlight for display and keys.
- LEDs: none described. TX/RX shown on the LCD. Nothing moves.

---
## 3. AN/PRC-152 / 152A (Falcon III handheld)

The in-game picture (black drum knob "OFF 1 2 3...", grey cylinder, small black part to its right) fits: black knob = function knob, grey cylinder = TNC antenna base, small black part = separate cipher switch. That mapping is INFERRED from layout and from the documented switch set.

| Control | Positions | Behaviour | Tag |
|---|---|---|---|
| Function knob | OFF, 1, 2, 3, 4, 5, S, F, Z (9 positions) | OFF = off (white dot mark, Marine training slide). 1 to 5 = presets. S = scan. F = front panel, full programming from keypad. Z = zeroize all | DS152A "Function Knob: Off, 1-5 presets, scan, front panel, Z-All" CONFIRMED. A real "1 to 10" run is NOT confirmed: reality is 5 numbered positions plus S, F, Z |
| Angle | 9-position rotary at 36 degree increments (TRI write-up about the original) | 8 steps x 36 = 288 degrees travel, end stops | secondary source, treat as ESTIMATE |
| Cipher switch | PT, LD, CT | PT clear. LD key load. CT cipher text | Set CONFIRMED (DS152A row text "PT, LD & CT", Marine slide); location INFERRED |
| Power | function knob OFF position. No button | | CONFIRMED (ACRE2, slide) |
| Volume | left-side up/down buttons above the PTT (ACRE2 models it this way); LCD shows level while adjusting | | game behaviour CONFIRMED, real hardware INFERRED. Not a knob. (TRI replica has a top volume pot, it is not the real unit) |
| Keypad | CLR, ENT, PRE +/-, arrows, legends CALL, LT, MODE, SQL, ZERO, OPT, PGM like the other Harris KDU radios | | legends INFERRED from family and ACRE2 text (7-OPT, 8-PGM) |

Lighting
- Display is NVG compatible. CONFIRMED (datasheet/Wikipedia). Backlight colour and keypad colour not sourced; INFERRED green, LT opens the same Harris light menu (momentary or none).
- Individual key lighting and LEDs: not described. INFERRED none. (TRI replica adds a red/green LED; the original is not described.)
- TX/RX: LCD signal bar appears during TX/RX of audio (TRI doc, replica). Nothing physical moves.

---
## 4. R-168 family (Akveduk), R-168-5UN(E) / 25U style

Weakest ledger: no readable operator manual for this exact panel. Facts found:
- R-168-25U-2 manual (https://ozlib.com/853686/tehnika/upravlenie_100u, https://studfile.net/preview/13584958/page:118/): menu keys РЖ, НАСТР, ВВ, ОТМ and arrows; display brightness is a menu item "Ярк. дисп. ПУ" scale 0 to 7; menu has Дисплей (brightness, LCD contrast, heating), Громкость, Выбор антенны, Реч. информ.; СРД = emergency erase of radio data. Its panel is a menu panel (two panels ПП1/ПП2, key РС), a different layout from your picture. CONFIRMED for that set.
- Older R-168-0,1У (https://studfile.net/preview/7510097/page:8/): power toggle ВКЛ./ВЫКЛ., РАБ-ЭК toggle, mode switch with ЗП (write), ЧМ (open telephone), 16; channel switch К1 to К8; keys РЖ (its LED lights when active), Т (next slot / swap TX-RX), С (clear), В (tone call about 1000 Hz), ПШ, ТМ (speech masking, LED lights when active), А, О, М, 1 to 8; volume is the ГР BUTTON; display (табло) shows text like ЗАП1, СВЯЗЬ А. Colour not stated. CONFIRMED for that set.
- R-168-5UN catalogue text (https://www.sozvezdie.su/catalog/sredstva_svyazi/radiostantsiya_nosimaya_metrovogo_diapazona/): indicator panel shows frequencies, speech-masking keys and modes; a separate LED shows connected antenna resistance. Colours not stated. CONFIRMED.

The in-game picture, read against these sources:

| Control | Reading | Tag |
|---|---|---|
| ПИТ ВКЛ/ОТКЛ toggle | power toggle, 2 positions | INFERRED (0,1У has the same type) |
| МШ / МАЛ toggle | not found; do not assign a meaning | UNKNOWN |
| РРУ / АРУ toggle | manual / automatic gain control (standard Russian abbreviations) | INFERRED |
| Mode switch ТЛФ, АТ, ЧТ, ЗП, СРД | ТЛФ telephone (confirmed term). ЗП write radio data (confirmed 0,1У). СРД emergency erase of radio data (confirmed 25У-2): never a normal-use position. ЧТ probably read. АТ unknown. Normal use: ТЛФ | ТЛФ, ЗП, СРД CONFIRMED in family; ЧТ INFERRED; АТ UNKNOWN |
| КАНАЛ knob 1 to 7 + СРД | channel select; СРД = erase. 0,1У has К1 to К8 | INFERRED |
| ГРОМКОСТЬ knob (0 mark) | rotary volume, 0 = minimum | INFERRED (0,1У uses a ГР button, 25У-2 a menu; this knob is undocumented) |
| ТЛФ and ТЛГ knobs | ТЛГ = telegraph (confirmed term). Probably level knobs; function not confirmed | INFERRED |
| Keypad РЖ, В, А, ЗК, ТМ, Н, digits, red С and Т | РЖ mode (LED), В call, С clear, Т next slot / swap TX-RX, ТМ speech masking (LED), А address. ЗК and Н not found | partly CONFIRMED in family |
| Display | amber segment display per the in-game picture; no source states colour | INFERRED from picture |

Angles: none published. ESTIMATE from photos: КАНАЛ 8 detents about 36 to 45 degrees; mode switch 5 detents about 45 degrees; ГРОМКОСТЬ about 270 degrees travel.
Lights/moves on TX/RX: not documented. INFERRED: табло text (СВЯЗЬ etc.); nothing physical.

Recommendation (followed): model the picture, but only animate confirmed things (power toggle, mode and channel detents, СРД as an erase position, РЖ and ТМ LEDs, display text). Keep МШ, РРУ/АРУ and the ТЛФ/ТЛГ knobs cosmetic until a manual for this exact panel is found (R-168-5УН-2 manual, section "Органы управления и индикации").

---
## 5. R-187-P1 Azart (handheld)

| Control | Facts | Tag |
|---|---|---|
| Keypad | digits 1 to 9, star (bell symbol), 0, hash (key symbol); two soft keys (left and right); UP and DOWN keys; green handset key ("Поднять трубку", call/answer); red key ("Отм. вкл./откл.", cancel and power) | CONFIRMED (AZ p.9 figure) |
| Power | no switch. On: press and HOLD the red key, then after self-test and the menu press "0" (open mode). Off: hold the red key until "Выключение питания". Emergency erase of keys and radio data: press "*", "0" and the red key together, confirm ДА | CONFIRMED (AZ pp.12-13) |
| Volume | KEYS: press UP, then UP/DOWN (works mid-call), or menu > громкость > выбрать. No knob | CONFIRMED (AZ p.12) |
| Squelch | DOWN then UP/DOWN, analog modes only | CONFIRMED (AZ p.13) |
| Keypad lock | unlock = DOWN then UP | CONFIRMED |
| Side and top | PTT ("Тангента"), direction joystick, Bluetooth antenna on the left side; IR window, antenna and speaker on top; accessory socket on the right | CONFIRMED (AZ p.9, cryptomuseum) |
| Flip cover | none in the manual figures. The real keypad has no flip cover | INFERRED from AZ pp.9-10 |
| Display | colour LCD with backlight; firmware 2.3 adds backlight always on. Colours not stated | existence CONFIRMED |
| Keypad backlight | not described (keys look dark grey with white legends) | INFERRED same timeout as display |
| LEDs | none described | INFERRED none |
| TX/RX | not described; INFERRED a display icon. Nothing moves | INFERRED |

---
## Reality versus the in-game pictures
1. 117G: picture matches. Volume is keys only; power is the OFF position; OFF and Z need a pull. LT opens a menu (momentary or none), it is not a plain toggle.
2. 150(C): picture order plausible but unverified; manual lists OFF, PT, CT, CC, LD, RV, Z. Z needs a pull. Volume is keys.
3. 152: real function knob is OFF, 1 to 5, S, F, Z (9 positions), not a long numbered run. The small black part is most likely the PT/LD/CT cipher switch. Volume is side buttons, not a knob.
4. R-168: the picture's panel could not be sourced; only power, ТЛФ, ЗП, СРД, РЖ and ТМ are confirmed in the family. СРД must act as an erase position, not a channel.
5. Azart: no flip cover on the real keypad. Power is a held red key, volume is keys. Green and red keys are call and end/power.
