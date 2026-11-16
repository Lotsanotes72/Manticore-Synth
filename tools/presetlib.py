"""Building blocks for Manticore factory presets: a Patch class with validated helpers."""
import json, os, random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAT = json.load(open(os.path.join(ROOT, 'tools', 'params.json')))
BY_ID = {p['id']: p for p in CAT}
DESTS = [p['name'] for p in CAT if p['mod'] and p['type'] == 'float']
DEST_SET = set(DESTS)
SOURCES = BY_ID['mtx1_src']['choices']

TABLES = BY_ID['osc1_table']['choices']


class Patch:
    def __init__(self, name, cat, tags, desc=''):
        self.name, self.cat, self.tags, self.desc = name, cat, list(tags), desc
        self.p = {}
        self.mseg = []
        self.map = []
        self.nmod = 0
        self.macro_text = {}
        self.used_slots = {}

    # ---------------------------------------------------------- raw
    def set(self, pid, v):
        if pid not in BY_ID:
            raise KeyError(f'{self.name}: unknown parameter {pid}')
        d = BY_ID[pid]
        if d['type'] == 'choice':
            if isinstance(v, int):
                v = d['choices'][v]
            if v not in d['choices']:
                raise ValueError(f'{self.name}: {pid} has no choice {v!r} (choices: {d["choices"][:12]}...)')
        elif d['type'] == 'bool':
            v = bool(v)
        else:
            v = float(v)
            if v < d['min'] - 1e-6 or v > d['max'] + 1e-6:
                raise ValueError(f'{self.name}: {pid}={v} outside {d["min"]}..{d["max"]}')
            v = round(v, 5)
        self.p[pid] = v
        return self

    def many(self, prefix, **kw):
        for k, v in kw.items():
            self.set(f'{prefix}_{k}', v)
        return self

    # ---------------------------------------------------------- grid / routing
    def grid(self, lane, *modules):
        """lane 1-4, modules like 'OSC 1', 'VCF 1' placed top to bottom"""
        for s, m in enumerate(modules):
            if m in self.used_slots:
                raise ValueError(f'{self.name}: {m} used twice')
            self.used_slots[m] = (lane, s)
            self.set(f'slot_l{lane}s{s + 1}', m)
        for s in range(len(modules), 4):
            self.set(f'slot_l{lane}s{s + 1}', 'Empty')
        return self

    def clear_grid(self):
        for l in range(1, 5):
            for s in range(1, 5):
                self.set(f'slot_l{l}s{s}', 'Empty')
        self.used_slots = {}
        return self

    def lane(self, lane, vol=0.0, pan=0.0, send1=0.0, send2=0.0, send3=0.0, inp=None, inlvl=1.0):
        self.set(f'lane{lane}_vol', vol).set(f'lane{lane}_pan', pan)
        self.set(f'lane{lane}_send1', send1).set(f'lane{lane}_send2', send2).set(f'lane{lane}_send3', send3)
        if inp:
            self.set(f'lane{lane}_in', inp).set(f'lane{lane}_inlvl', inlvl)
        return self

    def fxgrid(self, bus, *units):
        """bus 1 = Main, 2-4 = Aux 1-3"""
        for s, u in enumerate(units):
            self.set(f'fxslot_b{bus}s{s + 1}', u)
        for s in range(len(units), 4):
            self.set(f'fxslot_b{bus}s{s + 1}', 'Empty')
        return self

    # ---------------------------------------------------------- modules
    def osc(self, i, table='Classic', wave=0.66, tune=0, fine=0, vol=0.7, pan=0, uni=1, det=0.2, width=0.7,
            fx1='Off', fx1amt=0, fx2='Off', fx2amt=0, engine='Wavetable', free=False, phase=0, kt=1.0,
            partials=48, bright=0, oddeven=0, stretch=0, pdecay=0):
        return self.many(f'osc{i}', engine=engine, table=table, wave=wave, tune=tune, fine=fine, vol=vol, pan=pan,
                         univoices=uni, unidetune=det, uniwidth=width, fx1=fx1, fx1amt=fx1amt, fx2=fx2, fx2amt=fx2amt,
                         free=free, phase=phase, keytrack=kt, partials=partials, bright=bright, oddeven=oddeven,
                         stretch=stretch, pdecay=pdecay)

    def fm(self, i, ratio=1.0, depth=0.3, feedback=0.0, tune=0, fine=0, vol=0.6, pan=0, shape='Sine', inputfm=0, kt=1.0):
        return self.many(f'fm{i}', ratio=ratio, depth=depth, feedback=feedback, tune=tune, fine=fine, vol=vol, pan=pan,
                         modshape=shape, inputfm=inputfm, keytrack=kt)

    def noise(self, i, type='White', color=0, density=0.5, width=1.0, vol=0.3, pan=0):
        return self.many(f'noise{i}', type=type, color=color, density=density, width=width, vol=vol, pan=pan)

    def exc(self, i, mode='Mallet', decay=40, color=0, rate=6, level=0.8, width=0.3):
        return self.many(f'exc{i}', mode=mode, decay=decay, color=color, rate=rate, level=level, width=width)

    def res(self, i, profile='Marimba', tune=0, fine=0, decay=1.2, damping=0.4, bright=0, spread=0.1, input=0, mix=1.0):
        return self.many(f'res{i}', profile=profile, tune=tune, fine=fine, decay=decay, damping=damping, bright=bright,
                         spread=spread, input=input, mix=mix)

    def comb(self, i, mode='Comb', tune=0, fine=0, feedback=0.85, damping=0.3, input=1.0, mix=1.0, vol=0.7):
        return self.many(f'comb{i}', mode=mode, tune=tune, fine=fine, feedback=feedback, damping=damping, input=input,
                         mix=mix, vol=vol)

    def vcf(self, i, model='Ladder', response='LP 24', cutoff=2000, reso=0.2, drive=0.0, kt=0.5, spread=0.0, mix=1.0):
        return self.many(f'vcf{i}', model=model, response=response, cutoff=cutoff, reso=reso, drive=drive, keytrack=kt,
                         spread=spread, mix=mix)

    def shape(self, i, mode='Tube', drive=0.3, bias=0, tone=0.8, mix=1.0, out=0):
        return self.many(f'shape{i}', mode=mode, drive=drive, bias=bias, tone=tone, mix=mix, out=out)

    def ring(self, i, source='Sine', tune=7, fine=0, kt=1.0, mix=1.0):
        return self.many(f'ring{i}', source=source, tune=tune, fine=fine, keytrack=kt, mix=mix)

    def eq(self, i, lowfreq=200, lowgain=0, midfreq=1000, midgain=0, midq=1, highfreq=5000, highgain=0):
        return self.many(f'eq{i}', lowfreq=lowfreq, lowgain=lowgain, midfreq=midfreq, midgain=midgain, midq=midq,
                         highfreq=highfreq, highgain=highgain)

    def env(self, i, a=0.01, d=0.6, s=0.5, r=0.5, slope=0.3, vel=0.3, delay=0, keyscale=0, oneshot=False):
        return self.many(f'env{i}', attack=a, decay=d, sustain=s, release=r, slope=slope, velocity=vel, delay=delay,
                         keyscale=keyscale, oneshot=oneshot)

    def lfo(self, i, wave='Sine', rate=2.0, sync=False, div='1/4', phase=0, delay=0, fade=0, mode='Retrig',
            unipolar=False, amount=1.0):
        return self.many(f'lfo{i}', wave=wave, rate=rate, sync=sync, div=div, phase=phase, delay=delay, fade=fade,
                         mode=mode, unipolar=unipolar, amount=amount)

    def mseg_p(self, i, pts, ls=None, le=None, loop='One Shot', time=1.0, sync=False, bipolar=False, vel=0.0, amount=1.0):
        self.many(f'mseg{i}', time=time, sync=sync, loop=loop, bipolar=bipolar, velocity=vel, amount=amount)
        n = len(pts)
        self.mseg.append({'i': i - 1, 'pts': [[round(x, 4), round(y, 4), round(c, 3)] for x, y, c in pts],
                          'ls': ls if ls is not None else 0, 'le': le if le is not None else n - 1})
        return self

    def map_p(self, i, values, input='Key Follow', mode='Map', steps=None, div='1/16', smooth=0.0, amount=1.0):
        vals = list(values) + [0.0] * (32 - len(values))
        self.many(f'map{i}', input=input, mode=mode, steps=steps if steps else len(values), div=div, smooth=smooth, amount=amount)
        self.map.append({'i': i - 1, 'v': [round(max(-1, min(1, v)), 4) for v in vals[:32]]})
        return self

    def math(self, i, a, b, op='A x B', amount=1.0):
        return self.many(f'math{i}', srca=a, srcb=b, op=op, amount=amount)

    # ---------------------------------------------------------- modulation
    def mod(self, src, dst, amt, via='None'):
        if dst not in DEST_SET:
            raise ValueError(f'{self.name}: unknown mod destination {dst!r}')
        if src not in SOURCES:
            raise ValueError(f'{self.name}: unknown source {src!r}')
        self.nmod += 1
        if self.nmod > 24:
            raise ValueError(f'{self.name}: more than 24 matrix slots')
        n = self.nmod
        self.set(f'mtx{n}_src', src).set(f'mtx{n}_via', via).set(f'mtx{n}_dst', DESTS.index(dst) + 1).set(f'mtx{n}_amt', amt)
        return self

    def macro(self, letter, label, routes):
        """routes: list of (dest, amount)"""
        self.macro_text[letter] = label
        for dst, amt in routes:
            self.mod('Macro ' + letter, dst, amt)
        return self

    def vibrato(self, lfo, targets, depth=0.006, rate=5.2, delay=0.0, fade=0.0):
        self.lfo(lfo, 'Sine', rate, delay=delay, fade=fade)
        for t in targets:
            self.mod(f'LFO {lfo}', t, depth, via='Mod Wheel')
        return self

    # ---------------------------------------------------------- effects
    def reverb(self, i=1, model='Classic', size=0.6, decay=3.0, predelay=20, damping=0.4, diffusion=0.75, mod=0.3,
               width=1.0, lowcut=120, mix=1.0):
        return self.many(f'reverb{i}', model=model, size=size, decay=decay, predelay=predelay, damping=damping,
                         diffusion=diffusion, mod=mod, width=width, lowcut=lowcut, mix=mix)

    def delay(self, i=1, mode='Stereo', sync=True, divl='1/8.', divr='1/4', timems=375, feedback=0.4, lowcut=150,
              highcut=6000, mod=0.15, diffusion=0.0, style='Analog', mix=1.0):
        return self.many(f'delay{i}', mode=mode, sync=sync, divl=divl, divr=divr, timems=timems, feedback=feedback,
                         lowcut=lowcut, highcut=highcut, mod=mod, diffusion=diffusion, style=style, mix=mix)

    def modfx(self, i=1, mode='Chorus', rate=0.4, depth=0.5, feedback=0.0, voices=4, centre=0.5, width=0.8,
              character='Analog', mix=0.5):
        return self.many(f'modfx{i}', mode=mode, rate=rate, depth=depth, feedback=feedback, voices=voices, centre=centre,
                         width=width, character=character, mix=mix)

    def texture(self, i=1, grain=120, density=0.5, pitch=0, spread=0.1, reverse=0.2, feedback=0.5, jitter=0.4,
                damping=0.3, freeze=False, mix=1.0):
        return self.many(f'texture{i}', grain=grain, density=density, pitch=pitch, spread=spread, reverse=reverse,
                         feedback=feedback, jitter=jitter, damping=damping, freeze=freeze, mix=mix)

    def comp(self, i=1, threshold=-18, ratio=4, attack=10, release=150, knee=6, makeup=0, mix=1.0):
        return self.many(f'comp{i}', threshold=threshold, ratio=ratio, attack=attack, release=release, knee=knee,
                         makeup=makeup, mix=mix)

    def fxeq(self, i=1, **kw):
        return self.many(f'fxeq{i}', **kw)

    def dist(self, i=1, mode='Tube', drive=0.3, tone=0.6, bias=0, mix=1.0, out=0):
        return self.many(f'dist{i}', mode=mode, drive=drive, tone=tone, bias=bias, mix=mix, out=out)

    def fxfilter(self, i=1, type='Low Pass', cutoff=20000, reso=0.1, drive=0, mix=1.0):
        return self.many(f'fxfilter{i}', type=type, cutoff=cutoff, reso=reso, drive=drive, mix=mix)

    def voice(self, mode='Poly', glide=0.0, glidemode='Always', voices=16, bend=2):
        return self.set('vmode', mode).set('glide', glide).set('glidemode', glidemode).set('voices', voices).set('pbup', bend).set('pbdn', bend)

    def arp(self, mode='Up', div='1/16', steps=8, octaves=1, gate=0.8, swing=0.0, hold=False, pattern=None):
        self.set('arp_on', True).set('arp_mode', mode).set('arp_div', div).set('arp_steps', steps).set('arp_oct', octaves)
        self.set('arp_gate', gate).set('arp_swing', swing).set('arp_hold', hold)
        if pattern:
            for k, st in enumerate(pattern[:16]):
                on, tr, vel, g = st
                self.set(f'arp_s{k + 1}_on', on).set(f'arp_s{k + 1}_tr', tr).set(f'arp_s{k + 1}_vel', vel).set(f'arp_s{k + 1}_gate', g)
        return self

    # ---------------------------------------------------------- export
    def to_json(self):
        macro_line = ''
        if self.macro_text:
            parts = []
            for k, v in sorted(self.macro_text.items()):
                role, _, what = v.partition(': ')
                parts.append(f'Ctrl {k} {role.lower()}: {what}.' if what else f'Ctrl {k}: {role}.')
            macro_line = '\n' + '\n'.join(parts)
        o = {'name': self.name, 'cat': self.cat, 'tags': self.tags, 'desc': (self.desc + macro_line).strip(), 'macros': self.macro_text, 'p': self.p}
        if self.mseg:
            o['mseg'] = self.mseg
        if self.map:
            o['map'] = self.map
        return o
