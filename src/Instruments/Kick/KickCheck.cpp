#include "KickEngine.h"
#include "KickInstrument.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
    int fails = 0;

    void expect (bool ok, const std::string& message)
    {
        if (ok)
            return;
        std::cerr << "FAIL " << message << "\n";
        ++fails;
    }

    atrium::kick::Targets clean()
    {
        auto t = atrium::kick::defaults();
        t.impactSt = 0.0f;
        t.bodySt = 0.0f;
        t.clickOn = 0.0f;
        t.clickLevel = 0.0f;
        t.subOn = 0.0f;
        t.subLevel = 0.0f;
        t.drive = 0.0f;
        t.fold = 0.0f;
        t.asym = 0.0f;
        t.evenOdd = 0.0f;
        t.punch = 0.0f;
        t.atkEmph = 0.0f;
        t.comp = 0.0f;
        t.clip = 0.0f;
        t.tone = 0.0f;
        t.hp = 15.0f;
        t.cutoff = 18000.0f;
        t.output = 0.0f;
        t.attack = 0.0f;
        t.decay = 1800.0f;
        t.susShape = 0.0f;
        t.clickEnh = 0.0f;
        t.velAmt = 1.0f;
        t.velPitch = 0.0f;
        t.velDrive = 0.0f;
        t.velClick = 0.0f;
        t.bodyLevel = 0.8f;
        t.tune = 55.0f;
        t.phaseReset = 1.0f;
        t.enabled = 1.0f;
        t.satMode = 0.0f;
        t.bodyWave = 0.0f;
        t.morph = 0.0f;
        return t;
    }

    std::vector<float> render (const atrium::kick::Targets& live, int note, float velocity,
                               int samples, double rate, int block, int noteAt = 0,
                               const atrium::kick::Targets* snapB = nullptr,
                               const atrium::kick::Hit* extra = nullptr, int nExtra = 0,
                               atrium::kick::SampleView sample = {},
                               atrium::kick::Probes* probes = nullptr)
    {
        atrium::kick::Engine engine;
        engine.prepare (rate, block);
        engine.setSnapshots (live, snapB != nullptr ? *snapB : live);
        std::vector<float> out ((size_t) samples, 0.0f);
        int done = 0;
        bool armedNote = false;
        while (done < samples)
        {
            const int n = std::min (block, samples - done);
            atrium::kick::Hit hits[8];
            int nHits = 0;
            if (! armedNote && noteAt >= done && noteAt < done + n)
            {
                hits[nHits++] = { noteAt - done, 1, note, velocity, 1 };
                armedNote = true;
            }
            for (int i = 0; i < nExtra && nHits < 8; ++i)
                if (extra[i].offset >= done && extra[i].offset < done + n)
                {
                    hits[nHits] = extra[i];
                    hits[nHits].offset -= done;
                    ++nHits;
                }
            float* channel = out.data() + done;
            float* outputs[1] = { channel };
            engine.process (outputs, 1, n, live, hits, nHits, sample);
            done += n;
        }
        if (probes != nullptr)
            *probes = engine.probes();
        return out;
    }

    float peakOf (const std::vector<float>& x)
    {
        float peak = 0.0f;
        for (float s : x)
            peak = std::max (peak, std::abs (s));
        return peak;
    }

    float rmsOf (const std::vector<float>& x, int from, int to)
    {
        from = std::max (0, from);
        to = std::min ((int) x.size(), to);
        if (to <= from)
            return 0.0f;
        double sum = 0.0;
        for (int i = from; i < to; ++i)
            sum += (double) x[(size_t) i] * (double) x[(size_t) i];
        return (float) std::sqrt (sum / (double) (to - from));
    }

    bool finiteAudio (const std::vector<float>& x)
    {
        for (float s : x)
            if (! std::isfinite (s))
                return false;
        return true;
    }

    float goertzel (const std::vector<float>& x, int from, int to, float hz, double rate)
    {
        const float w = 6.28318530718f * hz / (float) rate;
        const float coeff = 2.0f * std::cos (w);
        float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
        for (int i = from; i < to && i < (int) x.size(); ++i)
        {
            s0 = x[(size_t) i] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return s1 * s1 + s2 * s2 - coeff * s1 * s2;
    }

    float dominantNear (const std::vector<float>& x, int from, int to, float expected, double rate)
    {
        float bestHz = expected;
        float best = -1.0f;
        for (int step = -20; step <= 20; ++step)
        {
            const float hz = expected + (float) step * 0.25f;
            const float power = goertzel (x, from, to, hz, rate);
            if (power > best)
            {
                best = power;
                bestHz = hz;
            }
        }
        return bestHz;
    }

    float dominantRange (const std::vector<float>& x, int from, int to, float lo, float hi, double rate)
    {
        float bestHz = lo;
        float best = -1.0f;
        for (float hz = lo; hz <= hi; hz += 0.5f)
        {
            const float power = goertzel (x, from, to, hz, rate);
            if (power > best)
            {
                best = power;
                bestHz = hz;
            }
        }
        return bestHz;
    }
}

int main()
{
    using namespace atrium::kick;
    juce::ScopedJuceInitialiser_GUI gui;

    {
        auto silent = render (clean(), 36, 1.0f, 2048, 48000.0, 256);
        silent = render (clean(), 36, 0.0f, 512, 48000.0, 128);
        expect (peakOf (silent) < 1.0e-4f, "velocity 0 must not trigger");
        auto filtered = clean();
        filtered.inputNote = 37.0f;
        filtered.noteLo = 0.0f;
        filtered.noteHi = 127.0f;
        auto other = render (filtered, 38, 1.0f, 4000, 48000.0, 256);
        expect (peakOf (other) < 1.0e-4f, "input note ignores notes other than 36");
        auto kept = render (filtered, 36, 1.0f, 4000, 48000.0, 256);
        expect (peakOf (kept) > 0.02f, "input note 36 still plays");
        Hit own { 0, 1, 50, 1.0f, 0 };
        auto bypass = render (filtered, 50, 0.0f, 4000, 48000.0, 256, 0, nullptr, &own, 1);
        expect (peakOf (bypass) > 0.02f, "trigger audition bypasses the input note");
    }

    {
        auto span = clean();
        expect (peakOf (render (span, 36, 1.0f, 4000, 48000.0, 256)) > 0.02f, "note 36 triggers inside the default range");
        expect (peakOf (render (span, 24, 1.0f, 4000, 48000.0, 256)) > 0.02f, "note 24 triggers inside the default range");
        expect (peakOf (render (span, 40, 1.0f, 4000, 48000.0, 256)) < 1.0e-4f, "note 40 is silent outside the default range");
        span.noteLo = 0.0f;
        span.noteHi = 127.0f;
        expect (peakOf (render (span, 40, 1.0f, 4000, 48000.0, 256)) > 0.02f, "full range restores note 40");
        span.noteLo = 40.0f;
        span.noteHi = 24.0f;
        expect (peakOf (render (span, 30, 1.0f, 4000, 48000.0, 256)) > 0.02f, "inverted note range still plays");
        expect (peakOf (render (span, 20, 1.0f, 4000, 48000.0, 256)) < 1.0e-4f, "inverted note range still rejects notes outside it");
        auto held = clean();
        held.sustain = 0.75f;
        held.attack = 1.0f;
        held.decay = 30.0f;
        held.release = 30.0f;
        Hit stray { (int) std::lround (0.12 * 48000.0), 0, 40, 0.0f, 1 };
        auto keptHold = render (held, 36, 1.0f, 16000, 48000.0, 256, 0, nullptr, &stray, 1);
        const float duringHold = rmsOf (keptHold, 2000, 5000);
        const float afterHold = rmsOf (keptHold, 11000, 15000);
        expect (duringHold > 0.02f && afterHold > duringHold * 0.5f, "note-off outside the range does not release the kick");
    }

    auto tone = clean();
    auto body = render (tone, 36, 1.0f, 24000, 48000.0, 256);
    expect (finiteAudio (body), "body audio is finite");
    expect (peakOf (body) > 0.05f, "body produces a kick");
    const float measured = dominantNear (body, 8000, 16000, 55.0f, 48000.0);
    expect (std::abs (measured - 55.0f) < 1.0f, "fundamental " + std::to_string (measured));

    Probes probes;
    auto swept = defaults();
    swept.drive = 0.0f;
    swept.clip = 0.0f;
    swept.comp = 0.0f;
    swept.punch = 0.0f;
    swept.atkEmph = 0.0f;
    swept.clickOn = 0.0f;
    swept.subOn = 0.0f;
    swept.output = 0.0f;
    swept.hp = 15.0f;
    swept.cutoff = 18000.0f;
    auto sweepAudio = render (swept, 36, 1.0f, 12000, 48000.0, 128, 0, nullptr, nullptr, 0, {}, &probes);
    expect (finiteAudio (sweepAudio), "sweep finite");
    const float expectStart = pitchHz (swept, 0.0f, fundamentalHz (swept));
    expect (probes.startHz > fundamentalHz (swept) * 4.0f, "start pitch is above the fundamental");
    expect (std::abs (probes.startHz - expectStart) < 1.5f, "start pitch matches the envelope");
    expect (probes.lateHz > 20.0f && probes.lateHz < expectStart * 0.5f, "pitch has fallen by 180 ms");

    auto again = render (tone, 36, 1.0f, 4096, 48000.0, 256);
    auto once = render (tone, 36, 1.0f, 4096, 48000.0, 256);
    float drift = 0.0f;
    for (size_t i = 0; i < once.size(); ++i)
        drift = std::max (drift, std::abs (once[i] - again[i]));
    expect (drift < 1.0e-5f, "phase reset is deterministic");

    auto coarse = render (tone, 36, 1.0f, 4096, 48000.0, 64);
    auto fine = render (tone, 36, 1.0f, 4096, 48000.0, 512);
    float blockDrift = 0.0f;
    for (size_t i = 0; i < coarse.size(); ++i)
        blockDrift = std::max (blockDrift, std::abs (coarse[i] - fine[i]));
    expect (blockDrift < 1.0e-4f, "buffer size changes the oscillator");

    for (double rate : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        const int n = (int) std::lround (rate * 0.4);
        auto audio = render (tone, 36, 1.0f, n, rate, 128);
        expect (finiteAudio (audio), "finite at " + std::to_string (rate));
        const int a = (int) std::lround (rate * 0.12);
        const int b = (int) std::lround (rate * 0.28);
        const float hz = dominantNear (audio, a, b, 55.0f, rate);
        expect (std::abs (hz - 55.0f) < 1.25f, "rate " + std::to_string (rate) + " hz " + std::to_string (hz));
    }

    auto sub = tone;
    sub.subOn = 1.0f;
    sub.subTrack = 1.0f;
    sub.subLevel = tone.bodyLevel;
    sub.subDecay = tone.decay;
    sub.subAttack = 0.0f;
    sub.subPhase = 0.5f;
    sub.subHarm = 0.0f;
    auto cancelled = render (sub, 36, 1.0f, 8000, 48000.0, 256);
    expect (rmsOf (cancelled, 1000, 5000) < rmsOf (body, 1000, 5000) * 0.25f, "inverted sub cancels the body");

    auto whoop = clean();
    whoop.bodyOn = 0.0f;
    whoop.bodyLevel = 0.0f;
    whoop.clickOn = 0.0f;
    whoop.clickLevel = 0.0f;
    whoop.subOn = 1.0f;
    whoop.subLevel = 0.95f;
    whoop.subTrack = 1.0f;
    whoop.subHarm = 0.0f;
    whoop.subAttack = 0.0f;
    whoop.subDecay = 2000.0f;
    whoop.decay = 2000.0f;
    whoop.tune = 55.0f;
    whoop.impactMs = 10.0f;
    whoop.impactSt = 0.0f;
    whoop.bodySt = 0.0f;
    whoop.followMs = 600.0f;
    whoop.followShape = 1.0f;
    whoop.followCurve = 0.0f;
    whoop.followT1 = 0.30f;
    whoop.followT2 = 0.70f;
    whoop.followSt0 = 0.0f;
    whoop.followSt1 = 12.0f;
    whoop.followSt2 = -4.0f;
    whoop.followSt3 = 7.0f;
    whoop.followSub = 0.0f;
    whoop.followDrive = 0.0f;
    whoop.followLevel = 0.0f;
    whoop.followRes = 0.0f;
    whoop.drive = 0.0f;
    whoop.fold = 0.0f;
    whoop.hp = 15.0f;
    whoop.cutoff = 18000.0f;
    auto whoopStill = render (whoop, 36, 1.0f, 24000, 48000.0, 256);
    const float stillEarly = dominantRange (whoopStill, 1400, 3600, 40.0f, 90.0f, 48000.0);
    const float stillLate = dominantRange (whoopStill, 8600, 12000, 40.0f, 140.0f, 48000.0);
    expect (std::abs (stillEarly - 55.0f) < 2.0f && std::abs (stillLate - 55.0f) < 2.0f,
            "follow depth 0 does not move the sub");
    whoop.followSub = 1.0f;
    auto whoopMove = render (whoop, 36, 1.0f, 24000, 48000.0, 256);
    expect (finiteAudio (whoopMove), "whoop audio is finite");
    const float moveEarly = dominantRange (whoopMove, 1400, 3200, 40.0f, 100.0f, 48000.0);
    const float moveLate = dominantRange (whoopMove, 8600, 11500, 70.0f, 150.0f, 48000.0);
    expect (moveLate > moveEarly + 8.0f, "follow depth moves the sub pitch " + std::to_string (moveEarly) + " -> " + std::to_string (moveLate));

    auto driveFollow = whoop;
    driveFollow.followSub = 0.0f;
    driveFollow.followRes = 0.0f;
    driveFollow.followLevel = 0.0f;
    driveFollow.drive = 0.28f;
    driveFollow.satMode = 0.0f;
    driveFollow.followDrive = 0.0f;
    auto driveFlat = render (driveFollow, 36, 1.0f, 24000, 48000.0, 256);
    driveFollow.followDrive = 1.0f;
    auto driveWhoop = render (driveFollow, 36, 1.0f, 24000, 48000.0, 256);
    const float dryF = std::max (1.0e-8f, goertzel (driveFlat, 8600, 11500, 55.0f, 48000.0));
    const float dryH = goertzel (driveFlat, 8600, 11500, 165.0f, 48000.0);
    const float wetF = std::max (1.0e-8f, goertzel (driveWhoop, 8600, 11500, 55.0f, 48000.0));
    const float wetH = goertzel (driveWhoop, 8600, 11500, 165.0f, 48000.0);
    expect (wetH / wetF > dryH / dryF * 1.35f, "drive follow raises harmonics on the whoop");
    expect (peakOf (driveWhoop) < 1.6f && finiteAudio (driveWhoop), "drive follow stays bounded");

    auto post = whoop;
    post.followSub = 0.0f;
    post.followDrive = 0.0f;
    post.followRes = 0.0f;
    post.followLevel = 0.0f;
    post.followAmpUse = 1.0f;
    post.followAmp0 = 0.0f;
    post.followAmp1 = 1.0f;
    post.followAmp2 = 0.15f;
    post.followAmp3 = 0.0f;
    post.followAmpT1 = 0.30f;
    post.followAmpT2 = 0.70f;
    post.followSt0 = 0.0f;
    post.followSt1 = 10.0f;
    post.followSt2 = -6.0f;
    post.followSt3 = 4.0f;
    auto ampOff = render (post, 36, 1.0f, 24000, 48000.0, 256);
    const float offEarly = dominantRange (ampOff, 1400, 3600, 40.0f, 90.0f, 48000.0);
    const float offLate = dominantRange (ampOff, 8600, 12000, 40.0f, 140.0f, 48000.0);
    expect (std::abs (offEarly - 55.0f) < 2.0f && std::abs (offLate - 55.0f) < 2.0f,
            "pitch depth 0 keeps the sub still");
    const float quiet = rmsOf (ampOff, 7800, 12000);
    post.followLevel = 1.0f;
    auto ampOn = render (post, 36, 1.0f, 24000, 48000.0, 256);
    const float boosted = rmsOf (ampOn, 7800, 12000);
    const float onEarly = dominantRange (ampOn, 1400, 3600, 40.0f, 90.0f, 48000.0);
    const float onLate = dominantRange (ampOn, 8600, 12000, 40.0f, 140.0f, 48000.0);
    expect (boosted > quiet * 1.25f, "post amp depth boosts the sub " + std::to_string (quiet) + " -> " + std::to_string (boosted));
    expect (std::abs (onEarly - 55.0f) < 2.5f && std::abs (onLate - 55.0f) < 2.5f,
            "post amp boost does not move pitch");
    expect (finiteAudio (ampOn) && peakOf (ampOn) < 1.6f, "post amp boost stays bounded");
    post.followLevel = 0.0f;
    post.followSub = 1.0f;
    auto pitched = render (post, 36, 1.0f, 24000, 48000.0, 256);
    const float pitEarly = dominantRange (pitched, 1400, 3200, 40.0f, 100.0f, 48000.0);
    const float pitLate = dominantRange (pitched, 8600, 11500, 70.0f, 160.0f, 48000.0);
    expect (pitLate > pitEarly + 8.0f, "pitch depth still moves the sub " + std::to_string (pitEarly) + " -> " + std::to_string (pitLate));
    auto ampDrive = post;
    ampDrive.followSub = 0.0f;
    ampDrive.followLevel = 0.0f;
    ampDrive.drive = 0.28f;
    ampDrive.followDrive = 0.0f;
    auto ampDry = render (ampDrive, 36, 1.0f, 24000, 48000.0, 256);
    ampDrive.followDrive = 1.0f;
    auto ampWet = render (ampDrive, 36, 1.0f, 24000, 48000.0, 256);
    const float ampDryF = std::max (1.0e-8f, goertzel (ampDry, 8600, 11500, 55.0f, 48000.0));
    const float ampDryH = goertzel (ampDry, 8600, 11500, 165.0f, 48000.0);
    const float ampWetF = std::max (1.0e-8f, goertzel (ampWet, 8600, 11500, 55.0f, 48000.0));
    const float ampWetH = goertzel (ampWet, 8600, 11500, 165.0f, 48000.0);
    expect (ampWetH / ampWetF > ampDryH / ampDryF * 1.35f, "drive follows the post amp envelope");

    auto click = clean();
    click.bodyOn = 0.0f;
    click.bodyLevel = 0.0f;
    click.clickOn = 1.0f;
    click.clickLevel = 1.0f;
    click.clickType = 3.0f;
    click.clickMs = 8.0f;
    click.clickPos = 0.0f;
    click.output = 0.0f;
    auto early = render (click, 36, 1.0f, 2000, 48000.0, 128);
    click.clickPos = 8.0f;
    auto late = render (click, 36, 1.0f, 2000, 48000.0, 128);
    int earlyAt = 0, lateAt = 0;
    for (int i = 1; i < (int) early.size(); ++i)
    {
        if (std::abs (early[(size_t) i]) > std::abs (early[(size_t) earlyAt])) earlyAt = i;
        if (std::abs (late[(size_t) i]) > std::abs (late[(size_t) lateAt])) lateAt = i;
    }
    expect (std::abs ((lateAt - earlyAt) - (int) std::lround (0.008 * 48000.0)) < 80, "click offset moves the transient");

    auto soft = render (tone, 36, 0.25f, 4000, 48000.0, 256);
    auto hard = render (tone, 36, 1.0f, 4000, 48000.0, 256);
    expect (rmsOf (soft, 0, 2000) < rmsOf (hard, 0, 2000) * 0.55f, "velocity scales amplitude");

    auto tracked = tone;
    tracked.tune = 110.0f;
    tracked.track = 0.0f;
    tracked.noteLo = 45.0f;
    tracked.noteHi = 57.0f;
    auto fixedLow = render (tracked, 45, 1.0f, 12000, 48000.0, 256);
    auto fixedHigh = render (tracked, 57, 1.0f, 12000, 48000.0, 256);
    const float fixedA = dominantNear (fixedLow, 4000, 9000, 110.0f, 48000.0);
    const float fixedB = dominantNear (fixedHigh, 4000, 9000, 110.0f, 48000.0);
    expect (std::abs (fixedA - 110.0f) < 1.5f && std::abs (fixedB - 110.0f) < 1.5f, "fixed pitch ignores the note");
    tracked.track = 1.0f;
    auto follow = render (tracked, 57, 1.0f, 12000, 48000.0, 256);
    const float followed = dominantNear (follow, 4000, 9000, 220.0f, 48000.0);
    expect (std::abs (followed - 220.0f) < 2.0f, "key track transposed to " + std::to_string (followed));

    auto keyed = tone;
    keyed.tune = 55.0f;
    keyed.track = 0.0f;
    keyed.noteLo = 24.0f;
    keyed.noteHi = 36.0f;
    keyed.keyAmount = 0.0f;
    Probes keyLowProbe, keyHighProbe;
    auto keyLow = render (keyed, 24, 1.0f, 20000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &keyLowProbe);
    auto keyHigh = render (keyed, 36, 1.0f, 20000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &keyHighProbe);
    const float keyLowHz = dominantNear (keyLow, 6000, 16000, 55.0f, 48000.0);
    const float keyHighHz = dominantNear (keyHigh, 6000, 16000, 55.0f, 48000.0);
    expect (std::abs (keyLowHz - 55.0f) < 1.5f && std::abs (keyHighHz - 55.0f) < 1.5f,
            "key amount 0 keeps notes 24 and 36 at " + std::to_string (keyLowHz) + " / " + std::to_string (keyHighHz));
    expect (std::abs (keyLowProbe.startHz - keyHighProbe.startHz) < 0.05f, "key amount 0 start pitch matches");

    keyed.keyAmount = 1.0f;
    Probes spreadLowProbe, spreadHighProbe;
    auto spreadLow = render (keyed, 24, 1.0f, 24000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &spreadLowProbe);
    auto spreadHigh = render (keyed, 36, 1.0f, 24000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &spreadHighProbe);
    const float spreadLowHz = dominantNear (spreadLow, 8000, 20000, 27.5f, 48000.0);
    const float spreadHighHz = dominantNear (spreadHigh, 8000, 20000, 110.0f, 48000.0);
    expect (std::abs (spreadLowProbe.startHz - 27.5f) < 0.5f, "low note tune spread " + std::to_string (spreadLowProbe.startHz));
    expect (std::abs (spreadHighProbe.startHz - 110.0f) < 0.5f, "high note tune spread " + std::to_string (spreadHighProbe.startHz));
    expect (std::abs (spreadLowHz - 27.5f) < 2.5f && std::abs (spreadHighHz - 110.0f) < 2.5f,
            "heard tune spread " + std::to_string (spreadLowHz) + " / " + std::to_string (spreadHighHz));
    expect (peakOf (render (keyed, 40, 1.0f, 4000, 48000.0, 256)) < 1.0e-4f, "key amount does not widen the note range");

    auto spreadMap = keyed;
    const auto lowMap = keyOffsets (spreadMap, 24);
    const auto highMap = keyOffsets (spreadMap, 36);
    expect (std::abs (lowMap.tuneSemis + 12.0f) < 0.001f && std::abs (highMap.tuneSemis - 12.0f) < 0.001f, "tune spread is ±12 st");
    expect (std::abs (lowMap.decayMul - 1.8f) < 0.001f && std::abs (highMap.decayMul - 0.4f) < 0.001f, "decay spread is ×1.8 to ×0.4");
    expect (std::abs (highMap.punch - 0.55f) < 0.001f, "punch spread");
    expect (std::abs (highMap.click - 0.45f) < 0.001f, "click spread");
    expect (std::abs (highMap.drive - 0.50f) < 0.001f, "drive spread");
    expect (std::abs (highMap.sub + 0.40f) < 0.001f, "sub spread");
    expect (std::abs (lowMap.morph + 0.25f) < 0.001f && std::abs (highMap.morph - 0.25f) < 0.001f, "morph spread");
    spreadMap.keyAmount = 0.5f;
    const auto halfMap = keyOffsets (spreadMap, 36);
    expect (std::abs (halfMap.tuneSemis - 6.0f) < 0.001f && std::abs (halfMap.decayMul - 0.7f) < 0.001f, "50% is half the offset");
    spreadMap.keyAmount = 1.0f;
    spreadMap.track = 1.0f;
    expect (std::abs (keyOffsets (spreadMap, 36).tuneSemis) < 0.001f
                && std::abs (keyOffsets (spreadMap, 24).decayMul - 1.8f) < 0.001f,
            "track keeps its transpose and still spreads the other dials");
    auto trackedKey = tone;
    trackedKey.tune = 110.0f;
    trackedKey.track = 1.0f;
    trackedKey.keyAmount = 1.0f;
    trackedKey.noteLo = 45.0f;
    trackedKey.noteHi = 57.0f;
    Probes trackRootProbe, trackTopProbe;
    auto trackRoot = render (trackedKey, 45, 1.0f, 16000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &trackRootProbe);
    auto trackTop = render (trackedKey, 57, 1.0f, 16000, 48000.0, 256, 0, nullptr, nullptr, 0, {}, &trackTopProbe);
    const float trackRootHz = dominantNear (trackRoot, 5000, 12000, 110.0f, 48000.0);
    const float trackTopHz = dominantNear (trackTop, 5000, 12000, 220.0f, 48000.0);
    expect (std::abs (trackRootProbe.startHz - 110.0f) < 0.5f && std::abs (trackTopProbe.startHz - 220.0f) < 0.5f,
            "track does not stack on the key spread " + std::to_string (trackRootProbe.startHz) + " / " + std::to_string (trackTopProbe.startHz));
    expect (std::abs (trackRootHz - 110.0f) < 2.5f && std::abs (trackTopHz - 220.0f) < 2.5f,
            "heard track pitches " + std::to_string (trackRootHz) + " / " + std::to_string (trackTopHz));

    auto character = tone;
    character.decay = 280.0f;
    character.clickOn = 1.0f;
    character.clickLevel = 0.20f;
    character.clickType = 2.0f;
    character.clickMs = 14.0f;
    character.clickHz = 4200.0f;
    character.clickHp = 80.0f;
    character.drive = 0.08f;
    character.punch = 0.15f;
    character.noteLo = 24.0f;
    character.noteHi = 36.0f;
    character.track = 0.0f;
    character.keyAmount = 0.0f;
    auto flatLow = render (character, 24, 1.0f, 24000, 48000.0, 256);
    auto flatHigh = render (character, 36, 1.0f, 24000, 48000.0, 256);
    float flatGap = 0.0f;
    for (size_t i = 0; i < flatLow.size() && i < flatHigh.size(); ++i)
        flatGap = std::max (flatGap, std::abs (flatLow[i] - flatHigh[i]));
    const float flatTailLow = rmsOf (flatLow, 8000, 14000);
    const float flatTailHigh = rmsOf (flatHigh, 8000, 14000);
    const float flatClickLow = goertzel (flatLow, 40, 700, 4200.0f, 48000.0);
    const float flatClickHigh = goertzel (flatHigh, 40, 700, 4200.0f, 48000.0);
    expect (flatGap < 1.0e-4f, "key amount 0 renders one kick " + std::to_string (flatGap));
    expect (rmsOf (flatLow, 800, 4000) > 0.02f
                && std::abs (flatTailLow - flatTailHigh) <= std::max (1.0e-5f, flatTailLow) * 0.02f,
            "depth 0 decay does not split");
    expect (std::abs (flatClickLow - flatClickHigh) <= std::max (flatClickLow, flatClickHigh) * 0.02f, "depth 0 brightness does not split");
    character.keyAmount = 1.0f;
    auto longLow = render (character, 24, 1.0f, 24000, 48000.0, 256);
    auto shortHigh = render (character, 36, 1.0f, 24000, 48000.0, 256);
    const float longTail = rmsOf (longLow, 8000, 14000);
    const float shortTail = rmsOf (shortHigh, 8000, 14000);
    const float lowClick = goertzel (longLow, 40, 700, 4200.0f, 48000.0);
    const float highClick = goertzel (shortHigh, 40, 700, 4200.0f, 48000.0);
    expect (longTail > 0.004f && longTail > shortTail * 4.0f,
            "low note decays longer " + std::to_string (longTail) + " / " + std::to_string (shortTail));
    expect (highClick > lowClick * 2.5f, "high note is brighter " + std::to_string (highClick) + " / " + std::to_string (lowClick));

    auto plain = tone;
    auto driven = tone;
    driven.drive = 1.0f;
    driven.satMode = 0.0f;
    auto cleanAudio = render (plain, 36, 1.0f, 12000, 48000.0, 256);
    auto hotAudio = render (driven, 36, 1.0f, 12000, 48000.0, 256);
    const float cleanH = goertzel (cleanAudio, 4000, 10000, 165.0f, 48000.0);
    const float hotH = goertzel (hotAudio, 4000, 10000, 165.0f, 48000.0);
    const float cleanF = std::max (1.0e-8f, goertzel (cleanAudio, 4000, 10000, 55.0f, 48000.0));
    const float hotF = std::max (1.0e-8f, goertzel (hotAudio, 4000, 10000, 55.0f, 48000.0));
    expect (hotH / hotF > cleanH / cleanF * 2.0f, "drive adds the third harmonic");

    auto grit = tone;
    grit.drive = 0.8f;
    grit.satMode = 0.0f;
    auto gritAudio = render (grit, 36, 1.0f, 12000, 48000.0, 256);
    const float gritH = goertzel (gritAudio, 4000, 10000, 165.0f, 48000.0);
    const float gritF = std::max (1.0e-8f, goertzel (gritAudio, 4000, 10000, 55.0f, 48000.0));
    expect (gritH / gritF > cleanH / cleanF * 2.0f, "drive 0.8 raises the third harmonic");
    expect (gritF > cleanF * 0.55f, "drive 0.8 keeps the fundamental");

    auto filtered = tone;
    filtered.hp = 28.0f;
    filtered.cutoff = 14000.0f;
    filtered.tone = 0.10f;
    filtered.fold = 0.10f;
    filtered.drive = 0.0f;
    auto filteredClean = render (filtered, 36, 1.0f, 12000, 48000.0, 256);
    filtered.drive = 0.8f;
    auto filteredHot = render (filtered, 36, 1.0f, 12000, 48000.0, 256);
    const float filtCleanH = goertzel (filteredClean, 4000, 10000, 165.0f, 48000.0);
    const float filtCleanF = std::max (1.0e-8f, goertzel (filteredClean, 4000, 10000, 55.0f, 48000.0));
    const float filtHotH = goertzel (filteredHot, 4000, 10000, 165.0f, 48000.0);
    const float filtHotF = std::max (1.0e-8f, goertzel (filteredHot, 4000, 10000, 55.0f, 48000.0));
    expect (filtHotH / filtHotF > filtCleanH / filtCleanF * 2.0f, "drive 0.8 cuts through the preset filters");
    expect (filtHotF > filtCleanF * 0.55f, "filtered drive keeps the fundamental");

    auto noisy = clean();
    noisy.bodyOn = 0.0f;
    noisy.clickOn = 1.0f;
    noisy.clickLevel = 0.9f;
    noisy.clickType = 0.0f;
    noisy.clickMs = 40.0f;
    noisy.clickHp = 40.0f;
    noisy.cutoff = 500.0f;
    auto dark = render (noisy, 36, 1.0f, 4000, 48000.0, 128);
    noisy.cutoff = 14000.0f;
    auto bright = render (noisy, 36, 1.0f, 4000, 48000.0, 128);
    expect (goertzel (dark, 0, 2000, 4000.0f, 48000.0) < goertzel (bright, 0, 2000, 4000.0f, 48000.0) * 0.5f,
            "low cutoff removes click brightness");

    auto open = tone;
    open.sustain = 1.0f;
    open.decay = 40.0f;
    open.bodyLevel = 0.95f;
    auto uncompressed = render (open, 36, 1.0f, 8000, 48000.0, 256);
    open.comp = 1.0f;
    open.thresh = -30.0f;
    open.ratio = 12.0f;
    open.compA = 0.2f;
    open.makeup = 0.0f;
    auto compressed = render (open, 36, 1.0f, 8000, 48000.0, 256);
    expect (peakOf (compressed) < peakOf (uncompressed) * 0.85f, "compressor reduces the peak");

    auto clipped = tone;
    clipped.bodyLevel = 1.0f;
    clipped.clip = 1.0f;
    clipped.clipHard = 1.0f;
    clipped.ceiling = -6.0f;
    clipped.output = 0.0f;
    auto limited = render (clipped, 36, 1.0f, 4000, 48000.0, 128);
    expect (peakOf (limited) <= std::pow (10.0f, -6.0f / 20.0f) + 0.02f, "hard clip respects the ceiling");

    Targets low = tone;
    Targets high = tone;
    low.tune = 40.0f;
    high.tune = 160.0f;
    low.morph = 0.5f;
    auto morphed = render (low, 36, 1.0f, 12000, 48000.0, 256, 0, &high);
    const float mid = dominantNear (morphed, 4000, 9000, 80.0f, 48000.0);
    expect (std::abs (mid - 80.0f) < 2.0f, "morph frequency is geometric " + std::to_string (mid));
    auto rangeA = clean();
    rangeA.morph = 0.5f;
    auto rangeB = rangeA;
    rangeB.noteLo = 0.0f;
    rangeB.noteHi = 60.0f;
    auto rangeMove = render (rangeA, 36, 1.0f, 8000, 48000.0, 256, 0, &rangeB);
    auto rangeStill = render (rangeA, 36, 1.0f, 8000, 48000.0, 256, 0, &rangeA);
    float rangeDrift = 0.0f;
    for (size_t i = 0; i < rangeMove.size(); ++i)
        rangeDrift = std::max (rangeDrift, std::abs (rangeMove[i] - rangeStill[i]));
    expect (rangeDrift < 1.0e-4f, "changing the note range does not move the audio");
    const float blended = lerpTargets (low, high, 0.5f).tune;
    expect (std::abs (blended - 80.0f) < 0.05f, "log lerp of 40 and 160 Hz");

    Engine crowd;
    crowd.prepare (48000.0, 32);
    auto hammer = clean();
    hammer.voice = 1.0f;
    hammer.decay = 80.0f;
    hammer.noteLo = 36.0f;
    hammer.noteHi = 40.0f;
    bool crowdOk = true;
    for (int n = 0; n < 400; ++n)
    {
        Hit hit { 0, 1, 36 + (n % 5), 0.8f, 1 };
        float frame[32];
        float* outputs[1] = { frame };
        crowd.process (outputs, 1, 32, hammer, &hit, 1, {});
        if (crowd.activeVoices() > Engine::kVoices)
            crowdOk = false;
        for (float s : frame)
            if (! std::isfinite (s))
                crowdOk = false;
    }
    expect (crowdOk, "dense triggering stays bounded and finite");

    std::vector<float> table (1000);
    for (int i = 0; i < (int) table.size(); ++i)
        table[(size_t) i] = std::sin (6.2831853f * 180.0f * (float) i / 48000.0f);
    SampleView sample { table.data(), (int) table.size(), 48000.0f };
    auto layer = clean();
    layer.bodyOn = 0.0f;
    layer.clickOn = 0.0f;
    layer.subOn = 0.0f;
    layer.layerOn = 1.0f;
    layer.layerLevel = 1.0f;
    layer.layerEnd = 1.0f;
    auto layered = render (layer, 36, 1.0f, 3000, 48000.0, 256, 0, nullptr, nullptr, 0, sample);
    expect (peakOf (layered) > 0.05f, "sample layer is audible without replacing synthesis");

    auto held = tone;
    held.sustain = 0.75f;
    held.attack = 1.0f;
    held.decay = 30.0f;
    held.release = 30.0f;
    Hit off { (int) std::lround (0.12 * 48000.0), 0, 36, 0.0f, 1 };
    auto released = render (held, 36, 1.0f, 16000, 48000.0, 256, 0, nullptr, &off, 1);
    const float during = rmsOf (released, 2000, 5000);
    const float after = rmsOf (released, 11000, 15000);
    expect (during > 0.02f && after < during * 0.35f, "note-off releases a sustained kick");

    struct Finger
    {
        float early, mid, tail, edge, low, high;
    };
    std::vector<Finger> fingerprints;
    fingerprints.reserve ((size_t) kPresetCount);
    for (int i = 0; i < kPresetCount; ++i)
    {
        auto preset = presetTargets (i);
        auto audio = render (preset, 36, 1.0f, 12000, 48000.0, 256);
        expect (finiteAudio (audio) && peakOf (audio) > 0.008f, std::string ("preset silent ") + presetName (i));
        double edge = 0.0;
        for (int n = 1; n < 4000 && n < (int) audio.size(); ++n)
        {
            const float d = audio[(size_t) n] - audio[(size_t) (n - 1)];
            edge += (double) d * (double) d;
        }
        Finger mark;
        mark.early = rmsOf (audio, 0, 1500);
        mark.mid = rmsOf (audio, 1500, 6000);
        mark.tail = rmsOf (audio, 6000, 11000);
        mark.edge = (float) std::sqrt (edge / 4000.0);
        mark.low = goertzel (audio, 500, 6000, 60.0f, 48000.0);
        mark.high = goertzel (audio, 0, 2500, 3000.0f, 48000.0);
        fingerprints.push_back (mark);
        if (i > 0)
        {
            bool same = true;
            const float* a = fieldData (presetTargets (i - 1));
            const float* b = fieldData (preset);
            for (int f = 0; f < kFieldCount; ++f)
                if (std::memcmp (a + f, b + f, sizeof (float)) != 0)
                    same = false;
            expect (! same, std::string ("preset duplicate ") + presetName (i));
        }
    }
    auto separated = [] (const Finger& a, const Finger& b)
    {
        const float scale = std::max (0.02f, std::max (a.early + a.mid + a.tail, b.early + b.mid + b.tail));
        const float energy = (std::abs (a.early - b.early) + std::abs (a.mid - b.mid) + std::abs (a.tail - b.tail)) / scale;
        const float brightness = std::abs (a.edge - b.edge) / std::max (0.002f, std::max (a.edge, b.edge));
        const float weight = std::abs (a.low - b.low) / std::max (1.0f, std::max (a.low, b.low));
        const float transient = std::abs (a.high - b.high) / std::max (1.0f, std::max (a.high, b.high));
        return energy > 0.08f || brightness > 0.18f || weight > 0.20f || transient > 0.35f;
    };
    int close = 0;
    for (int i = 0; i < kPresetCount; ++i)
        for (int j = i + 1; j < kPresetCount; ++j)
            if (! separated (fingerprints[(size_t) i], fingerprints[(size_t) j]))
            {
                std::cerr << "close " << presetName (i) << " / " << presetName (j) << "\n";
                ++close;
            }
    expect (close == 0, "preset renders are distinct");

    juce::String host;
    const int hostCode = KickInstrument::runSelfTest (host);
    if (hostCode != 0)
        std::cerr << host;
    expect (hostCode == 0, "host integration");

    if (fails != 0)
        std::cerr << fails << " kick checks failed\n";
    return fails == 0 ? 0 : 1;
}
