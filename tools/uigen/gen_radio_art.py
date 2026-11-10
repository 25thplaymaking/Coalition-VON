"""Builds the extra radio art Bifrost VON animates and lights. Run from the repo root:
    uv run --with pillow --with numpy --with opencv-python-headless python tools/uigen/gen_radio_art.py

Writes PNGs next to each radio's textures (UI/layouts/Radios/<radio>/data/); Workbench imports them as .edds.
Everything is cut from the radios' own pictures (the original Coalition art, git BASE), nothing drawn from
another mod. Coordinates are texture pixels (2560 x 1440).

- <radio>Keys.png (AN/PRC-117, -150, -152): keypad backlight. The legends printed on the keys, lit NVG green,
  shown with the display backlight (Harris radios light keypad and display together).
- ANPRC152Knob_<pos>.png: the top function knob (a drum seen side-on) at each position OFF 1 2 3 4 5 S F Z,
  36 degrees apart, the labels redrawn round the drum.
- R168.png: the panel with the channel (KANAL) lever painted out, so the lever can turn.
- R168Channel.png / R168ChannelShadow.png: that lever and its shadow, centred on the knob's hub.
- R168VolumeIndex.png: an index line for the volume knob cap (the panel has 0 and max marks for it).
- R168KeyLight.png: the light of a key with an LED (the R-168's mode key).
"""
import io, math, os, subprocess
import numpy as np
import cv2
from PIL import Image, ImageDraw, ImageFilter, ImageFont

BASE = '1ef7c5d'
ROOT = 'CoalitionVON/UI/layouts/Radios'
NVG = (150, 255, 135)

def git_png(path):
    data = subprocess.run(['git', 'show', BASE + ':' + path], capture_output=True, check=True).stdout
    return Image.open(io.BytesIO(data)).convert('RGBA')

def data(radio, name):
    return '/'.join((ROOT, radio, 'data', name))

def save(img, radio, name):
    assert img.width % 4 == 0 and img.height % 4 == 0, (name, img.size)
    img.save(data(radio, name))
    print('wrote', data(radio, name), img.size)

# ---------------------------------------------------------------------------------------------------------------
# Keypad backlight: key rects (x, y, w, h) from the layouts' key buttons
KEYS = {
    'ANPRC117': [(1154,768,106,80),(1310,768,106,80),(1466,768,106,80),(1154,878,106,80),(1310,878,106,80),(1466,878,106,80),
                 (1154,981,106,80),(1310,981,106,80),(1466,981,106,80),(1156,1069,106,80),(986,980,106,80),(1634,977,106,80),
                 (1310,1069,106,80),(1466,1069,106,80),(1632,768,106,93),(1632,868,106,92),(989,773,101,90),(989,866,101,90)],
    'ANPRC150': [(749,682,66,66),(849,682,66,66),(939,680,66,66),(750,768,66,66),(850,768,66,66),(940,766,66,66),
                 (751,860,66,66),(851,860,66,66),(942,858,66,66),(648,680,66,66),(1038,768,66,66),(1039,860,66,66),
                 (1036,682,66,66),(1139,682,66,66),(1139,774,66,74),(1138,851,66,77),(645,770,69,82),(645,853,69,82)],
    'ANPRC152': [(1202,727,46,33),(1264,727,46,33),(1326,727,46,33),(1202,777,46,33),(1264,777,46,33),(1326,777,46,33),
                 (1202,829,46,33),(1264,829,46,33),(1326,829,46,33),(1202,877,46,33),(1386,727,46,33),(1386,777,46,33),
                 (1264,877,46,33),(1326,877,46,33),(1387,822,46,89)],
}

# How much brighter than the key cap a pixel must be to count as legend (the PRC-152's caps have grey rims)
LEGEND_STEP = {'ANPRC117': 28, 'ANPRC150': 28}
WHITE_LEGENDS = {'ANPRC152': 110}
RIM_PX = 4

def keypad_light(radio):
    base = np.asarray(git_png(data(radio, radio + '.png')).convert('RGB')).astype(np.float32)
    lum = base @ np.array([0.299, 0.587, 0.114], np.float32)
    mask = np.zeros(lum.shape, np.float32)
    for x, y, w, h in KEYS[radio]:
        x0, y0, x1, y1 = x + 2, y + 2, x + w - 2, y + h - 2
        key = lum[y0:y1, x0:x1]
        if radio in WHITE_LEGENDS:
            # Legends as bright as the caps' rims: the rim is the one bright outline running round the whole
            # key, so drop any bright patch as wide or as tall as the key and keep the marks inside
            x0, y0, x1, y1 = x - 4, y - 4, x + w + 4, y + h + 4
            key = lum[y0:y1, x0:x1]
            bright = (key > WHITE_LEGENDS[radio]).astype(np.uint8)
            n, lab, stats, _ = cv2.connectedComponentsWithStats(bright, connectivity=8)
            inside = np.zeros_like(bright)
            for c in range(1, n):
                cw, ch = stats[c, cv2.CC_STAT_WIDTH], stats[c, cv2.CC_STAT_HEIGHT]
                if cw >= 0.75 * w or ch >= 0.85 * h:
                    # the rim (with any legend touching it): fill its outline, then shave the rim off
                    contours, _ = cv2.findContours((lab == c).astype(np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
                    shape = np.zeros_like(bright)
                    cv2.drawContours(shape, contours, -1, 1, thickness=-1)
                    inside |= cv2.erode(shape, np.ones((3, 3), np.uint8), iterations=RIM_PX)
            keep = (bright > 0) & (inside > 0)
            m = np.clip((key - WHITE_LEGENDS[radio]) / 45.0, 0, 1) * keep
        else:
            # Light legends on dark key caps: everything well above the cap's own shade
            cap = np.percentile(key, 35)
            m = np.clip((key - cap - LEGEND_STEP[radio]) / 70.0, 0, 1)
        mask[y0:y1, x0:x1] = np.maximum(mask[y0:y1, x0:x1], m)
    m = Image.fromarray((mask * 255).astype(np.uint8))
    glow = m.filter(ImageFilter.GaussianBlur(3))
    alpha = np.maximum(np.asarray(m, np.float32), np.asarray(glow, np.float32) * 0.55)
    out = np.zeros((lum.shape[0], lum.shape[1], 4), np.uint8)
    out[..., 0], out[..., 1], out[..., 2] = NVG
    out[..., 3] = np.clip(alpha * 0.92, 0, 255).astype(np.uint8)
    save(Image.fromarray(out, 'RGBA'), radio, radio + 'Keys.png')

# ---------------------------------------------------------------------------------------------------------------
# AN/PRC-152 function knob: a drum seen from the front, labels printed round it 36 degrees apart
DRUM_BOX = (1286, 140, 1390, 256)
DRUM_LABELS = ['OFF', '1', '2', '3', '4', '5', 'S', 'F', 'Z']
DRUM_STEP = 36.0

def drum_frames():
    on = git_png(data('ANPRC152', 'ANPRC152DialOn.png')).crop(DRUM_BOX)
    arr = np.asarray(on).copy()
    rgb = arr[..., :3].astype(np.float32)
    lum = rgb @ np.array([0.299, 0.587, 0.114], np.float32)
    # The labels are the bright marks on the black drum; their rows give the label band
    labels = (lum > 70) & (arr[..., 3] > 200)
    cols_dark = ((lum < 30) & (arr[..., 3] > 200)).sum(axis=0)
    xs = np.where(cols_dark > on.height * 0.3)[0]
    left, right = int(xs.min()), int(xs.max())
    cx, radius = (left + right) / 2.0, (right - left) / 2.0
    rows = np.where(labels[:, int(left + 4):int(right - 4)].any(axis=1))[0]
    top, bottom = int(rows.min()), int(rows.max())
    # Clean drum: labels painted out with the drum's own black
    inpaint_mask = cv2.dilate(labels.astype(np.uint8) * 255, np.ones((5, 5), np.uint8))
    bgr = cv2.cvtColor(arr[..., :3], cv2.COLOR_RGB2BGR)
    clean = cv2.inpaint(bgr, inpaint_mask, 4, cv2.INPAINT_TELEA)
    clean = cv2.cvtColor(clean, cv2.COLOR_BGR2RGB)
    drum = np.dstack([clean, arr[..., 3]])
    print('drum centre %.1f radius %.1f label rows %d-%d' % (cx + DRUM_BOX[0], radius, top + DRUM_BOX[1], bottom + DRUM_BOX[1]))

    band = bottom - top + 1
    font = ImageFont.truetype(r'C:\Windows\Fonts\arialbd.ttf', int(band * 0.8))
    small = ImageFont.truetype(r'C:\Windows\Fonts\arialbd.ttf', int(band * 0.42))

    def flat(label):
        """The label as printed, white on clear, band rows tall."""
        if label == 'OFF':
            # boxed and turned on its side, like the OFF printed on the original drum
            t = Image.new('L', (band, int(band * 0.62)), 0)
            d = ImageDraw.Draw(t)
            tw = d.textlength(label, font=small)
            d.text(((band - tw) / 2, int(band * 0.06)), label, fill=255, font=small)
            t = t.rotate(90, expand=True)
            box = Image.new('L', (t.width + 6, band), 0)
            box.paste(t, (3, (band - t.height) // 2))
            ImageDraw.Draw(box).rectangle((0, 0, box.width - 1, band - 1), outline=255, width=2)
            return box
        if label == 'Z':
            # zeroise: boxed like OFF, the letter upright
            d0 = ImageDraw.Draw(Image.new('L', (1, 1)))
            w = int(d0.textlength(label, font=font)) + 10
            box = Image.new('L', (w, band), 0)
            d = ImageDraw.Draw(box)
            d.text((5, -int(band * 0.12)), label, fill=255, font=font)
            d.rectangle((0, 0, w - 1, band - 1), outline=255, width=2)
            return box
        d0 = ImageDraw.Draw(Image.new('L', (1, 1)))
        w = int(d0.textlength(label, font=font)) + 2
        t = Image.new('L', (w, band), 0)
        ImageDraw.Draw(t).text((1, -int(band * 0.12)), label, fill=255, font=font)
        return t

    flats = [flat(l) for l in DRUM_LABELS]
    for pos, name in enumerate(DRUM_LABELS):
        frame = Image.fromarray(drum.copy(), 'RGBA')
        ink = Image.new('L', frame.size, 0)
        for i, f in enumerate(flats):
            theta = math.radians((i - pos) * DRUM_STEP)
            if abs(theta) >= math.radians(80):
                continue
            squash = math.cos(theta)
            w = max(1, int(round(f.width * squash)))
            g = f.resize((w, f.height), Image.LANCZOS)
            shade = 0.35 + 0.65 * squash
            g = g.point(lambda v, s=shade: int(v * s))
            x = cx + radius * math.sin(theta) - w / 2.0
            ink.paste(g, (int(round(x)), top), g)
        white = Image.new('RGBA', frame.size, (235, 235, 235, 255))
        frame.paste(white, (0, 0), ink)
        save(frame, 'ANPRC152', 'ANPRC152Knob_%s.png' % name)

# ---------------------------------------------------------------------------------------------------------------
# R-168 channel lever (KANAL): hub centre, pointer angle of the long end, length and width
HUB = (1981, 903)
HUB_R = 62
LEVER_ANGLE = -24.5			# degrees anticlockwise from 3 o'clock
LEVER_TIP, LEVER_TAIL, LEVER_HALF = 130, 46, 19
SPRITE = 264
SHADOW_SHIFT = [(dx, dy) for dx in range(0, 9, 2) for dy in range(0, 23, 2)]

def lever_polygon(cx, cy, half, tip, tail, shift=(0, 0)):
    a = math.radians(LEVER_ANGLE)
    ux, uy = math.cos(a), -math.sin(a)
    nx, ny = -uy, ux
    pts = []
    for s, t in ((tip, half), (tip, -half), (-tail, -half), (-tail, half)):
        pts.append((cx + ux * s + nx * t + shift[0], cy + uy * s + ny * t + shift[1]))
    return pts

def r168():
    base = git_png(data('R168', 'R168.png'))
    W, H = base.size
    rgb = np.asarray(base.convert('RGB')).copy()
    cx, cy = HUB

    lever = Image.new('L', (W, H), 0)
    ImageDraw.Draw(lever).polygon(lever_polygon(cx, cy, LEVER_HALF + 3, LEVER_TIP + 4, LEVER_TAIL + 4), fill=255)
    shadow = Image.new('L', (W, H), 0)
    ds = ImageDraw.Draw(shadow)
    for sh in SHADOW_SHIFT:
        ds.polygon(lever_polygon(cx, cy, LEVER_HALF + 3, LEVER_TIP + 4, LEVER_TAIL, sh), fill=255)
    lever_m = np.asarray(lever) > 0
    paint = (lever_m | (np.asarray(shadow) > 0))

    yy, xx = np.mgrid[0:H, 0:W]
    rr = np.hypot(xx - cx, yy - cy)
    on_hub = paint & (rr <= HUB_R + 2)
    off_hub = paint & ~on_hub

    # On the round hub: fill each covered pixel from the same radius just either side of the lever
    phi = np.arctan2(yy - cy, xx - cx)
    for y, x in zip(*np.where(on_hub)):
        r, p = rr[y, x], phi[y, x]
        found = []
        for sign in (-1, 1):
            for k in range(1, 180):
                q = p + sign * math.radians(k)
                sx, sy = int(round(cx + r * math.cos(q))), int(round(cy + r * math.sin(q)))
                if 0 <= sx < W and 0 <= sy < H and not paint[sy, sx]:
                    found.append((k, rgb[sy, sx].astype(np.float32)))
                    break
        if len(found) == 2:
            (k1, c1), (k2, c2) = found
            rgb[y, x] = ((c1 * k2 + c2 * k1) / (k1 + k2)).astype(np.uint8)
        elif found:
            rgb[y, x] = found[0][1].astype(np.uint8)
    # Off the hub (lever tip and its shadow on the panel): inpaint from the panel round it
    bgr = cv2.cvtColor(rgb, cv2.COLOR_RGB2BGR)
    bgr = cv2.inpaint(bgr, off_hub.astype(np.uint8) * 255, 7, cv2.INPAINT_TELEA)
    out = Image.fromarray(cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)).convert('RGBA')
    out.putalpha(base.getchannel('A'))
    save(out, 'R168', 'R168.png')

    # The lever itself, centred on the hub, with a soft edge
    big = Image.new('L', (W * 1, H * 1), 0)
    ImageDraw.Draw(big).polygon(lever_polygon(cx, cy, LEVER_HALF, LEVER_TIP, LEVER_TAIL), fill=255)
    edge = big.filter(ImageFilter.GaussianBlur(0.8))
    half = SPRITE // 2
    box = (cx - half, cy - half, cx + half, cy + half)
    sprite = base.crop(box)
    sprite.putalpha(edge.crop(box))
    save(sprite, 'R168', 'R168Channel.png')
    sh = Image.new('RGBA', (SPRITE, SPRITE), (0, 0, 0, 0))
    sh.putalpha(edge.crop(box).filter(ImageFilter.GaussianBlur(5)).point(lambda v: int(v * 0.6)))
    save(sh, 'R168', 'R168ChannelShadow.png')

    # Volume knob index line: a thin light line on the cap, pointing right (0 degrees) in the picture
    vol = Image.new('RGBA', (112, 112), (0, 0, 0, 0))
    d = ImageDraw.Draw(vol)
    d.line((56 + 18, 56, 56 + 44, 56), fill=(210, 210, 200, 235), width=4)
    save(vol.filter(ImageFilter.GaussianBlur(0.6)), 'R168', 'R168VolumeIndex.png')

    # Key light: the glow of a lit key cap
    kl = Image.new('L', (80, 68), 0)
    ImageDraw.Draw(kl).rounded_rectangle((6, 6, 73, 61), radius=10, fill=255)
    kl = kl.filter(ImageFilter.GaussianBlur(5))
    light = Image.new('RGBA', kl.size, (120, 255, 110, 0))
    light.putalpha(kl.point(lambda v: int(v * 0.55)))
    save(light, 'R168', 'R168KeyLight.png')

for r in KEYS:
    keypad_light(r)
drum_frames()
r168()
