"""Generates UI/layouts/HUD/VON/BVON_RadioPopup.layout: the Bifrost VON radio pop-up (talking, receiving,
channel and ear changes). Original layout, 1920x1080 reference space."""
import hashlib, sys

BOLD = '{EABA4FE9D014CCEF}UI/Fonts/RobotoCondensed/RobotoCondensed_Bold.fnt'
REG = '{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt'
_seen = set()

def wid(name, salt=''):
    h = '6C' + hashlib.sha1(('bvon-popup:' + name + salt).encode()).hexdigest()[2:16].upper()
    assert h not in _seen, name
    _seen.add(h)
    return '{' + h + '}'

def slot(name, x, y, w, h, anchor=(0, 0, 0, 0)):
    return ['Slot FrameWidgetSlot "%s" {' % wid(name, ':slot'), ' Anchor %g %g %g %g' % anchor,
            ' PositionX %g' % x, ' OffsetLeft %g' % x, ' PositionY %g' % y, ' OffsetTop %g' % y,
            ' SizeX %g' % w, ' OffsetRight %g' % -(x + w), ' SizeY %g' % h, ' OffsetBottom %g' % -(y + h), '}']

def widget(cls, name, s, props, children=()):
    out = ['%s "%s" {' % (cls, wid(name)), ' Name "%s"' % name] + [' ' + l for l in s] + [' ' + p for p in props]
    if children:
        out.append(' {')
        for c in children:
            out += ['  ' + l for l in c]
        out.append(' }')
    out.append('}')
    return out

def panel(name, x, y, w, h, c):
    return widget('ImageWidgetClass', name, slot(name, x, y, w, h), ['Color %g %g %g %g' % c])

def text(name, x, y, w, h, t, size, c, font=BOLD, align='Left'):
    return widget('TextWidgetClass', name, slot(name, x, y, w, h), [
        'Color %g %g %g %g' % c, 'Text "%s"' % t, '"Font Size" %d' % size, '"Min Font Size" %d' % max(10, size - 6),
        '"Horizontal Alignment" %s' % align, '"Vertical Alignment" Center',
        'FontProperties FontProperties "%s" {' % wid(name, ':font'), ' Font "%s"' % font, ' ShadowSize 2', '}'])

# Compact card (2026-11-07, Bryce: "take up a bit too much UI"): row 1 radio + TX/RX, row 2 channel,
# frequency, net + ear, row 3 the caller only while a call comes in. CVON_HUD.FillPopup sets the height
# (48 without the caller, 68 with) and trims long names so nothing runs past the edge.
W, H = 240, 68
kids = [
    panel('PopupBg', 0, 0, W, H, (0.035, 0.04, 0.035, 0.82)),
    panel('PopupAccent', 0, 0, 4, H, (0.56, 0.78, 0.36, 1)),
    text('PopupRadio', 12, 4, 156, 22, 'AN/PRC-152', 17, (0.95, 0.96, 0.93, 1)),
    text('PopupState', 168, 5, 62, 20, 'RX', 14, (0.56, 0.78, 0.36, 1), align='Right'),
    text('PopupChannel', 12, 25, 176, 20, 'CH 01  31120', 15, (0.95, 0.96, 0.93, 1)),
    text('PopupEars', 188, 25, 42, 20, 'LEFT', 12, (0.62, 0.66, 0.60, 1), font=REG, align='Right'),
    text('PopupSpeaker', 12, 45, 218, 20, 'Sgt Test', 15, (0.56, 0.78, 0.36, 1)),
]
popup = widget('FrameWidgetClass', 'BVON_RadioPopup', slot('BVON_RadioPopup', 28, -210, W, H, (0, 0.5, 0, 0.5)), ['"Is Visible" 0'], kids)
root = ['FrameWidgetClass {', ' Name "rootFrame"', ' {'] + ['  ' + l for l in popup] + [' }', '}']
data = '\r\n'.join(root) + '\r\n'
open(sys.argv[1], 'w', newline='').write(data)
print('widgets', len(_seen), 'bytes', len(data))
