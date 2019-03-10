#include <Arduino.h>
#include <KnightLab_LoRa.h>

#ifndef UNIT_TEST

#define TEST_SERVER_ID 51
#define RF95_CS 8
#define RF95_INT 3

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.print("Executing with RadioHead version: ");
    Serial.print(RH_VERSION_MAJOR);
    Serial.print(".");
    setupLoRa(TEST_SERVER_ID, RF95_CS, RF95_INT);
    Serial.println(RH_VERSION_MINOR);
}

uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

int counter = 0;
void loop() {
    if (++counter % 10000 == 0)
        Serial.print(".");
    if (counter % 1000000 == 0)
        Serial.println("");
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (LoRaRouter->recvfromAck(buf, &len, &from)) {
        Serial.print("got request from: ");
        Serial.print(from);
        Serial.print(": ");
        Serial.println((char*)buf);
        LoRaRouter->addRouteTo(from, from);
        if (LoRaRouter->sendtoWait(buf, len, from) != RH_ROUTER_ERROR_NONE) {
          Serial.println("sendtoWait failed");
        } else {
          Serial.println("sent reply");
        }
        memset(buf, 0, sizeof(buf));
    }
}
#endif