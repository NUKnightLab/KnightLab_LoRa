#ifndef KNIGHTLAB_LORA_H
#define KNIGHTLAB_LORA_H

#include <RH_RF95.h>
#include <RHRouter.h>

/**
 * We are finding that the maximum length that will successfully send is not
 * RH_RF95_MAX_MESSAGE_LEN which is 251. 246 seems to be the max that is working in our tests.
 */
#define KL_LORA_MAX_MESSAGE_LEN 246

extern RH_RF95 *LoRaRadio;
extern RHRouter *LoRaRouter;

extern void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int);

#endif