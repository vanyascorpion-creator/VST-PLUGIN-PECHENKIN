#include "PluginProcessor.h"
#include "PluginEditor.h"

MySynthAudioProcessorEditor::
MySynthAudioProcessorEditor(
    MySynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
    audioProcessor(processor)
{
    // Увеличиваем окно, чтобы справа поместились
    // параметры фильтра.
    setSize(1100, 500);

    // Общая настройка поворотной ручки.
    auto configureSlider =
        [this](
            juce::Slider& slider,
            juce::Label& label,
            const juce::String& text)
        {
            slider.setSliderStyle(
                juce::Slider::RotaryHorizontalVerticalDrag);

            slider.setTextBoxStyle(
                juce::Slider::TextBoxBelow,
                false,
                90,
                24);

            slider.setColour(
                juce::Slider::rotarySliderFillColourId,
                juce::Colours::deepskyblue);

            slider.setColour(
                juce::Slider::thumbColourId,
                juce::Colours::white);

            label.setText(
                text,
                juce::dontSendNotification);

            label.setJustificationType(
                juce::Justification::centred);

            label.setColour(
                juce::Label::textColourId,
                juce::Colours::white);

            addAndMakeVisible(slider);
            addAndMakeVisible(label);
        };

    // Настройка выпадающего списка формы волны.
    auto configureComboBox =
        [this](
            juce::ComboBox& comboBox,
            juce::Label& label,
            const juce::String& text)
        {
            comboBox.addItem("Sine", 1);
            comboBox.addItem("Saw", 2);
            comboBox.addItem("Square", 3);
            comboBox.addItem("Triangle", 4);

            comboBox.setColour(
                juce::ComboBox::backgroundColourId,
                juce::Colour(45, 45, 55));

            comboBox.setColour(
                juce::ComboBox::textColourId,
                juce::Colours::white);

            label.setText(
                text,
                juce::dontSendNotification);

            label.setJustificationType(
                juce::Justification::centred);

            label.setColour(
                juce::Label::textColourId,
                juce::Colours::white);

            addAndMakeVisible(comboBox);
            addAndMakeVisible(label);
        };

    configureSlider(
        oscillator1LevelSlider,
        oscillator1Label,
        "OSC 1 LEVEL");

    configureSlider(
        oscillator2LevelSlider,
        oscillator2Label,
        "OSC 2 LEVEL");

    configureSlider(
        detuneSlider,
        detuneLabel,
        "OSC 2 DETUNE");

    configureSlider(
        attackSlider,
        attackLabel,
        "ATTACK");

    configureSlider(
        decaySlider,
        decayLabel,
        "DECAY");

    configureSlider(
        sustainSlider,
        sustainLabel,
        "SUSTAIN");

    configureSlider(
        releaseSlider,
        releaseLabel,
        "RELEASE");

    configureSlider(
        cutoffSlider,
        cutoffLabel,
        "FILTER CUTOFF");

    configureSlider(
        resonanceSlider,
        resonanceLabel,
        "FILTER RESONANCE");

    configureComboBox(
        waveform1Box,
        waveform1Label,
        "OSC 1 WAVEFORM");

    configureComboBox(
        waveform2Box,
        waveform2Label,
        "OSC 2 WAVEFORM");

    // Связываем ручки с параметрами процессора.
    oscillator1Attachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "OSC1_LEVEL",
            oscillator1LevelSlider);

    oscillator2Attachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "OSC2_LEVEL",
            oscillator2LevelSlider);

    detuneAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "DETUNE",
            detuneSlider);

    attackAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "ATTACK",
            attackSlider);

    decayAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "DECAY",
            decaySlider);

    sustainAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "SUSTAIN",
            sustainSlider);

    releaseAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "RELEASE",
            releaseSlider);

    cutoffAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "CUTOFF",
            cutoffSlider);

    resonanceAttachment =
        std::make_unique<SliderAttachment>(
            audioProcessor.parameters,
            "RESONANCE",
            resonanceSlider);

    waveform1Attachment =
        std::make_unique<ComboBoxAttachment>(
            audioProcessor.parameters,
            "WAVEFORM1",
            waveform1Box);

    waveform2Attachment =
        std::make_unique<ComboBoxAttachment>(
            audioProcessor.parameters,
            "WAVEFORM2",
            waveform2Box);
}

MySynthAudioProcessorEditor::
~MySynthAudioProcessorEditor()
{
}

void MySynthAudioProcessorEditor::paint(
    juce::Graphics& graphics)
{
    graphics.fillAll(
        juce::Colour(25, 25, 30));

    graphics.setColour(
        juce::Colours::white);

    graphics.setFont(
        juce::FontOptions(24.0f));

    graphics.drawText(
        "PECHENKIN_SYNTH",
        0,
        15,
        getWidth(),
        35,
        juce::Justification::centred);

    graphics.setFont(
        juce::FontOptions(16.0f));

    graphics.setColour(
        juce::Colours::deepskyblue);

    graphics.drawText(
        "OSCILLATORS",
        30,
        55,
        400,
        25,
        juce::Justification::left);

    graphics.drawText(
        "ENVELOPE",
        30,
        285,
        400,
        25,
        juce::Justification::left);

    graphics.drawText(
        "FILTER",
        690,
        285,
        350,
        25,
        juce::Justification::left);
}

void MySynthAudioProcessorEditor::resized()
{
    const int controlWidth = 145;
    const int controlHeight = 145;
    const int spacing = 20;

    const int left = 30;
    const int oscillatorTop = 80;
    const int envelopeTop = 315;

    auto setSliderControl =
        [](
            juce::Slider& slider,
            juce::Label& label,
            int x,
            int y,
            int width,
            int height)
        {
            slider.setBounds(
                x,
                y,
                width,
                height);

            label.setBounds(
                x,
                y + height - 2,
                width,
                25);
        };

    // Осцилляторы.
    setSliderControl(
        oscillator1LevelSlider,
        oscillator1Label,
        left,
        oscillatorTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        oscillator2LevelSlider,
        oscillator2Label,
        left + controlWidth + spacing,
        oscillatorTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        detuneSlider,
        detuneLabel,
        left + (controlWidth + spacing) * 2,
        oscillatorTop,
        controlWidth,
        controlHeight);

    // ADSR.
    setSliderControl(
        attackSlider,
        attackLabel,
        left,
        envelopeTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        decaySlider,
        decayLabel,
        left + controlWidth + spacing,
        envelopeTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        sustainSlider,
        sustainLabel,
        left + (controlWidth + spacing) * 2,
        envelopeTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        releaseSlider,
        releaseLabel,
        left + (controlWidth + spacing) * 3,
        envelopeTop,
        controlWidth,
        controlHeight);

    // Фильтр.
    setSliderControl(
        cutoffSlider,
        cutoffLabel,
        690,
        envelopeTop,
        controlWidth,
        controlHeight);

    setSliderControl(
        resonanceSlider,
        resonanceLabel,
        855,
        envelopeTop,
        controlWidth,
        controlHeight);

    // Выпадающие списки форм волн.
    const int comboLeft = 690;
    const int comboWidth = 180;
    const int comboHeight = 30;

    waveform1Label.setBounds(
        comboLeft,
        105,
        comboWidth,
        24);

    waveform1Box.setBounds(
        comboLeft,
        130,
        comboWidth,
        comboHeight);

    waveform2Label.setBounds(
        comboLeft,
        190,
        comboWidth,
        24);

    waveform2Box.setBounds(
        comboLeft,
        215,
        comboWidth,
        comboHeight);
}