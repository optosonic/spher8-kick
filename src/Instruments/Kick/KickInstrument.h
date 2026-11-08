#pragma once

#include "KickEngine.h"

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

namespace atrium::kick
{
    class KickInstrument final : public juce::AudioPluginInstance
    {
    public:
        KickInstrument();
        ~KickInstrument() override;

        static juce::PluginDescription description();
        static bool isKickIdentifier (const juce::String& fileOrIdentifier);
        static int runSelfTest (juce::String& report);

        void fillInPluginDescription (juce::PluginDescription&) const override;
        const juce::String getName() const override { return "Spher8 KICK"; }
        void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
        void releaseResources() override {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
        bool isBusesLayoutSupported (const BusesLayout&) const override;

        double getTailLengthSeconds() const override;
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        int getNumPrograms() override { return kPresetCount; }
        int getCurrentProgram() override { return program; }
        void setCurrentProgram (int index) override;
        const juce::String getProgramName (int index) override;
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock&) override;
        void setStateInformation (const void*, int) override;

        void loadSampleFile (const juce::File&);
        void clearSample() noexcept;
        juce::String samplePath() const;
        void audition (int midiNote = 36) noexcept;
        // Message thread. Plays the kick immediately. When a host hook is set,
        // the same note is offered to the track so an armed record can keep it.
        void performTrigger (int midiNote, int velocity, int channel);
        void setTriggerHook (std::function<void (int note, int velocity, int channel)> hook);
        void storeSnapshot (int which);
        bool copyView (LiveView& dest) const noexcept { return engine.copyView (dest); }
        Engine& dsp() noexcept { return engine; }

        juce::AudioProcessorValueTreeState apvts;

    private:
        Targets readTargets() const noexcept;
        void writeTargets (const Targets&, bool notifyHost);
        void publishSample (const float* interleavedOrMono, int frames, int channels, double fileRate);

        Engine engine;
        std::array<std::atomic<float>*, kFieldCount> raw {};
        std::vector<float> sample;
        std::atomic<int> sampleFrames { 0 };
        std::atomic<int> sampleEpoch { 0 };
        std::atomic<float> sampleRateHz { 48000.0f };
        std::atomic<int> auditionNote { -1 };
        std::function<void (int, int, int)> triggerHook;
        int program = 0;
        double preparedRate = 48000.0;
        bool prepared = false;
        juce::String loadedSample;
        juce::AudioFormatManager formats;
        std::array<char, 1024> queuedPath {};
        std::atomic<int> queuedSample { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickInstrument)
    };
}
