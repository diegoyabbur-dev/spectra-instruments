// rtc.h
#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

void initRTC();
void createTimeStamp();
void setRTCTimeManually();
void readRTCTime();
void rtcAdjust();

#endif
