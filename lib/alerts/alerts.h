#ifndef _ALERTS_H_
#define _ALERTS_H_
#include <Arduino.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

void init_radio();
void radio_update();
void send_alert();
void heartbeat();
void alert_nrf();

#endif
