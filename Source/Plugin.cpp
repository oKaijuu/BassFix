#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class BassPresenceProcessor : public juce::AudioProcessor
{
public:
    BassPresenceProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMS", createLayout())
    {
        srcFreq  = apvts.getRawParameterValue("srcFreq");
        hpFreq   = apvts.getRawParameterValue("hpFreq");
        driveDb  = apvts.getRawParameterValue("drive");
        amountDb = apvts.getRawParameterValue("amount");
        warmth   = apvts.getRawParameterValue("warmth");
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID{"srcFreq", 1}, "Fonte LP (Hz)",
            NormalisableRange<float>(60.0f, 400.0f, 1.0f, 0.5f), 150.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID{"hpFreq", 1}, "Harmonicos HP (Hz)",
            NormalisableRange<float>(60.0f, 250.0f, 1.0f, 0.5f), 90.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID{"drive", 1}, "Drive (dB)",
            NormalisableRange<float>(0.0f, 30.0f, 0.1f), 12.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID{"amount", 1}, "Nivel dos harmonicos (dB)",
            NormalisableRange<float>(-40.0f, 0.0f, 0.1f), -15.0f));

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID{"warmth", 1}, "Warmth (pares)",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        return layout;
    }

    const juce::String getName() const override { return "Bass Presence"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        const auto out = layouts.getMainOutputChannelSet();
        if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
            return false;
        return out == layouts.getMainInputChannelSet();
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
        spec.numChannels = (juce::uint32) juce::jmax(1, getTotalNumOutputChannels());

        for (auto* f : { &lp1, &lp2 })
        {
            f->prepare(spec);
            f->setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        }
        for (auto* f : { &hp1, &hp2 })
        {
            f->prepare(spec);
            f->setType(juce::dsp::StateVariableTPTFilterType::highpass);
        }
        preparedChannels = (int) spec.numChannels;
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        const int numCh = juce::jmin(buffer.getNumChannels(), preparedChannels);
        const int numSamples = buffer.getNumSamples();

        const float src = srcFreq->load();
        const float hp  = hpFreq->load();
        for (auto* f : { &lp1, &lp2 }) f->setCutoffFrequency(src);
        for (auto* f : { &hp1, &hp2 }) f->setCutoffFrequency(hp);

        const float drive = juce::Decibels::decibelsToGain(driveDb->load());
        const float amt   = juce::Decibels::decibelsToGain(amountDb->load());
        const float bias  = warmth->load() * 0.5f;
        const float biasOffset = std::tanh(bias);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const float dry = data[i];

                float x = lp2.processSample(ch, lp1.processSample(ch, dry));
                float y = std::tanh(drive * x + bias) - biasOffset;
                y = hp2.processSample(ch, hp1.processSample(ch, y));

                data[i] = dry + amt * y;
            }
        }
    }

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor(*this);
    }

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        if (auto xml = apvts.copyState().createXml())
            copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (auto xml = getXmlFromBinary(data, sizeInBytes))
            if (xml->hasTagName(apvts.state.getType()))
                apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

private:
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* srcFreq  = nullptr;
    std::atomic<float>* hpFreq   = nullptr;
    std::atomic<float>* driveDb  = nullptr;
    std::atomic<float>* amountDb = nullptr;
    std::atomic<float>* warmth   = nullptr;

    juce::dsp::StateVariableTPTFilter<float> lp1, lp2, hp1, hp2;
    int preparedChannels = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassPresenceProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassPresenceProcessor();
}
