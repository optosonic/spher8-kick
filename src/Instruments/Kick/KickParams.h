#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>

// Spher8 KICK. Parameter order is the automation order: do not reorder fields.
// kind: 0 continuous, 1 boolean, 2 stepped choice. log: frequency-like blend.

#define ATRIUM_KICK_FIELDS(M) \
    M(enabled,     "On",           "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(tune,        "Tune",         "Hz",  20.0f,    250.0f,   70.0f,  0.0f, 0, 1) \
    M(semi,        "Semitones",    "st",  -36.0f,   36.0f,    0.0f,   1.0f, 0, 0) \
    M(fine,        "Fine",         "ct",  -100.0f,  100.0f,   0.0f,   0.0f, 0, 0) \
    M(a4,          "A4",           "Hz",  420.0f,   460.0f,   440.0f, 0.0f, 0, 0) \
    M(track,       "Key Track",    "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(voice,       "Voices",       "",    0.0f,     1.0f,     0.0f,   1.0f, 2, 0) \
    M(channel,     "Channel",      "",    0.0f,     16.0f,    0.0f,   1.0f, 0, 0) \
    M(bodyOn,      "Body",         "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(bodyWave,    "Body Wave",    "",    0.0f,     5.0f,     0.0f,   1.0f, 2, 0) \
    M(bodyLevel,   "Body Level",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(bodyHarm,    "Harmonics",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(bodyPol,     "Body Polarity","",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(impactSt,    "Impact",       "st",  0.0f,     72.0f,    0.0f,   0.0f, 0, 0) \
    M(impactMs,    "Impact Time",  "ms",  0.2f,     80.0f,    8.0f,   0.0f, 0, 0) \
    M(impactShape, "Impact Shape", "",    0.0f,     2.0f,     0.0f,   1.0f, 2, 0) \
    M(impactCurve, "Impact Curve", "",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(bodySt,      "Body Pitch",   "st",  -36.0f,   48.0f,    0.0f,   0.0f, 0, 0) \
    M(bodyMs,      "Body Time",    "ms",  5.0f,     1500.0f,  120.0f, 0.0f, 0, 0) \
    M(bodyShape,   "Body Shape",   "",    0.0f,     2.0f,     0.0f,   1.0f, 2, 0) \
    M(bodyCurve,   "Body Curve",   "",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(attack,      "Attack",       "ms",  0.0f,     80.0f,    4.0f,   0.0f, 0, 0) \
    M(decay,       "Decay",        "ms",  15.0f,    4000.0f,  250.0f, 0.0f, 0, 0) \
    M(sustain,     "Sustain",      "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(release,     "Release",      "ms",  5.0f,     2500.0f,  80.0f,  0.0f, 0, 0) \
    M(phaseReset,  "Phase Reset",  "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(velAmt,      "Vel Amp",      "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(velClick,    "Vel Click",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(velDrive,    "Vel Drive",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(velPitch,    "Vel Pitch",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(subOn,       "Sub On",       "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(subLevel,    "Sub",          "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(subHz,       "Sub Tune",     "Hz",  20.0f,    160.0f,   50.0f,  0.0f, 0, 1) \
    M(subTrack,    "Sub Track",    "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(subSemi,     "Sub Semitones","st",  -24.0f,   24.0f,    0.0f,   1.0f, 0, 0) \
    M(subAttack,   "Sub Attack",   "ms",  0.0f,     40.0f,    2.0f,   0.0f, 0, 0) \
    M(subDecay,    "Sub Decay",    "ms",  15.0f,    4000.0f,  280.0f, 0.0f, 0, 0) \
    M(subPhase,    "Sub Phase",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(subHarm,     "Sub Harm",     "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(clickOn,     "Click On",     "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(clickLevel,  "Click",        "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(clickType,   "Click Type",   "",    0.0f,     6.0f,     0.0f,   1.0f, 2, 0) \
    M(clickMs,     "Click Time",   "ms",  0.2f,     60.0f,    6.0f,   0.0f, 0, 0) \
    M(clickHz,     "Click Freq",   "Hz",  80.0f,    12000.0f, 2500.0f,0.0f, 0, 1) \
    M(clickBright, "Brightness",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(clickHp,     "Click HP",     "Hz",  20.0f,    8000.0f,  800.0f, 0.0f, 0, 1) \
    M(clickShape,  "Click Shape",  "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(clickPos,    "Click Offset", "ms",  -25.0f,   25.0f,    0.0f,   0.0f, 0, 0) \
    M(layerOn,     "Layer On",     "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(layerLevel,  "Layer Level",  "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(layerStart,  "Layer Start",  "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(layerEnd,    "Layer End",    "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(layerSemi,   "Layer Pitch",  "st",  -24.0f,   24.0f,    0.0f,   1.0f, 0, 0) \
    M(layerAttack, "Layer Attack", "ms",  0.0f,     80.0f,    2.0f,   0.0f, 0, 0) \
    M(layerDecay,  "Layer Decay",  "ms",  15.0f,    4000.0f,  200.0f, 0.0f, 0, 0) \
    M(layerPol,    "Layer Polarity","",   0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(layerPos,    "Layer Offset", "ms",  -25.0f,   25.0f,    0.0f,   0.0f, 0, 0) \
    M(soloBody,    "Solo Body",    "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(soloSub,     "Solo Sub",     "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(soloClick,   "Solo Click",   "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(soloLayer,   "Solo Layer",   "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(drive,       "Drive",        "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(evenOdd,     "Even Odd",     "",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(satMode,     "Saturation",   "",    0.0f,     4.0f,     0.0f,   1.0f, 2, 0) \
    M(fold,        "Fold",         "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(asym,        "Asymmetry",    "",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(tone,        "Tone",         "",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(cutoff,      "Cutoff",       "Hz",  40.0f,    18000.0f, 2000.0f,0.0f, 0, 1) \
    M(hp,          "High Pass",    "Hz",  15.0f,    4000.0f,  80.0f,  0.0f, 0, 1) \
    M(punch,       "Punch",        "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(atkEmph,     "Attack Emphasis","",  0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(susShape,    "Sustain Shape","",    -1.0f,    1.0f,     0.0f,   0.0f, 0, 0) \
    M(clickEnh,    "Click Enhance","",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(comp,        "Compress",     "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(thresh,      "Threshold",    "dB",  -48.0f,   0.0f,     0.0f,   0.0f, 0, 0) \
    M(ratio,       "Ratio",        "",    1.0f,     20.0f,    4.0f,   0.0f, 0, 0) \
    M(compA,       "Comp Attack",  "ms",  0.05f,    40.0f,    5.0f,   0.0f, 0, 0) \
    M(compR,       "Comp Release", "ms",  5.0f,     400.0f,   80.0f,  0.0f, 0, 0) \
    M(makeup,      "Makeup",       "dB",  0.0f,     18.0f,    0.0f,   0.0f, 0, 0) \
    M(clip,        "Clip",         "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(ceiling,     "Ceiling",      "dB",  -18.0f,   0.0f,     0.0f,   0.0f, 0, 0) \
    M(clipHard,    "Hard Clip",    "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(output,      "Output",       "dB",  -24.0f,   12.0f,    0.0f,   0.0f, 0, 0) \
    M(morph,       "Morph",        "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(inputNote,   "Input Note",   "",    0.0f,     128.0f,   0.0f,   1.0f, 0, 0) \
    M(followSub,   "Follow Sub",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followDrive, "Follow Drive", "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followLevel, "Follow Level", "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followRes,   "Follow Res",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followMs,    "Follow Time",  "ms",  15.0f,    4000.0f,  400.0f, 0.0f, 0, 0) \
    M(followCurve, "Follow Curve", "",   -1.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followShape, "Follow Shape", "",    0.0f,     2.0f,     0.0f,   1.0f, 2, 0) \
    M(followT1,    "Follow A",     "",    0.05f,    0.80f,    0.28f,  0.0f, 0, 0) \
    M(followT2,    "Follow B",     "",    0.15f,    0.96f,    0.62f,  0.0f, 0, 0) \
    M(followSt0,   "Follow St 1",  "st", -24.0f,    24.0f,    0.0f,   0.0f, 0, 0) \
    M(followSt1,   "Follow St 2",  "st", -24.0f,    24.0f,    0.0f,   0.0f, 0, 0) \
    M(followSt2,   "Follow St 3",  "st", -24.0f,    24.0f,    0.0f,   0.0f, 0, 0) \
    M(followSt3,   "Follow St 4",  "st", -24.0f,    24.0f,    0.0f,   0.0f, 0, 0) \
    M(followAmp0,  "Post Amp 1",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followAmp1,  "Post Amp 2",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followAmp2,  "Post Amp 3",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followAmp3,  "Post Amp 4",   "",    0.0f,     1.0f,     0.0f,   0.0f, 0, 0) \
    M(followAmpT1, "Post Amp A",   "",    0.05f,    0.80f,    0.28f,  0.0f, 0, 0) \
    M(followAmpT2, "Post Amp B",   "",    0.15f,    0.96f,    0.62f,  0.0f, 0, 0) \
    M(followAmpUse,"Post Amp On",  "",    0.0f,     1.0f,     0.0f,   1.0f, 1, 0) \
    M(noteLo,      "Note Low",     "",    0.0f,     127.0f,   24.0f,  1.0f, 0, 0) \
    M(noteHi,      "Note High",    "",    0.0f,     127.0f,   36.0f,  1.0f, 0, 0) \
    M(keyAmount,   "Key",          "%",   0.0f,     1.0f,     0.0f,   0.0f, 0, 0)

namespace atrium::kick
{
    inline constexpr int kFieldCount = 0
#define ATRIUM_KICK_COUNT(name, title, unit, lo, hi, centre, step, kind, log) + 1
        ATRIUM_KICK_FIELDS(ATRIUM_KICK_COUNT)
#undef ATRIUM_KICK_COUNT
        ;

    enum Field : int
    {
#define ATRIUM_KICK_ENUM(name, title, unit, lo, hi, centre, step, kind, log) name,
        ATRIUM_KICK_FIELDS(ATRIUM_KICK_ENUM)
#undef ATRIUM_KICK_ENUM
        count
    };

    static_assert((int) Field::count == kFieldCount, "kick field count");
    static_assert((int) Field::noteLo + 1 == (int) Field::noteHi, "note range order");
    static_assert((int) Field::noteHi + 1 == (int) Field::keyAmount, "key amount follows the note range");
    static_assert((int) Field::keyAmount + 1 == (int) Field::count, "key amount is appended");

    struct Targets
    {
#define ATRIUM_KICK_MEMBER(name, title, unit, lo, hi, centre, step, kind, log) float name = 0.0f;
        ATRIUM_KICK_FIELDS(ATRIUM_KICK_MEMBER)
#undef ATRIUM_KICK_MEMBER
    };

    static_assert(sizeof(Targets) == sizeof(float) * (size_t) kFieldCount, "kick targets are packed floats");

    inline float clampf (float value, float lo, float hi) noexcept
    {
        return std::max (lo, std::min (hi, value));
    }

    inline float sanef (float value) noexcept
    {
        return std::isfinite (value) ? value : 0.0f;
    }

    inline float lerpf (float a, float b, float t) noexcept
    {
        return a + (b - a) * t;
    }

    inline float logLerp (float a, float b, float t) noexcept
    {
        a = std::max (a, 1.0e-4f);
        b = std::max (b, 1.0e-4f);
        return std::exp (lerpf (std::log (a), std::log (b), t));
    }

    inline float dbToGain (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    inline float* fieldData (Targets& t) noexcept
    {
        return &t.enabled;
    }

    inline const float* fieldData (const Targets& t) noexcept
    {
        return &t.enabled;
    }

    inline Targets defaults() noexcept
    {
        Targets t;
        t.enabled = 1.0f;
        t.tune = 55.0f;
        t.semi = 0.0f;
        t.fine = 0.0f;
        t.a4 = 440.0f;
        t.track = 0.0f;
        t.voice = 0.0f;
        t.channel = 0.0f;
        t.bodyOn = 1.0f;
        t.bodyWave = 0.0f;
        t.bodyLevel = 0.84f;
        t.bodyHarm = 0.25f;
        t.bodyPol = 0.0f;
        t.impactSt = 36.0f;
        t.impactMs = 7.0f;
        t.impactShape = 0.0f;
        t.impactCurve = 0.15f;
        t.bodySt = 14.0f;
        t.bodyMs = 90.0f;
        t.bodyShape = 0.0f;
        t.bodyCurve = 0.0f;
        t.attack = 0.6f;
        t.decay = 260.0f;
        t.sustain = 0.0f;
        t.release = 40.0f;
        t.phaseReset = 1.0f;
        t.velAmt = 0.85f;
        t.velClick = 0.45f;
        t.velDrive = 0.15f;
        t.velPitch = 0.10f;
        t.subOn = 1.0f;
        t.subLevel = 0.62f;
        t.subHz = 55.0f;
        t.subTrack = 1.0f;
        t.subSemi = 0.0f;
        t.subAttack = 0.4f;
        t.subDecay = 280.0f;
        t.subPhase = 0.0f;
        t.subHarm = 0.08f;
        t.clickOn = 1.0f;
        t.clickLevel = 0.42f;
        t.clickType = 0.0f;
        t.clickMs = 4.0f;
        t.clickHz = 3200.0f;
        t.clickBright = 0.55f;
        t.clickHp = 900.0f;
        t.clickShape = 0.40f;
        t.clickPos = 0.0f;
        t.layerOn = 0.0f;
        t.layerLevel = 0.50f;
        t.layerStart = 0.0f;
        t.layerEnd = 1.0f;
        t.layerSemi = 0.0f;
        t.layerAttack = 0.0f;
        t.layerDecay = 200.0f;
        t.layerPol = 0.0f;
        t.layerPos = 0.0f;
        t.drive = 0.18f;
        t.evenOdd = 0.0f;
        t.satMode = 0.0f;
        t.fold = 0.10f;
        t.asym = 0.04f;
        t.tone = 0.10f;
        t.cutoff = 14000.0f;
        t.hp = 28.0f;
        t.punch = 0.35f;
        t.atkEmph = 0.22f;
        t.susShape = 0.0f;
        t.clickEnh = 0.20f;
        t.comp = 0.0f;
        t.thresh = -16.0f;
        t.ratio = 3.0f;
        t.compA = 2.0f;
        t.compR = 80.0f;
        t.makeup = 0.0f;
        t.clip = 0.0f;
        t.ceiling = -0.3f;
        t.clipHard = 0.0f;
        t.output = -1.5f;
        t.morph = 0.0f;
        t.followSub = 0.0f;
        t.followDrive = 0.0f;
        t.followLevel = 0.0f;
        t.followRes = 0.0f;
        t.followMs = 480.0f;
        t.followCurve = 0.0f;
        t.followShape = 1.0f;
        t.followT1 = 0.28f;
        t.followT2 = 0.62f;
        t.followSt0 = 0.0f;
        t.followSt1 = 0.0f;
        t.followSt2 = 0.0f;
        t.followSt3 = 0.0f;
        t.followAmp0 = 0.0f;
        t.followAmp1 = 0.0f;
        t.followAmp2 = 0.0f;
        t.followAmp3 = 0.0f;
        t.followAmpT1 = 0.28f;
        t.followAmpT2 = 0.62f;
        t.followAmpUse = 0.0f;
        t.noteLo = 24.0f;
        t.noteHi = 36.0f;
        t.keyAmount = 0.0f;
        return t;
    }

    inline void copyChanged (Targets& dest, const Targets& now, const Targets& prev) noexcept
    {
        float* d = fieldData (dest);
        const float* n = fieldData (now);
        const float* p = fieldData (prev);
        for (int i = 0; i < kFieldCount; ++i)
            if (std::memcmp (n + i, p + i, sizeof (float)) != 0)
                d[i] = n[i];
    }

    inline Targets lerpTargets (const Targets& a, const Targets& b, float morph) noexcept
    {
        morph = clampf (morph, 0.0f, 1.0f);
        Targets out;
        const float* af = fieldData (a);
        const float* bf = fieldData (b);
        float* of = fieldData (out);
        int index = 0;
#define ATRIUM_KICK_BLEND(name, title, unit, lo, hi, centre, step, kind, log) \
        of[index] = (log) ? logLerp (af[index], bf[index], morph) : lerpf (af[index], bf[index], morph); \
        ++index;
        ATRIUM_KICK_FIELDS(ATRIUM_KICK_BLEND)
#undef ATRIUM_KICK_BLEND
        auto snapNote = [] (float value) noexcept
        {
            return (float) std::max (0, std::min (127, (int) std::lround (sanef (value))));
        };
        out.noteLo = snapNote (out.noteLo);
        out.noteHi = snapNote (out.noteHi);
        return out;
    }

    // Inclusive keyboard span. An inverted pair is swapped so the range stays playable.
    inline void midiNoteSpan (float low, float high, int& lo, int& hi) noexcept
    {
        lo = (int) std::lround (sanef (low));
        hi = (int) std::lround (sanef (high));
        lo = std::max (0, std::min (127, lo));
        hi = std::max (0, std::min (127, hi));
        if (lo > hi)
            std::swap (lo, hi);
    }

    inline bool noteInsideRange (const Targets& t, int note) noexcept
    {
        int lo = 0, hi = 127;
        midiNoteSpan (t.noteLo, t.noteHi, lo, hi);
        return note >= lo && note <= hi;
    }

    inline float fundamentalHz (const Targets& t) noexcept
    {
        const float a4 = clampf (t.a4, 420.0f, 460.0f);
        float hz = clampf (t.tune, 20.0f, 250.0f) * std::exp2 ((t.semi + t.fine / 100.0f) / 12.0f);
        hz *= a4 / 440.0f;
        return clampf (sanef (hz), 10.0f, 500.0f);
    }

    inline float hzFromMidi (int note, float a4) noexcept
    {
        a4 = clampf (a4, 1.0f, 20000.0f);
        return a4 * std::pow (2.0f, ((float) note - 69.0f) / 12.0f);
    }

    inline int midiFromHz (float hz, float a4) noexcept
    {
        hz = std::max (1.0f, hz);
        a4 = std::max (1.0f, a4);
        return (int) std::lround (69.0 + 12.0 * std::log2 ((double) hz / (double) a4));
    }

    inline void formatNote (int midi, char* text, int size) noexcept
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        if (text == nullptr || size < 4)
            return;
        midi = std::max (0, std::min (127, midi));
        const int pc = midi % 12;
        const int octave = midi / 12 - 1;
        std::snprintf (text, (size_t) size, "%s%d", names[pc], octave);
    }

    // 1 at the start of the stage, 0 once the stage has elapsed.
    inline float stageEnv (float timeSec, float durSec, int shape, float curve) noexcept
    {
        if (! (durSec > 1.0e-5f) || timeSec >= durSec)
            return 0.0f;
        if (timeSec <= 0.0f)
            return 1.0f;
        const float u = clampf (timeSec / durSec, 0.0f, 1.0f);
        float y = 0.0f;
        if (shape == 1)
        {
            y = 1.0f - u;
        }
        else if (shape == 2)
        {
            const float p = std::max (0.25f, std::pow (2.0f, clampf (curve, -1.0f, 1.0f) * 2.5f));
            y = std::pow (1.0f - u, p);
        }
        else
        {
            const float k = std::exp (1.15f + clampf (curve, -1.0f, 1.0f) * 1.35f);
            const float end = std::exp (-k);
            y = (std::exp (-k * u) - end) / (1.0f - end);
        }
        return clampf (sanef (y), 0.0f, 1.0f);
    }

    inline float pitchSemis (const Targets& t, float timeSec) noexcept
    {
        const float impact = stageEnv (timeSec, t.impactMs * 0.001f, (int) std::lround (t.impactShape), t.impactCurve);
        const float body = stageEnv (timeSec, t.bodyMs * 0.001f, (int) std::lround (t.bodyShape), t.bodyCurve);
        return t.impactSt * impact + t.bodySt * body;
    }

    inline float pitchHz (const Targets& t, float timeSec, float fundamental) noexcept
    {
        const float hz = fundamental * std::exp2 (pitchSemis (t, timeSec) / 12.0f);
        return clampf (sanef (hz), 8.0f, 14000.0f);
    }

    // Post-impact window. Shape matches the body pitch stages: 0 exp, 1 linear, 2 curve.
    // Pitch points are semitone offsets. Amp points are a 0..1 level envelope.
    // followAmpUse stays off until the amp lane is edited, so older curves keep
    // driving level, drive, and harmonics from the pitch shape.
    inline void followAnchorsFrom (float t1, float t2, float& a, float& b) noexcept
    {
        a = clampf (t1, 0.05f, 0.80f);
        b = clampf (t2, 0.12f, 0.96f);
        if (b < a + 0.04f)
            b = std::min (0.97f, a + 0.04f);
        if (a > b - 0.04f)
            a = std::max (0.05f, b - 0.04f);
    }

    inline void followAnchors (const Targets& t, float& a, float& b) noexcept
    {
        followAnchorsFrom (t.followT1, t.followT2, a, b);
    }

    inline void followAmpAnchors (const Targets& t, float& a, float& b) noexcept
    {
        followAnchorsFrom (t.followAmpT1, t.followAmpT2, a, b);
    }

    inline float followBend (float x, int shape, float curve) noexcept
    {
        x = clampf (x, 0.0f, 1.0f);
        const float c = clampf (curve, -1.0f, 1.0f);
        if (shape == 1)
        {
            if (std::abs (c) < 0.001f)
                return x;
            return std::pow (x, std::exp (c * 0.85f));
        }
        if (shape == 2)
        {
            const float p = std::max (0.30f, std::pow (2.0f, c * 1.8f));
            return std::pow (x, p);
        }
        const float k = std::exp (1.05f + c * 1.15f);
        const float end = std::exp (-k);
        const float denom = std::max (1.0e-6f, 1.0f - end);
        return clampf ((1.0f - std::exp (-k * x)) / denom, 0.0f, 1.0f);
    }

    inline float followPiece (float timeAfterSec, float spanMs, float t1, float t2, int shape, float curve,
                              float s0, float s1, float s2, float s3) noexcept
    {
        const float span = std::max (0.005f, spanMs * 0.001f);
        const float u = clampf (timeAfterSec <= 0.0f ? 0.0f : timeAfterSec / span, 0.0f, 1.0f);
        float a = 0.0f, b = 0.0f;
        followAnchorsFrom (t1, t2, a, b);
        auto piece = [&] (float p, float q, float u0, float u1) noexcept
        {
            const float x = followBend ((u - u0) / std::max (1.0e-4f, u1 - u0), shape, curve);
            return lerpf (p, q, x);
        };
        float y = s3;
        if (u <= a)
            y = piece (s0, s1, 0.0f, a);
        else if (u <= b)
            y = piece (s1, s2, a, b);
        else
            y = piece (s2, s3, b, 1.0f);
        return sanef (y);
    }

    inline float followSemis (const Targets& t, float timeAfterSec, int shape) noexcept
    {
        return clampf (followPiece (timeAfterSec, t.followMs, t.followT1, t.followT2, shape, t.followCurve,
                                    clampf (t.followSt0, -24.0f, 24.0f), clampf (t.followSt1, -24.0f, 24.0f),
                                    clampf (t.followSt2, -24.0f, 24.0f), clampf (t.followSt3, -24.0f, 24.0f)),
                       -24.0f, 24.0f);
    }

    inline float followAmp (const Targets& t, float timeAfterSec, int shape) noexcept
    {
        return clampf (followPiece (timeAfterSec, t.followMs, t.followAmpT1, t.followAmpT2, shape, t.followCurve,
                                    clampf (t.followAmp0, 0.0f, 1.0f), clampf (t.followAmp1, 0.0f, 1.0f),
                                    clampf (t.followAmp2, 0.0f, 1.0f), clampf (t.followAmp3, 0.0f, 1.0f)),
                       0.0f, 1.0f);
    }

    inline float subFundamentalHz (const Targets& t, float fund) noexcept
    {
        const float base = (t.subTrack >= 0.5f ? fund : clampf (t.subHz, 20.0f, 160.0f))
                         * std::exp2 (clampf (t.subSemi, -24.0f, 24.0f) / 12.0f);
        return clampf (sanef (base), 10.0f, 400.0f);
    }

    // One-shot when sustain is ~0. Otherwise holds, then shows a release after a short hold.
    inline float ampEnvelope (const Targets& t, float timeSec) noexcept
    {
        const float attack = std::max (0.0f, t.attack * 0.001f);
        const float decay = std::max (0.001f, t.decay * 0.001f);
        const float sustain = clampf (t.sustain, 0.0f, 1.0f);
        if (timeSec <= 0.0f)
            return attack <= 1.0e-6f ? 1.0f : 0.0f;
        if (timeSec < attack)
            return attack <= 1.0e-6f ? 1.0f : timeSec / attack;
        const float u = (timeSec - attack) / decay;
        if (sustain <= 0.001f)
            return u >= 1.0f ? 0.0f : std::pow (0.001f, u);
        if (u < 1.0f)
            return sustain + (1.0f - sustain) * std::pow (0.001f, u);
        const float hold = attack + decay + 0.12f;
        if (timeSec < hold)
            return sustain;
        const float rel = std::max (0.001f, t.release * 0.001f);
        const float ru = (timeSec - hold) / rel;
        if (ru >= 1.0f)
            return 0.0f;
        return sustain * std::pow (0.001f, ru);
    }

    inline float playedFundamental (const Targets& t, int midiNote) noexcept
    {
        const float base = fundamentalHz (t);
        if (t.track < 0.5f)
            return base;
        const int root = midiFromHz (base, t.a4);
        return clampf (base * std::exp2 (((float) midiNote - (float) root) / 12.0f), 10.0f, 2000.0f);
    }

    // 0 at noteLo, 1 at noteHi. A single-note range stays at the low end of the map.
    inline float noteRangeT (const Targets& t, int note) noexcept
    {
        int lo = 0, hi = 0;
        midiNoteSpan (t.noteLo, t.noteHi, lo, hi);
        if (hi <= lo)
            return 0.0f;
        return clampf (((float) note - (float) lo) / ((float) hi - (float) lo), 0.0f, 1.0f);
    }

    // Offsets captured at note-on. keyAmount 0 is identity. At 1 the low note is
    // −12 st / ×1.8 decay and the high note is +12 st / ×0.4 decay, with punch,
    // click, drive, sub, and morph spread around the dials. Track keeps its own
    // transpose, so the tune offset stays off while track is on.
    struct KeyOffsets
    {
        float tuneSemis = 0.0f;
        float decayMul = 1.0f;
        float punch = 0.0f;
        float click = 0.0f;
        float drive = 0.0f;
        float sub = 0.0f;
        float morph = 0.0f;
    };

    inline KeyOffsets keyOffsets (const Targets& t, int note) noexcept
    {
        KeyOffsets o;
        const float amount = clampf (sanef (t.keyAmount), 0.0f, 1.0f);
        if (! (amount > 0.0f))
            return o;
        const float u = noteRangeT (t, note);
        if (t.track < 0.5f)
            o.tuneSemis = (u * 2.0f - 1.0f) * 12.0f * amount;
        const float fullDecay = lerpf (1.8f, 0.4f, u);
        o.decayMul = 1.0f + (fullDecay - 1.0f) * amount;
        o.punch = u * 0.55f * amount;
        o.click = u * 0.45f * amount;
        o.drive = u * 0.50f * amount;
        o.sub = -u * 0.40f * amount;
        o.morph = (u - 0.5f) * 0.5f * amount;
        return o;
    }

    inline float keyedFundamental (const Targets& t, int note, float tuneSemis) noexcept
    {
        if (tuneSemis == 0.0f)
            return playedFundamental (t, note);
        return clampf (sanef (fundamentalHz (t) * std::exp2 (tuneSemis / 12.0f)), 10.0f, 2000.0f);
    }

    inline float keyedDecayMs (float decayMs, float mul) noexcept
    {
        if (mul == 1.0f)
            return decayMs;
        return clampf (decayMs * mul, 15.0f, 4000.0f);
    }

    inline constexpr const char* kIdentifier = "atrium:spher8-kick";
    inline constexpr int kPresetCount = 39;

    const char* presetName (int index) noexcept;
    const char* presetCategory (int index) noexcept;
    Targets presetTargets (int index) noexcept;

    inline constexpr const char* kWaveNames[] = { "Sine", "Tri", "Sat", "Analogue", "PM", "Harm" };
    inline constexpr const char* kShapeNames[] = { "Exp", "Lin", "Curve" };
    inline constexpr const char* kClickNames[] = { "White", "Pink", "Sine", "Impulse", "Metal", "Band", "Sample" };
    inline constexpr const char* kSatNames[] = { "Soft", "Tube", "Tape", "Hard", "Fold" };
    inline constexpr const char* kVoiceNames[] = { "Retrigger", "Overlap" };
    inline constexpr int kWaveCount = 6;
    inline constexpr int kShapeCount = 3;
    inline constexpr int kClickCount = 7;
    inline constexpr int kSatCount = 5;
    inline constexpr int kVoiceCount = 2;

    inline const char* const* choiceNames (Field field, int& count) noexcept
    {
        if (field == bodyWave)             { count = kWaveCount;  return kWaveNames; }
        if (field == impactShape || field == bodyShape || field == followShape)
                                           { count = kShapeCount; return kShapeNames; }
        if (field == clickType)            { count = kClickCount; return kClickNames; }
        if (field == satMode)              { count = kSatCount;   return kSatNames; }
        if (field == voice)                { count = kVoiceCount; return kVoiceNames; }
        count = 0;
        return nullptr;
    }
}
