# Actualizar parche firmware Planta Twittera
___

Para actualizar el firmware y aplicarle nuevos parches a la Planta Twittera se ha desarrollado un pequeño script de ventana de comandos. Para utilizarlo seguir las siguientes instrucciones

## Instrucciones
1. Descargar el path
2. Conectar el Wemos D1 Mini de la Planta Twittera a un puerto USB del ordenador. No hace falta sacarlo del socket.
3. Mirar el puerto COM que le asigna el sistema operativo. COMXX en Windows (Mirar Administrador de Dispositivos)
4. Ejecutar *upload.bat* en caso de querer flashear la memoria completa (firmware + archivo de configuración) o *upload_onlyfirmware.bat* para no sobreescribir el archivo de configuración en la memoria flash.
5. Introducir el puerto serie correspondiente. Ejemplo "COM13" (SIN COMILLAS) y dar ENTER
6. Eperar a que se complete la actualización
