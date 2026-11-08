"""Rebuilds the display and keys of the five Bifrost VON radio layouts from the original Coalition layouts (commit BASE).

- Removes the old two-line display (RadioText, Frequency, Line3).
- Adds the LCD lines read by CVON_RadioMenu: LcdGlow (backlight), LcdStatus, LcdStatusRight, LcdName, LcdMain,
  and LcdInfo where the screen is big enough.
- Adds VolUp/VolDown hot spots over the volume controls drawn on the radio (not on the PRC-152, which has none).
- Rewrites the Help overlay labels for the new key functions and adds a short key legend.
Run from the repo root: python gen_radio_lcd.py
"""
import hashlib, subprocess

# The original Coalition layouts the rebuild starts from (bifrost/offshoot's first commit)
BASE = '1ef7c5d'
DIGI = '{D3E3853DAEBA9BD6}UI/fonts/DS_DIGI.fnt'
BOLD = '{EABA4FE9D014CCEF}UI/Fonts/RobotoCondensed/RobotoCondensed_Bold.fnt'
_seen = set()

def wid(radio, name, salt=''):
    h = '6D' + hashlib.sha1(('bvon-lcd:' + radio + ':' + name + salt).encode()).hexdigest()[2:16].upper()
    assert h not in _seen, (radio, name, salt)
    _seen.add(h)
    return '{' + h + '}'

def slot(radio, name, x, y, w, h, ind):
    p = ' ' * ind
    return [p + 'Slot FrameWidgetSlot "%s" {' % wid(radio, name, ':slot'), p + ' Anchor 0.5 0.5 0.5 0.5',
            p + ' PositionX %g' % x, p + ' OffsetLeft %g' % x, p + ' PositionY %g' % y, p + ' OffsetTop %g' % y,
            p + ' SizeX %g' % w, p + ' OffsetRight %g' % -(x + w), p + ' SizeY %g' % h, p + ' OffsetBottom %g' % -(y + h), p + '}']

def color(c):
    return 'Color %g %g %g %g' % c

def image(radio, name, x, y, w, h, c, ind=2, visible=True):
    p = ' ' * ind
    out = [p + 'ImageWidgetClass "%s" {' % wid(radio, name), p + ' Name "%s"' % name] + slot(radio, name, x, y, w, h, ind + 1)
    if not visible:
        out.append(p + ' "Is Visible" 0')
    out += [p + ' ' + color(c), p + '}']
    return out

def text(radio, name, x, y, w, h, t, size, c, font=DIGI, align='Left', ind=2, shadow=0):
    p = ' ' * ind
    out = [p + 'TextWidgetClass "%s" {' % wid(radio, name), p + ' Name "%s"' % name] + slot(radio, name, x, y, w, h, ind + 1)
    out += [p + ' ' + color(c), p + ' Text "%s"' % t, p + ' "Font Size" %d' % size, p + ' "Min Font Size" %d' % max(8, int(size * 0.7)),
            p + ' "Horizontal Alignment" %s' % align, p + ' "Vertical Alignment" Center',
            p + ' FontProperties FontProperties "%s" {' % wid(radio, name, ':font'), p + '  Font "%s"' % font]
    if shadow:
        out += [p + '  ShadowSize %d' % shadow, p + '  ShadowOffset 0 0', p + '  ShadowOpacity 0.8']
    out += [p + ' }', p + '}']
    return out

def tex_rect(tx, ty, tw, th):
    """Texture pixels (2560 x 1440, the radio pictures) to layout units from the centre."""
    return (tx * 0.75 - 960, ty * 0.75 - 540, tw * 0.75, th * 0.75)

FULL = (-960, -540, 1920, 1080)

def picture(radio, name, rect, texture, size, visible=True, ind=2):
    x, y, w, h = rect
    p = ' ' * ind
    out = [p + 'ImageWidgetClass "%s" {' % wid(radio, name), p + ' Name "%s"' % name] + slot(radio, name, x, y, w, h, ind + 1)
    if not visible:
        out.append(p + ' "Is Visible" 0')
    out += [p + ' Texture "%s"' % texture, p + ' Size %d %d' % size, p + '}']
    return out

def button(radio, name, x, y, w, h, tip):
    p = '  '
    return [p + 'ButtonWidgetClass "%s" {' % wid(radio, name), p + ' Name "%s"' % name] + slot(radio, name, x, y, w, h, 3) + [
        p + ' "Z Order" 1',
        p + ' Tooltip TextWidgetTooltip "%s" {' % wid(radio, name, ':tip'), p + '  Text "%s"' % tip, p + ' }',
        p + ' Color 1 1 1 0',
        p + ' components {', p + '  SCR_ButtonTextComponent "%s" {' % wid(radio, name, ':comp'), p + '  }', p + ' }', p + '}']

HARRIS_HELP = {'Menu Up': 'Next preset', 'Menu Down': 'Prev preset', 'Clear': 'Type freq / back',
               'Enter': 'Store / select', 'Power': 'Power on / off'}
LCD_GREEN = (0.62, 1.0, 0.45, 0.32)

# Animated knobs and lights (art from gen_radio_art.py). Pictures go right above the radio picture, under the
# key hot spots; textures are the imported .edds resources.
def keys_lit(radio, guid):
    return [('KeysLit', FULL, '{%s}UI/layouts/Radios/%s/data/%sKeys.edds' % (guid, radio, radio), (2560, 1440), False)]

R168_TEX = 'UI/layouts/Radios/R168/data/'
R168_PICTURES = [
    ('ChannelShadow', tex_rect(1855, 785, 264, 264), '{F0681274B159786D}' + R168_TEX + 'R168ChannelShadow.edds', (264, 264), True),
    ('ChannelKnob', tex_rect(1849, 771, 264, 264), '{8A402413CB1B6BC1}' + R168_TEX + 'R168Channel.edds', (264, 264), True),
    ('VolumeIndex', tex_rect(2164, 923, 112, 112), '{0BFE4BA3D3CA39DA}' + R168_TEX + 'R168VolumeIndex.edds', (112, 112), True),
    ('ModeLight', tex_rect(1094, 803, 80, 68), '{BF433C4608425926}' + R168_TEX + 'R168KeyLight.edds', (80, 68), False),
    ('MaskLight', tex_rect(1090, 1091, 80, 68), '{BF433C4608425926}' + R168_TEX + 'R168KeyLight.edds', (80, 68), False),
]
R168_BUTTONS = [('ChannelDown',) + tex_rect(1919, 841, 62, 124) + ('Channel knob: turn left',),
                ('ChannelUp',) + tex_rect(1981, 841, 62, 124) + ('Channel knob: turn right',),
                ('ModeKey',) + tex_rect(1094, 804, 80, 66) + ('Mode (options)',)]
# AN/PRC-152 top function knob: the drum frames are swapped by CVON_RadioMenu; two halves turn it
PRC152_KNOB = ('Knob', tex_rect(1286, 140, 104, 116), '{3DB0A91F32090092}UI/layouts/Radios/ANPRC152/data/ANPRC152Knob_1.edds', (104, 116), True)
PRC152_BUTTONS = [('KnobDown',) + tex_rect(1286, 140, 52, 116) + ('Turn knob left',),
                  ('KnobUp',) + tex_rect(1338, 140, 52, 116) + ('Turn knob right',)]

# Rects are layout units (1920x1080 reference, from the centre), measured on the radio pictures.
# lines: (name, top offset in the LCD, height, font size, align)
RADIOS = {
    'ANPRC152': dict(
        bars=(108, 0, 14),
        lcd=(-79, -113, 212, 66), pad=(7, 4), glow=LCD_GREEN,
        lines=[('LcdStatus', 0, 15, 15), ('LcdName', 15, 16, 16), ('LcdMain', 31, 27, 29)],
        vol=None,
        help=dict(HARRIS_HELP, **{'Menu Left': 'Volume down', 'Menu Right': 'Volume up', 'Power': 'Knob: OFF, presets 1-5, S, F'}),
        legend=(-900, -175), legend_lines=['Top knob  click a half to turn: OFF, presets 1-5, S scan, F front panel',
                                           '7 OPT  options: volume, power, ears, monitor, light, clock', '8 PGM  type a new frequency',
                                           '2 LT  light: off / auto / on', '6 and 9  up and down inside menus', 'Number pad  type a frequency'],
        pictures=keys_lit('ANPRC152', 'B1D95022EE5B0282'), knob=PRC152_KNOB, buttons=PRC152_BUTTONS, remove=['Power']),
    'ANPRC117': dict(
        bars=(262, 1, 26),
        lcd=(-195, -201, 514, 171), pad=(16, 10), glow=LCD_GREEN,
        lines=[('LcdStatus', 0, 28, 28), ('LcdName', 30, 32, 30), ('LcdMain', 62, 56, 58), ('LcdInfo', 120, 30, 26)],
        vol=[('VolUp', -218, 40, 76, 68, 'Volume up'), ('VolDown', -218, 110, 76, 68, 'Volume down')],
        help=dict(HARRIS_HELP, **{'Menu Left': 'Options', 'Menu Right': 'Options', 'Power': 'Knob: OFF / PT / CT'}),
        legend=(-900, -525), legend_lines=['VOL + / -  volume', '7 OPT  options: power, ears, monitor, light, clock', '8 PGM  type a new frequency',
                                          '2 LT  light: off / auto / on     6 and 9  up and down inside menus', 'Number pad  type a frequency'],
        pictures=keys_lit('ANPRC117', 'CF959279DAF8E047')),
    'ANPRC150': dict(
        bars=(150, 0, 16),
        lcd=(-408, -180, 293, 88), pad=(8, 4), glow=LCD_GREEN,
        lines=[('LcdStatus', 0, 17, 17), ('LcdName', 17, 18, 17), ('LcdMain', 35, 29, 30), ('LcdInfo', 64, 16, 15)],
        vol=[('VolUp', -476, 38, 52, 62, 'Volume up'), ('VolDown', -476, 100, 52, 62, 'Volume down')],
        help=dict(HARRIS_HELP, **{'Menu Left': 'Options', 'Menu Right': 'Options', 'Power': 'Knob: OFF / PT / CT'}),
        legend=(-900, -525), legend_lines=['VOL + / -  volume', '7 OPT  options: power, ears, monitor, light, clock', '8 PGM  type a new frequency',
                                          '2 LT  light: off / auto / on     6 and 9  up and down inside menus', 'Number pad  type a frequency'],
        pictures=keys_lit('ANPRC150', '3D4DF89DE90A8BF0')),
    'R168': dict(
        bars=(200, 0, 20),
        lcd=(-151, -101, 405, 102), pad=(12, 6), glow=(0.55, 0.06, 0.03, 0.35),
        lines=[('LcdStatus', 0, 22, 22), ('LcdMain', 22, 44, 46), ('LcdName', 66, 24, 22)],
        vol=[('VolDown', 645, 148, 45, 90, 'Volume down'), ('VolUp', 690, 148, 45, 90, 'Volume up')],
        help={'Menu Up': 'Next channel', 'Menu Down': 'Prev channel', 'Menu Left': 'Options', 'Menu Right': 'Options',
              'Clear': 'Clear / back', 'Enter': 'Store', 'Power': 'Power on / off'},
        legend=(-900, -505), legend_lines=['Volume knob  left half down, right half up', 'КАНАЛ knob  click a half to change channel 1-7',
                                           'Digits  type a frequency, T stores it',
                                           'РЖ, TM or H  options: power, ears, monitor, light, clock (РЖ lights up)',
                                           '+ / -  change a value inside options', 'Number pad  type a frequency'],
        pictures=R168_PICTURES, buttons=R168_BUTTONS),
    'R187': dict(
        bars=(2, 124, 24),
        lcd=(-70, -240, 132, 172), pad=(6, 8), glow=LCD_GREEN,
        lines=[('LcdStatus', 0, 18, 17), ('LcdName', 24, 20, 18), ('LcdMain', 52, 36, 36), ('LcdInfo', 96, 18, 13)],
        vol=[('VolUp', -133, -224, 20, 34, 'Volume up'), ('VolDown', -133, -150, 20, 34, 'Volume down')],
        help={'Menu Up': 'Next channel', 'Menu Down': 'Prev channel', 'Menu Left': 'Options', 'Menu Right': 'Options',
              'Clear': 'Back (hold: power)', 'Enter': 'Store', 'Power': ''},
        legend=(-900, -300), legend_lines=['Red key  hold 1 s to switch on or off', 'Side keys  volume',
                                           'Digits  type a frequency, green key stores it',
                                           'Top soft keys  options: power, ears, monitor, light, clock', 'Up / down  change a value inside options',
                                           'Number pad  type a frequency'],
        remove=['Knob', 'Power']),
}

def block_range(lines, name, indent):
    """Start and end line of the widget block named name at the given indent."""
    p = ' ' * indent
    for i, l in enumerate(lines):
        if l.startswith(p) and not l.startswith(p + ' ') and 'WidgetClass "{' in l and lines[i + 1].strip() == 'Name "%s"' % name:
            for j in range(i + 1, len(lines)):
                if lines[j] == p + '}':
                    return i, j
    raise KeyError(name)

def text_color(lines, start, end):
    for l in lines[start:end]:
        if l.startswith('   Color '):
            return tuple(float(v) for v in l.split()[1:5])
    return (0, 0, 0, 1)

for radio, cfg in RADIOS.items():
    path = 'CoalitionVON/UI/layouts/Radios/%s/%s.layout' % (radio, radio)
    src = subprocess.run(['git', 'show', BASE + ':' + path], capture_output=True, text=True, encoding='utf-8', check=True).stdout
    lines = src.replace('\r\n', '\n').split('\n')

    s, e = block_range(lines, 'Frequency', 2)
    ink = text_color(lines, s, e)
    for old in ('RadioText', 'Frequency', 'Line3'):
        s, e = block_range(lines, old, 2)
        del lines[s:e + 1]

    x, y, w, h = cfg['lcd']
    px, py = cfg['pad']
    lcd = image(radio, 'LcdGlow', x + 2, y + 2, w - 4, h - 4, cfg['glow'], visible=False)
    for name, top, lh, size in cfg['lines']:
        lcd += text(radio, name, x + px, y + py + top, w - 2 * px, lh, '', size, ink)
        if name == 'LcdStatus':
            lcd += text(radio, 'LcdStatusRight', x + px, y + py + top, w - 2 * px, lh, '', size, ink, align='Right')
    # TX / RX marker and signal bars (lit = ink; CVON_RadioMenu dims the unlit ones)
    bx, by, bh = cfg['bars']
    ox, oy = x + px + bx, y + py + by
    mw = round(bh * 0.8)
    lcd += text(radio, 'LcdLink', ox, oy, mw, bh, '', bh, ink)
    bw, gap = max(3, round(bh * 0.22)), max(2, round(bh * 0.12))
    for n in range(5):
        hgt = round(bh * (0.4 + 0.15 * n))
        lcd += image(radio, 'LcdBar%d' % n, ox + mw + gap + n * (bw + gap), oy + bh - hgt, bw, hgt, ink)
    _, e = block_range(lines, 'Image0', 2)
    lines[e + 1:e + 1] = lcd

    if cfg['vol']:
        s, _ = block_range(lines, 'Power', 2)
        keys = []
        for name, kx, ky, kw, kh, tip in cfg['vol']:
            keys += button(radio, name, kx, ky, kw, kh, tip)
        lines[s:s] = keys

    # Animated knobs and lights: pictures right above the radio picture, a replaced knob, extra hot spots, and
    # parts the real radio doesn't have (the R-187's keypad cover) taken out
    for name in cfg.get('remove', []):
        s, e = block_range(lines, name, 2)
        del lines[s:e + 1]
    if cfg.get('knob'):
        name, rect, tex, size, visible = cfg['knob']
        s, e = block_range(lines, name, 2)
        lines[s:e + 1] = picture(radio, name, rect, tex, size, visible)
    if cfg.get('pictures'):
        _, e = block_range(lines, 'Image0', 2)
        pics = []
        for name, rect, tex, size, visible in cfg['pictures']:
            pics += picture(radio, name, rect, tex, size, visible)
        lines[e + 1:e + 1] = pics
    if cfg.get('buttons'):
        s, _ = block_range(lines, 'Panel0', 2)
        keys = []
        for name, kx, ky, kw, kh, tip in cfg['buttons']:
            keys += button(radio, name, kx, ky, kw, kh, tip)
        lines[s:s] = keys

    # Help overlay: new label texts, plus a legend for the function keys
    hs, he = block_range(lines, 'HelpFrame', 2)
    i = hs
    while i < he:
        stripped = lines[i].strip()
        if stripped.startswith('Text "') and lines[i].startswith('     ') and stripped[6:-1] in cfg['help']:
            lines[i] = '     Text "%s"' % cfg['help'][stripped[6:-1]]
            # slot of this label: the lines above, back to its Slot line
            s0 = i
            while not lines[s0].strip().startswith('Slot FrameWidgetSlot'):
                s0 -= 1
            vals = {}
            for j in range(s0, i):
                parts = lines[j].split()
                if len(parts) == 2 and parts[0] in ('OffsetLeft', 'OffsetRight'):
                    vals[parts[0]] = float(parts[1])
            left, right = vals['OffsetLeft'], vals['OffsetRight']
            centre = left + (1920 - left - right) / 2
            if centre > 1100:
                align, right = 'Left', right - 300
            elif centre < 820:
                align, left = 'Right', left - 300
            else:
                align, left, right = 'Center', left - 80, right - 80
            for j in range(s0, i):
                key = lines[j].split()[0] if lines[j].split() else ''
                if key in ('PositionX', 'OffsetLeft'):
                    lines[j] = '      %s %g' % (key, left)
                elif key == 'OffsetRight':
                    lines[j] = '      OffsetRight %g' % right
                elif key == 'SizeX':
                    lines[j] = '      SizeX %g' % -(left + right)
            k = i + 1
            while lines[k] != '    }':
                k += 1
            for j in range(i + 1, k):
                if lines[j].strip().startswith('"Horizontal Alignment"'):
                    lines[j] = '     "Horizontal Alignment" %s' % align
            label_id = stripped[6:-1]
            shadow = ['     FontProperties FontProperties "%s" {' % wid(radio, 'help:' + label_id + str(i), ':font'),
                      '      Font "%s"' % BOLD, '      ShadowSize 4', '      ShadowOffset 0 0', '      ShadowOpacity 0.85', '     }']
            lines[k:k] = shadow
            he += len(shadow)
            i = k + len(shadow)
        i += 1
    lx, ly = cfg['legend']
    legend = image(radio, 'HelpLegendBg', lx - 14, ly - 10, 660, 34 * len(cfg['legend_lines']) + 20, (0, 0, 0, 0.62), ind=4)
    for n, t in enumerate(cfg['legend_lines']):
        legend += text(radio, 'HelpLegend%d' % n, lx, ly + 34 * n, 640, 32, t, 26, (1, 1, 1, 1), font=BOLD, ind=4, shadow=4)
    _, he = block_range(lines, 'HelpFrame', 2)
    # children list closes with '   }' right before the block's own '  }'
    assert lines[he - 1] == '   }', lines[he - 1]
    lines[he - 1:he - 1] = legend

    out = '\r\n'.join(lines)
    open(path, 'w', encoding='utf-8', newline='').write(out)
    print(radio, 'ink', ink, 'lines', len(lines))
