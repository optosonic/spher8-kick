# Spher8 KICK

Design copy of the Spher8 KICK panel, for updating Base44 and revising the design in turn.

The sounding instrument in the DAW still lives in Atrium at `src/Instruments/Kick/`. This repository is the design copy to update Base44 from. Design revisions made here are not automatically applied to Atrium.

The files under `src/Instruments/Kick/` are the current params, engine, presets, instrument, editor, and check. The build tree, JUCE, and the rest of the DAW are not in this repo.

## Current UI

Spher8 KICK is a dark metal panel. The background falls from steel to near-black. Accents are cyan (`#4EB8D0`) and amber (`#e0a84a`). The title reads Spher8 over KICK.

**Header.** Preset navigation (`<`, the preset name, `>`), then SAVE, COMPACT, and ON. ON reads OFF when the instrument is bypassed. COMPACT switches to the shorter window. When there is room, an amber FUNDAMENTAL readout (Hz and note) sits left of the preset nav.

**Center canvas.** One view at a time: WAVE, PITCH, SPECTRUM, LAYERS, or FOLLOW. A level meter sits on the right, with a numeric readout and an amber peak-hold mark. Clipping turns that mark orange.

**FOLLOW.** The post-kick window after the impact. A ruler marks the span in milliseconds. Two lanes share that window:

- POST AMP is the sub boost after the impact. SUB DEP scales it. Drive and harmonics follow this curve when their depths are up. It is separate from the body AMP page.
- PITCH is the sub pitch offset across the same window. PIT DEP scales it. This lane does not change level.

**Chip groups.**

- LAYERS: BODY, SUB, CLICK, LAYER
- VIEW: WAVE, PITCH, SPECTRUM, LAYERS, FOLLOW
- PAGE: PITCH, AMP, HARMONICS, DYNAMICS, OUTPUT (the second dial row in the full window)
- FOLLOW: POST AMP and PITCH lane chips, plus the post-kick destinations, when that view is open

**Main dials.** TUNE, PUNCH, DECAY, DRIVE, CLICK, SUB, MORPH. In the full window the row above them follows the PAGE selection. FOLLOW adds SUB DEP, DRV DEP, PIT DEP, SPAN, CURVE, and HARM.

**Bottom strip.** RANGE sets which notes trigger the kick. The default is C1 (24) through C2 (36). Notes outside that span are ignored. KEY % sits on the right and spreads those main dials across the range. 0% plays the designed kick on every note inside it.
