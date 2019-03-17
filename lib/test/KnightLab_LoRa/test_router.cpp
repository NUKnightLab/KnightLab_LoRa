#include <Arduino.h>
#include <unity.h>
#include <KnightLab_LoRa.h>
#include "test_router.h"

RH_RF95 *TestRadio;
KnightLab_LoRaRouter *TestRouter;

namespace Test_KnightLab_LoRaRouter {

    void KnightLab_LoRaRouter__test_test(void) {
        TEST_ASSERT_EQUAL(1,1);
    }

    void KnightLab_LoRaRouter__test_setup(void) {
        TestRouter = new KnightLab_LoRaRouter(*TestRadio, 1);
        TestRouter->init();
        TestRouter->clearRoutingTable();
        for (uint8_t i=1; i< 255; i++) {
            //TEST_ASSERT_EQUAL(0, TestRouter->getRouteTo(i)->next_hop);
            TEST_ASSERT_EQUAL(0, TestRouter->getRouteTo(i));
        }
    }

    /* test runners */

    void test_all() {
        /**
         * We are currently not actually using this radio in these tests other than to setup
         * the router. A mock would be better, but we'll just setup a real radio for now.
         */
        TestRadio = new RH_RF95(8, 3);
        /* Non-communication tests */
        RUN_TEST(KnightLab_LoRaRouter__test_test);
        RUN_TEST(KnightLab_LoRaRouter__test_setup);

        #ifdef RH_TEST_NETWORK 
        #if RH_TEST_NETWORK == 1
        #endif
        #endif

        delete TestRadio;
    }
}