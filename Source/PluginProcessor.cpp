#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioProcessor* JUCE_CALLTYPE createPluginFilter();
Effect* JUCE_CALLTYPE createEffect(); // callback to create the plugin instance (e.g. new MyEffect())

//==============================================================================
PluginAudioProcessor::PluginAudioProcessor()
: pEditor(NULL)
{
    program = 0;
    
    lastUIWidth = 1280;
    lastUIHeight = 600;

    lastPosInfo.resetToDefault();

    effect = createEffect();
    
    APDI::SAMPLE_RATE = 44100;
    
    formatManager.registerBasicFormats();
    transportSource.addChangeListener (this);
    
    loadResource("acousticguitar.aif");
}

PluginAudioProcessor::~PluginAudioProcessor()
{
    delete effect;
    effect = NULL;
}

void PluginAudioProcessor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == &transportSource)
        (reinterpret_cast<PluginAudioProcessorEditor*>(pEditor))->setPlaybackState(transportSource.isPlaying());
}

//==============================================================================
int PluginAudioProcessor::getNumParameters()
{
    return kNumberOfParameters;
}

float PluginAudioProcessor::getParameter (int index)
{
    // This method will be called by the host, probably on the audio thread, so
    // it's absolutely time-critical. Don't use critical sections or anything
    // UI-related, or anything at all that may block in any way!
    return effect->getParameter(index);
}

void PluginAudioProcessor::setParameter (int index, float newValue)
{
    // This method will be called by the host, probably on the audio thread, so
    // it's absolutely time-critical. Don't use critical sections or anything
    // UI-related, or anything at all that may block in any way!
    effect->setParameter(index, newValue);
}

const String PluginAudioProcessor::getParameterName (int index)
{
    return effect->getParameterName(index);
}

const String PluginAudioProcessor::getParameterText (int index)
{
    return String (getParameter (index), 2);
}

//==============================================================================
void PluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    APDI::SAMPLE_RATE = sampleRate;
    keyboardState.reset();
    
    transportSource.prepareToPlay (samplesPerBlock, sampleRate);
    stk::Stk::setSampleRate(sampleRate);
}

void PluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    transportSource.releaseResources();
    keyboardState.reset();
}

void PluginAudioProcessor::reset()
{
    // Use this method as the place to clear any delay lines, buffers, etc, as it
    // means there's been a break in the audio's continuity.
}

void PluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    
    // Pass any incoming midi messages to our keyboard state object, and let it
    // add messages to the buffer if the user is clicking on the on-screen keys
    keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
    
    APDI::SAMPLE_RATE = this->getSampleRate();
    
    static AudioSampleBuffer input(buffer.getNumChannels(), buffer.getNumSamples());
    input = buffer;
    

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
        input.clear (i, 0, numSamples);
    
    // get internal file playback (test sounds)
    AudioSourceChannelInfo channel(&buffer, 0, numSamples);
    transportSource.getNextAudioBlock (channel);
    input.addFrom(0, 0, buffer.getSampleData(0), numSamples);
    input.addFrom(1, 0, buffer.getSampleData(1), numSamples);
    
    // and now get the effect to process the input audio and generate its output.
    if(!isBypassed){
        buffer.clear();
        effect->process(input.getArrayOfChannels(), buffer.getArrayOfChannels(), numSamples);
    }
    
    if (getActiveEditor()){
        PluginAudioProcessorEditor* editor = dynamic_cast<PluginAudioProcessorEditor*>(pEditor);
        
        AudioSampleBuffer& mono = input;
        mono = buffer;
        mono.addFrom(0, 0, input.getSampleData(1), numSamples);
        mono.applyGain(0.5);

        if(editor && editor->scope_mode & SCOPE_VISIBLE){
            if(editor->oscilloscope && (editor->scope_mode & SCOPE_OSCILLOSCOPE))
                editor->oscilloscope->processBlock(mono.getSampleData(0), numSamples);
            else if(editor->spectrum && (editor->scope_mode & SCOPE_SPECTRUM))
                editor->spectrum->copySamples(mono.getSampleData(0), numSamples);
            else if(editor->sonogram)
                editor->sonogram->copySamples(mono.getSampleData(0), numSamples);
        }
    }
    
    // ask the host for the current time so we can display it...
    AudioPlayHead::CurrentPositionInfo newTime;

    if (getPlayHead() != nullptr && getPlayHead()->getCurrentPosition (newTime))
    {
        // Successfully got the current time from the host..
        lastPosInfo = newTime;
    }
    else
    {
        // If the host fails to fill-in the current time, we'll just clear it to a default..
        lastPosInfo.resetToDefault();
    }
}

void PluginAudioProcessor::loadResource(const char* filename){
    CFBundleRef plugBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.UWE.TestEffectAU"));
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(plugBundle);
    char path[PATH_MAX];
    CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX);
    CFRelease(resourcesURL);
    
    File file(std::string(path) + "/" + filename);
    auto* reader = formatManager.createReaderFor (file);
    if (reader != nullptr)
    {
        std::unique_ptr<AudioFormatReaderSource> newSource (new AudioFormatReaderSource (reader, true)); // [11]
        newSource->setLooping(true);
        transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);                     // [12]
        readerSource.reset (newSource.release());                                                        // [14]
    }
}

//==============================================================================
AudioProcessorEditor* PluginAudioProcessor::createEditor()
{
    return pEditor = new PluginAudioProcessorEditor (this);
}

//==============================================================================
void PluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // Here's an example of how you can use XML to make it easy and more robust:

    // Create an outer XML element..
    XmlElement xml ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xml.setAttribute ("uiWidth", lastUIWidth);
    xml.setAttribute ("uiHeight", lastUIHeight);
    
    for(int p=0; p<getNumParameters(); p++){
        String name;
        for (String::CharPointerType t (UI_CONTROLS[p].name.getCharPointer()); ! t.isEmpty(); ++t){
            if(t.isLetterOrDigit() || *t == '_' || *t == '-' || *t == ':'){
                name += *t;
            }
        }
        xml.setAttribute(name, getParameter(p));
    }

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xml, destData);
}

void PluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // This getXmlFromBinary() helper function retrieves our XML from the binary blob..
    ScopedPointer<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // make sure that it's actually our type of XML object..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our parameters..
            lastUIWidth  = xmlState->getIntAttribute ("uiWidth", lastUIWidth);
            lastUIHeight = xmlState->getIntAttribute ("uiHeight", lastUIHeight);

            for(int p=0; p<getNumParameters(); p++){
                String name;
                for (String::CharPointerType t (UI_CONTROLS[p].name.getCharPointer()); ! t.isEmpty(); ++t){
                    if(t.isLetterOrDigit() || *t == '_' || *t == '-' || *t == ':'){
                        name += *t;
                    }
                }
                setParameter(p, (float) xmlState->getDoubleAttribute (name, getParameter(p)));
            }
        }
    }
}

const String PluginAudioProcessor::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String PluginAudioProcessor::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool PluginAudioProcessor::isInputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool PluginAudioProcessor::isOutputChannelStereoPair (int /*index*/) const
{
    return true;
}

bool PluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double PluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginAudioProcessor::getNumPrograms()                                                {
    return sizeof(UI_PRESETS) / sizeof(Preset);
}
int PluginAudioProcessor::getCurrentProgram(){
    return program;
}
void PluginAudioProcessor::setCurrentProgram (int index){
    program = index;
    for(int p=0; p<kNumberOfParameters; p++){
        setParameter(p, UI_PRESETS[index].value[p]);
    }
}
const String PluginAudioProcessor::getProgramName (int index)                         {
    return UI_PRESETS[index].name;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginAudioProcessor();
}
