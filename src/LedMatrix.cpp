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

    _SPI = spi;
    _MyNumberOfDevices = numberOfDisplays;
    _MySlaveSelectPin = slaveSelectPin;

    if (mgos_gpio_set_mode(_MySlaveSelectPin, MGOS_GPIO_MODE_OUTPUT)) {
        _Columns = new uint8_t[numberOfDisplays * 8];
        _RotatedCols = new uint8_t[numberOfDisplays * 8];

        for (uint8_t device = 0; device < _MyNumberOfDevices; device++) {
            sendByte(device, MAX7219_REG_SCANLIMIT, 7);   // show all 8 digits
            sendByte(device, MAX7219_REG_DECODEMODE, 0);  // using an led matrix (not digits)
            sendByte(device, MAX7219_REG_DISPLAYTEST, 0); // no display test
            sendByte(device, MAX7219_REG_INTENSITY, 0);   // character intensity: range: 0 to 15
            sendByte(device, MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
        }
    }
    else {
        LOG(LL_ERROR, ("Invalid GPIO for slaveSelectPin: %d", _MySlaveSelectPin));
    }
}

void LedMatrix::sendByte(const uint8_t device, const uint8_t reg, const uint8_t data) {
    uint8_t _SPIRegister[8];
    uint8_t _SPIData[8];

    for (uint8_t i = 0; i < _MyNumberOfDevices; i++) {
        _SPIData[i] = (uint8_t)0;
        _SPIRegister[i] = (uint8_t)0;
    }

    // put our device data into the array
    _SPIRegister[device] = reg;
    _SPIData[device] = data;

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
    mgos_gpio_write(_MySlaveSelectPin, false);
    for (uint8_t i = 0; i < _MyNumberOfDevices; i++) {
        tx_data[0] = _SPIRegister[i];
        tx_data[1] = _SPIData[i];

        if (!mgos_spi_run_txn(_SPI, false, &txn)) {
            LOG(LL_ERROR, ("SPI transaction failed"));
        }
    }
    mgos_gpio_write(_MySlaveSelectPin, true);
}

void LedMatrix::sendByte (const uint8_t reg, const uint8_t data) {
    for (uint8_t device = 0; device < _MyNumberOfDevices; device++) {
        sendByte(device, reg, data);
    }
}

void LedMatrix::setIntensity(const uint8_t intensity) {
    sendByte(MAX7219_REG_INTENSITY, intensity);
}

void LedMatrix::setCharWidth(uint8_t charWidth) {
    _MyCharWidth = charWidth;
}

void LedMatrix::setTextAlignment(uint8_t textAlignment) {
    _TextAlignment = textAlignment;
    calculateTextAlignmentOffset();
}

void LedMatrix::calculateTextAlignmentOffset() {
    switch(_TextAlignment) {
        case TEXT_ALIGN_LEFT:
            _TextAlignmentOffset = 0;
            break;
        case TEXT_ALIGN_LEFT_END:
            _TextAlignmentOffset = _MyNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT:
            _TextAlignmentOffset = strlen(_Text) * _MyCharWidth - _MyNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT_END:
            _TextAlignmentOffset = - strlen(_Text) * _MyCharWidth;
            break;
    }
}

void LedMatrix::clear() {
    for (uint8_t col = 0; col < _MyNumberOfDevices * 8; col++) {
        _Columns[col] = 0;
    }
}

void LedMatrix::commit() {
    if (_RotationIsEnabled) {
        rotateLeft();
    }

    for (uint8_t col = 0; col < _MyNumberOfDevices * 8; col++) {
        sendByte(col / 8, col % 8 + 1, _Columns[col]);
    }
}

void LedMatrix::setText(const char *text) {
    _Text = text;
    _TextOffset = 0;
    calculateTextAlignmentOffset();
}

void LedMatrix::setNextText(const char *nextText) {
    _MyNextText = nextText;
}

void LedMatrix::scrollTextRight() {
    _TextOffset = modb((_TextOffset + 1), ((strlen(_Text) + _MyNumberOfDevices + 1) * _MyCharWidth));
}


void LedMatrix::scrollTextLeft() {
    _TextOffset = modb((_TextOffset - 1), (strlen(_Text) * _MyCharWidth + _MyNumberOfDevices * 8));

    if (_TextOffset == 0 && strlen(_MyNextText) > 0) {
        _Text = _MyNextText;
        _MyNextText = "";
        calculateTextAlignmentOffset();
    }
}

void LedMatrix::oscillateText() {
    int maxColumns = strlen(_Text) * _MyCharWidth;
    int maxDisplayColumns = _MyNumberOfDevices * 8;

    if (maxDisplayColumns > maxColumns) {
        return;
    }

    if (_TextOffset - maxDisplayColumns == -maxColumns) {
        _Increment = 1;
    }

    if (_TextOffset == 0) {
        _Increment = -1;
    }

    _TextOffset += _Increment;
}

void LedMatrix::startAnimatedText(uint32_t interval) {
    mgos_clear_timer(_TimerId);
    _TimerId = mgos_set_timer(interval, true, animatedTextLoop, this);
}

void LedMatrix::stopAnimatedText() {
    mgos_clear_timer(_TimerId);
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
    for (uint16_t i = 0; i < strlen(_Text); i++) {
        letter = _Text[i];
        for (uint8_t col = 0; col < 8; col++) {
            position = i * _MyCharWidth + col + _TextOffset + _TextAlignmentOffset;

            if (position >= 0 && position < _MyNumberOfDevices * 8) {
                setColumn(position, cp437_font[letter][col]);
            }
        }
    }
}

void LedMatrix::setColumn(int column, uint8_t value) {
    if (column < 0 || column >= _MyNumberOfDevices * 8) {
        return;
    }
    _Columns[column] = value;
}

void LedMatrix::setPixel(uint8_t x, uint8_t y) {
    bitWrite(_Columns[x], y, true);
}

void LedMatrix::rotateLeft() {
    for (uint8_t deviceNum = 0; deviceNum < _MyNumberOfDevices; deviceNum++) {
        for (uint8_t posY = 0; posY < 8; posY++) {
            for (uint8_t posX = 0; posX < 8; posX++) {
                bitWrite(_RotatedCols[8 * (deviceNum) + posY], posX, bitRead(_Columns[8 * (deviceNum) + 7 - posX], posY));
            }
        }
    }
    memcpy(_Columns, _RotatedCols, _MyNumberOfDevices * 8);
}

void LedMatrix::setRotation(bool enabled) {
    _RotationIsEnabled = enabled;
}
