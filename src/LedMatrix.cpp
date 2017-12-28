#include "cp437font.h"
#include "LedMatrix.h"
#include "mgos_gpio.h"
#include "mgos.h"

#ifndef ARDUINO
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#endif
#define modb(a, b) ((abs(a) % b) * (a < 0 ? -1 : 1))

#define MAX7219_SPI_FREQ         10000000
#define MAX7219_REG_NOOP         0x0
#define MAX7219_REG_DIGIT0       0x1
#define MAX7219_REG_DIGIT1       0x2
#define MAX7219_REG_DIGIT2       0x3
#define MAX7219_REG_DIGIT3       0x4
#define MAX7219_REG_DIGIT4       0x5
#define MAX7219_REG_DIGIT5       0x6
#define MAX7219_REG_DIGIT6       0x7
#define MAX7219_REG_DIGIT7       0x8
#define MAX7219_REG_DECODEMODE   0x9
#define MAX7219_REG_INTENSITY    0xA
#define MAX7219_REG_SCANLIMIT    0xB
#define MAX7219_REG_SHUTDOWN     0xC
#define MAX7219_REG_DISPLAYTEST  0xF

using namespace lm;

void LedMatrix::init() {
    init(mgos_spi_get_global(), mgos_sys_config_get_ledm_number_of_displays(), mgos_sys_config_get_ledm_slave_select_pin());
}

void LedMatrix::init(mgos_spi *spi) {
    init(spi, mgos_sys_config_get_ledm_number_of_displays(), mgos_sys_config_get_ledm_slave_select_pin());
}

void LedMatrix::init(uint8_t numberOfDisplays, uint8_t slaveSelectPin) {
    init(mgos_spi_get_global(), numberOfDisplays, slaveSelectPin);
}

void LedMatrix::init(mgos_spi *spi, uint8_t numberOfDisplays, uint8_t slaveSelectPin) {
    LOG(LL_INFO, ("LedMatrix displays=%d, Slave Pin=%d", numberOfDisplays, slaveSelectPin));

    m_SPI = spi;
    m_MyNumberOfDevices = numberOfDisplays;
    m_MySlaveSelectPin = slaveSelectPin;

    if (mgos_gpio_set_mode(m_MySlaveSelectPin, MGOS_GPIO_MODE_OUTPUT)) {
        m_Columns = new uint8_t[numberOfDisplays * 8];
        m_RotatedCols = new uint8_t[numberOfDisplays * 8];

        for (uint8_t device = 0; device < m_MyNumberOfDevices; device++) {
            sendByte(device, MAX7219_REG_SCANLIMIT, 7);   // show all 8 digits
            sendByte(device, MAX7219_REG_DECODEMODE, 0);  // using an led matrix (not digits)
            sendByte(device, MAX7219_REG_DISPLAYTEST, 0); // no display test
            sendByte(device, MAX7219_REG_INTENSITY, 0);   // character intensity: range: 0 to 15
            sendByte(device, MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
        }
    }
    else {
        LOG(LL_ERROR, ("Invalid GPIO for slaveSelectPin: %d", m_MySlaveSelectPin));
    }
}

void LedMatrix::sendByte(const uint8_t device, const uint8_t reg, const uint8_t data) {
    uint8_t m_SPIRegister[8];
    uint8_t m_SPIData[8];

    for (uint8_t i = 0; i < m_MyNumberOfDevices; i++) {
        m_SPIData[i] = (uint8_t)0;
        m_SPIRegister[i] = (uint8_t)0;
    }

    // put our device data into the array
    m_SPIRegister[device] = reg;
    m_SPIData[device] = data;

    // now shift out the data
    mgos_spi_txn txn;
    uint8_t tx_data[2];
    txn.cs = -1;
    txn.mode = 0;
    txn.freq = MAX7219_SPI_FREQ;
    txn.hd.tx_len = sizeof(tx_data);
    txn.hd.tx_data = tx_data;
    txn.hd.dummy_len = 0;
    txn.hd.rx_len = 0;

    // enable the line
    mgos_gpio_write(m_MySlaveSelectPin, false);
    for (uint8_t i = 0; i < m_MyNumberOfDevices; i++) {
        tx_data[0] = m_SPIRegister[i];
        tx_data[1] = m_SPIData[i];

        if (!mgos_spi_run_txn(m_SPI, false, &txn)) {
            LOG(LL_ERROR, ("SPI transaction failed"));
        }
    }
    mgos_gpio_write(m_MySlaveSelectPin, true);
}

void LedMatrix::sendByte (const uint8_t reg, const uint8_t data) {
    for (uint8_t device = 0; device < m_MyNumberOfDevices; device++) {
        sendByte(device, reg, data);
    }
}

void LedMatrix::setIntensity(const uint8_t intensity) {
    sendByte(MAX7219_REG_INTENSITY, intensity);
}

void LedMatrix::setCharWidth(uint8_t charWidth) {
    m_MyCharWidth = charWidth;
}

void LedMatrix::setTextAlignment(uint8_t textAlignment) {
    m_TextAlignment = textAlignment;
    calculateTextAlignmentOffset();
}

void LedMatrix::calculateTextAlignmentOffset() {
    switch(m_TextAlignment) {
        case TEXT_ALIGN_LEFT:
            m_TextAlignmentOffset = 0;
            break;
        case TEXT_ALIGN_LEFT_END:
            m_TextAlignmentOffset = m_MyNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT:
            m_TextAlignmentOffset = strlen(m_Text) * m_MyCharWidth - m_MyNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT_END:
            m_TextAlignmentOffset = - strlen(m_Text) * m_MyCharWidth;
            break;
    }
}

void LedMatrix::clear() {
    for (uint8_t col = 0; col < m_MyNumberOfDevices * 8; col++) {
        m_Columns[col] = 0;
    }
}

void LedMatrix::commit() {
    if (m_RotationIsEnabled) {
        rotateLeft();
    }

    for (uint8_t col = 0; col < m_MyNumberOfDevices * 8; col++) {
        sendByte(col / 8, col % 8 + 1, m_Columns[col]);
    }
}

void LedMatrix::setText(const char *text) {
    delete[] m_Text;
    m_Text = new char[strlen(text) + 1];
    strcpy(m_Text, text);
    m_TextOffset = 0;
    calculateTextAlignmentOffset();
}

void LedMatrix::setNextText(const char *nextText) {
    delete[] m_MyNextText;
    m_MyNextText = new char[strlen(nextText) + 1];
    strcpy(m_MyNextText, nextText);
}

void LedMatrix::scrollTextRight() {
    m_TextOffset = modb((m_TextOffset + 1), ((strlen(m_Text) + m_MyNumberOfDevices + 1) * m_MyCharWidth));
}


void LedMatrix::scrollTextLeft() {
    m_TextOffset = modb((m_TextOffset - 1), (strlen(m_Text) * m_MyCharWidth + m_MyNumberOfDevices * 8));

    if (m_TextOffset == 0 && strlen(m_MyNextText) > 0) {
        setText(m_MyNextText);
        setNextText("");
        calculateTextAlignmentOffset();
    }
}

void LedMatrix::oscillateText() {
    int maxColumns = strlen(m_Text) * m_MyCharWidth;
    int maxDisplayColumns = m_MyNumberOfDevices * 8;

    if (maxDisplayColumns > maxColumns) {
        return;
    }

    if (m_TextOffset - maxDisplayColumns == -maxColumns) {
        m_Increment = 1;
    }

    if (m_TextOffset == 0) {
        m_Increment = -1;
    }

    m_TextOffset += m_Increment;
}

void LedMatrix::startAnimatedText(uint32_t interval) {
    mgos_clear_timer(m_TimerId);
    m_TimerId = mgos_set_timer(interval, true, animatedTextLoop, this);
}

void LedMatrix::stopAnimatedText() {
    mgos_clear_timer(m_TimerId);
}

void LedMatrix::animatedTextLoop(void *args) {
    LedMatrix *me = static_cast<LedMatrix*>(args);

    me->clear();
    me->scrollTextLeft();
    me->drawText();
    me->commit();
}

void LedMatrix::drawText() {
    int letter;
    int position = 0;
    for (uint16_t i = 0; i < strlen(m_Text); i++) {
        letter = m_Text[i];
        for (uint8_t col = 0; col < 8; col++) {
            position = i * m_MyCharWidth + col + m_TextOffset + m_TextAlignmentOffset;

            if (position >= 0 && position < m_MyNumberOfDevices * 8) {
                setColumn(position, cp437_font[letter][col]);
            }
        }
    }
}

void LedMatrix::setColumn(int column, uint8_t value) {
    if (column < 0 || column >= m_MyNumberOfDevices * 8) {
        return;
    }
    m_Columns[column] = value;
}

void LedMatrix::setPixel(uint8_t x, uint8_t y) {
    bitWrite(m_Columns[x], y, true);
}

void LedMatrix::rotateLeft() {
    for (uint8_t deviceNum = 0; deviceNum < m_MyNumberOfDevices; deviceNum++) {
        for (uint8_t posY = 0; posY < 8; posY++) {
            for (uint8_t posX = 0; posX < 8; posX++) {
                bitWrite(m_RotatedCols[8 * (deviceNum) + posY], posX, bitRead(m_Columns[8 * (deviceNum) + 7 - posX], posY));
            }
        }
    }
    memcpy(m_Columns, m_RotatedCols, m_MyNumberOfDevices * 8);
}

void LedMatrix::setRotation(bool enabled) {
    m_RotationIsEnabled = enabled;
}
