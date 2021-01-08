//
//  EffectPlugin.cpp
//  TestEffectAU
//
//  Software Effect template project for UWE AMT/CMT/BAMT students.
//

#include "EffectPlugin.h"

////////////////////////////////////////////////////////////////////////////
// EFFECT - represents the whole effect plugin
////////////////////////////////////////////////////////////////////////////

// Called to create the effect (used to point JUCE to your effect)
Effect* JUCE_CALLTYPE createEffect() {
    return new MyEffect();
}

// Called when the effect is first created
void MyEffect::initialise()
{
    // Initialise effect variables here
    peak[0].initialise((int) (0.001 * getSampleRate()));
    peak[1].initialise((int) (0.001 * getSampleRate()));
    rms[0].initialise();
    rms[1].initialise();
    
    fSR = getSampleRate();
    iBufferSize = (int)(2.0 * fSR);
    pfCircularBuffer0 = new float[iBufferSize];
    pfCircularBuffer1 = new float[iBufferSize];
    
    for(int iPos = 0; iPos < iBufferSize; iPos++){
        pfCircularBuffer0[iPos] = 0;
        pfCircularBuffer1[iPos] = 0;
    }
    
    iBufferWritePos = 0;

    
}

void MyEffect::cleanup()
{
    // Put your own additional clean up code here (e.g. free memory)
}

// EVENT HANDLERS: handle different user input (button presses, preset selection, drop menus)

void MyEffect::presetLoaded(int iPresetNum, const char *sPresetName)
{
    // A preset has been loaded, so you could perform setup, such as retrieving parameter values
    // using getParameter and use them to set state variables in the plugin
}

void MyEffect::optionChanged(int iOptionMenu, int iItem)
{
    // An option menu, with index iOptionMenu, has been changed to the entry, iItem
}

void MyEffect::buttonPressed(int iButton)
{
    // A button, with index iButton, has been pressed
}

float MyEffect::delay(float *buffer, float input, float fLookahead)
{
    iBufferWritePos++;
    if(iBufferWritePos == iBufferSize){
        iBufferWritePos = 0;
    }
    
    int truncatedLookahead = fLookahead * 100;
    fLookahead = truncatedLookahead / 100.0;
    
    buffer[iBufferWritePos] = input;
    
    int iBufferReadPos = iBufferWritePos - (fLookahead * fSR);
    
    if(iBufferReadPos < 0){
        iBufferReadPos += iBufferSize;
    }
    
    return buffer[iBufferReadPos];
}

void MyEffect::compressAndSendToMeter(float fInput[][2], float fMeterLevel[][2], float fMakeupGain[], float fThreshold[], float fRatio[])
{
    for (int x = 0; x < 2; x++){
        for (int i = 0; i < 2; i++){
            fComp[x][i] = peak[i].compress(fMeterLevel[x][i], fThreshold[i], fRatio[i], kneeWidth);
            fCombinedSignal[x][i] = fInput[x][i] * fComp[x][i];
        }
    }
         
    float fHighPass = ((fCombinedSignal[0][0] + fCombinedSignal[1][0]) / 2.0 ) * fMakeupGain[0];
    float fLowPass = ((fCombinedSignal[0][1] + fCombinedSignal[1][1]) / 2.0) * fMakeupGain[1];
    
    fCompMonoMix = (fHighPass + fLowPass) / 2.0;
    
    fTotalCompression = (fComp[0][0] + fComp[0][1] + fComp[1][0] + fComp[1][1]) / 4.0;
    
    for (int i = 0; i < 2; i++){
        fLeftComp[i] = fCombinedSignal[0][i] * fMakeupGain[i];
        fRightComp[i] = fCombinedSignal[1][i] * fMakeupGain[i];
    }
    
    if (fCompType == 0){
        setParameter(kParam4, fMonoPeak);
    }
    else if (fCompType == 1){
        setParameter(kParam5, fMonoRms);
    }
    
    setParameter(kParam6, fTotalCompression);

}

float MyEffect::linearToDecibel(float parameter)
{
    return 20.0f * log10f(parameter);
}


// Applies audio processing to a buffer of audio
// (inputBuffer contains the input audio, and processed samples should be stored in outputBuffer)
void MyEffect::process(float** inputBuffers, float** outputBuffers, int numSamples)
{
    float fIn[2];
    float *pfInBuffer0 = inputBuffers[0], *pfInBuffer1 = inputBuffers[1];
    float *pfOutBuffer0 = outputBuffers[0], *pfOutBuffer1 = outputBuffers[1];
    float fBandPeakLevel[2][2], fBandRms[2][2];
    float fThresh[2] = {getParameter(kParam0), getParameter(kParam7)};
    float fMakeupGain[2] = {getParameter(kParam2), getParameter(kParam9)};
    float fRatio[2] = {getParameter(kParam1), getParameter(kParam8)};
    double fAttack = 0.1 - getParameter(kParam10);
    fRelease = 0.1 - getParameter(kParam11);
    float fCentreFreq = getParameter(kParam12);
    float convertToMono = getParameter(kParam13);
    float fDelSig[2][2];
    
    kneeWidth = getParameter(kParam14);
    kneeWidth = linearToDecibel(kneeWidth);
    fCompType = getParameter(kParam3);
    fLookahead = getParameter(kParam15);
    
    for (int i = 0; i < 2; i++){
        fThresh[i] = linearToDecibel(fThresh[i]);
        fMakeupGain[i]  = 1.0 + linearToDecibel(fMakeupGain[i]);
        
        if (fThresh[i] < -100.0){
            fThresh[i] = -60.0;
        }
    }

    while(numSamples--)
    {
        fIn[0] = *pfInBuffer0++;                                                    // Get sample from input
        fIn[1] = *pfInBuffer1++;
        
        for (int i = 0; i < 2; i++){
            lpf[i].setCutoff(fCentreFreq);                                           //set filter cutoffs around the centre frequency
            hpf[i].setCutoff(fCentreFreq);
        }
        
        for (int x = 0; x < 2; x++){
                fBand[x][0] = hpf[x].tick(fIn[x] * -1.0);                           //filter signal for both left and right channel
                fBand[x][1] = lpf[x].tick(fIn[x]);
            }

        fMonoPeak = (peak[0].process(fIn[0], 0.1, 0.0003) + peak[1].process(fIn[1], 0.1, 0.0003)) / 2.0;     //get average mono peak and rms values
        fMonoRms = (rms[0].process(fIn[0], 0.1, 0.0003) + rms[1].process(fIn[1], 0.1, 0.0003)) / 2.0;
        
        for (int x = 0; x < 2; x++){
            fDelSig[x][0] = delay(pfCircularBuffer0, fBand[x][0], fLookahead);
            fDelSig[x][1] = delay(pfCircularBuffer0, fBand[x][1], fLookahead);
        }
        
        for (int x = 0; x < 2; x++){
            for (int i = 0; i < 2; i++){
                fBandPeakLevel[x][i] = peak[i].process(fBand[x][i], fAttack, fRelease);      //get stereo peak and rms values with attack and release times
                fBandRms[x][i] = rms[i].process(fBand[x][i], fAttack, fRelease);
            }
        }
        
        if (fCompType == 0){
            setParameter(kParam5, 0.0);                                                             //reset rms metre
            compressAndSendToMeter(fDelSig, fBandPeakLevel, fMakeupGain, fThresh, fRatio);            //compress the signal based on the peak metre reading
        }
        else if (fCompType == 1){
            setParameter(kParam4, 0.0);
            compressAndSendToMeter(fDelSig, fBandRms, fMakeupGain, fThresh, fRatio);                  //compress the signal based on the rms metre reading
        }
        
        if (convertToMono == 0){
            *pfOutBuffer0++ = (fLeftComp[0] + fLeftComp[1]) / 2.0;                                  //output stereo compressed signal
            *pfOutBuffer1++ = (fRightComp[0] + fRightComp[1]) / 2.0;
        }
        else if (convertToMono == 1){
            *pfOutBuffer0++ = fCompMonoMix;                                                         //output mono compressed signal
            *pfOutBuffer1++ = fCompMonoMix;
        }
        
    }

}
