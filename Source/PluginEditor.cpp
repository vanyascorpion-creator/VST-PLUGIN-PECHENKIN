#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

#include <cmath>

MySynthAudioProcessorEditor::FilterResponseDisplay::FilterResponseDisplay(
    MySynthAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Обновляем график при изменениях параметров фильтра.
    startTimerHz(20);
}

void MySynthAudioProcessorEditor::FilterResponseDisplay::paint(
    juce::Graphics& graphics)
{
    const auto area = getLocalBounds().toFloat();
    graphics.setColour(juce::Colour(19, 21, 27));
    graphics.fillRoundedRectangle(area, 6.0f);

    auto graph = area.reduced(38.0f, 14.0f);
    if (graph.getWidth() <= 0.0f || graph.getHeight() <= 0.0f)
        return;

    // Читаем параметры безопасно для GUI-потока.
    const auto readParameter = [this](const juce::String& id, float fallback)
    {
        if (const auto* value = audioProcessor.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);

        return fallback;
    };

    const auto cutoff = readParameter("CUTOFF", 1000.0f);
    const auto resonance = juce::jmax(0.1f,
        readParameter("RESONANCE", 0.707f));
    const auto filterType = static_cast<int>(
        readParameter("FILTER_TYPE", 0.0f));
    const auto filterEnabled = readParameter("FILTER_ENABLED", 1.0f) >= 0.5f;
    const auto sampleRate = audioProcessor.getSampleRate() > 0.0
        ? audioProcessor.getSampleRate()
        : 44100.0;
    const auto safeCutoff = juce::jlimit(
        20.0,
        sampleRate * 0.49,
        static_cast<double>(cutoff));

    // Сетка показывает ориентиры уровня сигнала в децибелах.
    graphics.setFont(juce::FontOptions(11.0f));
    graphics.setColour(juce::Colour(48, 53, 62));
    for (const auto decibels : { 0.0f, -18.0f, -36.0f })
    {
        const auto y = juce::jmap(
            decibels, -36.0f, 12.0f,
            graph.getBottom(), graph.getY());
        graphics.drawHorizontalLine(
            juce::roundToInt(y), graph.getX(), graph.getRight());
        graphics.setColour(juce::Colours::lightgrey);
        graphics.drawText(
            juce::String(static_cast<int>(decibels)),
            2,
            juce::roundToInt(y - 7.0f),
            32,
            14,
            juce::Justification::centredRight);
        graphics.setColour(juce::Colour(48, 53, 62));
    }

    // При включённом фильтре строим частотную характеристику biquad.
    juce::Path response;
    constexpr int curvePoints = 180;

    const auto omega0 = juce::MathConstants<double>::twoPi
        * safeCutoff / sampleRate;
    const auto cosine = std::cos(omega0);
    const auto alpha = std::sin(omega0)
        / (2.0 * static_cast<double>(resonance));

    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    const auto a0 = 1.0 + alpha;
    const auto a1 = -2.0 * cosine;
    const auto a2 = 1.0 - alpha;

    if (filterType == 0)
    {
        b0 = (1.0 - cosine) * 0.5;
        b1 = 1.0 - cosine;
        b2 = b0;
    }
    else if (filterType == 1)
    {
        b0 = (1.0 + cosine) * 0.5;
        b1 = -(1.0 + cosine);
        b2 = b0;
    }
    else
    {
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
    }

    for (int point = 0; point < curvePoints; ++point)
    {
        const auto proportion = static_cast<double>(point)
            / static_cast<double>(curvePoints - 1);
        const auto frequency = 20.0 * std::pow(1000.0, proportion);
        const auto omega = juce::MathConstants<double>::twoPi
            * frequency / sampleRate;

        double magnitude = 1.0;
        if (filterEnabled)
        {
            const auto cos1 = std::cos(omega);
            const auto sin1 = std::sin(omega);
            const auto cos2 = std::cos(2.0 * omega);
            const auto sin2 = std::sin(2.0 * omega);

            const auto numeratorReal = b0 + b1 * cos1 + b2 * cos2;
            const auto numeratorImag = -b1 * sin1 - b2 * sin2;
            const auto denominatorReal = a0 + a1 * cos1 + a2 * cos2;
            const auto denominatorImag = -a1 * sin1 - a2 * sin2;

            const auto numeratorPower = numeratorReal * numeratorReal
                + numeratorImag * numeratorImag;
            const auto denominatorPower = denominatorReal * denominatorReal
                + denominatorImag * denominatorImag;
            magnitude = std::sqrt(numeratorPower
                / juce::jmax(denominatorPower, 1.0e-12));
        }

        const auto decibels = juce::jlimit(
            -36.0,
            12.0,
            20.0 * std::log10(juce::jmax(magnitude, 1.0e-6)));
        const auto x = graph.getX()
            + graph.getWidth() * static_cast<float>(proportion);
        const auto y = juce::jmap(
            static_cast<float>(decibels), -36.0f, 12.0f,
            graph.getBottom(), graph.getY());

        if (point == 0)
            response.startNewSubPath(x, y);
        else
            response.lineTo(x, y);
    }

    // Маркер Cutoff помогает связать регулятор с графиком.
    const auto cutoffX = graph.getX()
        + graph.getWidth() * static_cast<float>(
            std::log(safeCutoff / 20.0) / std::log(1000.0));
    graphics.setColour(juce::Colour(100, 110, 125));
    graphics.drawVerticalLine(
        juce::roundToInt(cutoffX), graph.getY(), graph.getBottom());

    graphics.setColour(juce::Colour(0, 190, 255));
    graphics.strokePath(response, juce::PathStrokeType(2.5f));

    // Подписываем крайние частоты логарифмической оси.
    graphics.setColour(juce::Colours::lightgrey);
    graphics.drawText("20 Hz", 36, getHeight() - 17, 45, 14,
        juce::Justification::left);
    graphics.drawText("20 kHz", getWidth() - 65, getHeight() - 17, 60, 14,
        juce::Justification::right);
}

void MySynthAudioProcessorEditor::FilterResponseDisplay::timerCallback()
{
    repaint();
}

MySynthAudioProcessorEditor::EnvelopeDisplay::EnvelopeDisplay(
    MySynthAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Перерисовываем график, чтобы он следовал за движением ADSR-ручек.
    startTimerHz(20);
}

void MySynthAudioProcessorEditor::EnvelopeDisplay::paint(
    juce::Graphics& graphics)
{
    const auto area = getLocalBounds().toFloat();

    graphics.setColour(juce::Colour(19, 21, 27));
    graphics.fillRoundedRectangle(area, 6.0f);

    auto graph = area.reduced(18.0f, 14.0f);
    if (graph.getWidth() <= 0.0f || graph.getHeight() <= 0.0f)
        return;

    // Считываем параметры атомарно: интерфейс не обращается к аудиопотоку.
    const auto readParameter = [this](const juce::String& id, float fallback)
    {
        if (const auto* value = audioProcessor.parameters.getRawParameterValue(id))
            return value->load(std::memory_order_relaxed);

        return fallback;
    };

    const auto attack = readParameter("ATTACK", 0.01f);
    const auto decay = readParameter("DECAY", 0.15f);
    const auto sustain = readParameter("SUSTAIN", 0.75f);
    const auto release = readParameter("RELEASE", 0.25f);

    // Фоновая сетка помогает видеть уровни и участки огибающей.
    graphics.setColour(juce::Colour(48, 53, 62));
    for (int i = 1; i < 4; ++i)
    {
        const auto y = graph.getY() + graph.getHeight() * i / 4.0f;
        graphics.drawHorizontalLine(
            juce::roundToInt(y), graph.getX(), graph.getRight());
    }

    for (int i = 1; i < 8; ++i)
    {
        const auto x = graph.getX() + graph.getWidth() * i / 8.0f;
        graphics.drawVerticalLine(
            juce::roundToInt(x), graph.getY(), graph.getBottom());
    }

    // Относительные длительности формируют положения точек по оси времени.
    const auto sustainDuration = 0.45f;
    const auto totalDuration = attack + decay + sustainDuration + release;
    const auto attackX = graph.getX()
        + graph.getWidth() * attack / totalDuration;
    const auto decayX = attackX
        + graph.getWidth() * decay / totalDuration;
    const auto releaseStartX = graph.getRight()
        - graph.getWidth() * release / totalDuration;
    const auto peakY = graph.getY();
    const auto sustainY = graph.getBottom()
        - graph.getHeight() * juce::jlimit(0.0f, 1.0f, sustain);

    juce::Path curve;
    curve.startNewSubPath(graph.getX(), graph.getBottom());
    curve.lineTo(attackX, peakY);
    curve.lineTo(decayX, sustainY);
    curve.lineTo(releaseStartX, sustainY);
    curve.lineTo(graph.getRight(), graph.getBottom());

    graphics.setColour(juce::Colour(0, 190, 255));
    graphics.strokePath(curve, juce::PathStrokeType(2.5f));

    // Подписываем характерные точки ADSR.
    graphics.setFont(juce::FontOptions(12.0f));
    graphics.setColour(juce::Colours::lightgrey);
    graphics.drawText("A", juce::roundToInt(attackX - 8.0f),
        juce::roundToInt(graph.getBottom() + 1.0f), 16, 14,
        juce::Justification::centred);
    graphics.drawText("D", juce::roundToInt(decayX - 8.0f),
        juce::roundToInt(graph.getBottom() + 1.0f), 16, 14,
        juce::Justification::centred);
    graphics.drawText("S", juce::roundToInt((decayX + releaseStartX) * 0.5f - 8.0f),
        juce::roundToInt(graph.getBottom() + 1.0f), 16, 14,
        juce::Justification::centred);
    graphics.drawText("R", juce::roundToInt(releaseStartX - 8.0f),
        juce::roundToInt(graph.getBottom() + 1.0f), 16, 14,
        juce::Justification::centred);
}

void MySynthAudioProcessorEditor::EnvelopeDisplay::timerCallback()
{
    repaint();
}

MySynthAudioProcessorEditor::
MySynthAudioProcessorEditor(
    MySynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
    audioProcessor(processor),
    midiKeyboardDisplay(processor),
    envelopeDisplay(processor),
    filterResponseDisplay(processor)
{
    // Увеличиваем окно, чтобы поместились график ADSR и MIDI-клавиатура.
    setSize(1100, 720);

    // Загружаем фон из ресурсов плагина, а не с диска пользователя.
    backgroundImage = juce::ImageCache::getFromMemory(
        BinaryData::MySynthBackground_png,
        BinaryData::MySynthBackground_pngSize);

    addAndMakeVisible(midiKeyboardDisplay);
    addAndMakeVisible(envelopeDisplay);
    addAndMakeVisible(filterResponseDisplay);

    presetBox.addItem("Init", 1);
    presetBox.addItem("Soft Pad", 2);
    presetBox.addItem("Lead", 3);
    presetBox.addItem("Pluck", 4);
    presetBox.setColour(
        juce::ComboBox::backgroundColourId,
        juce::Colour(45, 45, 55));
    presetBox.setColour(
        juce::ComboBox::textColourId,
        juce::Colours::white);
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(presetLabel);

    presetBox.onChange = [this]
    {
        struct Preset
        {
            float oscillator1Level;
            float oscillator2Level;
            float detune;
            int waveform1;
            int waveform2;
            float attack;
            float decay;
            float sustain;
            float release;
            float cutoff;
            float resonance;
            int filterType;
            bool filterEnabled;
        };

        Preset preset{};
        switch (presetBox.getSelectedId())
        {
        case 2:
            preset = { 0.48f, 0.32f, 0.08f, 0, 3,
                0.8f, 1.8f, 0.75f, 2.5f, 4200.0f, 0.8f, 0, true };
            break;
        case 3:
            preset = { 0.72f, 0.38f, 0.12f, 1, 0,
                0.005f, 0.18f, 0.65f, 0.22f, 7500.0f, 1.2f, 0, true };
            break;
        case 4:
            preset = { 0.8f, 0.2f, 0.0f, 3, 1,
                0.001f, 0.28f, 0.0f, 0.12f, 6200.0f, 0.9f, 0, true };
            break;
        case 1:
        default:
            preset = { 0.55f, 0.35f, 0.05f, 0, 1,
                0.01f, 0.15f, 0.75f, 0.25f, 1000.0f, 1.0f, 0, true };
            break;
        }

        const auto setParameter = [this](const juce::String& id, float value)
        {
            if (auto* parameter = audioProcessor.parameters.getParameter(id))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost(
                    parameter->convertTo0to1(value));
                parameter->endChangeGesture();
            }
        };

        setParameter("OSC1_LEVEL", preset.oscillator1Level);
        setParameter("OSC2_LEVEL", preset.oscillator2Level);
        setParameter("DETUNE", preset.detune);
        setParameter("WAVEFORM1", static_cast<float>(preset.waveform1));
        setParameter("WAVEFORM2", static_cast<float>(preset.waveform2));
        setParameter("ATTACK", preset.attack);
        setParameter("DECAY", preset.decay);
        setParameter("SUSTAIN", preset.sustain);
        setParameter("RELEASE", preset.release);
        setParameter("CUTOFF", preset.cutoff);
        setParameter("RESONANCE", preset.resonance);
        setParameter("FILTER_TYPE", static_cast<float>(preset.filterType));
        setParameter("FILTER_ENABLED", preset.filterEnabled ? 1.0f : 0.0f);
    };

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
    
    // Добавляем варианты типа фильтра.
    filterTypeBox.addItem("Low-pass", 1);
    filterTypeBox.addItem("High-pass", 2);
    filterTypeBox.addItem("Band-pass", 3);

    filterTypeBox.setColour(
        juce::ComboBox::backgroundColourId,
        juce::Colour(45, 45, 55));

    filterTypeBox.setColour(
        juce::ComboBox::textColourId,
        juce::Colours::white);

    filterTypeLabel.setText(
        "FILTER TYPE",
        juce::dontSendNotification);

    filterTypeLabel.setJustificationType(
        juce::Justification::centred);

    filterTypeLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white);

    addAndMakeVisible(filterTypeBox);
    addAndMakeVisible(filterTypeLabel);

    // Переключатель полностью обходит фильтр, не меняя его настройки.
    filterEnabledButton.setButtonText("FILTER ENABLED");
    filterEnabledButton.setColour(
        juce::ToggleButton::textColourId,
        juce::Colours::white);
    filterEnabledButton.setColour(
        juce::ToggleButton::tickColourId,
        juce::Colours::deepskyblue);
    addAndMakeVisible(filterEnabledButton);

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
    // Связываем список с параметром типа фильтра.
    filterTypeAttachment =
        std::make_unique<ComboBoxAttachment>(
            audioProcessor.parameters,
            "FILTER_TYPE",
            filterTypeBox);

    filterEnabledAttachment =
        std::make_unique<ButtonAttachment>(
            audioProcessor.parameters,
            "FILTER_ENABLED",
            filterEnabledButton);
}

MySynthAudioProcessorEditor::
~MySynthAudioProcessorEditor()
{
}

void MySynthAudioProcessorEditor::paint(
    juce::Graphics& graphics)
{
    // Заполняем окно фоном с сохранением пропорций и центральной обрезкой.
    if (backgroundImage.isValid())
    {
        graphics.drawImageWithin(
            backgroundImage,
            0,
            0,
            getWidth(),
            getHeight(),
            juce::RectanglePlacement::centred
                | juce::RectanglePlacement::fillDestination);

        // Слегка затемняем текстуру, чтобы подписи хорошо читались.
        graphics.setColour(juce::Colour(0, 0, 0).withAlpha(0.16f));
        graphics.fillAll();
    }
    else
    {
        graphics.fillAll(juce::Colour(25, 25, 30));
    }

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

    graphics.drawText(
        "MIDI INPUT",
        30,
        615,
        400,
        22,
        juce::Justification::left);
}

void MySynthAudioProcessorEditor::resized()
{
    const int controlWidth = 145;
    const int controlHeight = 145;
    const int spacing = 20;

    const int left = 30;
    const int oscillatorTop = 80;
    const int envelopeGraphTop = 315;
    const int envelopeTop = 430;

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
    envelopeDisplay.setBounds(
        left,
        envelopeGraphTop,
        controlWidth * 4 + spacing * 3,
        100);

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
        125,
        comboWidth,
        24);

    waveform1Box.setBounds(
        comboLeft,
        150,
        comboWidth,
        comboHeight);

    waveform2Label.setBounds(
        comboLeft,
        195,
        comboWidth,
        24);

    waveform2Box.setBounds(
        comboLeft,
        220,
        comboWidth,
        comboHeight);
    // Размещаем тип фильтра справа от списков осцилляторов.
    filterTypeLabel.setBounds(
        890,
        125,
        180,
        24);

    filterTypeBox.setBounds(
        890,
        150,
        180,
        30);

    filterEnabledButton.setBounds(
        890,
        195,
        180,
        26);

    presetLabel.setBounds(
        comboLeft,
        55,
        comboWidth,
        24);

    presetBox.setBounds(
        comboLeft,
        80,
        comboWidth,
        comboHeight);

    // График отклика расположен над ручками фильтра.
    filterResponseDisplay.setBounds(
        690,
        envelopeGraphTop,
        380,
        100);

    // Клавиатурный индикатор занимает нижнюю полосу редактора.
    midiKeyboardDisplay.setBounds(
        30,
        645,
        1040,
        60);
}