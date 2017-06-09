/* ESP8266_Repeater.ino
 * 
 * Version 1.00 June 2017
 * Just a simple AP for WiFi<-->Serial repeater
 * Not as restrictive as ESP-AT firmware
 * 
 * Perhaps, I'll turn this in to more of a router later down the road,
 * with module registration and direct packet writing.
 */

#include <ESP8266WiFi.h>

#define MAX_CLIENTS 20
#define PORT 8080

#define IP 10,0,50,1
#define SUBNET 255,255,255,0

const char *ssid = "XXXX";
const char *password = "XXXX";

uint8_t clcount = 0;

IPAddress local_ip(IP);
IPAddress local_subnet(SUBNET);
IPAddress local_gateway(IP);

WiFiServer myserver(PORT);
WiFiClient myserver_clients[MAX_CLIENTS];

extern "C" {
  #include "user_interface.h"
  #include "espconn.h"
}

void setup() {
  // setup serial
  Serial.begin(115200);
  Serial.println();
  Serial.setDebugOutput(true);

  // setup AP
  delay(1000);
  espconn_tcp_set_max_con(MAX_CLIENTS);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, local_gateway, local_subnet);
  WiFi.softAP(ssid, password);
  IPAddress my_ip = WiFi.softAPIP();

  // start server
  myserver.begin();
  myserver.setNoDelay(true);
}

void loop() {

  // check for clients

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

  // check for serial data and write to clients

  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);

    // write to all clients

    for (clcount = 0; clcount < MAX_CLIENTS; clcount++){
      if (myserver_clients[clcount] && myserver_clients[clcount].connected()) {
        myserver_clients[clcount].write(sbuf, len);
        delay(1);
      }
    }
  }
}
