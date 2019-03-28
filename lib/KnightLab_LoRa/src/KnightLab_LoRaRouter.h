// KnightLab_LoRaRouter.h
//

#ifndef KnightLab_LoRaRouter_h
#define KnightLab_LoRaRouter_h

#include <RHDatagram.h>

#define RH_TEST_NETWORK 1

// The acknowledgement bit in the FLAGS
// The top 4 bits of the flags are reserved for RadioHead. The lower 4 bits are reserved
// for application layer use.
#define RH_FLAGS_ACK 0x80
#define KL_FLAGS_ARP 0x08
#define KL_FLAGS_TEST_CONTROL 0x40
#define RH_DEFAULT_TIMEOUT 200
#define RH_DEFAULT_RETRIES 3

#define KL_TEST_CONTROL_CLEAR_ROUTES 1
#define KL_TEST_CONTROL_SET_RETRIES 2
#define KL_TEST_CONTROL_SET_TIMEOUT 3
#define KL_TEST_CONTROL_SET_CAD_TIMEOUT 4
#define KL_TEST_CONTROL_SET_MODEM_CONFIG 5

#define RH_DEFAULT_MAX_HOPS 30

// Error codes
#define RH_ROUTER_ERROR_NONE              0
#define RH_ROUTER_ERROR_INVALID_LENGTH    1
#define RH_ROUTER_ERROR_NO_ROUTE          2
#define RH_ROUTER_ERROR_TIMEOUT           3
#define RH_ROUTER_ERROR_NO_REPLY          4
#define RH_ROUTER_ERROR_UNABLE_TO_DELIVER 5

#define KL_ACK_CODE_NONE 0
#define KL_ACK_CODE_ERROR_NO_ROUTE 2

#define RH_ROUTER_MAX_MESSAGE_LEN (RH_MAX_MESSAGE_LEN - sizeof(KnightLab_LoRaRouter::RoutedMessageHeader))
// 255 - sizeof(RoutedMessageHeader)

//(RH_RF95_MAX_MESSAGE_LEN - sizeof(KnightLab_LoRaRouter::RoutedMessageHeader))
#define KL_ROUTER_MAX_MESSAGE_LEN 246

class KnightLab_LoRaRouter : public RHDatagram
{
public:
    /// Defines the structure of the RHRouter message header, used to keep track of end-to-end delivery parameters
    typedef struct {
	    uint8_t    dest;       ///< Destination node address
	    uint8_t    source;     ///< Originator node address
	    uint8_t    hops;       ///< Hops traversed so far
	    uint8_t    id;         ///< Originator sequence number
	    uint8_t    flags;      ///< Originator flags
	    // Data follows, Length is implicit in the overall message length
    } RoutedMessageHeader;
    /// Defines the structure of a RHRouter message
    typedef struct {
	    RoutedMessageHeader header;    ///< end-to-end delivery header
	    //uint8_t             data[RH_ROUTER_MAX_MESSAGE_LEN]; ///< Application payload data
	    uint8_t             data[KL_ROUTER_MAX_MESSAGE_LEN]; ///< Application payload data
    } RoutedMessage;

    KnightLab_LoRaRouter(RHGenericDriver& driver, uint8_t thisAddress = 0);
    bool init();
    void peekAtMessage(RoutedMessage* message, uint8_t messageLen);
    void setTimeout(uint16_t timeout);
    void setRetries(uint8_t retries);
    uint8_t retries();
    bool routeHelper(uint8_t* buf, uint8_t len, uint8_t address, uint8_t flags);
    bool routeArp(uint8_t* buf, uint8_t len, uint8_t address);
    bool routeTestControl(uint8_t* buf, uint8_t len, uint8_t address);
    uint8_t sendtoWait(uint8_t* buf, uint8_t len, uint8_t dest, uint8_t flags=0x00);
    uint8_t sendtoFromSourceWait(uint8_t* buf, uint8_t len, uint8_t dest, uint8_t source, uint8_t flags);
    bool recvfromAckHelper(uint8_t* buf, uint8_t* len, uint8_t* from = NULL, uint8_t* to = NULL, uint8_t* id = NULL, uint8_t* flags = NULL);
    bool recvfromAck(uint8_t* buf, uint8_t* len, uint8_t* from = NULL, uint8_t* to = NULL, uint8_t* id = NULL, uint8_t* flags = NULL);
    bool recvfromAckTimeout(uint8_t* buf, uint8_t* len,  uint16_t timeout, uint8_t* from = NULL, uint8_t* to = NULL, uint8_t* id = NULL, uint8_t* flags = NULL);
    uint32_t retransmissions();
    void resetRetransmissions(); 
    bool recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from=NULL, uint8_t* to=NULL, uint8_t* id=NULL, uint8_t* flags=NULL);
    uint8_t route(RoutedMessage* message, uint8_t messageLen);
    void addRouteTo(uint8_t dest, uint8_t next_hop, int16_t rssi=-32768);
    void setRouteSignalStrength(uint8_t dest, int16_t rssi);
    int16_t getRouteSignalStrength(uint8_t dest);
    uint8_t getRouteTo(uint8_t dest);
    uint8_t doArp(uint8_t dest);
    uint8_t getSequenceNumber();
    void clearRoutingTable();
    void broadcastClearRoutingTable();
    void broadcastRetries(uint8_t retries);
    void broadcastTimeout(uint16_t timeout);
    void broadcastCADTimeout(unsigned long timeout);
    void broadcastModemConfig(RH_RF95::ModemConfigChoice config);
    void printRoutingTable();
    bool passesTopologyTest(uint8_t from);

protected:
    void acknowledge(uint8_t id, uint8_t from, uint8_t ack_code=KL_ACK_CODE_NONE);
    bool haveNewMessage();
    uint8_t _lastE2ESequenceNumber;

private:
    uint32_t _retransmissions;
    uint8_t _lastSequenceNumber;
    uint8_t _max_hops;
    uint16_t _timeout;
    uint8_t _retries;
    uint8_t _seenIds[256];
    //uint8_t _last_received_from_id;

    uint8_t _static_routes[255];
    uint8_t _route_signal_strength[255];

    static RoutedMessage _tmpMessage;
    static RoutedMessage _arpMessage; // we don't want to overwrite the _tmpMessage with an arp
};

#endif

