/*
 * YoutubeApi - An Arduino wrapper for the YouTube API
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
 * Copyright (c) 2020 Brian Lough
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

#ifndef YoutubeApi_h
#define YoutubeApi_h

#include <vector>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Client.h>

#define YTAPI_HOST "www.googleapis.com"
#define YTAPI_SSL_PORT 443
#define YTAPI_TIMEOUT 1500
#define YTAPI_GET_MOVIE_SIZE 5

#define YTAPI_CHANNEL_ENDPOINT "/youtube/v3/channels"
#define YTAPI_SEARCH_ENDPOINT "/youtube/v3/search"
#define YTAPI_VIDEOS_ENDPOINT "/youtube/v3/videos"

struct channelStatistics{
  long viewCount;
  long commentCount; /* DEPRECATED */
  long subscriberCount;
  bool hiddenSubscriberCount;
  long videoCount;
};

struct BroadcastDetails {
  String schedualStartTime;
  String actualEndTime;
};

class YoutubeApi
{
  public:
    YoutubeApi (char *apiKey, Client &client);
    YoutubeApi (String apiKey, Client &client);
    int sendGetToYoutube(char *command);
    bool getChannelStatistics(char *channelId);
    bool getChannelStatistics(String channelId);
    std::vector<String> getUpcomingBroadcasts(char *channelId);
    std::vector<String> getUpcomingBroadcasts(String channelId);
    BroadcastDetails getBroadcastDetails(char *videoId);
    BroadcastDetails getBroadcastDetails(String videoId);
    channelStatistics channelStats;
    bool _debug = false;

  private:
    char *_apiKey;
    Client *client;
    int getHttpStatusCode();
    void skipHeaders();
    void closeClient();
};

#endif
