//
//  EffectEditor.h
//  TestEffectAU
//
//  Used to specify the contents and layout of the TestEffectAU User Interface (UI).
//

#ifndef __EffectEditor_h__
#define __EffectEditor_h__

enum CONTROL_TYPE
{
    ROTARY, // rotary knob (pot)
    BUTTON, // push button (trigger)
    TOGGLE, // on/off switch (toggle)
    SLIDER, // linear slider (fader)
    MENU,   // drop-down list (menu)
    METER,  // level meter (read-only: use setParameter() to set value)
};

typedef Rectangle<int> Bounds;

struct Control
{
    String name;            // name for control label / saved parameter
    int parameter;          // parameter index associated with control
    CONTROL_TYPE type;      // control type (see above)
    
    // ROTARY and SLIDER only:
    float min;              // minimum slider value (e.g. 0.0)
    float max;              // maximum slider value (e.g. 1.0)
    
    float initial;          // initial value for slider (e.g. 0.0)
    
    Bounds size;            // position (x,y) and size (height, width) of the control (use AUTO_SIZE for automatic layout)
    
    const char* const options[8]; // text options for menus and group buttons
};

const Bounds AUTO_SIZE = Bounds(-1,-1,-1,-1); // used to trigger automatic layout
enum { kParam0, kParam1, kParam2, kParam3, kParam4, kParam5, kParam6, kParam7, kParam8, kParam9, kParam10, kParam11, kParam12, kParam13, kParam14, kParam15, kParam16, kParam17};

//==========================================================================
// UI_CONTROLS - Use this array to completely specify your UI
// - tells the system what controls you want
// - add or remove controls by adding or taking away from the list
// - each control is linked to the specified parameter name and identifier
// - controls can be of different types - rotary, button, toggle, slider (see above)
// - for rotary and linear sliders, you can set the range of values
// - by default, the controls are laid out in a grid, but you can also move and size them manually
//   i.e. replace AUTO_SIZE with Bounds(50,50,100,100) to place a 100x100 control at (50,50)

const Control UI_CONTROLS[] = {
    //      name,       parameter,  type,   min, max, initial,size          // options (for MENU)
    {   "Threshold (dB)",  kParam0,    ROTARY, 0.0, 1.0, 1.0,    Bounds (15,25,55,55)   },
    {   "Ratio (x:1)",  kParam1,    ROTARY, 1.0, 16.0, 1.0,    Bounds (85,25,55,55)   },
    {   "Makeup Gain",  kParam2,    ROTARY, 1.0, 5.0, 1.0,    Bounds (155,25,55,55)   },
    {   "Detect Mode",  kParam3,    MENU, 0.0, 1.0, 0.0,    Bounds (230,25,60,20), "Peak", "RMS"   },
    {   "Peak",  kParam4,    METER, 0.0, 1.0, 0.0,    Bounds (310,30,20,200)   },
    
    {   "RMS",  kParam5,    METER, 0.0, 1.0, 0.0,    Bounds (340,30,20,200)   },
    {"Comp",  kParam6,    METER, 0.0, 1.0, 0.0,    Bounds (370,30,20,200)  },
    {   "Threshold (dB)",  kParam7,    ROTARY, 0.0, 1.0, 1.0,    Bounds (15,110,55,55)   },
    {   "Ratio (x:1)",  kParam8,    ROTARY, 1.0, 16.0, 1.0,    Bounds (85,110,55,55)   },
    {   "Makeup Gain",  kParam9,    ROTARY, 1.0, 5.0, 1.0,    Bounds (155,110,55,55)   },
    {   "Attack",  kParam10,    ROTARY, 0.0, 0.09999, 0.0,    Bounds (240,105,50,45)   },
    {   "Release",  kParam11,    ROTARY, 0.099, 0.09999, 0.099,    Bounds (240,175,50,45)   },
    {   "Centre Frequency",  kParam12,    ROTARY, 20.0, 5000.0, 1000.0,    Bounds (155,190,50,45)   },
    {   "Mono",  kParam13,    TOGGLE, 0.0, 1.0, 0.0,    Bounds (230,55,55,20)   },
    {   "Knee",  kParam14,    ROTARY, 1.0, 3.0, 1.0,    Bounds (85,190,50,45)   },
    {   "Lookahead (0-200ms)",  kParam15,    ROTARY, 0.0, 0.2, 0.0,    Bounds (20,190,50,45)   },
    
};

const int kNumberOfControls = sizeof(UI_CONTROLS) / sizeof(Control);
const int kNumberOfParameters = kNumberOfControls;

struct Preset
{
    String name;                        // name of preset
    float value[kNumberOfParameters];   // parameter values
};

//==========================================================================
// UI_PRESETS - Use this array to provide users with pluygin factory presets
// - give your preset a informative name
// - then specify each and every parameter value (as a float), corresponding to your UI_CONTROL array
// - if you change the UI_CONTROLS array, remember to update you UI_PRESETS
// - for ROTARY and SLIDER, use the values/positions
// - for TOGGLE, use 0 for OFF and 1 for ON
// - for MENU, use the number of the selected item (0,1,2,etc.)
// - for LEVEL, the value is read-only; simply insert 0

const Preset UI_PRESETS[] = {
    { "Bright Guitar", 0.45650, 3.81250, 1.62853, 0, 0, 0, 0, 0.59224, 1.65502, 1.12500, 0.03670, 0.09975, 3956.61792, 0, 2.08256, 0},
    { "Drums Sparkle", 0.39912, 16.00000, 2.02754, 0, 0, 0, 0, 0.11612, 16.00000, 1.90708, 0.04648, 0.09996, 2411.25439, 1.00000, 0.00000},
    { "Add Body", 0.41250, 9.40386, 1.12500, 0, 0, 0, 0, 0.41945, 10.54632, 2.87853, 0.01321, 0.09992, 1022.75934, 3.00000, 0.00320},
};

#endif
