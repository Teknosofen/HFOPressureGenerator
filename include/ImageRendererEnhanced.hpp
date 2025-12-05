#ifndef IMAGE_RENDERER_ENHANCED_H
#define IMAGE_RENDERER_ENHANCED_H

#include <TFT_eSPI.h>

// Colours for UI
#define TFT_LOGOBACKGROUND   0x85BA
#define TFT_LOGOBLUE         0x5497
#define TFT_DARKERBLUE       0x3A97
#define TFT_DEEPBLUE         0x1A6F
#define TFT_SLATEBLUE        0x2B4F
#define TFT_MIDNIGHTBLUE     0x1028
#define TFT_REDDISH_TINT     0xA4B2   
#define TFT_GREENISH_TINT    0x5DAD

struct DisplayPos {
    int x;
    int y;
    
    DisplayPos(int xPos = 0, int yPos = 0) : x(xPos), y(yPos) {}
};

class ImageRendererEnhanced {
public:
    ImageRendererEnhanced();
    
    void init();
    void setPositions(DisplayPos mode, DisplayPos value, DisplayPos dir, DisplayPos adc);
    
    // Original single motor display
    void updateMode(const char* modeText);
    void updateValue(const char* valueLabel, int value);
    void updateDirection(const char* dirText);
    void updateADC(int adcCounts);
    void drawFullUI(const char* modeText, const char* dirText,
                    const char* valueLabel, int value, int adcCounts);
    
    // Enhanced dual motor display
    void drawDualMotorUI(const char* escMode, const char* escDir, int escThrottle,
                        long stepperPos, long stepperTarget, float stepperSpeed, 
                        bool stepperRunning, bool stepperEnabled);
    
    void drawHeader(const char* title, const char* version);
    void drawStepperStatus(int x, int y, long pos, long target, float speed, 
                          bool running, bool enabled);
    void drawESCStatus(int x, int y, const char* mode, const char* dir, 
                      int throttle, int adc);

private:
    TFT_eSPI tft;
    
    DisplayPos modePos;
    DisplayPos valuePos;
    DisplayPos dirPos;
    DisplayPos adcPos;
    
    // Cache for selective updates
    String oldModeText;
    String oldDirText;
    String oldValueLabel;
    int oldValue;
    int oldAdcCounts;
    
    // Enhanced cache for dual motor
    long oldStepperPos;
    long oldStepperTarget;
    float oldStepperSpeed;
    bool oldStepperRunning;
    bool oldStepperEnabled;
    
    void eraseAndDraw(int x, int y, const char* oldText, const char* newText);
};

#endif // IMAGE_RENDERER_ENHANCED_H
