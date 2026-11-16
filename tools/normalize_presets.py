#!/usr/bin/env python3
"""Level-matches the factory presets from an offline render (ManticoreTests render out.json).

Target: the loudest 400 ms window of the test phrase sits near -14.5 dB RMS, with peaks kept under -1 dBFS.
Cuts go into the lane volumes first (before the effects, so delays and reverbs see sane levels), boosts go into
the master volume first. Run it two or three times (render -> normalize -> rebuild -> render) until it settles.
"""
import gzip, json, math, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GZ = os.path.join(ROOT, 'Resources', 'factory_presets.json.gz')
TARGET, PEAK_CEIL = -14.5, 0.89


def used_lanes(p):
    lanes = []
    for l in range(1, 5):
        mods = [p.get(f'slot_l{l}s{s}', 'Empty') for s in range(1, 5)]
        has = any(m != 'Empty' for m in mods) or p.get(f'lane{l}_in', 'Off') != 'Off'
        if has and p.get(f'lane{l}_vol', 0.0) > -59:
            lanes.append(l)
    return lanes


def main(render_json):
    stats = json.load(open(render_json))
    presets = json.load(gzip.open(GZ))
    assert len(stats) == len(presets)
    moved = []
    for st, pr in zip(stats, presets):
        p = pr['p']
        if not st['finite']:
            print('NOT FINITE:', pr['name'])
            continue
        g = min(TARGET - st['loudest'], 20 * math.log10(PEAK_CEIL / max(1e-6, st['peak'])))
        g = max(-40.0, min(30.0, g))
        if abs(g) < 0.25:
            continue
        master = p.get('master', -6.0)
        lanes = used_lanes(p)
        rest = g
        if g < 0 and lanes:
            lane_vols = [p.get(f'lane{l}_vol', 0.0) for l in lanes]
            step = max(g, -40.0 - max(lane_vols))
            for l in lanes:
                p[f'lane{l}_vol'] = round(p.get(f'lane{l}_vol', 0.0) + step, 2)
            rest = g - step
        new_master = master + rest
        if new_master > 6.0:
            extra = new_master - 6.0
            new_master = 6.0
            if lanes:
                room = min(6.0 - p.get(f'lane{l}_vol', 0.0) for l in lanes)
                up = min(extra, room)
                for l in lanes:
                    p[f'lane{l}_vol'] = round(p.get(f'lane{l}_vol', 0.0) + up, 2)
                extra -= up
            if extra > 0.5:
                print(f'  {pr["name"]}: still {extra:.1f} dB short')
        p['master'] = round(max(-60.0, new_master), 2)
        moved.append(abs(g))
    data = json.dumps(presets, separators=(',', ':')).encode()
    with gzip.open(GZ, 'wb', compresslevel=9) as f:
        f.write(data)
    levels = {pr['name']: {k: v for k, v in pr['p'].items() if k == 'master' or (k.startswith('lane') and k.endswith('_vol'))} for pr in presets}
    json.dump(levels, open(os.path.join(ROOT, 'tools', 'levels.json'), 'w'), indent=0)
    if moved:
        print(f'adjusted {len(moved)} presets, mean |gain| {sum(moved) / len(moved):.2f} dB, max {max(moved):.2f} dB')
    else:
        print('all presets already within 0.25 dB')


if __name__ == '__main__':
    main(sys.argv[1])
