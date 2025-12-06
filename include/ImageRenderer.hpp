#ifndef IMAGE_RENDERER_HPP
#define IMAGE_RENDERER_HPP

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
#define TFT_DARK_RED         0x0330
#define TFT_DARK_GREEN       0x4000
#define TFT_BRIGHT_GREEN     0x07E0
#define TFT_BRIGHT_RED       0xF800
#define TFT_BRIGHT_YELLOW    0xFFE0
#define TFT_BRIGHT_BLUE      0x001F

class ImageRenderer {
public:
    ImageRenderer();
    
    void init();
    void drawHeader(const char* title, const char* version);
    
    // Draw complete UI for ESC + Stepper
    void drawDualMotorUI(int escThrottle, int escADC, 
                        long stepperPos, long stepperTarget, 
                        float stepperSpeed, bool stepperRunning, bool stepperEnabled);

private:
    TFT_eSPI tft;
    
    // Cache for ESC
    int oldESCThrottle;
    int oldESCADC;
    
    // Cache for Stepper
    long oldStepperPos;
    long oldStepperTarget;
    float oldStepperSpeed;
    bool oldStepperRunning;
    bool oldStepperEnabled;
    
    void drawESCStatus(int x, int y, int throttle, int adc);
    void drawStepperStatus(int x, int y, long pos, long target, 
                          float speed, bool running, bool enabled);
};

#endif // IMAGE_RENDERER_HPP
