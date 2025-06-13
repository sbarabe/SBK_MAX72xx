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
 * @version 1.0.0
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

SBK_MAX72xxHard::SBK_MAX72xxHard(uint8_t csPin, uint8_t numDevices)
    : _dataPin(0), _clkPin(0), _csPin(csPin), _numDevices(numDevices)
{
    _buffer = new uint8_t[_numDevices * 8];
    _update = new bool[_numDevices]();
    memset(_buffer, 0, _numDevices * 8);
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
    digitalWrite(_csPin, HIGH);

    SPI.begin();
    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0)); // You can tune speed
    for (uint8_t i = 0; i < _numDevices; ++i)
    {
        setShutdown(i, false);            // Wake up
        setScanLimit(i, 7);               // Display all 8 digits
        spiTransfer(i, OP_DECODEMODE, 0); // No decode
        clear(i);                         // Clear display
        setBrightness(i, 8);              // Medium brightness
    }
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::setShutdown(uint8_t device, bool status)
{
    spiTransfer(device, OP_SHUTDOWN, status ? 0 : 1);
}

void SBK_MAX72xxHard::setScanLimit(uint8_t device, uint8_t limit)
{
    spiTransfer(device, OP_SCANLIMIT, limit & 0x07);
}

void SBK_MAX72xxHard::setBrightness(uint8_t device, uint8_t brightness)
{
    spiTransfer(device, OP_INTENSITY, brightness & 0x0F);
}

void SBK_MAX72xxHard::clear(uint8_t device)
{
    if (device >= _numDevices)
        return;

    for (uint8_t row = 0; row < 8; row++)
    {
        _buffer[device * 8 + row] = 0x00;
        _update[device] = true; // Mark this device for update
        spiTransfer(device, OP_DIGIT0 + row, 0x00);
    }
}

void SBK_MAX72xxHard::clear()
{
    for (uint8_t d = 0; d < _numDevices; d++)
    {
        clear(d);
    }
}

void SBK_MAX72xxHard::setLed(uint8_t device, uint8_t row, uint8_t col, bool state)
{
    if (device >= _numDevices || row > 7 || col > 7)
        return;

    uint8_t &val = _buffer[device * 8 + row];
    uint8_t prior = val;

    if (state)
        val |= (1 << (7 - col));
    else
        val &= ~(1 << (7 - col));

    if (val != prior)
        _update[device] = true;
}

bool SBK_MAX72xxHard::getLed(uint8_t device, uint8_t row, uint8_t col) const
{
    if (device >= _numDevices || row > 7 || col > 7)
        return false;

    return (_buffer[device * 8 + row] >> (7 - col)) & 0x01;
}

void SBK_MAX72xxHard::setRow(uint8_t device, uint8_t row, uint8_t value)
{
    if (device >= _numDevices || row > 7)
        return;

    if (_buffer[device * 8 + row] != value)
    {
        _buffer[device * 8 + row] = value;
        _update[device] = true; // Mark device for update
    }
}

void SBK_MAX72xxHard::show()
{
    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0));
    for (uint8_t d = 0; d < _numDevices; d++)
    {
        if (_update[d])
        {
            for (uint8_t row = 0; row < 8; row++)
            {
                _writeRowToAllDevices(d, row, _buffer[d * 8 + row]);
            }
            _update[d] = false;
        }
    }
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::show(uint8_t device)
{
    SPI.beginTransaction(SPISettings(_spiClock, MSBFIRST, SPI_MODE0));
    if (device >= _numDevices || !_update[device])
        return;

    for (uint8_t row = 0; row < 8; row++)
    {
        _writeRowToAllDevices(device, row, _buffer[device * 8 + row]);
    }
    _update[device] = false;
    SPI.endTransaction(); // 💡 Restores SPI state for other users
}

void SBK_MAX72xxHard::spiTransfer(uint8_t targetDevice, uint8_t opcode, uint8_t data)
{
    digitalWrite(_csPin, LOW);

    for (int8_t i = _numDevices - 1; i >= 0; i--)
    {
        uint8_t op = (i == targetDevice) ? opcode : OP_NOOP;
        uint8_t val = (i == targetDevice) ? data : 0;

        SPI.transfer(op);
        SPI.transfer(val);
    }

    digitalWrite(_csPin, HIGH);
}

inline void SBK_MAX72xxHard::_writeRowToAllDevices(uint8_t targetDevice, uint8_t row, uint8_t data)
{
    digitalWrite(_csPin, LOW);

    for (int8_t i = _numDevices - 1; i >= 0; i--)
    {
        uint8_t opcode = (i == targetDevice) ? (OP_DIGIT0 + row) : OP_NOOP;
        uint8_t val = (i == targetDevice) ? data : 0;

        SPI.transfer(opcode);
        SPI.transfer(val);
    }

    digitalWrite(_csPin, HIGH);
}