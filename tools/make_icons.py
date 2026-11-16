#!/usr/bin/env python3
"""Renders the Manticore icons: Resources/icon.png (app), installer/Manticore.ico (desktop shortcut),
installer/ManticoreSetup.ico (setup exe) and docs/icons_preview.png. Needs Pillow."""
import math, os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEAL, AMBER = (31, 208, 191), (255, 179, 63)
BG0, BG1, EDGE = (10, 16, 19), (24, 36, 42), (58, 82, 90)


def tile(S, radius_frac=0.22):
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    grad = Image.new('RGBA', (S, S))
    gd = ImageDraw.Draw(grad)
    for y in range(S):
        t = y / (S - 1)
        c = tuple(int(BG1[i] * (1 - t) + BG0[i] * t) for i in range(3)) + (255,)
        gd.line([(0, y), (S, y)], fill=c)
    mask = Image.new('L', (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, S - 1, S - 1], radius=int(S * radius_frac), fill=255)
    img.paste(grad, (0, 0), mask)
    d = ImageDraw.Draw(img)
    w = max(2, S // 96)
    d.rounded_rectangle([w // 2, w // 2, S - 1 - w // 2, S - 1 - w // 2], radius=int(S * radius_frac), outline=EDGE, width=w)
    return img


def arc_poly(cx, cy, r_in, r_out, a0, a1, steps=60):
    pts = []
    for i in range(steps + 1):
        a = a0 + (a1 - a0) * i / steps
        pts.append((cx + r_out * math.sin(a), cy - r_out * math.cos(a)))
    for i in range(steps, -1, -1):
        a = a0 + (a1 - a0) * i / steps
        pts.append((cx + r_in * math.sin(a), cy - r_in * math.cos(a)))
    return pts


def emblem(d, cx, cy, R, simple=False):
    gap = 0.20 if not simple else 0.28
    thick = 0.17 if not simple else 0.24
    for i in range(4):
        a0 = 2 * math.pi * i / 4 + gap / 2
        a1 = a0 + 2 * math.pi / 4 - gap
        d.polygon(arc_poly(cx, cy, R * (0.88 - thick), R * 0.88, a0, a1), fill=TEAL if i % 2 == 0 else AMBER)
    if not simple:
        w = max(1, int(R * 0.035))
        d.ellipse([cx - R, cy - R, cx + R, cy + R], outline=EDGE, width=w)
    # scorpion tail: overlapping segments arcing up and over, ending in a barbed stinger
    tail(d, cx, cy, R, simple)


def tail_points(cx, cy, R, n):
    r = R * 0.34
    cy0 = cy - R * 0.02
    pts = []
    for k in range(n):
        t = k / (n - 1)
        th = math.radians(228 - 250 * t)
        pts.append((cx + r * math.cos(th), cy0 - r * math.sin(th), th, t))
    return pts


def tail(d, cx, cy, R, simple):
    n = 9 if not simple else 6
    pts = tail_points(cx, cy, R, n)
    for (x, y, th, t) in pts:
        sz = R * (0.15 - 0.075 * t) * (1.2 if simple else 1.0)
        d.ellipse([x - sz, y - sz, x + sz, y + sz], fill=AMBER, outline=BG0, width=max(1, int(R * 0.025)))
    x, y, th, t = pts[-1]
    dx, dy = math.sin(th), math.cos(th)            # travel direction (clockwise on screen)
    px, py = -dy, dx                                # perpendicular
    sz = R * 0.085
    L = R * 0.30
    # curved barb: bend the tip back towards the centre
    tipx, tipy = x + dx * L - px * L * 0.35, y + dy * L - py * L * 0.35
    sting = [(x + px * sz, y + py * sz), (x + dx * L * 0.55 + px * sz * 0.6, y + dy * L * 0.55 + py * sz * 0.6), (tipx, tipy),
             (x + dx * L * 0.45 - px * sz * 0.9, y + dy * L * 0.45 - py * sz * 0.9), (x - px * sz, y - py * sz)]
    d.polygon(sting, fill=TEAL)


def render(size, setup=False):
    S = 1024
    simple = size <= 32
    img = tile(S)
    d = ImageDraw.Draw(img)
    if setup:
        emblem(d, S * 0.44, S * 0.44, S * 0.34, simple)
        bx, by, br = S * 0.73, S * 0.73, S * 0.22
        d.ellipse([bx - br - S * 0.02, by - br - S * 0.02, bx + br + S * 0.02, by + br + S * 0.02], fill=BG0)
        d.ellipse([bx - br, by - br, bx + br, by + br], fill=AMBER)
        aw = br * 0.22
        d.rectangle([bx - aw / 2, by - br * 0.55, bx + aw / 2, by + br * 0.1], fill=BG0)
        d.polygon([(bx - br * 0.45, by), (bx + br * 0.45, by), (bx, by + br * 0.55)], fill=BG0)
        d.rectangle([bx - br * 0.5, by + br * 0.62, bx + br * 0.5, by + br * 0.72], fill=BG0)
    else:
        emblem(d, S / 2, S / 2, S * 0.40, simple)
    return img.resize((size, size), Image.LANCZOS)


def ico(path, setup):
    sizes = [16, 24, 32, 48, 64, 128, 256]
    imgs = [render(s, setup) for s in sizes]
    imgs[-1].save(path, format='ICO', sizes=[(s, s) for s in sizes], append_images=imgs[:-1])


if __name__ == '__main__':
    os.makedirs(os.path.join(ROOT, 'installer'), exist_ok=True)
    render(512).save(os.path.join(ROOT, 'Resources', 'icon.png'))
    ico(os.path.join(ROOT, 'installer', 'Manticore.ico'), False)
    ico(os.path.join(ROOT, 'installer', 'ManticoreSetup.ico'), True)
    prev = Image.new('RGBA', (900, 380), BG0 + (255,))
    pd = ImageDraw.Draw(prev)
    a, b = render(256), render(256, True)
    prev.paste(a, (60, 50), a)
    prev.paste(b, (380, 50), b)
    x = 690
    for s in (16, 32, 48, 64):
        im = render(s)
        prev.paste(im, (x, 70), im)
        im2 = render(s, True)
        prev.paste(im2, (x, 200), im2)
        x += s + 14
    try:
        f = ImageFont.truetype(os.path.join(ROOT, 'Resources', 'Fonts', 'Rajdhani-SemiBold.ttf'), 22)
    except Exception:
        f = None
    pd.text((130, 322), 'Desktop icon', fill=(220, 232, 229), font=f)
    pd.text((460, 322), 'Setup icon', fill=(220, 232, 229), font=f)
    pd.text((690, 322), 'Small sizes', fill=(220, 232, 229), font=f)
    os.makedirs(os.path.join(ROOT, 'docs'), exist_ok=True)
    prev.save(os.path.join(ROOT, 'docs', 'icons_preview.png'))
    print('icons written')
