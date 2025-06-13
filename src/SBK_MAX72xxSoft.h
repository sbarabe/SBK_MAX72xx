/**
 * @file SBK_MAX72xxSoft.h
 * @brief Low-level driver for controlling multiple MAX7219/MAX7221 LED matrix chips via software SPI.
 *
 * Part of the SBK BarDrive Arduino Library
 * https://github.com/yourusername/SBK_BarDrive
 *
 * This class handles communication with daisy-chained MAX7219/MAX7221 devices using software SPI.
 * It supports setting individual LEDs, rows, brightness, shutdown mode, and clearing displays
 * for one or multiple devices.
 *
 * It includes a per-device display buffer for efficient batch updates.
 *
 * @example examples/simpleDemo/simpleDemo.ino
 *
 * @author
 * Samuel Barabé (Smart Builds & Kits)
 *
 * @version 1.0.0
 *
 * @license MIT
 *
 * @copyright
 * Copyright (c) 2025 Samuel Barabé
 *
 * Repository: https://github.com/sbarabe/SBK_MAX72xx
 */

/*
 *  Adapted from the original Ledcontrol.h library by Eberhard Fahle:
 *  <https://github.com/wayoda/LedControl>
 *
 *  Original library:
 *  LEdControl.h - A library for controlling LEDs with a MAX7219/MAX7221
 *  Copyright (c) 2007 Eberhard Fahle
 *
 *  All credit for the original library goes to Eberhard Fahle.
 */

#pragma once

#define SBK_MAX72xx_IS_DEFINED

#include <Arduino.h>
#include <SPI.h>

/**
 * @class SBK_MAX72xxSoft
 * @brief Controls multiple MAX7219/MAX7221 LED drivers via software SPI.
 */
class SBK_MAX72xxSoft
{
public:
    /**
     * @brief Construct a new Software SPI SBK_MAX72xxSoft object.
     *
     * @param dataPin  Data input pin (DIN)
     * @param clkPin   Clock pin (CLK)
     * @param csPin    Chip Select pin (CS)
     * @param numDevices Number of daisy-chained devices (max 8 recommended)
     */
    SBK_MAX72xxSoft(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices = 1);

    ~SBK_MAX72xxSoft(); // Destructor

    /**
     * @brief Initialize SPI pins and all MAX72xx chips.
     */
    void begin();

    /**
     * @brief Enable or disable shutdown mode on a specific device.
     *
     * @param device Index of the target device.
     * @param status false = shutdown, true = normal operation
     */
    void setShutdown(uint8_t device, bool status);

    /**
     * @brief Set the scan limit (number of active digits) for a specific device.
     *
     * @param device Index of the target device.
     * @param limit  Value from 0 to 7.
     */
    void setScanLimit(uint8_t device, uint8_t limit);

    /**
     * @brief Set display brightness for a specific device.
     *
     * @param device Index of the target device.
     * @param brightness Value from 0 (min) to 15 (max).
     */
    void setBrightness(uint8_t device, uint8_t brightness);

    /**
     * @brief Clear display buffer and hardware for one device.
     *
     * @param device Index of the target device.
     */
    void clear(uint8_t device);

    /**
     * @brief Clear display buffers and hardware for all devices.
     */
    void clear();

    /**
     * @brief Set the state of a specific LED.
     *
     * @param device Index of the target device.
     * @param row    Row number (0 to 7).
     * @param col    Column number (0 to 7).
     * @param state  true = ON, false = OFF.
     */
    void setLed(uint8_t device, uint8_t row, uint8_t col, bool state);

    /**
     * @brief Get the state of a specific LED from the internal buffer.
     *
     * This function reads the last known state of a given LED at the specified row and column
     * on a target device. It does not access the physical display hardware, but instead reads from
     * the internal RAM buffer used for batching updates.
     *
     * @note This function may not reflect real-time display contents unless `show()` has been called
     * after `setLed()`. For animations or state-dependent logic, ensure consistency by calling `show()` regularly.
     *
     * @param device Index of the target device (0-based).
     * @param row Row index (0–7).
     * @param col Column index (0–7).
     * @return true if the LED is currently set ON in the buffer, false if OFF or invalid.
     */
    bool getLed(uint8_t _deviceIndex, uint8_t row, uint8_t col) const;

    /**
     * @brief Set the entire row value for a specific device (buffer only).
     *
     * @param device Index of the target device.
     * @param row    Row number (0 to 7).
     * @param value  8-bit value for the row.
     */
    void setRow(uint8_t device, uint8_t row, uint8_t value);

    /**
     * @brief Push current buffer to all devices.
     */
    void show();
    void show(uint8_t device);

private:
    void spiTransfer(uint8_t targetDevice, uint8_t opcode, uint8_t data);
    void _writeRowToAllDevices(uint8_t targetDevice, uint8_t row, uint8_t data);

    const uint8_t _dataPin;
    const uint8_t _clkPin;
    const uint8_t _csPin;
    const uint8_t _numDevices;

    uint8_t *_buffer; // Internal display buffer
    bool *_update;    // Array to track if data has changed per device

    uint32_t _spiClock = 1000000; // Default 1 MHz
};
