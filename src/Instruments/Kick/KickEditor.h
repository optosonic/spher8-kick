#pragma once

#include "KickInstrument.h"
#include "Instruments/NoteRangeBar.h"

namespace atrium::kick
{
    class KickEditor : public juce::AudioProcessorEditor,
                       private juce::Timer,
                       public juce::FileDragAndDropTarget
    {
    public:
        explicit KickEditor (KickInstrument&);
        ~KickEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        bool isInterestedInFileDrag (const juce::StringArray&) override;
        void filesDropped (const juce::StringArray&, int, int) override;

    private:
        class LookAndFeel : public juce::LookAndFeel_V4
        {
        public:
            void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                                   float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                   juce::Slider&) override;
        };

        class Dial : public juce::Component
        {
        public:
            Dial (const juce::String& paramId, const juce::String& caption, juce::AudioProcessorValueTreeState&, LookAndFeel&, bool compact = false);
            ~Dial() override;
            void paint (juce::Graphics&) override;
            void resized() override;
            juce::Slider slider;
            juce::String caption;
            juce::String paramId;
            juce::String unit;
            bool compact = false;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        };

        class Chip : public juce::Button
        {
        public:
            Chip();
            void paintButton (juce::Graphics&, bool highlighted, bool down) override;
            int role = 0;
        };

        class Screen : public juce::Component
        {
        public:
            explicit Screen (KickEditor& owner) : editor (owner) {}
            void paint (juce::Graphics&) override;
            void mouseDown (const juce::MouseEvent&) override;
            void mouseDrag (const juce::MouseEvent&) override;

            struct Lanes
            {
                juce::Rectangle<float> title;
                juce::Rectangle<float> pitch;
                juce::Rectangle<float> amp;
                juce::Rectangle<float> ruler;
                juce::Rectangle<float> pitchGutter;
                juce::Rectangle<float> ampGutter;
            };

            KickEditor& editor;
            int dragHandle = -1;

            Lanes followLanes() const;

        private:
            juce::Rectangle<float> plotArea() const;
            juce::Rectangle<float> meterArea() const;
            void paintMeter (juce::Graphics&);
        };

        void timerCallback() override;
        void layoutHeader (juce::Rectangle<int>);
        void applyWindowLimits();
        int preferredHeight() const;
        void showSection (int section);
        void stepPreset (int delta);
        void showPresetMenu();
        void askSavePreset();
        void savePreset (const juce::String&);
        void loadPresetFile (const juce::File&);
        juce::File presetDirectory() const;
        void setChoice (Field field, int index);
        float param (Field field) const;
        void setParam (Field field, float value);
        void commitAmpFromPitch();
        void refreshInputChips();
        void showInputNoteMenu();
        void showChannelMenu();
        void fireTrigger();
        void resetMeter() noexcept;
        int triggerNote() const;
        int triggerChannel() const;
        Targets shownTargets() const;

        KickInstrument& kick;
        LookAndFeel look;
        juce::Typeface::Ptr wordmark;
        juce::Typeface::Ptr brand;
        Screen screen;
        NoteRangeBar rangeBar;
        std::unique_ptr<Dial> keyDial;
        juce::OwnedArray<Dial> primary;
        juce::OwnedArray<Dial> pitchKnobs;
        juce::OwnedArray<Dial> ampKnobs;
        juce::OwnedArray<Dial> harmKnobs;
        juce::OwnedArray<Dial> dynKnobs;
        juce::OwnedArray<Dial> outKnobs;
        juce::OwnedArray<Dial> followKnobs;

        Chip powerChip, compactChip, saveChip, prevChip, nextChip, nameChip, auditionChip;
        Chip noteInChip, channelChip, triggerChip;
        Chip storeA, storeB;
        Chip layerChip[4];
        Chip viewChip[5];
        Chip sectionChip[5];
        Chip waveChip[kWaveCount];
        Chip destChip[3];
        Chip shapeChip[3];
        Chip laneChip[2];
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> layerAttach[4];

        int section = 0;
        int view = 0;
        int followLane = 0;
        bool compactMode = false;
        juce::Rectangle<int> layersTag, viewTag, pageTag, followTag;
        juce::Rectangle<int> statusArea;
        juce::Rectangle<int> navRowBounds;
        int navSplitLayer = -1;
        int navSplitStore = -1;
        int knobRuleY = -1;
        LiveView live;
        float meterShown = -60.0f;
        float meterHold = -60.0f;
        bool meterClipped = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickEditor)
    };
}
