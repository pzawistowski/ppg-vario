#include <M5StickCPlus2.h>
#include <TFT_eSPI.h>
#include <ESP32Time.h>
#include <EEPROM.h>
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#include "Noto.h"
#include "smallFont.h"
#include "middleFont.h"
#include "bigFont.h"
#include "secFont.h"
#include "Arduino.h"
#include "ppg.h"

// main.h - macros
#define LED 36     // LED indicator pin (built-in LED)
#define TRIGGER 37 // the button pin / D6 on wemos D1 mini
#define NUM_ELEM(x) (sizeof(x) / sizeof(*(x)));

#define DEBUG 0
#define DEBUGP(x) \
  if (DEBUG == 1) \
  {               \
    x;            \
  }

#define NOP __asm__ __volatile__("nop")
#define DELAY_CNT 25

// end - main.h - macros

void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code);
void delay_ten_us(uint16_t us);
uint8_t read_bits(uint8_t count);
uint16_t rawData[300];

#define putstring_nl(s) Serial.println(s)
#define putstring(s) Serial.print(s)
#define putnum_ud(n) Serial.print(n, DEC)
#define putnum_uh(n) Serial.print(n, HEX)

#define MAX_WAIT_TIME 65535 // tens of us (ie: 655.350ms)

extern uint8_t num_NAcodes, num_EUcodes;

uint8_t bitsleft_r = 0;
uint8_t bits_r = 0;
uint8_t code_ptr;

ESP32Time rtc(0);
long timerStartEpoch = rtc.getEpoch();
#define EEPROM_SIZE 4

// colors
unsigned short grays[12];
#define blue 0x1314
#define blue2 0x022F
unsigned short tmpColor = 0;

double rad = 0.01745;
float x[10];
float y[10];
float px[10];
float py[10];

float x2[10];
float y2[10];
float px2[10];
float py2[10];

float x3[61];
float y3[61];
float px3[61];
float py3[61];

int r = 8;
int sx = 120;
int sy = 84;

int r2 = 16;
int sx2 = 23;
int sy2 = 178;

int r3 = 21;
int sx3 = 65;
int sy3 = 98;

int poz = 0;

// time variables
String h, m, s;
int day, month;
String months[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// settime variables
bool configMode = false;
int setData[8]; // setHour,setMin,setSec,setDate,setMonth,setYear; SET BEEPER, SET REGION;
String setDataLbl[8] = {"HOUR", "MIN", "SEC", "DATE", "MON", "YEAR", "SOUND", "NA"};
int setMin[8] = {0, 0, 0, 1, 1, 24, 0, 0};
int setMax[8] = {24, 60, 60, 32, 13, 36, 2, 2};
int setPosX[8] = {10, 50, 91, 10, 50, 91, 8, 8};
int setPosY[8] = {54, 54, 54, 124, 124, 124, 172, 192};
int chosen = 0;

// brightness and battery
int brightnes[6] = {16, 32, 48, 64, 96, 180};
int b = 2;
int vol;
int volE;

// region and buzze and ir
int buzzer = 0;
bool sendIR = false;

// stopwatch variable
int fase = 0; // 0=0 1=start 2=stop

// sleep variables
int sleepTime = 10;
int ts, tts = 0;
bool slp = false;

#define BUTTON_PRESSED LOW
#define BUTTON_RELEASED HIGH

uint16_t ontime, offtime;
uint8_t i, num_codes;
uint8_t region;

void setup() // ##############  SETUP ##################
{

  pinMode(35, INPUT_PULLUP);
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  // StickCP2.Rtc.setDateTime( { { 2024, 2, 17 }, { 8, 20, 0 } } );
  timerStartEpoch = rtc.getEpoch();
  pinMode(LED, OUTPUT);
  pinMode(TRIGGER, INPUT_PULLUP);

  delay_ten_us(5000);

  EEPROM.begin(EEPROM_SIZE);
  b = EEPROM.read(0);
  if (b > 7)
    b = 2;

  // buzzer = EEPROM.read(1);
  // if (buzzer > 1)
    // buzzer = 0;

  StickCP2.Display.setBrightness(brightnes[b]);

  sprite.createSprite(240, 135);
  sprite.setSwapBytes(true);

  for (int i = 0; i < 8; i++)
  {
    x[i] = ((r - 1) * cos(rad * (i * 45))) + sx;
    y[i] = ((r - 1) * sin(rad * (i * 45))) + sy;
    px[i] = (r * cos(rad * (i * 45))) + sx;
    py[i] = (r * sin(rad * (i * 45))) + sy;
  }

  for (int i = 0; i < 10; i++)
  {
    x2[i] = ((r2 - 3) * cos(rad * (i * 36))) + sx2;
    y2[i] = ((r2 - 3) * sin(rad * (i * 36))) + sy2;
    px2[i] = (r2 * cos(rad * (i * 36))) + sx2;
    py2[i] = (r2 * sin(rad * (i * 36))) + sy2;
  }

  int an = 270;
  for (int i = 0; i < 60; i++)
  {
    x3[i] = ((r3 - 6) * cos(rad * an)) + sx3;
    y3[i] = ((r3 - 6) * sin(rad * an)) + sy3;
    px3[i] = (r3 * cos(rad * an)) + sx3;
    py3[i] = (r3 * sin(rad * an)) + sy3;
    an = an + 6;
    if (an >= 360)
      an = 0;
  }

  int co = 216;
  for (int i = 0; i < 12; i++)
  {
    grays[i] = tft.color565(co, co, co);
    co = co - 20;
  }
}

void drawMain()
{

  sprite.fillSprite(TFT_BLACK);
  sprite.setTextDatum(4);

  
  // sprite.fillSmoothRoundRect(46, 158, 84, 75, 2, grays[7], TFT_BLACK);
  // sprite.fillSmoothRoundRect(48, 160, 80, 71, 2, TFT_BLACK, grays[7]);
  // sprite.fillSmoothRoundRect(52, 209, 72, 18, 2, blue2, TFT_BLACK);
  // sprite.fillSmoothRoundRect(6, 214, 34, 18, 2, TFT_ORANGE, TFT_BLACK);

  // // seconds %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // sprite.loadFont(secFont);
  // sprite.setTextColor(grays[5], TFT_BLACK);

  // for (int i = 0; i < 60; i++)
  //   if (i % 5 == 0)
  //     sprite.fillSmoothCircle(px3[i], py3[i], 1, TFT_ORANGE, TFT_BLACK);
  // sprite.drawWedgeLine(sx3, sy3, x3[s.toInt()], y3[s.toInt()], 4, 1, TFT_ORANGE, TFT_BLACK);
  // sprite.fillSmoothCircle(sx3, sy3, 2, TFT_BLACK, TFT_ORANGE);

  // sprite.unloadFont();

  // // date #########################################################
  // sprite.loadFont(smallFont);
  // sprite.setTextColor(0x35D7, TFT_BLACK);
  // sprite.drawString(months[month - 1], 22, 84);
  // sprite.fillRect(12, 94, 22, 30, grays[8]);
  // sprite.setTextColor(TFT_ORANGE, grays[8]);
  // sprite.drawString(String(day), 22, 110);
  // sprite.setTextColor(grays[3], TFT_BLACK);
  // sprite.drawString(String(sleepTime), sx2, sy2 + 2);
  // sprite.unloadFont();

 

  // time ##########################
  drawTime();

  sprite.loadFont(Noto);

  // //////brightness region
  // sprite.fillSmoothRoundRect(110, 70, 20, 58, 2, blue, TFT_BLACK);
  // sprite.fillSmoothCircle(sx, sy, 5, grays[1], blue);
  // for (int i = 0; i < 8; i++)
  //   sprite.drawWedgeLine(px[i], py[i], x[i], y[i], 1, 1, grays[1], blue);
  // for (int i = 0; i < b + 1; i++)
  //   sprite.fillRect(115, 122 - (i * 5), 10, 3, grays[1]);

  // for (int i = 0; i < 10; i++)
  //   sprite.drawWedgeLine(px2[i], py2[i], x2[i], y2[i], 2, 2, 0x7000, TFT_BLACK);

  // for (int i = 0; i < sleepTime; i++)
  //   sprite.drawWedgeLine(px2[i], py2[i], x2[i], y2[i], 2, 2, TFT_RED, TFT_BLACK);

  // // butons
  // sprite.setTextColor(TFT_BLACK, TFT_ORANGE);
  // sprite.drawString("SET", 22, 225);
  // sprite.setTextColor(grays[1], blue2);
  // sprite.drawString("STA/STP", 87, 220);
  // sprite.setTextColor(grays[3], TFT_BLACK);
  sprite.unloadFont();

  // batery region
  drawBattery(200, 4);
  // // region

  
  // // buzzer
  // sprite.setTextColor(TFT_ORANGE, TFT_BLACK);
  // sprite.drawString("BEEP", 110, 134);
  // if (buzzer)
  //   sprite.drawString("ON", 110, 146);
  // else
  //   sprite.drawString("OFF", 110, 146);

  // sprite.setTextColor(grays[6], TFT_BLACK);
  // sprite.drawString("VOLOS YT", 55, 198);
  // sprite.setTextColor(grays[6], TFT_BLACK);
  // sprite.drawString(String(s), 88, 116);

  // sprite.fillTriangle(94, 142, 104, 135, 104, 151, TFT_ORANGE);
  // sprite.fillRect(94, 139, 8, 9, TFT_ORANGE);

  // sprite.fillRect(36, 134, 3, 5, TFT_RED);
  // for (int i = 0; i < 10; i++)
  //   if (i == poz)
  //     sprite.fillRect(30 + (i * 6), 142, 3, 6, TFT_RED);
  //   else
  //     sprite.fillRect(30 + (i * 6), 142, 3, 6, grays[6]);

  pushSprite();
}

void drawTime()
{
  int rectX = 6;
  // int rectWidth = 98;
  int rectWidth = 120;
  int midX = (rectX + rectWidth)/2;
  int border = 2;
  sprite.fillSmoothRoundRect(rectX, 6, rectWidth, 124, grays[7], TFT_BLACK);
  sprite.fillSmoothRoundRect(rectX+border, 8, rectWidth-2*border, 120, 2, TFT_BLACK, grays[7]);

  sprite.loadFont(bigFont);
  sprite.setTextDatum(MC_DATUM);

  sprite.setTextColor(grays[0], TFT_BLACK);
  int textRectHeight = 52;
  sprite.drawString(h + ":" + m, midX, textRectHeight);
  // Flight time
  
  int diff = rtc.getEpoch() - timerStartEpoch;
  String flightTime = formatTime(diff / 3600) + ":" + formatTime(diff / 60 % 60) + ":" + formatTime(diff % 60);
  sprite.drawString(flightTime, midX, 2*textRectHeight+2*border);
  sprite.unloadFont();

  
  sprite.setTextDatum(TC_DATUM);
  sprite.setTextColor(grays[4], TFT_BLACK);
  sprite.drawString("TIME", midX, 14);
  sprite.drawString("FLIGHT TIME", midX, 72);
}

void pushSprite()
{
  StickCP2.Display.setRotation(1);
  StickCP2.Display.pushImage(0, 0, 240, 135, (uint16_t *)sprite.getPointer());
  StickCP2.Display.setRotation(0);
}

void drawBattery(int topX, int topY)
{
  // sprite.setTextColor(grays[5], TFT_BLACK);
  // sprite.drawString(String(vol / 1000.00), topX+18, topY+20);
  sprite.drawRect(topX+3, topY, 28, 14, grays[2]);
  sprite.fillRect(topX, topY+4, 3, 6, grays[2]);
  for (int i = 0; i < volE; i++)
    sprite.fillRect(topX+26 - (i * 5), topY+2, 3, 10, TFT_GREEN);
}


void drawSet()
{
  // sprite.fillSprite(TFT_BLACK);
  sprite.fillSprite(TFT_NAVY);
  // sprite.setTextDatum(4);
  // sprite.fillSmoothRoundRect(48, 214, 76, 18, 2, blue2, TFT_BLACK);
  // sprite.fillSmoothRoundRect(6, 214, 34, 18, 2, TFT_ORANGE, TFT_BLACK);
  // sprite.fillSmoothRoundRect(91, 190, 34, 18, 2, TFT_ORANGE, TFT_BLACK);

  // sprite.loadFont(Noto);
  // sprite.setTextColor(TFT_BLACK, TFT_ORANGE);
  // sprite.drawString("OK", 22, 225);
  // sprite.drawString("CNG", 108, 200);
  // sprite.setTextColor(grays[1], blue2);
  // sprite.drawString("SET", 87, 225);
  // sprite.unloadFont();

  // sprite.loadFont(smallFont);
  // sprite.setTextColor(TFT_ORANGE, TFT_BLACK);
  // sprite.drawString("SETUP", 28, 18);
  // sprite.unloadFont();

  // for (int i = 0; i < 7; i++)
  // {
  //   if (chosen == i)
  //     tmpColor = grays[5];
  //   else
  //     tmpColor = grays[9];

  //   if (i < 6)
  //   {
  //     sprite.fillRect(setPosX[i], setPosY[i], 36, 40, tmpColor);
  //     sprite.loadFont(smallFont);
  //     sprite.setTextColor(0x35D7, TFT_BLACK);
  //     sprite.drawString(setDataLbl[i], setPosX[i] + 18, setPosY[i] - 12);
  //     sprite.unloadFont();

  //     sprite.loadFont(secFont);
  //     sprite.setTextColor(grays[1], tmpColor);
  //     sprite.drawString(String(setData[i]), setPosX[i] + 17, setPosY[i] + 22);
  //     sprite.unloadFont();
  //   }
  //   else
  //   {
  //     sprite.setTextDatum(0);
  //     sprite.fillRect(setPosX[i], setPosY[i], 76, 16, tmpColor);
  //     sprite.setTextColor(grays[1], tmpColor);
  //     sprite.drawString(setDataLbl[i], setPosX[i] + 4, setPosY[i] + 5);
  //     if (i == 6)
  //       if (setData[i] == 0)
  //         sprite.drawString("OFF", setPosX[i] + 50, setPosY[i] + 5);
  //       else
  //         sprite.drawString("ON", setPosX[i] + 50, setPosY[i] + 5);
  //   }
  // }

  pushSprite();
}

void loop() // ##########################  LOOP  ##################################
{
  StickCP2.update();
  if (fase == 0)
  {
    timerStartEpoch = rtc.getEpoch();        
  }
  if (configMode)
  {
    configScreen();    
  }
  else
  {
    mainScreen();
  }
}

void mainScreen()
{
  auto imu_update = StickCP2.Imu.update();
  auto data = StickCP2.Imu.getImuData();
  poz = map(data.accel.y * 100, -30, 100, 0, 10);

  vol = StickCP2.Power.getBatteryVoltage();
  volE = map(vol, 3000, 4180, 0, 5);
  auto dt = StickCP2.Rtc.getDateTime();

  if (digitalRead(35) == 0)
  {
    configMode = !configMode;
    setData[0] = dt.time.hours;
    setData[1] = dt.time.minutes;
    setData[2] = dt.time.seconds;
    setData[3] = dt.date.date;
    setData[4] = dt.date.month;
    setData[5] = dt.date.year - 2000;
    setData[6] = buzzer;
    setData[7] = 0;

    if (buzzer)
      StickCP2.Speaker.tone(6000, 100);
    delay(200);
  }

  if (slp)
  {
    StickCP2.Display.setBrightness(brightnes[b]);
    slp = false;
    fase = -1;
    sleepTime = 10;
  }

  if (StickCP2.BtnB.wasPressed())
  {
    sleepTime = 10;
    b++;
    if (b == 6)
      b = 0;
    EEPROM.write(0, b);
    EEPROM.commit();
    StickCP2.Display.setBrightness(brightnes[b]);
    if (buzzer)
      StickCP2.Speaker.tone(6000, 100);
  }

  if (StickCP2.BtnA.wasPressed())
  {
    sleepTime = 10;
    if (fase == 0 && poz == 1)
    {
    }
    else
    {
      fase++;
      if (fase == 3)
        fase = 0;
      if (fase == 1)
        timerStartEpoch = rtc.getEpoch();
      
      if (buzzer)
        StickCP2.Speaker.tone(6000, 100);
    }
  }

  if (dt.time.seconds < 10)
    s = "0" + String(dt.time.seconds);
  else
    s = String(dt.time.seconds);
  if (dt.time.minutes < 10)
    m = "0" + String(dt.time.minutes);
  else
    m = String(dt.time.minutes);

  if (dt.time.hours < 10)
    h = "0" + String(dt.time.hours);
  else
    h = String(dt.time.hours);

  day = dt.date.date;
  month = dt.date.month;

  ts = dt.time.seconds;
  if (tts != ts && fase == 0)
  {
    sleepTime--;
    tts = ts;
  }

  if (sleepTime == 0 && fase != 1)
  {
    slp = true;
    StickCP2.Display.setBrightness(0);
    delay(20);
    StickCP2.Power.lightSleep();
  }

  drawMain();
}

String formatTime(int time)
{
  if (time < 10)
    return "0" + String(time);
  else
    return String(time);
}

void configScreen()
{
  if (StickCP2.BtnB.wasPressed())
  {
    chosen++;
    if (chosen == 8)
      chosen = 0;
    if (buzzer)
      StickCP2.Speaker.tone(6000, 100);
  }

  if (StickCP2.BtnA.wasPressed())
  {
    setData[chosen]++;
    if (setData[chosen] == setMax[chosen])
      setData[chosen] = setMin[chosen];
    if (buzzer)
      StickCP2.Speaker.tone(6000, 100);
  }

  if (digitalRead(35) == 0)
  {
    buzzer = setData[6];
    EEPROM.write(1, buzzer);
    EEPROM.commit();
    sleepTime = 10;
    configMode = !configMode;

    StickCP2.Rtc.setDateTime({{setData[5] + 2000, setData[4], setData[3]}, {setData[0], setData[1], setData[2]}});

    if (buzzer)
      StickCP2.Speaker.tone(6000, 100);
    delay(300);
  }

  drawSet();
}
void delay_ten_us(uint16_t us)
{
  uint8_t timer;
  while (us != 0)
  {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer = 0; timer <= DELAY_CNT; timer++)
    {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}
