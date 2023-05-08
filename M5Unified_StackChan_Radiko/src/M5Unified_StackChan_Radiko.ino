//#define WIFI_SSID "SET YOUR WIFI SSID"
//#define WIFI_PASS "SET YOUR WIFI PASS"
//#define RADIKO_USER "SET YOUR MAIL-ADDRESS"
//#define RADIKO_PASS "SET YOUR PREMIUM PASS"

#define USE_SERVO
#ifdef USE_SERVO
//#define SERVO_PIN_X 13
//#define SERVO_PIN_Y 14
#define SERVO_PIN_X 33  //Core2 PORT A
#define SERVO_PIN_Y 32
#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing       
#endif

#include <math.h>
#include <WiFi.h>
#include <SD.h>
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <nvs.h>

//#define SEPARATE_DOWNLOAD_TASK
#include <WebRadio_Radiko.h>
#include <AudioOutputM5Speaker.h>

#include "Avatar.h"
#include "AtaruFace.h"
#include "RamFace.h"
#include "DannFace.h"
#include "DogFace.h"
#include "PaletteColor.h"

int BatteryLevel = -1;

static constexpr uint8_t select_pref = 0;

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;
static constexpr uint8_t m5spk_task_pinned_core = APP_CPU_NUM;

#define FFT_SIZE 256
class fft_t
{
  float _wr[FFT_SIZE + 1];
  float _wi[FFT_SIZE + 1];
  float _fr[FFT_SIZE + 1];
  float _fi[FFT_SIZE + 1];
  uint16_t _br[FFT_SIZE + 1];
  size_t _ie;

public:
  fft_t(void)
  {
#ifndef M_PI
#define M_PI 3.141592653
#endif
    _ie = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;
    for ( int i = 1 ; i < s4 ; ++i)
    {
    float f = cosf(omega * i);
      _wi[s4 + i] = f;
      _wi[s4 - i] = f;
      _wr[     i] = f;
      _wr[s2 - i] = -f;
    }
    _wi[s4] = _wr[0] = 1;

    size_t je = 1;
    _br[0] = 0;
    _br[1] = FFT_SIZE / 2;
    for ( size_t i = 0 ; i < _ie - 1 ; ++i )
    {
      _br[ je << 1 ] = _br[ je ] >> 1;
      je = je << 1;
      for ( size_t j = 1 ; j < je ; ++j )
      {
        _br[je + j] = _br[je] + _br[j];
      }
    }
  }

  void exec(const int16_t* in)
  {
    memset(_fi, 0, sizeof(_fi));
    for ( size_t j = 0 ; j < FFT_SIZE / 2 ; ++j )
    {
      float basej = 0.25 * (1.0-_wr[j]);
      size_t r = FFT_SIZE - j - 1;

      /// perform han window and stereo to mono convert.
      _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
      _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
    }

    size_t s = 1;
    size_t i = 0;
    do
    {
      size_t ke = s;
      s <<= 1;
      size_t je = FFT_SIZE / s;
      size_t j = 0;
      do
      {
        size_t k = 0;
        do
        {
          size_t l = s * j + k;
          size_t m = ke * (2 * j + 1) + k;
          size_t p = je * k;
          float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
          float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
          _fr[m] = _fr[l] - Wxmr;
          _fi[m] = _fi[l] - Wxmi;
          _fr[l] += Wxmr;
          _fi[l] += Wxmi;
        } while ( ++k < ke) ;
      } while ( ++j < je );
    } while ( ++i < _ie );
  }

  uint32_t get(size_t index)
  {
    return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[ index ] * _fr[ index ] + _fi[ index ] * _fi[ index ]) : 0u;
  }
};

static constexpr size_t WAVE_SIZE = 320;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static Radiko radio(&out, m5spk_task_pinned_core);

static fft_t fft;
static bool fft_enabled = false;
static bool wave_enabled = false;
static uint16_t prev_y[(FFT_SIZE / 2)+1];
static uint16_t peak_y[(FFT_SIZE / 2)+1];
static int16_t wave_y[WAVE_SIZE];
static int16_t wave_h[WAVE_SIZE];
static int16_t raw_data[WAVE_SIZE * 2];
static int header_height = 0;
static char stream_title[128] = { 0 };
static const char* meta_text[2] = { nullptr, stream_title };
static const size_t meta_text_num = sizeof(meta_text) / sizeof(meta_text[0]);
static uint8_t meta_mod_bits = 0;

static int px;  // draw volume bar
static int prev_level_x[2];
static int peak_level_x[2];

static uint32_t bgcolor(LGFX_Device* gfx, int y)
{
  auto h = gfx->height()/4;
  auto dh = h - header_height;
  int v = ((h - y)<<5) / dh;
  if (dh > 44)
  {
    int v2 = ((h - y - 1)<<5) / dh;
    if ((v >> 2) != (v2 >> 2))
    {
      return 0x666666u;
    }
  }
  return gfx->color888(v + 2, v, v + 6);
}

static void gfxSetup(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
//  if (gfx->width() < gfx->height())
//  {
//    gfx->setRotation(gfx->getRotation()^1);
//  }
  gfx->setFont(&fonts::lgfxJapanGothic_12);
  gfx->setEpdMode(epd_mode_t::epd_fastest);
  gfx->setTextWrap(false);
  gfx->setCursor(0, 8);
  gfx->println("WebRadio player");
  gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

  header_height = (gfx->height() > 80) ? 33 : 21;
  fft_enabled = !gfx->isEPD();
  if (fft_enabled)
  {
    wave_enabled = (gfx->getBoard() != m5gfx::board_M5UnitLCD);

    for (int y = header_height; y < gfx->height(); ++y)
    {
      gfx->drawFastHLine(0, y, gfx->width(), bgcolor(gfx, y));
    }
  }

  for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
  {
//    prev_y[x] = INT16_MAX;
    prev_y[x] = gfx->height()/4;
    peak_y[x] = INT16_MAX;
  }
  for (int x = 0; x < WAVE_SIZE; ++x)
  {
    wave_y[x] = gfx->height()/4;
    wave_h[x] = 0;
  }

  px = 0;  // draw volume bar
  prev_level_x[0] = prev_level_x[1] = 0;
  peak_level_x[0] = peak_level_x[1] = 0;

}

void gfxLoop(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
  if (header_height > 32)
  {
    if (meta_mod_bits)
    {
      gfx->startWrite();
      for (int id = 0; id < meta_text_num; ++id)
      {
        if (0 == (meta_mod_bits & (1<<id))) { continue; }
        meta_mod_bits &= ~(1<<id);
        size_t y = id * 12;
        if (y+12 >= header_height) { continue; }
        gfx->setCursor(4, 8 + y);
        gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
        gfx->print(meta_text[id]);
        gfx->print(" "); // Garbage data removal when UTF8 characters are broken in the middle.
      }
      gfx->display();
      gfx->endWrite();
    }
  }
  else
  {
    static int title_x;
    static int title_id;
    static int wait = INT16_MAX;

    if (meta_mod_bits)
    {
      if (meta_mod_bits & 1)
      {
        title_x = 4;
        title_id = 0;
        gfx->fillRect(0, 8, gfx->width(), 12, gfx->getBaseColor());
      }
      meta_mod_bits = 0;
      wait = 0;
    }

    if (--wait < 0)
    {
      int tx = title_x;
      int tid = title_id;
      wait = 3;
      gfx->startWrite();
      uint_fast8_t no_data_bits = 0;
      do
      {
        if (tx == 4) { wait = 255; }
        gfx->setCursor(tx, 8);
        const char* meta = meta_text[tid];
        if (meta[0] != 0)
        {
          gfx->print(meta);
          gfx->print("  /  ");
          tx = gfx->getCursorX();
          if (++tid == meta_text_num) { tid = 0; }
          if (tx <= 4)
          {
            title_x = tx;
            title_id = tid;
          }
        }
        else
        {
          if ((no_data_bits |= 1 << tid) == ((1 << meta_text_num) - 1))
          {
            break;
          }
          if (++tid == meta_text_num) { tid = 0; }
        }
      } while (tx < gfx->width());
      --title_x;
      gfx->display();
      gfx->endWrite();
    }
  }

  if (fft_enabled)
  {
//    static int prev_levelx[2];
//    static int peak_levelx[2];

    auto buf = out.getBuffer();
    if (buf)
    {
      memcpy(raw_data, buf, WAVE_SIZE * 2 * sizeof(int16_t)); // stereo data copy
      gfx->startWrite();

      // draw stereo level meter
      for (size_t i = 0; i < 2; ++i)
      {
        int32_t level = 0;
        for (size_t j = i; j < 640; j += 32)
        {
          uint32_t lv = abs(raw_data[j]);
          if (level < lv) { level = lv; }
        }

        int32_t x = (level * gfx->width()) / INT16_MAX;
        int32_t px = prev_level_x[i];
        if (px != x)
        {
          gfx->fillRect(x, i * 3, px - x, 2, px < x ? 0xFF9900u : 0x330000u);
          prev_level_x[i] = x;
        }
        px = peak_level_x[i];
        if (px > x)
        {
          gfx->writeFastVLine(px, i * 3, 2, TFT_BLACK);
          px--;
        }
        else
        {
          px = x;
        }
        if (peak_level_x[i] != px)
        {
          peak_level_x[i] = px;
          gfx->writeFastVLine(px, i * 3, 2, TFT_WHITE);
        }
      }
      gfx->display();

      // draw FFT level meter
      fft.exec(raw_data);
      size_t bw = gfx->width() / 60;
      if (bw < 3) { bw = 3; }
      int32_t dsp_height = gfx->height()/4;
      int32_t fft_height = dsp_height - header_height - 1;
      size_t xe = gfx->width() / bw;
      if (xe > (FFT_SIZE/2)) { xe = (FFT_SIZE/2); }
      int32_t wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[0] + raw_data[1])) * fft_height) >> 17);

      uint32_t bar_color[2] = { 0x000033u, 0x99AAFFu };

      for (size_t bx = 0; bx <= xe; ++bx)
      {
        size_t x = bx * bw;
        if ((x & 7) == 0) { gfx->display(); taskYIELD(); }
        int32_t f = fft.get(bx);
        int32_t y = (f * fft_height) >> 18;
        if (y > fft_height) { y = fft_height; }
        y = dsp_height - y;
        int32_t py = prev_y[bx];
        if (y != py)
        {
          gfx->fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
          prev_y[bx] = y;
        }
        py = peak_y[bx] + 1;
        if (py < y)
        {
          gfx->writeFastHLine(x, py - 1, bw - 1, bgcolor(gfx, py - 1));
        }
        else
        {
          py = y - 1;
        }
        if (peak_y[bx] != py)
        {
          peak_y[bx] = py;
          gfx->writeFastHLine(x, py, bw - 1, TFT_WHITE);
        }


        if (wave_enabled)
        {
          for (size_t bi = 0; bi < bw; ++bi)
          {
            size_t i = x + bi;
            if (i >= gfx->width() || i >= WAVE_SIZE) { break; }
            y = wave_y[i];
            int32_t h = wave_h[i];
            bool use_bg = (bi+1 == bw);
            if (h>0)
            { /// erase previous wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (use_bg || y < peak_y[bx]) ? bgcolor(gfx, y)
                            : (y == peak_y[bx]) ? 0xFFFFFFu
                            : bar_color[(y >= prev_y[bx])];
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
            size_t i2 = i << 1;
            int32_t y1 = wave_next;
            wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[i2] + raw_data[i2 + 1])) * fft_height) >> 17);
            int32_t y2 = wave_next;
            if (y1 > y2)
            {
              int32_t tmp = y1;
              y1 = y2;
              y2 = tmp;
            }
            y = y1;
            h = y2 + 1 - y;
            wave_y[i] = y;
            wave_h[i] = h;
            if (h>0)
            { /// draw new wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (y < prev_y[bx]) ? 0xFFCC33u : 0xFFFFFFu;
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
          }
        }
      }
      gfx->display();
      gfx->endWrite();
    }
  }

  if (!gfx->displayBusy())
  { // draw volume bar
//    static int px;
    uint8_t v = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
    int x = v * (gfx->width()) >> 8;
    if (px != x)
    {
      gfx->fillRect(x, 6, px - x, 2, px < x ? 0xAAFFAAu : 0u);
      gfx->display();
      px = x;
    }
  }
}

using namespace m5avatar;
Avatar* avatar;

#ifdef USE_SERVO
#define START_DEGREE_VALUE_X 90
//#define START_DEGREE_VALUE_Y 90
#define START_DEGREE_VALUE_Y 85 //
ServoEasing servo_x;
ServoEasing servo_y;
bool servo_home = false;
#endif
bool levelMeter = true;
bool balloon = false;

void behavior(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
   for (;;)
  {
    level = abs(*out.getBuffer());
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    if(!balloon){
      avatar->setRotation(gazeX * 5);
    } else {
      avatar->setRotation(0.0);
    }
#ifdef USE_SERVO
    if(!servo_home)
    {
        servo_x.setEaseTo(START_DEGREE_VALUE_X + (int)(20.0 * gazeX));
        if(gazeY < 0) {
          int tmp = (int)(15.0 * gazeY + open * 15.0);
          if(tmp > 15) tmp = 15;
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + tmp);
        } else {
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + (int)(10.0 * gazeY) - open * 15.0);
        }
    } else {
       servo_x.setEaseTo(START_DEGREE_VALUE_X); 
       servo_y.setEaseTo(START_DEGREE_VALUE_Y);
    }
    synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
    delay(50);
  }
}

void Servo_setup() {
#ifdef USE_SERVO
  if (servo_x.attach(SERVO_PIN_X, START_DEGREE_VALUE_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo x");
  }
  if (servo_y.attach(SERVO_PIN_Y, START_DEGREE_VALUE_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE)) {
    Serial.print("Error attaching servo y");
  }
  servo_x.setEasingType(EASE_QUADRATIC_IN_OUT);
  servo_y.setEasingType(EASE_QUADRATIC_IN_OUT);
  setSpeedForAllServos(30);
#endif
}

void Wifi_setup() {
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  M5.Display.println("WiFi begin");
  Serial.println("WiFi begin");
#if defined ( WIFI_SSID ) && defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  // 前回接続時情報で接続する
  WiFi.begin();
#endif
  // 前回接続時情報で接続する
  //M5.Display.println("WiFi begin");
  //Serial.println("WiFi begin");
  //WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    Serial.print(".");
    delay(500);
    // 10秒以上接続できなかったら抜ける
    if ( 10000 < millis() ) {
      break;
    }
  }
  M5.Display.println("");
  Serial.println("");
  // 未接続の場合にはSmartConfig待受
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    M5.Display.println("Waiting for SmartConfig");
    Serial.println("Waiting for SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      M5.Display.print("#");
      Serial.print("#");
      // 30秒以上接続できなかったら抜ける
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    // Wi-fi接続
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("Waiting for WiFi");
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Display.print(".");
      Serial.print(".");
      // 60秒以上接続できなかったら抜ける
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("WiFi Connected.");
    Serial.println("WiFi Connected.");
  }
  M5.Display.print("IP Address: ");
  Serial.print("IP Address: ");
  M5.Display.println(WiFi.localIP());
  Serial.println(WiFi.localIP());
}

Face* faces[6];
const int facesSize = sizeof(faces) / sizeof(Face*);
//int faceIdx = 1;
int16_t faceIdx = 1;
ColorPalette* cps[6];
const int cpsSize = sizeof(cps) / sizeof(ColorPalette*);
int cpsIdx = 0;
const uint16_t color_table[facesSize] = {
  TFT_BLACK,  //Default
  TFT_WHITE,  //AtaruFace
  TFT_WHITE,  //RamFace
  0xef55,     //DannFace
  TFT_WHITE,  //DogFace
  0xef55,     //DannFace
};

void Avatar_setup() {
  avatar = new Avatar();
  faces[0] = avatar->getFace();
  faces[1] = new AtaruFace();
  faces[2] = new RamFace();
  faces[3] = new DannFace();
  faces[4] = new DogFace();
  faces[5] = avatar->getFace();

  cps[0] = new ColorPalette();
  cps[1] = new ColorPalette();
  cps[2] = new ColorPalette();
  cps[3] = new ColorPalette();
  cps[4] = new ColorPalette();
  cps[5] = new ColorPalette();
  cps[1]->set(COLOR_PRIMARY, PC_BLACK);  //AtaruFace
  cps[1]->set(COLOR_SECONDARY, PC_WHITE);
  cps[1]->set(COLOR_BACKGROUND, PC_WHITE);
  cps[2]->set(COLOR_PRIMARY, PC_BLACK);  //RamFace
  cps[2]->set(COLOR_SECONDARY, PC_WHITE);
  cps[2]->set(COLOR_BACKGROUND, PC_WHITE);
  cps[3]->set(COLOR_PRIMARY, PC_BLACK); //DannFace
  cps[3]->set(COLOR_BACKGROUND, 9);
  cps[3]->set(COLOR_SECONDARY, PC_WHITE);
  cps[4]->set(COLOR_PRIMARY, PC_BLACK);  //DogFace
  cps[4]->set(COLOR_SECONDARY, PC_WHITE);
  cps[4]->set(COLOR_BACKGROUND, PC_WHITE);
  cps[5] = cps[3];

  avatar->setFace(faces[faceIdx]);
  avatar->setColorPalette(*cps[faceIdx]);
  switch (M5.getBoard())
  {
    case m5::board_t::board_M5StickCPlus:
      avatar->setScale(0.45);
      avatar->setOffset(-90, 30);
      break;
    case m5::board_t::board_M5StickC:
      avatar->setScale(0.25);
      avatar->setOffset(-120, 0);
      break;
    case m5::board_t::board_M5Stack:
    case m5::board_t::board_M5StackCore2:
    case m5::board_t::board_M5Tough:
      avatar->setScale(0.80);
      avatar->setOffset(0, 52);
      break;
    default:
      avatar->setScale(0.45);
      avatar->setOffset(-90, 30);
      break;
  }
  avatar->init(); // start drawing
  avatar->addTask(behavior, "behavior");
}
void select_face(int idx){
  avatar->setFace(faces[1]);
  avatar->setColorPalette(*cps[1]);
}

struct box_t
{
  int x;
  int y;
  int w;
  int h;
  int touch_id = -1;

  void setupBox(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w)
        && this->y <= y && y < (this->y + this->h);
  }
};

static box_t box_level;
static box_t box_servo;
static box_t box_balloon;
static box_t box_face;

void setup(void)
{
  audioLogger = &Serial;
  
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT

  M5.begin(cfg);
  M5.update();
  if(M5.BtnA.isPressed() && M5.BtnB.isPressed() && M5.BtnC.isPressed()) {
    uint32_t nvs_handle;
    if(ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      M5.Display.println("nvs_flash_ersce");
      nvs_erase_all(nvs_handle);
      delay(3000);
    }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 144000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = m5spk_task_pinned_core;
    spk_cfg.task_priority = 5;
    M5.Speaker.config(spk_cfg);
  }

  M5.Speaker.begin();
/*
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

#if defined ( WIFI_SSID ) && defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif
*/
#if defined( RADIKO_USER ) && defined( RADIKO_PASS )
  radio.setAuthorization(RADIKO_USER, RADIKO_PASS);
#endif

  Wifi_setup();

  /// settings
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
/*    /// wifi
    auto fs = SD.open("/wifi.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      int y = 0;
      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
      }
      WiFi.begin(buf, &buf[y]);
    }
*/
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      /// radiko-premium
      auto fs = SD.open("/radiko.txt", FILE_READ);
      if(fs) {
        size_t sz = fs.size();
        char buf[sz + 1];
        fs.read((uint8_t*)buf, sz);
        buf[sz] = 0;
        fs.close();
  
        int y = 0;
        for(int x = 0; x < sz; x++) {
          if(buf[x] == 0x0a || buf[x] == 0x0d)
            buf[x] = 0;
          else if (!y && x > 0 && !buf[x - 1] && buf[x])
            y = x;
        }

        nvs_set_str(nvs_handle, "radiko_user", buf);
        nvs_set_str(nvs_handle, "radiko_pass", &buf[y]);
      }
      
      nvs_close(nvs_handle);
    }
    SD.end();
  }
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("Avatar", NVS_READONLY, &nvs_handle)) {
      nvs_get_i16(nvs_handle, "faceIdx", &faceIdx);
      if(faceIdx < 0 || faceIdx >= facesSize) {
        faceIdx = 0;
      }
      nvs_close(nvs_handle);
    }
  }
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READONLY, &nvs_handle)) {
      size_t volume;
      nvs_get_u32(nvs_handle, "volume", &volume);
      M5.Speaker.setVolume(volume);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);

      size_t length1;
      size_t length2;
      if(ESP_OK == nvs_get_str(nvs_handle, "radiko_user", nullptr, &length1) && ESP_OK == nvs_get_str(nvs_handle, "radiko_pass", nullptr, &length2) && length1 && length2) {
        char user[length1 + 1];
        char pass[length2 + 1];
        if(ESP_OK == nvs_get_str(nvs_handle, "radiko_user", user, &length1) && ESP_OK == nvs_get_str(nvs_handle, "radiko_pass", pass, &length2)) {
          M5.Display.print("premium member: ");
          M5.Display.println(user);
          radio.setAuthorization(user, pass);
        }
      }
      nvs_close(nvs_handle);
    }
  }
/*
  // Try forever
  M5.Display.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    delay(100);
  }
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());  
*/
  M5.Display.clear();

  gfxSetup(&M5.Display);
  M5.Display.fillRect(0, M5.Display.height()/4+1, M5.Display.width(), M5.Display.height(), color_table[faceIdx]); //

  Servo_setup();

// radiko
  radio.onPlay = [](const char * station_name, const size_t station_idx) {
    Serial.printf("onPlay:%d %s", station_idx, station_name);
    Serial.println();
    meta_text[0] = station_name;
    stream_title[0] = 0;
    meta_mod_bits = 3;
  };
  radio.onInfo = [](const char *text) {
    Serial.println(text);
  };
  radio.onError = [](const char * text) {
    Serial.println(text);
  };
  radio.onProgram = [](const char * program_title) {
    strcpy(stream_title, program_title);
    meta_mod_bits |= 2;
  };
#ifdef SYSLOG_TO
  radio.setSyslog(SYSLOG_TO);
#endif
  if(!radio.begin()) {
    Serial.println("failed: radio.begin()");
    for(;;);
  } 
  radio.play();

  Avatar_setup();
  box_level.setupBox(0, 0, 320, 60);
  box_servo.setupBox(80, 120, 80, 80);
  box_balloon.setupBox(0, 160, M5.Display.width(), 80);
  box_face.setupBox(280, 100, 40, 60);
}

void loop(void)
{
  static unsigned long long saveSettings = 0;
  radio.handle();
  if(levelMeter) gfxLoop(&M5.Display);
  avatar->draw();
  if(!levelMeter && balloon)   avatar->setSpeechText(meta_text[0]);

  {
    static int prev_frame;
    int frame;
    do
    {
      delay(1);
    } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
    prev_frame = frame;
  }
 
  static int lastms = 0;
  if (millis()-lastms > 1000) {
    lastms = millis();
    BatteryLevel = M5.Power.getBatteryLevel();
//    printf("%d\n\r",BatVoltage);
   }

  M5.update();
  auto count = M5.Touch.getCount();
  if (count)
  {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())
    {    
      if (box_level.contain(t.x, t.y))
      {
        levelMeter = !levelMeter;
        if(levelMeter)
        {
          M5.Display.clear();
          gfxSetup(&M5.Display);
          M5.Display.fillRect(0, M5.Display.height()/4+1, M5.Display.width(), M5.Display.height(), color_table[faceIdx]); //
          avatar->setScale(0.80);
          avatar->setOffset(0, 52);
          if(balloon) {
            avatar->setSpeechText("");
            balloon = false;
          }
       } else {
          M5.Display.fillScreen(color_table[faceIdx]); //
          avatar->setScale(1.0);
          avatar->setOffset(0, 0);
        }
        M5.Speaker.tone(1000, 100);
      }
#ifdef USE_SERVO
      if (box_servo.contain(t.x, t.y))
      {
        servo_home = !servo_home;
        M5.Speaker.tone(1000, 100);
      }
#endif
      if (box_balloon.contain(t.x, t.y) && !levelMeter)
      {
        balloon = !balloon;
        if(!balloon) avatar->setSpeechText("");
        M5.Speaker.tone(1000, 100);
      }
      if (box_face.contain(t.x, t.y))
      {
        faceIdx = (faceIdx + 1) % facesSize;
        if(levelMeter)
        {
          M5.Display.fillRect(0, M5.Display.height()/4+1, M5.Display.width(), M5.Display.height(), color_table[faceIdx]); //Dann
        } else {
          M5.Display.fillScreen(color_table[faceIdx]); //Dann
        }
        avatar->setFace(faces[faceIdx]);
        avatar->setColorPalette(*cps[faceIdx]);
        {
          uint32_t nvs_handle;
          if (ESP_OK == nvs_open("Avatar", NVS_READWRITE, &nvs_handle)) {
            nvs_set_i16(nvs_handle, "faceIdx", faceIdx);
            nvs_close(nvs_handle);
          }
        }
        M5.Speaker.tone(1000, 100);
      }
    }
  }
  if (M5.BtnA.wasPressed())
  {
    M5.Speaker.tone(440, 50);
  }
  if (M5.BtnA.wasDeciedClickCount())
  {
    switch (M5.BtnA.getClickCount())
    {
    case 1:
      M5.Speaker.tone(1000, 100);
      radio.play(true);
      break;

    case 2:
      M5.Speaker.tone(800, 100);
      radio.play(false);
      break;
    }
  }
  if (M5.BtnA.isHolding() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
//  if (M5.BtnA.isHolding() || M5.BtnB.isPressed())
  {
    size_t v = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
    int add = (M5.BtnB.isPressed()) ? -1 : 1;
    if (M5.BtnA.isHolding())
    {
      add = M5.BtnA.getClickCount() ? -1 : 1;
    }
    v += add;
    if (v <= 255)
    {
      M5.Speaker.setVolume(v);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, v);
      saveSettings = millis() + 5000;
    }
  }

  if (saveSettings > 0 && millis() > saveSettings)
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      size_t volume = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
      nvs_set_u32(nvs_handle, "volume", volume);
      nvs_close(nvs_handle);
    }
    saveSettings = 0;
  }
}