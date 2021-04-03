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


#include "YoutubeApi.h"

YoutubeApi::YoutubeApi(String apiKey, Client &client)	{
	int strLen = apiKey.length() + 1; 
	char tempStr[strLen];
	apiKey.toCharArray(tempStr, strLen);

	YoutubeApi(tempStr, client);
}

YoutubeApi::YoutubeApi(char *apiKey, Client &client)	{
	_apiKey = apiKey;
	this->client = &client;
}

int YoutubeApi::sendGetToYoutube(char *command) {
	client->flush();
    client->setTimeout(YTAPI_TIMEOUT);
	if (!client->connect(YTAPI_HOST, YTAPI_SSL_PORT))
    {
        Serial.println(F("Connection failed"));
        return false;
    }
	// give the esp a breather
    yield();

    // Send HTTP request
    client->print(F("GET "));
    client->print(command);
    client->println(F(" HTTP/1.1"));

	//Headers
    client->print(F("Host: "));
    client->println(YTAPI_HOST);

    client->println(F("Cache-Control: no-cache"));

	if (client->println() == 0)
    {
        Serial.println(F("Failed to send request"));
        return -1;
    }

	int statusCode = getHttpStatusCode();
    
    // Let the caller of this method parse the JSon from the client
    skipHeaders();
    return statusCode;
}

bool YoutubeApi::getChannelStatistics(String channelId){

	int strLen = channelId.length() + 1; 
	char tempStr[strLen];
	channelId.toCharArray(tempStr, strLen);

	return getChannelStatistics(tempStr);
}

std::vector<String> YoutubeApi::getUpcomingBroadcasts(String channelId){

	int strLen = channelId.length() + 1; 
	char tempStr[strLen];
	channelId.toCharArray(tempStr, strLen);

	return getUpcomingBroadcasts(tempStr);
}
    
BroadcastDetails YoutubeApi::getBroadcastDetails(String videoId){

	int strLen = videoId.length() + 1; 
	char tempStr[strLen];
	videoId.toCharArray(tempStr, strLen);

	return getBroadcastDetails(tempStr);
}

bool YoutubeApi::getChannelStatistics(char *channelId){
	char command[150] = YTAPI_CHANNEL_ENDPOINT;
    char params[120];
    sprintf(params, "?part=statistics&id=%s&key=%s", channelId, _apiKey);
    strcat(command, params);

    if (_debug)
    {
        Serial.println(command);
    }

	bool wasSuccessful = false;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = JSON_ARRAY_SIZE(1) 
							+ JSON_OBJECT_SIZE(2) 
							+ 2*JSON_OBJECT_SIZE(4) 
							+ JSON_OBJECT_SIZE(5)
                            + 330;

    int httpStatus = sendGetToYoutube(command);

    if (httpStatus == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
			wasSuccessful = true;

            JsonObject itemStatistics = doc["items"][0]["statistics"];

			channelStats.viewCount = itemStatistics["viewCount"].as<long>();
			channelStats.subscriberCount = itemStatistics["subscriberCount"].as<long>();
			channelStats.commentCount = itemStatistics["commentCount"].as<long>();
			channelStats.hiddenSubscriberCount = itemStatistics["hiddenSubscriberCount"].as<bool>();
			channelStats.videoCount = itemStatistics["videoCount"].as<long>();
        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    } else {
        Serial.print("Unexpected HTTP Status Code: ");
        Serial.println(httpStatus);
    }
    closeClient();

	return wasSuccessful;
}

std::vector<String> YoutubeApi::getUpcomingBroadcasts(char *channelId){
	char command[150] = YTAPI_SEARCH_ENDPOINT;
    char params[120];
    sprintf(params, "?part=id&channelId=%s&key=%s&eventType=upcoming&type=video", channelId, _apiKey);
    strcat(command, params);

    if (_debug)
    {
        Serial.println(command);
    }

	std::vector<String> results;

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 1000;

    int httpStatus = sendGetToYoutube(command);

    if (httpStatus == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
            JsonObject pageInfo = doc["pageInfo"];
            int resultsPerPage = pageInfo["resultsPerPage"].as<int>();
            for (int i = 0; i < resultsPerPage; i++) {
                JsonObject itemStatistics = doc["items"][i]["id"];
			    results.push_back(itemStatistics["videoId"].as<String>());
            }
        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    } else {
        Serial.print("Unexpected HTTP Status Code: ");
        Serial.println(httpStatus);
    }
    closeClient();

	return results;
}

BroadcastDetails YoutubeApi::getBroadcastDetails(char *videoId){
	char command[150] = YTAPI_VIDEOS_ENDPOINT;
    char params[120];
    sprintf(params, "?part=liveStreamingDetails&id=%s&key=%s", videoId, _apiKey);
    strcat(command, params);

    if (_debug)
    {
        Serial.println(command);
    }

	BroadcastDetails result = {
        "null",
        "null"
    };

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = 1000;

    int httpStatus = sendGetToYoutube(command);

    if (httpStatus == 200)
    {
        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
        DeserializationError error = deserializeJson(doc, *client);
        if (!error)
        {
            JsonObject itemStatistics = doc["items"][0]["liveStreamingDetails"];

			result.schedualStartTime = itemStatistics["scheduledStartTime"].as<String>();
            result.actualEndTime = itemStatistics["actualEndTime"].as<String>();
        }
        else
        {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
        }
    } else {
        Serial.print("Unexpected HTTP Status Code: ");
        Serial.println(httpStatus);
    }
    closeClient();

	return result;
}

void YoutubeApi::skipHeaders()
{
    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client->find(endOfHeaders))
    {
        Serial.println(F("Invalid response"));
        return;
    }

    // Was getting stray characters between the headers and the body
    // This should toss them away
    while (client->available() && client->peek() != '{')
    {
        char c = 0;
        client->readBytes(&c, 1);
        if (_debug)
        {
            Serial.print("Tossing an unexpected character: ");
            Serial.println(c);
        }
    }
}

int YoutubeApi::getHttpStatusCode()
{
    // Check HTTP status
    if(client->find("HTTP/1.1")){
        int statusCode = client->parseInt();
        return statusCode;
    } 

    return -1;
}

void YoutubeApi::closeClient() {
	if(client->connected()) {
		if(_debug) { Serial.println(F("Closing client")); }
		client->stop();
	}
}
