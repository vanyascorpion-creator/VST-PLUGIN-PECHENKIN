#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

MySynthAudioProcessor::MySynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
        .withOutput("Output",
            juce::AudioChannelSet::stereo(),
            true)
#endif
    ),
#endif
    parameters(*this,
        nullptr,
        "PARAMETERS",
        createParameterLayout())
{
    for (int i = 0; i < 8; ++i)
    {
        auto* voice = new SynthVoice();

        voice->setParameterPointers(
            parameters.getRawParameterValue("OSC1_LEVEL"),
            parameters.getRawParameterValue("OSC2_LEVEL"),
            parameters.getRawParameterValue("DETUNE"),
            parameters.getRawParameterValue("WAVEFORM1"),
            parameters.getRawParameterValue("WAVEFORM2"),
            parameters.getRawParameterValue("ATTACK"),
            parameters.getRawParameterValue("DECAY"),
            parameters.getRawParameterValue("SUSTAIN"),
            parameters.getRawParameterValue("RELEASE"),
            parameters.getRawParameterValue("CUTOFF"),
            parameters.getRawParameterValue("RESONANCE"),
            parameters.getRawParameterValue("FILTER_TYPE"),
            parameters.getRawParameterValue("FILTER_ENABLED"));

        synthesiser.addVoice(voice);
    }

    synthesiser.addSound(new SynthSound());
}

MySynthAudioProcessor::~MySynthAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
MySynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC1_LEVEL",
        "Oscillator 1 Level",
        juce::NormalisableRange<float>(
            0.0f, 1.0f, 0.001f),
        0.55f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OSC2_LEVEL",
        "Oscillator 2 Level",
        juce::NormalisableRange<float>(
            0.0f, 1.0f, 0.001f),
        0.35f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "DETUNE",
        "Oscillator 2 Detune",
        juce::NormalisableRange<float>(
            -12.0f, 12.0f, 0.01f),
        0.05f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "WAVEFORM1",
        "Oscillator 1 Waveform",
        juce::StringArray{
            "Sine",
            "Saw",
            "Square",
            "Triangle"
        },
        0));

    // Отдельный флаг сохраняет существующие индексы типов фильтра.
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "FILTER_ENABLED",
        "Filter Enabled",
        true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "WAVEFORM2",
        "Oscillator 2 Waveform",
        juce::StringArray{
            "Sine",
            "Saw",
            "Square",
            "Triangle"
        },
        1));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ATTACK",
        "Attack",
        juce::NormalisableRange<float>(
            0.001f, 5.0f, 0.001f),
        0.01f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "DECAY",
        "Decay",
        juce::NormalisableRange<float>(
            0.001f, 5.0f, 0.001f),
        0.15f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "SUSTAIN",
        "Sustain",
        juce::NormalisableRange<float>(
            0.0f, 1.0f, 0.001f),
        0.75f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RELEASE",
        "Release",
        juce::NormalisableRange<float>(
            0.001f, 5.0f, 0.001f),
        0.25f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "CUTOFF",
        "Cutoff",
        juce::NormalisableRange<float>(
            20.0f, 20000.0f, 1.0f),
        1000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "RESONANCE",
        "Resonance",
        juce::NormalisableRange<float>(
            0.1f, 10.0f, 0.1f),
        1.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "FILTER_TYPE",
        "Filter Type",
        juce::StringArray{
            "Low-pass",
            "High-pass",
            "Band-pass"
        },
        0));

    return layout;
}

const juce::String MySynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MySynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool MySynthAudioProcessor::producesMidi() const
{
    return false;
}

bool MySynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double MySynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MySynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int MySynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MySynthAudioProcessor::setCurrentProgram(int)
{
}

const juce::String MySynthAudioProcessor::getProgramName(int)
{
    return {};
}

void MySynthAudioProcessor::changeProgramName(
    int,
    const juce::String&)
{
}

void MySynthAudioProcessor::prepareToPlay(
    double sampleRate,
    int)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0;
        i < synthesiser.getNumVoices();
        ++i)
    {
        if (auto* voice =
            dynamic_cast<SynthVoice*>(
                synthesiser.getVoice(i)))
        {
            voice->prepare(sampleRate);
        }
    }
}

void MySynthAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MySynthAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output =
        layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}
#endif

void MySynthAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Отслеживаем входящие ноты для визуальной клавиатуры.
    // Атомарные флаги позволяют GUI читать состояние без блокировки аудио.
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            activeMidiNotes[static_cast<size_t>(message.getNoteNumber())]
                .store(true, std::memory_order_relaxed);
        }
        else if (message.isNoteOff())
        {
            activeMidiNotes[static_cast<size_t>(message.getNoteNumber())]
                .store(false, std::memory_order_relaxed);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            for (auto& noteActive : activeMidiNotes)
                noteActive.store(false, std::memory_order_relaxed);
        }
    }

    buffer.clear();

    synthesiser.renderNextBlock(
        buffer,
        midiMessages,
        0,
        buffer.getNumSamples());
}

bool MySynthAudioProcessor::isMidiNoteActive(
    int midiNoteNumber) const noexcept
{
    if (midiNoteNumber < 0 || midiNoteNumber >= 128)
        return false;

    return activeMidiNotes[static_cast<size_t>(midiNoteNumber)]
        .load(std::memory_order_relaxed);
}

bool MySynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
MySynthAudioProcessor::createEditor()
{
    return new MySynthAudioProcessorEditor(*this);
}

void MySynthAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    const auto state = parameters.copyState();
    const auto xml = state.createXml();

    copyXmlToBinary(*xml, destData);
}

void MySynthAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    const auto xmlState =
        getXmlFromBinary(data, sizeInBytes);

    if (xmlState != nullptr
        && xmlState->hasTagName(
            parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// SynthVoice

void MySynthAudioProcessor::SynthVoice::setParameterPointers(
    std::atomic<float>* newOscillator1Level,
    std::atomic<float>* newOscillator2Level,
    std::atomic<float>* newDetune,
    std::atomic<float>* newWaveform1,
    std::atomic<float>* newWaveform2,
    std::atomic<float>* newAttack,
    std::atomic<float>* newDecay,
    std::atomic<float>* newSustain,
    std::atomic<float>* newRelease,
    std::atomic<float>* newCutoff,
    std::atomic<float>* newResonance,
    std::atomic<float>* newFilterType,
    std::atomic<float>* newFilterEnabled)

{
    oscillator1Level = newOscillator1Level;
    oscillator2Level = newOscillator2Level;
    detune = newDetune;
    waveform1 = newWaveform1;
    waveform2 = newWaveform2;

    attack = newAttack;
    decay = newDecay;
    sustain = newSustain;
    release = newRelease;

    cutoff = newCutoff;
    resonance = newResonance;

    // Сохраняем указатель на параметр типа фильтра.
    filterType = newFilterType;
    filterEnabled = newFilterEnabled;
}

void MySynthAudioProcessor::SynthVoice::prepare(
    double newSampleRate)
{
    sampleRate = newSampleRate;

    adsrParameters.attack = 0.01f;
    adsrParameters.decay = 0.15f;
    adsrParameters.sustain = 0.75f;
    adsrParameters.release = 0.25f;

    adsr.setSampleRate(sampleRate);
    adsr.setParameters(adsrParameters);
    // Сбрасываем внутреннее состояние фильтра
// перед началом воспроизведения.
    filter.reset();

    // Начальная настройка фильтра:
    // полностью открытый low-pass без заметного окрашивания.
    filter.setCoefficients(
        juce::IIRCoefficients::makeLowPass(
            sampleRate,
            20000.0,
            0.707));

}

void MySynthAudioProcessor::SynthVoice::startNote(
    int midiNoteNumber,
    float velocity,
    juce::SynthesiserSound*,
    int)
{
    const auto frequency1 =
        juce::MidiMessage::getMidiNoteInHertz(
            midiNoteNumber);

    const auto detuneSemitones =
        detune != nullptr
        ? detune->load()
        : 0.0f;

    const auto frequency2 =
        frequency1 * std::pow(
            2.0,
            static_cast<double>(
                detuneSemitones) / 12.0);

    phase1 = 0.0;
    phase2 = 0.0;

    phaseIncrement1 = frequency1 / sampleRate;
    phaseIncrement2 = frequency2 / sampleRate;

    level = velocity;
    isPlaying = true;

    adsrParameters.attack =
        attack != nullptr
        ? attack->load()
        : 0.01f;

    adsrParameters.decay =
        decay != nullptr
        ? decay->load()
        : 0.15f;

    adsrParameters.sustain =
        sustain != nullptr
        ? sustain->load()
        : 0.75f;

    adsrParameters.release =
        release != nullptr
        ? release->load()
        : 0.25f;

    adsr.reset();
    adsr.setParameters(adsrParameters);
    adsr.noteOn();
}

void MySynthAudioProcessor::SynthVoice::stopNote(
    float,
    bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
        isPlaying = false;
    }
}

void MySynthAudioProcessor::SynthVoice::renderNextBlock(
    juce::AudioBuffer<float>& outputBuffer,
    int startSample,
    int numSamples)
{
    if (!isPlaying)
        return;

    const auto level1 =
        oscillator1Level != nullptr
        ? oscillator1Level->load()
        : 0.55f;

    const auto level2 =
        oscillator2Level != nullptr
        ? oscillator2Level->load()
        : 0.35f;

    const auto selectedWaveform1 =
        waveform1 != nullptr
        ? static_cast<int>(waveform1->load())
        : 0;

    const auto selectedWaveform2 =
        waveform2 != nullptr
        ? static_cast<int>(waveform2->load())
        : 1;

    const auto generateWave =
        [](double phase, int waveform)
        {
            switch (waveform)
            {
            case 1:
                return (2.0 * phase) - 1.0;

            case 2:
                return phase < 0.5 ? 1.0 : -1.0;

            case 3:
                return 1.0
                    - 4.0 * std::abs(phase - 0.5);

            default:
                return std::sin(
                    phase
                    * juce::MathConstants<double>::twoPi);
            }
        };
    // Читаем текущие параметры фильтра.
// Значения приходят из AudioProcessorValueTreeState.
    const auto currentCutoff =
        cutoff != nullptr
        ? cutoff->load()
        : 20000.0f;

    const auto currentResonance =
        resonance != nullptr
        ? resonance->load()
        : 0.707f;

    // Обновляем коэффициенты low-pass-фильтра.
    // Cutoff задаёт частоту среза,
    // Resonance — усиление около частоты среза.
        // Индекс типа фильтра: 0 — Low-pass, 1 — High-pass, 2 — Band-pass.
    const auto selectedFilterType =
        filterType != nullptr
        ? static_cast<int>(filterType->load())
        : 0;

    // Флаг управляет обходом фильтра в аудиопетле ниже.
    const auto isFilterEnabled =
        filterEnabled == nullptr || filterEnabled->load() >= 0.5f;

    // Настраиваем выбранный фильтр с текущими Cutoff и Resonance.
    switch (selectedFilterType)
    {
    case 1:
        filter.setCoefficients(
            juce::IIRCoefficients::makeHighPass(
                sampleRate,
                static_cast<double>(currentCutoff),
                static_cast<double>(currentResonance)));
        break;

    case 2:
        filter.setCoefficients(
            juce::IIRCoefficients::makeBandPass(
                sampleRate,
                static_cast<double>(currentCutoff),
                static_cast<double>(currentResonance)));
        break;

    default:
        filter.setCoefficients(
            juce::IIRCoefficients::makeLowPass(
                sampleRate,
                static_cast<double>(currentCutoff),
                static_cast<double>(currentResonance)));
        break;
    }

    for (int sample = 0;
        sample < numSamples;
        ++sample)
    {
        const auto envelope =
            adsr.getNextSample();

        const auto oscillator1 =
            generateWave(
                phase1,
                selectedWaveform1);

        const auto oscillator2 =
            generateWave(
                phase2,
                selectedWaveform2);

        const auto mixedSample =
            static_cast<float>(
                (oscillator1 * level1
                    + oscillator2 * level2)
                * level
                * envelope);
        // Пропускаем смешанный сигнал через low-pass-фильтр.
        // При выключенном фильтре пропускаем звук без обработки.
        const auto filteredSample = isFilterEnabled
            ? filter.processSingleSampleRaw(mixedSample)
            : mixedSample;

        phase1 += phaseIncrement1;
        phase2 += phaseIncrement2;

        if (phase1 >= 1.0)
            phase1 -= 1.0;

        if (phase2 >= 1.0)
            phase2 -= 1.0;

        for (int channel = 0;
            channel < outputBuffer.getNumChannels();
            ++channel)
        {
            outputBuffer.addSample(
                channel,
                startSample + sample,
                filteredSample);
        }

        if (!adsr.isActive())
        {
            clearCurrentNote();
            isPlaying = false;
            break;
        }
    }
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MySynthAudioProcessor();
}