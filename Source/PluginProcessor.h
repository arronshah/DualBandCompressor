#ifndef __PLUGINPROCESSOR_H__
#define __PLUGINPROCESSOR_H__

#define CA_USE_AUDIO_PLUGIN_ONLY 1

#include "../JuceLibraryCode/JuceHeader.h"
#include "modules/stk_module/stk.h"

class IPluginParameters
{
public:
    virtual ~IPluginParameters() {}
    
    virtual int getNumParameters() const = 0;
    virtual float getParameter (int index) const = 0;
    virtual void setParameter (int index, float newValue) = 0;
    virtual const String getParameterName (int index) const = 0;
    virtual const String getParameterText (int index) const = 0;
};

#include "EffectEditor.h"
#include "PluginWrapper.h"

using namespace APDI;
#define Delay APDI::Delay

template <int COUNT>
class PluginParameters : public IPluginParameters
{
public:
    PluginParameters() {
        // Set up some default values..
        for(int p=0; p<COUNT; p++)
            parameters[p] = 0.0f;
    }
    
    //==============================================================================
    int getNumParameters() const
    {
        return COUNT;
    }
    
    float getParameter (int index) const
    {
        if(index >= 0 && index < COUNT)
            return parameters[index];
        return 0.0f;
    }
    
    void setParameter (int index, float newValue)
    {
        if(index >= 0 && index < COUNT)
            parameters[index] = newValue;
    }
    
    const String getParameterName (int index) const
    {
        if(index >= 0 && index < COUNT)
            return UI_CONTROLS[index].name;
        return String::empty;
    }
    
    const String getParameterText (int index) const
    {
        return String (getParameter (index), 2);
    }
private:
    float parameters[COUNT];
};

class Effect : public PluginParameters<kNumberOfParameters> {
public:
    Effect() {
        APDI::SAMPLE_RATE = 44100.0; // sample rate potentially not valid before playback
        
        for(int p=0; p<kNumberOfParameters; p++)
            setParameter(p, UI_CONTROLS[p].initial);
    }
    
    virtual void initialise() {}
    virtual void cleanup() {}

    virtual void presetLoaded(int iPresetNum, const char *sPresetName) {}
    virtual void optionChanged(int iOptionMenu, int iItem) {}
    virtual void buttonPressed(int iButton) {}
    
    virtual void process(float** inputBuffers, float** outputBuffers, int numSamples) {}
    
//    void setCurrentPlaybackSampleRate (const double newRate){
//        Synthesiser::setCurrentPlaybackSampleRate(APDI::SAMPLE_RATE = newRate);
//    }
};

//==============================================================================
/**
*/
class PluginAudioProcessor  : public AudioProcessor, public ChangeListener //, public IPluginParameters
{
    friend class PluginAudioProcessorEditor;
public:
    //==============================================================================
    PluginAudioProcessor();
    ~PluginAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    void reset();

    //==============================================================================
    bool hasEditor() const                  { return pEditor != NULL; }
    AudioProcessorEditor* createEditor();

    //==============================================================================
    const String getName() const            { return JucePlugin_Name; }

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;
    
    int getNumParameters();
    float getParameter (int index);
    void setParameter (int index, float newValue);
    const String getParameterName (int index);
    const String getParameterText (int index);

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int /*index*/);
    const String getProgramName (int /*index*/);
    void changeProgramName (int /*index*/, const String& /*newName*/){}
    void setBypass(bool bypass = true){ isBypassed = bypass; }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);
    
    void changeListenerCallback (ChangeBroadcaster* source) override;

    // this is kept up to date with the midi messages that arrive, and the UI component
    // registers with it so it can represent the incoming messages
    MidiKeyboardState keyboardState;

    // this keeps a copy of the last set of time info that was acquired during an audio
    // callback - the UI component will read this and display it.
    AudioPlayHead::CurrentPositionInfo lastPosInfo;

    // these are used to persist the UI's size - the values are stored along with the
    // filter's other parameters, and the UI component will update them when it gets
    // resized.
    int lastUIWidth, lastUIHeight;
    
    void onButtonClicked(int control) {}
    void loadResource(const char* filename);
    AudioTransportSource* getTransport() { return &transportSource; }

private:
    AudioProcessorEditor* pEditor;
    
    Effect* effect;
    
    int program;
    bool isBypassed;
    
    AudioFormatManager formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginAudioProcessor)
};

#endif  // __PLUGINPROCESSOR_H_526ED7A9__
