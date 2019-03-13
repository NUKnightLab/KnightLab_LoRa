#include "KnightLab_LoRa.h"
#include <Arduino.h>

RH_RF95 *LoRaRadio;
RHRouter *LoRaRouter;
uint8_t StaticRoutes[255];


void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int, uint8_t tx_power) {
    for (uint8_t i=0; i<255; i++) {
        StaticRoutes[i] = i;
    }
    LoRaRadio = new RH_RF95(rf95_cs, rf95_int);
    LoRaRadio->setFrequency(915.0);
    LoRaRadio->setTxPower(tx_power);
    LoRaRouter = new RHRouter(*LoRaRadio, node_id);
    LoRaRouter->init();
    LoRaRouter->clearRoutingTable();
    LoRaRadio->sleep();
}

void setStaticRoute(uint8_t dest, uint8_t next_hop) {
    StaticRoutes[dest] = next_hop;
    LoRaRouter->addRouteTo(dest, next_hop);
}

/**
 * It is recommended that you call these functions rather than direct calls from the RadioHead
 * library unless you are explicitly managing:
 *   - routes in the routing table.
 *   - radio sleep
 */

/**
 * Send a message to another LoRa device.
 */
uint8_t sendLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id, uint8_t flags, bool sleep) {
    // Assume the node is in range if a route is not defined
    if (LoRaRouter->getRouteTo(to_id)->state != RHRouter::Valid) {
        //LoRaRouter->addRouteTo(to_id, to_id);
        LoRaRouter->addRouteTo(to_id, StaticRoutes[to_id]);
    }
    uint8_t status = LoRaRouter->sendtoWait(msg, len, to_id, flags);
    if (sleep) {
        LoRaRadio->sleep();
    }
    return status;
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
    sendLoRaMessage(msg, len, to_id, flags, false);
    //LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from));
}