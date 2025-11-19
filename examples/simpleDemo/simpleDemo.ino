/**
 * @file simpleDemo.ino
 * @brief Basic test sketch for SBK_MAX72xxSoft and SBK_MAX72xxHard drivers.
 *
 * Lights up a diagonal LED pattern on an 8x8 matrix connected to a MAX7219.
 * This sketch demonstrates both software and hardware SPI initialization.
 *
 * Select one driver and comment out the other.
 *
 * @author
 * Samuel Barabé (Smart Builds & Kits)
 * @version 2.0.2
 * @license MIT
 */

#include <Arduino.h>

// Uncomment ONE of the following driver types:

// --- Software SPI example ---
#include <SBK_MAX72xxSoft.h>
SBK_MAX72xxSoft matrix(4, 5, 6, 1); // data, clk, cs, num devices

// --- Hardware SPI example ---
// #include <SBK_MAX72xxHard.h>
// SBK_MAX72xxHard matrix(10, 1); // cs pin, num devices

void setup() {
  matrix.begin();              // Initialize SPI and MAX72xx
  matrix.setBrightness(0, 10); // Set brightness (0–15)
  matrix.clear(0);             // Clear device 0's display

  // Draw diagonal pattern
  for (uint8_t i = 0; i < 8; i++) {
    matrix.setLed(0, i, i, true); // Turn on LED at (row=i, col=i)
  }

  matrix.show(0); // Send buffer to device
}

void loop() {
  // No updates required
}
