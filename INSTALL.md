# INSTALLATION GUIDE

### Code Upload Steps

1) Install Visual Studio Code
2) Install PlatformIO by following instructions at http://docs.platformio.org/en/latest/ide/vscode.html
3) Import the anwi code into Visual Studio Code using PlatformIO ( PIO Home)
4) Connect the Wesmos D1 sensor to the computer using USB cable
5) Build and upload the code to the D1 sensor using the build option in PlatformIO

### Sensor Configuration Steps - Serial PORT

1) Open Arduino IDE's serial monitor and select Sensor serial PORT
2) Enter 'c' to configure the sensor
3) Enter the configuration data asked by sensor
4) Sensor will restart once data is confirmed by user
5) The sensor will enter "Protection Mode" on reboot

### Sensor Configuration Steps - Web Configuration

1) Provide power to the sensor
2) The sensor will become an Access Point with name ANWI_Sensor after 20 seconds delay
3) Connect to the sensor using computer
4) Use browser to connect to 192.168.4.1
5) Fill in the required data on the form displayed
6) Click on "Save Settings" button once the data is filled
7) The sensor will reboot and enter "Protection Mode"

### Standalone Alert aka IFTTT Alert Configuration Steps

1) Create an account on https://ifttt.com
2) Follow the steps mentioned on https://ifttt.com/maker_webhooks
3) Provide the unique IFTTT key to sensor during configuration phase

### Server Configuration Steps

1) Install node-red by following steps mentioned on https://nodered.org/docs/getting-started/installation
2) Copy the contents of server_nodered.json and import to node-red dashboard
3) Execute the Flow
4) Provide the Server IP to the sensor during configuration phase


