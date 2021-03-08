/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

    tasks.c

Module Description:

    The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE

************************************************************************************/
#include <stdarg.h>
#include <vector>

#include "bsp.h"
#include "print.h"
#include "mp3Util.h"
#include "drivers.h"

#define PENRADIUS 3

long MapTouchToScreen(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#define BUFSIZE 256

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/

static OS_STK   LcdTouchDemoTaskStk[APP_CFG_TASK_START_STK_SIZE];
static OS_STK   Mp3DemoTaskStk[APP_CFG_TASK_START_STK_SIZE];


// Task prototypes
void LcdTouchDemoTask(void* pdata);
void Mp3DemoTask(void* pdata);

// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);
void DebugSDContents();
void ListMP3Files();

struct Song {
  std::string filename;
};

#define MAX_SONGS 64
static Song songHeapArray[MAX_SONGS];
static OS_MEM* songHeap = NULL;
static std::vector<Song*> songs;

// Globals
BOOLEAN nextSong = OS_FALSE;

/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
    char buf[BUFSIZE];

    PrintWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");

    // Start the system tick
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");
    SetSysTick(OS_TICKS_PER_SEC);

    PrintWithBuf(buf, BUFSIZE, "StartupTask: Initializing Hardware\n");
    InitializeSD();
    InitializeLCD(lcdCtrl);
    InitializeTouch(touchCtrl);

    PrintWithBuf(buf, BUFSIZE, "StartupTask: SD Card Contents:\n");
    DebugSDContents();

    // initialize songHeap
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Initializeing Song Heap\n");
    if (songHeap == NULL) {
      INT8U uCOSerr;
      songHeap = OSMemCreate(songHeapArray, MAX_SONGS, sizeof(Song), &uCOSerr);
      if (uCOSerr != OS_ERR_NONE) while (1);
    }

    ListMP3Files();
    PrintWithBuf(buf, sizeof(buf), "Songs:\n");
    for (auto it = songs.begin(); it != songs.end(); ++it) {
      PrintWithBuf(buf, sizeof(buf), "  %s\n", (*it)->filename.c_str());
    }

    // Create the test tasks
    PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

    // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
    OSTaskCreate(Mp3DemoTask, (void*)0, &Mp3DemoTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST1_PRIO);
    OSTaskCreate(LcdTouchDemoTask, (void*)0, &LcdTouchDemoTaskStk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_TEST2_PRIO);

    // Delete ourselves, letting the work be done in the new tasks.
    PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
	OSTaskDel(OS_PRIO_SELF);
}

static void DrawLcdContents()
{
    char buf[BUFSIZE];
    OS_CPU_SR cpu_sr;

    // allow slow lower pri drawing operation to finish without preemption
    OS_ENTER_CRITICAL();

    lcdCtrl.fillScreen(ILI9341_BLACK);

    // Print a message on the LCD
    lcdCtrl.setCursor(40, 60);
    lcdCtrl.setTextColor(ILI9341_WHITE);
    lcdCtrl.setTextSize(2);
    PrintToLcdWithBuf(buf, BUFSIZE, "Hello World!");

    OS_EXIT_CRITICAL();

}

/************************************************************************************

   Runs LCD/Touch demo code

************************************************************************************/
void LcdTouchDemoTask(void* pdata)
{
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "LcdTouchDemoTask: starting\n");

    DrawLcdContents();

    int currentcolor = ILI9341_RED;
    while (1) {
        boolean touched;

        touched = touchCtrl.touched();

        if (! touched) {
            OSTimeDly(5);
            continue;
        }

        TS_Point point;

        point = touchCtrl.getPoint();
        if (point.x == 0 && point.y == 0)
        {
            continue; // usually spurious, so ignore
        }

        // transform touch orientation to screen orientation.
        TS_Point p = TS_Point();
        p.x = MapTouchToScreen(point.x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0);
        p.y = MapTouchToScreen(point.y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0);

        lcdCtrl.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
    }
}
/************************************************************************************

   Runs MP3 demo code

************************************************************************************/
void Mp3DemoTask(void* pdata)
{
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "Mp3DemoTask: starting\n");

    HANDLE hMp3;
    InitializeMP3(hMp3);

    int count = 0;

    while (1)
    {
        OSTimeDly(500);
#if 0
        PrintWithBuf(buf, BUFSIZE, "Begin streaming sound file  count=%d\n", ++count);
        Mp3Stream(hMp3, (INT8U*)Train_Crossing, sizeof(Train_Crossing));
        PrintWithBuf(buf, BUFSIZE, "Done streaming sound file  count=%d\n", count);
#endif
    }
}


// Renders a character at the current cursor position on the LCD
static void PrintCharToLcd(char c)
{
    lcdCtrl.write(c);
}

/************************************************************************************

   Print a formated string with the given buffer to LCD.
   Each task should use its own buffer to prevent data corruption.

************************************************************************************/
void PrintToLcdWithBuf(char *buf, int size, char *format, ...)
{
    va_list args;
    va_start(args, format);
    PrintToDeviceWithBuf(PrintCharToLcd, buf, size, format, args);
    va_end(args);
}

void DebugSDContentsHelper(File dir, int level = 0) {
  char buf[24]; // holds 8.3 name
  if (!dir) return;
  PrintWithBuf(buf, sizeof(buf), "%*s%s\n", 2*level, "", dir.name());
  level += 1;
  while (1) {
    File entry = dir.openNextFile();
    if (!entry) { break; }
    if (entry.isDirectory()) { DebugSDContentsHelper(entry, level); continue; }
    PrintWithBuf(buf, sizeof(buf), "%*s%s\n", 2*level, "", entry.name());
    entry.close();
  }
}

void DebugSDContents() {
  File dir = SD.open("/");
  DebugSDContentsHelper(dir);
}

void ListMP3FilesHelper(File dir) {
  char buf[24];
  if (!dir) { return; }
  while (1) {
    File entry = dir.openNextFile();
    if (!entry) { break; }
    if (entry.isDirectory()) { ListMP3FilesHelper(entry); }
    else {
      if (!strstr(entry.name(), ".MP3")) { continue; }
      // allocate space for song
      INT8U uCOSerr;
      auto song = static_cast<Song*>(OSMemGet(songHeap, &uCOSerr));
      if (uCOSerr != OS_ERR_NONE) while (1);
      // populate song fields
      song->filename = std::string{ entry.name() };
      songs.push_back(song);
      PrintWithBuf(buf, sizeof(buf), "%s\n", entry.name());
    }
    entry.close();
  }
}

void ListMP3Files() {
  File dir = SD.open("/");
  ListMP3FilesHelper(dir);
  dir.close();
}


