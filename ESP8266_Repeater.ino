/* Arduino: ESP8266_Repeater.ino v.1.01 2017/07/11 17:30:00 baseprime Exp $ */
/*
 * Copyright (c) 2017 Tracey Emery <tracey@traceyemery.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* ESP8266_Repeater.ino
 * 
 * Version 1.01 July 2017
 * Just a simple AP for WiFi<-->Serial repeater
 * Not as restrictive as ESP-AT firmware
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

extern "C" {
#include "user_interface.h"
}

// proto

void get_station(const WiFiEventSoftAPModeStationConnected&);

#define UDP 1
#define MAX_CLIENTS 8
#define PORT 8080
#define CLIENT_PORT 8080

#define BROADCAST 10,0,50,255
#define IP 10,0,50,1
#define SUBNET 255,255,255,0
#define DEBUG 0

const char *ssid = "SSID";
const char *password = "PASS";

uint8_t clcount = 0;

IPAddress local_ip(IP);
IPAddress local_subnet(SUBNET);
IPAddress local_gateway(IP);
IPAddress local_broadcast(BROADCAST);

#if UDP
  WiFiUDP myserver;
  WiFiEventHandler wifi_event;
  char udp_packet[4096];
  unsigned char number_clients;
  struct station_info *stat_info;
  struct ip_addr *this_station;
  IPAddress station_address;
#else
  WiFiServer myserver(PORT);
  WiFiClient myserver_clients[MAX_CLIENTS];
#endif

extern "C" {
  #include "user_interface.h"
  #include "espconn.h"
}

void setup() {
  // setup serial
  Serial.begin(115200);
  Serial.println();
  Serial.setDebugOutput(DEBUG);

  // setup AP
  delay(1000);
  espconn_tcp_set_max_con(MAX_CLIENTS);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, local_gateway, local_subnet);
  WiFi.softAP(ssid, password);
  IPAddress my_ip = WiFi.softAPIP();

  if (DEBUG) {
    WiFi.printDiag(Serial);
  }
  // start server
  #if UDP
    if (DEBUG) {
      Serial.println("Starting UDP Mode");
    }

    myserver.begin(PORT);
    wifi_event = WiFi.onSoftAPModeStationConnected(&get_station);
  #else
    myserver.setNoDelay(true);
    myserver.begin();
  #endif
}

void loop() {

  // check for clients
  #if UDP
    if (number_clients > 0) {
      int packet_size = myserver.parsePacket();
      if (packet_size) {
        int len = myserver.read(udp_packet, sizeof(udp_packet));
        if (len > 0) {
          udp_packet[len] = 0;
        }
        Serial.print(udp_packet);
      }
    }
  #else
    if (myserver.hasClient()) {
      for (clcount = 0; clcount < MAX_CLIENTS; clcount++) {
        if (!myserver_clients[clcount] || !myserver_clients[clcount].connected()) {
          if (myserver_clients[clcount])
            myserver_clients[clcount].stop();
          myserver_clients[clcount] = myserver.available();
          continue;
        }
      }

      // reject client

      WiFiClient myserver_reject = myserver.available();
      myserver_reject.stop();
    }

    // check for client data

    for (clcount = 0; clcount < MAX_CLIENTS; clcount++) {
      if (myserver_clients[clcount] && myserver_clients[clcount].connected()) {
        if(myserver_clients[clcount].available()) {
          // write to serial
          while(myserver_clients[clcount].available()) Serial.write(myserver_clients[clcount].read());        
        }
      }
    }
  #endif
  // check for serial data and write to clients

  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    #if UDP

      // broadcast

      if (number_clients > 0) {
        myserver.beginPacketMulticast(local_broadcast, CLIENT_PORT, WiFi.localIP());
        myserver.write(sbuf, len);
        myserver.endPacket();
      }
    #else

      // write to all clients
  
      for (clcount = 0; clcount < MAX_CLIENTS; clcount++) {
        if (myserver_clients[clcount] && myserver_clients[clcount].connected()) {
          myserver_clients[clcount].write(sbuf, len);
          delay(1);
        }
      }
    #endif
  }
}

void
get_station(const WiFiEventSoftAPModeStationConnected& evt)
{
  number_clients = wifi_softap_get_station_num();
}

