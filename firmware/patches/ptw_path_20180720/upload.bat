TITLE Planta Twittera updater tool
@echo off

set /p Serial=Enter serial port: 

set currentpath=%~dp0
set binary=planta_twittera_rev2_20180720.bin
set config_file=planta_twittera_rev2_spiffs_20180720.bin

echo Uploading config file to SPIFFS...
call esptool.exe -v -cd nodemcu -cb 921600 -cp %Serial% -ca 0x00300000 -cf %config_file%

echo Flashing FW Wemos D1 Mini ESP8266...
call esptool.exe -v -cd nodemcu -cb 921600 -cp %Serial% -ca 0x00000 -cf %binary%

pause