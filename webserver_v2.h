// webserver_v2.h
#ifndef WEBSERVER_V2_H
#define WEBSERVER_V2_H

#include <Arduino.h>

void initWebServer_V2();
void handleWebServer();

void runServer_V2();
void handleNotFound();
void handleDeleteData();
void handleDownloadDataRange();

#endif
