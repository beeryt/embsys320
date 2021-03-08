#include "drivers.h"
#include "mp3Util.h"

Adafruit_ILI9341 lcdCtrl = Adafruit_ILI9341(); // The LCD controller
Adafruit_FT6206 touchCtrl = Adafruit_FT6206(); // The touch controller

void InitializeSD() {
  INT32U length = sizeof(HANDLE);
  PjdfErrCode pjdfErr;
  HANDLE hSD, hSPI;

  // Initialize the SD handle
  hSD = Open(PJDF_DEVICE_ID_SD_ADAFRUIT, 0);
  if (!PJDF_IS_VALID_HANDLE(hSD)) while (1);

  // We talk to the SD controller over a SPI interface therefore we open
  // an instance of the SPI driver and pass the handle to the SD driver.
  hSPI = Open(SD_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while (1);

  // Call Ioctl on hSD to set the SPI handle
  pjdfErr = Ioctl(hSD, PJDF_CTRL_SD_SET_SPI_HANDLE, &hSPI, &length);
  if (PJDF_IS_ERROR(pjdfErr)) while (1);

  // Initialize the SD driver
  if (!SD.begin(hSD)) while (1);
}

void InitializeLCD(Adafruit_ILI9341& lcdCtrl) {
  INT32U length = sizeof(HANDLE);
  PjdfErrCode pjdfErr;
  HANDLE hLCD, hSPI;

  // Initialize the LCD handle
  hLCD = Open(PJDF_DEVICE_ID_LCD_ILI9341, 0);
  if (!PJDF_IS_VALID_HANDLE(hLCD)) while (1);

  // We talk to the LCD controller over a SPI interface therefore we open
  // an instance of the SPI driver and pass the handle to the LCD driver.
  hSPI = Open(SD_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while (1);

  // Call Ioctl on hLCD to set the SPI handle
  pjdfErr = Ioctl(hLCD, PJDF_CTRL_LCD_SET_SPI_HANDLE, &hSPI, &length);
  if (PJDF_IS_ERROR(pjdfErr)) while (1);

  lcdCtrl.setPjdfHandle(hLCD);
  lcdCtrl.begin();
}

void InitializeTouch(Adafruit_FT6206& touchCtrl) {
  INT8U addr = FT6206_ADDR;
  INT32U length = sizeof(addr);
  PjdfErrCode pjdfErr;
  HANDLE hI2C;

  // Initialize the I2C handle
  hI2C = Open(LCD_I2C_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hI2C)) while (1);

  // Call Ioctl on hI2C to set the device address
  pjdfErr = Ioctl(hI2C, PJDF_CTRL_I2C_SET_DEVICE_ADDRESS, &addr, &length);
  if (PJDF_IS_ERROR(pjdfErr)) while (1);

  touchCtrl.setPjdfHandle(hI2C);
  if (!touchCtrl.begin(TOUCH_SENSITIVITY)) while (1);
}

void InitializeMP3(HANDLE& hMP3) {
  INT32U length = sizeof(HANDLE);
  PjdfErrCode pjdfErr;

  // Initialize the MP3 handle
  hMP3 = Open(PJDF_DEVICE_ID_MP3_VS1053, 0);
  if (!PJDF_IS_VALID_HANDLE(hMP3)) while (1);

  // We talk to the MP3 decoder over a SPI interface therefore we open
  // an instance of the SPI driver and pass the handle to the MP3 driver.
  HANDLE hSPI = Open(MP3_SPI_DEVICE_ID, 0);
  if (!PJDF_IS_VALID_HANDLE(hSPI)) while (1);

  // Call Ioctl on hMP3 to set the SPI handle
  pjdfErr = Ioctl(hMP3, PJDF_CTRL_MP3_SET_SPI_HANDLE, &hSPI, &length);
  if (PJDF_IS_ERROR(pjdfErr)) while (1);

  // initialize MP3 driver
  Mp3Init(hMP3);
}
