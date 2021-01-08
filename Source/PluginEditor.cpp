#include "PluginProcessor.h"
#include "PluginEditor.h"

enum { PLAY, STOP, BYPASS };

//==============================================================================
PluginAudioProcessorEditor::PluginAudioProcessorEditor (PluginAudioProcessor* ownerFilter)
    : AudioProcessorEditor (ownerFilter),
      scope_mode(SCOPE_VISIBLE|SCOPE_SONOGRAM), oscilloscope(NULL), spectrum(NULL), sonogram(NULL), scopeThread("Scope Thread"),
      tabScope(TabbedButtonBar::TabsAtTop), infoLabel (String::empty)
{    
    // add controls..
    for(int c=0; c<kNumberOfControls; c++){
        const Control& control = UI_CONTROLS[c];
        
        switch(control.type){
        case ROTARY:
        case SLIDER:
        case METER:
        {   controls[c] = control.type == METER ? new Meter() : new Slider();
            Slider* pSlider = (Slider*)controls[c];
        
            addAndMakeVisible (pSlider);
            
            switch((int)control.type){
            case ROTARY: pSlider->setSliderStyle (Slider::Rotary); break;
            case METER:  pSlider->setEnabled(false);
                         pSlider->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            case SLIDER: pSlider->setSliderStyle (control.size.getWidth() > control.size.getHeight() ? Slider::LinearBar : Slider::LinearBarVertical); break;
                    break;
            }
            pSlider->setTextBoxStyle (Slider::TextBoxBelow, false, 50, 20);
            pSlider->addListener (this);
            pSlider->setRange (UI_CONTROLS[c].min, UI_CONTROLS[c].max, 0.00001); // HEY CHRIS - I MODIFIED THIS!
            
            // add a label
            label[c].setText(UI_CONTROLS[c].name, dontSendNotification);
            label[c].attachToComponent(controls[c], false);
            label[c].setFont (Font (11.0f));
            label[c].setJustificationType(Justification::centred);
        } break;
        case TOGGLE:
        case BUTTON:
        {   controls[c] = new TextButton();
            TextButton* pButton = (TextButton*)controls[c];
                
            addAndMakeVisible (pButton);
            pButton->addListener (this);
            pButton->setButtonText(UI_CONTROLS[c].name);
            pButton->setClickingTogglesState(UI_CONTROLS[c].type == TOGGLE);
        } break;
        case MENU:
        {
            controls[c] = new ComboBox();
            ComboBox* pList = (ComboBox*)controls[c];
            addAndMakeVisible(pList);
            for(int i=0; i<8 && UI_CONTROLS[c].options[i]; i++)
                pList->addItem(UI_CONTROLS[c].options[i], i+1);
            pList->addListener (this);
            
            // add a label
            label[c].setText(UI_CONTROLS[c].name, dontSendNotification);
            label[c].attachToComponent(controls[c], false);
            label[c].setFont (Font (11.0f));
            label[c].setJustificationType(Justification::centred);
        }
        }

    }
    
    const char* szTestSounds[] = {
        "acousticguitar.aif",
        "constanttone.aif",
        "drumpattern.aif",
        "drumwithnoise.aif",
        "drumwithreverb.aif",
        "electricguitar.aif",
        "leftrightEP.aif",
        "leftrightsynth.aif",
        "mandolin.aif",
        "metronome.aif",
        "stereosynth.aif",
        "drumbreak.aif",
        "slapbass.aif",
        "femalevocal.aif"
    };
    
    const int nbTestSounds = sizeof(szTestSounds)/sizeof(const char*);
    
    // add test sounds lists
    addAndMakeVisible(&listTestSounds);
    for(int i=0; i < nbTestSounds; i++)
        listTestSounds.addItem(szTestSounds[i], i+1);
    listTestSounds.addListener (this);
    listTestSounds.setSelectedItemIndex(0);
    
    // add a label
    addAndMakeVisible(&labelTestSounds);
    labelTestSounds.setText("Test Sounds", dontSendNotification);
//    labelTestSounds.attachToComponent(&listTestSounds, false);
    labelTestSounds.setFont (Font (11.0f));
    labelTestSounds.setJustificationType(Justification::left);
    
    // add play buttons
    const char* szButtons[] = { "Play", "Stop", "Bypass" };
    for(int b=0; b<3; b++){
        addAndMakeVisible (&btnPlayback[b]);
        btnPlayback[b].setClickingTogglesState(b == 0 || b == 2);
        btnPlayback[b].addListener (this);
        btnPlayback[b].setButtonText(szButtons[b]);
    }
    
    // add an oscilloscope..
    oscilloscope = new AudioOscilloscope();
    oscilloscope->setHorizontalZoom(0.001);
    oscilloscope->setTraceColour(Colours::white);
    
    spectrum = new Spectroscope(10);
    spectrum->setLogFrequencyDisplay(true);
    
    sonogram = new Sonogram(10);
    sonogram->setLogFrequencyDisplay(true);
    
    addAndMakeVisible(&tabScope);
    tabScope.addTab("Oscilloscope", Colours::whitesmoke, oscilloscope, false, 0);
    tabScope.addTab("Spectrum", Colours::whitesmoke, spectrum, false, 1);
    tabScope.addTab("Sonogram", Colours::whitesmoke, sonogram, false, 2);
    tabScope.setTabBarDepth(24);
    tabScope.setIndent(4);

    // add the triangular resizer component for the bottom-right of the UI
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &resizeLimits));
    resizeLimits.setSizeLimits (400, 250, 1280, 720);

    // set our component's initial size to be the last one that was stored in the filter's settings
    setSize (  ownerFilter->lastUIWidth,
               ownerFilter->lastUIHeight);

    startTimer (50);
}

PluginAudioProcessorEditor::~PluginAudioProcessorEditor()
{
    scope_mode = SCOPE_HIDDEN;
    
    removeChildComponent(&tabScope);

    scopeThread.stopThread(1000);
    stopTimer();
    
    for(int c=0; c<kNumberOfControls; c++){
        delete controls[c];
        controls[c] = NULL;
    }
    
    delete spectrum;
    spectrum = NULL;
    
    delete sonogram;
    sonogram = NULL;
    
    delete oscilloscope;
    oscilloscope = NULL;
}

void PluginAudioProcessorEditor::setPlaybackState(bool playing){
    if(!playing)
        btnPlayback[PLAY].setToggleState(playing, dontSendNotification);
}

void PluginAudioProcessorEditor::userTriedToCloseWindow(){
    scope_mode = SCOPE_HIDDEN;
}

//==============================================================================
void PluginAudioProcessorEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Colours::white, 0, 0, Colours::grey, 0, (float) getHeight(), false));
    g.fillAll();
}

void PluginAudioProcessorEditor::resized()
{
    Bounds size;
    for(int c=0; c<kNumberOfControls; c++){
        if(UI_CONTROLS[c].size == AUTO_SIZE){
            int column = c % 5;
            int row = c / 5;
            
            switch (UI_CONTROLS[c].type){
                case ROTARY:
                case SLIDER:
                case METER:
                    size.setBounds(20 + 75 * column, 30 + 120 * row, 50, 80);
                    break;
                case TOGGLE:
                case BUTTON:
                    size.setBounds (15 + 75 * column, 10 + 120 * row, 60, 20);
                    break;
                case MENU:
                    size.setBounds (15 + 75 * column, 30 + 120 * row, 60, 20);
                    break;
            }
        }else{
            size = UI_CONTROLS[c].size;
        }

        controls[c]->setBounds (size.getX(), size.getY(), size.getWidth(), size.getHeight());
        label[c].setTopLeftPosition(size.getX() - 20, size.getY() - 20);
        label[c].setSize(size.getWidth() + 40, 20);
    }
    
    for(int b=0; b<3; b++){
        btnPlayback[b].setBounds(230 + b*48, MAX(260, getHeight() - 40), 45, 20);
    }
    
    labelTestSounds.setBounds(15, MAX(260, getHeight() - 40), 60, 20);
    listTestSounds.setBounds(77, MAX(260, getHeight() - 40), 145, 20);
    tabScope.setBounds(400, 4, getWidth() - 403 - 6, getHeight() - 12);
    
    resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
    
    scope_mode = (scope_mode & ~SCOPE_VISIBLE) | (getWidth() > 400 ? SCOPE_VISIBLE : SCOPE_HIDDEN);
    
    getProcessor()->lastUIWidth = getWidth();
    getProcessor()->lastUIHeight = getHeight();
}

//==============================================================================
// This timer periodically checks whether any of the filter's parameters have changed...
void PluginAudioProcessorEditor::timerCallback()
{
    PluginAudioProcessor* ourProcessor = getProcessor();

    AudioPlayHead::CurrentPositionInfo newPos (ourProcessor->lastPosInfo);

    if (lastDisplayedPosition != newPos)
        displayPositionInfo (newPos);

    for(int c=0; c<kNumberOfControls && controls[c]; c++){
        switch (UI_CONTROLS[c].type){
        case ROTARY:
        case SLIDER:
        case METER:
            ((Slider*)controls[c])->setValue (ourProcessor->getParameter(c), dontSendNotification);
            break;
        case MENU:
            ((ComboBox*)controls[c])->setSelectedId(ourProcessor->getParameter(c)+1, dontSendNotification);
            break;
        case BUTTON:
            break;
        case TOGGLE:
            ((TextButton*)controls[c])->setToggleState(ourProcessor->getParameter(c) != 0.0, sendNotification);
            break;
        }
    }
    
    if (scope_mode & SCOPE_VISIBLE) {
        scope_mode = SCOPE_VISIBLE | (2 << tabScope.getCurrentTabIndex());
        
        if(spectrum && (scope_mode & SCOPE_SPECTRUM)){
            spectrum->process();
            spectrum->timerCallback();
        }else if(sonogram && (scope_mode & SCOPE_SONOGRAM)){
            sonogram->process();
            sonogram->timerCallback();
        }
    }
}

// This is our Slider::Listener callback, when the user drags a slider.
void PluginAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
    for(int c=0; c<kNumberOfControls; c++){
        if (slider == controls[c])
        {
            // It's vital to use setParameterNotifyingHost to change any parameters that are automatable
            // by the host, rather than just modifying them directly, otherwise the host won't know
            // that they've changed.
            getProcessor()->setParameterNotifyingHost (c, (float) slider->getValue());
        }
    }
}


void PluginAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    for(int c=0; c<kNumberOfControls; c++){
        if (comboBox == controls[c])
        {
            // It's vital to use setParameterNotifyingHost to change any parameters that are automatable
            // by the host, rather than just modifying them directly, otherwise the host won't know
            // that they've changed.
            getProcessor()->setParameterNotifyingHost (c, (float) comboBox->getSelectedId() - 1);
            getProcessor()->effect->optionChanged(c, comboBox->getSelectedId() - 1);
        }
    }
    
    if(comboBox == &listTestSounds){
        bool bPlaying = getProcessor()->getTransport()->isPlaying();
        getProcessor()->loadResource(listTestSounds.getText().getCharPointer());
        if(bPlaying){
            getProcessor()->getTransport()->setPosition(0.0);
            getProcessor()->getTransport()->start();
        }
    }
}

void PluginAudioProcessorEditor::buttonClicked(Button* button)
{
    for(int c=0; c<kNumberOfControls; c++){
        if (button == controls[c])
        {
            getProcessor()->onButtonClicked(c);
            getProcessor()->effect->buttonPressed(c);
        }
    }
}

void PluginAudioProcessorEditor::buttonStateChanged(Button* button)
{
    for(int c=0; c<kNumberOfControls; c++){
        if (button == controls[c])
        {
            if(UI_CONTROLS[c].type == TOGGLE){
                // It's vital to use setParameterNotifyingHost to change any parameters that are automatable
                // by the host, rather than just modifying them directly, otherwise the host won't know
                // that they've changed.
                getProcessor()->setParameterNotifyingHost(c, button->getToggleState() ? 1.0 : 0.0);
            }
        }
    }
    
    if(button == &btnPlayback[BYPASS]){
        getProcessor()->setBypass(button->getToggleState());
    }else if(button->isDown()){
        AudioTransportSource* transport = getProcessor()->getTransport();
        if(button == &btnPlayback[PLAY]){
            if(transport->isPlaying())
                btnPlayback[PLAY].setToggleState(false, dontSendNotification);
            transport->setPosition(0.0);
            transport->start();
        }else if(button == &btnPlayback[STOP]){
            transport->stop();
        }
    }
}

// Updates the text in our position label.
void PluginAudioProcessorEditor::displayPositionInfo (const AudioPlayHead::CurrentPositionInfo& pos)
{
    lastDisplayedPosition = pos;
    String displayText;
    displayText.preallocateBytes (128);

    displayText << String (pos.bpm, 2) << " bpm, "
                << pos.timeSigNumerator << '/' << pos.timeSigDenominator
                << "  -  " << timeToTimecodeString (pos.timeInSeconds)
                << "  -  " << ppqToBarsBeatsString (pos.ppqPosition, pos.ppqPositionOfLastBarStart,
                                                    pos.timeSigNumerator, pos.timeSigDenominator);

    if (pos.isRecording)
        displayText << "  (recording)";
    else if (pos.isPlaying)
        displayText << "  (playing)";

    infoLabel.setText (displayText, dontSendNotification);
}
