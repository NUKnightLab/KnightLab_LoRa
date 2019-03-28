#include <Arduino.h>
#include <KnightLab_LoRa.h>
//#include <KnightLab_LoRaRouter.h>

#ifndef UNIT_TEST


/**
 * For network testing, deploy 2 servers with IDs 1 and 2 and define RH_TEST_NETWORK as 1.
 * Test with a client of ID 3 which should have direct access to 2 and hopped access to 1.
 */
#define TEST_SERVER_ID 3
// #define RH_TEST_NETWORK 1
#define RF95_CS 8
#define RF95_INT 3
#define TX_POWER 7

#define KL_FLAGS_NOECHO 0x80

void setup() {
    Serial.begin(9600);
    //while (!Serial);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Executing with RadioHead version: ");
    Serial.print(RH_VERSION_MAJOR);
    Serial.print(".");
    //initializeLoRaRoutes(); // sets all routes to single-hop
    Serial.print(RH_VERSION_MINOR);
    Serial.print("; Server ID: ");
    Serial.println(TEST_SERVER_ID);
    setupLoRa(TEST_SERVER_ID, RF95_CS, RF95_INT, TX_POWER);
    #ifdef RH_TEST_NETWORK 
    #if RH_TEST_NETWORK == 1
    //if (TEST_SERVER_ID == 1) {
    //    LoRaRouter->addRouteTo(3, 2);
    //}
    #endif
    #endif
}

void loop() {
    static uint8_t buf[255];
    static int counter = 0;
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
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.print("RECEIVED LEN: ");
        Serial.print(len);
        Serial.print("; HEADER FROM: ");
        Serial.print (LoRaRouter->headerFrom());
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
        //LoRaRouter->addRouteTo(source, LoRaRouter->headerFrom());
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
    digitalWrite(LED_BUILTIN, LOW);
}
#endif