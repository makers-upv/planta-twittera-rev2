@echo off

set Serial=COM8

set currentpath=%~dp0
set binary=planta_twittera_rev2.ino.bin
set config_file=planta_twittera_rev2.spiffs.bin

echo Uploading config file to SPIFFS...
call esptool.exe -vv -cd nodemcu -cb 921600 -cp %Serial% -ca 0x00300000 -cf %config_file%

echo Flashing FW Wemos D1 Mini ESP8266...
call esptool.exe -vv -cd nodemcu -cb 921600 -cp %Serial% -ca 0x00000 -cf %binary%

pause