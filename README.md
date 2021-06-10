# Tiny FM Radio v1
A small battery-powered FM Radio powered by the ATtiny3216 microcontroller.

It was originally intented to use the SI4703 FM tuner and some other ICs, but due to some PCB design mistakes and since this was originally a birthday project for my mother and I had to finish it ASAP, I was forced to drop some features for now and integrate in the project an FM tuner kit based on the HEX3653 IC I found at a local electronic components store.

# Present features
  * SH1106 monochrome OLED display (128x64 px, I2C interface)
  * TPA2005D1 1.4W audio amplifier fed by a PAM2401 5V/1A boost converter
  * 2W / 8Ohm speaker
  * 6 tactile buttons (up, down, left, right, ok and power)
  * 1200mAh LiPo battery
  * MCP73831 500mA LiPo / Li-Ion battery charger IC
  * Micro USB port for battery charging
  * AP2127K-3.3TRG1 3.3V / 400mA voltage regulator to supply the ICs. Uses the PAM2401 5V supply as input
  * ATtiny3216 microcontroller. Features:
    * Controls the HEX3653 FM tuner and the shutdown pins of the TPA2005D1 and PAM2401 ICs via a menu displayed on the OLED
    * Monitors the battery voltage
    * Shuts down the 5V supply when the battery voltage reaches a critical voltage

# Planned features (possible after fixing the PCB model)
  * SI4703-C19 FM tuner IC featuring volume control, RDS and more (needs external 32.768Khz oscillator)
  * Timekeeping using the MCP79402 real-time clock
