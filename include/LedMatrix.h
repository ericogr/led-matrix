#ifndef __LEDMATRIX_H__
#define __LEDMATRIX_H__

#include "mgos_spi.h"

#define TEXT_ALIGN_LEFT          0 // Text is aligned to left side of the display
#define TEXT_ALIGN_LEFT_END      1 // Beginning of text is just outside the right end of the display
#define TEXT_ALIGN_RIGHT         2 // End of text is aligned to the right of the display
#define TEXT_ALIGN_RIGHT_END     3 // End of text is just outside the left side of the display

namespace lm {
  class LedMatrix {
  public:

    LedMatrix() {
    }

    /**
    * Initializes the SPI interface with default values
    */
    void init();

    /**
    * Initializes the SPI interface
    * spi mongoose-os spi
    */
    void init(mgos_spi *spi);

    /**
    * Initializes the SPI interface
    * numberOfDisplays: number of connected devices
    * slaveSelectPin: CS (or SS) pin connected to your device
    */
    void init(uint8_t numberOfDisplays, uint8_t slaveSelectPin);

    /**
    * Initializes the SPI interface
    * spi mongoose-os spi
    * numberOfDisplays: number of connected devices
    * slaveSelectPin: CS (or SS) pin connected to your device
    */
    void init(mgos_spi *spi, uint8_t numberOfDisplays, uint8_t slaveSelectPin);

    /**
    * Sets the intensity on all devices.
    * intensity: 0-15
    */
    void setIntensity(uint8_t intensity);

    /**
    * Sets the width in pixels for one character.
    * Default is 7.
    */
    void setCharWidth(uint8_t charWidth);

    /**
    * Sets the text alignment.
    * Default is TEXT_ALIGN_LEFT_END.
    *
    */
    void setTextAlignment(uint8_t textAlignment);

    /**
    * Send a uint8_t to a specific device.
    */
    void sendByte (const uint8_t device, const uint8_t reg, const uint8_t data);

    /**
    * Send a uint8_t to all devices (convenience method).
    */
    void sendByte (const uint8_t reg, const uint8_t data);

    /**
    * Turn on pixel at position (x,y).
    */
    void setPixel(uint8_t x, uint8_t y);

    /**
    * Clear the frame buffer.
    */
    void clear();

    /**
    * Draw the currently set text.
    */
    void drawText();

    /**
    * Start draw the currently set text
    */
    void startAnimatedText(uint32_t interval);

    /**
    * Stop draw the currently set text
    */
    void stopAnimatedText();

    /**
    * Set the current text.
    */
    void setText(const char * text);

    /**
    * Set the text that will replace the current text after a complete scroll
    * cycle.
    */
    void setNextText(const char * nextText);

    /**
    * Set a specific column with a uint8_t value to the framebuffer.
    */
    void setColumn(int column, uint8_t value);

    /**
    * Writes the framebuffer to the displays.
    */
    void commit();

    /**
    * Scroll the text to the right.
    */
    void scrollTextRight();

    /**
    * Scroll the text to the left.
    */
    void scrollTextLeft();

    /**
    * Oscilate the text between the two limits.
    */
    void oscillateText();

    /**
    * Must be enabled, depending of your display wire arrange
    * @param enabled enable display rotation
    */
    void setRotation(bool enabled);

  private:
    mgos_spi *m_SPI;

    uint8_t *m_Columns;
    uint8_t *m_RotatedCols;
    uint8_t m_SPIRegister[8];
    uint8_t m_SPIData[8];
    char *m_Text = new char[1]();
    char *m_MyNextText = new char[1]();
    int m_TextOffset {0};
    int m_TextAlignmentOffset {TEXT_ALIGN_LEFT_END};
    int m_Increment {-1};
    uint8_t m_MyNumberOfDevices {0};
    uint8_t m_MySlaveSelectPin {0};
    uint8_t m_MyCharWidth {7};
    uint8_t m_TextAlignment {1};
    bool m_RotationIsEnabled {false};
    uint8_t m_SPINr;
    uint8_t m_SCK;
    uint8_t m_MOSI;
    mgos_timer_id m_TimerId;

    static void animatedTextLoop(void *args);
    void calculateTextAlignmentOffset();
    void rotateLeft();
  };
}

#endif
