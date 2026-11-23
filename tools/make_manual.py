#!/usr/bin/env python3
"""Builds docs/Manticore_User_Manual.pdf (style shared with the other Fellowship manuals: see manual_head.py)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'manual_head.py')).read())

S['small'] = ParagraphStyle('small', fontName='Raj', fontSize=9, leading=11, textColor=INK)
S['smallb'] = ParagraphStyle('smallb', fontName='RajBold', fontSize=9, leading=11, textColor=INK)
S['smallh'] = ParagraphStyle('smallh', fontName='RajBold', fontSize=9.5, leading=11, textColor=colors.white)


def page(c, doc):
    W, H = letter
    c.setFillColor(BG)
    c.rect(0, H - 0.55 * inch, W, 0.55 * inch, fill=1, stroke=0)
    c.setFillColor(colors.HexColor('#dce8e5'))
    c.setFont('Cinzel', 11)
    c.drawString(0.75 * inch, H - 0.36 * inch, 'MANTICORE')
    c.setFillColor(AMBER_BRIGHT)
    c.setFont('RajSemi', 10)
    c.drawRightString(W - 0.75 * inch, H - 0.355 * inch, 'USER MANUAL')
    c.setStrokeColor(TEAL_BRIGHT)
    c.setLineWidth(1.5)
    c.line(0, H - 0.55 * inch, W, H - 0.55 * inch)
    c.setFillColor(DIM)
    c.setFont('Raj', 10)
    c.drawCentredString(W / 2, 0.45 * inch, str(doc.page))


def small_table(rows, widths):
    data = [[Paragraph(c, S['smallh']) for c in rows[0]]]
    for r in rows[1:]:
        data.append([Paragraph(r[0], S['smallb'])] + [Paragraph(c, S['small']) for c in r[1:]])
    t = Table(data, colWidths=widths, repeatRows=1)
    st = [('VALIGN', (0, 0), (-1, -1), 'TOP'), ('LEFTPADDING', (0, 0), (-1, -1), 4), ('RIGHTPADDING', (0, 0), (-1, -1), 4),
          ('TOPPADDING', (0, 0), (-1, -1), 2), ('BOTTOMPADDING', (0, 0), (-1, -1), 3), ('BACKGROUND', (0, 0), (-1, 0), INK),
          ('LINEBELOW', (0, 0), (-1, -1), 0.3, colors.HexColor('#d5e2e1'))]
    for i in range(2, len(data), 2):
        st.append(('BACKGROUND', (0, i), (-1, i), colors.HexColor('#f5f9f9')))
    t.setStyle(TableStyle(st))
    return [t, Spacer(1, 6)]


def bullets(items):
    return [P('&bull;&nbsp;&nbsp;' + t, 'step') for t in items]


doc = BaseDocTemplate(OUT, pagesize=letter, title='Manticore User Manual', author='Fellowship of the String',
                      leftMargin=0.75 * inch, rightMargin=0.75 * inch, topMargin=0.85 * inch, bottomMargin=0.75 * inch)
frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id='f')
doc.addPageTemplates([PageTemplate('cover', frames=[frame], onPage=cover), PageTemplate('page', frames=[frame], onPage=page)])

s = [NextPageTemplate('page'), PageBreak()]

# ------------------------------------------------------------------ contents
s.append(P('Contents', 'h1'))
toc = ['1. Welcome', '2. Installation', '3. The Window at a Glance', '4. The Generator Grid', '5. Generator Modules',
       '6. Modulation', '7. Effects', '8. The Arpeggiator', '9. The XY Vector Mixer', '10. Mutate, Breed and Haunt',
       '11. Voice, Performance and MIDI', '12. Presets, the Browser and Favourites', '13. Sound Design Recipes',
       '14. Factory Preset List', '15. Specifications and Troubleshooting', 'Appendix: Building from GitHub']
for t in toc:
    s.append(P(t, 'step'))
s.append(Spacer(1, 12))
s += tipbox('HOVER FOR HELP', 'Every knob, switch, slot and tab in Manticore has a tooltip. Rest the mouse on anything for a moment '
            'and it explains itself, so this manual is the reference and the tooltips are the reminder.')

# ------------------------------------------------------------------ 1 Welcome
s.append(PageBreak())
s.append(P('1. Welcome', 'h1'))
s.append(P('Manticore is a modular synthesizer built around a grid. The manticore of legend is a beast assembled from many '
           'creatures, and this one is assembled from yours: four lanes, each a column of four slots, and a pool of modules '
           '(wavetable and additive oscillators, FM oscillators, noise, physical-modelling exciters and resonators, comb filters, '
           'thirteen filter models, shapers, ring modulators and EQs) that you drop into the slots in any order.', 'lead'))
s.append(P('Sound flows down each lane from top to bottom. Lanes can feed each other, so a lane can hold a stack of oscillators '
           'and the next lane can filter, distort and resonate the result. Around the grid sit a modulation rack (four envelopes, '
           'four LFOs, four drawable MSEGs, four mappers and four mod-math units, wired through a 24-slot matrix), an effects '
           'grid with a main bus and three aux send buses, and a step arpeggiator.'))
s.append(P('The layout follows the logic of the modular grid synths it was inspired by: generators on the left, the selected '
           'module\'s editor beside them, the modulation rack below, effects on their own page. It wears the Fellowship of the '
           'String colours: dark slate, teal and amber, with Cinzel and Rajdhani type.'))
s += tipbox('QUICK START', 'Open the BROWSE page, pick a category on the left and click any preset to hear it. Turn the four '
            'Ctrl A-D macros on the SYNTH page: in every factory preset A shapes the tone, B adds motion, C opens up the space '
            'and D adds grit or character. Click the star beside a preset to keep it in your Favourites. Then press MUTATE in the '
            'toolbar for a fresh variation, or HAUNT to let the sound drift on its own.')
s += tipbox('NEW IN 1.1', 'Mutate (whole patch or one module), Breed (including Breed with a Favourite), Haunt with Follow and '
            'Capture, padlocks on every section including the grid layout, 48-step Undo, the XY Vector Mixer page, and clearer names '
            'for 40 factory presets whose instrument word did not match their sound. Presets you had starred keep their stars.',
            PALE_AMBER, AMBER)
s += img('main.png', caption='The SYNTH page with the Wyrmglass Cathedral preset: OSC 1 selected in the grid, ENV 1 in the modulation rack.')

# ------------------------------------------------------------------ 2 Installation
s.append(PageBreak())
s.append(P('2. Installation', 'h1'))
s.append(P('Run <b>Manticore-1.1.0-Setup.exe</b>. The installer has three parts. The VST3 plug-in always goes to '
           '<b>C:\\Program Files\\Common Files\\VST3</b>, the standard folder REAPER scans. The standalone app is optional and '
           'can put a Manticore shortcut on your desktop. This manual is copied alongside it.'))
s.append(P('Then, in REAPER:'))
for t in ['<b>1.</b>&nbsp;&nbsp;Open <b>Options &gt; Preferences &gt; Plug-ins &gt; VST</b>.',
          '<b>2.</b>&nbsp;&nbsp;Click <b>Re-scan</b> (or <b>Clear cache/re-scan</b> if Manticore does not appear).',
          '<b>3.</b>&nbsp;&nbsp;Insert it as a virtual instrument on a track: <b>VST3i: Manticore (Fellowship of the String)</b>.',
          '<b>4.</b>&nbsp;&nbsp;Arm the track, enable input monitoring and pick your MIDI keyboard as the track input.']:
    s.append(P(t, 'step'))
s.append(P('Your own presets are saved to <b>Documents\\Manticore\\Presets</b> as .mtpreset files, and your favourites list lives in '
           '<b>Documents\\Manticore\\Favourites.txt</b>. Both survive reinstalling. The 500 factory presets are built into the plug-in '
           'and also appear in REAPER\'s preset menu at the top of the FX window.'))
s.append(P('The standalone app is handy for playing without a DAW. Choose your audio and MIDI devices in its <b>Options</b> menu '
           'at the top left. Without a host the arpeggiator and tempo-synced LFOs run at 120 BPM.'))

# ------------------------------------------------------------------ 3 Window
s.append(P('3. The Window at a Glance', 'h1'))
s.append(P('Drag the corner at the bottom right to resize the window from 60% to 200% of its normal 1400 x 944 size. It keeps its '
           'proportions and remembers the size, the page you were on and the module you were editing.'))
s += img(crop('main.png', (0, 0, 1400, 112), 'crop_header.png'), caption='The header and the toolbar')
s.append(P('<b>The header</b> holds the page tabs (SYNTH, EFFECTS, ARP, XY, BROWSE) and the preset bar (arrows step through the '
           'browser\'s current list, the name opens the browser, the star marks a favourite, SAVE stores your own, INIT starts from '
           'scratch). Under the buttons, a status line shows how many voices are sounding and confirms actions such as saving.'))
s.append(P('<b>The toolbar</b> below it holds the sound tools described in section 10: MUTATE with its Strength, BREED and Breed '
           'with a Favourite (the button with the star), and HAUNT with its Depth, Speed, FOLLOW and CAPTURE. At the right are the '
           'A/B comparison buttons with copy, UNDO and REDO.'))
s.append(P('<b>The SYNTH page</b> has the generator grid, lane mixer, macros and voice settings down the left; the editor for the '
           'selected module top right; and the modulation rack bottom right. <b>EFFECTS</b> shows the effects grid, aux returns, '
           'lane sends, the selected effect\'s editor and a live signal-flow map. <b>ARP</b> is the step arpeggiator and '
           '<b>XY</b> the vector mixer and <b>BROWSE</b> the preset browser.'))
s.append(P('Every knob works the same way: drag up and down to turn it, double-click it to reset, double-click the value under it '
           'to type a number, and right-click it to modulate it (see section 6). An amber arc just outside a knob shows the range '
           'that modulation sweeps it through.'))

# ------------------------------------------------------------------ 4 Grid
s.append(PageBreak())
s.append(P('4. The Generator Grid', 'h1'))
s += img(crop('braam.png', (0, 112, 460, 646), 'crop_grid.png'), width=3.6 * inch,
         caption='Lane 1 stacks three oscillators and a shaper; lane 2 takes lane 1 as its input and filters it.')
s.append(P('The grid has four lanes, each with four slots. Click an empty slot (or right-click any slot) to choose a module. '
           'Click a filled slot to open that module in the editor. Signal flows down each lane: generators (OSC, FM, NOISE, EXC) '
           'add their sound to whatever is already flowing, and processors (VCF, SHAPE, COMB, RES, RING, EQ) work on it.'))
s.append(P('Modules come from a pool, as on classic grid synths, and each can sit in only one slot at a time. The pool holds '
           '4 OSC, 4 FM, 2 NOISE, 2 EXC (exciters), 2 RES (resonators), 2 COMB, 4 VCF, 4 SHAPE, 2 RING and 2 EQ. If you choose '
           'a module that is already in use, the menu shows where it is and moves it to the new slot. A module that shows red '
           'with "Duplicate" is inactive. The module editor says "NOT IN THE GRID" when you are editing a module that is not placed.'))
s.append(P('<b>Lane inputs.</b> The box above each lane can feed the output of an earlier lane (one to its left) into the top '
           'of that lane. Use it to run a whole oscillator stack through one filter, put a resonator after a mix of sources, or '
           'process lane 1 twice in different ways. If a lane only exists to feed another, turn its volume all the way down.'))
s.append(P('<b>The amp envelope.</b> ENV 1 shapes the volume of every lane at its output, so it is the amp envelope for the whole '
           'voice. A voice ends when ENV 1 finishes its release.'))
s.append(P('<b>The lane mixer</b> under the grid sets each lane\'s volume and pan, and how much it sends to the three aux effect '
           'buses. Sends are taken after the lane volume.'))
s += img(crop('main.png', (0, 450, 460, 944), 'crop_lanes.png'), width=3.6 * inch, caption='Lane mixer, macros and voice settings.')

# ------------------------------------------------------------------ 5 Modules
s.append(PageBreak())
s.append(P('5. Generator Modules', 'h1'))
s.append(P('Every module\'s editor has a live display on the left: the oscillator\'s waveform with its morph frames stacked '
           'behind it, a filter\'s response curve, a resonator\'s partials, a shaper\'s transfer curve and so on.'))
s.append(P('OSC: wavetable and additive oscillator', 'h2'))
s.append(P('Each oscillator plays one of 24 built-in tables. Every table has eight frames and <b>Wave</b> morphs smoothly '
           'between them; sweep it with an envelope, LFO or MSEG for evolving tones. The tables are band-limited, so high '
           'notes stay clean.'))
rows = [['Tables', 'What they are'],
        ['Classic', 'Sine to triangle to saw to square: the analogue basics in one table.'],
        ['Pulse Width', 'Square narrowing to a thin pulse. Sweep it with an LFO for PWM.'],
        ['Formant Vox, Choir', 'Vowels (A-E-I-O-U) and soft choir formants.'],
        ['Hollow Odd, Reed', 'Odd-harmonic, clarinet-like tones, Reed adding even harmonics as it morphs.'],
        ['Organ Bars, Sine Stack', 'Drawbar registrations, and harmonics added one by one.'],
        ['Bell Partials, Glass, Metallic', 'Bright, bell-like spectra; even better in the Additive engine with Stretch.'],
        ['Sync Sweep, Fold Sweep, PD Resonant, Phase Warp, Triangle Fold', 'Classic digital and analogue techniques captured as tables.'],
        ['Brass Swell, Bowed String, Clav Pluck', 'Instrument-like spectra that open as they morph.'],
        ['Growl, Digital Bits', 'Snarling bass harmonics and stepped, lo-fi digital tones.'],
        ['Dark Pad, Spline Shapes, Ghost', 'Soft pad spectra, random spline shapes and an eerie random spectrum.']]
s += table(rows, [2.3 * inch, 4.7 * inch])
s.append(P('<b>Engine.</b> Wavetable reads the table directly and supports <b>Unison</b> (up to 16 detuned copies with '
           'Detune and Width). Additive rebuilds the frame\'s spectrum from up to 64 sine partials (<b>Partials</b>), which '
           'lets you tilt it (<b>Bright</b>), keep only odd or even partials (<b>Odd/Even</b>), pull the partials off the '
           'harmonic series for bells and gongs (<b>Stretch</b>) and make the upper partials die first (<b>P.Decay</b>). '
           'In Additive mode the Width knob spreads odd and even partials across the stereo field.'))
s.append(P('<b>Wave FX.</b> Two FX slots reshape the waveform. Sync, Phase Dist, Bend +/-, Mirror, Pulse, Formant and Quantize '
           'warp the cycle as it is read (wavetable engine only); Fold, Squash, Crush and Ring distort the result (both engines). '
           '<b>Free Run</b> keeps the oscillator running between notes; otherwise it restarts at <b>Phase</b> for a consistent '
           'attack. <b>Keytrack</b> at 0% gives a fixed pitch for drones and effects.'))
s.append(P('FM: two-operator FM oscillator', 'h2'))
s.append(P('A sine modulator at <b>Ratio</b> times the carrier frequency bends the carrier\'s phase by <b>Depth</b>. Whole-number '
           'ratios are harmonic; in-between values clang. <b>Feedback</b> lets the modulator modulate itself, from sine to saw to noise. '
           '<b>Carrier</b> offers sine and three classic alternative shapes. <b>Input FM</b> uses whatever is already in the lane as a '
           'third operator, so an OSC above an FM module can modulate it.'))
s.append(P('NOISE', 'h2'))
s.append(P('White, Pink, Brown and Blue noise, Crackle (vinyl-like dust, with Density setting how often it pops) and Digital '
           '(stepped sample-and-hold noise, with Density as the step rate). Color tilts the spectrum darker or brighter.'))
s.append(P('EXC and RES: physical modelling', 'h2'))
s.append(P('An <b>exciter</b> strikes; a <b>resonator</b> rings. Put an EXC in a slot and a RES (or COMB) under it. Exciter modes '
           'are Impulse (a click), Burst (a noise burst), Mallet (a soft, rounded strike), Bow (continuous noise while the key is '
           'held) and Pattern (repeated strikes at Rate). Decay sets the strike length and Color its hardness. Try velocity on Color '
           'for a harder mallet when you play harder.'))
s.append(P('The resonator models an object with twelve modes. <b>Profile</b> chooses the object (Marimba, Vibes, Bell, Glass, Gong, '
           'Plate, String, Tube), <b>Decay</b> sets the ring time and <b>Damping</b> makes the upper modes die faster (wood versus '
           'metal). <b>Bright</b> tilts the upper modes, <b>Spread</b> detunes them and the stereo channels apart. Resonators also '
           'process oscillators and noise: lower <b>Input</b> when feeding them sustained sounds.'))
s.append(P('COMB', 'h2'))
s.append(P('A tuned delay that rings at the played pitch. Fed by a short Burst exciter it is a plucked string (Karplus-Strong): raise '
           'Feedback for longer sustain and lower Damping for a brighter string. Negative feedback rings an octave lower with a '
           'hollow, square-wave character. Diffuse mode spreads it across four delay lines into a small metallic room.'))
s.append(P('VCF: thirteen filter models', 'h2'))
rows = [['Model', 'Character'],
        ['Ladder', 'Fat, creamy four-pole ladder. Self-oscillates at high Reso.'],
        ['Cascade', 'Four clean one-pole stages with a gentler resonance.'],
        ['SVF', 'Clean state-variable filter; resonant on both stages in 24 dB modes.'],
        ['Diode', 'Squelchy, slightly gritty diode ladder.'],
        ['Sallen-Key', 'Aggressive, screaming resonance.'],
        ['OTA', 'Smooth, warm two-stage filter.'],
        ['Steiner', 'Saturating integrators: dirty and lively.'],
        ['Bite', 'Heavily driven and loud, for metal leads.'],
        ['Smooth', 'Very clean, for EQ-like shaping.'],
        ['Formant', 'Three formant peaks: Cutoff moves through the vowels.'],
        ['Phaser', 'Eight all-pass stages with feedback (Reso). Peak response inverts the notches.'],
        ['Vintage LPG', 'A low-pass gate: closing the cutoff also lowers the level, like a plucked West Coast tone.'],
        ['Twin Peak', 'Two resonant peaks, the second 2.5 times higher: vocal, wah-like sweeps.']]
s += table(rows, [1.6 * inch, 5.4 * inch])
s.append(P('<b>Response</b> chooses LP 24/18/12/6, BP 12/6, HP 24/12/6, Notch, Peak or All Pass (some models approximate some '
           'responses). <b>Drive</b> pushes the filter\'s saturation, <b>Keytrack</b> makes the cutoff follow the keyboard (100% = '
           'one octave per octave), <b>Spread</b> offsets the left and right cutoffs for stereo movement, and <b>Mix</b> blends the '
           'dry signal back in.'))
s.append(P('SHAPE, RING and EQ', 'h2'))
s.append(P('<b>SHAPE</b> distorts with eight curves: Tube, Hard Clip, Wavefold, Asym Fold, Sine Shaper, Rectify, Bitcrush and '
           'Decimate, followed by a Tone low-pass. <b>RING</b> multiplies the lane by an internal sine, triangle or square (tuned '
           'relative to the note) or by an earlier lane. <b>EQ</b> is a per-voice low shelf, mid peak and high shelf.'))

# ------------------------------------------------------------------ 6 Modulation
s.append(PageBreak())
s.append(P('6. Modulation', 'h1'))
s.append(P('The modulation rack under the module editor has five families of modulators (ENV, LFO, MSEG, MAP, MATH), four of '
           'each, plus the MATRIX. Choose the family on the left and the number on the right.'))
s += img(crop('bowed.png', (456, 501, 1400, 944), 'crop_mseg.png'), caption='MSEG 1: a drawable multi-segment envelope.')
s.append(P('Envelopes', 'h2'))
s.append(P('Delay, Attack, Decay, Sustain and Release, with <b>Slope</b> bending the curves (right is snappy, left is slow), '
           '<b>Velocity</b> scaling the whole envelope by how hard you play, <b>Key Scale</b> shortening higher notes and <b>One Shot</b> '
           'running the whole shape regardless of how long you hold the key. ENV 1 is always the amp envelope.'))
s.append(P('LFOs', 'h2'))
s.append(P('Seven waves (sine, triangle, saw up and down, square, sample-and-hold, smooth random), a free rate or a tempo-synced '
           'Division, a start Phase, a Delay and a Fade In for vibrato that blooms. <b>Mode</b> chooses Retrig (restarts per note), '
           'Free (per voice, never restarts) or Global (one LFO shared by all voices, which can also modulate effects). '
           'A global LFO shows a moving dot in its display.'))
s.append(P('MSEGs', 'h2'))
s.append(P('Draw any shape with up to 16 points. Drag points to move them (they snap to a grid; hold Alt to move freely), '
           'double-click empty space to add a point and double-click a point to delete it. Drag the small amber diamonds up and '
           'down to bend a segment. Right-click a point to set the loop start or end, or right-click empty space for ready-made '
           'shapes. <b>Loop</b> chooses One Shot, Sustain Loop (cycles between the markers while the key is held, then plays on) or '
           'Repeat. <b>Sync</b> turns grid units into beats; <b>Time</b> scales everything; <b>Bipolar</b> makes the bottom -1.'))
s.append(P('Mappers', 'h2'))
s.append(P('A mapper is a row of up to 32 bars you draw with the mouse. In <b>Map</b> mode an input (Key Follow, velocity, an '
           'envelope, an LFO and more) picks a bar, so you can draw custom keyboard scaling or velocity curves. In <b>Step Seq</b> '
           'mode the bars play in time at the chosen Division, turning the mapper into a step sequencer. Smooth glides between bars. '
           'Right-click for ready-made patterns.'))
s.append(P('Mod Math', 'h2'))
s.append(P('Combines two sources: A+B, A-B, AxB, Max, Min, Average, Lag A by B (smooths a stepped source), Quantize A by B '
           '(turns a smooth source into steps), Abs A and Invert A, scaled by Amount. Use the result like any other source.'))
s.append(P('The matrix', 'h2'))
s += img(crop('mallet_matrix.png', (456, 501, 1400, 944), 'crop_matrix.png'), caption='The modulation matrix.')
s.append(P('24 slots, each connecting a <b>Source</b> to a <b>Target</b> by an <b>Amount</b> (negative inverts). <b>Via</b> is an '
           'optional second source that scales the connection: put Mod Wheel there and the wheel fades the modulation in. Targets '
           'are chosen from a menu grouped by module. An Amount of 100% sweeps the whole range of the target.'))
s += tipbox('THE FASTEST WAY', 'Right-click any knob and choose <b>Modulate with</b>. Manticore fills the next free matrix slot '
            'at 25% and an amber arc appears around the knob. Right-click again to remove it.')
s.append(P('Sources are the four of each modulator, plus Velocity, Key Follow, Mod Wheel, Aftertouch (channel or polyphonic), '
           'Pitch Bend, Macros A-D, Random (a new value per note), Gate, Expression (CC 11) and Alternate (flips between +1 and -1 '
           'on every note, for round-robin variation). Effect parameters are processed once for all voices, so they respond only to '
           'the global sources: the macros, mod wheel, aftertouch, pitch bend, expression and LFOs set to Global.'))
s.append(P('Macros (Ctrl A-D)', 'h2'))
s.append(P('Four performance knobs on the SYNTH page. They do nothing by themselves; route them in the matrix. Every factory '
           'preset follows the same pattern so your hands learn them once: <b>A = Tone</b> (usually brightness or the filter), '
           '<b>B = Motion</b> (LFO depth, sequence depth, vibrato), <b>C = Space</b> (reverb and delay) and <b>D = Grit or '
           'Character</b> (drive, ring modulation, detune). Each preset\'s description in the browser says exactly what its macros do. '
           'Map them to MIDI knobs with REAPER\'s parameter learn.'))

# ------------------------------------------------------------------ 7 Effects
s.append(PageBreak())
s.append(P('7. Effects', 'h1'))
s += img('fx_texture.png', caption='The EFFECTS page: FX grid, returns, lane sends, the TextureVerb editor and the signal-flow map.')
s.append(P('The FX grid has four buses of four slots. <b>Main</b> processes the whole mix last. <b>Aux 1-3</b> are send buses: each '
           'lane sends to them with the knobs in the lane mixer (or the Lane Sends panel here), each runs its own effect chain, and '
           'their returns feed into the Main bus. Effects on an aux bus should usually be 100% wet. New patches start with REVERB 1 '
           'on Aux 1 and DELAY 1 on Aux 2. The signal-flow map shows what is connected, live.'))
rows = [['Effect', 'Pool', 'What it does'],
        ['Mod FX', '3', 'Chorus (2-8 voices), Flanger or Phaser (2-24 stages), digital or analogue character.'],
        ['Delay', '2', 'Mono, stereo or ping-pong, synced or free, filtered and tape-wobbled repeats, diffusion, analogue saturation. '
                       'Feedback above 100% builds into a self-oscillating wall.'],
        ['Reverb', '2', 'Classic (focused hall) or Lush (dense, modulated cathedral) from an 8-line feedback network, up to 30 s.'],
        ['TextureVerb', '2', 'A granular cloud: grains of the input, pitched, scattered, reversed and fed back. Freeze holds the cloud.'],
        ['Compressor', '2', 'Threshold, ratio, attack, release, knee, makeup and parallel mix.'],
        ['Master EQ', '2', 'Low shelf, two mid peaks and a high shelf.'],
        ['Distortion', '2', 'Tube, Hard Clip, Fuzz, Fold, Rectify, Bitcrush and Downsample with a tone control.'],
        ['FX Filter', '2', 'A resonant low, high, band-pass or notch filter for whole-mix sweeps (map a macro to its cutoff).']]
s += table(rows, [1.2 * inch, 0.5 * inch, 5.3 * inch])

# ------------------------------------------------------------------ 8 Arp
s.append(P('8. The Arpeggiator', 'h1'))
s += img('arp.png', caption='The ARP page.')
s.append(P('Turn on <b>ARP ON</b> and hold notes: Manticore plays them one step at a time in the chosen <b>Order</b> (Up, Down, '
           'Up Down, Down Up, As Played, Random, or Chord, which plays all held notes on every step), across 1-4 <b>Octaves</b>, at '
           'the step <b>Rate</b>, locked to REAPER\'s tempo and bar position. <b>Steps</b> sets the pattern length, so a 7-step '
           'pattern against 4/4 drifts in true prog fashion. <b>Swing</b> delays every second step and <b>Gate</b> scales all note '
           'lengths. <b>HOLD</b> latches the notes until you play a new chord.'))
s.append(P('Each of the 16 steps can be switched on or off (a rest), transposed (-24 to +24 semitones), given its own velocity, '
           'and its own gate length. A step at 100% gate ties into the next step, which glides in Mono and Legato voice modes. '
           'Drag the bars to set them; double-click a bar to reset it.'))

# ------------------------------------------------------------------ 9 XY
s.append(PageBreak())
s.append(P('9. The XY Vector Mixer', 'h1'))
s += img('xy.png', width=6.0 * inch, caption='The XY page: XY 1 opens the filter and morphs the wavetable to the right, adds resonance upwards.')
s.append(P('The XY page has four pads. Each pad moves up to four targets on its horizontal axis and four on its vertical axis, so a '
           'single gesture can morph a whole patch: brighter and wider to the right, darker and closer to the left, more resonant '
           'towards the top. The centre of a pad leaves the sound exactly as stored; moving away pushes every target on that axis '
           'by up to its amount (negative amounts move it the other way).'))
s += bullets([
    '<b>Assigning targets:</b> click a target box under a pad and choose a parameter, then set its amount with the slider. Quicker: '
    'right-click any knob and choose <b>Assign to XY pad</b>, then the pad and axis. The knob\'s amber arc shows the range the pad sweeps.',
    '<b>Playing:</b> drag the dot. Double-click a pad to send it back to the centre.',
    '<b>Automation:</b> the eight pad positions (XY 1 X to XY 4 Y) are host parameters, so you can draw pad movements in REAPER '
    'or link them to a controller with REAPER\'s parameter modulation.',
    '<b>As modulation sources:</b> the pads also appear as XY 1 X ... XY 4 Y in the Mod Matrix sources and in Mod Math. They are '
    'global sources, so unlike envelopes they can move effect parameters too.'])
s += tipbox('A ONE-HAND MORPH', 'Put VCF 1 Cutoff (+50%) and OSC 1 Wave (+40%) on XY 1 X, VCF 1 Reso (+35%) and OSC 1 Detune (-30%) on '
            'XY 1 Y, and Aux 1 Return on XY 2 Y. Now one hand plays the tone while the other plays the space.')

# ------------------------------------------------------------------ 10 Mutate / Breed / Haunt
s.append(PageBreak())
s.append(P('10. Mutate, Breed and Haunt', 'h1'))
s.append(P('Three tools in the toolbar make new sounds from the one you have. All three respect the padlocks, keep the pitch and '
           'key protected, and leave a level-matched result, so you can keep pressing without your monitors jumping. Every press '
           'can be undone.', 'lead'))
s.append(P('Mutate', 'h2'))
s.append(P('<b>MUTATE</b> makes a new variation of the current sound by nudging the unlocked controls. The <b>Strength</b> slider '
           'beside it sets how far: around 10-25% gives close cousins, 35-50% clear variations, 70% and up wild new creatures. '
           'At higher strengths Mutate may also rearrange the grid (swap two processors in a lane, add a filter, shaper, comb, ring '
           'modulator or EQ below the sources, take one away, or switch a lane input), add or rewire a few matrix routings and '
           'reorder effects. It never changes any Tune or Fine knob, transpose, the macro knobs, routings driven by a macro, or Arp '
           'on/off; ratios and ring-modulator intervals only jump to musical values. Behind the scenes it renders a short test '
           'chord offline, rejects candidates that collapse, blow up or can\'t reach the original loudness, and matches the level. The preset name gains '
           'a marker such as <b>~3</b>.'))
s.append(P('<b>Mutate one module.</b> Every module editor (generators, filters, modulators and effects) has its own <b>MUTATE</b> '
           'button at the top right. It varies only that module at the toolbar\'s Strength and ignores the padlocks, since you chose '
           'the module yourself. Use it to try new LFO shapes, a different filter model or a fresh reverb without touching anything else.'))
s.append(P('Padlocks', 'h2'))
s += img(crop('locks.png', (0, 112, 1400, 545), 'crop_locks.png'), caption='Padlocks closed on the grid Layout, Lane 2 and the Modulators (amber = locked).')
s.append(P('Each section has a padlock. A closed (amber) padlock tells Mutate, Breed and Haunt to leave that section alone:'))
s += small_table([['Padlock', 'Where', 'What it keeps'],
                  ['Layout', 'Generator Grid title', 'Every module in its slot and the lane inputs: Mutate and Breed can\'t move, add or remove modules'],
                  ['Lane 1-4', 'Beside each LANE caption', 'The settings of the modules in that lane and its lane mixer controls'],
                  ['Modulators', 'Modulation rack header', 'All envelopes, LFOs, MSEGs, mappers and mod math (the drawn shapes are never mutated)'],
                  ['Matrix', 'Modulation rack header', 'The 24 matrix routings and amounts'],
                  ['Effects', 'FX Grid title (EFFECTS page)', 'Effect chains, effect settings and aux returns'],
                  ['Arp', 'ARP page', 'The arpeggiator pattern and settings'],
                  ['XY pads', 'XY page', 'The pad targets and amounts (and Haunt won\'t move the pads)']],
                 [1.0 * inch, 1.8 * inch, 4.2 * inch])
s += tipbox('LOCK, THEN MUTATE', 'Found a pad whose envelope and effects are perfect? Lock Modulators, Effects and Layout, then mutate: '
            'only the oscillator and filter settings change. Lock everything except the Matrix to discover new movement for a sound you '
            'otherwise love.', PALE_AMBER, AMBER)
s.append(P('Breed and A/B', 'h2'))
s.append(P('<b>A</b> and <b>B</b> hold two complete sounds; loading presets or editing only changes the slot you are on. Fill both '
           'and press <b>BREED</b>: each lane (with the modules in it) comes from one parent, each modulator from one parent, the '
           'matrix, effects, arpeggiator and XY pads from one parent each, and values that exist in both are blended part of the way '
           'towards the other parent. Some matrix routings are inherited from the other parent too. Modules are re-assigned when '
           'both parents use the same one, so the child never has an inactive duplicate. The child replaces the slot you are on (the '
           'other parent stays in its slot, so you can breed again), sits at the parents\' average loudness, and gets a name built from '
           'both: <i>Kingsbane Mallets</i> bred with <i>Phantom Tapestry</i> might become <i>Kingsbane Tapestry</i>. Locked '
           'sections come from the sound you are on. Hover over BREED to see which two sounds it will cross.'))
s.append(P('<b>Breed with a Favourite</b> (the BREED button with the star) picks one of your starred presets at random, puts it in the '
           'other slot and breeds it with the current sound. Star a handful of presets you love and press it repeatedly for an '
           'endless supply of relatives.'))
s.append(P('Haunt', 'h2'))
s += img(crop('haunt.png', (846, 250, 1390, 495), 'crop_haunt.png'), width=4.6 * inch,
         caption='Haunt running: pale blue ghost pointers show where each control has drifted. The stored settings do not move.')
s.append(P('Press <b>HAUNT</b> and the patch begins to drift on its own. Every unlocked control wanders slowly and smoothly around '
           'its setting, each at its own unrelated speed, so the sound never quite repeats. That is ideal for ambient beds, '
           'paranormal atmospheres and long drones. Matrix amounts drift too, so routings breathe in and out, and so do the XY pads. '
           'The stored knob positions never move, so nothing is written to your automation; instead, pale blue <b>ghost pointers</b> '
           'on the knobs (and a ghost ring on the XY pads) show where each control has wandered. <b>Depth</b> sets how far and '
           '<b>Speed</b> how fast. Turn Haunt off and the sound glides home over about a second.'))
s += bullets([
    '<b>FOLLOW</b> links the drift to your playing: dig in with hard velocities (or push the mod wheel) and it wanders further; play '
    'softly or stop and it settles home.',
    '<b>CAPTURE</b> keeps what Haunt is playing right now as the new knob settings, then turns Haunt off. Haunt is a sound-design '
    'tool as much as a performance one: let it wander, and capture the moment you like.',
    'Pitch (Tune, Fine, key tracking), choices, switches and the envelope delay never drift. Locked sections stay still.',
    'Haunt, Depth, Speed and Follow are host parameters, so you can automate Haunt switching on for a bridge or a breakdown. They are '
    'performance settings: loading presets, A/B, Undo and Mutate leave them as they are, and they are not saved in presets.'])
s.append(P('Undo and Redo', 'h2'))
s.append(P('<b>UNDO</b> and <b>REDO</b> step through the last 48 changes: preset loads, knob and switch moves, grid changes, drawn '
           'shapes, A/B switches, Mutate, Breed and Capture. Each knob gesture counts as one step.'))

# ------------------------------------------------------------------ 11 Voice
s.append(PageBreak())
s.append(P('11. Voice, Performance and MIDI', 'h1'))
s.append(P('<b>Voices</b> sets the polyphony (1-32). <b>Voice Mode</b> is Poly, Mono (every new note retriggers the envelopes) or '
           'Legato (envelopes only retrigger when you lift between notes). <b>Glide</b> sets the portamento time and <b>Glide Mode</b> '
           'whether it always happens or only between overlapping notes. <b>Bend +/-</b> set the pitch-bend range and <b>Transpose</b> '
           'shifts everything. <b>Master</b> is the final output level, after the effects, with peak meters beside it.'))
s.append(P('Manticore responds to note on and off with velocity, pitch bend, mod wheel (CC 1), expression (CC 11), sustain pedal '
           '(CC 64), channel aftertouch, polyphonic aftertouch and all-notes-off. The mod wheel is pre-routed to vibrato or a '
           'timbral control in most factory presets.'))

# ------------------------------------------------------------------ 10 Presets
s.append(PageBreak())
s.append(P('12. Presets, the Browser and Favourites', 'h1'))
s += img('browser.png', caption='The BROWSE page.')
s.append(P('The browser lists all 500 factory presets plus your own. Choose a category on the left (or All, Favourites or User), '
           'narrow it with the genre tags (Rock, Metal, Prog, Cinematic, Ambient, Game; several tags show presets that match any '
           'of them) or type in the search box, which also searches descriptions. <b>A single click loads a preset</b>, so you can '
           'audition quickly; the arrow keys step through the list. The panel on the right shows the description and what the '
           'macros do.'))
s.append(P('Favourites', 'h2'))
s.append(P('Click the star beside any name in the list, the star in the preset panel, or the star in the header to add the current '
           'preset to your Favourites. The Favourites category then shows only those. Favourites are stored in '
           'Documents\\Manticore\\Favourites.txt and are shared by every instance and project. User presets can be favourited too.'))
s.append(P('Saving, A/B and undo', 'h2'))
s.append(P('SAVE (in the header or the browser) asks for a name, a category, genre tags and a description, and writes the preset to '
           'Documents\\Manticore\\Presets. The header arrows step through whatever the browser is currently showing, so you can '
           'filter to "Metal Leads" and then flip through them while you play. <b>A</b> and <b>B</b> hold two complete settings '
           'for comparison and the copy button clones one into the other (A/B is also what Breed works with, see section 10). '
           '<b>UNDO</b> and <b>REDO</b> step through the last 48 changes. <b>INIT</b> returns to a plain oscillator into a filter. '
           'A mutated or bred sound is new, so save it before starring it.'))

# ------------------------------------------------------------------ 11 Recipes
s.append(P('13. Sound Design Recipes', 'h1'))
recipes = [
    ('A plucked fantasy lute', 'INIT. Set slot L1-S1 to EXC 1 (Burst, Decay 6 ms), S2 to COMB 1 (Feedback 0.99, Damping 0.2), S3 to '
     'VCF 1 (SVF LP 12, cutoff 6 kHz). Set ENV 1 Sustain 0, Decay 3 s. Right-click EXC 1 Color, Modulate with Velocity.'),
    ('A struck bell', 'EXC 1 (Mallet) above RES 1 (Bell, Decay 4 s, Damping 0.4). ENV 1 Sustain 0. Raise RES Spread for an older, '
     'beaten bell; route Velocity to EXC Color for harder strikes.'),
    ('A trailer braam', 'Three OSCs tuned down an octave (saw, Growl, Brass Swell) and SHAPE 1 (Tube) in lane 1. Lane 1 volume to '
     'minimum, lane 2 Input = Lane 1, VCF 1 (Ladder LP 24, 400 Hz) in lane 2. ENV 2 (Attack 0.5 s) to the cutoff. Lush reverb on Aux 1.'),
    ('A shredding metal lead', 'OSC 1 saw with FX 1 = Sync, OSC 2 saw an octave down, SHAPE 1 Hard Clip, VCF 1 Bite. Legato mode, '
     'Glide 0.04 s. ENV 2 (fast decay) to OSC 1 FX 1 Amt for a tearing attack; LFO 1 to both OSC tunes with Via = Mod Wheel for vibrato.'),
    ('A haunted pad', 'OSC 1 on the Ghost table (Additive, Stretch -0.3), RING 1 (sine, +6.5 st, mix 30%), VCF 1 Phaser. Slow ENV 1. '
     'Smooth Rnd LFO to the cutoff and ring tune. TextureVerb on Aux 3 with Pitch -12 and Reverse 60%.'),
    ('A prog step sequence', 'Saw into a Diode filter. MAP 1 in Step Seq mode, 7 steps at 1/16, routed to the cutoff. Hold one note '
     'and let the 7-step pattern cycle against the bar.'),
]
for title, text in recipes:
    s += tipbox(title.upper(), text, PALE_AMBER, AMBER)

# ------------------------------------------------------------------ 12 Preset list
s.append(PageBreak())
s.append(P('14. Factory Preset List', 'h1'))
s.append(P('500 presets in twelve categories, each tagged with the genres it suits best. The first sentence of each description is '
           'shown here; the browser shows it in full with the macro assignments. All presets are level-matched to sit at a similar '
           'loudness.'))
presets = json.load(open(os.path.join(ROOT, 'docs', 'presets.json')))
order = []
for p in presets:
    if p['cat'] not in order:
        order.append(p['cat'])
for cat in order:
    rows = [[cat, 'Genres', 'Sound']]
    for p in presets:
        if p['cat'] == cat:
            first = p['desc'].split('\n')[0]
            first = first.split('. ')[0].rstrip('.') + '.'
            rows.append([p['name'], ', '.join(p['tags']), first])
    s.append(CondPageBreak(1.4 * inch))
    s += small_table(rows, [1.55 * inch, 1.25 * inch, 4.2 * inch])

# ------------------------------------------------------------------ 13 Specs
s.append(PageBreak())
s.append(P('15. Specifications and Troubleshooting', 'h1'))
rows = [['Item', 'Detail'],
        ['Format', 'VST3 instrument (64-bit Windows) and standalone app'],
        ['Architecture', '4 lanes x 4 slots; module pool of 28 generators and processors; lane-to-lane routing'],
        ['Oscillators', '24 wavetables x 8 frames, 16-voice unison, additive engine (64 partials), 12 wave FX'],
        ['Filters', '13 models, 12 responses'],
        ['Physical modelling', '5 exciter modes, 8 resonator profiles with 12 modes each, tuned comb with diffuse mode'],
        ['Modulation', '4 envelopes, 4 LFOs, 4 MSEGs (16 points), 4 mappers (32 steps), 4 mod math, 24-slot matrix, 4 macros'],
        ['Effects', 'Main bus plus 3 aux buses, 4 slots each; 17 effect units in 8 types'],
        ['Arpeggiator', '16 steps with per-step on, transpose, velocity and gate; 7 orders; host-synced'],
        ['XY mixer', '4 pads, 4 targets per axis, automatable, also global modulation sources'],
        ['Sound tools', 'Mutate (patch or module), Breed (A/B or with a favourite), Haunt with Follow and Capture, 10 padlocks, 48-step undo'],
        ['Polyphony', '1 to 32 voices; Poly, Mono and Legato modes with glide'],
        ['Sample rates', 'Tested at 44.1, 48, 96 and 192 kHz'],
        ['Presets', '500 factory presets, unlimited user presets, favourites'],
        ['Fonts', 'Cinzel and Rajdhani, SIL Open Font License (licence texts installed with the app)']]
s += table(rows, [1.6 * inch, 5.4 * inch])
s.append(P('Troubleshooting', 'h2'))
s += bullets([
    '<b>No sound:</b> check the track is armed and receiving MIDI, that ENV 1 has some Sustain or a long Decay, and that at least '
    'one lane has a generator in it and its volume up.',
    '<b>A module does nothing:</b> it may not be in the grid (the editor title says NOT IN THE GRID) or it may be a red Duplicate.',
    '<b>An effect does not react to modulation:</b> effects only follow global sources. Set the LFO to Global, or use a macro or the mod wheel.',
    '<b>The arpeggiator is silent:</b> make sure some steps are on and Steps is at least 1; with the transport stopped it runs at the session tempo.',
    '<b>High CPU:</b> unison above 8 voices, many additive partials and long polyphonic releases cost the most. Lower Voices, '
    'Unison or Partials, or freeze the track in REAPER.',
    '<b>Plug-in missing after install:</b> clear the cache and re-scan in REAPER\'s VST preferences.',
    '<b>Mutate or Breed says it couldn\'t find a playable result:</b> lower the Strength, or open a padlock or two. Breed also needs two '
    'different sounds in A and B.',
    '<b>The sound keeps moving on its own:</b> Haunt is on (the HAUNT button is lit). Turn it off, or press CAPTURE to keep the current state.',
    '<b>A preset name changed after updating:</b> 40 factory presets were renamed in 1.1 so their names match their sound '
    '(for example Kingsbane Glockenspiel is now Kingsbane Mallets). Stars and saved projects follow the new names automatically.'])

# ------------------------------------------------------------------ appendix
s.append(P('Appendix: Building from GitHub', 'h1'))
s.append(P('Manticore builds the same way as the other Fellowship plug-ins. Upload the project folder to a GitHub repository; '
           'the included workflow (.github/workflows/build.yml) runs automatically on every push to main, or by hand from the '
           '<b>Actions</b> tab with <b>Run workflow</b>. When it finishes (about 15-25 minutes), open the run and download '
           '<b>Manticore-Setup</b>: inside is Manticore-1.1.0-Setup.exe. A second download, Manticore-VST3, holds the bare plug-in.'))
s.append(P('The factory presets are generated by tools/make_presets.py and level-matched by tools/normalize_presets.py; the icons '
           'come from tools/make_icons.py and this manual from tools/make_manual.py. You only need these if you want to change '
           'them. The built-in presets are read from Resources/factory_presets.json.gz.'))

doc.build(s)
print('wrote', OUT)
