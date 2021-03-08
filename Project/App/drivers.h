#pragma once

#include <SD.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

#define TOUCH_SENSITIVITY 40

// Hardware initialization functions
void InitializeSD();
void InitializeLCD(Adafruit_ILI9341&);
void InitializeTouch(Adafruit_FT6206&);
void InitializeMP3(HANDLE&);

// Global Hardware Controllers
extern Adafruit_ILI9341 lcdCtrl;  // The LCD controller
extern Adafruit_FT6206 touchCtrl; // The touch controller