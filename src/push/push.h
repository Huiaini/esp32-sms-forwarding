#pragma once
#include <Arduino.h>
#include "config/config.h"

void sendPushNotification(const String& sender, const String& message, const String& timestamp, MsgType msgType = MSG_TYPE_SMS);
bool sendPushChannel(int channelIdx, const String& sender, const String& message, const String& timestamp, MsgType msgType);
