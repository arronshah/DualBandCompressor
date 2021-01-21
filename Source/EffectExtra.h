//
//  EffectExtra.h
//  TestEffectAU
//
//  This file is a workspace for developing new DSP objects or functions to use in your plugin.
//

#include "PluginWrapper.h"

class Peak
{
public:
    
    void initialise(int length)
    {
        iMeasuredLength = length;
        iMeasuredItems = 0;
        fMax = fMaxOld = fMaxNew = 0.0;
    }
    
    float process(float fIn, double fAttack, double fRelease)
    {
        fAval = fabs(fIn);                                                              //peak level detector
        
        if (fAval > fMax)
        {
            fMax = fAval;
        }
        
        iMeasuredItems++;
        
        if (iMeasuredItems == iMeasuredLength)
        {
            fMaxNew = log10(fMax * 39 + 1) / fLog40;
            fMax = iMeasuredItems = 0;
        }
        
        Float32 coeff = (fMaxNew > fMaxOld) ? fAttack : fRelease;
        return fMaxOld = coeff * fMaxNew + (1 - coeff) * fMaxOld;
    }
    
    float compress (float input, float thresh, float ratio, float kneeWidth)
    {
        float x = 20.0f * log10f(input);
        float valueToSquare = x - thresh + (kneeWidth / 2.0);
        float absolute = fabs(x - thresh);

        if (2 * (x - thresh) < -kneeWidth){
            y = x;                                                                                      //no compression
        }
        else if (2 * absolute <= kneeWidth){
            y = x + (((1.0 / ratio - 1.0) * (valueToSquare * valueToSquare)) / (kneeWidth * 2.0));       //second order interpolation for soft knee
        }
        else if (2 * (x - thresh) > kneeWidth){
            y = thresh + (x - thresh) / ratio;                                                              //hard knee
        }
        
        linX = powf(10.0f, 0.05f * x);                                                                  //convert decibel result to linear gain multiplier
        linY = powf(10.0f, 0.05f * y);
        
        if(input <= 0){
            return 1;
        }
        else{
            return linY / linX;
        }
    }
    
    int iMeasuredLength, iMeasuredItems;
    float fMax, fMaxOld, fMaxNew, fAval, fThresh, fRatio, fKneeWidth;
    const float fLog40 = log10(40);
    float kneeResult;
    float y = 0.0;
    float output, linY, linX;
    
    
private:

};

class RMS
{
public:
    
    void initialise()
    {
        iMeasuredItems = fSumOfSamples = 0.0;
        
    }
    
    float process (float fIn, double fAttack, double fRelease)
    {
        fAval = fabs(fIn);                                                                      //RMS level detector
        
        fSumOfSamples += (fAval*fAval);
        
        iMeasuredItems++;
        
        if (iMeasuredItems == 512)
        {
            fSumOfSamples = fSumOfSamples / 512.0;
            fSumOfSamples = sqrt(fSumOfSamples);
            newSum = log10(fSumOfSamples * 39 + 1) / log10(40);
            initialise();
        }
        
        Float32 coeff = (newSum > oldSum) ? fAttack : fRelease;
        return oldSum = coeff * newSum + (1 - coeff) * oldSum;
    }
    
    float output;
    float fAval;
    float fSumOfOldSamples, fSumOfSamples = 0.0;
    int iMeasuredItems;
    float oldSum, newSum;
private:
    
};




