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
#include "util.h"

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
void ReadMp3Files();
void DrawLcdContents();

// global song list
Heap<Song, 64> songHeap;
SongList g_songs;

// event queue
Queue<Event, 4> eventQueue;

// command queue
enum class Command { PREVIOUS, PLAY, PAUSE, NEXT };
Queue<Command, 4> commandQueue;

// song mailbox
Mailbox<Song> songMbox;
Mailbox<int> progressMbox;
Mailbox<int> durationMbox;

// Globals
bool isPlaying = false;
bool nextSong = OS_FALSE;
Bitmap play_texture;
Bitmap prev_texture;
Bitmap next_texture;
Bitmap pause_texture;
Bitmap art_texture;

/************************************************************************************

   This task is the initial task running, started by main(). It starts
   the system tick timer and creates all the other tasks. Then it deletes itself.

************************************************************************************/
void StartupTask(void* pdata)
{
  INT8U uCOSerr;

  // Start the system tick
  SetSysTick(OS_TICKS_PER_SEC);

  // Initialize hardware drivers
  InitializeSD();
  InitializeLCD(lcdCtrl);
  InitializeTouch(touchCtrl);

  // initialize heaps, queues, and mailboxes
  uCOSerr = songHeap.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);
  uCOSerr = eventQueue.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);
  uCOSerr = commandQueue.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);
  uCOSerr = songMbox.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);
  uCOSerr = progressMbox.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);
  uCOSerr = durationMbox.initialize();
  if (uCOSerr != OS_ERR_NONE) while (1);

  // Read SD card contents
  ReadMp3Files();
#ifdef DEBUG_SONG_LIST
  for (const auto& song : g_songs.list) {
    PrintWithBuf(buf, sizeof(buf), "  %s:\n", song->filename.c_str());
    PrintWithBuf(buf, sizeof(buf), "    title:    %s\n", song->info.title);
    PrintWithBuf(buf, sizeof(buf), "    artist:   %s\n", song->info.artist);
    PrintWithBuf(buf, sizeof(buf), "    album:    %s\n", song->info.album);
    PrintWithBuf(buf, sizeof(buf), "    duration: %d\n", song->duration);
  }
#endif

  // load bitmap images
  play_texture = loadBitmap("icon/play.pgm");
  pause_texture = loadBitmap("icon/pause.pgm");
  prev_texture = loadBitmap("icon/prev.pgm");
  next_texture = loadBitmap("icon/next.pgm");
  art_texture = loadBitmap("icon/music.pgm");

  // Create the test tasks
  OSStatInit();

  // The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h
  for (auto& it : tasks) {
    OSTaskCreate(it.task, it.arg, it.stack, it.priority);
  }

  // Delete ourselves, letting the work be done in the new tasks.
  OSTaskDel(OS_PRIO_SELF);
}

/************************************************************************************

   Runs LCD/Touch demo code

************************************************************************************/
void LcdDisplayTask(void* pdata)
{
  INT8U uCOSerr;

  MP3 b(lcdCtrl.width(), lcdCtrl.height());
  b.refreshLayout();
  b.setGFX(&lcdCtrl);
  lcdCtrl.fillScreen(0);

  b.controls.prev.setOnClick([](){
    commandQueue.push(Command::PREVIOUS);
  });

  b.controls.play.setOnClick([&](){
    bool paused = b.controls.play.isPressed();
    commandQueue.push(paused ? Command::PLAY : Command::PAUSE);
  });

  b.controls.next.setOnClick([](){
    commandQueue.push(Command::NEXT);
  });

  INT32U time = OSTimeGet();

  while (1) {
    // handle all events in eventQueue
    do {
      // Pop msg from eventQueue
      Event e;
      auto err = eventQueue.pop(&e);
      if (err != OS_ERR_NONE) {
#ifdef DEBUG_EVENT_QUEUE
        PrintWithBuf(buf, sizeof(buf), "eventQueue.pop ERROR: %d\n", uCOSerr);
#endif
        continue;
      }

      // Do something with the event
#ifdef DEBUG_EVENT_QUEUE
      PrintWithBuf(buf, sizeof(buf), "Receiving e: %d (%d,%d)\n", e.type, e.position.x, e.position.y);
#endif
      b.input(e);
    } while (uCOSerr == OS_ERR_NONE);

    // check for change in songMbox
    auto song = songMbox.accept();
    if (song) {
      // copy strings because mailbox may get rewritten
      static std::string title, artist, album;
      title = song->info.title;
      artist = song->info.artist;
      album = song->info.album;
      // updaate GUI elements
      b.title.setText(title.c_str());
      b.artist.setText(artist.c_str());
      b.album.setText(album.c_str());
    }

    auto duration = durationMbox.accept();
    if (duration) {
      b.playback.setDuration(*duration);
    }

    auto progress = progressMbox.accept();
    if (progress) {
      b.playback.setProgress(*progress);
    }

    INT32U next = OSTimeGet();
    INT32U delta = next - time;
    b.process(delta);
    time = next;
    OSTimeDly(20);
  }
}

void TouchInputTask(void* pdata)
{
  const unsigned TIMEOUT = 4;

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
#ifdef DEBUG_EVENT_QUEUE
    PrintWithBuf(buf, sizeof(buf), "Sending e: %d (%d,%d)\n", e.type, e.position.x, e.position.y);
#endif
    eventQueue.push(e);
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

    HANDLE hMp3;
    InitializeMP3(hMp3);
    File currentSong;

    INT32U songProgress = 0;
    songMbox.post(*g_songs.current());
    bool songChanged = true;

    while (1) {
      // handle command queue
      Command c;
      while (commandQueue.pop(&c) == OS_ERR_NONE) {
#ifdef DEBUG_COMMAND_QUEUE
        PrintWithBuf(buf, sizeof(buf), "Received command: %d\n", c);
#endif
        switch (c) {
        case Command::PREVIOUS:
          g_songs.prev();
          songChanged = true;
          break;

        case Command::PLAY:
        case Command::PAUSE:
          isPlaying = !isPlaying;
          break;

        case Command::NEXT:
          g_songs.next();
          songChanged = true;
          break;
        }
      }

      // update current song global
      if (songChanged) {
        songMbox.flush();
        songMbox.post(*g_songs.current());
        currentSong.close();
        currentSong = SD.open(g_songs.current()->filename.c_str(), O_READ);
        songProgress = 0;
        progressMbox.flush();
        progressMbox.post(songProgress);
        int duration = (currentSong.size() * 8) / 96000;
        durationMbox.flush();
        durationMbox.post(duration);
        songChanged = false;
      }

      static auto last_time = OSTimeGet();
      static bool last_playing = !isPlaying;
      if (last_playing != isPlaying) {
        if (isPlaying) last_time = OSTimeGet();
        last_playing = isPlaying;
      }

      auto elapsed = songProgress / 1000;
      static auto lastElapsed = elapsed;
      if (lastElapsed != elapsed) {
        progressMbox.flush();
        progressMbox.post(elapsed);
      }

      // stream file
      if (isPlaying) {
        // advance progress
        auto this_time = OSTimeGet();
        songProgress += this_time - last_time;
        last_time = this_time;
        // play song chunk
        auto song = g_songs.current();
        if (Mp3StreamSDFilePart(hMp3, currentSong)) {
          g_songs.next();
          songChanged = true;
          // TODO: behavior is different if repeat feature is added
        }
        OSTimeDly(1);
      } else {
        OSTimeDly(20);
      }
    }
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

size_t getDuration(const std::string& fname) {
  auto file = SD.open(fname.c_str(), O_READ);
  const int bitrate = 96000;
  size_t duration = (file.size() * 8) / bitrate;
#ifdef DEBUG_DURATION
  char buf[64];
  PrintWithBuf(buf, sizeof(buf), "  duration: %d\n", duration);
  PrintWithBuf(buf, sizeof(buf), "fname: %s\n", fname.c_str());
  PrintWithBuf(buf, sizeof(buf), "  file.size(): %d\n", file.size());
  PrintWithBuf(buf, sizeof(buf), "  bitrate: %d\n", bitrate);
#endif
  file.close();
  return duration;
}

void readId3Tag(File& file, Song* song){
  // be cautious of compiler-added padding to the Song::Info struct
  file.seek(file.size() - sizeof(Song::Info));
  // read the ID3 tag into song->info
  int s = file.read(&song->info, sizeof(Song::Info));
#define DEFAULT_SONG_DETAILS
#ifdef DEFAULT_SONG_DETAILS
  if (strlen(song->info.title) == 0) { strncpy(song->info.title, "Unknown Title", sizeof(Song::Info::title)); }
  if (strlen(song->info.artist) == 0) { strncpy(song->info.artist, "Unknown Artist", sizeof(Song::Info::artist)); }
  if (strlen(song->info.album) == 0) { strncpy(song->info.album, "Unknown Album", sizeof(Song::Info::album)); }
#endif
}

void ReadMp3FilesHelper(File dir) {
  INT8U uCOSerr;
  if (!dir) { return; }
  while (1) {
    File entry = dir.openNextFile();
    if (!entry) { break; }
    if (entry.isDirectory()) { ReadMp3FilesHelper(entry); }
    else {
      if (!strstr(entry.name(), ".MP3")) { continue; }
      auto song = songHeap.get(&uCOSerr);
      if (uCOSerr != OS_ERR_NONE) while (1);
      song->filename = std::string{ entry.name() };
      song->duration = getDuration(entry.name());
      readId3Tag(entry, song);
      g_songs.add(song);
    }
    entry.close();
  }
}

void ReadMp3Files() {
  File dir = SD.open("/");
  ReadMp3FilesHelper(dir);
  dir.close();
}

uint8_t bitmapHeapArr[64*64*5];
OS_MEM* bitmapHeap = NULL;

Bitmap loadBitmap(const char* filename) {
  INT8U uCOSerr;
  if (bitmapHeap == NULL) {
    bitmapHeap = OSMemCreate(bitmapHeapArr, 5, 64*64, &uCOSerr);
    if (uCOSerr != OS_ERR_NONE) while (1);
  }
  Bitmap out;
  auto file = SD.open(filename);

  // look for Netpbm PGM magic
  uint16_t magic;
  int ret = file.read(&magic, sizeof(magic));
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

#ifdef DEBUG_loadBitmap
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

