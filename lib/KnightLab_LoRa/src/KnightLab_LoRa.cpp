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
