# SBK\_MAX72xx Library

Arduino-compatible driver for controlling MAX7219/MAX7221 LED matrix chips. Supports both software and hardware SPI implementations under a unified API, compatible with the SBK\_BarDrive animation framework or standalone use.

---

## 🆕 What’s New in Version 2.0.4

- Updated the begin() function to improve initialization stability.

> **Note:** All **version 1.x.x** releases are **deprecated**. Please migrate to version 2.0+ for full feature support and long-term maintenance.

---

## ✅ Features

* Supports both **hardware SPI** (`SBK_MAX72xxHard`) and **software SPI** (`SBK_MAX72xxSoft`)
* Control individual LEDs or full rows on daisy-chained MAX72xx chips
* Internal buffer system for efficient display updates
* Compatible with `SBK_BarDrive` for advanced LED bar meter animations

---

## 📦 Installation

1. Download or clone this repository:

   ```bash
   git clone https://github.com/sbarabe/SBK_MAX72xx.git
   ```
2. Copy it into your Arduino `libraries/` folder

---

## 🧪 Example (Software SPI)

**File**: `examples/simpleDemo/simpleDemo.ino`

```cpp
#include <SBK_MAX72xxSoft.h>

SBK_MAX72xxSoft matrix(4, 5, 6, 1); // data, clk, cs, 1 device

void setup() {
  matrix.begin();
  matrix.setBrightness(0, 10);
  matrix.clear(0);

  // Light up a diagonal
  for (uint8_t i = 0; i < 8; i++) {
    matrix.setLed(0, i, i, true);
  }

  matrix.show(0);
}

void loop() {
  // nothing
}
```

---

## 🧪 Example (Hardware SPI)

**File**: `examples/simpleDemo/simpleDemo.ino`

```cpp
#include <SBK_MAX72xxHard.h>

SBK_MAX72xxHard matrix(10, 1); // CS pin, 1 device

void setup() {
  matrix.begin();
  matrix.setSPIClock(2000000); // Optional: Set SPI speed to 2 MHz
  matrix.setBrightness(0, 12);
  matrix.clear(0);

  // Create an X pattern
  for (uint8_t i = 0; i < 8; i++) {
    matrix.setLed(0, i, i, true);
    matrix.setLed(0, i, 7 - i, true);
  }

  matrix.show(0);
}

void loop() {
  // nothing
}
```

---

## 🔧 API Overview

### Classes

| Class             | Description                           |
| ----------------- | ------------------------------------- |
| `SBK_MAX72xxSoft` | Software SPI driver for MAX72xx chips |
| `SBK_MAX72xxHard` | Hardware SPI driver for MAX72xx chips |

### Methods (Common)

| Method            | Description                            |
| ----------------- | -------------------------------------- |
| `begin()`         | Initialize SPI pins and MAX72xx chips  |
| `clear()`         | Clear all devices                      |
| `clear(device)`   | Clear specific device                  |
| `setLed()`        | Set individual LED state               |
| `getLed()`        | Get LED state from buffer              |
| `setRow()`        | Set all bits in a row                  |
| `setBrightness()` | Set display brightness                 |
| `setScanLimit()`  | Set number of visible digits (0-7)     |
| `setShutdown()`   | Enable or disable a device             |
| `show()`          | Push buffer content to all devices     |
| `show(device)`    | Push buffer content to specific device |
| `devsNum()`       | Return number of active devices        |
| `maxRows(device)` | Always returns 8 (API wrapper)         |
| `maxColumns()`    | Always returns 8                       |
| `maxSegments(dev)`| Always returns 64 (8 × 8)(API wrapper) |


### Additional (Hardware SPI Only)

| Method          | Description         |
| --------------- | ------------------- |
| `setSPIClock()` | Set SPI clock speed |
| `end()`         | End SPI session     |

---

## 🧩 Integration with SBK\_BarDrive

To use with [`SBK_BarDrive`](https://github.com/sbarabe/SBK_BarDrive):

* Include either `<SBK_MAX72xxSoft.h>` or `<SBK_MAX72xxHard.h>`
* Pass the driver into a `SBK_BarDrive<SBK_MAX72xx...>` instance

---

## 🪪 License

This library is released under the MIT License.

---

## 🫠 Credits

* Based on the original LedControl library by [Eberhard Fahle](https://github.com/wayoda/LedControl)
* Extended and maintained by **Samuel Barabé** (Smart Builds & Kits)
