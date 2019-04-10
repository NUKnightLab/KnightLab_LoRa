#ifndef KNIGHTLAB_LORA_H
#define KNIGHTLAB_LORA_H

#include <Arduino.h>
#include <RH_RF95.h>
#include <KnightLab_LoRaRouter.h>
//#include "KLArpableReliableDatagram.h"
#include <stdint.h>

/**
 * We are finding that the maximum length that will successfully send is not
 * RH_RF95_MAX_MESSAGE_LEN which is 251. 246 seems to be the max that is working in our tests.
 */
// TODO: set this according to:
// if (((uint16_t)len + sizeof(RoutedMessageHeader)) > _driver.maxMessageLength())
//#define KL_LORA_MAX_MESSAGE_LEN 246

extern RH_RF95 *LoRaRadio;
extern KnightLab_LoRaRouter *LoRaRouter;

static uint8_t LoRaReceiveBuffer[KL_ROUTER_MAX_MESSAGE_LEN];
//static uint8_t LoRaReceiveBuffer[RH_ROUTER_MAX_MESSAGE_LEN];

void setupLoRa(uint8_t node_id, uint8_t rf95_cs, uint8_t rf95_int, uint8_t tx_power=10);
uint8_t sendLoRaMessage(uint8_t *msg, uint8_t len, uint8_t to_id, uint8_t flags=0x00);
bool receiveLoRaMessage(uint16_t timeout=0, bool sleep=false);
void initializeLoRaRoutes();
void fetchRoutesFrom(uint8_t node_id);

#endif