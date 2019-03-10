#include <unity.h>
#include <SPI.h>
#include <KnightLab_LoRa.h>

#define TEST_SERVER_ID 51
uint8_t node_id = 57;
uint8_t LoRaReceiveBuffer[KL_LORA_MAX_MESSAGE_LEN];
uint8_t long_msg[] = "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";


void test_test(void) {
    TEST_ASSERT_EQUAL(1,1);
}

void test_RH_version(void) {
    TEST_ASSERT_EQUAL(1, RH_VERSION_MAJOR);
    TEST_ASSERT_EQUAL(85, RH_VERSION_MINOR);
}

void test_echo(void) {
    uint8_t msg[] = "TEST";
    // RH_ROUTER_ERROR_NONE 0
    // RH_ROUTER_ERROR_NO_ROUTE 2
    // RH_ROUTER_ERROR_NO_REPLY 4
    // RH_ROUTER_ERROR_UNABLE_TO_DELIVER 5
    TEST_ASSERT_EQUAL(
        RH_ROUTER_ERROR_NONE,
        LoRaRouter->sendtoWait(msg, sizeof(msg), TEST_SERVER_ID));
    uint8_t len = sizeof(LoRaReceiveBuffer);
    uint8_t from;
    TEST_ASSERT_TRUE(
        LoRaRouter->recvfromAckTimeout(LoRaReceiveBuffer, &len, 3000, &from));
    TEST_ASSERT_EQUAL(TEST_SERVER_ID, from);
    TEST_ASSERT_EQUAL(sizeof(msg), len);
    TEST_ASSERT_EQUAL_STRING_LEN(msg, LoRaReceiveBuffer, len);
    LoRaRadio->sleep();
}

void test_long_echo(void) {
    TEST_ASSERT_EQUAL(RH_RF95_MAX_MESSAGE_LEN, sizeof(long_msg));
    TEST_ASSERT_EQUAL(
        RH_ROUTER_ERROR_NONE,
        LoRaRouter->sendtoWait(long_msg, KL_LORA_MAX_MESSAGE_LEN, TEST_SERVER_ID));
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;
    TEST_ASSERT_TRUE(
        LoRaRouter->recvfromAckTimeout(buf, &len, 3000, &from));
    TEST_ASSERT_EQUAL(TEST_SERVER_ID, from);
    TEST_ASSERT_EQUAL(KL_LORA_MAX_MESSAGE_LEN, len);
    TEST_ASSERT_EQUAL_STRING_LEN(long_msg, buf, len);
    LoRaRadio->sleep();
}

void setup() {
    while (!Serial);
    setupLoRa(node_id, 8, 3);
    LoRaRouter->addRouteTo(TEST_SERVER_ID, TEST_SERVER_ID);
    UNITY_BEGIN();
    /**
     * NOTE: Always be sure to have more than one message sending test, otherwise the server
     * must be reset between tests runs to prevent message rejection based on the incrementing
     * message ID.
     */
    RUN_TEST(test_RH_version);
    RUN_TEST(test_test);
    RUN_TEST(test_echo);
    RUN_TEST(test_echo);
    RUN_TEST(test_long_echo);
    UNITY_END(); // stop unit testing
}

void loop() {
}