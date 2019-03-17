#include <Arduino.h>
#include <unity.h>
#include "KnightLab_LoRa/test.h"
#include "KnightLab_LoRa/test_router.h"


void setup() {
    while (!Serial);
    UNITY_BEGIN();
    Test_KnightLab_LoRaRouter::test_all();
    Test_KnightLab_LoRa::test_all();
    UNITY_END(); // stop unit testing
}

void loop() {
}