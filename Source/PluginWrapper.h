//
//  PluginWrapper.h
//  Effect & Synth Plugin Wrapper
//
//  Created by Chris Nash on 02/10/2013.
//  Updated by Chris Nash on 18/01/2016 with support for OS X 10.10 SDK.
//  Updated by Chris Nash on 02/10/2018 with support for TestEffectAU.
//
//  This file describes a number of abstractions and extensions to STK,
//  to support audio processing and programming teaching at UWE.

#ifndef _PluginWrapper_h_
#define _PluginWrapper_h_

#include "PluginProcessor.h"

//==============================================================================
// DSP OBJECTS - These STK objects have been adapted to support UWE development.
// The original STK objects they are based on are identified by the stk:: label
// (e.g. SineWave for sine). To find out more about them, you can either drill
// down to see their implementation (selecting them and "Jump to Definition"),
// or look them up in the STK documentation (found online).
//==============================================================================

namespace APDI {
    
    static float SAMPLE_RATE = 0.0f;
    static float getSampleRate() { return APDI::SAMPLE_RATE; }
    
    typedef stk::Generator Oscillator;
    
    class Sine : public stk::SineWave
    {
    public:
        void setFrequency(float f) {
            frequency = f;
            stk::SineWave::setFrequency(f);
        }
        
        float getFrequency() const {
            return frequency;
        }
    protected:
        float frequency;
    };
    
    class Square : public stk::BlitSquare {};
    // class Triangle {};
    class Saw : public stk::BlitSaw {};
    class Noise : public stk::Noise {};
    
    class Delay : public stk::DelayL {};
    
    class Filter : public stk::BiQuad {};
    class LPF : public Filter {
    public:
        LPF() : Filter() {
            setCutoff(20000.0);
        }
        
        void setCutoff(float frequency){
            Float32 fOmega = M_PI * (frequency/sampleRate());
            Float32 fKval = tan(fOmega);
            Float32 fKvalsq = fKval * fKval;
            Float32 fRootTwo = sqrt(2.0);
            Float32 ffrac = 1.0 / (1.0 + fRootTwo * fKval + fKvalsq);
            
            setB0(fKvalsq * ffrac);
            setB1(2.0 * fKvalsq * ffrac);
            setB2(fKvalsq * ffrac);
            
            //      setA0(0.0);
            setA1(2.0 * (fKvalsq - 1.0) * ffrac);
            setA2((1.0 - fRootTwo * fKval + fKvalsq) * ffrac);
        }
    };
    class HPF : public Filter {
    public:
        HPF() : Filter() {
            setCutoff(0.0);
        }
        
        void setCutoff(float frequency){
            Float32 fOmega = M_PI * (frequency/sampleRate());
            Float32 fKval = tan(fOmega);
            Float32 fKvalsq = fKval * fKval;
            Float32 fRootTwo = sqrt(2.0);
            Float32 ffrac = 1.0 / (1.0 + fRootTwo * fKval + fKvalsq);
            
            setB0(ffrac);
            setB1(-2.0 * ffrac);
            setB2(ffrac);
            
            //      setA0(0.0);
            setA1(2.0 * (fKvalsq - 1.0) * ffrac);
            setA2((1.0 - fRootTwo * fKval + fKvalsq) * ffrac);
        }
    };
    
    class BPF : public Filter {
    public:
        BPF() : Filter() {
            set(1000.0, 100.0);
        }
        
        void setQ(float centre, float Q){
            set(centre, centre / Q);
        }
        
        void set(float centre, float bandwidth){
            const float fSampleRate = sampleRate();
            
            // if possible, better to fix out of range values than fail silently
            if(centre < 20) centre = 20; // value of 20 produces less clicks than allowing all the way to 0
            if(bandwidth < 0) bandwidth = 0;
            
            // Check for values which cause instability and make corrections (rather than
            // failing completely by returning)
            // In particular, tan(PI/2) tends to infinity, so must be avoided:
            // --> fc must be < 0.5 Fs
            // --> bw must be < 0.25 Fs
            // keep them slightly under just to be on the safe side
            if(centre > 0.49 * fSampleRate) {
                centre = 0.49 * fSampleRate;
            }
            if(bandwidth > 0.24 * fSampleRate) {
                bandwidth = 0.24 * fSampleRate;
            }
            
            Float32 fOmegaA = M_PI * (centre/fSampleRate);
            Float32 fOmegaB = M_PI * (bandwidth/fSampleRate);
            Float32 fCval = (tan(fOmegaB) - 1) / (tan(2.0 * fOmegaB) + 1);
            Float32 fDval = -1.0 * cos(2.0 * fOmegaA);
            
            setB0(-1.0 * fCval);
            setB1(fDval * (1.0 - fCval));
            setB2(1.0);
            
            //		setA0(0.0);
            setA1(-1.0 * fDval * (1.0 - fCval));
            setA2(fCval);
        }
        
        float tick(float sample){
            inputs_[2] = inputs_[1];
            inputs_[1] = inputs_[0];
            outputs_[2] = outputs_[1];
            outputs_[1] = outputs_[0];
            inputs_[0] = sample;
            
            outputs_[0] = inputs_[0] * b_[0]
            + inputs_[1] * b_[1]
            + inputs_[2] * b_[2]
            + outputs_[1] * a_[1]
            + outputs_[2] * a_[2];
            
            return 0.5 * (inputs_[0] - outputs_[0]); // BPF
            //      return 0.5 * (fFiltvalx[0] + fFiltvaly[0]); // BSF
        }
    };
    
    
    class Envelope : public stk::Envelope {
    public:
        enum STAGE
        {
            ENV_SUSTAIN,
            ENV_RELEASE,
            ENV_OFF
        };
        
        struct Point
        {
            float x;
            float y;
        };
        
        struct Points : public Point
        {
            Points(float x, float y){
                Point::x = x;
                Point::y = y;
                next = NULL;
            }
            Points(){
                delete next;
            }
            
            Points& operator()(float x, float y){
                last().next = new Points(x, y);
                return *this;
            }
            
            Points& last(){
                return next ? next->last() : *this;
            }
            
            int count() const {
                return next ? 1 + next->count() : 1;
            }
            
            Points* next;
        };
        
        struct Loop
        {
            Loop(int from = -1, int to = -1) : start(from), end(to) {}
            
            void set(int from, int to) { start = from; end = to; }
            void reset() { start = end = -1; }
            
            bool isActive() const { return start != -1 && end != -1; }
            
            int start;
            int end;
        };
        
        Envelope() : stk::Envelope() {
            set(Points(0.0,1.0));
            setLoop(0,0);
        }
        
        Envelope(const Points& points) : stk::Envelope() {
            set(points);
            setLoop(0,0);
        }
        
        void set(const Points& point){
            points.clear();
            
            const Points* pPoint = &point;
            while(pPoint){
                points.push_back(*pPoint);
                pPoint = pPoint->next;
            }
            
            initialise();
        }
        
        void setLoop(int startPoint, int endPoint){
            if(startPoint >= 0 && endPoint < points.size())
                loop.set(startPoint, endPoint);
        }
        
        void resetLoop(){
            loop.reset();
            if(stage == ENV_SUSTAIN && (point+1) < points.size())
                setTarget(points[point+1], points[point].x);
        }
        
        void setStage(STAGE stage){ this->stage = stage; }
        const STAGE getStage() const { return stage; }
        
        float getLength() const { return points.size() ? points[points.size() - 1].x : 0.0; }
        
        void release(float time){
            stage = ENV_RELEASE;
            setTime(time);
            stk::Envelope::setTarget(0.0);
        }
        
        void initialise(){
            point = 0;
            timeInc = 1.0 / getSampleRate();
            loop.reset();
            stage = ENV_SUSTAIN;
            if(points.size()){
                setValue(points[0].y);
                if(points.size() > 1)
                    setTarget(points[1], points[0].x);
            }else{
                stk::Envelope::setValue(1.0);
            }
        }
        
        void resize(int samples){
            float length = getLength();
            if(length == 0.0)
                return;
            
            float multiplier = samples/(sampleRate() * length);
            std::vector<Point>::iterator point = points.begin();
            while(point != points.end()){
                point->x *= multiplier;
                point++;
            }
            
            initialise();
        }
        
        void setTarget(Point& point, float time = 0.0){
            this->time = time;
            stk::Envelope::setTarget(point.y);
            stk::Envelope::setRate(fabs(point.y - value_) / ((point.x - time) * Stk::sampleRate()));
        }
        
        float tick(){
            float amplitude = stk::Envelope::tick();
            
            if(stage == ENV_SUSTAIN){
                time += timeInc;
                if(stk::Envelope::getState() == 0){ // envelop segment end reached
                    if(loop.isActive() && (point+1) >= loop.end){
                        point = loop.start;
                        stk::Envelope::setValue(points[point].y);
                        if(loop.start != loop.end){
                            setTarget(points[point+1], points[point].x);
                        }
                    }else if((point+1) < points.size()){
                        if(time >= points[point+1].x){ // reached target point
                            point++;
                            setValue(points[point].y); // make sure exact value is set
                            
                            if((point+1) < points.size()) // new target point?
                                setTarget(points[point+1], points[point].x);
                        }
                    }else{
                        stage = ENV_OFF;
                    }
                }
            }else if(stage == ENV_RELEASE){
                if(amplitude == 0.0)
                    stage = ENV_OFF;
            }
            
            return amplitude;
        }
        
        const Point& operator[](int point) const {
            return points[point];
        }
        
    private:
        std::vector<Point> points;
        Loop loop;
        
        int point;
        float time, timeInc;
        STAGE stage;
    };
    
    
    typedef float (*Function)(float x);
    
    class Wavetable : public stk::FileLoop
    {
    public:
        Wavetable() : fBaseFrequency(261.626) {}
        
        void openResource(std::string filename){
            CFBundleRef plugBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.UWE.TestEffectAU"));
            CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(plugBundle);
            char path[PATH_MAX];
            CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX);
            CFRelease(resourcesURL);
            
            openFile(std::string(path) + "/" + filename);
            normalize();
        }
        
        void setFrequency( float frequency ) {
            setRate( frequency / fBaseFrequency);
        };
        
        void setBaseFrequency( float frequency ) {
            fBaseFrequency = frequency;
        }
        
        virtual float tick(){
            interpolate_ = true;
            return FileLoop::tick();
        }
        
        virtual float tick(float phase){
            interpolate_ = true;
            time_ = phase * file_.fileSize();
            return FileLoop::tick();
        }
        
        Wavetable& operator=(const Wavetable& in){
            // Call close() in case another file is already open.
            this->closeFile();
            
            // Attempt to open the file ... an error might be thrown here.
            file_ = in.file_;
            
            // Determine whether chunking or not.
            if ( in.file_.fileSize() > chunkThreshold_ ) {
                chunking_ = in.chunking_;
                chunkPointer_ = in.chunkPointer_;
                data_.resize( in.chunkSize_ + 1, in.file_.channels() );
                normalizing_ = in.normalizing_;
            }
            else {
                chunking_ = false;
                data_.resize( in.file_.fileSize() + 1, in.file_.channels() );
            }
            
            // Load all or part of the data.
            data_ = in.data_;
            
            if ( chunking_ ) { // If chunking, save the first sample frame for later.
                firstFrame_.resize( 1, data_.channels() );
                for ( unsigned int i=0; i<data_.channels(); i++ )
                    firstFrame_[i] = data_[i];
            }
            else {  // If not chunking, copy the first sample frame to the last.
                for ( unsigned int i=0; i<data_.channels(); i++ )
                    data_( data_.frames() - 1, i ) = data_[i];
            }
            
            // Resize our lastOutputs container.
            lastFrame_.resize( 1, in.file_.channels() );
            
            // Set default rate based on file sampling rate.
            
            this->setRate( in.data_.dataRate() / Stk::sampleRate() );
            this->reset();
            
            fBaseFrequency = in.fBaseFrequency;
            
            return *this;
        }
        
        Wavetable& operator=(const Envelope& envelope){
            openResource("Silence.wav");
            
            int iEnvLength = envelope.getLength();
            if(!iEnvLength)
                return *this;
            
            int waveLength = file_.fileSize();
            
            Envelope new_envelope = envelope;
            new_envelope.resize(waveLength);
            
            float* pSample = &data_[0];
            int nbChannels = data_.channels();
            
            for(int x=0; x<waveLength; x++){
                float sample = x == 0 ? new_envelope.lastOut() : new_envelope.tick();
                for(int c=0; c<nbChannels; c++)
                    *pSample++ = sample;
            }
            
            setBaseFrequency(getSampleRate()/waveLength);
            
            return *this;
        }
        
        void distort(Function function){
            float waveLength = file_.fileSize();
            float* pSample = &data_[0];
            int nbChannels = data_.channels();
            
            for(int x=0; x<waveLength; x++){
                float sample = (function)(*pSample);
                for(int c=0; c<nbChannels; c++)
                    *pSample++ = sample;
            }
        }
        
        void setOffset(float samples){
            time_ = samples;
            
            if ( time_ < 0.0 ) time_ = 0.0;
            if ( time_ > file_.fileSize() - 1.0 ) {
                time_ = file_.fileSize() - 1.0;
                for ( unsigned int i=0; i<lastFrame_.size(); i++ ) lastFrame_[i] = 0.0;
                finished_ = true;
            }
        }
        
        void generate(Function function){
            openResource("Silence.wav");
            float waveLength = file_.fileSize();
            
            float* pSample = &data_[0];
            int nbChannels = data_.channels();
            
            for(int x=0; x<waveLength; x++){
                float sample = (function)(((float)x)/waveLength);
                for(int c=0; c<nbChannels; c++)
                    *pSample++ = sample;
            }
            
            setBaseFrequency(getSampleRate()/waveLength);
        }
        
    private:
        float fBaseFrequency;
    };
    
    
    class Buffer : public Wavetable
    {
    public:
        float tick(){
            return stk::FileWvIn::tick();
        }
        
        float getDuration() const {
            return getSize() / getSampleRate();
        }
    };
    
} // namespace APDI

//==============================================================================
/** A simple synth sound ... */
class SimpleSound : public SynthesiserSound
{
public:
    SimpleSound() {}
    
    bool appliesToNote (const int /*midiNoteNumber*/)           { return true; }
    bool appliesToChannel (const int /*midiChannel*/)           { return true; }
};

class MySynth;

//==============================================================================
/** An (abstract) class for an STK-based synthesized voice (can be hidden from students) */
class Voice  : public SynthesiserVoice
{
public:
    Voice()
    :   tailOff (0.0), bSilent (true), pParameters(NULL), buffer(2,512), pSynth(NULL)
    {
    }
    
    virtual ~Voice(){}
    
    void setSynthesiser(MySynth* synth) { pSynth = synth; }
    MySynth* getSynthesiser() { return pSynth; }
    
    void setParameters(IPluginParameters* parameters){ pParameters = parameters; }
    float getParameter(int index){ return pParameters->getParameter(index); }
    void setParameter(int index, float value){ pParameters->setParameter(index, value); }
    
    virtual bool canPlaySound (SynthesiserSound* sound)
    {
        return dynamic_cast <SimpleSound*> (sound) != 0;
    }
    
    virtual void startNote (const int midiNoteNumber, const float velocity,
                            SynthesiserSound* /*sound*/, const int /*currentPitchWheelPosition*/)
    {
        level = 1.0;//velocity * 0.5;
        tailOff = 0.0;
        
        onStartNote(midiNoteNumber, velocity);
        bSilent = false;
    }
    
    virtual void onStartNote(const int midiNoteNumber, const float velocity) = 0;
    
    virtual void stopNote (const bool allowTailOff)
    {
        if(!onStopNote()){
            // do not kill note
        }else if (allowTailOff){
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.
            
            if (tailOff == 0.0) // we only need to begin a tail-off if it's not already doing so - the
                // stopNote method could be called more than once.
                tailOff = 1.0;
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..
            clearCurrentNote();
            bSilent = true;
        }
        
        
    }
    
    virtual bool onStopNote() = 0;
    
    virtual void onPitchWheel(const int value) {}
    virtual void pitchWheelMoved (const int newValue)
    {    onPitchWheel(newValue);    }
    
    virtual void onControlChange(const int controller, const int value) {}
    virtual void controllerMoved (const int controllerNumber, const int newValue)
    {   onControlChange(controllerNumber, newValue);    }
    
    virtual void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
    {
        const int numChannels = outputBuffer.getNumChannels() < 2 ? 2 : outputBuffer.getNumChannels();
        const int bufferSize = numSamples;
        
        if (!bSilent)
        {
            buffer.setSize (numChannels, bufferSize, false, false, true);
            
            if(!process (buffer.getArrayOfChannels(), numChannels, numSamples))
            {
                clearCurrentNote();
                tailOff = 0.0f;
                bSilent = true;
            }
            
            if (tailOff > 0)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    for(int c=0; c<numChannels; c++)
                        *buffer.getSampleData (c, i) *= level * tailOff;
                    
                    tailOff *= 0.99;
                    
                    if (!bSilent && tailOff <= 0.005)
                    {
                        clearCurrentNote();
                        tailOff = 0.0f;
                        bSilent = true;
                    }
                }
            }
            else
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    for(int c=0; c<numChannels; c++)
                        *buffer.getSampleData (c, i) *= level;
                }
            }
            
            for(int c=0; c< outputBuffer.getNumChannels(); c++)
                outputBuffer.addFrom(c, startSample, buffer, c, 0, bufferSize);
        }
    }
    virtual bool process (float** outputBuffer, int numChannels, int numSamples) = 0;
    
protected:
    double level, tailOff;
    
private:
    bool bSilent;
    IPluginParameters *pParameters;
    AudioSampleBuffer buffer;
    
    MySynth* pSynth;
};



#endif
