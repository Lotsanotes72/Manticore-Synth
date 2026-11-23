#!/usr/bin/env python3
"""Generates Manticore's 500 factory presets -> Resources/factory_presets.json.gz and docs/presets.json.

Each category has hand-written archetypes (signal chains with musical ranges); every preset is a seeded variation of one.
All presets share the same macro idea: Ctrl A = tone, B = motion, C = space, D = grit or character.
Run tools/normalize_presets.py afterwards to level-match them from an offline render.
"""
import gzip, json, os, random, sys
from presetlib import Patch, ROOT

R = random.Random(1611)
TAG_ALL = ['Rock', 'Metal', 'Prog', 'Cinematic', 'Ambient', 'Game']

# =========================================================================== names
ADJ = {
    'fantasy': ['Elder', 'Rune', 'Wyrm', 'Barrow', 'Ember', 'Iron', 'Oath', 'Thorn', 'Moonlit', 'Ashen', 'Silver', 'Fell',
                'Hallowed', 'Dragon', 'Ancient', 'Gilded', 'Wild', 'Frost', 'Storm', 'Raven', 'Sable', 'Amber', 'Bronze',
                'Starsteel', 'Briar', 'Hearth', 'Warden', 'Sunken', 'Fae', 'Druid', 'Thane', 'Mythic', 'Highborn', 'Grail',
                'Wolf', 'Stag', 'Kingsbane', 'Elven', 'Dwarven', 'Wizard', 'Witchwood', 'Sorcerer', 'Griffin', 'Basilisk'],
    'scifi': ['Ion', 'Quasar', 'Nebula', 'Orbital', 'Cryo', 'Void', 'Plasma', 'Neon', 'Chrome', 'Photon', 'Neutron', 'Vector',
              'Pulsar', 'Hyper', 'Xeno', 'Cyber', 'Synthetic', 'Stellar', 'Gravity', 'Binary', 'Lunar', 'Solar', 'Derelict',
              'Android', 'Warp', 'Tachyon', 'Hologram', 'Exo', 'Starship', 'Cosmic', 'Graviton', 'Fusion', 'Titanium',
              'Antimatter', 'Satellite', 'Interstellar', 'Mech', 'Colony', 'Terminal', 'Ozone'],
    'horror': ['Hollow', 'Crypt', 'Grave', 'Bone', 'Blood', 'Black', 'Rotten', 'Wretched', 'Pale', 'Cursed', 'Hex', 'Coffin',
               'Ossuary', 'Plague', 'Grim', 'Dread', 'Carrion', 'Sunless', 'Withered', 'Rust', 'Butcher', 'Asylum', 'Gallows',
               'Feral', 'Mire', 'Rotwood', 'Bleak', 'Marrow', 'Night', 'Lamprey', 'Abattoir', 'Catacomb', 'Sinister', 'Leech'],
    'paranormal': ['Ghost', 'Spectral', 'Phantom', 'Ethereal', 'Astral', 'Seance', 'Poltergeist', 'Revenant', 'Apparition',
                   'Wraith', 'Shade', 'Otherworld', 'Veiled', 'Witching', 'Spirit', 'Haunted', 'Liminal', 'Twilight',
                   'Uncanny', 'Banshee', 'Eldritch', 'Occult', 'Mystic', 'Shadow', 'Psychic', 'Ectoplasm', 'Cryptid', 'Omen',
                   'Lantern', 'Mirror', 'Whispering', 'Moonshadow', 'Grey Lady', 'Will-o-Wisp'],
}
NOUN = {
    'Pads': ['Veil', 'Shroud', 'Mist', 'Halo', 'Tapestry', 'Canopy', 'Expanse', 'Horizon', 'Sanctum', 'Haven', 'Aurora', 'Dominion',
             'Reverie', 'Vigil', 'Cathedral', 'Mantle', 'Cloak', 'Fields', 'Dawn', 'Dusk', 'Firmament', 'Bastion', 'Glade', 'Nave'],
    'Atmospheres': ['Winds', 'Hollows', 'Ether', 'Drift', 'Ruins', 'Wastes', 'Marsh', 'Depths', 'Sky', 'Fog', 'Abyss', 'Vapours',
                    'Tides', 'Wilds', 'Barrens', 'Fen', 'Moor', 'Tundra', 'Caverns', 'Shallows', 'Twilight', 'Orbit'],
    'Leads': ['Blade', 'Fang', 'Lance', 'Talon', 'Spear', 'Howl', 'Scream', 'Saber', 'Edge', 'Shriek', 'Sting', 'Arrow', 'Spark',
              'Comet', 'Rider', 'Wail', 'Cry', 'Whip', 'Crown', 'Herald', 'Cutlass', 'Flare', 'Siren', 'Javelin'],
    'Bass': ['Titan', 'Behemoth', 'Quake', 'Golem', 'Leviathan', 'Undertow', 'Colossus', 'Tremor', 'Maw', 'Rumble', 'Juggernaut',
             'Anvil', 'Bulwark', 'Crusher', 'Hammer', 'Rampart', 'Ogre', 'Troll', 'Foundation', 'Bedrock', 'Gorge', 'Mammoth'],
    'Keys & Bells': ['Chime', 'Relic', 'Lantern', 'Altar', 'Bells', 'Carillon', 'Reliquary', 'Keys', 'Music Box', 'Tines',
                     'Harmonium', 'Orrery', 'Clocktower', 'Belfry', 'Glockenspiel', 'Celesta', 'Gamelan', 'Hammers', 'Ivory'],
    'Plucks': ['Harp', 'Lyre', 'Shard', 'Spark', 'Dew', 'Thread', 'Strings', 'Pluck', 'Needle', 'Splinter', 'Droplet', 'Prism',
               'Quill', 'Zither', 'Lute', 'Ripple', 'Pebble', 'Glint'],
    'Arps & Sequences': ['Engine', 'Clockwork', 'Cipher', 'Sequence', 'Pulse', 'Machine', 'Gears', 'Circuit', 'Labyrinth',
                         'Ritual', 'Pattern', 'Signal', 'Code', 'Orbit', 'Mechanism', 'Automaton', 'Loom', 'Chase', 'Run'],
    'Brass & Strings': ['Fanfare', 'Legion', 'Herald', 'Bows', 'Armada', 'Ensemble', 'Horns', 'Strings', 'Battalion',
                        'Procession', 'Overture', 'Charge', 'Banner', 'Cohort', 'Vanguard', 'March', 'Section', 'Regiment'],
    'Choirs & Voices': ['Choir', 'Chant', 'Hymn', 'Requiem', 'Psalm', 'Litany', 'Voices', 'Chorus', 'Vespers', 'Canticle',
                        'Whispers', 'Sirens', 'Oracle', 'Congregation', 'Monks', 'Plainsong'],
    'Drones': ['Monolith', 'Throne', 'Depths', 'Obelisk', 'Undercroft', 'Reactor', 'Hum', 'Pillar', 'Vault', 'Core', 'Mass',
               'Tomb', 'Cistern', 'Engine Room', 'Furnace', 'Well', 'Crater'],
    'Cinematic FX': ['Impact', 'Riser', 'Rift', 'Collapse', 'Breach', 'Descent', 'Strike', 'Eruption', 'Awakening', 'Omen',
                     'Portal', 'Shockwave', 'Fall', 'Detonation', 'Summoning', 'Warning', 'Siege', 'Landing'],
    'Textures': ['Dust', 'Static', 'Grain', 'Ash', 'Embers', 'Frost', 'Spores', 'Sand', 'Rain', 'Smoke', 'Rust', 'Lichen',
                 'Glass', 'Cinders', 'Silt', 'Pollen'],
}
PLACES = ['the Fen', 'the Deep', 'Ashes', 'the Barrow', 'Orion', 'the Void', 'the Hollow', 'Kings', 'Embers', 'the Moor',
          'Saturn', 'Dust', 'the Crypt', 'Sorrow', 'the North', 'Thorns', 'Glass', 'the Ninth Moon', 'the Drowned',
          'Lost Stars', 'the Old Gods', 'Iron Hills', 'the Vale', 'Night', 'the Forgotten', 'Europa', 'the Wastes', 'the Abbey',
          'Ten Towers', 'the Last Light', 'Black Water', 'Elders', 'the Rift', 'Titan', 'the Tide', 'Winter']
ROOTS = {'fantasy': ['Wyrm', 'Rune', 'Iron', 'Ember', 'Thorn', 'Storm', 'Frost', 'Raven', 'Oak', 'Dragon', 'Star', 'Moon', 'Ash'],
         'scifi': ['Void', 'Ion', 'Neon', 'Star', 'Cryo', 'Flux', 'Hex', 'Arc', 'Nova', 'Grav', 'Chrome', 'Pulse'],
         'horror': ['Bone', 'Grave', 'Blood', 'Rot', 'Hex', 'Crypt', 'Ash', 'Plague', 'Dread', 'Gloom', 'Mire'],
         'paranormal': ['Ghost', 'Shade', 'Wraith', 'Spirit', 'Moon', 'Mist', 'Omen', 'Veil', 'Wisp', 'Shadow', 'Night']}
SUFFIX = ['glass', 'light', 'fire', 'born', 'fall', 'wake', 'shard', 'song', 'veil', 'mark', 'forge', 'heart', 'spire', 'gate',
          'wing', 'thorn', 'scar', 'bloom', 'reach', 'hold', 'fang', 'vein', 'crest', 'shroud']
USED = set()
NOUN_COUNT = {}


def make_name(cat, theme):
    for _ in range(400):
        form = R.random()
        nouns = NOUN[cat]
        noun = R.choice(nouns)
        if NOUN_COUNT.get(noun, 0) >= 4:
            continue
        if form < 0.58:
            name = f'{R.choice(ADJ[theme])} {noun}'
        elif form < 0.78:
            name = f'{noun} of {R.choice(PLACES)}'
        else:
            name = f'{R.choice(ROOTS[theme])}{R.choice(SUFFIX)} {noun}'
        if len(name) > 26 or name in USED:
            continue
        USED.add(name)
        NOUN_COUNT[noun] = NOUN_COUNT.get(noun, 0) + 1
        return name
    raise RuntimeError('ran out of names')


def pick(*xs):
    return R.choice(xs)


def rr(a, b, nd=3):
    return round(R.uniform(a, b), nd)


# =========================================================================== shared patch scaffolding
def base(p):
    p.clear_grid()
    p.fxgrid(1)
    p.fxgrid(2, 'REVERB 1')
    p.fxgrid(3, 'DELAY 1')
    p.fxgrid(4)
    p.reverb(1, mix=1.0)
    p.delay(1, mix=1.0)
    p.set('master', -6)
    return p


def space(p, lanes, rev=0.25, dly=0.0, model='Classic', size=0.6, decay=3.0, divl='1/8.', divr='1/4', fb=0.35, damping=0.4, predelay=20):
    p.reverb(1, model=model, size=size, decay=decay, damping=damping, predelay=predelay, mix=1.0)
    p.delay(1, divl=divl, divr=divr, feedback=fb, mix=1.0)
    for l in lanes:
        p.set(f'lane{l}_send1', rev).set(f'lane{l}_send2', dly)


def macros_std(p, a_label, a_routes, b_label, b_routes, c_label='Space: reverb and delay returns', c_routes=None,
               d_label=None, d_routes=None):
    p.macro('A', a_label, a_routes)
    p.macro('B', b_label, b_routes)
    p.macro('C', c_label, c_routes if c_routes is not None else [('Aux 1 Return', 0.15), ('Aux 2 Return', 0.15)])
    if d_label:
        p.macro('D', d_label, d_routes)


# =========================================================================== PADS
def pad_analog(p, v):
    tune2 = pick(-12, 7, 0, 12, -5)
    p.osc(1, 'Classic', wave=rr(0.6, 0.72), uni=pick(3, 4, 5, 6), det=rr(0.18, 0.35), vol=0.6)
    p.osc(2, pick('Classic', 'Brass Swell', 'Dark Pad', 'Bowed String'), wave=rr(0.3, 0.8), tune=tune2, fine=rr(-8, 8, 1), uni=pick(1, 2, 3), det=0.2, vol=0.45)
    p.vcf(1, pick('Ladder', 'Cascade', 'OTA'), 'LP 24', cutoff=rr(700, 2200, 0), reso=rr(0.1, 0.35), drive=rr(0, 0.25), kt=0.4)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.4, 1.6), d=1.5, s=0.85, r=rr(1.5, 3.5), slope=0.2)
    p.env(2, a=rr(0.8, 2.5), d=rr(1.5, 4), s=rr(0.3, 0.6), r=2.5, vel=0.4)
    p.mod('Env 2', 'VCF 1 Cutoff', rr(0.12, 0.25))
    p.lfo(1, pick('Sine', 'Triangle', 'Smooth Rnd'), rr(0.08, 0.3))
    p.mod('LFO 1', 'OSC 1 Wave', rr(0.04, 0.1))
    p.lfo(2, 'Smooth Rnd', rr(0.2, 0.6))
    p.mod('LFO 2', 'VCF 1 Cutoff', rr(0.02, 0.06))
    p.vibrato(3, ['OSC 1 Tune', 'OSC 2 Tune'], 0.005, rate=4.8)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=rr(0.2, 0.6), depth=rr(0.3, 0.6), voices=4, mix=rr(0.3, 0.5))
    space(p, [1], rev=rr(0.3, 0.55), dly=rr(0.0, 0.15), model=pick('Classic', 'Lush'), decay=rr(3, 6))
    macros_std(p, 'Tone: opens the filter', [('VCF 1 Cutoff', 0.3)], 'Motion: deeper drift', [('LFO 1 Amount', 0.5), ('LFO 2 Amount', 0.4)],
               d_label='Grit: drives the filter', d_routes=[('VCF 1 Drive', 0.6)])
    p.set('lfo1_amount', 0.5); p.set('lfo2_amount', 0.6)
    m = p.p["vcf1_model"]
    return f'Warm detuned saw pad through {"an" if m[0] in "AEIOU" else "a"} {m} low-pass that blooms with a slow filter envelope and drifts with two slow LFOs. Mod wheel adds vibrato.'


def pad_glass(p, v):
    p.osc(1, pick('Glass', 'Sine Stack', 'Bell Partials'), engine='Additive', wave=rr(0.2, 0.8), partials=pick(24, 32, 48),
          bright=rr(-0.3, 0.3), stretch=rr(0.0, 0.15), vol=0.55, width=0.9)
    p.osc(2, 'Sine Stack', wave=rr(0.1, 0.4), tune=12, fine=rr(3, 9, 1), vol=0.3, uni=2, det=0.15)
    p.vcf(1, 'SVF', 'LP 12', cutoff=rr(3000, 8000, 0), reso=0.1, kt=0.3)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.8, 2.2), d=2, s=0.9, r=rr(2.5, 5), slope=-0.2)
    p.mseg_p(1, [(0, 0, 0), (rr(2, 4), 1, -0.3), (8, 0.3, 0.2)], loop='One Shot', time=1.0)
    p.mod('MSEG 1', 'OSC 1 Wave', rr(0.3, 0.6))
    p.lfo(1, 'Triangle', rr(0.05, 0.2))
    p.mod('LFO 1', 'OSC 1 Bright', 0.15)
    p.fxgrid(2, 'REVERB 1')
    p.fxgrid(4, 'TEXTURE 1')
    p.texture(1, grain=rr(120, 300, 0), density=0.6, pitch=12, spread=0.05, reverse=0.3, feedback=0.4, mix=1.0)
    p.lane(1, send1=rr(0.4, 0.6), send2=0.1, send3=rr(0.15, 0.35))
    p.reverb(1, model='Lush', size=0.8, decay=rr(5, 9), mix=1.0)
    macros_std(p, 'Tone: brighter partials', [('OSC 1 Bright', 0.4), ('VCF 1 Cutoff', 0.2)], 'Motion: morph speed', [('LFO 1 Rate', 0.3)],
               c_routes=[('Aux 1 Return', 0.2), ('Aux 3 Return', 0.3)], d_label='Character: stretches partials into bells',
               d_routes=[('OSC 1 Stretch', 0.35)])
    return 'Additive glass pad: partials rebuilt from the wavetable, slowly morphed by an MSEG, with an octave-up shimmer cloud from TextureVerb.'


def pad_formant(p, v):
    p.osc(1, pick('Formant Vox', 'Choir'), wave=0.0, uni=pick(3, 4), det=0.25, vol=0.6)
    p.osc(2, 'Classic', wave=0.5, tune=-12, vol=0.35)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(300, 1500, 0), reso=rr(0.3, 0.6), kt=0.2)
    p.vcf(2, 'SVF', 'LP 12', cutoff=6000, reso=0.1)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1', 'VCF 2')
    p.env(1, a=rr(0.6, 1.5), d=1.5, s=0.85, r=rr(2, 4))
    p.mseg_p(1, [(0, 0, 0), (2, 0.5, 0), (4, 1, 0), (6, 0.25, 0), (8, 0, 0)], loop='Repeat', time=rr(1.0, 2.0))
    p.mod('MSEG 1', 'OSC 1 Wave', 1.0)
    p.lfo(1, 'Smooth Rnd', rr(0.1, 0.3))
    p.mod('LFO 1', 'VCF 1 Cutoff', rr(0.06, 0.12))
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.3, depth=0.5, mix=0.4)
    space(p, [1], rev=0.45, dly=0.12, model='Lush', decay=5)
    macros_std(p, 'Tone: vowel position', [('VCF 1 Cutoff', 0.35)], 'Motion: vowel cycling speed', [('MSEG 1 Time', -0.3)],
               d_label='Character: voice formant sharpness', d_routes=[('VCF 1 Reso', 0.4)])
    return 'Vowel-shifting pad: the Formant Vox table cycles through A-E-I-O-U on a repeating MSEG while a formant filter wanders.'


def pad_pwm(p, v):
    p.osc(1, 'Pulse Width', wave=0.2, uni=pick(2, 3, 4), det=rr(0.15, 0.3), vol=0.6)
    p.osc(2, 'Pulse Width', wave=0.4, tune=pick(-12, 0, 12), fine=-6, vol=0.45)
    p.vcf(1, pick('Cascade', 'OTA', 'Ladder'), 'LP 24', cutoff=rr(1500, 3500, 0), reso=0.15)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.2, 0.9), d=1, s=0.9, r=rr(1, 2.5))
    p.lfo(1, 'Triangle', rr(0.3, 1.1))
    p.lfo(2, 'Triangle', rr(0.2, 0.8), phase=0.5)
    p.mod('LFO 1', 'OSC 1 Wave', 0.25)
    p.mod('LFO 2', 'OSC 2 Wave', 0.25)
    p.vibrato(3, ['OSC 1 Tune', 'OSC 2 Tune'], 0.005)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.5, depth=0.6, voices=6, mix=0.5)
    space(p, [1], rev=0.35, dly=0.1)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: PWM speed', [('LFO 1 Rate', 0.25), ('LFO 2 Rate', 0.25)],
               d_label='Character: chorus depth', d_routes=[('MODFX 1 Depth', 0.4), ('MODFX 1 Mix', 0.3)])
    return 'Classic pulse-width-modulated string pad: two pulse waves swept by offset LFOs, warmed by ensemble chorus. Mod wheel adds vibrato.'


def pad_horror(p, v):
    p.osc(1, pick('Ghost', 'Spline Shapes', 'Metallic'), engine=pick('Wavetable', 'Additive'), wave=rr(0.2, 0.8), stretch=rr(-0.4, 0.4),
          uni=3, det=rr(0.3, 0.6), vol=0.6)
    p.osc(2, 'Dark Pad', wave=0.4, tune=pick(-12, -24, 6, 13), vol=0.4)
    p.ring(1, pick('Sine', 'Triangle'), tune=rr(5.5, 7.5, 2), mix=rr(0.15, 0.4))
    p.vcf(1, pick('Phaser', 'Twin Peak', 'Steiner'), pick('LP 12', 'Notch'), cutoff=rr(500, 1800, 0), reso=rr(0.3, 0.6))
    p.grid(1, 'OSC 1', 'OSC 2', 'RING 1', 'VCF 1')
    p.env(1, a=rr(1.5, 4), d=2, s=0.9, r=rr(3, 6), slope=-0.3)
    p.lfo(1, 'Smooth Rnd', rr(0.05, 0.25))
    p.mod('LFO 1', 'VCF 1 Cutoff', rr(0.1, 0.2))
    p.mod('LFO 1', 'RING 1 Tune', 0.02)
    p.lfo(2, 'Sine', rr(0.03, 0.08))
    p.mod('LFO 2', 'OSC 1 Wave', 0.3)
    p.fxgrid(4, 'TEXTURE 1')
    p.texture(1, grain=rr(200, 500, 0), density=0.5, pitch=pick(-12, -5, 0), spread=0.3, reverse=0.6, feedback=0.6, mix=1.0)
    p.lane(1, send1=0.5, send2=0.0, send3=0.35)
    p.reverb(1, model='Lush', size=0.9, decay=rr(7, 14), damping=0.6, mix=1.0)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: unrest (random drift)', [('LFO 1 Rate', 0.4)],
               c_routes=[('Aux 1 Return', 0.15), ('Aux 3 Return', 0.3)], d_label='Character: ring-mod dissonance', d_routes=[('RING 1 Mix', 0.5)])
    return 'Unsettling pad for horror scoring: a drifting table ring-modulated off-pitch, smeared by reversed grains and a long dark hall.'


def pad_scifi(p, v):
    p.osc(1, pick('Spline Shapes', 'Digital Bits', 'Phase Warp', 'Sync Sweep'), wave=0.0, uni=pick(2, 4), det=0.2, vol=0.6,
          fx1=pick('Off', 'Mirror', 'Phase Dist'), fx1amt=rr(0.1, 0.4))
    p.osc(2, 'Sine Stack', wave=rr(0.2, 0.5), tune=-12, vol=0.4)
    p.vcf(1, pick('Twin Peak', 'SVF', 'Smooth'), pick('LP 12', 'BP 12'), cutoff=rr(1200, 3500, 0), reso=rr(0.3, 0.55))
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.5, 1.5), d=1, s=0.85, r=rr(2, 4))
    p.mseg_p(1, [(0, 0, 0), (1, 1, 0.3), (2, 0.2, -0.3), (3, 0.8, 0.3), (4, 0, 0)], loop='Repeat', sync=True, time=pick(2, 4), amount=0.5)
    p.mod('MSEG 1', 'OSC 1 Wave', 1.0)
    p.lfo(1, 'Sine', 0.2, sync=True, div=pick('2 bars', '4 bars'))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.15)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Phaser', rate=0.15, depth=0.6, feedback=0.4, voices=8, mix=0.4)
    space(p, [1], rev=0.35, dly=0.3, model='Lush', divl='1/4.', divr='1/8.', fb=0.45)
    p.delay(1, mode='Ping Pong', divl='1/8.', feedback=0.45, mix=1.0)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: wavetable scan depth', [('MSEG 1 Amount', 0.5)],
               d_label='Character: wave FX', d_routes=[('OSC 1 FX 1 Amt', 0.5)])
    return 'Evolving sci-fi pad: a tempo-synced MSEG scans the wavetable while a slow LFO sweeps the filter; ping-pong delay and phaser add motion.'


def pad_dark(p, v):
    p.osc(1, 'Dark Pad', wave=rr(0.3, 0.9), uni=3, det=0.25, vol=0.65)
    p.osc(2, pick('Classic', 'Hollow Odd'), wave=rr(0.3, 0.6), tune=-12, vol=0.4)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(350, 900, 0), reso=rr(0.15, 0.3), kt=0.3)
    p.noise(1, 'Pink', color=0.3, vol=0.12)
    p.comb(1, 'Diffuse', tune=pick(0, 7, 12), feedback=0.75, damping=0.6, mix=0.8, vol=0.5)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.grid(2, 'NOISE 1', 'COMB 1')
    p.lane(2, vol=-8)
    p.env(1, a=rr(1, 3), d=2, s=0.9, r=rr(2.5, 5))
    p.env(2, a=rr(2, 5), d=4, s=0.5, r=3)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.2)
    p.lfo(1, 'Smooth Rnd', 0.1)
    p.mod('LFO 1', 'OSC 1 Wave', 0.2)
    space(p, [1, 2], rev=0.45, model='Lush', decay=rr(5, 9), damping=0.6)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.35)], 'Motion: table drift', [('LFO 1 Amount', 0.6)],
               d_label='Character: air (noise comb level)', d_routes=[('Lane 2 Volume', 0.15)])
    p.set('lfo1_amount', 0.3)
    return 'Dark low pad with a breathy comb-filtered noise layer in lane 2; the ladder filter barely opens until Ctrl A.'


def pad_choir(p, v):
    p.osc(1, 'Choir', wave=rr(0.0, 0.6), uni=pick(4, 5, 6), det=rr(0.2, 0.35), vol=0.6)
    p.osc(2, 'Formant Vox', wave=pick(0.0, 0.25, 0.75), tune=pick(0, 12), vol=0.3)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(400, 900, 0), reso=0.35, mix=0.6)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.6, 1.5), d=1, s=0.9, r=rr(2, 4))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.004, rate=5.0, delay=0.4, fade=1.0)
    p.lfo(2, 'Smooth Rnd', 0.15)
    p.mod('LFO 2', 'OSC 1 Wave', 0.25)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.25, depth=0.5, voices=6, mix=0.5)
    space(p, [1], rev=0.55, model='Lush', decay=rr(5, 9))
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: breath drift', [('LFO 2 Amount', 0.6)],
               d_label='Character: formant mix', d_routes=[('VCF 1 Mix', 0.4)])
    p.set('lfo2_amount', 0.4)
    return 'Synthetic choir pad stacked from the Choir and Formant Vox tables with a formant filter and wide ensemble.'


def pad_brass_swell(p, v):
    p.osc(1, 'Brass Swell', wave=0.1, uni=pick(3, 4), det=0.18, vol=0.6)
    p.osc(2, 'Brass Swell', wave=0.2, tune=-12, fine=5, vol=0.45)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(900, 1600, 0), reso=0.15, drive=0.2)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.3, 0.8), d=1, s=0.85, r=1.5)
    p.env(2, a=rr(0.6, 1.4), d=2, s=0.4, r=2)
    p.mod('Env 2', 'OSC 1 Wave', 0.7)
    p.mod('Env 2', 'OSC 2 Wave', 0.6)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.2)
    p.mod('Velocity', 'VCF 1 Cutoff', 0.15)
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.005)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.4, depth=0.35, mix=0.3)
    space(p, [1], rev=0.35, dly=0.08)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: swell depth', [('ENV 2 Attack', 0.3)],
               d_label='Grit: filter drive', d_routes=[('VCF 1 Drive', 0.6)])
    return 'Prog-rock synth-brass pad: the Brass Swell table opens with a slow envelope like a horn section leaning in. Play harder for brighter.'


def pad_organ(p, v):
    p.osc(1, 'Organ Bars', wave=rr(0.3, 1.0), vol=0.6, uni=2, det=0.05)
    p.osc(2, 'Organ Bars', wave=rr(0.0, 0.5), tune=12, vol=0.3)
    p.vcf(1, 'Smooth', 'LP 12', cutoff=rr(3000, 7000, 0), reso=0.05)
    p.shape(1, 'Tube', drive=rr(0.1, 0.35), tone=0.7)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1', 'SHAPE 1')
    p.env(1, a=rr(0.05, 0.4), d=1, s=0.95, r=rr(0.4, 1.2))
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=rr(0.8, 1.5), depth=0.5, voices=2, character='Analog', mix=0.45)
    p.lfo(1, 'Sine', 0.7, mode='Global')
    p.mod('Mod Wheel', 'MODFX 1 Rate', 0.5)
    space(p, [1], rev=0.3)
    macros_std(p, 'Tone: brightness', [('VCF 1 Cutoff', 0.3)], 'Motion: rotary speed (also mod wheel)', [('MODFX 1 Rate', 0.4)],
               d_label='Grit: overdrive', d_routes=[('SHAPE 1 Drive', 0.5)])
    return 'Drawbar-style organ pad with tube drive and a rotary-flavoured chorus; the mod wheel speeds up the rotor like a classic prog organ.'


# =========================================================================== ATMOSPHERES
def atm_wind(p, v):
    p.noise(1, pick('Pink', 'White', 'Brown'), color=rr(-0.3, 0.2), vol=0.5, width=1.0)
    p.vcf(1, pick('SVF', 'Twin Peak', 'Formant'), 'BP 12', cutoff=rr(400, 1500, 0), reso=rr(0.4, 0.75), kt=0.6)
    p.vcf(2, 'SVF', 'BP 6', cutoff=rr(1500, 4000, 0), reso=0.5, kt=0.3)
    p.grid(1, 'NOISE 1', 'VCF 1')
    p.noise(2, 'Pink', vol=0.35)
    p.grid(2, 'NOISE 2', 'VCF 2')
    p.lane(2, vol=-6)
    p.env(1, a=rr(1, 3), d=2, s=1.0, r=rr(3, 6))
    p.lfo(1, 'Smooth Rnd', rr(0.08, 0.3))
    p.lfo(2, 'Smooth Rnd', rr(0.1, 0.4))
    p.mod('LFO 1', 'VCF 1 Cutoff', rr(0.15, 0.25))
    p.mod('LFO 2', 'VCF 2 Cutoff', 0.2)
    p.mod('LFO 1', 'Lane 1 Pan', 0.4)
    p.mod('LFO 2', 'Lane 2 Pan', -0.4)
    space(p, [1, 2], rev=0.5, model='Lush', decay=rr(5, 10))
    macros_std(p, 'Tone: howl pitch', [('VCF 1 Cutoff', 0.3), ('VCF 2 Cutoff', 0.2)], 'Motion: gustiness', [('LFO 1 Rate', 0.4), ('LFO 2 Rate', 0.4)],
               d_label='Character: resonance whistle', d_routes=[('VCF 1 Reso', 0.3)])
    return 'Wind and air: resonant band-passed noise that gusts and pans on random LFOs. The played note tunes the howl.'


def atm_resonant(p, v):
    prof = pick('Glass', 'Gong', 'Plate', 'Bell', 'Vibes')
    p.exc(1, 'Pattern', decay=rr(20, 80, 0), color=rr(-0.2, 0.5), rate=rr(1.5, 7, 2), level=0.8)
    p.res(1, prof, tune=pick(0, 12, 7), decay=rr(2, 6), damping=rr(0.2, 0.6), bright=rr(-0.2, 0.3), spread=rr(0.2, 0.6))
    p.grid(1, 'EXC 1', 'RES 1')
    p.noise(1, 'Pink', vol=0.08)
    p.vcf(1, 'SVF', 'HP 12', cutoff=3000, reso=0.2)
    p.grid(2, 'NOISE 1', 'VCF 1')
    p.env(1, a=rr(0.5, 1.5), d=1, s=1, r=rr(3, 6))
    p.lfo(1, 'Smooth Rnd', 0.3)
    p.mod('LFO 1', 'EXC 1 Rate', 0.1)
    p.mod('LFO 1', 'RES 1 Bright', 0.2)
    space(p, [1, 2], rev=0.55, dly=0.25, model='Lush', decay=rr(6, 10))
    p.delay(1, mode='Ping Pong', divl='1/8.', feedback=0.5, mix=1.0)
    macros_std(p, 'Tone: brightness of the body', [('RES 1 Bright', 0.5)], 'Motion: strike rate', [('EXC 1 Rate', 0.25)],
               d_label='Character: body spread', d_routes=[('RES 1 Spread', 0.4)])
    return f'A {prof.lower()} body struck by a pattern exciter, ringing into a ping-pong delay and lush hall: self-playing tuned percussion.'


def atm_cloud(p, v):
    p.osc(1, pick('Spline Shapes', 'Ghost', 'Glass', 'Choir'), wave=rr(0, 1), uni=pick(2, 3), det=0.3, vol=0.5)
    p.vcf(1, 'SVF', 'LP 12', cutoff=rr(2000, 5000, 0), reso=0.15)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=rr(2, 4), d=2, s=1, r=rr(4, 8))
    p.mseg_p(1, [(0, 0, 0), (4, 1, 0), (8, 0, 0)], loop='Repeat', time=rr(1, 3))
    p.mod('MSEG 1', 'OSC 1 Wave', 0.8)
    p.fxgrid(2, 'TEXTURE 1', 'REVERB 1')
    p.texture(1, grain=rr(150, 450, 0), density=rr(0.5, 0.8), pitch=pick(0, 12, -12, 7), spread=rr(0.1, 0.4), reverse=rr(0.2, 0.6),
              feedback=rr(0.5, 0.75), jitter=0.6, mix=1.0)
    p.reverb(1, model='Lush', size=0.85, decay=rr(6, 12), mix=0.6)
    p.lane(1, send1=0.8)
    p.set('aux1_ret', 0)
    p.lfo(1, 'Smooth Rnd', 0.2, mode='Global')
    p.mod('LFO 1', 'TEXTURE 1 Pitch', 0.05)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: grain density', [('TEXTURE 1 Density', 0.35)],
               c_label='Space: cloud size (grain length)', c_routes=[('TEXTURE 1 Grain', 0.3)],
               d_label='Character: cloud feedback', d_routes=[('TEXTURE 1 Feedback', 0.25)])
    return 'Granular cloud: a slowly morphing tone is scattered into TextureVerb grains that feed back into themselves. Hold long notes.'


def atm_haunt(p, v):
    p.osc(1, 'Formant Vox', wave=0.5, uni=2, det=0.4, vol=0.5)
    p.noise(1, 'Pink', vol=0.25)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(500, 1500, 0), reso=0.5)
    p.grid(1, 'OSC 1', 'NOISE 1', 'VCF 1')
    p.env(1, a=rr(1, 3), d=1, s=1, r=rr(3, 6))
    p.lfo(1, 'Smooth Rnd', rr(0.15, 0.5))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.3)
    p.mod('LFO 1', 'OSC 1 Wave', 0.5)
    p.lfo(2, 'Smooth Rnd', 0.08)
    p.mod('LFO 2', 'OSC 1 Tune', 0.01)
    p.fxgrid(4, 'TEXTURE 1')
    p.texture(1, grain=300, density=0.4, pitch=-12, reverse=0.8, feedback=0.6, mix=1.0)
    p.lane(1, send1=0.5, send3=0.4)
    p.reverb(1, model='Lush', decay=10, size=0.9, damping=0.6, mix=1.0)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: whisper speed', [('LFO 1 Rate', 0.4)],
               c_routes=[('Aux 1 Return', 0.15), ('Aux 3 Return', 0.3)], d_label='Character: breath (noise level)', d_routes=[('NOISE 1 Volume', 0.4)])
    return 'Paranormal whispers: breathy vowel tones that never settle, with reversed, octave-down grains trailing behind like voices in another room.'


def atm_space(p, v):
    p.osc(1, 'Sine Stack', wave=rr(0.1, 0.4), tune=-12, vol=0.55)
    p.fm(1, ratio=pick(1.41, 2.76, 3.5, 0.5), depth=rr(0.15, 0.35), feedback=0.2, vol=0.3)
    p.vcf(1, 'SVF', 'LP 12', cutoff=rr(1500, 4000, 0), reso=0.2)
    p.grid(1, 'OSC 1', 'FM 1', 'VCF 1')
    p.env(1, a=rr(2, 4), d=2, s=1, r=rr(4, 7))
    p.lfo(1, 'Sine', rr(0.03, 0.1))
    p.mod('LFO 1', 'FM 1 Depth', 0.15)
    p.lfo(2, 'S&H', rr(0.5, 2))
    p.mod('LFO 2', 'FM 1 Ratio', 0.01)
    space(p, [1], rev=0.6, dly=0.3, model='Lush', decay=rr(8, 15))
    p.delay(1, mode='Ping Pong', divl='1/4.', feedback=0.6, diffusion=0.5, mix=1.0)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: FM drift', [('FM 1 Depth', 0.2), ('LFO 1 Rate', 0.3)],
               d_label='Character: inharmonic FM ratio', d_routes=[('FM 1 Ratio', 0.15)])
    return 'Deep-space atmosphere: a low sine bed with an inharmonic FM layer that drifts and twinkles through a diffused ping-pong delay.'


# =========================================================================== LEADS
def lead_rock(p, v):
    p.osc(1, 'Classic', wave=rr(0.62, 0.7), uni=pick(1, 2), det=0.08, vol=0.65)
    p.osc(2, pick('Classic', 'Pulse Width'), wave=rr(0.4, 0.9), tune=pick(0, -12, 7, 12), fine=rr(-7, 7, 1), vol=0.45)
    p.shape(1, pick('Tube', 'Hard Clip'), drive=rr(0.35, 0.65), tone=rr(0.55, 0.8))
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(1800, 4000, 0), reso=rr(0.15, 0.35), kt=0.5)
    p.grid(1, 'OSC 1', 'OSC 2', 'SHAPE 1', 'VCF 1')
    p.voice('Legato', glide=rr(0.03, 0.09), glidemode='Legato Only', voices=1)
    p.env(1, a=0.003, d=0.8, s=0.85, r=rr(0.15, 0.35))
    p.env(2, a=0.005, d=rr(0.3, 0.8), s=0.4, r=0.3)
    p.mod('Env 2', 'VCF 1 Cutoff', rr(0.12, 0.22))
    p.mod('Velocity', 'VCF 1 Cutoff', 0.1)
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.007, rate=5.6)
    space(p, [1], rev=0.2, dly=0.25)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: auto-vibrato', [],
               d_label='Grit: more drive', d_routes=[('SHAPE 1 Drive', 0.35)])
    p.mod('LFO 1', 'OSC 1 Tune', 0.006, via='Macro B')
    p.mod('LFO 1', 'OSC 2 Tune', 0.006, via='Macro B')
    return 'Overdriven mono lead for rock solos: two oscillators into a tube or clip stage, then a ladder filter. Legato glide, mod wheel vibrato.'


def lead_metal(p, v):
    p.osc(1, pick('Classic', 'Growl', 'Sync Sweep'), wave=rr(0.6, 0.8), uni=pick(2, 3), det=rr(0.1, 0.2), vol=0.6,
          fx1=pick('Sync', 'Off'), fx1amt=rr(0.1, 0.3))
    p.osc(2, 'Classic', wave=0.66, tune=-12, fine=4, vol=0.4)
    p.shape(1, pick('Hard Clip', 'Asym Fold', 'Tube'), drive=rr(0.6, 0.85), bias=rr(-0.2, 0.2), tone=rr(0.5, 0.7))
    p.vcf(1, pick('Bite', 'Diode', 'Sallen-Key'), 'LP 24', cutoff=rr(2500, 5000, 0), reso=rr(0.2, 0.4))
    p.grid(1, 'OSC 1', 'OSC 2', 'SHAPE 1', 'VCF 1')
    p.voice('Legato', glide=rr(0.02, 0.06), glidemode='Legato Only', voices=1)
    p.env(1, a=0.002, d=1, s=0.9, r=0.25)
    p.env(2, a=0.002, d=0.4, s=0.3, r=0.3)
    p.mod('Env 2', 'OSC 1 FX 1 Amt', 0.3)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.15)
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.008, rate=6.0)
    p.fxgrid(1, 'COMP 1')
    p.comp(1, threshold=-14, ratio=4, attack=5, release=120, mix=1.0)
    space(p, [1], rev=0.15, dly=0.2, divl='1/8', divr='1/8.')
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: sync sweep', [('OSC 1 FX 1 Amt', 0.4)],
               d_label='Grit: destroy', d_routes=[('SHAPE 1 Drive', 0.3), ('SHAPE 1 Bias', 0.3)])
    return 'Shredding metal lead: synced, hard-clipped oscillators into an aggressive filter with a compressor holding it in your face.'


def lead_prog(p, v):
    p.osc(1, 'Classic', wave=rr(0.62, 0.7), vol=0.55)
    p.osc(2, 'Classic', wave=rr(0.85, 1.0), tune=pick(0, -12, 12), fine=rr(4, 10, 1), vol=0.45)
    p.osc(3, 'Classic', wave=0.66, tune=pick(-12, 7, 19), fine=-5, vol=0.3)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(900, 2200, 0), reso=rr(0.25, 0.5), drive=0.3, kt=0.6)
    p.grid(1, 'OSC 1', 'OSC 2', 'OSC 3', 'VCF 1')
    p.voice('Mono', glide=rr(0.06, 0.15), glidemode='Always', voices=1)
    p.env(1, a=0.005, d=0.6, s=0.8, r=0.25)
    p.env(2, a=rr(0.01, 0.15), d=rr(0.4, 1.0), s=0.35, r=0.3, vel=0.5)
    p.mod('Env 2', 'VCF 1 Cutoff', rr(0.18, 0.3))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune', 'OSC 3 Tune'], 0.006, rate=5.0, delay=0.3, fade=0.6)
    space(p, [1], rev=0.25, dly=0.2, divl='1/4.', divr='1/4.')
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: filter envelope depth', [('VCF 1 Reso', 0.2)],
               d_label='Grit: ladder drive', d_routes=[('VCF 1 Drive', 0.6)])
    return 'Classic three-oscillator prog lead with portamento on every note and a squelchy ladder envelope. Delayed vibrato on the mod wheel.'


def lead_sync(p, v):
    p.osc(1, pick('Classic', 'Sine Stack'), wave=rr(0.5, 0.7), vol=0.6, fx1='Sync', fx1amt=rr(0.2, 0.5))
    p.osc(2, 'Classic', wave=0.66, tune=-12, vol=0.3)
    p.vcf(1, 'SVF', 'LP 12', cutoff=rr(3000, 7000, 0), reso=0.15)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.voice('Legato', glide=0.04, voices=1)
    p.env(1, a=0.003, d=1, s=0.9, r=0.3)
    p.env(2, a=0.002, d=rr(0.4, 1.2), s=0.2, r=0.4)
    p.mod('Env 2', 'OSC 1 FX 1 Amt', rr(0.3, 0.5))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.006)
    space(p, [1], rev=0.2, dly=0.25)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: sync sweep depth', [('OSC 1 FX 1 Amt', 0.35)],
               d_label='Grit: second wave FX (fold)', d_routes=[('OSC 1 FX 2 Amt', 0.6)])
    p.set('osc1_fx2', 'Fold')
    return 'Hard-sync lead: an envelope sweeps the sync amount for that tearing, vocal attack. Great for 80s-flavoured rock and game themes.'


def lead_fm(p, v):
    p.fm(1, ratio=pick(1, 2, 3), depth=rr(0.3, 0.55), feedback=rr(0.1, 0.4), vol=0.6)
    p.fm(2, ratio=pick(1, 0.5), depth=0.2, tune=pick(0, 12, -12), fine=6, vol=0.35)
    p.vcf(1, 'Smooth', 'LP 12', cutoff=6000, reso=0.1)
    p.grid(1, 'FM 1', 'FM 2', 'VCF 1')
    p.voice('Legato', glide=0.05, voices=1)
    p.env(1, a=0.003, d=1.2, s=0.8, r=0.3)
    p.env(2, a=0.002, d=0.5, s=0.3, r=0.3)
    p.mod('Env 2', 'FM 1 Depth', 0.2)
    p.mod('Velocity', 'FM 1 Depth', 0.1)
    p.vibrato(1, ['FM 1 Tune', 'FM 2 Tune'], 0.006)
    space(p, [1], rev=0.2, dly=0.25)
    macros_std(p, 'Tone: FM brightness', [('FM 1 Depth', 0.3)], 'Motion: modulator feedback', [('FM 1 Feedback', 0.4)],
               d_label='Character: ratio bend', d_routes=[('FM 1 Ratio', 0.1)])
    return 'Biting FM lead with a feedback modulator; harder playing adds edge. Mod wheel vibrato.'


def lead_chip(p, v):
    p.osc(1, 'Pulse Width', wave=pick(0.0, 0.5, 0.75), vol=0.6, fx1='Quantize', fx1amt=rr(0.3, 0.6))
    p.vcf(1, 'Smooth', 'LP 12', cutoff=9000, reso=0.0, kt=0.0)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.voice('Mono', glide=pick(0, 0.02), voices=1)
    p.env(1, a=0.001, d=0.3, s=0.7, r=0.08, slope=0)
    p.lfo(2, 'Triangle', 5.5, delay=0.25)
    p.mod('LFO 2', 'OSC 1 Tune', 0.004)
    p.mseg_p(1, [(0, 1, 0), (0.06, 0.5, 0), (0.12, 0.5, 0)], loop='One Shot', time=1.0)
    p.mod('MSEG 1', 'OSC 1 Tune', 0.03)
    space(p, [1], rev=0.1, dly=0.2, divl='1/8', divr='1/8.')
    macros_std(p, 'Tone: pulse width', [('OSC 1 Wave', 0.25)], 'Motion: wider vibrato', [],
               d_label='Character: bit-crunch', d_routes=[('OSC 1 FX 1 Amt', 0.4)])
    p.mod('LFO 2', 'OSC 1 Tune', 0.008, via='Macro B')
    return '8-bit style game lead: a quantized pulse wave with an instant pitch blip on each note and delayed vibrato.'


def lead_whistle(p, v):
    p.osc(1, 'Classic', wave=0.0, vol=0.7)
    p.osc(2, 'Classic', wave=0.33, tune=12, vol=0.12)
    p.vcf(1, 'SVF', 'LP 12', cutoff=4000, reso=0.05)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.voice('Legato', glide=rr(0.12, 0.3), voices=1)
    p.env(1, a=rr(0.05, 0.2), d=1, s=1, r=rr(0.4, 1.0))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.004, rate=6.5)
    p.lfo(2, 'Sine', 6.5, delay=0.3, fade=0.8)
    p.mod('LFO 2', 'OSC 1 Tune', 0.003)
    p.mod('LFO 2', 'OSC 2 Tune', 0.003)
    space(p, [1], rev=0.45, dly=0.2, model='Lush', decay=5)
    macros_std(p, 'Tone: hollow overtone', [('OSC 1 Wave', 0.3)], 'Motion: deeper vibrato', [],
               d_label='Character: octave overtone', d_routes=[('OSC 2 Volume', 0.3)])
    p.mod('LFO 2', 'OSC 1 Tune', 0.006, via='Macro B')
    return 'Theremin-like sine with long glide and blooming vibrato, the sound of 1950s sci-fi and haunted houses.'


def lead_formant(p, v):
    p.osc(1, 'Classic', wave=0.66, uni=2, det=0.1, vol=0.6)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(500, 1500, 0), reso=rr(0.4, 0.7))
    p.shape(1, 'Tube', drive=rr(0.2, 0.5))
    p.grid(1, 'OSC 1', 'SHAPE 1', 'VCF 1')
    p.voice('Legato', glide=0.05, voices=1)
    p.env(1, a=0.005, d=1, s=0.9, r=0.25)
    p.lfo(1, pick('Sine', 'Triangle'), rr(0.5, 3), sync=pick(True, False), div='1/8')
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.2, via='Mod Wheel')
    p.env(2, a=0.05, d=0.5, s=0.5, r=0.3)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.15)
    space(p, [1], rev=0.2, dly=0.2)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: talk rate', [('LFO 1 Rate', 0.3)],
               d_label='Grit: drive', d_routes=[('SHAPE 1 Drive', 0.4)])
    return 'Talking lead: a driven saw through the formant filter. The mod wheel brings in a "yoy" vowel LFO, like a talk box.'


# =========================================================================== BASS
def bass_sub(p, v):
    p.osc(1, 'Classic', wave=0.0, vol=0.75, tune=pick(0, -12))
    p.osc(2, 'Classic', wave=rr(0.5, 0.7), vol=0.25)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(200, 500, 0), reso=0.1, kt=0.3)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.voice('Mono', glide=pick(0.0, 0.03), voices=1)
    p.env(1, a=0.003, d=0.5, s=0.9, r=0.15)
    p.env(2, a=0.002, d=rr(0.1, 0.3), s=0.2, r=0.2)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.15)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.25)], 'Motion: envelope punch', [('ENV 2 Decay', 0.2)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Grit: saw level', d_routes=[('OSC 2 Volume', 0.4)])
    return 'Solid sub bass: a sine with a whisper of filtered saw for definition on small speakers.'


def bass_growl(p, v):
    p.osc(1, pick('Growl', 'Fold Sweep', 'Metallic'), wave=rr(0.2, 0.5), vol=0.6, uni=2, det=0.1)
    p.osc(2, 'Classic', wave=0.0, tune=-12, vol=0.5)
    p.shape(1, pick('Asym Fold', 'Tube', 'Wavefold'), drive=rr(0.4, 0.7), bias=rr(-0.2, 0.2), tone=0.6)
    p.vcf(1, pick('Ladder', 'Diode', 'Bite'), 'LP 24', cutoff=rr(500, 1400, 0), reso=rr(0.2, 0.4))
    p.grid(1, 'OSC 1', 'SHAPE 1', 'VCF 1')
    p.grid(2, 'OSC 2')
    p.lane(2, vol=-3)
    p.voice('Mono', glide=0.03, voices=1)
    p.env(1, a=0.003, d=0.6, s=0.9, r=0.15)
    p.env(2, a=0.003, d=0.4, s=0.35, r=0.2)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.2)
    p.mod('Env 2', 'OSC 1 Wave', 0.3)
    p.fxgrid(1, 'COMP 1')
    p.comp(1, threshold=-16, ratio=3, attack=8, release=100)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: growl (table scan)', [('OSC 1 Wave', 0.4)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Grit: fold', d_routes=[('SHAPE 1 Drive', 0.3)])
    return 'Snarling bass for metal and djent-flavoured synths: a folded growl table over a clean sine sub in its own lane.'


def bass_fm(p, v):
    p.fm(1, ratio=pick(1, 2, 0.5), depth=rr(0.3, 0.6), feedback=rr(0.0, 0.3), vol=0.65)
    p.osc(1, 'Classic', wave=0.0, tune=-12, vol=0.35)
    p.vcf(1, 'Smooth', 'LP 12', cutoff=5000, reso=0.05, kt=0.3)
    p.grid(1, 'FM 1', 'OSC 1', 'VCF 1')
    p.voice('Mono', glide=0.02, voices=1)
    p.env(1, a=0.002, d=0.8, s=0.6, r=0.15)
    p.env(2, a=0.001, d=rr(0.12, 0.35), s=0.15, r=0.2)
    p.mod('Env 2', 'FM 1 Depth', 0.3)
    p.mod('Velocity', 'FM 1 Depth', 0.15)
    macros_std(p, 'Tone: FM depth', [('FM 1 Depth', 0.25)], 'Motion: feedback', [('FM 1 Feedback', 0.4)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Character: ratio', d_routes=[('FM 1 Ratio', 0.08)])
    return 'Punchy FM bass with a fast index envelope and sine sub. Velocity adds bite, great for prog and game grooves.'


def bass_analog(p, v):
    p.osc(1, 'Classic', wave=0.66, vol=0.55)
    p.osc(2, 'Classic', wave=pick(0.66, 1.0), tune=-12, fine=rr(3, 8, 1), vol=0.5)
    p.osc(3, 'Classic', wave=1.0, tune=pick(0, 12), fine=-4, vol=0.25)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(350, 900, 0), reso=rr(0.2, 0.45), drive=rr(0.1, 0.4), kt=0.4)
    p.grid(1, 'OSC 1', 'OSC 2', 'OSC 3', 'VCF 1')
    p.voice('Mono', glide=rr(0.0, 0.08), glidemode='Legato Only', voices=1)
    p.env(1, a=0.002, d=0.6, s=0.8, r=0.12)
    p.env(2, a=0.002, d=rr(0.15, 0.5), s=0.25, r=0.15, vel=0.5)
    p.mod('Env 2', 'VCF 1 Cutoff', rr(0.2, 0.35))
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: resonance', [('VCF 1 Reso', 0.3)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Grit: ladder drive', d_routes=[('VCF 1 Drive', 0.5)])
    return 'Three-oscillator analogue bass in the classic prog mould: fat, squelchy and responsive to velocity.'


def bass_rock(p, v):
    p.osc(1, 'Classic', wave=0.66, vol=0.6)
    p.osc(2, 'Pulse Width', wave=0.3, tune=-12, vol=0.45)
    p.shape(1, pick('Tube', 'Hard Clip'), drive=rr(0.45, 0.7), tone=0.55)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(700, 1500, 0), reso=0.15)
    p.eq(1, lowfreq=100, lowgain=3, midfreq=800, midgain=rr(-4, 2), highgain=-3)
    p.grid(1, 'OSC 1', 'OSC 2', 'SHAPE 1', 'VCF 1')
    p.grid(2)
    p.voice('Mono', glide=0.0, voices=1)
    p.env(1, a=0.002, d=0.7, s=0.85, r=0.1)
    p.env(2, a=0.002, d=0.2, s=0.4, r=0.1)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.15)
    p.fxgrid(1, 'COMP 1')
    p.comp(1, threshold=-18, ratio=4, attack=10, release=120)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: pick attack', [('ENV 2 Decay', -0.2)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Grit: overdrive', d_routes=[('SHAPE 1 Drive', 0.3)])
    return 'Distorted rock bass with a compressor: sits under guitars like an overdriven bass amp.'


def bass_chip(p, v):
    p.osc(1, 'Pulse Width', wave=0.0, vol=0.65, fx1='Quantize', fx1amt=0.5)
    p.osc(2, 'Triangle Fold', wave=0.0, tune=-12, vol=0.4, fx1='Quantize', fx1amt=0.7)
    p.grid(1, 'OSC 1', 'OSC 2')
    p.voice('Mono', voices=1)
    p.env(1, a=0.001, d=0.25, s=0.6, r=0.05, slope=0)
    macros_std(p, 'Tone: pulse width', [('OSC 1 Wave', 0.3)], 'Motion: triangle layer', [('OSC 2 Volume', 0.3)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Character: crunch', d_routes=[('OSC 1 FX 1 Amt', 0.4)])
    return 'Console-style game bass: quantized pulse plus a stepped triangle an octave down.'


def bass_detuned(p, v):
    p.osc(1, 'Classic', wave=0.66, uni=pick(3, 5), det=rr(0.2, 0.35), vol=0.55)
    p.osc(2, 'Classic', wave=0.0, tune=-12, vol=0.5)
    p.vcf(1, pick('Ladder', 'OTA'), 'LP 24', cutoff=rr(400, 1000, 0), reso=0.2)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.grid(2, 'OSC 2')
    p.voice('Mono', glide=0.05, voices=1)
    p.env(1, a=0.003, d=0.6, s=0.9, r=0.2)
    p.lfo(1, 'Sine', rr(0.1, 0.4))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.08)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: slow filter drift', [('LFO 1 Rate', 0.3), ('VCF 1 Reso', 0.2)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Character: unison spread', d_routes=[('OSC 1 Detune', 0.3)])
    return 'Wide detuned-saw bass over a mono sub lane, for dark cinematic and game cues.'


# =========================================================================== KEYS & BELLS
def keys_epiano(p, v):
    p.fm(1, ratio=1, depth=rr(0.25, 0.4), feedback=0.05, vol=0.6)
    p.fm(2, ratio=pick(14, 13, 7), depth=rr(0.1, 0.25), vol=0.25)
    p.grid(1, 'FM 1', 'FM 2')
    p.env(1, a=0.002, d=rr(2, 4), s=0.0, r=rr(0.4, 0.8), slope=0.5, keyscale=0.4, vel=0.5)
    p.env(2, a=0.001, d=rr(0.6, 1.2), s=0.1, r=0.3, keyscale=0.3)
    p.env(3, a=0.001, d=0.08, s=0.0, r=0.05)
    p.mod('Env 2', 'FM 1 Depth', 0.25)
    p.mod('Velocity', 'FM 1 Depth', 0.2)
    p.mod('Env 3', 'FM 2 Volume', 0.3)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, pick('Chorus', 'Phaser'), rate=0.6, depth=0.4, mix=0.35)
    space(p, [1], rev=0.25)
    macros_std(p, 'Tone: bark (FM depth)', [('FM 1 Depth', 0.25)], 'Motion: tremolo', [],
               d_label='Character: tine level', d_routes=[('FM 2 Volume', 0.3)])
    p.lfo(1, 'Sine', 4.5)
    p.mod('LFO 1', 'Lane 1 Volume', 0.04, via='Macro B')
    return 'Electric piano in the FM tradition: a soft body with a metallic tine attack that gets barkier the harder you play.'


def keys_bell(p, v):
    if R.random() < 0.5:
        p.osc(1, pick('Bell Partials', 'Glass', 'Metallic'), engine='Additive', wave=rr(0.3, 1.0), partials=40,
              stretch=rr(0.15, 0.45), pdecay=rr(0.4, 0.8), bright=rr(-0.2, 0.2), vol=0.6)
        p.grid(1, 'OSC 1')
        kind = 'additive bell with stretched partials that decay from the top down'
        cutoff_dst = [('OSC 1 Bright', 0.4)]
        d_routes = [('OSC 1 Stretch', 0.3)]
    else:
        p.exc(1, 'Mallet', decay=rr(10, 40, 0), color=rr(0.0, 0.5))
        p.res(1, pick('Bell', 'Glass', 'Vibes'), tune=pick(0, 12), decay=rr(2, 5), damping=rr(0.3, 0.6), bright=rr(0.0, 0.3), spread=0.2)
        p.grid(1, 'EXC 1', 'RES 1')
        kind = 'physically modelled bell: a mallet striking a resonator'
        cutoff_dst = [('RES 1 Bright', 0.4)]
        d_routes = [('RES 1 Spread', 0.4)]
    p.env(1, a=0.001, d=rr(3, 6), s=0.0, r=rr(1.5, 3), slope=0.5, keyscale=0.3, vel=0.6)
    p.lfo(1, 'Sine', 4.5)
    space(p, [1], rev=0.4, dly=0.15, model='Lush', decay=5)
    macros_std(p, 'Tone: brightness', cutoff_dst, 'Motion: tremolo', [], d_label='Character: inharmonicity', d_routes=d_routes)
    p.mod('LFO 1', 'Lane 1 Volume', 0.04, via='Macro B')
    return f'Clear {kind}. Velocity shapes level and brightness.'


def keys_musicbox(p, v):
    p.osc(1, pick('Glass', 'Sine Stack'), engine='Additive', wave=0.5, partials=24, stretch=rr(0.05, 0.15), pdecay=0.8, vol=0.55)
    p.osc(2, 'Classic', wave=0.0, tune=24, fine=rr(-14, 14, 1), vol=0.15)
    p.grid(1, 'OSC 1', 'OSC 2')
    p.env(1, a=0.001, d=rr(1.2, 2.5), s=0.0, r=1.2, slope=0.6, keyscale=0.5)
    p.lfo(1, 'Smooth Rnd', 0.3)
    p.mod('LFO 1', 'OSC 1 Tune', rr(0.002, 0.006))
    space(p, [1], rev=0.5, dly=0.3, model='Lush', decay=6, divl='1/4.', divr='1/8.')
    macros_std(p, 'Tone: brightness', [('OSC 1 Bright', 0.4)], 'Motion: detune wobble (creepier)', [],
               d_label='Character: high overtone', d_routes=[('OSC 2 Volume', 0.3)])
    p.mod('LFO 1', 'OSC 1 Tune', 0.01, via='Macro B')
    return 'A slightly broken music box: delicate additive tines that waver out of tune. Perfect for horror lullabies and game menus.'


def keys_clav(p, v):
    p.osc(1, 'Clav Pluck', wave=rr(0.3, 0.8), vol=0.6)
    p.vcf(1, pick('SVF', 'Bite'), pick('BP 12', 'LP 12', 'HP 12'), cutoff=rr(1500, 3500, 0), reso=0.35, kt=0.6)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=0.001, d=rr(0.4, 0.9), s=0.2, r=0.1)
    p.env(2, a=0.001, d=0.15, s=0.2, r=0.1)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.2)
    p.mod('Velocity', 'VCF 1 Cutoff', 0.1)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Phaser', rate=0.5, depth=0.5, feedback=0.3, mix=0.4)
    space(p, [1], rev=0.15)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: phaser', [('MODFX 1 Mix', 0.4)],
               d_label='Character: pulse width', d_routes=[('OSC 1 Wave', 0.3)])
    return 'Funky clavinet-style keys with a quick filter snap and phaser: a prog and fusion staple.'


def keys_organ(p, v):
    p.osc(1, 'Organ Bars', wave=rr(0.4, 1.0), vol=0.6)
    p.osc(2, 'Organ Bars', wave=rr(0.0, 0.6), tune=pick(12, 19), vol=0.3)
    p.shape(1, 'Tube', drive=rr(0.25, 0.55), tone=0.65)
    p.grid(1, 'OSC 1', 'OSC 2', 'SHAPE 1')
    p.env(1, a=0.003, d=1, s=1.0, r=0.08)
    p.env(2, a=0.001, d=0.06, s=0, r=0.05)
    p.mod('Env 2', 'OSC 2 Volume', 0.25)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=rr(0.7, 1.2), depth=0.55, voices=2, mix=0.5)
    p.mod('Mod Wheel', 'MODFX 1 Rate', 0.45)
    space(p, [1], rev=0.2)
    macros_std(p, 'Tone: drawbars', [('OSC 1 Wave', 0.3)], 'Motion: rotary speed (also mod wheel)', [('MODFX 1 Rate', 0.45)],
               d_label='Grit: overdriven amp', d_routes=[('SHAPE 1 Drive', 0.4)])
    return 'Overdriven tonewheel-style organ with key click and a rotary chorus. The mod wheel switches the rotor fast.'


def keys_mallet(p, v):
    prof = pick('Marimba', 'Vibes', 'Glass', 'Plate')
    p.exc(1, 'Mallet', decay=rr(8, 30, 0), color=rr(-0.4, 0.2))
    p.res(1, prof, decay=rr(0.6, 2.5), damping=rr(0.3, 0.7), bright=rr(-0.2, 0.2), spread=rr(0.05, 0.2))
    p.grid(1, 'EXC 1', 'RES 1')
    p.env(1, a=0.001, d=3, s=0.0, r=rr(0.5, 1.5), slope=0.5, vel=0.6)
    p.mod('Velocity', 'EXC 1 Color', 0.3)
    p.lfo(1, 'Sine', 4.0)
    space(p, [1], rev=0.3)
    macros_std(p, 'Tone: mallet hardness', [('EXC 1 Color', 0.5)], 'Motion: vibraphone tremolo', [],
               d_label='Character: decay length', d_routes=[('RES 1 Decay', 0.2)])
    p.mod('LFO 1', 'Lane 1 Volume', 0.05, via='Macro B')
    return f'Modelled {prof.lower()}: a mallet exciter striking a tuned resonator. Harder playing uses a harder mallet.'


# =========================================================================== PLUCKS
def pluck_string(p, v):
    p.exc(1, pick('Burst', 'Impulse', 'Mallet'), decay=rr(3, 12, 1), color=rr(-0.2, 0.5))
    p.comb(1, 'Comb', feedback=rr(0.986, 0.996), damping=rr(0.08, 0.35), mix=1.0, vol=1.0)
    p.vcf(1, 'SVF', 'LP 12', cutoff=rr(3000, 9000, 0), reso=0.05)
    p.grid(1, 'EXC 1', 'COMB 1', 'VCF 1')
    p.env(1, a=0.001, d=3, s=0.0, r=rr(0.6, 1.5), slope=0.4, keyscale=0.3)
    p.mod('Velocity', 'EXC 1 Color', 0.3)
    space(p, [1], rev=0.3, dly=0.15)
    macros_std(p, 'Tone: brightness', [('COMB 1 Damping', -0.4)], 'Motion: string sustain', [('COMB 1 Feedback', 0.02)],
               d_label='Character: pick hardness', d_routes=[('EXC 1 Color', 0.4)])
    return 'Karplus-style plucked string: an exciter burst into a tuned comb. Ideal for harp, lute and fantasy-tavern parts.'


def pluck_synth(p, v):
    p.osc(1, pick('Classic', 'Brass Swell', 'Pulse Width'), wave=rr(0.5, 0.8), uni=pick(1, 2, 3), det=0.12, vol=0.6)
    p.osc(2, 'Classic', wave=0.66, tune=pick(12, -12, 7), vol=0.3)
    p.vcf(1, pick('Ladder', 'SVF', 'Cascade'), 'LP 24', cutoff=rr(300, 900, 0), reso=rr(0.2, 0.45), kt=0.6)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=0.001, d=rr(0.3, 0.8), s=0.0, r=rr(0.2, 0.5), slope=0.5)
    p.env(2, a=0.001, d=rr(0.15, 0.4), s=0.0, r=0.2, vel=0.5)
    p.mod('Env 2', 'VCF 1 Cutoff', rr(0.3, 0.45))
    space(p, [1], rev=0.25, dly=0.25, divl='1/8.', divr='1/8')
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: decay length', [('ENV 2 Decay', 0.3), ('ENV 1 Decay', 0.3)],
               d_label='Character: resonance', d_routes=[('VCF 1 Reso', 0.3)])
    return 'Snappy subtractive pluck with a fast filter envelope and dotted delay.'


def pluck_fm(p, v):
    p.fm(1, ratio=pick(2, 3, 4, 3.5), depth=rr(0.3, 0.6), feedback=0.1, vol=0.6)
    p.grid(1, 'FM 1')
    p.env(1, a=0.001, d=rr(0.6, 1.5), s=0.0, r=0.4, slope=0.5)
    p.env(2, a=0.001, d=rr(0.1, 0.3), s=0, r=0.2)
    p.mod('Env 2', 'FM 1 Depth', 0.3)
    space(p, [1], rev=0.3, dly=0.25)
    macros_std(p, 'Tone: FM depth', [('FM 1 Depth', 0.25)], 'Motion: feedback', [('FM 1 Feedback', 0.3)],
               d_label='Character: ratio (clang)', d_routes=[('FM 1 Ratio', 0.06)])
    return 'Glassy FM pluck: the modulation index decays fast for a percussive, kalimba-like tone.'


def pluck_fantasy(p, v):
    p.exc(1, 'Burst', decay=rr(4, 10, 1), color=0.3)
    p.comb(1, 'Comb', feedback=rr(0.97, 0.99), damping=0.25, vol=0.6)
    p.res(1, pick('String', 'Glass', 'Tube'), decay=1.5, damping=0.5, mix=rr(0.3, 0.6), input=-6)
    p.grid(1, 'EXC 1', 'COMB 1', 'RES 1')
    p.env(1, a=0.001, d=3, s=0, r=1.2, slope=0.4)
    p.lfo(1, 'Sine', 5.0, delay=0.3, fade=0.5)
    p.mod('LFO 1', 'COMB 1 Tune', 0.003, via='Mod Wheel')
    space(p, [1], rev=0.4, dly=0.2, model='Lush', decay=4)
    macros_std(p, 'Tone: brightness', [('COMB 1 Damping', -0.4)], 'Motion: sustain', [('COMB 1 Feedback', 0.015)],
               d_label='Character: sympathetic body', d_routes=[('RES 1 Mix', 0.4)])
    return 'Plucked string with a sympathetic resonator body, evoking zither and lute. Mod wheel bends a gentle vibrato.'


def pluck_dark(p, v):
    p.osc(1, pick('Ghost', 'Hollow Odd', 'Reed'), wave=rr(0.2, 0.8), uni=2, det=0.3, vol=0.6)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(300, 700, 0), reso=0.35)
    p.ring(1, 'Sine', tune=pick(6, 11, 13), mix=rr(0.1, 0.3))
    p.grid(1, 'OSC 1', 'RING 1', 'VCF 1')
    p.env(1, a=0.001, d=rr(0.6, 1.2), s=0, r=0.8)
    p.env(2, a=0.001, d=0.3, s=0, r=0.3)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.3)
    space(p, [1], rev=0.4, dly=0.25, model='Lush', decay=6, damping=0.6)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: ring dissonance', [('RING 1 Mix', 0.4)],
               d_label='Character: unison spread', d_routes=[('OSC 1 Detune', 0.4)])
    return 'Dark, slightly dissonant pluck for horror and dungeon music: a ring-modulated tone with a short low-pass snap.'


# =========================================================================== ARPS & SEQUENCES
ODD_STEPS = [5, 7, 9, 11, 13]


def arp_pattern(kind):
    if kind == 'octave':
        return [(True, 0, 1.0, 0.6), (True, 12, 0.7, 0.5), (True, 0, 0.85, 0.6), (True, 7, 0.7, 0.5)] * 4
    if kind == 'gallop':
        return [(True, 0, 1.0, 0.4), (True, 0, 0.6, 0.4), (True, 0, 0.75, 0.4), (False, 0, 0.8, 0.5)] * 4
    if kind == 'rand':
        return [(R.random() > 0.2, R.choice([0, 0, 7, 12, -12, 5]), rr(0.55, 1.0), rr(0.3, 0.9)) for _ in range(16)]
    if kind == 'legato':
        return [(True, 0, 0.9, 1.0), (True, 0, 0.7, 0.6), (True, 12, 0.8, 1.0), (True, 0, 0.7, 0.5)] * 4
    return [(True, 0, 0.9 if k % 4 == 0 else 0.7, 0.7) for k in range(16)]


def arp_synth(p, v):
    kind = pick('octave', 'gallop', 'rand', 'legato', 'plain')
    steps = pick(8, 16, *ODD_STEPS) if 'Prog' in p.tags else pick(8, 16, 6, 12)
    p.osc(1, pick('Classic', 'Pulse Width', 'Sync Sweep', 'Brass Swell'), wave=rr(0.3, 0.8), uni=pick(1, 2), det=0.1, vol=0.6)
    p.osc(2, 'Classic', wave=0.66, tune=pick(12, -12, 7), vol=0.3)
    p.vcf(1, pick('Ladder', 'Diode', 'SVF'), 'LP 24', cutoff=rr(500, 1500, 0), reso=rr(0.3, 0.55), kt=0.5)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=0.001, d=0.4, s=0.2, r=0.15)
    p.env(2, a=0.001, d=rr(0.12, 0.3), s=0.1, r=0.1)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.3)
    p.lfo(1, 'Triangle', 0.1, sync=True, div=pick('2 bars', '4 bars'))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.15)
    p.arp(mode=pick('Up', 'Up Down', 'As Played', 'Random', 'Down Up'), div=pick('1/16', '1/16', '1/8', '1/16T'),
          steps=steps, octaves=pick(1, 2, 3), gate=rr(0.6, 0.9), pattern=arp_pattern(kind))
    space(p, [1], rev=0.25, dly=0.3, divl='1/8.', divr='1/4', fb=0.4)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: envelope snap', [('ENV 2 Decay', 0.3)],
               d_label='Character: resonance', d_routes=[('VCF 1 Reso', 0.3)])
    return f'Tempo-synced arpeggio ({steps}-step pattern) with a swept low-pass. Hold a chord; the pattern follows your DAW tempo.'


def arp_chip(p, v):
    p.osc(1, 'Pulse Width', wave=pick(0.0, 0.5, 0.75), vol=0.6, fx1='Quantize', fx1amt=0.5)
    p.grid(1, 'OSC 1')
    p.env(1, a=0.001, d=0.15, s=0.5, r=0.05, slope=0)
    p.arp(mode=pick('Up', 'Up Down', 'Chord'), div=pick('1/16', '1/32', '1/16T'), steps=pick(4, 8, 16), octaves=pick(1, 2, 3),
          gate=0.6, pattern=arp_pattern(pick('octave', 'plain', 'rand')))
    space(p, [1], rev=0.08, dly=0.2, divl='1/8', divr='1/8.')
    macros_std(p, 'Tone: pulse width', [('OSC 1 Wave', 0.3)], 'Motion: gate length', [('ENV 1 Decay', 0.2)],
               d_label='Character: crunch', d_routes=[('OSC 1 FX 1 Amt', 0.4)])
    return 'Retro game arpeggio: a fast quantized-pulse arp in the style of handheld consoles. Hold a chord.'


def seq_mapper(p, v):
    steps = pick(8, 16, 7, 5, 12)
    pattern = [R.choice([-1, -0.5, 0, 0.25, 0.5, 1]) for _ in range(steps)]
    p.osc(1, pick('Classic', 'Growl', 'Digital Bits', 'Spline Shapes'), wave=rr(0.3, 0.7), uni=2, det=0.12, vol=0.6)
    p.vcf(1, pick('Ladder', 'Diode', 'Twin Peak'), 'LP 24', cutoff=rr(400, 1200, 0), reso=rr(0.35, 0.6))
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=0.01, d=1, s=1, r=0.3)
    p.map_p(1, pattern, mode='Step Seq', div=pick('1/16', '1/8', '1/16T'), smooth=rr(0, 0.3), amount=0.6)
    p.mod('Map 1', 'VCF 1 Cutoff', rr(0.2, 0.35))
    p.mod('Map 1', 'OSC 1 Wave', 0.2)
    p.lfo(1, 'Sine', 0.1, sync=True, div='4 bars')
    p.mod('LFO 1', 'VCF 1 Reso', 0.15)
    space(p, [1], rev=0.25, dly=0.25, divl='1/8.', divr='1/4')
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: sequence depth', [('MAP 1 Amount', 0.4)],
               d_label='Character: resonance', d_routes=[('VCF 1 Reso', 0.3)])
    return f'Hold a note: a {steps}-step mapper sequence pumps the filter in time, prog-style. Edit the bars in MAP 1.'


def seq_bass(p, v):
    p.osc(1, 'Classic', wave=0.66, vol=0.6)
    p.osc(2, 'Classic', wave=1.0, tune=-12, vol=0.4)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(300, 700, 0), reso=0.4, drive=0.2)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.voice('Mono', voices=1)
    p.env(1, a=0.001, d=0.3, s=0.3, r=0.1)
    p.env(2, a=0.001, d=0.18, s=0.0, r=0.1)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.35)
    p.arp(mode='As Played', div=pick('1/16', '1/8'), steps=pick(8, 16, 7, 6), octaves=1, gate=0.7,
          pattern=[(R.random() > 0.15, R.choice([0, 0, 0, 12, -12, 7, 10]), rr(0.6, 1.0), rr(0.4, 0.9)) for _ in range(16)])
    space(p, [1], rev=0.1, dly=0.15)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: envelope snap', [('ENV 2 Decay', 0.3)],
               c_routes=[('Aux 1 Return', 0.1)], d_label='Grit: ladder drive', d_routes=[('VCF 1 Drive', 0.5)])
    return 'Sequenced bassline: hold one note and the arpeggiator plays an octave-jumping riff in time with the host.'


def seq_bells(p, v):
    p.exc(1, 'Mallet', decay=10, color=0.3)
    p.res(1, pick('Bell', 'Glass', 'Vibes'), decay=rr(1, 3), damping=0.4, bright=0.2)
    p.grid(1, 'EXC 1', 'RES 1')
    p.env(1, a=0.001, d=3, s=0, r=1.5)
    p.arp(mode=pick('Up', 'Random', 'Up Down'), div=pick('1/16', '1/8', '1/16T'), steps=pick(8, 16, 5, 7), octaves=pick(2, 3), gate=0.5,
          pattern=arp_pattern(pick('rand', 'octave')))
    space(p, [1], rev=0.45, dly=0.35, model='Lush', decay=6, divl='1/8.', divr='1/4.')
    p.delay(1, mode='Ping Pong', divl='1/8.', feedback=0.5, mix=1.0)
    macros_std(p, 'Tone: brightness', [('RES 1 Bright', 0.4)], 'Motion: ring time', [('RES 1 Decay', 0.2)],
               d_label='Character: mallet hardness', d_routes=[('EXC 1 Color', 0.4)])
    return 'Twinkling arpeggiated bells through ping-pong delay: magical forests, crystal caves and title screens.'


# =========================================================================== BRASS & STRINGS
def brass_synth(p, v):
    p.osc(1, 'Brass Swell', wave=0.0, uni=pick(2, 3, 4), det=0.15, vol=0.6)
    p.osc(2, 'Classic', wave=0.66, tune=pick(0, -12), fine=6, uni=2, det=0.1, vol=0.45)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(600, 1200, 0), reso=0.15, drive=0.25)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.02, 0.08), d=0.8, s=0.8, r=0.3)
    p.env(2, a=rr(0.05, 0.2), d=rr(0.5, 1.0), s=0.4, r=0.3, vel=0.6)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.3)
    p.mod('Env 2', 'OSC 1 Wave', 0.6)
    p.mod('Velocity', 'VCF 1 Cutoff', 0.1)
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.005)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.4, depth=0.3, mix=0.25)
    space(p, [1], rev=0.3)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: swell time', [('ENV 2 Attack', 0.25)],
               d_label='Grit: ladder drive', d_routes=[('VCF 1 Drive', 0.5)])
    return 'Bold synth brass: the filter and brass table swell together on each note. Play chords with conviction.'


def brass_stab(p, v):
    p.osc(1, 'Classic', wave=0.66, uni=pick(3, 5), det=0.2, vol=0.6)
    p.osc(2, 'Brass Swell', wave=0.5, tune=12, vol=0.35)
    p.vcf(1, pick('Ladder', 'OTA'), 'LP 24', cutoff=rr(800, 1500, 0), reso=0.2)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=0.002, d=rr(0.3, 0.6), s=0.0, r=0.3)
    p.env(2, a=0.002, d=0.25, s=0, r=0.2)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.35)
    space(p, [1], rev=0.4, model='Lush', decay=3)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: stab length', [('ENV 1 Decay', 0.3)],
               d_label='Character: octave brass layer', d_routes=[('OSC 2 Volume', 0.3)])
    return 'Short brass stab with a hall tail, for prog hits, game fanfares and trailer punctuation.'


def strings_ensemble(p, v):
    p.osc(1, 'Bowed String', wave=rr(0.2, 0.7), uni=pick(4, 6, 8), det=rr(0.2, 0.3), vol=0.6)
    p.osc(2, 'Pulse Width', wave=0.3, tune=pick(0, 12), uni=2, det=0.2, vol=0.3)
    p.vcf(1, 'Cascade', 'LP 24', cutoff=rr(2500, 5000, 0), reso=0.1)
    p.vcf(2, 'Smooth', 'HP 12', cutoff=120, reso=0.0, kt=0)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1', 'VCF 2')
    p.env(1, a=rr(0.2, 0.6), d=1, s=0.9, r=rr(0.6, 1.5))
    p.lfo(1, 'Triangle', 0.5)
    p.mod('LFO 1', 'OSC 2 Wave', 0.2)
    p.vibrato(2, ['OSC 1 Tune', 'OSC 2 Tune'], 0.004, rate=5.0)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.6, depth=0.6, voices=6, mix=0.5)
    space(p, [1], rev=0.4, model='Lush', decay=4)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: bow position', [('OSC 1 Wave', 0.4)],
               d_label='Character: ensemble width', d_routes=[('MODFX 1 Depth', 0.3), ('OSC 1 Detune', 0.2)])
    return 'String ensemble machine with a bowed-string table and wide chorus; think 70s prog symphonic.'


def strings_bowed(p, v):
    p.exc(1, 'Bow', decay=rr(80, 250, 0), color=rr(0.0, 0.4), level=0.7)
    p.comb(1, 'Comb', feedback=rr(0.97, 0.99), damping=rr(0.3, 0.5), vol=0.5)
    p.res(1, 'String', decay=2, damping=0.6, mix=0.5, input=-12)
    p.grid(1, 'EXC 1', 'COMB 1', 'RES 1')
    p.osc(1, 'Bowed String', wave=0.4, vol=0.3)
    p.vcf(1, 'Cascade', 'LP 24', cutoff=3000)
    p.grid(2, 'OSC 1', 'VCF 1')
    p.lane(2, vol=-6)
    p.env(1, a=rr(0.15, 0.4), d=1, s=1, r=rr(0.5, 1.2))
    p.vibrato(1, ['COMB 1 Tune', 'OSC 1 Tune'], 0.004, rate=5.2, delay=0.3, fade=0.6)
    space(p, [1, 2], rev=0.45, model='Lush', decay=4)
    macros_std(p, 'Tone: bow brightness', [('EXC 1 Color', 0.4)], 'Motion: bow pressure', [('EXC 1 Level', 0.2)],
               d_label='Character: body resonance', d_routes=[('RES 1 Mix', 0.3)])
    return 'Physically modelled bowed string: a bow exciter drives a tuned comb with a resonant body, doubled by a soft wavetable lane.'


def brass_horn(p, v):
    p.fm(1, ratio=1, depth=rr(0.2, 0.35), feedback=0.25, vol=0.6)
    p.fm(2, ratio=1, depth=0.2, tune=-12, fine=4, vol=0.4)
    p.vcf(1, 'Smooth', 'LP 12', cutoff=rr(1500, 3000, 0), reso=0.1)
    p.grid(1, 'FM 1', 'FM 2', 'VCF 1')
    p.env(1, a=rr(0.04, 0.12), d=1, s=0.85, r=0.4)
    p.env(2, a=rr(0.05, 0.2), d=0.6, s=0.5, r=0.4, vel=0.6)
    p.mod('Env 2', 'FM 1 Depth', 0.25)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.2)
    p.vibrato(1, ['FM 1 Tune', 'FM 2 Tune'], 0.005, delay=0.4, fade=0.5)
    space(p, [1], rev=0.45, model='Lush', decay=4)
    macros_std(p, 'Tone: brassiness', [('FM 1 Depth', 0.25)], 'Motion: swell', [('ENV 2 Attack', 0.25)],
               d_label='Character: rasp', d_routes=[('FM 1 Feedback', 0.4)])
    return 'FM horn section: brassy swells for heroic game themes and fantasy fanfares.'


# =========================================================================== CHOIRS & VOICES
def choir_formant(p, v):
    p.osc(1, 'Choir', wave=rr(0.0, 0.8), uni=pick(5, 6, 8), det=rr(0.2, 0.35), vol=0.6)
    p.osc(2, 'Formant Vox', wave=pick(0.0, 0.5, 1.0), tune=pick(0, -12, 12), uni=3, det=0.25, vol=0.35)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(350, 900, 0), reso=0.4, mix=0.7)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.4, 1.0), d=1, s=0.9, r=rr(1.5, 3))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.004, rate=5.0, delay=0.5, fade=1.0)
    p.mseg_p(1, [(0, 0, 0), (3, 1, 0), (6, 0.3, 0), (9, 0.7, 0), (12, 0, 0)], loop='Repeat', time=1.0, amount=0.5)
    p.mod('MSEG 1', 'VCF 1 Cutoff', 0.15)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.3, depth=0.5, voices=6, mix=0.45)
    space(p, [1], rev=0.6, model='Lush', decay=rr(5, 9), predelay=40)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: vowel drift', [('MSEG 1 Amount', 0.5)],
               d_label='Character: voice layer', d_routes=[('OSC 2 Volume', 0.3)])
    return 'Cathedral choir: Choir and Formant Vox tables with a slowly wandering formant filter in a big, pre-delayed hall.'


def choir_monk(p, v):
    p.osc(1, 'Formant Vox', wave=pick(0.75, 1.0), tune=-12, uni=3, det=0.2, vol=0.6)
    p.osc(2, 'Organ Bars', wave=0.1, tune=-24, vol=0.25)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(250, 450, 0), reso=0.5)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.env(1, a=rr(0.6, 1.5), d=1, s=1, r=rr(2, 4))
    p.lfo(1, 'Smooth Rnd', 0.1)
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.05)
    space(p, [1], rev=0.6, model='Lush', decay=rr(7, 12), damping=0.6)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.25)], 'Motion: drift', [('LFO 1 Rate', 0.4)],
               d_label='Character: low drone layer', d_routes=[('OSC 2 Volume', 0.3)])
    return 'Low chanting monks: dark "oo" vowels over a sub-octave drone. Play low, sparse chords.'


def voice_ghost(p, v):
    p.osc(1, 'Formant Vox', wave=0.5, uni=2, det=0.5, vol=0.55)
    p.noise(1, 'Pink', vol=0.2)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(500, 1500, 0), reso=0.5)
    p.grid(1, 'OSC 1', 'NOISE 1', 'VCF 1')
    p.env(1, a=rr(0.5, 1.5), d=1, s=1, r=rr(2, 4))
    p.lfo(1, 'Smooth Rnd', rr(0.3, 0.8))
    p.mod('LFO 1', 'OSC 1 Wave', 0.6)
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.15)
    p.lfo(2, 'Smooth Rnd', 0.2)
    p.mod('LFO 2', 'OSC 1 Tune', 0.012)
    p.fxgrid(4, 'TEXTURE 1')
    p.texture(1, grain=400, density=0.35, pitch=pick(-12, 0, 5), reverse=0.9, feedback=0.6, mix=1.0)
    p.lane(1, send1=0.5, send3=0.45)
    p.reverb(1, model='Lush', decay=10, size=0.9, damping=0.6, mix=1.0)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: restlessness', [('LFO 1 Rate', 0.4)],
               c_routes=[('Aux 1 Return', 0.15), ('Aux 3 Return', 0.3)], d_label='Character: breath', d_routes=[('NOISE 1 Volume', 0.3)])
    return 'Ghost voices: vowels that wander off pitch, breathing into reversed grains. For seances, haunted halls and things in the dark.'


def voice_siren(p, v):
    p.osc(1, 'Choir', wave=0.8, vol=0.6)
    p.osc(2, 'Classic', wave=0.0, tune=12, vol=0.2)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(800, 2000, 0), reso=0.45, mix=0.6)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.voice('Legato', glide=rr(0.15, 0.35), voices=1)
    p.env(1, a=rr(0.2, 0.5), d=1, s=1, r=rr(1, 2))
    p.vibrato(1, ['OSC 1 Tune', 'OSC 2 Tune'], 0.005, rate=5.5, delay=0.3, fade=0.8)
    p.lfo(2, 'Sine', 5.5, delay=0.4, fade=1.0)
    p.mod('LFO 2', 'OSC 1 Tune', 0.003)
    space(p, [1], rev=0.55, dly=0.25, model='Lush', decay=6)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: vibrato depth', [],
               d_label='Character: pure tone layer', d_routes=[('OSC 2 Volume', 0.3)])
    p.mod('LFO 2', 'OSC 1 Tune', 0.006, via='Macro B')
    return 'A lone wordless siren voice with long glides between notes, for laments, sea myths and eerie title themes.'


# =========================================================================== DRONES
def drone_dark(p, v):
    p.osc(1, 'Sine Stack', wave=rr(0.2, 0.5), tune=-12, vol=0.55)
    p.osc(2, pick('Dark Pad', 'Hollow Odd', 'Ghost'), wave=rr(0.2, 0.8), tune=pick(-12, -5, 7), uni=3, det=0.3, vol=0.35)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(300, 800, 0), reso=0.3)
    p.grid(1, 'OSC 1', 'OSC 2', 'VCF 1')
    p.noise(1, 'Brown', vol=0.2)
    p.comb(1, 'Diffuse', tune=pick(0, 12), feedback=0.8, damping=0.6, vol=0.5)
    p.grid(2, 'NOISE 1', 'COMB 1')
    p.lane(2, vol=-9)
    p.env(1, a=rr(3, 6), d=2, s=1, r=rr(5, 9))
    p.lfo(1, 'Smooth Rnd', rr(0.03, 0.1))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.15)
    p.mod('LFO 1', 'OSC 2 Wave', 0.3)
    space(p, [1, 2], rev=0.5, model='Lush', decay=rr(8, 15), damping=0.6)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.35)], 'Motion: drift speed', [('LFO 1 Rate', 0.4)],
               d_label='Character: rumble layer', d_routes=[('Lane 2 Volume', 0.15)])
    return 'Brooding cinematic drone: low sines and a drifting dark table with a rumbling comb-noise floor. Hold one note and let it breathe.'


def drone_scifi(p, v):
    p.fm(1, ratio=pick(0.5, 1.5, 2.01, 3.03), depth=rr(0.2, 0.45), feedback=rr(0.2, 0.5), tune=-12, vol=0.5)
    p.ring(1, pick('Sine', 'Square'), tune=pick(-5, 7, 11.3, 0.3), mix=rr(0.2, 0.5))
    p.vcf(1, pick('Twin Peak', 'Phaser', 'SVF'), 'LP 12', cutoff=rr(800, 2500, 0), reso=0.4)
    p.grid(1, 'FM 1', 'RING 1', 'VCF 1')
    p.osc(1, 'Digital Bits', wave=0.5, tune=-24, vol=0.25)
    p.grid(2, 'OSC 1')
    p.env(1, a=rr(1, 3), d=2, s=1, r=rr(3, 6))
    p.lfo(1, 'Sine', rr(0.05, 0.2))
    p.mod('LFO 1', 'FM 1 Depth', 0.15)
    p.lfo(2, 'S&H', rr(1, 4))
    p.mod('LFO 2', 'VCF 1 Cutoff', 0.08)
    space(p, [1, 2], rev=0.4, dly=0.2, model='Lush', decay=6)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: engine throttle', [('FM 1 Depth', 0.25), ('LFO 1 Rate', 0.3)],
               d_label='Character: ring-mod', d_routes=[('RING 1 Mix', 0.4)])
    return 'Starship engine room: a detuned FM hum ring-modulated against itself, with a stepped digital sub-layer.'


def drone_ritual(p, v):
    p.osc(1, 'Organ Bars', wave=rr(0.0, 0.5), tune=-12, vol=0.55)
    p.osc(2, 'Formant Vox', wave=0.9, tune=-12, fine=7, vol=0.35)
    p.osc(3, 'Classic', wave=0.33, tune=7 - 12, vol=0.25)
    p.vcf(1, 'Formant', 'BP 12', cutoff=rr(250, 600, 0), reso=0.4, mix=0.6)
    p.grid(1, 'OSC 1', 'OSC 2', 'OSC 3', 'VCF 1')
    p.env(1, a=rr(2, 5), d=2, s=1, r=rr(4, 8))
    p.mseg_p(1, [(0, 0, 0), (4, 1, 0.2), (8, 0, -0.2)], loop='Repeat', time=rr(1.5, 3))
    p.mod('MSEG 1', 'VCF 1 Cutoff', 0.15)
    space(p, [1], rev=0.55, model='Lush', decay=10, damping=0.6)
    macros_std(p, 'Tone: vowel', [('VCF 1 Cutoff', 0.3)], 'Motion: breathing cycle', [('MSEG 1 Time', -0.3)],
               d_label='Character: fifth layer', d_routes=[('OSC 3 Volume', 0.3)])
    return 'Ritual drone: organ, voice and a fifth below, breathing on a slow MSEG cycle. Old stone, candles and chanting.'


def drone_horror(p, v):
    p.osc(1, 'Ghost', engine='Additive', stretch=rr(-0.6, -0.2), wave=0.5, tune=-12, vol=0.5)
    p.osc(2, 'Metallic', wave=rr(0.2, 0.8), tune=pick(1, 6, 11), uni=3, det=0.5, vol=0.3)
    p.ring(1, 'Triangle', tune=rr(0.1, 0.9, 2), mix=0.3)
    p.vcf(1, 'Steiner', 'LP 12', cutoff=rr(400, 1200, 0), reso=0.5)
    p.grid(1, 'OSC 1', 'OSC 2', 'RING 1', 'VCF 1')
    p.env(1, a=rr(2, 5), d=2, s=1, r=rr(5, 9))
    p.lfo(1, 'Smooth Rnd', rr(0.05, 0.2))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.15)
    p.mod('LFO 1', 'OSC 2 Tune', 0.006)
    p.mseg_p(1, [(0, 0, 0), (0.12, 1, 0), (0.25, 0, 0), (0.37, 0.7, 0), (1.2, 0, 0)], loop='Repeat', time=rr(0.6, 1.0))
    p.fxgrid(4, 'TEXTURE 1')
    p.texture(1, grain=500, density=0.3, pitch=-12, reverse=0.7, feedback=0.7, mix=1.0)
    p.lane(1, send1=0.5, send3=0.4)
    p.reverb(1, model='Lush', decay=14, size=1.0, damping=0.65, mix=1.0)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: heartbeat pulse', [], d_label='Character: dissonance',
               d_routes=[('RING 1 Mix', 0.4), ('OSC 2 Volume', 0.2)], c_routes=[('Aux 1 Return', 0.15), ('Aux 3 Return', 0.3)])
    p.mod('MSEG 1', 'Lane 1 Volume', -0.25, via='Macro B')
    return 'Horror drone with a hidden heartbeat (Ctrl B brings it up): inharmonic metals and reversed undertow grains.'


# =========================================================================== CINEMATIC FX
def fx_braam(p, v):
    p.osc(1, 'Classic', wave=0.66, tune=-12, uni=pick(4, 6), det=0.25, vol=0.55)
    p.osc(2, 'Growl', wave=0.3, tune=-24, uni=2, det=0.15, vol=0.45)
    p.osc(3, 'Brass Swell', wave=0.4, tune=-12, vol=0.4)
    p.shape(1, 'Tube', drive=rr(0.4, 0.7), tone=0.55)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=rr(250, 600, 0), reso=0.3, drive=0.3)
    p.grid(1, 'OSC 1', 'OSC 2', 'OSC 3', 'SHAPE 1')
    p.grid(2, 'VCF 1')
    p.lane(1, vol=-60)
    p.lane(2, inp='Lane 1', inlvl=1.0)
    p.env(1, a=rr(0.02, 0.1), d=3, s=0.7, r=rr(2, 4))
    p.env(2, a=rr(0.3, 0.8), d=rr(2, 4), s=0.3, r=2)
    p.mod('Env 2', 'VCF 1 Cutoff', 0.3)
    p.mod('Env 2', 'OSC 2 Wave', 0.4)
    space(p, [2], rev=0.45, model='Lush', decay=rr(5, 8), size=0.9)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: swell time', [('ENV 2 Attack', 0.3)],
               d_label='Grit: saturation', d_routes=[('SHAPE 1 Drive', 0.3)])
    return 'Trailer braam: three huge low oscillators saturated in lane 1, fed into lane 2 for a slow ladder swell. Play low octaves.'


def fx_riser(p, v):
    beats = pick(4, 8, 16)
    p.osc(1, 'Classic', wave=0.66, uni=6, det=0.35, vol=0.5)
    p.noise(1, 'White', vol=0.4)
    p.vcf(1, 'SVF', 'BP 12', cutoff=300, reso=0.5, kt=0)
    p.grid(1, 'OSC 1', 'NOISE 1', 'VCF 1')
    p.env(1, a=0.5, d=1, s=1, r=1.5)
    p.mseg_p(1, [(0, 0, 0), (beats, 1, -0.4), (beats + 0.5, 1, 0)], loop='One Shot', sync=True, time=1.0)
    p.mod('MSEG 1', 'VCF 1 Cutoff', 0.55)
    p.mod('MSEG 1', 'OSC 1 Tune', 0.125)
    p.mod('MSEG 1', 'OSC 1 Detune', 0.4)
    p.fxgrid(1, 'FILTER 1')
    p.fxfilter(1, 'High Pass', cutoff=20, reso=0.2)
    p.mod('Macro A', 'FILTER 1 Cutoff', 0.3)
    p.macro_text['A'] = 'Tone: thins out the bottom'
    space(p, [1], rev=0.5, dly=0.3, model='Lush', decay=5, divl='1/8', divr='1/8.', fb=0.5)
    p.macro('B', 'Motion: rise speed', [('MSEG 1 Time', -0.3)])
    p.macro('C', 'Space: reverb and delay returns', [('Aux 1 Return', 0.15), ('Aux 2 Return', 0.15)])
    p.macro('D', 'Character: noise level', [('NOISE 1 Volume', 0.3)])
    return f'Tempo-synced riser: pitch, filter and detune climb for {beats} beats, then hold. Trigger it {beats} beats before the drop in your cue.'


def fx_impact(p, v):
    p.osc(1, 'Classic', wave=0.0, tune=-12, vol=0.8)
    p.noise(1, pick('White', 'Pink', 'Brown'), vol=0.6)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=3000, reso=0.1, kt=0)
    p.grid(1, 'OSC 1')
    p.grid(2, 'NOISE 1', 'VCF 1')
    p.env(1, a=0.001, d=rr(1.5, 3), s=0, r=2, slope=0.6, oneshot=True)
    p.env(2, a=0.001, d=0.4, s=0, r=0.4, oneshot=True)
    p.mseg_p(1, [(0, 1, 0), (rr(0.4, 1.2), 0, 0.6)], loop='One Shot', time=1.0, amount=0.7)
    p.mod('MSEG 1', 'OSC 1 Tune', rr(0.15, 0.3))
    p.mod('Env 2', 'VCF 1 Cutoff', 0.35)
    p.fxgrid(1, 'DIST 1', 'COMP 1')
    p.dist(1, 'Tube', drive=rr(0.2, 0.4), tone=0.6, mix=0.6)
    p.comp(1, threshold=-20, ratio=6, attack=2, release=300)
    space(p, [1, 2], rev=0.45, model='Lush', decay=rr(4, 8), size=0.9)
    macros_std(p, 'Tone: noise brightness', [('VCF 1 Cutoff', 0.3)], 'Motion: pitch drop depth', [('MSEG 1 Amount', 0.3)],
               d_label='Grit: distortion', d_routes=[('DIST 1 Drive', 0.4)])
    return 'Cinematic impact: a pitch-dropping sub boom with a noise crack, saturated and compressed into a long tail. One shot per note.'


def fx_downer(p, v):
    p.osc(1, pick('Classic', 'Growl', 'Sync Sweep'), wave=0.6, uni=4, det=0.3, vol=0.6)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=2500, reso=0.3)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=0.01, d=4, s=0, r=2, oneshot=True)
    p.mseg_p(1, [(0, 1, 0), (pick(2, 4), 0, 0.3)], loop='One Shot', sync=True, amount=0.7)
    p.mod('MSEG 1', 'OSC 1 Tune', rr(0.25, 0.4))
    p.mod('MSEG 1', 'VCF 1 Cutoff', 0.3)
    space(p, [1], rev=0.5, dly=0.25, model='Lush', decay=6)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: fall depth', [('MSEG 1 Amount', 0.3)],
               d_label='Character: unison spread', d_routes=[('OSC 1 Detune', 0.3)])
    p.set('osc1_tune', -24)
    return 'Downer: a detuned stack that falls two octaves over a synced MSEG. For collapses, defeats and power-downs.'


def fx_scrape(p, v):
    p.exc(1, 'Bow', decay=rr(50, 300, 0), color=rr(0.3, 0.8))
    p.res(1, pick('Gong', 'Plate', 'Bell'), tune=pick(-12, 0, -5), decay=rr(3, 8), damping=0.3, spread=rr(0.4, 0.8), input=-6)
    p.grid(1, 'EXC 1', 'RES 1')
    p.env(1, a=rr(0.2, 1.0), d=2, s=1, r=rr(3, 6))
    p.lfo(1, 'Smooth Rnd', rr(0.3, 1.0))
    p.mod('LFO 1', 'EXC 1 Color', 0.4)
    p.mod('LFO 1', 'RES 1 Spread', 0.2)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Flanger', rate=0.1, depth=0.6, feedback=0.6, mix=0.4)
    space(p, [1], rev=0.5, model='Lush', decay=8, damping=0.6)
    macros_std(p, 'Tone: scrape brightness', [('EXC 1 Color', 0.4)], 'Motion: bow movement', [('LFO 1 Rate', 0.4)],
               d_label='Character: metal spread', d_routes=[('RES 1 Spread', 0.3)])
    return 'Bowed metal: a bow exciter scraping a gong or plate resonator, flanged and drenched. Tension, dread and derelict ships.'


def fx_heartbeat(p, v):
    p.osc(1, 'Classic', wave=0.0, tune=-24, vol=0.8)
    p.vcf(1, 'Ladder', 'LP 24', cutoff=200, reso=0.1, kt=0)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=0.01, d=1, s=1, r=1)
    bpm_div = pick(1.0, 0.8, 1.2)
    p.mseg_p(1, [(0, 0, 0), (0.03, 1, 0), (0.2, 0, 0.5), (0.3, 0.7, 0), (0.55, 0, 0.5), (bpm_div, 0, 0)], loop='Repeat', time=1.0)
    p.set('lane1_vol', -60)
    p.mod('MSEG 1', 'Lane 1 Volume', 0.9)
    p.mod('MSEG 1', 'VCF 1 Cutoff', 0.15)
    space(p, [1], rev=0.3, model='Classic', decay=2)
    macros_std(p, 'Tone: thump brightness', [('VCF 1 Cutoff', 0.25)], 'Motion: heart rate', [('MSEG 1 Time', -0.4)],
               c_routes=[('Aux 1 Return', 0.2)], d_label='Character: pitch', d_routes=[('OSC 1 Tune', 0.06)])
    return 'A low heartbeat that keeps pounding while you hold the key. Ctrl B speeds up the pulse as the tension rises.'


def fx_laser(p, v):
    p.fm(1, ratio=pick(1, 2, 0.5), depth=rr(0.4, 0.7), feedback=rr(0.2, 0.6), vol=0.6)
    p.grid(1, 'FM 1')
    p.env(1, a=0.001, d=rr(0.2, 0.6), s=0, r=0.2, oneshot=True)
    p.mseg_p(1, [(0, 1, 0), (rr(0.1, 0.4), 0, 0.5)], loop='One Shot', time=1.0)
    p.mod('MSEG 1', 'FM 1 Tune', rr(0.3, 0.45))
    p.mod('MSEG 1', 'FM 1 Depth', 0.2)
    p.set('fm1_tune', 12)
    space(p, [1], rev=0.2, dly=0.3, divl='1/16', divr='1/8', fb=0.4)
    macros_std(p, 'Tone: brightness', [('FM 1 Depth', 0.25)], 'Motion: zap length', [('MSEG 1 Time', 0.4)],
               d_label='Character: feedback grit', d_routes=[('FM 1 Feedback', 0.3)])
    return 'Arcade laser zap: an FM tone diving in pitch. Different keys give different blasters.'


def fx_alarm(p, v):
    p.osc(1, pick('Pulse Width', 'Classic'), wave=0.5, vol=0.55)
    p.vcf(1, 'SVF', 'BP 12', cutoff=1500, reso=0.3)
    p.grid(1, 'OSC 1', 'VCF 1')
    p.env(1, a=0.01, d=1, s=1, r=0.3)
    p.lfo(1, pick('Triangle', 'Square', 'Saw Up'), rr(0.8, 3), sync=pick(True, False), div=pick('1/4', '1/2', '1/8'))
    p.mod('LFO 1', 'OSC 1 Tune', rr(0.04, 0.08))
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.1)
    space(p, [1], rev=0.3, dly=0.15, model='Classic', decay=3)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: siren speed', [('LFO 1 Rate', 0.3)],
               d_label='Character: sweep depth', d_routes=[])
    p.mod('LFO 1', 'OSC 1 Tune', 0.04, via='Macro D')
    return 'Sci-fi alarm siren: a pitch-swept tone for base breaches, reactor warnings and boss incoming.'


# =========================================================================== TEXTURES
def tex_grains(p, v):
    p.osc(1, pick('Spline Shapes', 'Digital Bits', 'Ghost', 'Glass', 'Metallic'), wave=rr(0, 1), uni=2, det=0.3, vol=0.5)
    p.noise(1, pick('Crackle', 'Digital'), density=rr(0.3, 0.7), vol=0.2)
    p.vcf(1, 'SVF', 'BP 12', cutoff=rr(800, 3000, 0), reso=0.3)
    p.grid(1, 'OSC 1', 'NOISE 1', 'VCF 1')
    p.env(1, a=rr(0.5, 2), d=2, s=1, r=rr(3, 6))
    p.lfo(1, 'S&H', rr(2, 8))
    p.mod('LFO 1', 'OSC 1 Wave', 0.3)
    p.mod('LFO 1', 'VCF 1 Cutoff', 0.1)
    p.fxgrid(2, 'TEXTURE 1', 'REVERB 1')
    p.texture(1, grain=rr(40, 150, 0), density=rr(0.6, 0.9), pitch=pick(0, 12, -12, 7, 19), spread=rr(0.2, 0.6), reverse=0.5,
              feedback=rr(0.4, 0.7), jitter=0.8, mix=1.0)
    p.reverb(1, model='Lush', decay=6, mix=0.5)
    p.lane(1, send1=0.8)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: grain density', [('TEXTURE 1 Density', 0.3)],
               c_label='Space: grain pitch scatter', c_routes=[('TEXTURE 1 Spread', 0.3)], d_label='Character: crackle', d_routes=[('NOISE 1 Volume', 0.3)])
    return 'Granular texture: stepped tones and crackle shredded into short, scattered grains. Layer under pads or use alone for glitchy unease.'


def tex_vinyl(p, v):
    p.osc(1, pick('Dark Pad', 'Organ Bars', 'Choir', 'Reed'), wave=rr(0.2, 0.8), uni=3, det=0.2, vol=0.5)
    p.vcf(1, 'Smooth', 'LP 12', cutoff=rr(1200, 2500, 0), reso=0.1)
    p.shape(1, 'Bitcrush', drive=rr(0.3, 0.6), mix=0.4)
    p.grid(1, 'OSC 1', 'VCF 1', 'SHAPE 1')
    p.noise(1, 'Crackle', density=0.4, vol=0.25)
    p.grid(2, 'NOISE 1')
    p.lane(2, vol=-6)
    p.env(1, a=rr(0.3, 1.5), d=1, s=1, r=rr(1.5, 3))
    p.lfo(1, 'Sine', 0.5)
    p.mod('LFO 1', 'OSC 1 Tune', 0.003)
    p.fxgrid(1, 'MODFX 1')
    p.modfx(1, 'Chorus', rate=0.2, depth=0.4, mix=0.3)
    space(p, [1, 2], rev=0.3)
    macros_std(p, 'Tone: filter', [('VCF 1 Cutoff', 0.3)], 'Motion: tape wow', [],
               d_label='Character: crackle', d_routes=[('Lane 2 Volume', 0.15)])
    p.mod('LFO 1', 'OSC 1 Tune', 0.008, via='Macro B')
    return 'Worn, wobbly memory: a lo-fi bitcrushed tone with vinyl crackle in its own lane. Haunted ballrooms and found footage.'


def tex_frozen(p, v):
    p.osc(1, pick('Glass', 'Sine Stack', 'Bell Partials'), wave=rr(0.2, 0.8), vol=0.5)
    p.exc(1, 'Pattern', decay=15, rate=rr(3, 9), color=0.6, level=0.5)
    p.res(1, 'Glass', tune=24, decay=1.5, damping=0.5, mix=0.6)
    p.grid(1, 'OSC 1')
    p.grid(2, 'EXC 1', 'RES 1')
    p.lane(2, vol=-10)
    p.env(1, a=1, d=1, s=1, r=4)
    p.fxgrid(2, 'TEXTURE 1', 'REVERB 1')
    p.texture(1, grain=rr(250, 600, 0), density=0.7, pitch=12, spread=0.05, reverse=0.4, feedback=0.75, jitter=0.5, damping=0.2, mix=1.0)
    p.reverb(1, model='Lush', decay=12, size=1.0, mix=0.6)
    p.lane(1, send1=0.7)
    p.lane(2, vol=-10, send1=0.7)
    macros_std(p, 'Tone: crystal brightness', [('RES 1 Bright', 0.4)], 'Motion: sparkle rate', [('EXC 1 Rate', 0.3)],
               c_label='Space: frost feedback', c_routes=[('TEXTURE 1 Feedback', 0.2)], d_label='Character: grain size', d_routes=[('TEXTURE 1 Grain', 0.3)])
    return 'Frozen texture: glassy tones and tiny ice crystals rising an octave through feedback grains. Snowfields, ice caves and silence.'


# =========================================================================== category plan
PLAN = [
    # (category, count, [(archetype, weight, tags, theme choices)])
    ('Pads', 60, [
        (pad_analog, 3, ['Ambient', 'Cinematic', 'Prog', 'Rock'], ['fantasy', 'scifi']),
        (pad_glass, 2, ['Ambient', 'Cinematic', 'Game'], ['fantasy', 'paranormal']),
        (pad_formant, 2, ['Ambient', 'Cinematic', 'Prog'], ['paranormal', 'fantasy']),
        (pad_pwm, 2, ['Prog', 'Rock', 'Ambient'], ['scifi', 'fantasy']),
        (pad_horror, 2, ['Cinematic', 'Game', 'Ambient'], ['horror', 'paranormal']),
        (pad_scifi, 2, ['Cinematic', 'Game', 'Prog'], ['scifi']),
        (pad_dark, 2, ['Cinematic', 'Ambient', 'Metal'], ['horror', 'fantasy']),
        (pad_choir, 1, ['Cinematic', 'Ambient', 'Game'], ['fantasy', 'paranormal']),
        (pad_brass_swell, 1, ['Prog', 'Rock', 'Cinematic'], ['fantasy']),
        (pad_organ, 1, ['Prog', 'Rock'], ['fantasy', 'horror']),
    ]),
    ('Atmospheres', 45, [
        (atm_wind, 2, ['Ambient', 'Cinematic', 'Game'], ['fantasy', 'paranormal', 'horror']),
        (atm_resonant, 2, ['Ambient', 'Cinematic', 'Game'], ['fantasy', 'scifi']),
        (atm_cloud, 2, ['Ambient', 'Cinematic'], ['paranormal', 'scifi', 'fantasy']),
        (atm_haunt, 2, ['Cinematic', 'Game', 'Ambient'], ['paranormal', 'horror']),
        (atm_space, 2, ['Ambient', 'Cinematic', 'Game'], ['scifi']),
    ]),
    ('Leads', 55, [
        (lead_rock, 3, ['Rock', 'Prog', 'Metal'], ['fantasy', 'scifi']),
        (lead_metal, 3, ['Metal', 'Rock'], ['horror', 'fantasy']),
        (lead_prog, 3, ['Prog', 'Rock'], ['fantasy', 'scifi']),
        (lead_sync, 2, ['Rock', 'Game', 'Prog'], ['scifi']),
        (lead_fm, 1, ['Prog', 'Game'], ['scifi', 'fantasy']),
        (lead_chip, 2, ['Game'], ['scifi', 'fantasy']),
        (lead_whistle, 1, ['Cinematic', 'Game', 'Ambient'], ['paranormal', 'scifi']),
        (lead_formant, 1, ['Rock', 'Prog'], ['paranormal', 'scifi']),
    ]),
    ('Bass', 50, [
        (bass_sub, 1, ['Cinematic', 'Ambient', 'Game'], ['fantasy', 'scifi']),
        (bass_growl, 3, ['Metal', 'Cinematic', 'Game'], ['horror', 'fantasy']),
        (bass_fm, 2, ['Prog', 'Game', 'Rock'], ['scifi']),
        (bass_analog, 3, ['Prog', 'Rock'], ['fantasy', 'scifi']),
        (bass_rock, 2, ['Rock', 'Metal'], ['fantasy', 'horror']),
        (bass_chip, 1, ['Game'], ['scifi']),
        (bass_detuned, 2, ['Cinematic', 'Game', 'Metal'], ['horror', 'scifi']),
    ]),
    ('Keys & Bells', 45, [
        (keys_epiano, 2, ['Prog', 'Rock', 'Ambient'], ['fantasy', 'scifi']),
        (keys_bell, 3, ['Cinematic', 'Ambient', 'Game'], ['fantasy', 'paranormal']),
        (keys_musicbox, 2, ['Cinematic', 'Game'], ['horror', 'paranormal']),
        (keys_clav, 1, ['Prog', 'Rock'], ['fantasy']),
        (keys_organ, 2, ['Prog', 'Rock', 'Metal'], ['fantasy', 'horror']),
        (keys_mallet, 2, ['Ambient', 'Game', 'Cinematic'], ['fantasy']),
    ]),
    ('Plucks', 40, [
        (pluck_string, 3, ['Cinematic', 'Game', 'Ambient'], ['fantasy']),
        (pluck_synth, 2, ['Prog', 'Rock', 'Game'], ['scifi', 'fantasy']),
        (pluck_fm, 2, ['Game', 'Ambient', 'Prog'], ['scifi']),
        (pluck_fantasy, 2, ['Cinematic', 'Game', 'Ambient'], ['fantasy']),
        (pluck_dark, 2, ['Cinematic', 'Game', 'Metal'], ['horror', 'paranormal']),
    ]),
    ('Arps & Sequences', 45, [
        (arp_synth, 4, ['Prog', 'Game', 'Rock'], ['scifi', 'fantasy']),
        (arp_chip, 2, ['Game'], ['scifi']),
        (seq_mapper, 2, ['Prog', 'Cinematic', 'Metal'], ['scifi', 'horror']),
        (seq_bass, 2, ['Prog', 'Rock', 'Game'], ['scifi', 'fantasy']),
        (seq_bells, 2, ['Ambient', 'Game', 'Cinematic'], ['fantasy', 'paranormal']),
    ]),
    ('Brass & Strings', 40, [
        (brass_synth, 3, ['Prog', 'Rock', 'Cinematic'], ['fantasy']),
        (brass_stab, 2, ['Prog', 'Game', 'Cinematic'], ['fantasy', 'scifi']),
        (strings_ensemble, 3, ['Prog', 'Rock', 'Ambient'], ['fantasy']),
        (strings_bowed, 2, ['Cinematic', 'Ambient'], ['fantasy', 'horror']),
        (brass_horn, 2, ['Game', 'Cinematic'], ['fantasy']),
    ]),
    ('Choirs & Voices', 30, [
        (choir_formant, 3, ['Cinematic', 'Game', 'Ambient'], ['fantasy', 'paranormal']),
        (choir_monk, 2, ['Cinematic', 'Game', 'Metal'], ['fantasy', 'horror']),
        (voice_ghost, 2, ['Cinematic', 'Game', 'Ambient'], ['paranormal', 'horror']),
        (voice_siren, 2, ['Cinematic', 'Ambient', 'Prog'], ['paranormal', 'fantasy']),
    ]),
    ('Drones', 35, [
        (drone_dark, 3, ['Cinematic', 'Ambient', 'Metal'], ['horror', 'fantasy']),
        (drone_scifi, 2, ['Cinematic', 'Game', 'Ambient'], ['scifi']),
        (drone_ritual, 2, ['Cinematic', 'Ambient', 'Metal'], ['fantasy', 'paranormal']),
        (drone_horror, 2, ['Cinematic', 'Game'], ['horror', 'paranormal']),
    ]),
    ('Cinematic FX', 35, [
        (fx_braam, 2, ['Cinematic', 'Metal'], ['horror', 'scifi']),
        (fx_riser, 2, ['Cinematic', 'Game'], ['scifi', 'paranormal']),
        (fx_impact, 2, ['Cinematic', 'Game', 'Metal'], ['horror', 'fantasy']),
        (fx_downer, 1, ['Cinematic', 'Game'], ['scifi', 'horror']),
        (fx_scrape, 2, ['Cinematic', 'Ambient'], ['horror', 'paranormal']),
        (fx_heartbeat, 1, ['Cinematic', 'Game'], ['horror']),
        (fx_laser, 1, ['Game'], ['scifi']),
        (fx_alarm, 1, ['Game', 'Cinematic'], ['scifi']),
    ]),
    ('Textures', 20, [
        (tex_grains, 2, ['Ambient', 'Cinematic', 'Game'], ['scifi', 'paranormal']),
        (tex_vinyl, 1, ['Ambient', 'Cinematic'], ['paranormal', 'horror']),
        (tex_frozen, 1, ['Ambient', 'Cinematic', 'Game'], ['fantasy', 'paranormal']),
    ]),
]

# =========================================================================== name / sound agreement
# Names are drawn at random per category, so an instrument word in a name can contradict the sound
# ("Kingsbane Glockenspiel" was a marimba). This pass reads each preset's description and swaps any
# instrument word that doesn't fit for one that does. It runs after build() and uses its own RNG,
# so the sounds (and tools/levels.json) are untouched.
NEEDS = {   # instrument word -> description keywords that justify it (any)
    'Glockenspiel': ['bell', 'music box', 'plate'], 'Celesta': ['music box', 'bell'],
    'Gamelan': ['bell', 'plate', 'vibes', 'gong', 'metallic'],
    'Carillon': ['bell'], 'Bells': ['bell', 'music box', 'plate', 'vibes'], 'Belfry': ['bell'], 'Clocktower': ['bell', 'music box'],
    'Chime': ['bell', 'music box', 'glass', 'vibes', 'plate', 'chime'],
    'Music Box': ['music box'], 'Harmonium': ['organ'], 'Tines': ['tine', 'electric piano', 'music box', 'kalimba'],
    'Keys': ['piano', 'organ', 'clavinet'], 'Hammers': ['piano', 'clavinet', 'mallet', 'marimba'], 'Ivory': ['piano', 'organ', 'clavinet'],
    'Harp': ['harp', 'plucked string'], 'Lyre': ['harp', 'lute', 'zither', 'plucked string'], 'Lute': ['lute', 'zither', 'plucked string'],
    'Zither': ['zither', 'lute'], 'Strings': ['string'], 'Bows': ['bow', 'string'],
    'Horns': ['brass', 'horn'], 'Fanfare': ['brass', 'horn', 'fanfare'],
    'Monks': ['monk'], 'Plainsong': ['monk', 'chant'], 'Chant': ['monk', 'chant', 'choir'],
    'Choir': ['choir', 'monk'], 'Congregation': ['choir', 'monk'], 'Chorus': ['choir', 'monk', 'voices'], 'Vespers': ['choir', 'monk'],
    'Hymn': ['choir', 'monk'], 'Psalm': ['choir', 'monk'], 'Canticle': ['choir', 'monk'], 'Requiem': ['choir', 'monk', 'ghost'],
    'Litany': ['choir', 'monk', 'chant'], 'Sirens': ['siren'], 'Whispers': ['ghost', 'whisper', 'breath'],
}
PREFER = [  # first matching keyword in the description -> words that describe the sound
    ('marimba', ['Marimba', 'Mallets', 'Hammers']), ('vibes', ['Vibes', 'Chime']), ('music box', ['Music Box', 'Celesta', 'Tines']),
    ('electric piano', ['Tines', 'Keys', 'Ivory']), ('clavinet', ['Keys', 'Hammers']), ('organ', ['Harmonium', 'Keys', 'Organ']),
    ('plate', ['Gamelan', 'Bells', 'Chime']), ('glass', ['Chime', 'Glass', 'Prism']), ('bell', ['Bells', 'Carillon', 'Belfry', 'Chime']),
    ('zither', ['Zither', 'Lute', 'Lyre']), ('plucked string', ['Harp', 'Lyre', 'Lute', 'Strings']), ('kalimba', ['Kalimba', 'Tines', 'Droplet']),
    ('monk', ['Monks', 'Chant', 'Plainsong']), ('siren', ['Sirens', 'Lament']), ('ghost', ['Whispers', 'Voices', 'Lament']),
    ('choir', ['Choir', 'Chorus', 'Hymn', 'Vespers']), ('bow', ['Bows', 'Strings']), ('string ensemble', ['Strings', 'Ensemble', 'Section']),
    ('horn', ['Horns', 'Fanfare']), ('brass', ['Horns', 'Fanfare', 'Brass']),
]


def word_ok(word, desc):
    need = NEEDS.get(word)
    return need is None or any(k in desc for k in need)


def fix_names(presets):
    rng = random.Random(4077)
    taken = {p['name'] for p in presets}
    renamed = {}
    for p in presets:
        name, desc = p['name'], p['desc'].lower()
        nouns = NOUN[p['cat']] + [w for w in NEEDS if w not in NOUN[p['cat']]]
        # the category noun sits at the start ("Harp of the Fen") or at the end ("Elder Harp")
        hit = None
        for w in sorted(nouns, key=len, reverse=True):
            if name.endswith(' ' + w):
                hit = ('end', w); break
            if name.startswith(w + ' of '):
                hit = ('start', w); break
        if hit is None or word_ok(hit[1], desc):
            continue
        cands = []
        for key, words in PREFER:
            if key in desc:
                cands = [w for w in words if word_ok(w, desc)]
                break
        if not cands:
            cands = [w for w in NOUN[p['cat']] if w not in NEEDS]
        rng.shuffle(cands)
        for w in cands:
            new = name[: -len(hit[1])] + w if hit[0] == 'end' else w + name[len(hit[1]):]
            if new not in taken and len(new) <= 28:
                taken.discard(name); taken.add(new)
                renamed[name] = new
                p['was'] = name
                p['name'] = new
                break
    return renamed


SPECIAL = {'Pads': 'Wyrmglass Cathedral'}   # a named showcase preset used for screenshots


def build():
    out = []
    first = True
    for cat, count, arch in PLAN:
        weights = [a[1] for a in arch]
        # spread evenly: cycle through archetypes proportional to weight
        seq = []
        for a in arch:
            seq += [a] * a[1]
        k = 0
        for n in range(count):
            fn, _, tags, themes = seq[k % len(seq)]
            k += 1
            theme = R.choice(themes)
            if cat in SPECIAL and n == 0:
                name = SPECIAL[cat]
                USED.add(name)
            else:
                name = make_name(cat, theme)
            # 2-3 genre tags from the archetype's list
            t = tags[:]
            R.shuffle(t)
            ntags = min(len(t), R.choice([2, 2, 3]))
            p = Patch(name, cat, sorted(t[:ntags], key=TAG_ALL.index))
            base(p)
            desc = fn(p, n)
            p.desc = desc
            out.append(p.to_json())
    return out


if __name__ == '__main__':
    presets = build()
    assert len(presets) == 500, len(presets)
    renamed = fix_names(presets)
    for old, new in renamed.items():
        print(f'  renamed  {old:28s} -> {new}')
    print(len(renamed), 'names changed to match their sound')
    names = [p['name'] for p in presets]
    assert len(set(names)) == len(names)
    # apply the stored loudness normalisation (tools/levels.json, written by normalize_presets.py; keyed by the original names)
    lvpath = os.path.join(ROOT, 'tools', 'levels.json')
    if os.path.exists(lvpath) and '--raw' not in sys.argv:
        levels = json.load(open(lvpath))
        for pr in presets:
            pr['p'].update(levels.get(pr['name'], levels.get(pr.get('was', ''), {})))
    data = json.dumps(presets, separators=(',', ':')).encode()
    with gzip.open(os.path.join(ROOT, 'Resources', 'factory_presets.json.gz'), 'wb', compresslevel=9) as f:
        f.write(data)
    meta = [{'name': p['name'], 'cat': p['cat'], 'tags': p['tags'], 'desc': p['desc'], 'macros': p.get('macros', {})} for p in presets]
    os.makedirs(os.path.join(ROOT, 'docs'), exist_ok=True)
    json.dump(meta, open(os.path.join(ROOT, 'docs', 'presets.json'), 'w'), indent=1)
    from collections import Counter
    print(len(presets), 'presets,', len(data) // 1024, 'KB json')
    print(Counter(p['cat'] for p in presets))
    print(Counter(t for p in presets for t in p['tags']))
