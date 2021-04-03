/*
 * M5StickC YouTube broadcast notice App
 *
 * Copyright (c) 2021 Koya Aoki
 * 
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <Arduino.h>
#include <M5StickC.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Ticker.h"
#include "YoutubeApi.h"
#include "fullColorLedDriver.hpp"

static void shutdown(void);
static void reserveShutdown(void);
static time_t getNowSec(void);
static void debugPrintTime(const time_t &time);
static void updateNextTime();
static tm getCloser(tm &one, tm &two);
static void handleShutdown();
static int getDiffHourFromNow(tm &nextTime);
static void updateLeds();

#define UTC          0
#define API_KEY      "your-API-key"
#define PIN_SHUTDOWN 36
#define CHANNEL_NUM  12
#define NODATA       -9
#define NOW          -1
#define LED_ON_MS    1000

FullColorLedDriver *fullColorLedDriver;

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);

typedef struct channel {
  String id;
  int nextStreamTime;
  Color color;
}Channel;

typedef struct ledStatus {
  unsigned long nextChangeTime;
}LedStatus;

Channel channels[CHANNEL_NUM] {
  {"UC0Owc36U9lOyi9Gx9Ic-4qg", NODATA, {200, 70, 10}},
  {"UC2kyQhzGOB-JPgcQX9OMgEw", NODATA, {106, 191, 242}},
  {"UCRvpMpzAXBRKJQuk-8-Sdvg", NODATA, {65, 200, 55}},
  {"UCW8WKciBixmaqaGqrlTITRQ", NODATA, {220, 50, 50}},
  {"UCXp7sNC0F_qkjickvlYkg-Q", NODATA, {204, 20, 8}},
  {"UCtzCQnCT9E4o6U3mHHSHbQQ", NODATA, {200, 70, 70}},
  {"UC_BlXOQe5OcRC7o0GX8kp8A", NODATA, {70, 180, 100}},
  {"UCFsWaTQ7kT76jNNGeGIKNSA", NODATA, {200, 20, 20}},
  {"UC_WOBIopwUih0rytRnr_1Ag", NODATA, {250, 10, 10}},
  {"UCqskJ0nmw-_eweWfsKvbrzQ", NODATA, {200, 70, 30}},
  {"UC3xG1XWzAKt5dxSxktJvtxg", NODATA, {36, 122, 142}},
  {"UC4PrHgUcAtOoj_LKmUL-uLQ", NODATA, {179, 163, 50}}
};
time_t lastUpdateTime = 0;

// wifiのSSIDとパスワードを代入
const char* ssid       = "your-ssid";
const char* password   = "your-password";

static bool do_shutdown = false;
Ticker shutdownHandler;
static LedStatus ledStatuses[CHANNEL_NUM] {};

void setup() {
  M5.begin();
  M5.Axp.ScreenBreath(10);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  
  pinMode(PIN_SHUTDOWN, INPUT);

  Wire.begin(26, 0);

  fullColorLedDriver = new FullColorLedDriver();

  Wire.setClock(400000);

  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    retry_count++;
    if (retry_count > 20) {
      Serial.println("restart!");
      ESP.restart();
    }
    delay(500);
  }
  Serial.printf("Connected to %s\n", ssid);

  configTime(UTC, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

  attachInterrupt(PIN_SHUTDOWN, reserveShutdown, FALLING);
  shutdownHandler.attach(5, handleShutdown);

  // TODO: 次回配信時間取得
  updateNextTime();
  lastUpdateTime = getNowSec();
  debugPrintTime(lastUpdateTime);

  int id = 0;
  unsigned long now = millis();
  for (auto channel: channels) {
    Serial.printf("[%d] nextStreamTimeH: %d\n", id, channel.nextStreamTime);
    if (channel.nextStreamTime == NOW || channel.nextStreamTime >= 0) {
      Serial.printf("led ON, id = %d\n", id);
      fullColorLedDriver->drive(id, channel.color);
      ledStatuses[id].nextChangeTime = now + LED_ON_MS;
    } else {
      Serial.printf("led off, id = %d\n", id);
      fullColorLedDriver->drive(id, {0, 0, 0});
      //fullColorLedDriver->drive(id, channel.color);
    }
    id++;
  }
}

void tri(int i) {
  Color c = {0, 0, 0};
  for (int f = 0; f < 256; f++) {
    c.red = f;
    fullColorLedDriver->drive(i, c);
    delay(10);
  }
  c = {0, 0, 0};
  for (int f = 0; f < 256; f++) {
    c.green = f;
    fullColorLedDriver->drive(i, c);
    delay(10);
  }
  c = {0, 0, 0};
  for (int f = 0; f < 256; f++) {
    c.blue = f;
    fullColorLedDriver->drive(i, c);
    delay(10);
  }
}

void loop() {
  M5.update();
  updateLeds();
}

static void reserveShutdown(void){
  Serial.println("reserve shutdown");
  do_shutdown = true;
}

static void shutdown(void){
  do_shutdown = false;
  Serial.println("shutdown!");
  M5.Axp.PowerOff();
}

static time_t getNowSec(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return tv.tv_sec;
}

static void debugPrintTime(const time_t &time) {
  char date[64];
  strftime(date, sizeof(date), "%Y/%m/%d %a %H:%M:%S", localtime(&time));
  Serial.printf("lastUpdateTime: %s\n", date);
}

static void updateNextTime() {
  for (auto &channel: channels) {
    Serial.println("--- chennal: " + channel.id + " ---");
    std::vector<String> videos = api.getUpcomingBroadcasts(channel.id);
    struct tm nextStreamTime;
    Serial.printf("video count: %d\n", videos.size());
    bool hasUpdate = false;
    for (int i = 0; i < videos.size(); i++) {
      String video = videos[i];
      Serial.println("video id: " + video);
      BroadcastDetails detail = api.getBroadcastDetails(video);
      Serial.println(" schedual start time: " + detail.schedualStartTime);
      Serial.println(" actual end time: " + detail.actualEndTime);
      if (detail.actualEndTime != "null") {
        continue;
      }
      struct tm gotTime;
      char scheduledTimeChar[50];
      detail.schedualStartTime.toCharArray(scheduledTimeChar, 50);
      strptime(scheduledTimeChar, "%Y-%m-%dT%H:%M:%SZ", &gotTime);
      if (i == 0) {
        nextStreamTime = gotTime;
      } else {
        nextStreamTime = getCloser(nextStreamTime, gotTime);
      }
      hasUpdate = true;
    }
    if (hasUpdate) {
      int nextStreamTimeH = getDiffHourFromNow(nextStreamTime);
      Serial.printf("diff hour: %d\n", nextStreamTimeH);
      if (nextStreamTimeH < 0) {
        channel.nextStreamTime = NOW;
      } else {
        channel.nextStreamTime = nextStreamTimeH;
      }
      Serial.printf("channel next stream time: %d\n", channel.nextStreamTime);
    }
  }
}

static int getDiffHourFromNow(tm &nextTime) {
  time_t next = mktime(&nextTime);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return (next - tv.tv_sec) / 3600;
}

static tm getCloser(tm &one, tm &two) {
  time_t one_t;
  time_t two_t;
  one_t = mktime(&one);
  two_t = mktime(&two);
  if ( one_t > two_t ) {
    return two;
  }
  return one;
}

static void handleShutdown() {
  if(do_shutdown){
    shutdown();
  }
}

static void updateLeds() {
  unsigned long now = millis();
  for (int id = 0; id < CHANNEL_NUM; id++) {
    if (channels[id].nextStreamTime <= 0) {
      continue;
    }
    if (now < ledStatuses[id].nextChangeTime) {
      continue;
    }
    if (fullColorLedDriver->getBrightness(id) <= 0) {
      fullColorLedDriver->drive(id, channels[id].color);
      ledStatuses[id].nextChangeTime = now + LED_ON_MS;
    } else {
      fullColorLedDriver->drive(id, {0, 0, 0});
      ledStatuses[id].nextChangeTime = now + channels[id].nextStreamTime * 1000;
    }
    id++;
  }
}