#ifndef IMAGE_RENDERER_H
#define IMAGE_RENDERER_H

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

class ImageRenderer {
public:
    ImageRenderer();
    
    void init();
    void setPositions(DisplayPos mode, DisplayPos value, DisplayPos dir, DisplayPos adc);
    
    void updateMode(const char* modeText);
    void updateValue(const char* valueLabel, int value);
    void updateDirection(const char* dirText);
    void updateADC(int adcCounts);
    
    void drawFullUI(const char* modeText, const char* dirText,
                    const char* valueLabel, int value, int adcCounts);
    void drawHeader(const char* hfoLabel, const char* hfoVer);

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
    
    void eraseAndDraw(int x, int y, const char* oldText, const char* newText);
};

#endif // IMAGE_RENDERER_H
