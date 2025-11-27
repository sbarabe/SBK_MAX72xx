/**
 * @file SBK_MAX72xxHard.cpp
 * @brief Implementation of the SBK_MAX72xxHard class for controlling MAX7219/MAX7221 LED drivers.
 *
 * Part of the SBK BarDrive Arduino Library
 * https://github.com/yourusername/SBK_BarDrive
 *
 * This file contains the method implementations for managing multiple daisy-chained
 * MAX7219/MAX7221 chips using software SPI. Enables per-device LED control and display management.
 *
 * @author
 * Samuel Barabé (Smart Builds & Kits)
 *
 * @version 2.0.4
 *
 * @license MIT
 *
 * Copyright (c) 2025 Samuel Barabé
 */

#include "SBK_MAX72xxHard.h"

// MAX7219/MAX7221 Opcodes
#define OP_NOOP 0x00
#define OP_DIGIT0 0x01
#define OP_DIGIT1 0x02
#define OP_DIGIT2 0x03
#define OP_DIGIT3 0x04
#define OP_DIGIT4 0x05
#define OP_DIGIT5 0x06
#define OP_DIGIT6 0x07
#define OP_DIGIT7 0x08
#define OP_DECODEMODE 0x09
#define OP_INTENSITY 0x0A
#define OP_SCANLIMIT 0x0B
#define OP_SHUTDOWN 0x0C
#define OP_DISPLAYTEST 0x0F

SBK_MAX72xxHard::SBK_MAX72xxHard(uint8_t csPin,
                                 uint8_t devsNum)
    : _dataPin(0),
      _clkPin(0),
      _csPin(csPin),
      _devsNum(constrain(devsNum, 1, 8))
{
    _buffer = new uint8_t[_devsNum * _defaultColBufferSize];
    _update = new bool[_devsNum]();
    memset(_buffer, 0, _devsNum * _defaultColBufferSize);
}

void SBK_MAX72xxHard::setSPIClock(uint32_t frequency)
{
    _spiClock = frequency;
}

void SBK_MAX72xxHard::end()
{
    SPI.end();
}

SBK_MAX72xxHard::~SBK_MAX72xxHard()
{
    // Release the dynamically allocated memory
    delete[] _buffer;
    delete[] _update;
}

void SBK_MAX72xxHard::begin()
{
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH); // ensure chip deselected early
    SPI.begin();
    delay(50); // small stabilization delay

    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0)); // You can tune speed
    for (uint8_t i = 0; i < _devsNum; ++i)
    {
        setShutdown(i, false);             // Wake up
        setScanLimit(i, maxColumns() - 1); // Display all 8 digits
        _spiTransfer(i, OP_DECODEMODE, 0); // No decode
        testMode(i, false);                // <-- NEW: ensure test mode is OFF
        clear(i);                          // Clear display
        setBrightness(i, 8);               // Medium brightness
    }
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::setShutdown(uint8_t devIdx, bool status)
{
    _spiTransfer(devIdx, OP_SHUTDOWN, status ? 0 : 1);
}

void SBK_MAX72xxHard::setScanLimit(uint8_t devIdx, uint8_t limit)
{
    _spiTransfer(devIdx, OP_SCANLIMIT, limit & 0x07);
}

void SBK_MAX72xxHard::setBrightness(uint8_t devIdx, uint8_t brightness)
{
    // constrain the brightness to a 4-bit number (0–15)
    brightness &= 0x0F; // limit to 0–15

    _spiTransfer(devIdx, OP_INTENSITY, brightness & 0x0F);
}

void SBK_MAX72xxHard::clear(uint8_t devIdx)
{
    if (devIdx >= _devsNum)
        return;

    _update[devIdx] = true; // Mark this device for update

    for (uint8_t colIdx = 0; colIdx < maxColumns(); colIdx++)
    {
        _buffer[_colIndex(devIdx, colIdx)] = 0x00;
        _spiTransfer(devIdx, OP_DIGIT0 + colIdx, 0x00);
    }
}

void SBK_MAX72xxHard::clear()
{
    for (uint8_t d = 0; d < _devsNum; d++)
    {
        clear(d);
    }
}

void SBK_MAX72xxHard::setLed(uint8_t devIdx, uint8_t rowIdx, uint8_t colIdx, bool state)
{
    if (devIdx >= _devsNum || rowIdx >= maxRows(devIdx) || colIdx >= maxColumns())
        return;

    uint8_t &val = _buffer[_colIndex(devIdx, colIdx)];
    uint8_t prior = val;

    if (state)
        val |= _bitMaskRow(devIdx, rowIdx);
    else
        val &= ~_bitMaskRow(devIdx, rowIdx);

    if (val != prior)
        _update[devIdx] = true;

    Serial.print("[setLed] Dev: ");
    Serial.print(devIdx);
    Serial.print(" Row: ");
    Serial.print(rowIdx);
    Serial.print(" Col: ");
    Serial.print(colIdx);
    Serial.print(" State: ");
    Serial.println(state ? "ON" : "OFF");
}

bool SBK_MAX72xxHard::getLed(uint8_t devIdx, uint8_t rowIdx, uint8_t colIdx) const
{
    if (devIdx >= _devsNum || rowIdx >= maxRows(devIdx) || colIdx >= maxColumns())
        return false;

    return (_buffer[_colIndex(devIdx, colIdx)] & _bitMaskRow(devIdx, rowIdx)) != 0;
}

void SBK_MAX72xxHard::setCol(uint8_t devIdx, uint8_t colIdx, uint8_t value)
{
    if (devIdx >= _devsNum || colIdx >= maxColumns())
        return;

    if (_buffer[_colIndex(devIdx, colIdx)] != value)
    {
        _buffer[_colIndex(devIdx, colIdx)] = value;
        _update[devIdx] = true; // Mark device for update
    }
}

void SBK_MAX72xxHard::show()
{
    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0));
    for (uint8_t devIdx = 0; devIdx < _devsNum; devIdx++)
    {
        if (_update[devIdx])
        {
            for (uint8_t colIdx = 0; colIdx < maxColumns(); colIdx++)
            {
                _writeColToAllDevices(devIdx, colIdx, _buffer[_colIndex(devIdx, colIdx)]);
            }
            _update[devIdx] = false;
        }
    }
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::show(uint8_t devIdx)
{
    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0));
    if (devIdx >= _devsNum || !_update[devIdx])
        return;

    for (uint8_t colIdx = 0; colIdx < maxColumns(); colIdx++)
    {
        _writeColToAllDevices(devIdx, colIdx, _buffer[_colIndex(devIdx, colIdx)]);
    }
    _update[devIdx] = false;
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::testMode(uint8_t devIdx, bool enable)
{
    if (devIdx >= _devsNum)
        return;

    // MAX7219 test mode uses register 0x0F
    // value 1 = test ON (all segments), 0 = test OFF
    _spiTransfer(devIdx, OP_DISPLAYTEST, enable ? 1 : 0);
}

void SBK_MAX72xxHard::_spiTransfer(uint8_t targetDevice, uint8_t opcode, uint8_t data)
{
    if (targetDevice >= _devsNum)
        return; // Prevent invalid access

    digitalWrite(_csPin, LOW);

    for (int8_t i = _devsNum - 1; i >= 0; i--)
    {
        uint8_t op = (i == static_cast<int8_t>(targetDevice)) ? opcode : OP_NOOP;
        uint8_t val = (i == static_cast<int8_t>(targetDevice)) ? data : 0;

        SPI.transfer(op);
        SPI.transfer(val);
    }

    digitalWrite(_csPin, HIGH);
}

inline void SBK_MAX72xxHard::_writeColToAllDevices(uint8_t targetDevice, uint8_t colIdx, uint8_t data)
{
    if (targetDevice >= _devsNum || colIdx >= maxColumns())
        return;

    digitalWrite(_csPin, LOW);

    for (int8_t i = _devsNum - 1; i >= 0; i--)
    {
        uint8_t opcode = (i == static_cast<int8_t>(targetDevice)) ? (OP_DIGIT0 + colIdx) : OP_NOOP;
        uint8_t val = (i == static_cast<int8_t>(targetDevice)) ? data : 0;

        SPI.transfer(opcode);
        SPI.transfer(val);
    }

    digitalWrite(_csPin, HIGH);
}

inline uint8_t SBK_MAX72xxHard::_bitMaskRow(uint8_t devIdx, uint8_t rowIdx) const
{
    return 1 << ((maxRows(devIdx) - 1) - rowIdx);
}

inline uint8_t SBK_MAX72xxHard::_colIndex(uint8_t devIdx, uint8_t colIdx) const
{
    return devIdx * _defaultColBufferSize + colIdx;
}
