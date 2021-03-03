# Assignment 5 - MP3PlayerDriver
## [EMBSYS 320 Winter 2021](/../../)
#### Carl B. Smiley

### Description
Migrated the I2C code within the Adafruit library into a PJDF driver "file".

### Example Output
```
MP3 Player Demo: Built Feb 17 2021 09:24:50.

main: Running OSInit()...
Initializing PJDF driver framework...
main: Creating start up task.
Starting multi-tasking.
StartupTask: Begin
StartupTask: Starting timer tick
HCLK frequency = 16 MHz
Configured ticksPerSec = 1000
Opening handle to SD driver: /dev/sd_adafruit
Opening SD SPI driver: /dev/spi1
StartupTask: Creating the application tasks
StartupTask: deleting self
Mp3DemoTask: starting
Opening MP3 driver: /dev/mp3_vs1053
Opening MP3 SPI driver: /dev/spi1
Starting MP3 device test
LcdTouchDemoTask: starting
Opening LCD driver: /dev/lcd_ili9341
Opening LCD SPI driver: /dev/spi1
Initializing LCD controller
Opening FT6206 I2C driver: /dev/i2c1
Initializing FT6206 touchscreen controller
Begin streaming sound file  count=1
Done streaming sound file  count=1
Begin streaming sound file  count=2
Done streaming sound file  count=2
Begin streaming sound file  count=3
Done streaming sound file  count=3
```