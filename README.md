# BLE_Bat

General BMS drivers and battery system emulators.

Includes:
* Bluetooth UARTS
* Vestwood BMS driver
* Daly BMS driver
* Series/parallel pack drivers
* Series pack balancer
* Emulated Pylon CAN output - including cell over/under voltage management and protection
* Serial logging formulated for easy parsing
* Log parser to InfluxDB driver

## Device selection

There are probably a number of Arduino devices that will do the trick, but
my shopping list was:
* Bluetooth LE (to communicate with Bluetooth BMSs)
* 5V I/O or 5V tolerant IO wih 5V CMOS compatible voltages (to connect to cheapie MCP-CAN modules)

What I had available which met these requirements was an ESP32 Dev Board.

It should be portable to other hardware, but may require some work as there
are some hard-coded ESP bits (like watchdog functions).

## Arduino configuration

Out of the box, Arduino-ESP32 has a hard limit of 3 simultaneous Bluetooth
connections.  To update this, follow these instructions:

### Install:

https://github.com/espressif/arduino-esp32

From github (NOT board manager) as in manual install instructions

Need to increase max connections:

https://github.com/espressif/esp-idf/issues/2524

### Download:

https://github.com/espressif/esp32-arduino-lib-builder

### Edit config:

configs/defconfig.esp32

Add:

CONFIG_BTDM_CTRL_BLE_MAX_CONN=9
CONFIG_BT_ACL_CONNECTIONS=9

### Do:

./build.sh
./tools/copy-to-arduino.sh

## Modules

The following modules are completed:
* Bluetooth UART (bleUART)
* BLE connection manager (batBLEManager)
* Vestwood BMS protocol client (batVestwoods)
* Daly BMS protocol client (batDaly)
* BMS poll manager (batBMSManager)
* Serial pack balancer (batBalancer)
* Serial/parallel battery combination/emulator (batBank)
* Protection parameter generation for output drivers (batDerate)
* Emulated Pylon CAN output (outPylonCAN)

## TODO

A great deal:
* These modules should be in a stand-alone library
* Better configuration (nasty mix of global, local and parametrized configuration - *puke* )
* Pylon RS485 output
* More BMS protocol clients
* Clean up data cleansing (currently Daly BMS occasionaly spits out bad data, and this is cleaned up in batDerate...) 

## Hardware

For CAN output, connect MCP CAN shield/module to SPI bus (board dependent,
but just a plain SPI connection, no special considerations other than 5V
supply and 5V IO).

For balancer, the module has one pin per battery.  This pin is asserted
(active level configurable in constructor) to discharge the associated
battery.

I have tested with a 4 channel relay module:

https://www.robotics.org.za/RELAY-4CH-5V

(active low input, so set invert to true!)

and SSRs:

https://www.robotics.org.za/P5DD60

In the end, I chose SSRs, as this will have a very high cycle count in the
long run.

Each realy connects one dummy load across one battery.  For dummy loads, I
have used trailer indicator lights.