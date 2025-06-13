/**
 * @file SBK_MAX72xxSoft.cpp
 * @brief Implementation of the SBK_MAX72xxSoft class for controlling MAX7219/MAX7221 LED drivers.
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

#include "SBK_MAX72xxSoft.h"

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

SBK_MAX72xxSoft::SBK_MAX72xxSoft(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices)
    : _dataPin(dataPin), _clkPin(clkPin), _csPin(csPin), _numDevices(numDevices)
{
    _buffer = new uint8_t[_numDevices * 8];
    _update = new bool[_numDevices]();
    memset(_buffer, 0, _numDevices * 8);
}

SBK_MAX72xxSoft::~SBK_MAX72xxSoft()
{
    // Release the dynamically allocated memory
    delete[] _buffer;
    delete[] _update;
}

void SBK_MAX72xxSoft::begin()
{
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);
    pinMode(_dataPin, OUTPUT);
    pinMode(_clkPin, OUTPUT);

    for (uint8_t i = 0; i < _numDevices; ++i)
    {
        setShutdown(i, false);            // Wake up
        setScanLimit(i, 7);               // Display all 8 digits
        spiTransfer(i, OP_DECODEMODE, 0); // No decode
        clear(i);                         // Clear display
        setBrightness(i, 8);              // Medium brightness
    }
}

void SBK_MAX72xxSoft::setShutdown(uint8_t device, bool status)
{
    spiTransfer(device, OP_SHUTDOWN, status ? 0 : 1);
}

void SBK_MAX72xxSoft::setScanLimit(uint8_t device, uint8_t limit)
{
    spiTransfer(device, OP_SCANLIMIT, limit & 0x07);
}

void SBK_MAX72xxSoft::setBrightness(uint8_t device, uint8_t brightness)
{
    spiTransfer(device, OP_INTENSITY, brightness & 0x0F);
}

void SBK_MAX72xxSoft::clear(uint8_t device)
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

void SBK_MAX72xxSoft::clear()
{
    for (uint8_t d = 0; d < _numDevices; d++)
    {
        clear(d);
    }
}

void SBK_MAX72xxSoft::setLed(uint8_t device, uint8_t row, uint8_t col, bool state)
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

bool SBK_MAX72xxSoft::getLed(uint8_t device, uint8_t row, uint8_t col) const
{
    if (device >= _numDevices || row > 7 || col > 7)
        return false;

    return (_buffer[device * 8 + row] >> (7 - col)) & 0x01;
}

void SBK_MAX72xxSoft::setRow(uint8_t device, uint8_t row, uint8_t value)
{
    if (device >= _numDevices || row > 7)
        return;

    if (_buffer[device * 8 + row] != value)
    {
        _buffer[device * 8 + row] = value;
        _update[device] = true; // Mark device for update
    }
}

void SBK_MAX72xxSoft::show()
{
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
}

void SBK_MAX72xxSoft::show(uint8_t device)
{
    if (device >= _numDevices || !_update[device])
        return;

    for (uint8_t row = 0; row < 8; row++)
    {
        _writeRowToAllDevices(device, row, _buffer[device * 8 + row]);
    }
    _update[device] = false;
}

void SBK_MAX72xxSoft::spiTransfer(uint8_t targetDevice, uint8_t opcode, uint8_t data)
{
    digitalWrite(_csPin, LOW);

    for (int8_t i = _numDevices - 1; i >= 0; i--)
    {
        uint8_t op = (i == targetDevice) ? opcode : OP_NOOP;
        uint8_t val = (i == targetDevice) ? data : 0;

        shiftOut(_dataPin, _clkPin, MSBFIRST, op);
        shiftOut(_dataPin, _clkPin, MSBFIRST, val);
    }

    digitalWrite(_csPin, HIGH);
}

inline void SBK_MAX72xxSoft::_writeRowToAllDevices(uint8_t targetDevice, uint8_t row, uint8_t data)
{
    digitalWrite(_csPin, LOW);

    for (int8_t i = _numDevices - 1; i >= 0; i--)
    {
        uint8_t opcode = (i == targetDevice) ? (OP_DIGIT0 + row) : OP_NOOP;
        uint8_t val = (i == targetDevice) ? data : 0;

        shiftOut(_dataPin, _clkPin, MSBFIRST, opcode);
        shiftOut(_dataPin, _clkPin, MSBFIRST, val);
    }

    digitalWrite(_csPin, HIGH);
}