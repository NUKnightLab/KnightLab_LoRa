#include "KnightLab_LoRa.h"
#include <Arduino.h>

RH_RF95 *LoRaRadio;
KnightLab_LoRaRouter *LoRaRouter;
uint8_t StaticRoutes[255];

#define RFM95_RST 4


/**
 * Setup the radio driver and the router for LoRa communications.
 * 
 * IMPORTANT: The order of operations in this setup is extremely important. An improperly
 * initialized driver or router can result in code that appears to work but which transmits
 * at extremely low power levels. Be sure to check the RSSI values you are getting after
 * making any changes to this function!!!
 * 
 * You should be getting RSSI values well above -100 (e.g. -50 to -80) from across a room (open
 * line of sight) at even the lowest power setting (5 dB). If your RSSI values are -100 or below,
 * there may be something wrong here.
 */
void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int, uint8_t tx_power) {
    for (uint8_t i=0; i<255; i++) {
        StaticRoutes[i] = i;
    }
    LoRaRadio = new RH_RF95(rf95_cs, rf95_int);
    /**
     * What does this pin mode and write of the RST pin actually do? This is in the Adafruit docs
     * but no real indication it actually does anything to make a difference for our radio
     * communication. Leaving this here for documentation purposes, but as far as we can tell,
     * this does nothing.
     * 
     * https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/using-the-rfm-9x-radio
     */
    /// From Adafruit docs but seems unnecessary. "Manual reset" whatever that means
    /// pinMode(RFM95_RST, OUTPUT);
    /// digitalWrite(RFM95_RST, LOW);
    /// delay(10);
    /// digitalWrite(RFM95_RST, HIGH);
    /// delay(10);

    LoRaRouter = new KnightLab_LoRaRouter(*LoRaRadio, node_id);
    if (!LoRaRouter->init()) {
        Serial.println("LoRa initialization failed.");
        while(1);
    }

    /** Radio driver settings **/

    LoRaRadio->setFrequency(915.0);
    LoRaRadio->setTxPower(tx_power, false); // 5 to 23
    /**
     * Modem configuration. See: http://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#ab9605810c11c025758ea91b2813666e3
     * 
     * Bw125Cr45Sf128 Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range.
     * Bw500Cr45Sf128 Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range.
     * Bw31_25Cr48Sf512 Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range.
     * Bw125Cr48Sf4096 Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range. 
     * 
     * It is not clear what the relationship is between these modem configurations and the config
     * changes implemented by setLowDataDataRate which is associated with setting alternative
     * spreading factor and/or bandwidth values. If we decide to explore these low-level tweaks,
     * this relationship should be investigated.
     */
    LoRaRadio->setModemConfig(RH_RF95::Bw125Cr45Sf128);
    LoRaRadio->setCADTimeout(0); // 0 has no effect. Set to a valude to wait CAD

    /** Router settings **/

    // LoRaRouter->setRetries(3);
    LoRaRouter->setTimeout(1000); // is this long enough for long range communication?
    LoRaRadio->sleep();
}

void initializeLoRaRoutes() {
    LoRaRouter->initializeAllRoutes();
}

/**
 * It is recommended that you call these functions rather than direct calls from the RadioHead
 * library unless you are explicitly managing routes in the routing table.
 */

/**
 * Send a message to another LoRa device.
 */
uint8_t sendLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id, uint8_t flags) {
    return LoRaRouter->sendtoWait(msg, len, to_id, flags);
}

/*
bool receiveLoRaMessage(uint16_t timeout, bool sleep) {
    if (timeout > 0) {
        LoRaRouter->recvfromAckTimeout(buf, &len, timeout, &from));
    } else {
        LoRaRouter->recvfromAck(buf, &len, &from));
    }
}
*/

/**
 * Send a message to another LoRa device and receive a response to the receive buffer.
 */
uint8_t sendAndReceiveLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id, uint8_t flags) {
    sendLoRaMessage(msg, len, to_id, flags);
    //LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from));
}