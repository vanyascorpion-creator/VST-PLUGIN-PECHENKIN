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

    MySynthAudioProcessor& audioProcessor;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MySynthAudioProcessorEditor)
};