#include "KnightLab_LoRa.h"
#include <Arduino.h>

RH_RF95 *LoRaRadio;
RHRouter *LoRaRouter;

void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int, uint8_t tx_power) {
    LoRaRadio = new RH_RF95(rf95_cs, rf95_int);
    LoRaRadio->setFrequency(915.0);
    LoRaRadio->setTxPower(tx_power);
    LoRaRouter = new RHRouter(*LoRaRadio, node_id);
    LoRaRouter->init();
    LoRaRouter->clearRoutingTable();
    LoRaRadio->sleep();
}

/**
 * It is recommended that you call this function rather than directly calling sendtoWait unless you
 * are explicitly managing the routes in the routing table.
 */
uint8_t sendLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id, uint8_t flags) {
    // Assume the node is in range if a route is not defined
    if (LoRaRouter->getRouteTo(to_id)->state != RHRouter::Valid) {
        LoRaRouter->addRouteTo(to_id, to_id);
    }
    uint8_t status = LoRaRouter->sendtoWait(msg, len, to_id, flags);
    LoRaRadio->sleep();
    return status;
}