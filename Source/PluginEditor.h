#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MySynthAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit MySynthAudioProcessorEditor(
        MySynthAudioProcessor&);

    ~MySynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    using ComboBoxAttachment =
        juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;

    MySynthAudioProcessor& audioProcessor;
    juce::Image backgroundImage;

    // Рисует MIDI-клавиши и подсвечивает ноты, активные в хосте.
    class MidiKeyboardDisplay : public juce::Component,
                                private juce::Timer
    {
    public:
        explicit MidiKeyboardDisplay(MySynthAudioProcessor& processor)
            : audioProcessor(processor)
        {
            // Частое обновление даёт отзывчивую подсветку, не опрашивая
            // состояние клавиш в аудиопотоке.
            startTimerHz(30);
        }

        void paint(juce::Graphics& graphics) override
        {
            constexpr int firstMidiNote = 36; // C2
            constexpr int numberOfNotes = 48;  // C2..B5
            constexpr int numberOfWhiteKeys = 28;

            const auto bounds = getLocalBounds().toFloat();
            const auto whiteKeyWidth =
                bounds.getWidth() / static_cast<float>(numberOfWhiteKeys);

            // Сначала рисуем белые клавиши, чтобы чёрные легли поверх них.
            int whiteKeyIndex = 0;
            for (int offset = 0; offset < numberOfNotes; ++offset)
            {
                const auto note = firstMidiNote + offset;
                const auto pitchClass = note % 12;

                if (pitchClass == 1 || pitchClass == 3
                    || pitchClass == 6 || pitchClass == 8
                    || pitchClass == 10)
                    continue;

                auto key = juce::Rectangle<float>(
                    whiteKeyIndex * whiteKeyWidth,
                    0.0f,
                    whiteKeyWidth,
                    bounds.getHeight()).reduced(0.5f);

                graphics.setColour(
                    audioProcessor.isMidiNoteActive(note)
                        ? juce::Colour(0, 190, 255)
                        : juce::Colour(235, 237, 240));
                graphics.fillRoundedRectangle(key, 2.0f);

                graphics.setColour(juce::Colour(40, 45, 52));
                graphics.drawRoundedRectangle(key, 2.0f, 1.0f);
                ++whiteKeyIndex;
            }

            // Чёрные клавиши располагаются по центру промежутков белых.
            const auto blackKeyWidth = whiteKeyWidth * 0.58f;
            const auto blackKeyHeight = bounds.getHeight() * 0.62f;
            int whiteKeysBefore = 0;

            for (int offset = 0; offset < numberOfNotes; ++offset)
            {
                const auto note = firstMidiNote + offset;
                const auto pitchClass = note % 12;
                const auto isBlackKey = pitchClass == 1 || pitchClass == 3
                    || pitchClass == 6 || pitchClass == 8
                    || pitchClass == 10;

                if (isBlackKey)
                {
                    auto key = juce::Rectangle<float>(
                        whiteKeysBefore * whiteKeyWidth - blackKeyWidth * 0.5f,
                        0.0f,
                        blackKeyWidth,
                        blackKeyHeight).reduced(0.5f);

                    graphics.setColour(
                        audioProcessor.isMidiNoteActive(note)
                            ? juce::Colour(0, 190, 255)
                            : juce::Colour(35, 39, 46));
                    graphics.fillRoundedRectangle(key, 2.0f);

                    graphics.setColour(juce::Colour(15, 17, 21));
                    graphics.drawRoundedRectangle(key, 2.0f, 1.0f);
                }
                else
                {
                    ++whiteKeysBefore;
                }
            }
        }

    private:
        void timerCallback() override
        {
            repaint();
        }

        MySynthAudioProcessor& audioProcessor;
    };

    MidiKeyboardDisplay midiKeyboardDisplay;

    // Показывает расчётную форму амплитудной ADSR-огибающей.
    class EnvelopeDisplay : public juce::Component,
                            private juce::Timer
    {
    public:
        explicit EnvelopeDisplay(MySynthAudioProcessor& processor);
        void paint(juce::Graphics& graphics) override;

    private:
        void timerCallback() override;

        MySynthAudioProcessor& audioProcessor;
    };

    EnvelopeDisplay envelopeDisplay;

    // Отображает АЧХ выбранного типа фильтра.
    class FilterResponseDisplay : public juce::Component,
                                 private juce::Timer
    {
    public:
        explicit FilterResponseDisplay(MySynthAudioProcessor& processor);
        void paint(juce::Graphics& graphics) override;

    private:
        void timerCallback() override;

        MySynthAudioProcessor& audioProcessor;
    };

    FilterResponseDisplay filterResponseDisplay;

    // Громкость и расстройка осцилляторов.
    juce::Slider oscillator1LevelSlider;
    juce::Slider oscillator2LevelSlider;
    juce::Slider detuneSlider;

    // ADSR-конверт.
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    // Параметры фильтра.
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;

    // Выбор формы волны.
    juce::ComboBox waveform1Box;
    juce::ComboBox waveform2Box;

    juce::ComboBox presetBox;
    juce::Label presetLabel;

    // Выбор типа фильтра.
    juce::ComboBox filterTypeBox;
    juce::ToggleButton filterEnabledButton;

    juce::Label oscillator1Label;
    juce::Label oscillator2Label;
    juce::Label detuneLabel;

    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;

    juce::Label cutoffLabel;
    juce::Label resonanceLabel;

    juce::Label waveform1Label;
    juce::Label waveform2Label;
    juce::Label filterTypeLabel;

    std::unique_ptr<SliderAttachment>
        oscillator1Attachment;

    std::unique_ptr<SliderAttachment>
        oscillator2Attachment;

    std::unique_ptr<SliderAttachment>
        detuneAttachment;

    std::unique_ptr<SliderAttachment>
        attackAttachment;

    std::unique_ptr<SliderAttachment>
        decayAttachment;

    std::unique_ptr<SliderAttachment>
        sustainAttachment;

    std::unique_ptr<SliderAttachment>
        releaseAttachment;

    std::unique_ptr<SliderAttachment>
        cutoffAttachment;

    std::unique_ptr<SliderAttachment>
        resonanceAttachment;

    std::unique_ptr<ComboBoxAttachment>
        waveform1Attachment;

    std::unique_ptr<ComboBoxAttachment>
        waveform2Attachment;
    
    std::unique_ptr<ComboBoxAttachment>
        filterTypeAttachment;

    std::unique_ptr<ButtonAttachment>
        filterEnabledAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MySynthAudioProcessorEditor)
};