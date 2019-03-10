#include "KnightLab_LoRa.h"

RH_RF95 *LoRaRadio;
RHRouter *LoRaRouter;

void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int) {
    LoRaRadio = new RH_RF95(rf95_cs, rf95_int);
    LoRaRadio->setFrequency(915.0);
    LoRaRouter = new RHRouter(*LoRaRadio, node_id);
    LoRaRouter->init();
    LoRaRouter->clearRoutingTable();
    LoRaRadio->sleep();
}

uint8_t sendLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id) {
    // Assume the node is in range if a route is not defined
    if (LoRaRouter->getRouteTo(to_id)->state != RHRouter::Valid) {
        LoRaRouter->addRouteTo(to_id, to_id);
    }
    return LoRaRouter->sendtoWait(msg, len, to_id);
}