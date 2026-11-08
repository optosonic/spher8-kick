#pragma once

#include "KickParams.h"

#include <atomic>

namespace atrium::kick
{
    struct Hit
    {
        int offset = 0;
        int kind = 1; // 1 note-on, 0 note-off, 2 all sound off
        int note = 36;
        float velocity = 1.0f;
        int channel = 1;
    };

    struct SampleView
    {
        const float* data = nullptr;
        int frames = 0;
        float rate = 48000.0f;
    };

    struct Probes
    {
        float startHz = 0.0f;
        float midHz = 0.0f;
        float lateHz = 0.0f;
        int voices = 0;
        float peak = 0.0f;
    };

    struct LiveView
    {
        float fundamental = 55.0f;
        float startHz = 110.0f;
        float peak = 0.0f;
        float meter = 0.0f;
        int meterClip = 0;
        int note = -1;
        int velocity = 0;
        int activity = 0;
        float pitch[128] {};
        float wave[384] {};
        float body[384] {};
        float sub[384] {};
        float click[384] {};
        float layer[384] {};
        int points = 0;
        float spanSec = 0.4f;
        int generation = 0;
    };

    class Engine
    {
    public:
        static constexpr int kVoices = 4;
        static constexpr int kViewPoints = 384;

        Engine() noexcept;

        void prepare (double sampleRate, int maximumBlock) noexcept;
        void reset() noexcept;
        void setSnapshots (const Targets& a, const Targets& b) noexcept;
        void requestSnapshot (int which, const Targets& value) noexcept;
        bool readSnapshots (Targets& a, Targets& b) const noexcept;

        void process (float* const* outputs, int numChannels, int numSamples,
                      const Targets& live, const Hit* hits, int numHits,
                      SampleView sample) noexcept;

        int activeVoices() const noexcept { return sounding; }
        Probes probes() const noexcept { return lastProbes; }
        bool copyView (LiveView& dest) const noexcept;
        int latencySamples() const noexcept { return 0; }

    private:
        struct Svf
        {
            float ic1 = 0.0f;
            float ic2 = 0.0f;
            void clear() noexcept { ic1 = ic2 = 0.0f; }
            float process (float x, float hz, float q, double sampleRate, int mode) noexcept;
        };

        struct Voice
        {
            bool active = false;
            int note = 36;
            float velocity = 1.0f;
            int channel = 1;
            double bodyPhase = 0.0;
            double subPhase = 0.0;
            double clickPhase = 0.0;
            double metalPhase[4] {};
            int age = 0;
            int bodyWait = 0;
            int clickWait = 0;
            bool released = false;
            int releaseAge = 0;
            float releaseLevel = 0.0f;
            float level = 0.0f;
            float bleed = 0.0f;
            float last = 0.0f;
            float fund = 55.0f;
            float tuneSemis = 0.0f;
            float decayMul = 1.0f;
            float punchAdd = 0.0f;
            float clickAdd = 0.0f;
            float driveAdd = 0.0f;
            float subAdd = 0.0f;
            float morphAdd = 0.0f;
            unsigned rng = 1u;
            float pink0 = 0.0f;
            float pink1 = 0.0f;
            float pink2 = 0.0f;
            float nLow = 0.0f;
            float nHigh = 0.0f;
            int stamp = 0;
            float layerPos = 0.0f;
            bool layerOn = false;
            float followSm = 0.0f;
            float followAmpSm = 0.0f;
            Svf res;
        };

        void applySnapshotRequests() noexcept;
        void startVoice (Voice& voice, int note, float velocity, int channel, const Targets& sound) noexcept;
        int allocateVoice() noexcept;
        void releaseMatching (int note) noexcept;
        void silenceAll() noexcept;
        float oscillator (int type, double phase, float harm, float pm) const noexcept;
        float clickSample (Voice& voice, const Targets& sound, int type) noexcept;
        float shapeSample (float x, const Targets& sound) noexcept;

        double sr = 48000.0;
        bool armed = false;
        int sounding = 0;
        int stamp = 1;
        int youngestAge = 0;
        int shownNote = -1;
        int shownVelocity = 0;
        int shownActivity = 0;
        Probes lastProbes {};
        Targets snapA {};
        Targets snapB {};
        Targets lastLive {};
        Targets pendingA {};
        Targets pendingB {};
        std::atomic<int> wantA { 0 };
        std::atomic<int> wantB { 0 };
        Targets publishedA {};
        Targets publishedB {};
        std::atomic<unsigned> publishSeq { 0 };

        float smoothMorph = 0.0f;
        float smoothDrive = 0.0f;
        float driveEnv = 0.0f;
        float followLift = 0.0f;
        float meterEnv = 0.0f;
        int meterClipHold = 0;
        float smoothOut = 1.0f;
        float smoothEnabled = 1.0f;
        float toneState = 0.0f;
        float punchState = 0.0f;
        float compEnv = 0.0f;
        float dcX = 0.0f;
        float dcY = 0.0f;
        Svf lowPass;
        Svf highPass;

        LiveView views[2] {};
        std::atomic<int> publishedView { 0 };
        std::atomic<unsigned> viewSeq { 0 };
        int capPos = 0;
        int capStep = 1;
        int capTick = 0;
        float accWave = 0.0f;
        float accBody = 0.0f;
        float accSub = 0.0f;
        float accClick = 0.0f;
        float accLayer = 0.0f;
        int accN = 0;
        bool capturing = false;
        int capGen = 0;
        float capPitch[128] {};
        int capPitchAt = 0;
        int capPitchStep = 1;
        float capSpan = 0.4f;

        Voice voice[kVoices];
    };
}
