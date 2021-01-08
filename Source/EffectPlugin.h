//
//  EffectPlugin.h
//  TestEffectAU
//
//  Software Effect template project for UWE AMT/CMT/BAMT students.
//

#ifndef __EffectPlugin_h__
#define __EffectPlugin_h__

#include "PluginProcessor.h"
#include "EffectExtra.h"


class MyEffect : public Effect
{
public:
    MyEffect() : Effect() {
        initialise();
    }
    
    void initialise();
    void cleanup();
    void process(float** inputBuffers, float** outputBuffers, int numSamples);
    
    void presetLoaded(int iPresetNum, const char *sPresetName);
    void optionChanged(int iOptionMenu, int iItem);
    void buttonPressed(int iButton);
    void compressAndSendToMeter (float fInput[][2], float fMeterLevel[][2], float fMakeupGain[], float fThreshold[], float fRatio[]);
    float linearToDecibel(float parameter);
    float decibelToLinear(float decibel);
    float delay(float *buffer, float input, float fLookahead);
    

private:
    // Declare shared effect variables here
    float fComp[2][2], fCombinedSignal[2][2], fBand[2][2], fLeftComp[2], fRightComp[2];
    float fCompType, fMonoPeak, fMonoRms, fCompMonoMix, fTotalCompression, kneeWidth, fLookahead, fSR;
    float *pfCircularBuffer0, *pfCircularBuffer1;
    int iBufferSize, iBufferWritePos;
    double fRelease;

    Peak peak[2];
    RMS rms[2];
    LPF lpf[2];
    HPF hpf[2];
    
    

};

#endif
