#include "KickInstrument.h"
#include "KickEditor.h"

#include <cmath>

namespace atrium::kick
{
    namespace
    {
        struct Spec
        {
            const char* id;
            const char* title;
            const char* unit;
            float lo;
            float hi;
            float centre;
            float step;
            int kind;
            int log;
        };

        constexpr Spec kSpecs[] = {
#define ATRIUM_KICK_SPEC(name, title, unit, lo, hi, centre, step, kind, log) \
            { #name, title, unit, lo, hi, centre, step, kind, log },
            ATRIUM_KICK_FIELDS (ATRIUM_KICK_SPEC)
#undef ATRIUM_KICK_SPEC
        };

        static_assert ((int) (sizeof (kSpecs) / sizeof (kSpecs[0])) == kFieldCount, "kick spec count");

        juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
        {
            using namespace juce;
            AudioProcessorValueTreeState::ParameterLayout layout;
            const Targets init = defaults();
            const float* base = fieldData (init);
            auto keyText = [] (float value, int) -> juce::String
            {
                return juce::String (juce::roundToInt (clampf (value, 0.0f, 1.0f) * 100.0f)) + "%";
            };
            auto keyValue = [] (const juce::String& text) -> float
            {
                const auto body = text.trim().upToFirstOccurrenceOf ("%", false, false).trim();
                const float n = body.getFloatValue();
                const float unit = (text.containsChar ('%') || n > 1.0f) ? n / 100.0f : n;
                return clampf (unit, 0.0f, 1.0f);
            };
            for (int i = 0; i < kFieldCount; ++i)
            {
                const Spec& s = kSpecs[i];
                const float value = base[i];
                if (juce::String (s.id) == "keyAmount")
                {
                    layout.add (std::make_unique<AudioParameterFloat> (
                        ParameterID { s.id, 1 }, s.title,
                        NormalisableRange<float> (s.lo, s.hi, s.step), value,
                        AudioParameterFloatAttributes()
                            .withLabel ("%")
                            .withStringFromValueFunction (keyText)
                            .withValueFromStringFunction (keyValue)));
                    continue;
                }
                if (s.kind == 1)
                {
                    layout.add (std::make_unique<AudioParameterBool> (
                        ParameterID { s.id, 1 }, s.title, value >= 0.5f));
                }
                else if (s.kind == 2)
                {
                    int count = 0;
                    const char* const* names = choiceNames ((Field) i, count);
                    StringArray choices;
                    for (int c = 0; c < count; ++c)
                        choices.add (names[c]);
                    layout.add (std::make_unique<AudioParameterChoice> (
                        ParameterID { s.id, 1 }, s.title, choices,
                        juce::jlimit (0, juce::jmax (0, count - 1), (int) std::lround (value))));
                }
                else
                {
                    NormalisableRange<float> range (s.lo, s.hi, s.step);
                    if (s.log != 0)
                        range.setSkewForCentre (s.centre);
                    layout.add (std::make_unique<AudioParameterFloat> (
                        ParameterID { s.id, 1 }, s.title, range, value, s.unit));
                }
            }
            return layout;
        }

        juce::String snapsText (const Targets& t)
        {
            juce::String text;
            const float* data = fieldData (t);
            for (int i = 0; i < kFieldCount; ++i)
            {
                if (i > 0)
                    text << " ";
                text << juce::String (data[i], 6);
            }
            return text;
        }

        bool snapsFrom (const juce::String& text, Targets& t)
        {
            auto parts = juce::StringArray::fromTokens (text, " ", "");
            parts.removeEmptyStrings();
            constexpr int kLegacyFields = 98;
            if (parts.size() < kLegacyFields)
                return false;
            t = defaults();
            float* data = fieldData (t);
            const int n = std::min (parts.size(), kFieldCount);
            for (int i = 0; i < n; ++i)
                data[i] = parts[i].getFloatValue();
            return true;
        }
    }

    KickInstrument::KickInstrument()
        : juce::AudioPluginInstance (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "Spher8Kick", createLayout())
    {
        sample.assign ((size_t) (96000 * 4), 0.0f);
        formats.registerBasicFormats();
        const Targets init = defaults();
        engine.setSnapshots (init, init);
        for (int i = 0; i < kFieldCount; ++i)
            raw[(size_t) i] = apvts.getRawParameterValue (kSpecs[i].id);
    }

    KickInstrument::~KickInstrument() = default;

    juce::PluginDescription KickInstrument::description()
    {
        juce::PluginDescription d;
        d.name = "Spher8 KICK";
        d.descriptiveName = "Spher8 KICK synthesiser";
        d.pluginFormatName = "Atrium";
        d.category = "Drum";
        d.manufacturerName = "Spher8";
        d.version = "1.0.0";
        d.fileOrIdentifier = kIdentifier;
        d.uniqueId = d.deprecatedUid = 0x41504B31;
        d.isInstrument = true;
        d.numInputChannels = 0;
        d.numOutputChannels = 2;
        return d;
    }

    bool KickInstrument::isKickIdentifier (const juce::String& fileOrIdentifier)
    {
        return fileOrIdentifier == kIdentifier
            || fileOrIdentifier.endsWithIgnoreCase ("spher8-kick");
    }

    void KickInstrument::fillInPluginDescription (juce::PluginDescription& d) const
    {
        d = description();
    }

    bool KickInstrument::isBusesLayoutSupported (const BusesLayout& layout) const
    {
        const auto out = layout.getMainOutputChannelSet();
        return layout.inputBuses.isEmpty()
            && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
    }

    void KickInstrument::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
    {
        preparedRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
        engine.prepare (preparedRate, maximumExpectedSamplesPerBlock);
        setLatencySamples (0);
        prepared = true;
        if (queuedSample.load (std::memory_order_acquire) == 1
            && juce::MessageManager::existsAndIsCurrentThread())
            loadSampleFile (juce::File (queuedPath.data()));
    }

    double KickInstrument::getTailLengthSeconds() const
    {
        const float decayMs = raw[(size_t) decay] != nullptr ? raw[(size_t) decay]->load() : 260.0f;
        const float releaseMs = raw[(size_t) release] != nullptr ? raw[(size_t) release]->load() : 40.0f;
        return 0.05 + (double) decayMs * 0.001 + (double) releaseMs * 0.001;
    }

    Targets KickInstrument::readTargets() const noexcept
    {
        Targets t = defaults();
        float* data = fieldData (t);
        for (int i = 0; i < kFieldCount; ++i)
            if (raw[(size_t) i] != nullptr)
                data[i] = raw[(size_t) i]->load();
        return t;
    }

    void KickInstrument::writeTargets (const Targets& t, bool notifyHost)
    {
        const float* data = fieldData (t);
        for (int i = 0; i < kFieldCount; ++i)
        {
            if (i == (int) morph || i == (int) inputNote)
                continue;
            auto* param = apvts.getParameter (kSpecs[i].id);
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
            if (ranged == nullptr)
                continue;
            const float norm = ranged->convertTo0to1 (data[i]);
            if (notifyHost)
                ranged->setValueNotifyingHost (norm);
            else
                ranged->setValue (norm);
        }
    }

    void KickInstrument::setCurrentProgram (int index)
    {
        program = juce::jlimit (0, kPresetCount - 1, index);
        const Targets preset = presetTargets (program);
        const bool notify = juce::MessageManager::existsAndIsCurrentThread();
        writeTargets (preset, notify);
        engine.requestSnapshot (3, preset);
    }

    const juce::String KickInstrument::getProgramName (int index)
    {
        return presetName (index);
    }

    void KickInstrument::audition (int midiNote) noexcept
    {
        auditionNote.store (juce::jlimit (0, 127, midiNote), std::memory_order_release);
    }

    void KickInstrument::setTriggerHook (std::function<void (int, int, int)> hook)
    {
        triggerHook = std::move (hook);
    }

    void KickInstrument::performTrigger (int midiNote, int velocity, int channel)
    {
        midiNote = juce::jlimit (0, 127, midiNote);
        velocity = juce::jlimit (1, 127, velocity);
        channel = juce::jlimit (1, 16, channel);
        audition (midiNote);
        if (triggerHook)
            triggerHook (midiNote, velocity, channel);
    }

    void KickInstrument::storeSnapshot (int which)
    {
        engine.requestSnapshot (which, readTargets());
    }

    juce::String KickInstrument::samplePath() const
    {
        return loadedSample;
    }

    void KickInstrument::clearSample() noexcept
    {
        sampleEpoch.store (1, std::memory_order_release);
        sampleFrames.store (0, std::memory_order_release);
        sampleEpoch.store (2, std::memory_order_release);
        loadedSample.clear();
    }

    void KickInstrument::publishSample (const float* data, int frames, int channels, double fileRate)
    {
        frames = juce::jlimit (0, (int) sample.size(), frames);
        channels = juce::jmax (1, channels);
        sampleEpoch.store (1, std::memory_order_release);
        sampleFrames.store (0, std::memory_order_release);
        for (int i = 0; i < frames; ++i)
        {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += data[(size_t) i * (size_t) channels + (size_t) c];
            sample[(size_t) i] = sum / (float) channels;
        }
        sampleRateHz.store ((float) (fileRate > 1000.0 ? fileRate : 48000.0), std::memory_order_relaxed);
        sampleFrames.store (frames, std::memory_order_release);
        sampleEpoch.store (2, std::memory_order_release);
    }

    void KickInstrument::loadSampleFile (const juce::File& file)
    {
        if (! juce::MessageManager::existsAndIsCurrentThread())
        {
            const auto utf8 = file.getFullPathName().toRawUTF8();
            int n = 0;
            while (utf8[n] != 0 && n < (int) queuedPath.size() - 1)
            {
                queuedPath[(size_t) n] = utf8[n];
                ++n;
            }
            queuedPath[(size_t) n] = 0;
            queuedSample.store (1, std::memory_order_release);
            return;
        }
        queuedSample.store (0, std::memory_order_relaxed);
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            return;
        const int frames = (int) juce::jmin ((juce::int64) sample.size(), reader->lengthInSamples);
        juce::AudioBuffer<float> buffer ((int) juce::jmax (1, (int) reader->numChannels), frames);
        reader->read (&buffer, 0, frames, 0, true, true);
        std::vector<float> interleaved ((size_t) frames * (size_t) buffer.getNumChannels());
        for (int i = 0; i < frames; ++i)
            for (int c = 0; c < buffer.getNumChannels(); ++c)
                interleaved[(size_t) i * (size_t) buffer.getNumChannels() + (size_t) c] = buffer.getSample (c, i);
        publishSample (interleaved.data(), frames, buffer.getNumChannels(), reader->sampleRate);
        loadedSample = file.getFullPathName();
    }

    void KickInstrument::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;
        if (! prepared)
        {
            buffer.clear();
            return;
        }

        Hit hits[128];
        int nHits = 0;
        const int audition = auditionNote.exchange (-1, std::memory_order_acq_rel);
        if (audition >= 0 && nHits < 128)
            hits[nHits++] = { 0, 1, audition, 0.92f, 0 };

        for (const auto meta : midi)
        {
            if (nHits >= 128)
                break;
            const auto message = meta.getMessage();
            const int at = juce::jlimit (0, juce::jmax (0, buffer.getNumSamples() - 1), meta.samplePosition);
            if (message.isAllNotesOff() || message.isAllSoundOff())
                hits[nHits++] = { at, 2, 0, 0.0f, message.getChannel() };
            else if (message.isNoteOn())
                hits[nHits++] = { at, 1, message.getNoteNumber(), message.getFloatVelocity(), message.getChannel() };
            else if (message.isNoteOff())
                hits[nHits++] = { at, 0, message.getNoteNumber(), 0.0f, message.getChannel() };
        }

        SampleView view;
        if ((sampleEpoch.load (std::memory_order_acquire) & 1) == 0)
        {
            view.data = sample.data();
            view.frames = sampleFrames.load (std::memory_order_acquire);
            view.rate = sampleRateHz.load (std::memory_order_acquire);
        }

        engine.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(),
                        readTargets(), hits, nHits, view);
    }

    void KickInstrument::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
    {
        buffer.clear();
    }

    void KickInstrument::getStateInformation (juce::MemoryBlock& dest)
    {
        auto tree = apvts.copyState();
        tree.setProperty ("program", program, nullptr);
        tree.setProperty ("sample", loadedSample, nullptr);
        Targets a, b;
        engine.readSnapshots (a, b);
        tree.setProperty ("snapA", snapsText (a), nullptr);
        tree.setProperty ("snapB", snapsText (b), nullptr);
        juce::MemoryOutputStream stream (dest, false);
        tree.writeToStream (stream);
    }

    void KickInstrument::setStateInformation (const void* data, int sizeInBytes)
    {
        auto tree = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
        if (! tree.isValid())
            return;
        program = juce::jlimit (0, kPresetCount - 1, (int) tree.getProperty ("program", program));
        apvts.replaceState (tree);
        Targets a, b;
        const bool gotA = snapsFrom (tree.getProperty ("snapA").toString(), a);
        const bool gotB = snapsFrom (tree.getProperty ("snapB").toString(), b);
        if (gotA && gotB)
        {
            engine.requestSnapshot (1, a);
            engine.requestSnapshot (2, b);
        }
        const auto path = tree.getProperty ("sample").toString();
        if (path.isNotEmpty())
            loadSampleFile (juce::File (path));
    }

    juce::AudioProcessorEditor* KickInstrument::createEditor()
    {
        return new KickEditor (*this);
    }

    int KickInstrument::runSelfTest (juce::String& report)
    {
        auto fail = [&] (const juce::String& why)
        {
            report += "FAIL " + why + "\n";
            return 1;
        };

        const auto desc = description();
        if (! desc.isInstrument || desc.numInputChannels != 0 || desc.numOutputChannels != 2)
            return fail ("description");
        if (! isKickIdentifier (desc.fileOrIdentifier))
            return fail ("identifier");
        if (isKickIdentifier ("atrium:salamander-grand-piano-v3"))
            return fail ("piano id collided");

        KickInstrument kick;
        juce::AudioProcessor::BusesLayout layout;
        layout.outputBuses.add (juce::AudioChannelSet::stereo());
        if (! kick.setBusesLayout (layout))
            return fail ("buses");
        if (! kick.acceptsMidi() || kick.producesMidi() || kick.isMidiEffect())
            return fail ("midi flags");
        if (kick.getNumPrograms() < 32)
            return fail ("programs");
        kick.prepareToPlay (48000.0, 256);

        bool sawTune = false, sawMorph = false, sawDecay = false, sawOutput = false;
        for (auto* param : kick.getParameters())
        {
            if (param == nullptr)
                continue;
            const auto name = param->getName (48);
            if (name == "Tune") sawTune = true;
            if (name == "Morph") sawMorph = true;
            if (name == "Decay") sawDecay = true;
            if (name == "Output") sawOutput = true;
        }
        if (! sawTune || ! sawMorph || ! sawDecay || ! sawOutput)
            return fail ("parameter names");

        const float before = kick.readTargets().tune;
        kick.setCurrentProgram (16);
        if (std::abs (kick.readTargets().tune - before) < 0.01f
            && std::abs (kick.readTargets().decay - defaults().decay) < 1.0f)
            return fail ("program did not change the patch");

        juce::MemoryBlock state;
        kick.getStateInformation (state);
        kick.setCurrentProgram (0);
        kick.setStateInformation (state.getData(), (int) state.getSize());
        if (kick.getCurrentProgram() != 16)
            return fail ("state program");

        juce::AudioBuffer<float> audio (2, 256);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);
        float peak = 0.0f;
        for (int i = 0; i < 40; ++i)
        {
            if (i > 0)
                midi.clear();
            audio.clear();
            kick.processBlock (audio, midi);
            peak = std::max (peak, audio.getMagnitude (0, audio.getNumSamples()));
        }
        if (peak < 0.02f)
            return fail ("silent instrument");

        auto* editor = kick.createEditor();
        if (editor == nullptr)
            return fail ("editor");
        editor->setSize (980, 700);
        juce::Image frame (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
        juce::Graphics g (frame);
        editor->paint (g);
        delete editor;

        report += "KICK host ok peak " + juce::String (peak, 3) + "\n";
        return 0;
    }
}
