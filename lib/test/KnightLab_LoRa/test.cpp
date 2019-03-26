#include <Arduino.h>
#include <unity.h>
#include <KnightLab_LoRa.h>
#include "test.h"

#define TEST_SERVER_ID 2
#define HOPPED_TEST_SERVER_ID 1
#define KL_FLAGS_NOECHO 0x80
#define RH_TEST_NETWORK 1
#define RF95_CS 8
#define RF95_INT 3
#define TX_POWER 5

uint8_t node_id = 3;
uint8_t long_msg[] = "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";

/**
 * It seems that unity does not provide any setup mechanism for individual tests. Thus, for
 * consistent, independent testing, call this setup for each function that tests wireless
 * connectivity and routing.
 * 
 * Initializes testing conditions to:
 *  - no retries
 *  - default timeout
 *  - CAD timeout of 500
 *  - empty routing table
 */
void _setup()
{
    LoRaRouter->broadcastModemConfig();
    LoRaRadio->setModemConfig(RH_RF95::Bw125Cr48Sf4096);

    LoRaRouter->setRetries(0); // default retries is 3
    LoRaRouter->broadcastRetries(0);

    LoRaRouter->setTimeout(10000); // Default timeout is 200
    LoRaRouter->broadcastTimeout(10000);

    LoRaRadio->setCADTimeout(100000);
    LoRaRouter->broadcastCADTimeout(100000);

    LoRaRouter->broadcastClearRoutingTable();
    LoRaRouter->clearRoutingTable();
}

/**
 * Forces a hopped route by sending a message to the next hop even if not in the routing table.
 * This will result in an arp on the next hop node as it tries to route the message to the
 * destination. We wait out the arp by ensuring enough retries.
 * 
 * Designed for testing purposes only. It is not yet clear if a route forcing mechanism will be
 * needed in deployment.
 */
uint8_t _forceRoute(uint8_t to, uint8_t next_hop)
{
    uint8_t retries_orig = LoRaRouter->retries();
    if (retries_orig < 3) {
        LoRaRouter->setRetries(3);
    }
    uint8_t msg[] = "FORCE ARP"; // informational only
    LoRaRouter->addRouteTo(to, next_hop);
    /// We expect this to fail delivery b/c the receiving node does not yet have a route to the dest
    sendLoRaMessage(msg, sizeof(msg), to, KL_FLAGS_NOECHO);
    uint8_t new_route = LoRaRouter->doArp(HOPPED_TEST_SERVER_ID);
    LoRaRouter->setRetries(retries_orig);
    return new_route;
}

namespace Test_KnightLab_LoRa {

    void KnightLab_LoRa__test_test(void) {
        TEST_ASSERT_EQUAL(1,1);
    }

    void test_test(void) {
        TEST_ASSERT_EQUAL(1,1);
    }

    void test_RH_version(void) {
        TEST_ASSERT_EQUAL(1, RH_VERSION_MAJOR);
        TEST_ASSERT_EQUAL(85, RH_VERSION_MINOR);
    }


    void test_initializeLoRaRoutes(void) {
        LoRaRouter->clearRoutingTable();
        initializeLoRaRoutes();
        TEST_ASSERT_EQUAL(
            TEST_SERVER_ID,
            LoRaRouter->getRouteTo(TEST_SERVER_ID));
    }

    void test_router_setup(void) {
        setupLoRa(node_id, RF95_CS, RF95_INT, TX_POWER);
        TEST_ASSERT_EQUAL(0, LoRaRouter->getRouteTo(TEST_SERVER_ID));
    }

    void test_long_echo(void) {
        LoRaRouter->broadcastClearRoutingTable();
        LoRaRouter->clearRoutingTable();
        TEST_ASSERT_EQUAL(KL_ROUTER_MAX_MESSAGE_LEN, sizeof(long_msg));
        TEST_ASSERT_EQUAL(
            RH_ROUTER_ERROR_NONE,
            //LoRaRouter->sendtoWait(long_msg, KL_LORA_MAX_MESSAGE_LEN, TEST_SERVER_ID));
            //LoRaRouter->sendtoWait(long_msg, RH_ROUTER_MAX_MESSAGE_LEN, TEST_SERVER_ID));
            LoRaRouter->sendtoWait(long_msg, KL_ROUTER_MAX_MESSAGE_LEN, TEST_SERVER_ID));
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        //uint8_t len = sizeof(LoRaReceiveBuffer);
        uint8_t from;
        TEST_ASSERT_TRUE(
            LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from));
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, from);
        //TEST_ASSERT_EQUAL(KL_LORA_MAX_MESSAGE_LEN, len);
        TEST_ASSERT_EQUAL(KL_ROUTER_MAX_MESSAGE_LEN, len);
        TEST_ASSERT_EQUAL_STRING_LEN(long_msg, buf, len);
    }

    void test_sendLoRaMessage(void) {
        uint8_t msg[] = "TEST SEND";
        TEST_ASSERT_EQUAL(
            RH_ROUTER_ERROR_NONE,
            sendLoRaMessage(msg, sizeof(msg), TEST_SERVER_ID, KL_FLAGS_NOECHO));
        TEST_ASSERT_TRUE(LoRaRouter->getRouteTo(TEST_SERVER_ID));
    }

    void test_long_hopped_send_receive(void) {
        LoRaRouter->broadcastClearRoutingTable();
        LoRaRouter->clearRoutingTable();
        TEST_ASSERT_EQUAL( 0, LoRaRouter->getRouteTo(HOPPED_TEST_SERVER_ID));
        // force the route
        LoRaRouter->addRouteTo(HOPPED_TEST_SERVER_ID, TEST_SERVER_ID);
        TEST_ASSERT_EQUAL(
            TEST_SERVER_ID,
            LoRaRouter->getRouteTo(HOPPED_TEST_SERVER_ID)
        );
        TEST_ASSERT_EQUAL(
            RH_ROUTER_ERROR_NONE,
            //sendLoRaMessage(long_msg, KL_LORA_MAX_MESSAGE_LEN, HOPPED_TEST_SERVER_ID)
            sendLoRaMessage(long_msg, KL_ROUTER_MAX_MESSAGE_LEN, HOPPED_TEST_SERVER_ID)
        );
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        uint8_t source;
        uint8_t dest;
        TEST_ASSERT_TRUE(
            LoRaRouter->recvfromAckTimeout(buf, &len, 2000, &source, &dest));
        TEST_ASSERT_EQUAL(HOPPED_TEST_SERVER_ID, source);
        TEST_ASSERT_EQUAL(node_id, dest);
        //TEST_ASSERT_EQUAL(KL_LORA_MAX_MESSAGE_LEN, len);
        TEST_ASSERT_EQUAL(KL_ROUTER_MAX_MESSAGE_LEN, len);
        TEST_ASSERT_EQUAL_STRING_LEN(long_msg, buf, len);
    }

    /**
     * OBSOLETE. This test no longer needed with custom KLRouter.
     * 
     * RadioHead only allows for 10 routes, but we are dynamically setting the next hop routes
     * in the sendLoRaMessage function, thus we expect successful delivery even after route
     * saturation.
     * 
     * This is somewhat non-deterministic because we do't really know what the oldest route is, so
     * we need to add enough routes to be sure the test server route is cleared. 2x the table size
     * should do it.
     */
    /*
    void test_many_routes(void) {
        //LoRaRouter->printRoutingTable();
        for (uint8_t id=0; id<RH_ROUTING_TABLE_SIZE*2; id++) {
            if (id != TEST_SERVER_ID) {
                LoRaRouter->addRouteTo(id, id);
            }
        }
        //LoRaRouter->printRoutingTable();
        TEST_ASSERT_NULL(LoRaRouter->getRouteTo(TEST_SERVER_ID));
        uint8_t msg[] = "TEST MANY ROUTES";
        TEST_ASSERT_EQUAL(
            RH_ROUTER_ERROR_NONE,
            sendLoRaMessage(msg, sizeof(msg), TEST_SERVER_ID, false));
    }
    */

    void test_echo(void) {
        _setup();
        static uint8_t last_seq = 255;
        uint8_t msg[] = "TEST ECHO";
        // RH_ROUTER_ERROR_NONE 0
        // RH_ROUTER_ERROR_INVALID_LENGTH    1
        // RH_ROUTER_ERROR_NO_ROUTE 2
        // RH_ROUTER_ERROR_NO_REPLY 4
        // RH_ROUTER_ERROR_UNABLE_TO_DELIVER 5
        //uint8_t buf[KL_LORA_MAX_MESSAGE_LEN];
        uint8_t buf[KL_ROUTER_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        uint8_t from;
        //LoRaRouter->broadcastClearRoutingTable();
        //LoRaRouter->clearRoutingTable();
        //TEST_ASSERT_EQUAL(0, LoRaRouter->getRouteTo(TEST_SERVER_ID));
        TEST_ASSERT_EQUAL(
            RH_ROUTER_ERROR_NONE,
            LoRaRouter->sendtoWait(msg, sizeof(msg), TEST_SERVER_ID));
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->getRouteTo(TEST_SERVER_ID));
        Serial.println("TEST SUCCESSFUL sendtoWait");
        Serial.println("TEST SENDING recvfromAckTimeout");
        TEST_ASSERT_TRUE(
            LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from));
        TEST_ASSERT(last_seq != LoRaRouter->getSequenceNumber());
        last_seq = LoRaRouter->getSequenceNumber();
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, from);
        TEST_ASSERT_EQUAL(sizeof(msg), len);
        TEST_ASSERT_EQUAL_STRING_LEN(msg, buf, len);
    }

    /*
    void test_getLastFrom(void) {
        uint8_t msg[] = "TEST getLastFrom";
        uint8_t buf[KL_LORA_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        uint8_t from;
        LoRaRouter->sendtoWait(msg, sizeof(msg), TEST_SERVER_ID);
        LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from);
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->getLastFrom());
        LoRaRouter->sendtoWait(msg, sizeof(msg), HOPPED_TEST_SERVER_ID);
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->getLastFrom());
        LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from);
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->getLastFrom());
    }
    */

    void test_forceRoute(void) {
        _setup();
        TEST_ASSERT_EQUAL(
            TEST_SERVER_ID,
            _forceRoute(HOPPED_TEST_SERVER_ID, TEST_SERVER_ID)
        );
    }

    void test_doArp(void) {
        _setup();
        //LoRaRouter->setRetries(3);
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->doArp(TEST_SERVER_ID));
        TEST_ASSERT_EQUAL(
            TEST_SERVER_ID,
            _forceRoute(HOPPED_TEST_SERVER_ID, TEST_SERVER_ID)
        );
        LoRaRouter->clearRoutingTable();
        TEST_ASSERT_EQUAL(TEST_SERVER_ID, LoRaRouter->doArp(HOPPED_TEST_SERVER_ID));
        TEST_ASSERT_EQUAL(4, LoRaRouter->doArp(4));
    }

    /* test runners */

    void test_all() {
        /* Non-communication tests */
        RUN_TEST(test_RH_version);
        RUN_TEST(test_test);

        /* Communication tests. Always run the setup test. Subsequent tests depend on it. */
        RUN_TEST(test_router_setup);


        /**
         * NOTE: Always be sure to have more than one message sending test, otherwise the server
         * must be reset between tests runs to prevent message rejection based on the incrementing
         * message ID.
         * 
         * We could possibly change the node ID between tests as a way to manage this, but there
         * doesn't seem to be a good deterministic way to start with a node ID that is sure to be
         * different from the previous test run. For this reason and potential added complexity,
         * it seems simpler to always be sure to have more than one test. The simple test_echo
         * is executed twice below as a reminder.
         */

        RUN_TEST(KnightLab_LoRa__test_test);
        RUN_TEST(test_echo); RUN_TEST(test_echo); /* Do not remove. See NOTE above */
        RUN_TEST(test_forceRoute);
        RUN_TEST(test_doArp);
        //RUN_TEST(test_long_echo);
        //RUN_TEST(test_sendLoRaMessage);
        //RUN_TEST(test_getLastFrom);
        #ifdef RH_TEST_NETWORK 
        #if RH_TEST_NETWORK == 1
        //RUN_TEST(test_long_hopped_send_receive);
        #endif
        #endif
    }
}
