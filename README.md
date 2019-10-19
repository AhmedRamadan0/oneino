# Oneino

## Features?

- Inbuilt wifi configurator. Connect with home wifi or use as Access Point. Choice is yours.
- Inbuilt timer to wake a connected device or make it sleep in hours and minutes
- Control upto **four** switches



## How to set it up?

1. Grab an esp8266 (or esp8266_nodemcu 1.0) and 3v controlled relay (or a 5v relay and use a voltage step down IC)

2. Download esptool.py using
`pip install esptool`

3. Open terminal and run the command:
`esptool.py erase_flash`

4. Download the latest precompiled binary from releases section.

5. Flash the binary using esptool. Please refer here: https://github.com/espressif/esptool

6. Please reset the device after flashing is complete.


## How to configure?

1. Post flashing a wifi AP will show up with SSID: oneinoXXXXX. The password is same as SSID.

2. Navigate to the link http://192.168.4.1/root to configure your device.

3. Click on "Change Password" and enter a mininimum of 8character password/ Connect to WiFi manually.

4. Depending on the above choice the new password may be required for the login to the device.

5. Navigate to http://192.168.4.1/ to control your devices (or alternatively, oneino.local).

6. Enter hour and minutes and click on set timer for that device to make it wake up or sleep depending on it's present state.



## How to set the connection?

- Connect D1, D2, D5, D7 on the individual pins of the relay. Connect Vin and GND respectively.
Reference: http://www.microdev.it/wp/wp-content/uploads/2017/04/NodemcuRelay-1.png
