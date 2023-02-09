# BLE_Bat
Bluetooth Battery to CAN

## Arduino configuration

Install:

https://github.com/espressif/arduino-esp32

From github (NOT board manager) as in manual install instructions

Need to increase max connections:

https://github.com/espressif/esp-idf/issues/2524

Download:

https://github.com/espressif/esp32-arduino-lib-builder

Edit config:

configs/defconfig.esp32

Add:

CONFIG_BTDM_CTRL_BLE_MAX_CONN=9
CONFIG_BT_ACL_CONNECTIONS=9

Do:

./build.sh
./tools/copy-to-arduino.sh
