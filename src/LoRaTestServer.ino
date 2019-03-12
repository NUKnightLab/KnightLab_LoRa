#include <Arduino.h>
#include <KnightLab_LoRa.h>

#ifndef UNIT_TEST


/**
 * For network testing, deploy 2 servers with IDs 1 and 2 and define RH_TEST_NETWORK as 1.
 * Test with a client of ID 3 which should have direct access to 2 and hopped access to 1.
 */
#define TEST_SERVER_ID 1
#define RH_TEST_NETWORK 1
#define RF95_CS 8
#define RF95_INT 3
#define TX_POWER 5

#define KL_FLAGS_NOECHO 0x80

void setup() {
    Serial.begin(9600);
    //while (!Serial);
    Serial.print("Executing with RadioHead version: ");
    Serial.print(RH_VERSION_MAJOR);
    Serial.print(".");
    setupLoRa(TEST_SERVER_ID, RF95_CS, RF95_INT, TX_POWER);
    Serial.print(RH_VERSION_MINOR);
    Serial.print("; Server ID: ");
    Serial.println(TEST_SERVER_ID);
}

uint8_t buf[RH_ROUTER_MAX_MESSAGE_LEN];

int counter = 0;
void loop() {
    if (++counter % 10000 == 0)
        Serial.print(".");
    if (counter % 1000000 == 0)
        Serial.println("");
    uint8_t len = sizeof(buf);
    uint8_t source;
    uint8_t dest;
    uint8_t msg_id;
    uint8_t flags;
    if (LoRaRouter->recvfromAck(buf, &len, &source, &dest, &msg_id, &flags)) {
        Serial.print("RECEIVED LEN: ");
        Serial.print(len);
        Serial.print("; SOURCE: ");
        Serial.print(source);
        Serial.print("; DEST: ");
        Serial.print(dest);
        Serial.print("; ID: ");
        Serial.print(msg_id);
        Serial.print("; FLAGS: ");
        Serial.print(flags, HEX);
        Serial.print("; RSSI: ");
        Serial.println(LoRaRadio->lastRssi());
        Serial.println((char*)buf);
        LoRaRouter->addRouteTo(source, source);
        if ( (flags & KL_FLAGS_NOECHO) == KL_FLAGS_NOECHO) {
            Serial.println("FLAG: NOECHO. Not sending reply");
        } else {
            if (LoRaRouter->sendtoWait(buf, len, source) != RH_ROUTER_ERROR_NONE) {
                Serial.println("sendtoWait FAILED");
            } else {
                Serial.println("SENT REPLY");
            }
        }
        memset(buf, 0, sizeof(buf));
    }
}
#endif