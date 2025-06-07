#pragma once
#include <WiFi.h>

extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;
extern IPAddress secondaryDNS;

extern int wifi_reconnect_attempts;

void setup_wifi();
void start_wifi_monitor_task();
void setup_webserver();
void ensure_wifi_connected();
