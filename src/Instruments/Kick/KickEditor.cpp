#include "KickEditor.h"

#include <cmath>

namespace atrium::kick
{
    namespace
    {
        const juce::Colour gem (0xff4EB8D0);
        const juce::Colour amber (0xffe0a84a);
        const juce::Colour ink (0xffe8eef3);
        const juce::Colour mute (0xff9aa6b2);
        const juce::Colour steelHi (0xff3a424c);
        const juce::Colour steelLo (0xff101216);

        const char* kPrimaryId[] = { "tune", "punch", "decay", "drive", "clickLevel", "subLevel", "morph" };
        const char* kPrimaryName[] = { "TUNE", "PUNCH", "DECAY", "DRIVE", "CLICK", "SUB", "MORPH" };
        const char* kPitchId[] = { "impactSt", "impactMs", "bodySt", "bodyMs", "impactCurve", "bodyCurve" };
        const char* kPitchName[] = { "IMPACT", "IMPACT MS", "BODY", "BODY MS", "I CURVE", "B CURVE" };
        const char* kAmpId[] = { "attack", "sustain", "release", "clickMs", "clickHz", "clickPos" };
        const char* kAmpName[] = { "ATTACK", "SUSTAIN", "RELEASE", "CLICK MS", "CLICK HZ", "OFFSET" };
        const char* kHarmId[] = { "evenOdd", "fold", "asym", "tone", "cutoff", "hp" };
        const char* kHarmName[] = { "EVEN/ODD", "FOLD", "ASYM", "TONE", "CUTOFF", "HP" };
        const char* kDynId[] = { "atkEmph", "susShape", "comp", "thresh", "ratio", "clip", "ceiling", "output" };
        const char* kDynName[] = { "ATTACK", "SUSTAIN", "COMP", "THRESH", "RATIO", "CLIP", "CEILING", "OUTPUT" };
        const char* kOutId[] = { "semi", "fine", "a4", "subHz", "subPhase", "bodyHarm" };
        const char* kOutName[] = { "SEMI", "FINE", "A4", "SUB HZ", "SUB PHASE", "HARM" };
        const char* kFollowId[] = { "followLevel", "followDrive", "followSub", "followMs", "followCurve", "followRes" };
        const char* kFollowName[] = { "SUB DEP", "DRV DEP", "PIT DEP", "SPAN", "CURVE", "HARM" };
        const char* kLayerId[] = { "bodyOn", "subOn", "clickOn", "layerOn" };
        const char* kLayerName[] = { "BODY", "SUB", "CLICK", "LAYER" };
        const char* kViewName[] = { "WAVE", "PITCH", "SPECTRUM", "LAYERS", "FOLLOW" };
        const char* kSectionName[] = { "PITCH", "AMP", "HARMONICS", "DYNAMICS", "OUTPUT" };
        const char* kDestName[] = { "SUB", "DRIVE", "HARM" };
        const char* kLaneName[] = { "POST AMP", "PITCH" };
        constexpr int kViewCount = 5;
        constexpr int kFollowView = 4;

        // KICK-only layout. Shared widgets stay untouched; these sizes are what
        // the full and compact windows are summed from.
        constexpr int kPadX = 16;
        constexpr int kPadY = 8;
        constexpr int kHeaderH = 48;
        constexpr int kHeaderGap = 4;
        constexpr int kAfterScreen = 6;
        constexpr int kNavH = 30;
        constexpr int kNavGap = 6;
        constexpr int kBeforeKnobs = 6;
        constexpr int kDialH = 108;
        constexpr int kKnobGap = 8;
        constexpr int kRangeH = 34;
        constexpr int kAboveRange = 6;
        constexpr int kPerfH = 30;
        constexpr int kScreenFull = 200;
        constexpr int kScreenCompact = 220;
        constexpr int kDialCompact = 116;
        constexpr int kFollowDialH = 88;
        constexpr int kFollowChipH = 30;
        constexpr int kWidthFull = 980;
        constexpr int kWidthCompact = 860;

        juce::String formatKnob (double value, const juce::String& unit)
        {
            if (unit == "Hz")
                return juce::String (value, value >= 100.0 ? 0 : 1);
            if (unit == "ms")
                return juce::String (value, value >= 100.0 ? 0 : 1);
            if (unit == "st" || unit == "dB")
                return juce::String (value, 1);
            if (unit == "ct")
                return juce::String (value, 0);
            return juce::String (value, 2);
        }

        juce::String formatHz (float hz)
        {
            return juce::String (hz, hz >= 100.0f ? 0 : 1);
        }

        void prep (juce::Button& chip, const juce::String& text, bool toggle)
        {
            chip.setButtonText (text);
            chip.setClickingTogglesState (toggle);
        }

        juce::String noteLabel (int note)
        {
            static const char* kName[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            note = juce::jlimit (0, 127, note);
            auto text = juce::String (kName[note % 12]) + juce::String (note / 12 - 1);
            if (note == 36)
                text << " KICK";
            return text;
        }
    }

    void KickEditor::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                    juce::Slider& slider)
    {
        const bool compact = slider.getProperties()["compact"];
        const float inset = compact ? 1.5f : 4.0f;
        const float insetY = compact ? 1.5f : 16.0f;
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (inset, insetY);
        const float side = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto disc = juce::Rectangle<float> (side, side).withCentre (bounds.getCentre());
        if (disc.getWidth() < 12.0f)
            return;
        const auto centre = disc.getCentre();
        const float radius = disc.getWidth() * 0.5f;
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcRadius = radius * 0.84f;

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (centre.x - radius * 0.72f, centre.y - radius * 0.72f + 3.0f, radius * 1.44f, radius * 1.44f);
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff2c343c));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (sliderPos > 0.004f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour (compact ? amber : gem);
            g.strokePath (value, juce::PathStrokeType (compact ? 2.2f : 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const float hub = radius * 0.66f;
        juce::ColourGradient body (juce::Colour (0xff727a84), centre.x, centre.y - hub,
                                   juce::Colour (0xff14181d), centre.x, centre.y + hub, false);
        g.setGradientFill (body);
        g.fillEllipse (centre.x - hub, centre.y - hub, hub * 2.0f, hub * 2.0f);
        g.setColour (juce::Colour (0xffa8b2bc).withAlpha (0.45f));
        g.drawEllipse (centre.x - hub, centre.y - hub, hub * 2.0f, hub * 2.0f, 1.1f);
        const auto tip = centre.getPointOnCircumference (hub * 0.86f, angle);
        const auto root = centre.getPointOnCircumference (hub * 0.18f, angle);
        g.setColour (compact ? amber : gem);
        g.drawLine (root.x, root.y, tip.x, tip.y, compact ? 1.4f : 1.8f);
        g.fillEllipse (tip.x - 2.4f, tip.y - 2.4f, 4.8f, 4.8f);
    }

    KickEditor::Dial::Dial (const juce::String& id, const juce::String& name,
                            juce::AudioProcessorValueTreeState& state, LookAndFeel& feel, bool isCompact)
        : caption (name), paramId (id), compact (isCompact)
    {
        addAndMakeVisible (slider);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&feel);
        slider.setWantsKeyboardFocus (false);
        if (compact)
            slider.getProperties().set ("compact", true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
        if (auto* parameter = state.getParameter (id))
            unit = parameter->getLabel();
        if (compact)
        {
            slider.setTooltip ("Spread the main dials across the note range. 0% plays the designed kick on every note.");
            slider.onValueChange = [this] { repaint(); };
        }
    }

    KickEditor::Dial::~Dial()
    {
        attachment.reset();
        slider.setLookAndFeel (nullptr);
    }

    void KickEditor::Dial::resized()
    {
        if (compact)
        {
            auto bounds = getLocalBounds();
            slider.setBounds (bounds.removeFromLeft (juce::jmin (30, bounds.getWidth())));
        }
        else
            slider.setBounds (getLocalBounds().reduced (0, 2));
    }

    void KickEditor::Dial::paint (juce::Graphics& g)
    {
        if (compact)
        {
            auto text = getLocalBounds().toFloat().withTrimmedLeft ((float) slider.getWidth() + 2.0f);
            g.setColour (amber);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.12f));
            g.drawText ("KEY", text.removeFromTop (text.getHeight() * 0.46f), juce::Justification::centredLeft);
            g.setColour (ink);
            g.setFont (juce::Font (juce::FontOptions (12.0f)));
            const int percent = juce::roundToInt (slider.getValue() * 100.0);
            g.drawText (juce::String (percent) + "%", text, juce::Justification::centredLeft);
            return;
        }
        auto area = getLocalBounds();
        auto value = area.removeFromBottom (16);
        auto name = area.removeFromBottom (14);
        g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.08f));
        g.setColour (mute);
        g.drawText (caption, name, juce::Justification::centred);
        g.setColour (ink);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText (formatKnob (slider.getValue(), unit), value, juce::Justification::centred);
    }

    KickEditor::Chip::Chip() : juce::Button ({})
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (false);
    }

    void KickEditor::Chip::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const bool on = getToggleState();
        juce::Colour accent = gem;
        bool fillOff = true;
        juce::Colour offText (0xffd7dee6);
        juce::Colour offStroke = gem.withAlpha (highlighted ? 0.8f : 0.38f);
        if (role == 1 || role == 6)
        {
            accent = amber;
            offText = amber.brighter (0.08f);
            offStroke = amber.withAlpha (highlighted ? 0.95f : 0.62f);
        }
        else if (role == 2)
        {
            fillOff = false;
            offText = mute;
            offStroke = juce::Colour (0xff6a7380).withAlpha (highlighted ? 0.95f : 0.7f);
        }
        else if (role == 3 || role == 4)
        {
            accent = amber;
            fillOff = false;
            offText = amber;
            offStroke = amber.withAlpha (highlighted ? 0.95f : 0.7f);
        }
        else if (role == 5)
        {
            offText = gem.brighter (0.12f);
            offStroke = gem.withAlpha (highlighted ? 0.95f : 0.7f);
        }
        if (on)
        {
            g.setColour (accent.withAlpha (0.22f));
            g.fillRoundedRectangle (bounds.expanded (1.2f), 6.0f);
            g.setColour (down ? accent.darker (0.1f) : accent);
            g.fillRoundedRectangle (bounds, 5.0f);
            g.setColour (juce::Colour (0xff061014));
        }
        else
        {
            auto fill = fillOff ? juce::Colour (0xff171c22) : juce::Colour (0xff10141a);
            if (highlighted) fill = fill.brighter (0.08f);
            if (down) fill = fill.darker (0.08f);
            g.setColour (fillOff ? fill : fill.withAlpha (0.45f));
            g.fillRoundedRectangle (bounds, 5.0f);
            g.setColour (offStroke);
            g.drawRoundedRectangle (bounds, 5.0f, 1.1f);
            g.setColour (offText);
        }
        g.setFont (juce::Font (juce::FontOptions (getHeight() >= 26 ? 11.0f : 10.0f)).withExtraKerningFactor (0.04f));
        g.drawFittedText (getButtonText(), getLocalBounds().reduced (2, 0), juce::Justification::centred, 1);
    }

    KickEditor::KickEditor (KickInstrument& instrument)
        : juce::AudioProcessorEditor (instrument), kick (instrument), screen (*this),
          rangeBar (instrument.apvts, "noteLo", "noteHi",
                    "Notes from the low handle through the high handle trigger the kick. Notes outside are ignored, so a drum kit on this track can play them.")
    {
       #if defined (ATRIUM_SOURCE_DIR)
        const juce::File root (ATRIUM_SOURCE_DIR);
        const auto word = root.getChildFile ("apps/studio/Resources/Fonts/Audiowide-Regular.ttf");
        const auto face = root.getChildFile ("apps/studio/Resources/Fonts/SairaSemiBold.ttf");
        juce::MemoryBlock wordData, faceData;
        if (word.existsAsFile() && word.loadFileAsData (wordData))
            wordmark = juce::Typeface::createSystemTypefaceFor (wordData.getData(), wordData.getSize());
        if (face.existsAsFile() && face.loadFileAsData (faceData))
            brand = juce::Typeface::createSystemTypefaceFor (faceData.getData(), faceData.getSize());
       #endif

        addAndMakeVisible (screen);
        addAndMakeVisible (rangeBar);
        keyDial = std::make_unique<Dial> ("keyAmount", "KEY", kick.apvts, look, true);
        addAndMakeVisible (keyDial.get());
        auto addGroup = [this] (juce::OwnedArray<Dial>& group, const char* const* ids,
                                const char* const* names, int count)
        {
            for (int i = 0; i < count; ++i)
                addAndMakeVisible (group.add (new Dial (ids[i], names[i], kick.apvts, look)));
        };
        addGroup (primary, kPrimaryId, kPrimaryName, 7);
        addGroup (pitchKnobs, kPitchId, kPitchName, 6);
        addGroup (ampKnobs, kAmpId, kAmpName, 6);
        addGroup (harmKnobs, kHarmId, kHarmName, 6);
        addGroup (dynKnobs, kDynId, kDynName, 8);
        addGroup (outKnobs, kOutId, kOutName, 6);
        addGroup (followKnobs, kFollowId, kFollowName, 6);

        prep (powerChip, "ON", true);
        prep (compactChip, "COMPACT", true);
        prep (saveChip, "SAVE", false);
        prep (prevChip, "<", false);
        prep (nextChip, ">", false);
        prep (nameChip, "INIT", false);
        prep (auditionChip, "AUDITION", false);
        prep (noteInChip, "NOTE ANY", false);
        prep (channelChip, "CH ANY", false);
        prep (triggerChip, "TRIGGER", false);
        prep (storeA, "STORE A", false);
        prep (storeB, "STORE B", false);
        for (int i = 0; i < 4; ++i)
        {
            prep (layerChip[i], kLayerName[i], true);
            layerChip[i].role = 0;
            addAndMakeVisible (layerChip[i]);
            layerAttach[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                kick.apvts, kLayerId[i], layerChip[i]);
        }
        for (int i = 0; i < kViewCount; ++i)
        {
            prep (viewChip[i], kViewName[i], false);
            viewChip[i].role = 1;
            addAndMakeVisible (viewChip[i]);
            viewChip[i].onClick = [this, i]
            {
                view = i;
                screen.dragHandle = -1;
                showSection (section);
            };
        }
        for (int i = 0; i < 3; ++i)
        {
            prep (destChip[i], kDestName[i], false);
            destChip[i].role = 3;
            addAndMakeVisible (destChip[i]);
            destChip[i].onClick = [this, i]
            {
                const Field field = i == 0 ? followLevel : (i == 1 ? followDrive : followRes);
                const float amount = param (field);
                setParam (field, amount > 0.02f ? 0.0f : 0.75f);
            };
        }
        destChip[0].setTooltip ("Route the post-kick amp envelope to the sub level. Lit means SUB DEP is up. Depth 0 leaves the sub alone.");
        destChip[1].setTooltip ("Route the same post-kick amp envelope into drive, so the loud parts of the boost pick up grit.");
        destChip[2].setTooltip ("Route the post-kick amp envelope into sub harmonics. Depth 0 leaves the sine bare.");
        for (int i = 0; i < kShapeCount; ++i)
        {
            prep (shapeChip[i], kShapeNames[i], false);
            shapeChip[i].role = 4;
            addAndMakeVisible (shapeChip[i]);
            shapeChip[i].onClick = [this, i] { setChoice (followShape, i); };
        }
        for (int i = 0; i < 2; ++i)
        {
            prep (laneChip[i], kLaneName[i], false);
            laneChip[i].role = i == 0 ? 5 : 6;
            addAndMakeVisible (laneChip[i]);
            laneChip[i].onClick = [this, i]
            {
                followLane = i;
                screen.dragHandle = -1;
                laneChip[0].setToggleState (i == 0, juce::dontSendNotification);
                laneChip[1].setToggleState (i == 1, juce::dontSendNotification);
                repaint();
            };
        }
        laneChip[0].setTooltip ("Post-kick sub boost after the impact. Not the body AMP page. SUB DEP scales it. Drive and harmonics follow this curve when their depths are up.");
        laneChip[1].setTooltip ("Sub pitch offset across the same post-kick window. PIT DEP scales it. This lane does not change level.");
        laneChip[0].setToggleState (true, juce::dontSendNotification);
        viewChip[kFollowView].setTooltip ("Post-kick window after the impact. POST AMP boosts the sub. PITCH offsets the sub. The ruler is the span in milliseconds.");
        for (int i = 0; i < 5; ++i)
        {
            prep (sectionChip[i], kSectionName[i], false);
            sectionChip[i].role = 2;
            addAndMakeVisible (sectionChip[i]);
            sectionChip[i].onClick = [this, i] { showSection (i); };
        }
        sectionChip[1].setTooltip ("Body amplitude envelope: attack, sustain, release. Not the post-kick POST AMP lane.");
        for (int i = 0; i < kWaveCount; ++i)
        {
            prep (waveChip[i], kWaveNames[i], false);
            addAndMakeVisible (waveChip[i]);
            waveChip[i].onClick = [this, i] { setChoice (bodyWave, i); };
        }
        for (auto* chip : { &powerChip, &compactChip, &saveChip, &prevChip, &nextChip, &nameChip,
                            &auditionChip, &noteInChip, &channelChip, &triggerChip, &storeA, &storeB })
            addAndMakeVisible (chip);

        powerAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            kick.apvts, "enabled", powerChip);
        compactChip.onClick = [this]
        {
            compactMode = compactChip.getToggleState();
            applyWindowLimits();
            setSize (compactMode ? kWidthCompact : kWidthFull, preferredHeight());
            showSection (section);
        };
        prevChip.onClick = [this] { stepPreset (-1); };
        nextChip.onClick = [this] { stepPreset (1); };
        nameChip.onClick = [this] { showPresetMenu(); };
        saveChip.onClick = [this] { askSavePreset(); };
        auditionChip.onClick = [this] { kick.audition (triggerNote()); };
        noteInChip.onClick = [this] { showInputNoteMenu(); };
        channelChip.onClick = [this] { showChannelMenu(); };
        triggerChip.onClick = [this] { fireTrigger(); };
        noteInChip.setTooltip ("Single MIDI note inside the range. ANY plays every note in the range. C2 is note 36, the Matrix Kick pad.");
        channelChip.setTooltip ("MIDI channel. ANY listens to every channel. Match this to the Matrix pad channel.");
        triggerChip.setTooltip ("Play the input note. When this track is record-armed, the same note is written into the clip.");
        auditionChip.setTooltip ("Play the input note inside the plug-in, without writing it to the track.");
        storeA.onClick = [this] { kick.storeSnapshot (1); };
        storeB.onClick = [this] { kick.storeSnapshot (2); };
        viewChip[0].setToggleState (true, juce::dontSendNotification);
        applyWindowLimits();
        setSize (kWidthFull, preferredHeight());
        showSection (0);
        startTimerHz (30);
    }

    KickEditor::~KickEditor()
    {
        stopTimer();
        powerAttach.reset();
        for (auto& attach : layerAttach)
            attach.reset();
        keyDial.reset();
        primary.clear();
        pitchKnobs.clear();
        ampKnobs.clear();
        harmKnobs.clear();
        dynKnobs.clear();
        outKnobs.clear();
        followKnobs.clear();
    }

    Targets KickEditor::shownTargets() const
    {
        Targets t = defaults();
        float* data = fieldData (t);
        const auto& params = kick.getParameters();
        const int n = juce::jmin (kFieldCount, params.size());
        for (int i = 0; i < n; ++i)
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[i]))
                data[i] = ranged->convertFrom0to1 (ranged->getValue());
        return t;
    }

    float KickEditor::param (Field field) const
    {
        return fieldData (shownTargets())[field];
    }

    void KickEditor::setParam (Field field, float value)
    {
        const auto& params = kick.getParameters();
        if (! juce::isPositiveAndBelow (field, params.size()))
            return;
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (params[(int) field]))
            ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
    }

    void KickEditor::commitAmpFromPitch()
    {
        const auto t = shownTargets();
        if (t.followAmpUse >= 0.5f)
            return;
        setParam (followAmpUse, 1.0f);
        setParam (followAmpT1, t.followT1);
        setParam (followAmpT2, t.followT2);
        const float st[4] = { t.followSt0, t.followSt1, t.followSt2, t.followSt3 };
        const Field fields[4] = { followAmp0, followAmp1, followAmp2, followAmp3 };
        for (int i = 0; i < 4; ++i)
            setParam (fields[i], clampf (st[i] / 12.0f, 0.0f, 1.0f));
    }

    void KickEditor::setChoice (Field field, int index)
    {
        setParam (field, (float) index);
    }

    int KickEditor::triggerNote() const
    {
        const int coded = (int) std::lround (param (inputNote));
        if (coded >= 1 && coded <= 128)
            return coded - 1;
        return 36;
    }

    int KickEditor::triggerChannel() const
    {
        const int heard = (int) std::lround (param (Field::channel));
        return heard >= 1 && heard <= 16 ? heard : 1;
    }

    void KickEditor::refreshInputChips()
    {
        const int coded = (int) std::lround (param (inputNote));
        const auto noteText = (coded >= 1 && coded <= 128) ? noteLabel (coded - 1) : juce::String ("ANY");
        if (noteInChip.getButtonText() != noteText)
            noteInChip.setButtonText (noteText);
        const int heard = (int) std::lround (param (Field::channel));
        const auto channelText = (heard >= 1 && heard <= 16) ? ("CH " + juce::String (heard))
                                                             : juce::String ("CH ANY");
        if (channelChip.getButtonText() != channelText)
            channelChip.setButtonText (channelText);
    }

    void KickEditor::showInputNoteMenu()
    {
        juce::PopupMenu menu;
        const int coded = (int) std::lround (param (inputNote));
        menu.addItem (1, "Any note", true, coded <= 0 || coded > 128);
        for (int oct = -1; oct <= 9; ++oct)
        {
            juce::PopupMenu sub;
            for (int pc = 0; pc < 12; ++pc)
            {
                const int note = (oct + 1) * 12 + pc;
                if (note > 127)
                    break;
                sub.addItem (100 + note, noteLabel (note), true, coded == note + 1);
            }
            if (sub.getNumItems() > 0)
                menu.addSubMenu (juce::String (oct), sub);
        }
        auto safe = juce::Component::SafePointer<KickEditor> (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&noteInChip),
                            [safe] (int result)
                            {
                                if (safe == nullptr || result <= 0)
                                    return;
                                if (result == 1)
                                    safe->setParam (inputNote, 0.0f);
                                else if (result >= 100 && result <= 227)
                                    safe->setParam (inputNote, (float) (result - 100 + 1));
                                if (safe != nullptr)
                                    safe->refreshInputChips();
                            });
    }

    void KickEditor::showChannelMenu()
    {
        juce::PopupMenu menu;
        const int heard = (int) std::lround (param (Field::channel));
        menu.addItem (1, "Any channel", true, heard < 1 || heard > 16);
        for (int n = 1; n <= 16; ++n)
            menu.addItem (1 + n, "Channel " + juce::String (n), true, heard == n);
        auto safe = juce::Component::SafePointer<KickEditor> (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&channelChip),
                            [safe] (int result)
                            {
                                if (safe == nullptr || result <= 0)
                                    return;
                                safe->setParam (Field::channel, result == 1 ? 0.0f : (float) (result - 1));
                                safe->refreshInputChips();
                            });
    }

    void KickEditor::fireTrigger()
    {
        kick.performTrigger (triggerNote(), 110, triggerChannel());
    }

    void KickEditor::showSection (int next)
    {
        section = juce::jlimit (0, 4, next);
        for (int i = 0; i < 5; ++i)
            sectionChip[i].setToggleState (i == section, juce::dontSendNotification);
        auto show = [this] (juce::OwnedArray<Dial>& group, bool on)
        {
            for (auto* dial : group)
                dial->setVisible (on && ! compactMode);
        };
        show (pitchKnobs, section == 0);
        show (ampKnobs, section == 1);
        show (harmKnobs, section == 2);
        show (dynKnobs, section == 3);
        show (outKnobs, section == 4);
        const bool follow = view == kFollowView;
        for (auto* dial : followKnobs)
            dial->setVisible (follow);
        for (int i = 0; i < kWaveCount; ++i)
            waveChip[i].setVisible (! follow && ! compactMode && section == 0);
        for (int i = 0; i < 3; ++i)
            destChip[i].setVisible (follow);
        for (int i = 0; i < kShapeCount; ++i)
            shapeChip[i].setVisible (follow);
        for (int i = 0; i < 2; ++i)
            laneChip[i].setVisible (follow);
        for (int v = 0; v < kViewCount; ++v)
            viewChip[v].setToggleState (v == view, juce::dontSendNotification);
        const int extra = (! compactMode && view == kFollowView) ? kFollowDialH : 0;
        const int target = preferredHeight() + extra;
        const int delta = getHeight() - target;
        if (delta == kFollowDialH || delta == -kFollowDialH)
            setSize (getWidth(), target);
        else if (extra > 0 && getHeight() < target)
            setSize (getWidth(), getHeight() + extra);
        else
            resized();
    }

    void KickEditor::resetMeter() noexcept
    {
        const float floorDb = -60.0f;
        float now = floorDb;
        if (live.meter > 1.0e-6f && std::isfinite (live.meter))
            now = juce::jlimit (floorDb, 3.0f, 20.0f * std::log10 (live.meter));
        meterShown = now;
        meterHold = now;
        meterClipped = live.meterClip != 0 || now >= -0.05f;
        repaint();
    }

    void KickEditor::timerCallback()
    {
        kick.copyView (live);
        const float floorDb = -60.0f;
        float target = floorDb;
        if (live.meter > 1.0e-6f && std::isfinite (live.meter))
            target = juce::jlimit (floorDb, 3.0f, 20.0f * std::log10 (live.meter));
        if (target >= meterShown)
            meterShown = target;
        else
            meterShown += (target - meterShown) * 0.32f;
        if (meterShown < floorDb)
            meterShown = floorDb;
        if (target > meterHold)
            meterHold = target;
        if (live.meterClip != 0 || target >= -0.05f)
            meterClipped = true;
        const float destAmt[3] = { param (followLevel), param (followDrive), param (followRes) };
        for (int i = 0; i < 3; ++i)
            destChip[i].setToggleState (destAmt[i] > 0.02f, juce::dontSendNotification);
        laneChip[0].setToggleState (followLane == 0, juce::dontSendNotification);
        laneChip[1].setToggleState (followLane == 1, juce::dontSendNotification);
        const int shapeNow = (int) std::lround (param (followShape));
        for (int i = 0; i < kShapeCount; ++i)
            shapeChip[i].setToggleState (i == shapeNow, juce::dontSendNotification);
        refreshInputChips();
        powerChip.setButtonText (powerChip.getToggleState() ? "ON" : "OFF");
        const auto title = kick.getProgramName (kick.getCurrentProgram());
        if (nameChip.getButtonText() != title)
            nameChip.setButtonText (title);
        const int wave = (int) std::lround (param (bodyWave));
        for (int i = 0; i < kWaveCount; ++i)
            waveChip[i].setToggleState (i == wave, juce::dontSendNotification);
        repaint();
    }

    void KickEditor::stepPreset (int delta)
    {
        int next = kick.getCurrentProgram() + delta;
        if (next < 0) next = kPresetCount - 1;
        if (next >= kPresetCount) next = 0;
        kick.setCurrentProgram (next);
        nameChip.setButtonText (kick.getProgramName (next));
    }

    juce::File KickEditor::presetDirectory() const
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("Spher8 Studio").getChildFile ("KICK").getChildFile ("Presets");
    }

    void KickEditor::showPresetMenu()
    {
        juce::PopupMenu menu;
        juce::String last;
        for (int i = 0; i < kPresetCount; ++i)
        {
            const auto cat = juce::String (presetCategory (i));
            if (cat != last)
            {
                menu.addSectionHeader (cat);
                last = cat;
            }
            menu.addItem (i + 1, presetName (i), true, i == kick.getCurrentProgram());
        }
        const auto files = presetDirectory().findChildFiles (juce::File::findFiles, false, "*.kick");
        if (files.size() > 0)
        {
            menu.addSeparator();
            menu.addSectionHeader ("Saved");
            for (int i = 0; i < files.size(); ++i)
                menu.addItem (2000 + i, files[i].getFileNameWithoutExtension());
        }
        auto safe = juce::Component::SafePointer<KickEditor> (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameChip),
                            [safe, files] (int result)
                            {
                                if (safe == nullptr || result == 0)
                                    return;
                                if (result >= 1 && result <= kPresetCount)
                                    safe->kick.setCurrentProgram (result - 1);
                                else if (result >= 2000 && result - 2000 < files.size())
                                    safe->loadPresetFile (files.getReference (result - 2000));
                                if (safe != nullptr)
                                    safe->nameChip.setButtonText (safe->kick.getProgramName (safe->kick.getCurrentProgram()));
                            });
    }

    void KickEditor::askSavePreset()
    {
        auto* window = new juce::AlertWindow ("Save KICK Preset", "Name this preset.", juce::AlertWindow::NoIcon);
        window->addTextEditor ("name", kick.getProgramName (kick.getCurrentProgram()), "Preset name");
        window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window] (int result)
        {
            if (result == 1)
                savePreset (window->getTextEditorContents ("name"));
        }), true);
    }

    void KickEditor::savePreset (const juce::String& name)
    {
        auto clean = name.trim();
        if (clean.isEmpty())
            return;
        auto dir = presetDirectory();
        dir.createDirectory();
        juce::MemoryBlock block;
        kick.getStateInformation (block);
        dir.getChildFile (clean + ".kick").replaceWithData (block.getData(), block.getSize());
    }

    void KickEditor::loadPresetFile (const juce::File& file)
    {
        juce::MemoryBlock block;
        if (file.loadFileAsData (block))
            kick.setStateInformation (block.getData(), (int) block.getSize());
    }

    bool KickEditor::isInterestedInFileDrag (const juce::StringArray& files)
    {
        for (auto& file : files)
            if (file.endsWithIgnoreCase (".wav") || file.endsWithIgnoreCase (".aif") || file.endsWithIgnoreCase (".aiff"))
                return true;
        return false;
    }

    void KickEditor::filesDropped (const juce::StringArray& files, int, int)
    {
        for (auto& file : files)
            if (file.endsWithIgnoreCase (".wav") || file.endsWithIgnoreCase (".aif") || file.endsWithIgnoreCase (".aiff"))
            {
                kick.loadSampleFile (juce::File (file));
                setParam (layerOn, 1.0f);
                return;
            }
    }

    void KickEditor::applyWindowLimits()
    {
        setResizable (true, true);
        if (compactMode)
            setResizeLimits (800, 448, 1800, 1200);
        else
            setResizeLimits (840, 628, 1800, 1200);
    }

    int KickEditor::preferredHeight() const
    {
        int stacked = kHeaderH + kHeaderGap + kAfterScreen + kNavH + kAboveRange + kRangeH + kPerfH;
        int screenH = kScreenFull;
        if (compactMode)
        {
            stacked += kDialCompact;
            screenH = kScreenCompact;
            if (view == kFollowView)
                stacked += kFollowChipH + kFollowDialH;
        }
        else
            stacked += kNavGap + kNavH + kBeforeKnobs + kDialH + kKnobGap + kDialH;
        return kPadY * 2 + stacked + screenH;
    }

    void KickEditor::layoutHeader (juce::Rectangle<int> header)
    {
        auto buttons = header.removeFromBottom (30).reduced (0, 1);
        buttons.removeFromRight (2);
        powerChip.setBounds (buttons.removeFromRight (58).reduced (3, 0));
        compactChip.setBounds (buttons.removeFromRight (96).reduced (3, 0));
        saveChip.setBounds (buttons.removeFromRight (68).reduced (3, 0));
        nextChip.setBounds (buttons.removeFromRight (34).reduced (3, 0));
        nameChip.setBounds (buttons.removeFromRight (196).reduced (3, 0));
        prevChip.setBounds (buttons.removeFromRight (34).reduced (3, 0));
    }

    void KickEditor::resized()
    {
        auto area = getLocalBounds().reduced (kPadX, kPadY);
        layoutHeader (area.removeFromTop (kHeaderH));
        area.removeFromTop (kHeaderGap);

        auto footer = area.removeFromBottom (kRangeH + kPerfH);
        area.removeFromBottom (kAboveRange);
        auto rangeRow = footer.removeFromTop (kRangeH);
        if (keyDial != nullptr)
            keyDial->setBounds (rangeRow.removeFromRight (132).reduced (18, 2));
        rangeBar.setBounds (rangeRow.reduced (0, 2));
        auditionChip.setBounds (footer.removeFromRight (96).reduced (3, 2));
        triggerChip.setBounds (footer.removeFromRight (88).reduced (3, 2));
        channelChip.setBounds (footer.removeFromRight (80).reduced (3, 2));
        noteInChip.setBounds (footer.removeFromRight (88).reduced (3, 2));
        statusArea = footer.reduced (2, 1);

        auto placeDials = [] (juce::Rectangle<int> row, juce::OwnedArray<Dial>& group)
        {
            const int n = juce::jmax (1, group.size());
            const int w = row.getWidth() / n;
            for (int i = 0; i < group.size(); ++i)
            {
                auto cell = (i == group.size() - 1) ? row : row.removeFromLeft (w);
                group.getUnchecked (i)->setBounds (cell.reduced (2, 0));
            }
        };
        auto placeChips = [] (juce::Rectangle<int> strip, Chip* first, int n)
        {
            if (n <= 0 || strip.isEmpty())
                return;
            const int w = strip.getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                auto cell = (i == n - 1) ? strip : strip.removeFromLeft (w);
                first[i].setBounds (cell.reduced (2, 2));
            }
        };

        placeDials (area.removeFromBottom (compactMode ? kDialCompact : kDialH), primary);
        layersTag = viewTag = pageTag = followTag = {};
        navSplitLayer = navSplitStore = knobRuleY = -1;
        navRowBounds = {};
        const bool follow = view == kFollowView;
        if (! compactMode)
        {
            auto knobGap = area.removeFromBottom (kKnobGap);
            knobRuleY = knobGap.getCentreY();
            juce::OwnedArray<Dial>* group = &pitchKnobs;
            if (section == 1) group = &ampKnobs;
            else if (section == 2) group = &harmKnobs;
            else if (section == 3) group = &dynKnobs;
            else if (section == 4) group = &outKnobs;
            placeDials (area.removeFromBottom (kDialH), *group);
            area.removeFromBottom (kBeforeKnobs);
            auto chips = area.removeFromBottom (kNavH);
            if (follow)
            {
                auto side = chips.removeFromRight (juce::jmin (560, juce::jmax (400, chips.getWidth() - 390)));
                followTag = side.removeFromLeft (58);
                const int laneW[2] = { 86, 68 };
                for (int i = 0; i < 2; ++i)
                    laneChip[i].setBounds (side.removeFromLeft (laneW[i]).reduced (2, 2));
                side.removeFromLeft (8);
                const int dw = juce::jmax (1, side.getWidth() / 6);
                for (int i = 0; i < 3; ++i)
                    destChip[i].setBounds (side.removeFromLeft (dw).reduced (2, 2));
                for (int i = 0; i < kShapeCount; ++i)
                    shapeChip[i].setBounds (side.removeFromLeft (dw).reduced (2, 2));
            }
            else
            {
                const int tagW = 50;
                const int available = juce::jmax (0, chips.getWidth() - tagW);
                int each = juce::jmin (88, juce::jmax (54, (available - 28) / 11));
                int gap = available - each * 11;
                if (gap < 16)
                {
                    gap = 16;
                    each = juce::jmax (1, (available - gap) / 11);
                }
                pageTag = chips.removeFromLeft (tagW);
                placeChips (chips.removeFromLeft (each * 5), sectionChip, 5);
                chips.removeFromLeft (juce::jmin (gap, chips.getWidth()));
                placeChips (chips, waveChip, kWaveCount);
            }
            if (follow)
            {
                pageTag = chips.removeFromLeft (50);
                const int sectionW = juce::jmax (1, chips.getWidth() / 5);
                for (int i = 0; i < 5; ++i)
                    sectionChip[i].setBounds (chips.removeFromLeft (sectionW).reduced (2, 2));
            }
            area.removeFromBottom (kNavGap);
        }
        else if (follow)
        {
            placeDials (area.removeFromBottom (kFollowDialH), followKnobs);
            auto chips = area.removeFromBottom (kFollowChipH);
            followTag = chips.removeFromLeft (58);
            const int laneW[2] = { 86, 68 };
            for (int i = 0; i < 2; ++i)
                laneChip[i].setBounds (chips.removeFromLeft (laneW[i]).reduced (2, 2));
            const int dw = juce::jmax (1, chips.getWidth() / 6);
            for (int i = 0; i < 3; ++i)
                destChip[i].setBounds (chips.removeFromLeft (dw).reduced (2, 2));
            for (int i = 0; i < kShapeCount; ++i)
                shapeChip[i].setBounds (chips.removeFromLeft (dw).reduced (2, 2));
        }
        for (int i = 0; i < 5; ++i)
            sectionChip[i].setVisible (! compactMode);
        if (compactMode && ! follow)
            for (int i = 0; i < kWaveCount; ++i)
                waveChip[i].setVisible (false);

        auto row = area.removeFromBottom (kNavH);
        navRowBounds = row;
        const int storeW = 76;
        storeB.setBounds (row.removeFromRight (storeW).reduced (2, 2));
        storeA.setBounds (row.removeFromRight (storeW).reduced (2, 2));
        auto storeGap = row.removeFromRight (14);
        navSplitStore = storeGap.getCentreX();
        layersTag = row.removeFromLeft (56);
        const int viewLabelW = 44;
        const int midGapW = 16;
        const int buttonSpace = juce::jmax (0, row.getWidth() - viewLabelW - midGapW);
        const int layerBlock = buttonSpace * 4 / 9;
        placeChips (row.removeFromLeft (layerBlock), layerChip, 4);
        auto midGap = row.removeFromLeft (midGapW);
        navSplitLayer = midGap.getCentreX();
        viewTag = row.removeFromLeft (viewLabelW);
        placeChips (row, viewChip, kViewCount);

        if (follow && ! compactMode)
            placeDials (area.removeFromBottom (kFollowDialH), followKnobs);
        area.removeFromBottom (kAfterScreen);
        screen.setBounds (area);
    }

    void KickEditor::paint (juce::Graphics& g)
    {
        juce::ColourGradient wash (steelHi, 0.0f, 0.0f, steelLo, 0.0f, (float) getHeight(), false);
        wash.addColour (0.45, juce::Colour (0xff241c22));
        g.setGradientFill (wash);
        g.fillAll();
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRect (0, 0, getWidth(), 1);

        auto title = juce::Rectangle<float> (20.0f, 8.0f, 280.0f, 40.0f);
        g.setColour (juce::Colours::white);
        auto brandFace = brand != nullptr ? juce::Font (juce::FontOptions (brand)).withHeight (15.0f)
                                          : juce::Font (juce::FontOptions (15.0f)).boldened();
        g.setFont (brandFace);
        g.drawText ("Spher8", title.removeFromTop (16.0f), juce::Justification::bottomLeft);
        g.setColour (ink);
        auto face = wordmark != nullptr ? juce::Font (juce::FontOptions (wordmark)).withHeight (20.0f)
                                        : juce::Font (juce::FontOptions (20.0f));
        g.setFont (face);
        g.drawText ("KICK", title, juce::Justification::bottomLeft);

        const auto targets = shownTargets();
        const float fund = live.fundamental > 1.0f ? live.fundamental : fundamentalHz (targets);
        const float start = live.startHz > 1.0f ? live.startHz : pitchHz (targets, 0.0f, fund);
        char note[16] = {};
        formatNote (midiFromHz (fund, targets.a4), note, (int) sizeof note);
        const float fundLeft = 172.0f;
        const float fundRight = (float) prevChip.getX() - 16.0f;
        auto readout = juce::Rectangle<float> (fundLeft, (float) prevChip.getY() - 1.0f,
                                               juce::jmax (0.0f, juce::jmin (230.0f, fundRight - fundLeft)),
                                               (float) prevChip.getHeight() + 2.0f);
        if (readout.getWidth() >= 108.0f && readout.getRight() <= (float) prevChip.getX() - 8.0f)
        {
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.12f));
            g.drawText ("FUNDAMENTAL", readout.removeFromTop (12.0f), juce::Justification::centredLeft);
            g.setColour (amber);
            g.setFont (juce::Font (juce::FontOptions (15.0f)));
            g.drawText (juce::String (fund, 2) + " Hz   " + juce::String (note), readout, juce::Justification::centredLeft);
        }

        if (powerChip.getY() > 12)
        {
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (8.0f)).withExtraKerningFactor (0.14f));
            auto midi = powerChip.getBounds().toFloat().withHeight (11.0f).withY ((float) powerChip.getY() - 12.0f);
            g.drawText ("MIDI", midi, juce::Justification::centred);
        }

        char heard[16] = {};
        if (live.note >= 0)
            formatNote (live.note, heard, (int) sizeof heard);
        else
            heard[0] = '-';
        g.setColour (mute);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        const float peakDb = live.peak > 1.0e-6f ? 20.0f * std::log10 (live.peak) : -120.0f;
        g.drawFittedText ("NOTE " + juce::String (heard)
                              + "    VEL " + juce::String (live.velocity)
                              + "    START " + juce::String (start, 1) + " Hz"
                              + "    PEAK " + juce::String (peakDb, 1) + " dB",
                          statusArea, juce::Justification::centredLeft, 1);
        if (knobRuleY >= 0)
        {
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.drawHorizontalLine (knobRuleY, (float) kPadX, (float) getWidth() - (float) kPadX);
        }
        auto drawSplit = [&] (int x)
        {
            if (x < 0 || navRowBounds.isEmpty())
                return;
            g.setColour (mute.withAlpha (0.35f));
            g.drawVerticalLine (x, (float) navRowBounds.getY() + 6.0f, (float) navRowBounds.getBottom() - 6.0f);
        };
        drawSplit (navSplitLayer);
        drawSplit (navSplitStore);
        auto drawTag = [&] (juce::Rectangle<int> tag, const char* text)
        {
            if (tag.isEmpty())
                return;
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (9.0f)).withExtraKerningFactor (0.12f));
            g.drawFittedText (text, tag.reduced (0, 3), juce::Justification::centredRight, 1);
        };
        drawTag (layersTag, "LAYERS");
        drawTag (viewTag, "VIEW");
        drawTag (pageTag, "PAGE");
        drawTag (followTag, "FOLLOW");
        juce::ignoreUnused (gem);
    }

    void KickEditor::Screen::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff0c0f13).withAlpha (0.72f));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (gem.withAlpha (0.35f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
        paintMeter (g);
        auto plot = plotArea();
        if (plot.isEmpty())
            return;

        const bool ampSection = editor.section == 1 && editor.view == 0;
        const bool pitchView = editor.view == 1;
        const auto targets = editor.shownTargets();
        const float fund = fundamentalHz (targets);

        if (editor.view == 2)
        {
            const int n = juce::jmax (8, editor.live.points);
            const float span = juce::jmax (0.05f, editor.live.spanSec);
            const float rate = (float) n / span;
            juce::Path bars;
            const int bins = 40;
            for (int b = 0; b < bins; ++b)
            {
                const float hz = 30.0f * std::pow (8000.0f / 30.0f, (float) b / (float) (bins - 1));
                const float w = 6.2831853f * hz / rate;
                float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
                const float coeff = 2.0f * std::cos (w);
                for (int i = 0; i < n && i < Engine::kViewPoints; ++i)
                {
                    s0 = editor.live.wave[i] + coeff * s1 - s2;
                    s2 = s1;
                    s1 = s0;
                }
                const float power = std::sqrt (std::max (0.0f, s1 * s1 + s2 * s2 - coeff * s1 * s2));
                const float mag = juce::jlimit (0.0f, 1.0f, power * 0.15f);
                const float x = plot.getX() + plot.getWidth() * (float) b / (float) bins;
                const float h = mag * plot.getHeight();
                g.setColour (gem.withAlpha (0.85f));
                g.fillRect (x, plot.getBottom() - h, plot.getWidth() / (float) bins - 2.0f, h);
            }
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.14f));
            g.drawText ("SPECTRUM", plot.removeFromTop (14.0f), juce::Justification::topLeft);
            return;
        }

        if (editor.view == 3)
        {
            const juce::Colour colours[] = { gem, amber, juce::Colour (0xffd0d4d8), juce::Colour (0xff7d8c99) };
            const float* layers[] = { editor.live.body, editor.live.sub, editor.live.click, editor.live.layer };
            const char* names[] = { "BODY", "SUB", "CLICK", "LAYER" };
            for (int layer = 0; layer < 4; ++layer)
            {
                auto row = plot.removeFromTop (plot.getHeight() / (float) (4 - layer));
                juce::Path path;
                const int n = juce::jmax (2, editor.live.points);
                for (int i = 0; i < n && i < Engine::kViewPoints; ++i)
                {
                    const float x = row.getX() + row.getWidth() * (float) i / (float) (n - 1);
                    const float y = row.getCentreY() - layers[layer][i] * row.getHeight() * 0.45f;
                    if (i == 0) path.startNewSubPath (x, y);
                    else path.lineTo (x, y);
                }
                g.setColour (colours[layer]);
                g.strokePath (path, juce::PathStrokeType (1.4f));
                g.setFont (juce::Font (juce::FontOptions (10.0f)));
                g.drawText (names[layer], row.removeFromLeft (54.0f), juce::Justification::centredLeft);
            }
            return;
        }

        if (ampSection)
        {
            juce::Path path;
            const float span = juce::jmax (0.05f, (targets.attack + targets.decay + targets.release) * 0.001f + 0.05f);
            for (int i = 0; i < 160; ++i)
            {
                const float t = span * (float) i / 159.0f;
                const float env = ampEnvelope (targets, t);
                const float x = plot.getX() + plot.getWidth() * (float) i / 159.0f;
                const float y = plot.getBottom() - env * plot.getHeight();
                if (i == 0) path.startNewSubPath (x, y);
                else path.lineTo (x, y);
            }
            g.setColour (amber);
            g.strokePath (path, juce::PathStrokeType (2.0f));
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.12f));
            g.drawText ("AMPLITUDE", plot.removeFromTop (14.0f), juce::Justification::topLeft);
            return;
        }

        if (pitchView)
        {
            const float span = juce::jmax (0.04f, (targets.impactMs + targets.bodyMs + 40.0f) * 0.001f);
            juce::Path designed;
            float minHz = fund, maxHz = fund;
            for (int i = 0; i < 160; ++i)
            {
                const float hz = pitchHz (targets, span * (float) i / 159.0f, fund);
                minHz = std::min (minHz, hz);
                maxHz = std::max (maxHz, hz);
            }
            maxHz = std::max (maxHz, minHz * 1.01f);
            auto yFor = [&] (float hz)
            {
                const float u = (std::log (std::max (hz, 1.0f)) - std::log (minHz)) / (std::log (maxHz) - std::log (minHz));
                return plot.getBottom() - u * plot.getHeight();
            };
            for (int i = 0; i < 160; ++i)
            {
                const float x = plot.getX() + plot.getWidth() * (float) i / 159.0f;
                const float y = yFor (pitchHz (targets, span * (float) i / 159.0f, fund));
                if (i == 0) designed.startNewSubPath (x, y);
                else designed.lineTo (x, y);
            }
            g.setColour (gem);
            g.strokePath (designed, juce::PathStrokeType (2.0f));
            if (editor.live.pitch[10] > 1.0f)
            {
                juce::Path actual;
                for (int i = 0; i < 128; ++i)
                {
                    if (editor.live.pitch[i] <= 1.0f)
                        continue;
                    const float x = plot.getX() + plot.getWidth() * (float) i / 127.0f;
                    const float y = yFor (editor.live.pitch[i]);
                    if (actual.isEmpty()) actual.startNewSubPath (x, y);
                    else actual.lineTo (x, y);
                }
                g.setColour (amber.withAlpha (0.85f));
                g.strokePath (actual, juce::PathStrokeType (1.2f));
            }
            const float totalMs = std::max (8.0f, targets.impactMs + targets.bodyMs);
            auto handle = [&] (float ms, float semis)
            {
                const float x = plot.getX() + plot.getWidth() * (ms / totalMs);
                const float hz = fund * std::exp2 (semis / 12.0f);
                g.setColour (ink);
                g.fillEllipse (x - 4.0f, yFor (hz) - 4.0f, 8.0f, 8.0f);
            };
            handle (0.0f, targets.impactSt + targets.bodySt);
            handle (targets.impactMs, targets.bodySt);
            handle (totalMs, 0.0f);
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.12f));
            g.drawText ("PITCH  " + juce::String (pitchHz (targets, 0.0f, fund), 1) + " Hz  >  "
                            + juce::String (fund, 2) + " Hz",
                        plot.removeFromTop (14.0f), juce::Justification::topLeft);
            return;
        }

        if (editor.view == kFollowView)
        {
            const auto lanes = followLanes();
            const int shape = (int) std::lround (targets.followShape);
            const float spanMs = std::max (15.0f, targets.followMs);
            const float spanSec = spanMs * 0.001f;
            const bool ampLive = targets.followAmpUse >= 0.5f;
            const bool ampActive = editor.followLane == 0;
            const float subFund = subFundamentalHz (targets, fund);
            float pitchA = 0.0f, pitchB = 0.0f, ampA = 0.0f, ampB = 0.0f;
            followAnchors (targets, pitchA, pitchB);
            if (ampLive)
                followAmpAnchors (targets, ampA, ampB);
            else
            {
                ampA = pitchA;
                ampB = pitchB;
            }
            auto sampleAmp = [&] (float u)
            {
                const float semis = followSemis (targets, u * spanSec, shape);
                return ampLive ? followAmp (targets, u * spanSec, shape) : clampf (semis / 12.0f, 0.0f, 1.0f);
            };
            auto drawCurve = [&] (juce::Rectangle<float> box, bool pitchLane, bool active)
            {
                if (box.isEmpty())
                    return;
                juce::Path path;
                for (int i = 0; i < 160; ++i)
                {
                    const float u = (float) i / 159.0f;
                    const float norm = pitchLane
                        ? (clampf (followSemis (targets, u * spanSec, shape), -24.0f, 24.0f) + 24.0f) / 48.0f
                        : sampleAmp (u);
                    const float x = box.getX() + box.getWidth() * u;
                    const float y = box.getBottom() - clampf (norm, 0.0f, 1.0f) * box.getHeight();
                    if (i == 0) path.startNewSubPath (x, y);
                    else path.lineTo (x, y);
                }
                const auto colour = pitchLane ? amber : gem;
                g.setColour (colour.withAlpha (active ? 1.0f : 0.38f));
                g.strokePath (path, juce::PathStrokeType (active ? 2.1f : 1.2f));
            };
            g.setColour (ampActive ? gem.withAlpha (0.07f) : amber.withAlpha (0.06f));
            g.fillRect (ampActive ? lanes.amp : lanes.pitch);
            g.setColour (juce::Colour (0xff2c343c));
            for (int i = 0; i <= 4; ++i)
            {
                const float x = lanes.pitch.getX() + lanes.pitch.getWidth() * (float) i / 4.0f;
                g.drawVerticalLine ((int) std::lround (x), lanes.pitch.getY(), lanes.amp.getBottom());
            }
            const float zeroY = lanes.pitch.getCentreY();
            g.setColour (amber.withAlpha (0.28f));
            g.drawHorizontalLine ((int) std::lround (zeroY), lanes.pitch.getX(), lanes.pitch.getRight());
            g.setColour (juce::Colour (0xff243038));
            g.drawHorizontalLine ((int) std::lround (lanes.amp.getY()), lanes.amp.getX(), lanes.amp.getRight());
            if (ampActive)
            {
                drawCurve (lanes.pitch, true, false);
                drawCurve (lanes.amp, false, true);
            }
            else
            {
                drawCurve (lanes.amp, false, false);
                drawCurve (lanes.pitch, true, true);
            }

            const auto box = ampActive ? lanes.amp : lanes.pitch;
            const float at[4] = { 0.0f, ampActive ? ampA : pitchA, ampActive ? ampB : pitchB, 1.0f };
            const float pitchSt[4] = { targets.followSt0, targets.followSt1, targets.followSt2, targets.followSt3 };
            const float ampPt[4] = {
                ampLive ? clampf (targets.followAmp0, 0.0f, 1.0f) : clampf (pitchSt[0] / 12.0f, 0.0f, 1.0f),
                ampLive ? clampf (targets.followAmp1, 0.0f, 1.0f) : clampf (pitchSt[1] / 12.0f, 0.0f, 1.0f),
                ampLive ? clampf (targets.followAmp2, 0.0f, 1.0f) : clampf (pitchSt[2] / 12.0f, 0.0f, 1.0f),
                ampLive ? clampf (targets.followAmp3, 0.0f, 1.0f) : clampf (pitchSt[3] / 12.0f, 0.0f, 1.0f)
            };
            g.setColour (ampActive ? gem : amber);
            for (int i = 0; i < 4; ++i)
            {
                const float norm = ampActive ? ampPt[i] : (clampf (pitchSt[i], -24.0f, 24.0f) + 24.0f) / 48.0f;
                const float x = box.getX() + box.getWidth() * at[i];
                const float y = box.getBottom() - clampf (norm, 0.0f, 1.0f) * box.getHeight();
                g.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
                if (dragHandle == i)
                {
                    const float pointMs = at[i] * spanMs;
                    juce::String tip = juce::String (juce::roundToInt (pointMs)) + " ms";
                    if (ampActive)
                        tip << "   " << juce::String (ampPt[i], 2);
                    else
                    {
                        const float hz = subFund * std::exp2 (pitchSt[i] / 12.0f);
                        tip << "   " << juce::String (pitchSt[i], 1) << " st   " << formatHz (hz) << " Hz";
                    }
                    auto bubble = juce::Rectangle<float> (x - 70.0f, y - 18.0f, 140.0f, 14.0f);
                    if (bubble.getY() < box.getY())
                        bubble = bubble.withY (y + 8.0f);
                    g.setColour (ink);
                    g.setFont (juce::Font (juce::FontOptions (10.0f)));
                    g.drawText (tip, bubble, juce::Justification::centred);
                    g.setColour (ampActive ? gem : amber);
                }
            }

            juce::String head = "0 " + juce::String::charToString (juce::juce_wchar (0x2013)) + " "
                                + juce::String (juce::roundToInt (spanMs)) + " ms";
            const float depth = ampActive ? targets.followLevel : targets.followSub;
            if (depth < 0.03f)
                head << "    DEPTH 0";
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.08f));
            g.drawText (head, lanes.title, juce::Justification::centredLeft);
            auto pitchName = lanes.pitch;
            auto ampName = lanes.amp;
            g.setColour (ampActive ? amber.withAlpha (0.55f) : amber);
            g.drawText ("PITCH", pitchName.removeFromTop (12.0f).removeFromLeft (64.0f), juce::Justification::centredLeft);
            g.setColour (ampActive ? gem : gem.withAlpha (0.55f));
            g.drawText ("POST AMP", ampName.removeFromTop (12.0f).removeFromLeft (78.0f), juce::Justification::centredLeft);
            g.setColour (mute);
            g.setFont (juce::Font (juce::FontOptions (9.0f)));
            for (int i = 0; i <= 4; ++i)
            {
                const float u = (float) i / 4.0f;
                const float x = lanes.ruler.getX() + lanes.ruler.getWidth() * u;
                const int ms = juce::roundToInt (spanMs * u);
                g.drawText (juce::String (ms), juce::Rectangle<float> (x - 22.0f, lanes.ruler.getY(), 44.0f, lanes.ruler.getHeight()),
                            juce::Justification::centred);
            }
            return;
        }

        juce::Path wave;
        const int n = juce::jmax (2, editor.live.points);
        for (int i = 0; i < n && i < Engine::kViewPoints; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (n - 1);
            const float y = plot.getCentreY() - editor.live.wave[i] * plot.getHeight() * 0.42f;
            if (i == 0) wave.startNewSubPath (x, y);
            else wave.lineTo (x, y);
        }
        g.setColour (gem);
        g.strokePath (wave, juce::PathStrokeType (1.6f));
        g.setColour (mute);
        g.setFont (juce::Font (juce::FontOptions (10.0f)).withExtraKerningFactor (0.12f));
        g.drawText ("WAVE", plot.removeFromTop (14.0f), juce::Justification::topLeft);
    }

    KickEditor::Screen::Lanes KickEditor::Screen::followLanes() const
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        bounds.removeFromRight (46.0f);
        bounds = bounds.reduced (12.0f, 8.0f);
        Lanes lanes;
        lanes.ruler = bounds.removeFromBottom (16.0f);
        auto upper = bounds.removeFromTop (bounds.getHeight() * 0.52f);
        lanes.title = upper.removeFromTop (16.0f);
        lanes.pitch = upper.reduced (0.0f, 2.0f);
        lanes.amp = bounds.reduced (0.0f, 3.0f);
        return lanes;
    }

    juce::Rectangle<float> KickEditor::Screen::meterArea() const
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        const float yPad = juce::jlimit (8.0f, 12.0f, bounds.getHeight() * 0.05f);
        return bounds.removeFromRight (46.0f).reduced (6.0f, yPad);
    }

    juce::Rectangle<float> KickEditor::Screen::plotArea() const
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        bounds.removeFromRight (46.0f);
        const float yPad = juce::jlimit (8.0f, 12.0f, bounds.getHeight() * 0.05f);
        return bounds.reduced (12.0f, yPad);
    }

    void KickEditor::Screen::paintMeter (juce::Graphics& g)
    {
        auto column = meterArea();
        if (column.getHeight() < 24.0f)
            return;
        auto readout = column.removeFromBottom (16.0f);
        auto title = column.removeFromTop (12.0f);
        auto track = column.reduced (column.getWidth() * 0.28f, 2.0f);
        g.setColour (mute);
        g.setFont (juce::Font (juce::FontOptions (8.0f)).withExtraKerningFactor (0.12f));
        g.drawText ("LEVEL", title, juce::Justification::centred);
        g.setColour (juce::Colour (0xff14181d));
        g.fillRoundedRectangle (track, 2.0f);
        g.setColour (juce::Colour (0xff2c343c));
        g.drawRoundedRectangle (track, 2.0f, 1.0f);

        const float floorDb = -60.0f;
        auto norm = [floorDb] (float db)
        {
            return juce::jlimit (0.0f, 1.0f, (db - floorDb) / 60.0f);
        };
        const float shown = norm (editor.meterShown);
        if (shown > 0.001f)
        {
            auto fill = track;
            const float h = track.getHeight() * shown;
            fill = fill.removeFromBottom (h);
            g.setColour (editor.meterClipped ? juce::Colour (0xffff6a3c) : gem);
            g.fillRoundedRectangle (fill.reduced (1.0f, 0.0f), 1.5f);
        }
        const float top = track.getY();
        g.setColour (editor.meterClipped ? juce::Colour (0xffff6a3c) : mute.withAlpha (0.7f));
        g.fillRect (track.getX() - 1.0f, top, track.getWidth() + 2.0f, editor.meterClipped ? 2.0f : 1.0f);
        const float holdY = track.getBottom() - norm (editor.meterHold) * track.getHeight();
        const bool holdClip = editor.meterClipped && editor.meterHold >= -0.2f;
        g.setColour (holdClip ? juce::Colour (0xffff6a3c) : amber);
        g.fillRect (track.getX() - 3.0f, holdY - 1.0f, track.getWidth() + 6.0f, 2.0f);

        juce::String text = "-inf";
        if (editor.meterShown > floorDb + 0.4f)
            text = juce::String (editor.meterShown, 1);
        g.setColour (editor.meterClipped ? juce::Colour (0xffff6a3c) : ink);
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawText (text, readout, juce::Justification::centred);
    }

    void KickEditor::Screen::mouseDown (const juce::MouseEvent& event)
    {
        if (meterArea().expanded (2.0f, 4.0f).contains (event.position))
        {
            editor.resetMeter();
            dragHandle = -1;
            return;
        }
        editor.kick.audition (editor.triggerNote());
        dragHandle = -1;
        const auto plot = plotArea();
        if (editor.view == kFollowView)
        {
            const auto lanes = followLanes();
            const float mid = (lanes.pitch.getBottom() + lanes.amp.getY()) * 0.5f;
            editor.followLane = event.position.y >= mid ? 0 : 1;
            editor.laneChip[0].setToggleState (editor.followLane == 0, juce::dontSendNotification);
            editor.laneChip[1].setToggleState (editor.followLane == 1, juce::dontSendNotification);
            const auto targets = editor.shownTargets();
            const auto box = editor.followLane == 0 ? lanes.amp : lanes.pitch;
            float anchorA = 0.0f, anchorB = 0.0f;
            if (editor.followLane == 0 && targets.followAmpUse >= 0.5f)
                followAmpAnchors (targets, anchorA, anchorB);
            else
                followAnchors (targets, anchorA, anchorB);
            const float xs[4] = { 0.0f, anchorA, anchorB, 1.0f };
            float best = 22.0f;
            for (int i = 0; i < 4; ++i)
            {
                const float x = box.getX() + box.getWidth() * xs[i];
                const float distance = std::abs (event.position.x - x);
                if (distance < best)
                {
                    best = distance;
                    dragHandle = i;
                }
            }
            return;
        }
        if (editor.view != 1)
            return;
        const auto targets = editor.shownTargets();
        const float total = std::max (8.0f, targets.impactMs + targets.bodyMs);
        const float xs[] = { 0.0f, targets.impactMs / total, 1.0f };
        float best = 24.0f;
        for (int i = 0; i < 3; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * xs[i];
            const float distance = std::abs (event.position.x - x);
            if (distance < best)
            {
                best = distance;
                dragHandle = i;
            }
        }
    }

    void KickEditor::Screen::mouseDrag (const juce::MouseEvent& event)
    {
        if (dragHandle < 0)
            return;
        const auto plot = plotArea();
        const float x = juce::jlimit (0.0f, 1.0f, (event.position.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
        const float y = juce::jlimit (0.0f, 1.0f, 1.0f - (event.position.y - plot.getY()) / juce::jmax (1.0f, plot.getHeight()));
        if (editor.view == kFollowView)
        {
            const auto lanes = followLanes();
            const auto box = editor.followLane == 0 ? lanes.amp : lanes.pitch;
            const float fx = juce::jlimit (0.0f, 1.0f, (event.position.x - box.getX()) / juce::jmax (1.0f, box.getWidth()));
            const float fy = juce::jlimit (0.0f, 1.0f, 1.0f - (event.position.y - box.getY()) / juce::jmax (1.0f, box.getHeight()));
            if (editor.followLane == 0)
            {
                editor.commitAmpFromPitch();
                const Field fields[4] = { followAmp0, followAmp1, followAmp2, followAmp3 };
                editor.setParam (fields[dragHandle], fy);
                if (dragHandle == 1)
                    editor.setParam (followAmpT1, juce::jlimit (0.05f, 0.80f, fx));
                else if (dragHandle == 2)
                    editor.setParam (followAmpT2, juce::jlimit (0.15f, 0.96f, fx));
            }
            else
            {
                const float stored = juce::jlimit (-24.0f, 24.0f, (fy - 0.5f) * 48.0f);
                const Field fields[4] = { followSt0, followSt1, followSt2, followSt3 };
                editor.setParam (fields[dragHandle], stored);
                if (dragHandle == 1)
                    editor.setParam (followT1, juce::jlimit (0.05f, 0.80f, fx));
                else if (dragHandle == 2)
                    editor.setParam (followT2, juce::jlimit (0.15f, 0.96f, fx));
            }
            return;
        }
        if (editor.view != 1)
            return;
        const auto targets = editor.shownTargets();
        const float total = std::max (8.0f, targets.impactMs + targets.bodyMs);
        const float semis = y * 72.0f;
        if (dragHandle == 0)
            editor.setParam (impactSt, juce::jlimit (0.0f, 72.0f, semis - targets.bodySt));
        else if (dragHandle == 1)
        {
            editor.setParam (impactMs, juce::jlimit (0.2f, 80.0f, x * total));
            editor.setParam (bodySt, juce::jlimit (-36.0f, 48.0f, semis));
        }
        else
            editor.setParam (bodyMs, juce::jlimit (5.0f, 1500.0f, x * total - targets.impactMs));
    }
}
