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

 *
 * @example examples/simpleDemo/simpleDemo.ino
 *
 * @author
 * Samuel Barabé (Smart Builds & Kits)
 *
 * @version 2.0.3
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
     * @param dataPin     Data input pin (DIN)
     * @param clkPin      Clock pin (CLK)
     * @param csPin       Chip Select pin (CS)
     * @param devsNum     Number of daisy-chained MAX72xx devices (typically up to 8). Default is 1.
     *
     * Initializes internal display buffer for each device in the chain.
     * Each device reserves 8 bytes — one for each digit line (DIG0–DIG7), representing columns (cathode outputs).
     *
     * @note Each byte in the buffer holds segment (SEG0–SEG7) values for one column.
     */
    SBK_MAX72xxSoft(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t devsNum = 1);

    ~SBK_MAX72xxSoft(); // Destructor

    /**
     * @brief Returns the number of addressable row lines (anode outputs = SEGx).
     *
     * @param devIdx Index of the target device (0-based in daisy chain).
     *               This parameter is ignored for MAX7219/7221 chips,
     *               but included for API compatibility with SBK_BarDrive.
     *
     * For MAX7219/7221 drivers, this value is always 8, since each column (DIGx)
     * can display up to 8 vertical segments connected to SEG0–SEG7 (anode lines).
     *
     * @return Number of row lines (SEGx/anodes), always 8 for MAX72xx devices.
     */
    uint8_t maxRows(uint8_t devIdx = 0) const
    {
        (void)devIdx;
        return _defaultRowBufferSize;
    }

    /**
     * @brief Returns the number of addressable columns (cathode outputs = DIGx).
     *
     * This is a fixed value of 8 for MAX7219/7221, since each digit line (DIG0–DIG7) selects one column (cathode).
     * Each DIGx line maps to one 8-bit buffer entry representing the vertical SEGx lines.
     *
     * @return Number of columns (DIGx = cathode outputs), always 8 for MAX7219/7221.
     */
    uint8_t maxColumns() const { return _defaultColBufferSize; }

    /**
     * @brief Returns the total number of addressable LED segments for this device.
     *
     * @param devIdx Index of the target device (0-based in daisy chain).
     *               This parameter is ignored for MAX7219/7221 chips,
     *               but is included for API compatibility with SBK_BarDrive.
     *
     * This value is computed as:
     * `maxRows(devIdx) × maxColumns()`
     * For MAX7219/7221, this is always 8 × 8 = 64 segments per device.
     *
     * @return Total number of addressable LED segments (pixels) for this device.
     */
    uint8_t maxSegments(uint8_t devIdx = 0) const { return maxRows(devIdx) * maxColumns(); }

    /**
     * @brief Initialize SPI pins and all MAX72xx chips.
     */
    void begin();

    /**
     * @brief Enable or disable shutdown mode on a specific device.
     *
     * @param devIdx Index of the target device.
     * @param status false = shutdown, true = normal operation
     */
    void setShutdown(uint8_t devIdx, bool status);

    /**
     * @brief Set the scan limit (number of active digits) for a specific device.
     *
     * @param devIdx Target device index.
     * @param limit  Value from 0 to 7.
     */
    void setScanLimit(uint8_t devIdx, uint8_t limit);

    /**
     * @brief Set display brightness for a specific device.
     *
     * @param devIdx Target device index.
     * @param brightness Value from 0 (min) to 15 (max).
     */
    void setBrightness(uint8_t devIdx, uint8_t brightness);

    /**
     * @brief Return the number of actives driver devices.
     *
     * @return number of actives driver devices.
     */
    uint8_t devsNum() const { return _devsNum; }

    /**
     * @brief Clear display buffer and hardware for one device.
     *
     * @param devIdx Target device index.
     */
    void clear(uint8_t devIdx);

    /**
     * @brief Clear display buffers and hardware for all devices.
     */
    void clear();

    /**
     * @brief Set the state of a specific LED in the device’s internal matrix buffer.
     *
     * @param devIdx    Index of the target device (0-based in daisy chain).
     * @param rowIdx    Logical rowIdx index (0 to maxRows(_devIdx) - 1) — vertical position (anode).
     * @param colIdx    Logical column index (0 to maxColumns() - 1) — horizontal position (cathode).
     * @param state     true = LED ON, false = LED OFF.
     *
     * @note The coordinate system follows a standard [row, col] layout.
     *       For MAX72xx drivers:
     *         - row corresponds to SEGx (segment outputs, V+ source = anode)
     *         - col corresponds to DIGx (digit selectors, GND sink = cathode)
     *
     * This function updates the internal buffer; call show() to apply changes to hardware.
     */
    void setLed(uint8_t devIdx, uint8_t rowIdx, uint8_t colIdx, bool state);

    /**
     * @brief Get the state of a specific LED in the device’s internal matrix buffer.
     *
     * This function reads the last known state of a given LED at the specified row and column
     * on a target device. It does not access the physical display hardware, but instead reads from
     * the internal RAM buffer used for batching updates.
     *
     * @note This function may not reflect real-time display contents unless `show()` has been called
     * after `setLed()`. For animations or state-dependent logic, ensure consistency by calling `show()` regularly.
     *
     * @param devIdx Index of the target device (0-based).
     * @param rowIdx Row index (0–7).
     * @param colIdx Column index (0–7).
     * @return true if the LED is currently set ON in the buffer, false if OFF or invalid.
     *
     * @note The coordinate system follows a standard [row, col] layout.
     *       For MAX72xx drivers:
     *         - row corresponds to SEGx (segment outputs, V+ source = anode)
     *         - col corresponds to DIGx (digit selectors, GND sink = cathode)
     */
    bool getLed(uint8_t devIdx, uint8_t rowIdx, uint8_t colIdx) const;

    /**
     * @brief Set the entire col value for a specific device (buffer only).
     *
     * @param devIdx    Index of the target device.
     * @param colIdx    Column number (0 to 7).
     * @param value     8-bit value for the row.
     */
    void setCol(uint8_t devIdx, uint8_t colIdx, uint8_t value);

    /**
     * @brief Push the internal display buffer to all connected devices.
     *
     * This flushes all buffered LED states to the physical hardware for every device
     * managed by this driver instance. Use this after making multiple `setLed()` calls
     * to apply the changes to the display.
     *
     * Typically used in split-device or multi-bar meter setups.
     */
    void show();

    /**
     * @brief Push the internal display buffer to a specific device.
     *
     * @param devIdx Index of the target device (0-based in the daisy chain).
     *
     * Only the specified device's display will be updated. Useful for optimized
     * partial updates when only one device has changed.
     *
     * @note The driver must track changes correctly for this to be meaningful.
     */
    void show(uint8_t devIdx);

    /**
     * @brief Enable/disable display-test mode on all devices.
     */
    void testMode(bool enable)
    {
        for (uint8_t i = 0; i < _devsNum; i++)
            testMode(i, enable);
    }

    /**
     * @brief Enable or disable the MAX72xx display test mode.
     *
     * @param devIdx Target device index (0-based)
     * @param enable true = enable test mode (all LEDs ON), false = disable
     */
    void testMode(uint8_t devIdx, bool enable);

private:
    void _spiTransfer(uint8_t targetDevice, uint8_t opcode, uint8_t data);
    void _writeColToAllDevices(uint8_t targetDevice, uint8_t colIdx, uint8_t data);
    inline uint8_t _bitMaskRow(uint8_t devIdx, uint8_t rowIdx) const;
    inline uint8_t _colIndex(uint8_t devIdx, uint8_t colIdx) const;

    const uint8_t _dataPin;
    const uint8_t _clkPin;
    const uint8_t _csPin;
    const uint8_t _devsNum = 1;

    static constexpr uint8_t _defaultRowBufferSize = 8;
    static constexpr uint8_t _defaultColBufferSize = 8;
    uint8_t *_buffer; // Internal display buffer
    bool *_update;    // Array to track if data has changed per device

    uint32_t _spiClock = 1000000; // Default 1 MHz
};
