/*
 * PCA9685 full color led driver
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

#ifndef FULL_COLOR_LED_DRIVER_HPP
#define FULL_COLOR_LED_DRIVER_HPP

#include <Adafruit_PWMServoDriver.h>

struct Color {
  char red;
  char green;
  char blue;
};

class FullColorLedDriver {
public:
  FullColorLedDriver(){
    _initialize_pca();
  };
  bool drive(int channel, Color color){
    if (channel < 0 || channel >= ledNum){
      return false;
    }
    _pca[_channels[channel][0]].setPWM(_channels[channel][1], 0, map(color.red, 0, 255, 0, 4095));
    _pca[_channels[channel][0]].setPWM(_channels[channel][2], 0, map(color.green, 0, 255, 0, 4095));
    _pca[_channels[channel][0]].setPWM(_channels[channel][3], 0, map(color.blue, 0, 255, 0, 4095));
    brightness[channel] = calcBrightness(color);
    return true;
  };
  int getBrightness(int channel){
    if (channel < 0 || channel >= ledNum){
      return 0;
    }
    return brightness[channel];
  }
private:
  static constexpr int ledNum = 15;
  // 0 ~ 100
  int brightness[ledNum] = {};
  Adafruit_PWMServoDriver _pca[3] = {0x40, 0x41, 0x42};
  void _initialize_pca(void){
    for (auto pca: _pca) {
      pca.begin();
      pca.setOscillatorFrequency(27000000);
      pca.setPWMFreq(1600);  // This is the maximum PWM frequency
    }
  };
  int _channels[ledNum][4] = {
    {0, 0, 1, 2},
    {0, 3, 4, 5},
    {0, 6, 7, 8},
    {0, 9, 10, 11},
    {0, 12, 13, 14},
    {1, 0, 1, 2},
    {1, 3, 4, 5},
    {1, 6, 7, 8},
    {1, 9, 11, 10},
    {1, 12, 13, 14},
    {2, 0, 1, 2},
    {2, 3, 4, 5},
    {2, 6, 7, 8},
    {2, 9, 10, 11},
    {2, 12, 13, 14}
  };
  int calcBrightness(Color color){
    return (int)((color.red + color.green + color.blue) / 7.65);
  }
};

#endif