/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

    tasks.c

Module Description:

    The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE

************************************************************************************/
#include <cstdarg>
#include <cctype>
#include <cassert>
#include <algorithm>
#include <vector>

#include "task.h"
#include "bsp.h"
#include "print.h"
#include "mp3Util.h"
#include "drivers.h"

#include "event.h"
#include "mp3.h"

#define PENRADIUS 3

template <typename T, typename U>
constexpr T map(U x, T imin, T imax, T omin, T omax) {
  return (x - imin) * (omax - omin) / (imax - imin) + omin;
}


#define BUFSIZE 256
#define DEFAULT_STK_SIZE 256

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/
// Task stacks
static OS_STK   LcdDisplayTaskStk[DEFAULT_STK_SIZE];
static OS_STK   TouchInputTaskStk[DEFAULT_STK_SIZE];
static OS_STK   Mp3StreamTaskStk[DEFAULT_STK_SIZE];

// Task prototypes
void LcdDisplayTask(void* pdata);
void TouchInputTask(void* pdata);
void Mp3StreamTask(void* pdata);

// Task list
const std::vector<Task> tasks = {
  CREATE_TASK(5, Mp3StreamTask, NULL, Mp3StreamTaskStk),
  CREATE_TASK(6, TouchInputTask, NULL, TouchInputTaskStk),
  CREATE_TASK(7, LcdDisplayTask, NULL, LcdDisplayTaskStk),
};

// Useful functions
void PrintToLcdWithBuf(char *buf, int size, char *format, ...);
void DebugSDContents();
void ListMP3Files();
void DrawLcdContents();

struct Song {
  std::string filename;
};

// global song list
#define MAX_SONGS 64
static Song songHeapArray[MAX_SONGS];
static OS_MEM* songHeap = NULL;
static std::vector<Song*> songs;

// event queue
#define MAX_EVENTS 32
Event eventHeapArray[MAX_EVENTS];
Event* eventQueueArray[MAX_EVENTS];
OS_MEM* eventHeap = NULL;
OS_EVENT* eventQueue = NULL;

// Globals
BOOLEAN nextSong = OS_FALSE;
Bitmap play_texture, prev_texture, next_texture, pause_texture;

/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
  INT8U uCOSerr;
  char buf[BUFSIZE];

  PrintWithBuf(buf, BUFSIZE, "StartupTask: Begin\n");

  // Start the system tick
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Starting timer tick\n");
  SetSysTick(OS_TICKS_PER_SEC);

  // Initialize hardware drivers
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Initializing Hardware\n");
  InitializeSD();
  InitializeLCD(lcdCtrl);
  InitializeTouch(touchCtrl);

  // initialize songHeap
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Initializeing Song Heap\n");
  if (songHeap == NULL) {
    songHeap = OSMemCreate(songHeapArray, MAX_SONGS, sizeof(Song), &uCOSerr);
    if (uCOSerr != OS_ERR_NONE) while (1);
  }

  // initialize eventHeap
  PrintWithBuf(buf, sizeof(buf), "StartupTask: Initializing eventQueue\n");
  if (eventHeap == NULL) {
    eventHeap = OSMemCreate(eventHeapArray, MAX_EVENTS, sizeof(Event), &uCOSerr);
    if (uCOSerr != OS_ERR_NONE) while (1);
  }

  // initialize eventQueue
  if (eventQueue == NULL) {
    eventQueue = OSQCreate((void**)&eventQueueArray[0], MAX_EVENTS);
    if (eventQueue == NULL) while (1);
  }

  // List SD card contents
  PrintWithBuf(buf, BUFSIZE, "StartupTask: SD Card Contents:\n");
  ListMP3Files();
  PrintWithBuf(buf, sizeof(buf), "Songs:\n");
  for (auto it = songs.begin(); it != songs.end(); ++it) {
    PrintWithBuf(buf, sizeof(buf), "  %s\n", (*it)->filename.c_str());
  }

  // load bitmap images
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Loading bitmap icons\n");
  play_texture = loadBitmap("icon/play.pgm");
  pause_texture = loadBitmap("icon/pause.pgm");
  prev_texture = loadBitmap("icon/prev.pgm");
  next_texture = loadBitmap("icon/next.pgm");

  // Create the test tasks
  PrintWithBuf(buf, BUFSIZE, "StartupTask: Creating the application tasks\n");

  OSStatInit();

  // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    OSTaskCreate(it->task, it->arg, it->stack, it->priority);
  }

  // Delete ourselves, letting the work be done in the new tasks.
  PrintWithBuf(buf, BUFSIZE, "StartupTask: deleting self\n");
      OSTaskDel(OS_PRIO_SELF);
}

/************************************************************************************

   Runs LCD/Touch demo code

************************************************************************************/
void LcdDisplayTask(void* pdata)
{
  INT8U uCOSerr;
  char buf[BUFSIZE];
  PrintWithBuf(buf, BUFSIZE, "LcdDisplayTask: starting\n");

  MP3 b(lcdCtrl.width(), lcdCtrl.height());
  b.title.setText("Kling To The Wreckage");
  b.artist.setText("The Crystal Method");
  b.refreshLayout();
  b.setGFX(&lcdCtrl);
  lcdCtrl.fillScreen(0);

  INT32U time = OSTimeGet();

  while (1) {
    // handle all events in eventQueue
    do {
      // Pop msg from eventQueue
      Event* msg = (Event*)OSQPend(eventQueue, 1, &uCOSerr);
      if (uCOSerr != OS_ERR_NONE) break;

      // Do something with the event
      b.input(*msg);

      // Free allocated memory
      uCOSerr = OSMemPut(eventHeap, msg);
      if (uCOSerr != OS_ERR_NONE) {
        PrintWithBuf(buf, sizeof(buf), "LcdDisplayTask: eventHeap failure %d\n", uCOSerr);
        while (1);
      }
    } while (uCOSerr == OS_ERR_NONE);

    INT32U next = OSTimeGet();
    b.process(next - time);
    time = next;
    OSTimeDly(20);
  }
}

void TouchInputTask(void* pdata)
{
  const unsigned TIMEOUT = 4;
  char buf[BUFSIZE];
  PrintWithBuf(buf, BUFSIZE, "TouchInputTask: starting\n");

  enum State {
    IDLE,
    TOUCH,
    RELEASE
  };

  // Convert a TS_Point to a Vec2<int16_t>
  auto getPoint = [&]() {
    auto point = touchCtrl.getPoint();
    int16_t x = map(point.x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0);
    int16_t y = map(point.y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0);
    return Vec2<>{ x,y };
  };

  auto sendEvent = [&](Event e) {
    INT8U uCOSerr;
    // Obtain an Event* from eventHeap
    auto event = static_cast<Event*>(OSMemGet(eventHeap, &uCOSerr));
    if (uCOSerr != OS_ERR_NONE) {
      PrintWithBuf(buf, sizeof(buf), "TouchInputTask: eventHeap failure %d\n", uCOSerr);
      while (1);
    }

    // Prepare event
    *event = e;

    // Post event to eventQueue
    uCOSerr = OSQPost(eventQueue, event);
    if (uCOSerr == OS_ERR_Q_FULL) {
      PrintWithBuf(buf, sizeof(buf), "TouchInputTask: Queue is full!\n");
    }
  };

  unsigned counter = TIMEOUT;   // counter for RELEASE->IDLE transition
  State state = State::IDLE;    // current input state
  Vec2<> point;
  while (1) {
    // Start with no event
    Event e{ Event::NONE, {} };

    // Determine next state from state of ILI9341
    bool touched = touchCtrl.touched();
    State next = state;
    switch (state) {
      case IDLE:    next = touched ? TOUCH : state; break;
      case TOUCH:   next = touched ? state : RELEASE; break;
      case RELEASE: next = touched ? TOUCH : state; break;
      default: state = IDLE; break;
    }

    // Update point while touch is ongoing
    if (touched) { point = getPoint(); }

    // If state would change do some extra checks
    if (state != next) {
      // We are leaving IDLE: trigger TOUCH event
      if (state == IDLE) { sendEvent({ Event::TOUCH,point }); }
      // We are entering RELEASE: reset countdown timer
      if (next == RELEASE) { counter = TIMEOUT; }
    } else {
      // While in RELEASE: trigger RELEASE on timeout
      if (state == RELEASE && !counter--) {
        sendEvent({ Event::RELEASE,point });
        next = IDLE;
      }
    }

    state = next;
    OSTimeDly(10);
  }
}

/************************************************************************************

   Runs MP3 demo code

************************************************************************************/
void Mp3StreamTask(void* pdata)
{
    char buf[BUFSIZE];
    PrintWithBuf(buf, BUFSIZE, "Mp3StreamTask: starting\n");

    HANDLE hMp3;
    InitializeMP3(hMp3);

    while (1) {
      for (auto it = songs.begin(); it != songs.end(); ++it) {
        const char* filename = (*it)->filename.c_str();
        PrintWithBuf(buf, sizeof(buf), "Begin streaming sound file: %s\n", filename);
        Mp3StreamSDFile(hMp3, filename);
        OSTimeDly(500);
      }
    }
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
      songs.insert(songs.begin(), song);
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

uint8_t bitmapHeapArr[64*64*4];
OS_MEM* bitmapHeap = NULL;

Bitmap loadBitmap(const char* filename) {
  INT8U uCOSerr;
  if (bitmapHeap == NULL) {
    bitmapHeap = OSMemCreate(bitmapHeapArr, 4, 64*64, &uCOSerr);
    if (uCOSerr != OS_ERR_NONE) while (1);
  }
  Bitmap out;
  int ret;

  auto file = SD.open(filename);

  // look for Netpbm PGM magic
  uint16_t magic;
  ret = file.read(&magic, sizeof(magic));
  magic = ntoh(magic);
  if (magic != 20533) return out;

  auto extractDecimalNumber = [&](uint32_t* number) {
    int c;

    // ignore whitespace
    for (c = file.peek(); isspace(c); c = file.read());

    // count digits
    uint32_t start, end;
    start = file.position();
    for (c = file.peek(); isdigit(c); c = file.read());
    end = file.position();

    // convert ascii to uint32_t
    char data[11]; // enough for 32-bit decimal (plus \0)
    memset(data, 0, sizeof(data));
    file.seek(start-1);
    ret = file.read(data, std::min(end-start, sizeof(data)-1));
    // TODO ret
    *number = atoi(data);
  };

  // extract header values
  uint32_t width, height, maxval;
  extractDecimalNumber(&width);
  extractDecimalNumber(&height);
  extractDecimalNumber(&maxval);

  // I don't support 16-bit PGM due to laziness
  assert(maxval < 256);

  // single whitespace between header and the promised data
  assert(isspace(file.read()));

#if 1
  char buf[80];
  PrintWithBuf(buf, sizeof(buf), "%s\n  width: %d\n  height: %d\n  maxval: %d\n",
               filename, width, height, maxval);
#endif

  auto data = (uint8_t*)OSMemGet(bitmapHeap, &uCOSerr);
  if (uCOSerr != OS_ERR_NONE) while (1);
  int i = 0;
  for (int h = 0; h < height; ++h) {
    for (int w = 0; w < width; ++w) {
      data[i++] = file.read();
    }
  }

  out.setBitmap(data, { width,height }); // WARNING data is lost to the ether!
  file.close();
  return out;
}

