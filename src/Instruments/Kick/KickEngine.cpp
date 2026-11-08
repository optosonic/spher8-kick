#include "KickEngine.h"

#include <cmath>

namespace atrium::kick
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kTwoPi = 6.28318530717958647692f;
        constexpr float kHeadroom = 0.70f;

        float coefMs (double sampleRate, float ms) noexcept
        {
            const float sec = std::max (0.00005f, ms * 0.001f);
            return 1.0f - std::exp (-1.0f / (float) (sampleRate * (double) sec));
        }

        float smooth (float u) noexcept
        {
            u = clampf (u, 0.0f, 1.0f);
            return u * u * (3.0f - 2.0f * u);
        }

        int stepped (float value, int hi) noexcept
        {
            return std::max (0, std::min (hi, (int) std::lround (value)));
        }
    }

    float Engine::Svf::process (float x, float hz, float q, double sampleRate, int mode) noexcept
    {
        if (! std::isfinite (x))
            return 0.0f;
        hz = clampf (hz, 12.0f, (float) sampleRate * 0.45f);
        q = clampf (q, 0.25f, 6.0f);
        const float g = std::tan (kPi * hz / (float) sampleRate);
        const float k = 1.0f / q;
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float a2 = g * a1;
        const float a3 = g * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        if (! std::isfinite (ic1) || ! std::isfinite (ic2))
        {
            clear();
            return 0.0f;
        }
        if (std::abs (ic1) < 1.0e-20f) ic1 = 0.0f;
        if (std::abs (ic2) < 1.0e-20f) ic2 = 0.0f;
        if (mode == 1)
            return sanef (x - k * v1 - v2);
        if (mode == 2)
            return sanef (v1);
        return sanef (v2);
    }

    Engine::Engine() noexcept
    {
        const Targets init = defaults();
        snapA = snapB = lastLive = publishedA = publishedB = pendingA = pendingB = init;
    }

    void Engine::prepare (double sampleRate, int) noexcept
    {
        sr = sampleRate > 1000.0 ? sampleRate : 48000.0;
        reset();
    }

    void Engine::reset() noexcept
    {
        for (auto& v : voice)
            v = {};
        sounding = 0;
        youngestAge = 0;
        armed = false;
        lowPass.clear();
        highPass.clear();
        toneState = punchState = compEnv = dcX = dcY = 0.0f;
        smoothMorph = smoothDrive = driveEnv = followLift = meterEnv = 0.0f;
        meterClipHold = 0;
        smoothOut = 1.0f;
        smoothEnabled = 1.0f;
        capturing = false;
        capPos = 0;
        lastProbes = {};
    }

    void Engine::setSnapshots (const Targets& a, const Targets& b) noexcept
    {
        snapA = a;
        snapB = b;
        lastLive = a;
        publishedA = a;
        publishedB = b;
    }

    void Engine::requestSnapshot (int which, const Targets& value) noexcept
    {
        if ((which & 1) != 0)
        {
            pendingA = value;
            wantA.store (1, std::memory_order_release);
        }
        if ((which & 2) != 0)
        {
            pendingB = value;
            wantB.store (1, std::memory_order_release);
        }
    }

    bool Engine::readSnapshots (Targets& a, Targets& b) const noexcept
    {
        for (int attempt = 0; attempt < 4; ++attempt)
        {
            const unsigned s0 = publishSeq.load (std::memory_order_acquire);
            if ((s0 & 1u) != 0)
                continue;
            a = publishedA;
            b = publishedB;
            const unsigned s1 = publishSeq.load (std::memory_order_acquire);
            if (s0 == s1)
            {
                if (wantA.load (std::memory_order_acquire) == 1)
                    a = pendingA;
                if (wantB.load (std::memory_order_acquire) == 1)
                    b = pendingB;
                return true;
            }
        }
        a = snapA;
        b = snapB;
        return true;
    }

    bool Engine::copyView (LiveView& dest) const noexcept
    {
        for (int attempt = 0; attempt < 4; ++attempt)
        {
            const unsigned s0 = viewSeq.load (std::memory_order_acquire);
            if ((s0 & 1u) != 0)
                continue;
            dest = views[0];
            const unsigned s1 = viewSeq.load (std::memory_order_acquire);
            if (s0 == s1)
                return true;
        }
        return false;
    }

    void Engine::applySnapshotRequests() noexcept
    {
        bool applied = false;
        if (wantA.load (std::memory_order_acquire) == 1)
        {
            snapA = pendingA;
            wantA.store (0, std::memory_order_relaxed);
            applied = true;
        }
        if (wantB.load (std::memory_order_acquire) == 1)
        {
            snapB = pendingB;
            wantB.store (0, std::memory_order_relaxed);
            applied = true;
        }
        if (applied)
            lastLive = snapA;
    }

    float Engine::oscillator (int type, double phase, float harm, float pm) const noexcept
    {
        const float p = (float) (phase - std::floor (phase));
        const float w = kTwoPi * p;
        switch (type)
        {
            case 1:
                return 1.0f - 4.0f * std::abs (p - 0.5f);
            case 2:
                return std::tanh (2.6f * std::sin (w));
            case 3:
            {
                const float s = std::sin (w);
                const float s2 = std::sin (2.0f * w + 0.17f);
                const float tri = 1.0f - 4.0f * std::abs (p - 0.5f);
                return s * 0.78f + s2 * 0.14f + tri * 0.08f;
            }
            case 4:
            {
                const float idx = 0.45f + 3.2f * clampf (pm, 0.0f, 1.0f);
                return std::sin (w + idx * std::sin (w));
            }
            case 5:
            {
                const float h = clampf (harm, 0.0f, 1.0f);
                const float y = std::sin (w)
                              + h * 0.42f * std::sin (2.0f * w)
                              + h * 0.20f * std::sin (3.0f * w)
                              + h * h * 0.10f * std::sin (4.0f * w);
                const float n = 1.0f + h * 0.42f + h * 0.20f + h * h * 0.10f;
                return y / n;
            }
            default:
                return std::sin (w);
        }
    }

    float Engine::clickSample (Voice& v, const Targets& sound, int type) noexcept
    {
        auto white = [&v]() noexcept
        {
            unsigned x = v.rng;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            if (x == 0)
                x = 1u;
            v.rng = x;
            return ((x >> 8) & 0xFFFFFFu) / float (0xFFFFFFu) * 2.0f - 1.0f;
        };

        if (type == 3)
        {
            const int age = v.age - v.clickWait;
            if (age == 0) return 1.0f;
            if (age == 1) return -0.55f;
            if (age == 2) return 0.22f;
            return 0.0f;
        }
        if (type == 2)
        {
            const float y = std::sin (kTwoPi * (float) v.clickPhase);
            v.clickPhase += (double) clampf (sound.clickHz, 40.0f, 12000.0f) / sr;
            v.clickPhase -= std::floor (v.clickPhase);
            return y;
        }
        if (type == 4)
        {
            static constexpr float ratios[4] = { 1.0f, 2.41f, 4.83f, 7.77f };
            static constexpr float amps[4] = { 0.55f, 0.28f, 0.16f, 0.09f };
            float y = 0.0f;
            const float base = clampf (sound.clickHz, 200.0f, 6000.0f);
            for (int i = 0; i < 4; ++i)
            {
                y += amps[i] * std::sin (kTwoPi * (float) v.metalPhase[i]);
                v.metalPhase[i] += (double) (base * ratios[i]) / sr;
                v.metalPhase[i] -= std::floor (v.metalPhase[i]);
            }
            return y;
        }

        const float w = white();
        if (type == 1)
        {
            v.pink0 = 0.99765f * v.pink0 + w * 0.0299f;
            v.pink1 = 0.96300f * v.pink1 + w * 0.0322f;
            v.pink2 = 0.57000f * v.pink2 + w * 0.0520f;
            return (v.pink0 + v.pink1 + v.pink2) * 0.45f;
        }
        if (type == 5)
        {
            const float hz = clampf (sound.clickHz, 200.0f, 8000.0f);
            v.nLow += (w - v.nLow) * clampf (hz / (float) sr * 8.0f, 0.01f, 0.8f);
            return w - v.nLow;
        }
        return w;
    }

    float Engine::shapeSample (float x, const Targets& sound) noexcept
    {
        const float driveCoef = coefMs (sr, 3.0f);
        const float lift = clampf (followLift, 0.0f, 1.0f);
        const float driveIn = clampf (sound.drive * (1.0f + clampf (sound.followDrive, 0.0f, 1.0f) * lift * 1.6f),
                                      0.0f, 1.0f);
        smoothDrive += (driveIn - smoothDrive) * driveCoef;
        const int modeA = stepped (snapA.satMode, 4);
        const int modeB = stepped (snapB.satMode, 4);
        const bool linear = smoothDrive < 0.0008f
                         && std::abs (sound.asym) < 0.0008f
                         && std::abs (sound.evenOdd) < 0.0008f
                         && sound.fold < 0.0008f
                         && modeA == 0
                         && modeB == 0;
        float y = x;
        if (! linear)
        {
            // Soft mode used to be tanh(x * g) / g, which restores a low kick sine
            // and hides the harmonics. Odd partials are added in proportion to drive
            // while the dry fundamental stays in the mix.
            const float d = clampf (smoothDrive, 0.0f, 1.0f);
            const float asym = clampf (sound.asym, -1.0f, 1.0f);
            float mixed = x + asym * 0.28f * x * std::abs (x);
            const float evenOdd = clampf (sound.evenOdd, -1.0f, 1.0f);
            if (evenOdd > 0.0f)
                mixed = lerpf (mixed, std::abs (mixed) * 1.15f - 0.15f, evenOdd);
            else if (evenOdd < 0.0f)
                mixed *= lerpf (1.0f, 1.35f, -evenOdd);

            const float ax = std::abs (mixed);
            const float envCoef = ax > driveEnv ? coefMs (sr, 0.6f) : coefMs (sr, 40.0f);
            driveEnv += (ax - driveEnv) * envCoef;
            if (driveEnv < 1.0e-8f)
                driveEnv = 0.0f;
            const float env = std::max (driveEnv, 0.02f);
            const float n = clampf (mixed / env, -1.15f, 1.15f);
            const float g = 1.0f + d * 6.0f;
            const float mix = modeA == modeB ? 0.0f : smooth ((smoothMorph - 0.25f) / 0.5f);

            auto saturate = [&] (int mode, float unit) noexcept
            {
                switch (mode)
                {
                    case 1: return std::tanh (unit * g + 0.22f * unit * std::abs (unit));
                    case 2: return (unit * g) / (1.0f + std::abs (unit * g));
                    case 3: return clampf (unit * (1.0f + d * 3.2f), -1.0f, 1.0f);
                    case 4:
                    {
                        const float k = 1.0f + clampf (sound.fold, 0.0f, 1.0f) * (1.4f + 2.4f * d);
                        return std::sin (unit * k);
                    }
                    default: return std::tanh (unit * g);
                }
            };
            auto fullScale = [&] (int mode) noexcept
            {
                switch (mode)
                {
                    case 1: return std::max (0.2f, std::tanh (g + 0.22f));
                    case 2: return std::max (0.2f, g / (1.0f + g));
                    case 3: return 1.0f;
                    case 4: return 1.0f;
                    default: return std::max (0.2f, std::tanh (g));
                }
            };

            if (d < 0.0008f && modeA == 0 && modeB == 0)
            {
                y = sanef (mixed);
            }
            else
            {
                // Explicit Chebyshev partials so harmonic energy rises with drive.
                // A normalised tanh can sit out of phase with those partials and
                // cancel the 3rd just where the knob is turned up.
                const float t3 = 4.0f * n * n * n - 3.0f * n;
                const float n2 = n * n;
                const float t5 = ((16.0f * n2 - 20.0f) * n2 + 5.0f) * n;
                float voiced = mixed * (1.0f - 0.05f * d);
                voiced += (d * 0.50f * t3 + d * d * 0.22f * t5) * env;
                voiced += d * 0.14f * n * std::abs (n) * env;
                if (modeA != 0 || modeB != 0)
                {
                    const float shaped = lerpf (saturate (modeA, n), saturate (modeB, n), mix);
                    const float full = std::max (0.05f, lerpf (fullScale (modeA), fullScale (modeB), mix));
                    voiced = lerpf (voiced, (shaped / full) * env, 0.35f);
                }
                y = clampf (sanef (voiced), -2.0f, 2.0f);
            }
        }

        const float tone = clampf (sound.tone, -1.0f, 1.0f);
        const float toneCoef = coefMs (sr, 0.35f);
        toneState += (y - toneState) * clampf (toneCoef, 0.001f, 0.2f);
        const float low = toneState;
        const float high = y - low;
        if (tone >= 0.0f)
            y = low + high * (1.0f + tone * 1.4f);
        else
            y = y + (low - y) * (-tone);

        if (sound.hp > 22.0f)
            y = highPass.process (y, sound.hp, 0.707f, sr, 1);
        if (sound.cutoff < 16000.0f)
            y = lowPass.process (y, sound.cutoff, 0.707f, sr, 0);

        const float punchCoef = 1.0f - std::exp (-kTwoPi * 380.0f / (float) sr);
        punchState += (y - punchState) * punchCoef;
        const float transient = y - punchState;
        const float burst = std::exp (-(float) youngestAge / (float) (sr * 0.0052));
        y += transient * burst * (clampf (sound.punch, 0.0f, 1.0f) * 2.1f
                                 + clampf (sound.atkEmph, 0.0f, 1.0f) * 1.35f);

        const float tail = clampf (((float) youngestAge - (float) sr * 0.022f) / (float) (sr * 0.045f), 0.0f, 1.0f);
        y *= 1.0f + clampf (sound.susShape, -1.0f, 1.0f) * tail * 0.85f;

        if (sound.comp > 0.001f)
        {
            const float atk = coefMs (sr, sound.compA);
            const float rel = coefMs (sr, std::max (5.0f, sound.compR));
            const float rect = std::abs (y);
            compEnv += (rect - compEnv) * (rect > compEnv ? atk : rel);
            const float thresh = std::max (1.0e-5f, dbToGain (sound.thresh));
            float gain = 1.0f;
            if (compEnv > thresh)
                gain = std::pow (compEnv / thresh, 1.0f / std::max (1.0f, sound.ratio) - 1.0f);
            gain = lerpf (1.0f, sanef (gain), clampf (sound.comp, 0.0f, 1.0f));
            y *= gain * dbToGain (sound.makeup);
        }
        if (std::abs (compEnv) < 1.0e-12f)
            compEnv = 0.0f;

        const float dc = y - dcX + 0.9975f * dcY;
        dcX = y;
        dcY = sanef (dc);
        if (std::abs (dcY) < 1.0e-15f)
            dcY = 0.0f;
        y = dcY;

        const float outCoef = coefMs (sr, 4.0f);
        smoothOut += (dbToGain (sound.output) - smoothOut) * outCoef;
        const float enabled = sound.enabled >= 0.5f ? 1.0f : 0.0f;
        smoothEnabled += (enabled - smoothEnabled) * outCoef;
        y *= smoothOut * smoothEnabled;

        const float amount = clampf (sound.clip, 0.0f, 1.0f);
        if (amount > 0.0008f)
        {
            const float ceiling = std::max (0.02f, dbToGain (sound.ceiling));
            const float pushed = y * (1.0f + amount * 4.0f);
            const float limited = sound.clipHard >= 0.5f
                                      ? clampf (pushed, -ceiling, ceiling)
                                      : ceiling * std::tanh (pushed / ceiling);
            y = lerpf (y, limited, amount);
        }
        return y;
    }

    void Engine::startVoice (Voice& v, int note, float velocity, int channel, const Targets& sound) noexcept
    {
        v.bleed = v.last;
        v.last = 0.0f;
        v.active = true;
        v.note = note;
        v.velocity = clampf (velocity, 0.0f, 1.0f);
        v.channel = channel;
        v.age = 0;
        v.released = false;
        v.releaseAge = 0;
        v.releaseLevel = 0.0f;
        v.layerOn = false;
        v.layerPos = 0.0f;
        v.followSm = 0.0f;
        v.followAmpSm = 0.0f;
        v.res.clear();
        v.stamp = ++stamp;
        const KeyOffsets spread = keyOffsets (sound, note);
        v.tuneSemis = spread.tuneSemis;
        v.decayMul = spread.decayMul;
        v.punchAdd = spread.punch;
        v.clickAdd = spread.click;
        v.driveAdd = spread.drive;
        v.subAdd = spread.sub;
        v.morphAdd = spread.morph;
        v.fund = keyedFundamental (sound, note, v.tuneSemis);
        if (sound.phaseReset >= 0.5f)
        {
            v.bodyPhase = 0.0;
            v.subPhase = (double) clampf (sound.subPhase, 0.0f, 1.0f);
            v.clickPhase = 0.0;
            for (double& phase : v.metalPhase)
                phase = 0.0;
        }
        const unsigned noteKey = (unsigned) note;
        const unsigned stampKey = (unsigned) v.stamp;
        unsigned seed = 0xA341316Cu ^ (noteKey * 0x9E3779B9u) ^ (stampKey * 0x85EBCA6Bu);
        v.rng = seed == 0 ? 1u : seed;
        v.pink0 = v.pink1 = v.pink2 = v.nLow = v.nHigh = 0.0f;
        const int shift = (int) std::lround ((double) sound.clickPos * 0.001 * sr);
        v.clickWait = std::max (0, shift);
        v.bodyWait = std::max (0, -shift);

        capturing = true;
        capPos = 0;
        capTick = 0;
        accN = 0;
        accWave = accBody = accSub = accClick = accLayer = 0.0f;
        capPitchAt = 0;
        const float decayMs = keyedDecayMs (sound.decay, v.decayMul);
        float sec = 0.03f + std::max (0.0f, decayMs) * 0.001f + std::max (0.0f, sound.release) * 0.001f;
        sec = clampf (sec, 0.05f, 4.0f);
        capSpan = sec;
        const int total = std::max (1, (int) std::lround (sec * sr));
        capStep = std::max (1, total / kViewPoints);
        capPitchStep = std::max (1, total / 128);
        for (float& p : capPitch)
            p = 0.0f;
        ++capGen;
    }

    int Engine::allocateVoice() noexcept
    {
        if (stepped (snapA.voice, 1) == 0 && smoothMorph < 0.5f)
            return 0;
        if (stepped (lerpTargets (snapA, snapB, smoothMorph).voice, 1) == 0)
            return 0;
        for (int i = 0; i < kVoices; ++i)
            if (! voice[i].active)
                return i;
        int best = 0;
        int oldest = voice[0].stamp;
        for (int i = 1; i < kVoices; ++i)
            if (voice[i].stamp < oldest)
            {
                oldest = voice[i].stamp;
                best = i;
            }
        return best;
    }

    void Engine::releaseMatching (int note) noexcept
    {
        for (auto& v : voice)
        {
            if (! v.active || v.released || v.note != note)
                continue;
            v.released = true;
            v.releaseAge = 0;
            v.releaseLevel = std::max (0.0f, v.level);
        }
    }

    void Engine::silenceAll() noexcept
    {
        for (auto& v : voice)
        {
            if (! v.active)
                continue;
            v.bleed = v.last;
            v.active = false;
        }
    }

    void Engine::process (float* const* outputs, int numChannels, int numSamples,
                          const Targets& liveIn, const Hit* hits, int numHits,
                          SampleView sample) noexcept
    {
        if (outputs == nullptr || numChannels <= 0 || numSamples <= 0)
            return;
        for (int c = 0; c < numChannels; ++c)
            if (outputs[c] != nullptr)
                for (int i = 0; i < numSamples; ++i)
                    outputs[c][i] = 0.0f;

        applySnapshotRequests();
        Targets live = liveIn;
        const float morph = clampf (live.morph, 0.0f, 1.0f);
        if (! armed)
        {
            smoothMorph = morph;
            smoothDrive = clampf (live.drive, 0.0f, 1.0f);
            smoothOut = dbToGain (live.output);
            smoothEnabled = live.enabled >= 0.5f ? 1.0f : 0.0f;
            if (morph <= 0.001f)
                snapA = live;
            lastLive = live;
            armed = true;
        }
        else
        {
            if (morph < 0.5f)
                copyChanged (snapA, live, lastLive);
            else
                copyChanged (snapB, live, lastLive);
            lastLive = live;
        }

        Hit ordered[128];
        const int nHits = std::max (0, std::min (numHits, 128));
        for (int i = 0; i < nHits; ++i)
            ordered[i] = hits[i];
        for (int i = 1; i < nHits; ++i)
        {
            const Hit key = ordered[i];
            int j = i - 1;
            while (j >= 0 && ordered[j].offset > key.offset)
            {
                ordered[j + 1] = ordered[j];
                --j;
            }
            ordered[j + 1] = key;
        }

        const float morphCoef = coefMs (sr, 4.0f);
        int hitCursor = 0;
        float blockPeak = 0.0f;
        int blockClip = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            while (hitCursor < nHits && ordered[hitCursor].offset <= i)
            {
                const Hit& hit = ordered[hitCursor++];
                Targets now = lerpTargets (snapA, snapB, smoothMorph);
                const int wantChannel = (int) std::lround (now.channel);
                const int wantNote = (int) std::lround (now.inputNote);
                const bool ownHit = hit.channel <= 0;
                if (hit.kind == 2)
                {
                    silenceAll();
                }
                else if (! ownHit && wantChannel >= 1 && wantChannel <= 16 && hit.channel != wantChannel)
                {
                }
                else if (! ownHit && wantNote >= 1 && wantNote <= 128 && hit.note != wantNote - 1)
                {
                }
                else if (! noteInsideRange (now, hit.note))
                {
                }
                else if (hit.kind == 1 && hit.velocity > 0.0f)
                {
                    Voice& v = voice[allocateVoice()];
                    startVoice (v, hit.note, hit.velocity, hit.channel, now);
                    shownNote = hit.note;
                    shownVelocity = (int) std::lround (hit.velocity * 127.0f);
                    ++shownActivity;
                }
                else
                {
                    if (now.sustain > 0.001f)
                        releaseMatching (hit.note);
                }
            }

            smoothMorph += (morph - smoothMorph) * morphCoef;
            const Targets blockSound = lerpTargets (snapA, snapB, smoothMorph);
            const bool anySolo = blockSound.soloBody >= 0.5f || blockSound.soloSub >= 0.5f
                              || blockSound.soloClick >= 0.5f || blockSound.soloLayer >= 0.5f;
            auto gate = [&] (float on, float solo) noexcept
            {
                if (on < 0.5f)
                    return 0.0f;
                if (anySolo && solo < 0.5f)
                    return 0.0f;
                return 1.0f;
            };

            const int waveA = stepped (snapA.bodyWave, 5);
            const int waveB = stepped (snapB.bodyWave, 5);
            const int clickA = stepped (snapA.clickType, 6);
            const int clickB = stepped (snapB.clickType, 6);
            const int impactA = stepped (snapA.impactShape, 2);
            const int impactB = stepped (snapB.impactShape, 2);
            const int bodyShapeA = stepped (snapA.bodyShape, 2);
            const int bodyShapeB = stepped (snapB.bodyShape, 2);
            const int followA = stepped (snapA.followShape, 2);
            const int followB = stepped (snapB.followShape, 2);

            float bodySum = 0.0f, subSum = 0.0f, clickSum = 0.0f, layerSum = 0.0f;
            int newest = -1;
            int newestStamp = -1;
            sounding = 0;

            for (int vi = 0; vi < kVoices; ++vi)
            {
                Voice& v = voice[vi];
                if (! v.active)
                    continue;
                ++sounding;
                if (v.stamp >= newestStamp)
                {
                    newestStamp = v.stamp;
                    newest = vi;
                }

                Targets keyedStore;
                const Targets* heard = &blockSound;
                float morphNow = smoothMorph;
                if (v.morphAdd != 0.0f)
                {
                    morphNow = clampf (smoothMorph + v.morphAdd, 0.0f, 1.0f);
                    keyedStore = lerpTargets (snapA, snapB, morphNow);
                    heard = &keyedStore;
                }
                const Targets& sound = *heard;
                const float waveMix = waveA == waveB ? 0.0f : smooth ((morphNow - 0.25f) / 0.5f);
                const float clickMix = clickA == clickB ? 0.0f : smooth ((morphNow - 0.25f) / 0.5f);
                const float impactMix = impactA == impactB ? 0.0f : smooth ((morphNow - 0.25f) / 0.5f);
                const float bodyShapeMix = bodyShapeA == bodyShapeB ? 0.0f : smooth ((morphNow - 0.25f) / 0.5f);
                const float followMix = followA == followB ? 0.0f : smooth ((morphNow - 0.25f) / 0.5f);

                const float timeSec = (float) v.age / (float) sr;
                const float fund = keyedFundamental (sound, v.note, v.tuneSemis);
                if (v.age == 0)
                    v.fund = fund;
                else
                    v.fund += (fund - v.fund) * coefMs (sr, 3.0f);

                const float impactDur = std::max (0.0001f, sound.impactMs * 0.001f);
                const float bodyDur = std::max (0.0001f, sound.bodyMs * 0.001f);
                const float impactEnv = lerpf (stageEnv (timeSec, impactDur, impactA, sound.impactCurve),
                                               stageEnv (timeSec, impactDur, impactB, sound.impactCurve),
                                               impactMix);
                const float bodyEnv = lerpf (stageEnv (timeSec, bodyDur, bodyShapeA, sound.bodyCurve),
                                             stageEnv (timeSec, bodyDur, bodyShapeB, sound.bodyCurve),
                                             bodyShapeMix);
                const float velPitch = lerpf (1.0f, v.velocity, clampf (sound.velPitch, 0.0f, 1.0f));
                const float semis = (sound.impactSt * impactEnv + sound.bodySt * bodyEnv) * velPitch;
                const float hz = clampf (v.fund * std::exp2 (semis / 12.0f), 8.0f, 14000.0f);

                if (v.stamp == newestStamp)
                {
                    if (v.age == 0)
                        lastProbes.startHz = hz;
                    if (v.age == (int) std::lround (0.040 * sr))
                        lastProbes.midHz = hz;
                    if (v.age == (int) std::lround (0.180 * sr))
                        lastProbes.lateHz = hz;
                }

                float amp = 0.0f;
                const float attack = std::max (0.0f, sound.attack * 0.001f);
                const float decay = std::max (0.001f, keyedDecayMs (sound.decay, v.decayMul) * 0.001f);
                const float sustain = clampf (sound.sustain, 0.0f, 1.0f);
                if (v.released && sustain > 0.001f)
                {
                    const float rel = std::max (0.001f, sound.release * 0.001f);
                    const float u = ((float) v.releaseAge / (float) sr) / rel;
                    amp = u >= 1.0f ? 0.0f : v.releaseLevel * std::pow (0.001f, u);
                    if (u >= 1.0f)
                        v.active = false;
                    ++v.releaseAge;
                }
                else if (timeSec < attack)
                {
                    amp = attack <= 1.0e-7f ? 1.0f : timeSec / attack;
                }
                else
                {
                    const float u = (timeSec - attack) / decay;
                    if (sustain <= 0.001f)
                    {
                        amp = u >= 1.0f ? 0.0f : std::pow (0.001f, u);
                        if (u >= 1.0f)
                            v.active = false;
                    }
                    else if (u < 1.0f)
                        amp = sustain + (1.0f - sustain) * std::pow (0.001f, u);
                    else
                        amp = sustain;
                }

                const float ampVel = lerpf (1.0f, v.velocity, clampf (sound.velAmt, 0.0f, 1.0f));
                const float pol = sound.bodyPol >= 0.5f ? -1.0f : 1.0f;
                {
                    float followTarget = 0.0f;
                    float ampTarget = 0.0f;
                    if (timeSec >= impactDur)
                    {
                        const float after = timeSec - impactDur;
                        const float a = followSemis (sound, after, followA);
                        const float b = followMix > 0.001f ? followSemis (sound, after, followB) : a;
                        followTarget = lerpf (a, b, followMix);
                        if (sound.followAmpUse >= 0.5f)
                        {
                            const float aa = followAmp (sound, after, followA);
                            const float bb = followMix > 0.001f ? followAmp (sound, after, followB) : aa;
                            ampTarget = lerpf (aa, bb, followMix);
                        }
                        else
                            ampTarget = clampf (followTarget / 12.0f, 0.0f, 1.0f);
                    }
                    const float followCoef = coefMs (sr, 3.0f);
                    v.followSm += (followTarget - v.followSm) * followCoef;
                    if (! std::isfinite (v.followSm) || std::abs (v.followSm) < 1.0e-6f)
                        v.followSm = 0.0f;
                    v.followSm = clampf (v.followSm, -24.0f, 24.0f);
                    v.followAmpSm += (ampTarget - v.followAmpSm) * followCoef;
                    if (! std::isfinite (v.followAmpSm) || v.followAmpSm < 1.0e-6f)
                        v.followAmpSm = 0.0f;
                    v.followAmpSm = clampf (v.followAmpSm, 0.0f, 1.0f);
                }
                const bool tonal = v.age >= v.bodyWait;
                if (tonal && v.active)
                {
                    const float pm = clampf (std::abs (semis) / 48.0f, 0.0f, 1.0f);
                    const float a = oscillator (waveA, v.bodyPhase, sound.bodyHarm, pm);
                    const float b = waveMix > 0.001f ? oscillator (waveB, v.bodyPhase, sound.bodyHarm, pm) : a;
                    const float body = lerpf (a, b, waveMix) * pol * amp * ampVel
                                     * clampf (sound.bodyLevel, 0.0f, 1.0f) * gate (sound.bodyOn, sound.soloBody);
                    bodySum += body;
                    v.bodyPhase += (double) hz / sr;
                    v.bodyPhase -= std::floor (v.bodyPhase);

                    const float subFund = (sound.subTrack >= 0.5f ? v.fund : clampf (sound.subHz, 20.0f, 160.0f))
                                        * std::exp2 (sound.subSemi / 12.0f);
                    const float subSemis = v.followSm * clampf (sound.followSub, 0.0f, 1.0f);
                    const float subHz = clampf (sanef (subFund * std::exp2 (subSemis / 12.0f)), 10.0f, 800.0f);
                    const float subTime = (float) std::max (0, v.age - v.bodyWait) / (float) sr;
                    const float subAtk = std::max (0.0f, sound.subAttack * 0.001f);
                    const float subDec = std::max (0.001f, sound.subDecay * 0.001f);
                    float subEnv = 1.0f;
                    if (subTime < subAtk)
                        subEnv = subAtk <= 1.0e-7f ? 1.0f : subTime / subAtk;
                    else
                        subEnv = std::pow (0.001f, clampf ((subTime - subAtk) / subDec, 0.0f, 1.0f));
                    const bool ampLane = sound.followAmpUse >= 0.5f;
                    const float envPeak = ampLane ? v.followAmpSm : clampf (v.followSm / 12.0f, 0.0f, 1.0f);
                    const float subDepth = clampf (sound.followSub, 0.0f, 1.0f);
                    const float resAmt = clampf (sound.followRes, 0.0f, 1.0f);
                    const float extra = ampLane ? envPeak * resAmt
                                                : envPeak * (0.28f * subDepth + 0.70f * resAmt);
                    const float harm = clampf (sound.subHarm + extra, 0.0f, 1.0f);
                    const float w = kTwoPi * (float) v.subPhase;
                    float sub = std::sin (w);
                    sub += harm * std::sin (2.0f * w);
                    if (extra > 0.001f)
                        sub += extra * 0.35f * std::sin (3.0f * w);
                    sub /= 1.0f + harm;
                    const float bpMix = ampLane ? envPeak * (0.55f * resAmt)
                                                : envPeak * (0.10f * subDepth + 0.42f * resAmt);
                    if (bpMix > 0.001f)
                    {
                        const float bp = v.res.process (std::sin (2.0f * w),
                                                        clampf (subHz * 2.0f, 25.0f, (float) sr * 0.42f),
                                                        lerpf (1.5f, 3.4f, resAmt), sr, 2);
                        sub += bp * bpMix;
                    }
                    const float levelMul = ampLane
                        ? clampf (1.0f + clampf (sound.followLevel, 0.0f, 1.0f) * envPeak, 0.0f, 1.85f)
                        : clampf (1.0f + clampf (sound.followLevel, 0.0f, 1.0f)
                                      * clampf (v.followSm / 12.0f, -1.0f, 1.0f),
                                  0.0f, 1.85f);
                    sub = clampf (sanef (sub), -1.6f, 1.6f);
                    sub *= subEnv * clampf (sound.subLevel + v.subAdd, 0.0f, 1.0f) * levelMul * gate (sound.subOn, sound.soloSub) * ampVel;
                    subSum += sub;
                    v.subPhase += (double) subHz / sr;
                    v.subPhase -= std::floor (v.subPhase);
                }

                if (v.age >= v.clickWait && v.active)
                {
                    const float clickAge = (float) (v.age - v.clickWait) / (float) sr;
                    const float clickDur = std::max (0.0002f, sound.clickMs * 0.001f);
                    const float u = clickAge / clickDur;
                    if (u < 1.0f)
                    {
                        const float bend = lerpf (1.35f, 0.45f, clampf (sound.clickShape, 0.0f, 1.0f));
                        const float cenv = std::pow (0.001f, std::pow (clampf (u, 0.0f, 1.0f), bend));
                        float raw = clickSample (v, sound, clickA);
                        if (clickMix > 0.001f)
                            raw = lerpf (raw, clickSample (v, sound, clickB), clickMix);
                        const float hpCoef = 1.0f - std::exp (-kTwoPi * clampf (sound.clickHp, 20.0f, 8000.0f) / (float) sr);
                        v.nLow += (raw - v.nLow) * clampf (hpCoef, 0.0f, 0.95f);
                        const float high = raw - v.nLow;
                        const float bright = clampf (sound.clickBright, 0.0f, 1.0f);
                        const float lpHz = lerpf (700.0f, 11000.0f, bright);
                        const float lpCoef = 1.0f - std::exp (-kTwoPi * lpHz / (float) sr);
                        v.nHigh += (high - v.nHigh) * clampf (lpCoef, 0.0f, 0.98f);
                        const float coloured = lerpf (v.nHigh, high, bright * 0.65f);
                        const float clickVel = lerpf (1.0f, v.velocity, clampf (sound.velClick, 0.0f, 1.0f));
                        const float level = clampf (sound.clickLevel + v.clickAdd, 0.0f, 1.0f)
                                          * (1.0f + clampf (sound.clickEnh, 0.0f, 1.0f));
                        float click = coloured * cenv * level * clickVel * gate (sound.clickOn, sound.soloClick);
                        if (clickA == 6 || (clickMix > 0.001f && clickB == 6))
                        {
                            if (sample.data != nullptr && sample.frames > 8)
                            {
                                const int pos = v.age - v.clickWait;
                                if (pos >= 0 && pos < sample.frames)
                                    click = lerpf (click, sample.data[pos] * cenv * level, clickA == 6 ? 1.0f : clickMix);
                            }
                        }
                        clickSum += click;
                    }
                }

                if (sample.data != nullptr && sample.frames > 4 && v.age >= v.bodyWait)
                {
                    const int frames = sample.frames;
                    int start = (int) (clampf (sound.layerStart, 0.0f, 1.0f) * (float) (frames - 2));
                    int end = (int) (clampf (sound.layerEnd, 0.0f, 1.0f) * (float) (frames - 1));
                    if (end < start + 2)
                        end = std::min (frames - 1, start + 2);
                    if (! v.layerOn)
                    {
                        v.layerOn = true;
                        v.layerPos = (float) start;
                    }
                    if (v.layerPos < (float) end)
                    {
                        const int i0 = std::max (0, std::min (frames - 2, (int) v.layerPos));
                        const float frac = v.layerPos - (float) i0;
                        const float s = sample.data[i0] * (1.0f - frac) + sample.data[i0 + 1] * frac;
                        const float layerTime = (float) (v.age - v.bodyWait) / (float) sr;
                        const float lAttack = std::max (0.0f, sound.layerAttack * 0.001f);
                        const float lDecay = std::max (0.001f, sound.layerDecay * 0.001f);
                        float lenv = 1.0f;
                        if (layerTime < lAttack)
                            lenv = lAttack <= 1.0e-7f ? 1.0f : layerTime / lAttack;
                        else
                            lenv = std::pow (0.001f, clampf ((layerTime - lAttack) / lDecay, 0.0f, 1.0f));
                        const float lpol = sound.layerPol >= 0.5f ? -1.0f : 1.0f;
                        layerSum += s * lenv * lpol * clampf (sound.layerLevel, 0.0f, 1.0f)
                                  * gate (sound.layerOn, sound.soloLayer);
                        const float inc = std::max (0.001f, sample.rate / (float) sr) * std::exp2 (sound.layerSemi / 12.0f);
                        v.layerPos += inc;
                    }
                }

                v.level = amp;
                if (v.age > (int) (sr * 8.0))
                    v.active = false;
                ++v.age;
            }

            if (newest >= 0)
                youngestAge = voice[newest].age;
            else
                youngestAge += 1;

            const float dry = (bodySum + subSum + clickSum + layerSum) * kHeadroom;
            const Targets* post = &blockSound;
            Targets postLocal;
            if (newest >= 0)
            {
                const Voice& nv = voice[newest];
                if (nv.morphAdd != 0.0f || nv.driveAdd != 0.0f || nv.punchAdd != 0.0f)
                {
                    if (nv.morphAdd != 0.0f)
                        postLocal = lerpTargets (snapA, snapB, clampf (smoothMorph + nv.morphAdd, 0.0f, 1.0f));
                    else
                        postLocal = blockSound;
                    if (nv.driveAdd != 0.0f)
                        postLocal.drive = clampf (postLocal.drive + nv.driveAdd, 0.0f, 1.0f);
                    if (nv.punchAdd != 0.0f)
                        postLocal.punch = clampf (postLocal.punch + nv.punchAdd, 0.0f, 1.0f);
                    post = &postLocal;
                }
            }
            followLift = 0.0f;
            if (newest >= 0)
                followLift = post->followAmpUse >= 0.5f
                    ? clampf (voice[newest].followAmpSm, 0.0f, 1.0f)
                    : clampf (voice[newest].followSm / 12.0f, 0.0f, 1.0f);
            float y = shapeSample (dry, *post);
            if (newest >= 0 && voice[newest].active)
                voice[newest].last = y;
            y = sanef (y);
            const float ay = std::abs (y);
            blockPeak = std::max (blockPeak, ay);
            if (ay >= 0.999f)
                blockClip = 1;
            for (int c = 0; c < numChannels; ++c)
                if (outputs[c] != nullptr)
                    outputs[c][i] = y;

            if (capturing && capPos < kViewPoints)
            {
                accWave += y;
                accBody += bodySum * kHeadroom;
                accSub += subSum * kHeadroom;
                accClick += clickSum * kHeadroom;
                accLayer += layerSum * kHeadroom;
                ++accN;
                if (++capTick >= capStep)
                {
                    const float nrm = 1.0f / (float) std::max (1, accN);
                    views[1].wave[capPos] = accWave * nrm;
                    views[1].body[capPos] = accBody * nrm;
                    views[1].sub[capPos] = accSub * nrm;
                    views[1].click[capPos] = accClick * nrm;
                    views[1].layer[capPos] = accLayer * nrm;
                    ++capPos;
                    accN = 0;
                    accWave = accBody = accSub = accClick = accLayer = 0.0f;
                    capTick = 0;
                }
            }
            if (capturing && capPitchAt < 128 && (i % capPitchStep) == 0)
            {
                const float t = (float) capPitchAt / 127.0f * capSpan;
                capPitch[capPitchAt] = pitchHz (blockSound, t, fundamentalHz (blockSound));
                ++capPitchAt;
            }
        }

        lastProbes.voices = sounding;
        lastProbes.peak = blockPeak;

        const unsigned seq = viewSeq.load (std::memory_order_relaxed);
        viewSeq.store (seq + 1, std::memory_order_release);
        const int nCopy = std::max (0, std::min (capPos, kViewPoints));
        for (int p = 0; p < nCopy; ++p)
        {
            views[0].wave[p] = views[1].wave[p];
            views[0].body[p] = views[1].body[p];
            views[0].sub[p] = views[1].sub[p];
            views[0].click[p] = views[1].click[p];
            views[0].layer[p] = views[1].layer[p];
        }
        views[0].fundamental = fundamentalHz (lerpTargets (snapA, snapB, smoothMorph));
        views[0].startHz = pitchHz (lerpTargets (snapA, snapB, smoothMorph), 0.0f, views[0].fundamental);
        views[0].peak = std::max (views[0].peak * 0.92f, blockPeak);
        const float blockSec = (float) numSamples / (float) std::max (1.0, sr);
        const float meterKeep = std::exp (-blockSec / 0.12f);
        meterEnv = std::max (blockPeak, meterEnv * meterKeep);
        if (meterEnv < 1.0e-8f)
            meterEnv = 0.0f;
        if (blockClip != 0)
            meterClipHold = (int) std::lround (sr * 0.28);
        views[0].meter = meterEnv;
        if (meterClipHold > 0)
        {
            views[0].meterClip = 1;
            meterClipHold = std::max (0, meterClipHold - numSamples);
        }
        else
            views[0].meterClip = 0;
        views[0].points = nCopy;
        views[0].spanSec = capSpan;
        views[0].generation = capGen;
        views[0].note = shownNote;
        views[0].velocity = shownVelocity;
        views[0].activity = shownActivity;
        for (int p = 0; p < 128; ++p)
            views[0].pitch[p] = capPitch[p];
        viewSeq.store (seq + 2, std::memory_order_release);

        const unsigned ps = publishSeq.load (std::memory_order_relaxed);
        publishSeq.store (ps + 1, std::memory_order_release);
        publishedA = snapA;
        publishedB = snapB;
        publishSeq.store (ps + 2, std::memory_order_release);
    }
}
