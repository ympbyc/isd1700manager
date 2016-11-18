ISD1700 Fragmented Memory Manager
=================================

Arduino sketch that controls ISD1700 record/playback ic over SPI. It's packed with awesome features. Read on.

<!-- ![Test Setup](docs/test-setup.jpg?raw=true "Test Setup") -->
<img src="docs/test-setup.jpg?raw=true" alt="Test Setup" title="Test Setup" width="480" style="max-width:100%;">

Caution
-------

It's a WIP and a little buggy.

Features
--------

+ Fragmented memory management: divides the memory into N fixed-size sections.
+ LED indicators: N LEDs can be attached to IO pins to indicate memory occupancy status and cursor position.
+ Multi-function buttons: Reduce the number of pushbuttons by utilizing single-click/double-click/long-press actions.
+ Sleep mode: Arduino enters SLEEP_MODE_PWR_DOWN after putting ISD1700 to power-down mode.

\* N being an integer

Default Setup
-------------

### Memory management

Address range is configured for ISD1790. Adjust LAST_ADDR macro value to match your chip. (Refer to ISD1700 Design Guide)

Number of sections the memory gets devided into is set by SLOT_NUM macro.

### APC register

By default the APC is setup to accept input from Mic. Change the APC macro value to match your need. (Refer to ISD1700 Design Guide)

### Push Buttons

#### MAIN_BTN

+ single-click: (play) play current section
+ double-click: (fwd)  move to next occupied section and play
+ long-press:   (rec)  start recording into first vacant section

#### SUB_BTN

+ single-click: erase current section
+ long-press:   erase entire memory

### LEDs

+ ON when the section is occupied
+ OFF when the section is vacant
+ Blink to indicate current section (play, fwd, rec)

Default Wiring
--------------

| ISD1700 | Arduino |
| ------- | ------- |
| MISO    | D12     |
| MOSI    | D11     |
| SCLK    | D13     |
| SS      | D10     |


| Arduino | LED/BTN  |
| ------- | -------- |
| D14(A0) | LED0     |
| D15(A1) | LED1     |
| ...     | LEDN-1   |
| D3      | MAIN_BTN |
| D2      | SUB_BTN  |


Schematic
---------

<!-- ![ISD1700 Schematic](docs/schematic-isd1700.jpg?raw=true "ISD1700 Schematic") -->
<img src="docs/schematic-isd1700.jpg?raw=true" alt="ISD1700 Schematic" title="ISD1700 Schematic" width="480" style="max-width:100%;">
<!-- ![Arduino Schematic](docs/schematic-arduino.jpg?raw=true "Arduino Schematic") -->
<img src="docs/schematic-arduino.jpg?raw=true" alt="Arduino Schematic" title="Arduino Schematic" width="480" style="max-width:100%;">
