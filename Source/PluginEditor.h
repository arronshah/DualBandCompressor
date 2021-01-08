#ifndef __PLUGINEDITOR_H__
#define __PLUGINEDITOR_H__

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "EffectEditor.h"

using namespace drow;

enum UI_MODE
{
    SCOPE_HIDDEN = 0,
    SCOPE_VISIBLE = 1,
    SCOPE_OSCILLOSCOPE = 2,
    SCOPE_SPECTRUM = 4,
    SCOPE_SONOGRAM = 8
};

class Meter : public Slider
{
public:
    String getTextFromValue (double value) override
    {
        return "";
    }
};

//==============================================================================
/** This is the editor component that our filter will display.
*/
class PluginAudioProcessorEditor  : public AudioProcessorEditor,
                                            public SliderListener,
                                            public ButtonListener,
                                            public ComboBoxListener,
                                            public Timer
{
    friend class PluginAudioProcessor;
public:
    PluginAudioProcessorEditor (PluginAudioProcessor* ownerFilter);
    ~PluginAudioProcessorEditor();

    //==============================================================================
    void timerCallback();
    void paint (Graphics& g);
    void resized();
    
    void userTriedToCloseWindow();
    
    void sliderValueChanged (Slider*);
    void buttonClicked(Button*);
    void buttonStateChanged(Button* button);
    void comboBoxChanged (ComboBox* comboBox);
    
    void setPlaybackState(bool playing);

private:
    int scope_mode;
    AudioOscilloscope *oscilloscope;
    Spectroscope *spectrum;
    Sonogram *sonogram;
    TimeSliceThread scopeThread;
    
    TabbedComponent tabScope;
    
    Label infoLabel;
    
    ComboBox listTestSounds;
    Label labelTestSounds;
    TextButton btnPlayback[4];
    
    Label label[kNumberOfControls];
    Component* controls[kNumberOfControls];
    
    ScopedPointer<ResizableCornerComponent> resizer;
    ComponentBoundsConstrainer resizeLimits;

    AudioPlayHead::CurrentPositionInfo lastDisplayedPosition;

    PluginAudioProcessor* getProcessor() const
    {
        return static_cast <PluginAudioProcessor*> (getAudioProcessor());
    }

    void displayPositionInfo (const AudioPlayHead::CurrentPositionInfo& pos);
};


#endif  // __PLUGINEDITOR_H_4ACCBAA__
