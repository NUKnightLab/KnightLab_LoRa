// KnightLab_LoRaRouter.cpp

// Based on Mike McCauley's RHReliableDatagram but too many core changes to subclass

#include <RH_RF95.h>
#include <KnightLab_LoRaRouter.h>

KnightLab_LoRaRouter::RoutedMessage KnightLab_LoRaRouter::_tmpMessage;
KnightLab_LoRaRouter::RoutedMessage KnightLab_LoRaRouter::_arpMessage;

////////////////////////////////////////////////////////////////////
// Constructors
KnightLab_LoRaRouter::KnightLab_LoRaRouter(RHGenericDriver& driver, uint8_t thisAddress)
    : RHDatagram(driver, thisAddress)
{
    _retransmissions = 0;
    _lastSequenceNumber = 0;
    _timeout = RH_DEFAULT_TIMEOUT;
    _retries = RH_DEFAULT_RETRIES;
    memset(_seenIds, 0, sizeof(_seenIds));
    _max_hops = RH_DEFAULT_MAX_HOPS;
    clearRoutingTable();
}

////////////////////////////////////////////////////////////////////
// Public methods

bool KnightLab_LoRaRouter::init()
{
    bool ret = RHDatagram::init();
    if (ret)
    _max_hops = RH_DEFAULT_MAX_HOPS;
    return ret;
}

void KnightLab_LoRaRouter::peekAtMessage(RoutedMessage* message, uint8_t messageLen)
{
  // Default does nothing
  (void)message; // Not used
  (void)messageLen; // Not used
}

void KnightLab_LoRaRouter::setTimeout(uint16_t timeout)
{
    _timeout = timeout;
}

////////////////////////////////////////////////////////////////////
void KnightLab_LoRaRouter::setRetries(uint8_t retries)
{
    _retries = retries;
}

////////////////////////////////////////////////////////////////////
uint8_t KnightLab_LoRaRouter::retries()
{
    return _retries;
}

uint8_t KnightLab_LoRaRouter::getRouteTo(uint8_t dest)
{
    if (dest == 255) {
        return 255;
    }
    return _static_routes[dest];
}

uint8_t KnightLab_LoRaRouter::sendtoWait(uint8_t* buf, uint8_t len, uint8_t dest, uint8_t flags)
{
    return sendtoFromSourceWait(buf, len, dest, _thisAddress, flags);
}

uint8_t KnightLab_LoRaRouter::sendtoFromSourceWait(uint8_t* buf, uint8_t len, uint8_t dest, uint8_t source, uint8_t flags)
{
    if (((uint16_t)len + sizeof(RoutedMessageHeader)) > _driver.maxMessageLength())
    return RH_ROUTER_ERROR_INVALID_LENGTH;
    // Construct a RH RouterMessage message
    _tmpMessage.header.source = source;
    _tmpMessage.header.dest = dest;
    _tmpMessage.header.hops = 0;
    _tmpMessage.header.id = _lastE2ESequenceNumber++;
    _tmpMessage.header.flags = flags; // application layer flags
    memcpy(_tmpMessage.data, buf, len);
    Serial.print("Constructed _tmpMessage. SOURCE: ");
    Serial.print(_tmpMessage.header.source);
    Serial.print("; DEST: ");
    Serial.print(_tmpMessage.header.dest);
    Serial.print("; ID: ");
    Serial.print(_tmpMessage.header.id);
    Serial.print("; APPLICATION FLAGS: ");
    Serial.println(_tmpMessage.header.flags, HEX);
    return route(&_tmpMessage, sizeof(RoutedMessageHeader)+len);
}

////////////////////////////////////////////////////////////////////
bool KnightLab_LoRaRouter::recvfromAckHelper(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    uint8_t _from;
    uint8_t _to;
    uint8_t _id;
    uint8_t _flags;

    // Get the message before its clobbered by the ACK (shared rx and tx buffer in some drivers
    if (available() && recvfrom(buf, len, &_from, &_to, &_id, &_flags)) {

        // If this is not an ack or arp and is from a source we have not yet seen. Add it to the
        // routing table
        // TODO: check the RSSI
        if ( !(_flags && RH_FLAGS_ACK) && !(_flags && KL_FLAGS_ARP) ) {
            Serial.print("Checking if we have a route to: ");
            Serial.println(_tmpMessage.header.source);
            if (!getRouteTo(_tmpMessage.header.source)) {
                Serial.print("Adding route to: ");
                Serial.print(_tmpMessage.header.source);
                Serial.print(" via: ");
                Serial.println(_from);
                addRouteTo(_tmpMessage.header.source, _from);
                printRoutingTable();
            }
        }

        // SEND_ROUTES
        /*
        if ( (_flags & KL_FLAGS_SEND_ROUTES) == KL_FLAGS_SEND_ROUTES) {
            Serial.println("Received SEND ROUTES request.");
            uint8_t sendbuf[255] = { 0 };
            uint8_t index = 0;
            for (uint8_t i=1; i<255; i++) {
                if (getRouteTo(i)) {
                    sendbuf[index++] = i;
                    sendbuf[index++] = getRouteTo(i);
                }
                // TODO: better handling of buffer limit
                if (index >= 255) {
                    break;
                }
            }
            if (sendtoWait(sendbuf, index, _tmpMessage.header.source, RH_FLAGS_ACK & KL_FLAGS_SEND_ROUTES) != RH_ROUTER_ERROR_NONE) {
                Serial.println("sendtoWait FAILED");
            } else {
                Serial.println("SENT ROUTES");
            }
            return false;
        }
        */

        // ACK
        if (_flags & RH_FLAGS_ACK) {
            Serial.print("Received ACK with data: ");
            Serial.println(buf[5]);
            return false;
        } // Never ACK an ACK
        //// ACKs are not routed. Everything else should be routed, thus here we assume that the
        //// buffer is a routed message and the first byte is the destination.
        uint8_t dest = buf[0];
        // ARP
        if (_flags & KL_FLAGS_ARP) {
            if (_to == _thisAddress) {
                acknowledge(_id, RH_BROADCAST_ADDRESS); // let everybody know I'm here, not just the requestor
            } else if (_to == RH_BROADCAST_ADDRESS && ( dest == 0 || getRouteTo(dest)> 0) )  {
                acknowledgeArp(_id, _from);
            }
            return false; // done processing ARP
        } else if (_to ==_thisAddress) {
            if (_to == dest || getRouteTo(dest)) {
                acknowledge(_id, _from);
            } else {
                acknowledge(_id, _from, KL_ACK_CODE_ERROR_NO_ROUTE);
            }
        }
        // If we have not seen this message before, then we are interested in it
        if (_id != _seenIds[_from]) {
            if (from)  *from =  _from;
            if (to)    *to =    _to;
            if (id)    *id =    _id;
            if (flags) *flags = _flags;
            _seenIds[_from] = _id;
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////
bool KnightLab_LoRaRouter::recvfromAck(uint8_t* buf, uint8_t* len, uint8_t* source, uint8_t* dest, uint8_t* id, uint8_t* flags)
{  
    uint8_t tmpMessageLen = sizeof(_tmpMessage);
    uint8_t _from;
    uint8_t _to;
    uint8_t _id;
    uint8_t _flags;
    if (recvfromAckHelper((uint8_t*)&_tmpMessage, &tmpMessageLen, &_from, &_to, &_id, &_flags))
    {
    peekAtMessage(&_tmpMessage, tmpMessageLen);

    // See if its for us or has to be routed
    if ( _tmpMessage.header.dest == _thisAddress
        || (_tmpMessage.header.dest == RH_BROADCAST_ADDRESS ))
            //&& !(_tmpMessage.header.flags & KL_FLAGS_ARP) )) // don't rebroadcast a broadcast arp
    {
        // Deliver it here
        if (source) *source  = _tmpMessage.header.source;
        if (dest)   *dest    = _tmpMessage.header.dest;
        if (id)     *id      = _tmpMessage.header.id;
        if (flags)  *flags   = _tmpMessage.header.flags;
        Serial.print("Calculating message length from tmpMessageLen: ");
        Serial.print(tmpMessageLen);
        Serial.print(" and sizeof(RoutedMessageHeader: ");
        Serial.println(sizeof(RoutedMessageHeader));
        uint8_t msgLen = tmpMessageLen - sizeof(RoutedMessageHeader);
        if (*len > msgLen) {
            *len = msgLen;
        }
        memcpy(buf, _tmpMessage.data, *len);
        return true; // Its for you!
    }
    else if (   _tmpMessage.header.dest != RH_BROADCAST_ADDRESS
         && _to == _thisAddress // don't reroute something that was not intentionally routed to us
         && getRouteTo(_tmpMessage.header.dest) != _from // a precaution against bouncing but is this needed?
         && _tmpMessage.header.hops++ < _max_hops)
         //&& getRouteTo(_tmpMessage.header.dest)) // don't forward if we don't already have a route
    {
        route(&_tmpMessage, tmpMessageLen);
    }
    // Discard it and maybe wait for another
    }
    return false;
}

bool KnightLab_LoRaRouter::recvfromAckTimeout(uint8_t* buf, uint8_t* len, uint16_t timeout, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    unsigned long starttime = millis();
    int32_t timeLeft;
    while ((timeLeft = timeout - (millis() - starttime)) > 0) {
        if (waitAvailableTimeout(timeLeft)) {
            if (recvfromAck(buf, len, from, to, id, flags)) {
                return true;
            }
        }
        YIELD;
    }
    return false;
}

uint32_t KnightLab_LoRaRouter::retransmissions()
{
    return _retransmissions;
}

void KnightLab_LoRaRouter::resetRetransmissions()
{
    _retransmissions = 0;
}

void KnightLab_LoRaRouter::acknowledge(uint8_t id, uint8_t from, uint8_t ack_code)
{
    setHeaderId(id);
    setHeaderFlags(RH_FLAGS_ACK, 0xFF);
    uint8_t ack = ack_code;
    Serial.print("acknowledge Sending ACK ID: ");
    Serial.print(id);
    Serial.print("; TO: ");
    Serial.print(from);
    Serial.print("; CODE: ");
    Serial.print(ack_code);
    Serial.print("; FLAGS: ");
    Serial.println(RH_FLAGS_ACK, HEX);
    sendto(&ack, sizeof(ack), from);
    waitPacketSent();
}

void KnightLab_LoRaRouter::acknowledgeArp(uint8_t id, uint8_t from)
{
    setHeaderId(id);
    setHeaderFlags(RH_FLAGS_ACK, 0xFF);
    uint8_t buf[255] = { 0 };
    buf[0] = KL_ACK_CODE_NONE;
    uint8_t index = 1;
    for (uint8_t i=1; i<255; i++) {
        if (_static_routes[i]) {
            buf[index++] = i;
            buf[index++] = _static_routes[i];
        } 
        // TODO: better handling of buffer limit
        if (index >= 255) {
            break;
        }
    }
    sendto(buf, index, from);
    waitPacketSent();
}

void KnightLab_LoRaRouter::clearRoutingTable()
{
    Serial.println("Clearing routing table");
    for (uint8_t i=0; i<sizeof(_static_routes); i++) {
        _static_routes[i] = 0;
        _route_signal_strength[i] = 255;
    }
}

void KnightLab_LoRaRouter::broadcastClearRoutingTable()
{
    uint8_t buf[] = { KL_TEST_CONTROL_CLEAR_ROUTES };
    uint8_t len = sizeof(buf);
    routeTestControl(buf, len, RH_BROADCAST_ADDRESS);
}

void KnightLab_LoRaRouter::broadcastModemConfig(RH_RF95::ModemConfigChoice config)
{
    uint8_t buf[] = { KL_TEST_CONTROL_SET_MODEM_CONFIG, config };
    uint8_t len = sizeof(buf);
    routeTestControl(buf, len, RH_BROADCAST_ADDRESS);
}

void KnightLab_LoRaRouter::broadcastRetries(uint8_t retries)
{
    uint8_t buf[] = { KL_TEST_CONTROL_SET_RETRIES, retries };
    uint8_t len = sizeof(buf);
    routeTestControl(buf, len, RH_BROADCAST_ADDRESS);
}

void KnightLab_LoRaRouter::broadcastTimeout(uint16_t timeout)
{
    uint8_t buf[] = { KL_TEST_CONTROL_SET_TIMEOUT, timeout >> 8, timeout & 0xff };
    uint8_t len = sizeof(buf);
    routeTestControl(buf, len, RH_BROADCAST_ADDRESS);
}

void KnightLab_LoRaRouter::broadcastCADTimeout(unsigned long timeout)
{
    uint8_t buf[] = { KL_TEST_CONTROL_SET_CAD_TIMEOUT,
        timeout >> 32, timeout >> 16, timeout >> 8, timeout & 0xff };
    uint8_t len = sizeof(buf);
    routeTestControl(buf, len, RH_BROADCAST_ADDRESS);
}

uint8_t KnightLab_LoRaRouter::getSequenceNumber() {
    return _lastE2ESequenceNumber;
}

/**
 * Request route to dest from send_to. If send_to is broadcast, the first responder with a route
 * will be utilized. Nodes should not respond unless they have a valid route.
 */
uint8_t KnightLab_LoRaRouter::doArp(uint8_t dest) {
    if (dest == 0) {
        routeArp(&dest, 1, 255);
    } else if (!routeArp(&dest, 1, dest)) {
        routeArp(&dest, 1, 255);
    };
    return getRouteTo(dest);
   /*
    if (!routeArp(&dest, 1, dest)) {
        routeArp(&dest, 1, 255);
    };
    return getRouteTo(dest);
    */
}

void KnightLab_LoRaRouter::printRoutingTable()
{
    Serial.println("ROUTING TABLE: ");
    for (uint8_t i=1; i<255; i++) {
        if (_static_routes[i]) {
            Serial.print(i);
            Serial.print(" -> ");
            Serial.print(_static_routes[i]);
            Serial.print(" (");
            Serial.print(getRouteSignalStrength(i));
            Serial.println("dB)");
        }
    }
}

void KnightLab_LoRaRouter::addRouteTo(uint8_t dest, uint8_t next_hop, int16_t rssi)
{
    if (dest == 255) {
        return;
    }
    Serial.print("Adding route TO: ");
    Serial.print(dest);
    Serial.print(" VIA: ");
    Serial.println(next_hop);
    _static_routes[dest] = next_hop;
    setRouteSignalStrength(dest, rssi);
}

void KnightLab_LoRaRouter::setRouteSignalStrength(uint8_t dest, int16_t rssi) {
    // RSSI values will always be <= 0. We compress the values by storing the absolute
    // value to normalize rssi to positive 8-bits. Better strength will now be the lower value
    if (rssi >= 0){
        _route_signal_strength[dest] = 0;
    } else if (rssi <= -255) {
        _route_signal_strength[dest] = 255;
    } else {
        _route_signal_strength[dest] = (uint8_t)(-rssi);
    }
}

int16_t KnightLab_LoRaRouter::getRouteSignalStrength(uint8_t dest) {
    return -((int16_t)_route_signal_strength[dest]);
}

bool KnightLab_LoRaRouter::routingTableIsEmpty()
{
    static uint8_t testblock [255] = {0};
    if (!memcmp(testblock, _static_routes, 255)) {
        return true;
    } else {
        return false;
    }
    /*
    for (int i=0; i<255; i++) {
        if (LoRaRouter->getRouteTo(i) > 0) {
            return false;
        }
    }
    return true;
    */
}

bool KnightLab_LoRaRouter::passesTopologyTest(uint8_t from)
{
    #if RH_TEST_NETWORK==1
        // This network looks like 1-2-3-4
        if ( !(
           (_thisAddress == 1 && from == 2)
        || (_thisAddress == 2 && (from == 1 || from == 3))
        || (_thisAddress == 3 && (from == 2 || from == 4))
        || (_thisAddress == 4 && from == 3))) {
            return false;
        }
    #elif RH_TEST_NETWORK==2
           // This network looks like 1-2-4
           //                         | | |
           //                         --3--
        if ( !(
           (_thisAddress == 1 && (from == 2 || from == 3))
        ||  _thisAddress == 2
        ||  _thisAddress == 3
        || (_thisAddress == 4 && (from == 2 || from == 3))) {
            return false;
        }
    #elif RH_TEST_NETWORK==3
           // This network looks like 1-2-4
           //                         |   |
           //                         --3--
        if ( !(
           (_thisAddress == 1 && (from == 2 || from == 3))
        || (_thisAddress == 2 && (from == 1 || from == 4))
        || (_thisAddress == 3 && (from == 1 || from == 4))
        || (_thisAddress == 4 && (from == 2 || from == 3))) {
            return false;
        }
    #elif RH_TEST_NETWORK==4
           // This network looks like 1-2-3
           //                           |
           //                           4
        if ( !(
           (_thisAddress == 1 && from == 2)
        ||  _thisAddress == 2
        || (_thisAddress == 3 && from == 2)
        || (_thisAddress == 4 && from == 2) ) {
            return false;
        }
    #elif RH_TEST_NETWORK==5
        // This network looks like 1-2-3-4-5
        if ( !(
           (_thisAddress == 1 && from == 2)
        || (_thisAddress == 2 && (from == 1 || from == 3))
        || (_thisAddress == 3 && (from == 2 || from == 4))
        || (_thisAddress == 4 && (from == 3 || from == 5))
        || (_thisAddress == 5 && from == 4))) {
            return false;
        }
    #endif
    return true;
}

/**
 * Receive the next message, be sure it passes the topology test and add the sender to the
 * routing table if appropriate.
 */
bool KnightLab_LoRaRouter::recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    bool ret = RHDatagram::recvfrom(buf, len, from, to, id, flags);
    // check for test controls before conforming to test network topology
    if (*to == RH_BROADCAST_ADDRESS && (*flags & KL_FLAGS_TEST_CONTROL)) {
        Serial.print("Received test control with data: ");
        Serial.println(buf[0]);
        if (buf[0] == KL_TEST_CONTROL_CLEAR_ROUTES) {
            Serial.println("CALLING clearRoutingTable");
            clearRoutingTable();
        } else if (buf[0] == KL_TEST_CONTROL_SET_RETRIES) {
            Serial.print("SETTING RETRIES: ");
            Serial.println(buf[1]);
            setRetries(buf[1]);
        } else if (buf[0] == KL_TEST_CONTROL_SET_TIMEOUT) {
            uint16_t timeout = (buf[1] << 8) | buf[2];
            Serial.print("SETTING TIMEOUT: ");
            Serial.println(timeout);
            setTimeout(timeout);
        } else if (buf[0] == KL_TEST_CONTROL_SET_CAD_TIMEOUT) {
            unsigned long cad_timeout = (buf[1] << 32) | (buf[2] << 16 | buf[3] << 8 | buf[4]);
            Serial.print("SETTING CAD TIMEOUT: ");
            Serial.println(cad_timeout);
            _driver.setCADTimeout(cad_timeout);
        } else if (buf[0] == KL_TEST_CONTROL_SET_MODEM_CONFIG) {
            Serial.print("SETTING MODEM CONFIG: ");
            Serial.println(buf[1]);
            (reinterpret_cast<RH_RF95*>(&_driver))->setModemConfig((RH_RF95::ModemConfigChoice)(buf[1]));
        }
        _seenIds[*from] = *id;
        return false;
    }
    if (!passesTopologyTest(*from)) {
        //Serial.print("REJECTING out-of-range message for testing from NODE: ");
        //Serial.println(*from);
        return false;
    }
    Serial.print("Received message ID: "); Serial.print(*id);
    Serial.print("; FROM: "); Serial.print(*from);
    Serial.print("; LEN: "); Serial.print(*len);
    Serial.print("; TO: "); Serial.print(*to);
    Serial.print("; FLAGS: "); Serial.println(*flags, HEX);
    printRoutingTable();
    // Check every signal that comes in against the routing table to always be adding new
    // single-hop routes as we see them. We prefer single-hop over multi-hop if the RSSI > -80
    // or otherwise is stronger than the multi-hop route.
    // For ACKs that contain routes, also add those routes
    if (ret) {
        uint8_t existing_route = getRouteTo(*from);
        int16_t last_rssi = _driver.lastRssi();
        if (!existing_route
            || (existing_route != *from && (
                        last_rssi > -80
                        || last_rssi > getRouteSignalStrength(*from)) )) {
                addRouteTo(*from, *from, last_rssi);
        }
        // TODO: use a specific ack code rather than depending on the length?
        /*
        if ( (*flags & RH_FLAGS_ACK) && (*len > 1) ) {
            Serial.print("RECEIVED ACK WITH ROUTES. ADDING RECEIVED ROUTES FROM: ");
            Serial.println(*from);
            //uint8_t msg_len = (uint8_t)*len;
            for (uint8_t i = 1; i<*len; i++) {
                uint8_t _dest = buf[i++];
                if (_dest == _thisAddress) {
                    continue;
                }
                existing_route = getRouteTo(_dest);
                if (!existing_route
                    || (existing_route != _dest && (
                        last_rssi > -80
                        || last_rssi > getRouteSignalStrength(_dest)) )) {
                    addRouteTo(_dest, *from, last_rssi);
                }
            }
        }
        printRoutingTable();
        */
    }
    return ret;
}

bool KnightLab_LoRaRouter::routeArp(uint8_t* buf, uint8_t len, uint8_t address)
{
    return routeHelper(buf, len, address, KL_FLAGS_ARP);
}

bool KnightLab_LoRaRouter::routeTestControl(uint8_t* buf, uint8_t len, uint8_t address)
{
    return routeHelper(buf, len, address, KL_FLAGS_TEST_CONTROL);
}

bool KnightLab_LoRaRouter::routeHelper(uint8_t* buf, uint8_t len, uint8_t address, uint8_t flags)
{
    Serial.print("routeHelper received flags: ");
    Serial.println(flags, HEX);
    // Assemble the message
    uint8_t thisSequenceNumber = ++_lastSequenceNumber;
    // Don't use 0 so we can tell if we've received any messages from a given node
    if (thisSequenceNumber == 0) {
        thisSequenceNumber = ++_lastSequenceNumber;
    }
    uint8_t retries = 0;
    do {
    setHeaderId(thisSequenceNumber);
    bool arp = (bool)(flags & KL_FLAGS_ARP);
    Serial.print("Setting header flags to: ");
    Serial.println(flags, HEX);
    setHeaderFlags(flags, 0xFF);
    Serial.print("RouteHelper Sending message LEN: ");
    Serial.print(len);
    Serial.print("; TO: ");
    Serial.print(address);
    Serial.print("; ID: ");
    Serial.print(thisSequenceNumber);
    Serial.print("; FLAGS: ");
    Serial.print(flags, HEX);
    Serial.print("; ARP: ");
    Serial.println(arp);
    // experimental. Was just a blind send. SBB
    if (!sendto(buf, len, address)) {
        return false;
    }
    waitPacketSent();
    // don't wait for ACKS to broadcasts unless it's an arp
    if (!arp && address == RH_BROADCAST_ADDRESS)
        return true;
    if (retries > 1)
        _retransmissions++;
    unsigned long thisSendTime = millis(); // Timeout does not include original transmit time
    // Compute a new timeout, random between _timeout and _timeout*2
    // This is to prevent collisions on every retransmit
    // if 2 nodes try to transmit at the same time
#if (RH_PLATFORM == RH_PLATFORM_RASPI) // use standard library random(), bugs in random(min, max)
    uint16_t timeout = _timeout + (_timeout * (random() & 0xFF) / 256);
#else
    uint16_t timeout = _timeout + (_timeout * random(0, 256) / 256);
#endif
    int32_t timeLeft;
        while ((timeLeft = timeout - (millis() - thisSendTime)) > 0)
    {
        if (waitAvailableTimeout(timeLeft))
        {
        uint8_t _from, _to, _id, _flags;
        uint8_t ackbuf[KL_ROUTER_MAX_MESSAGE_LEN];
        uint8_t acklen = sizeof(ackbuf);
        if (recvfrom(ackbuf, &acklen, &_from, &_to, &_id, &_flags)) // Discards the message
        {
            // Now have a message: is it our ACK?
            if (   (_from == address || address == RH_BROADCAST_ADDRESS)
                    && _to == _thisAddress
                    && (_flags & RH_FLAGS_ACK)
                    && (_id == thisSequenceNumber)) {
                uint8_t ack_code = ackbuf[0];
                Serial.print("Received ACK id: ");
                Serial.print(_id);
                Serial.print(" with code: ");
                Serial.println(ack_code);
                if (ack_code == KL_ACK_CODE_ERROR_NO_ROUTE) {
                    return false;
                }
                // Its the ACK we are waiting for
                // if it's an ack to an arp, we should record the route
                // and any additional routes sent
                /** is this being used?? SBB */
                /*
                if (arp) {
                    Serial.println("Received ACK for ARP");
                    addRouteTo(buf[0], _from, _driver.lastRssi());
                    //Serial.println("RECEIVED ACK TO ARP. ADDING RECEIVED ROUTES");
                    //for (uint8_t i = 1; i<acklen; i++) {
                    //    Serial.print(ackbuf[i++]);
                    //    Serial.print("--->");
                    //    Serial.println(ackbuf[i]);
                    //}
                }
                */
                return true;
            } else if ( arp && _to == RH_BROADCAST_ADDRESS) {
                // this is an "I am here" broadcast response to our arp request
                // route was added when signal received
                //addRouteTo(from, from, _driver.lastRssi());
                return true;
            //} else if ( arp & _to == _thisAddress) {
            //    Serial.print("*****  RECEIVED DIRECT ARP REQUEST FROM ");
            //    Serial.print(_from);
            //    Serial.println(" ******");
            //    return true;
            } else if (   !(_flags & RH_FLAGS_ACK)
                && (_id == _seenIds[_from])) {
                // This is a request we have already received. ACK it again
                acknowledge(_id, _from);
            }
            // Else discard it
        }
        }
        // Not the one we are waiting for, maybe keep waiting until timeout exhausted
        YIELD;
    }
    // Timeout exhausted, maybe retry
    YIELD;
    } while (++retries < _retries && address != RH_BROADCAST_ADDRESS);
    // Retries exhausted
    return false;
}

////////////////////////////////////////////////////////////////////
uint8_t KnightLab_LoRaRouter::route(RoutedMessage* message, uint8_t messageLen)
{
    bool discover_routes = true; // waiting for arpable reliable datagram
    // Reliably deliver it if possible. See if we have a route:
    uint8_t next_hop = RH_BROADCAST_ADDRESS;
    if (message->header.dest != RH_BROADCAST_ADDRESS) {
        uint8_t route = getRouteTo(message->header.dest);
        if (!route) {
            if (discover_routes) {
                Serial.print("No route to dest: ");
                Serial.print(message->header.dest);
                Serial.println(". Attempting to discover route");
                route = doArp(message->header.dest);
            }
            if (!route) {
                return RH_ROUTER_ERROR_NO_ROUTE;
            }
        }
        next_hop = route;
    }
    if (!routeHelper((uint8_t*)message, messageLen, next_hop, 0x00)) {
        return RH_ROUTER_ERROR_UNABLE_TO_DELIVER;
    }
    return RH_ROUTER_ERROR_NONE;
}

/*
void KnightLab_LoRaRouter::requestRoutes(uint8_t address) {
    uint8_t sendbuf[] = "RR";
    sendtoWait(sendbuf, sizeof(sendbuf), address, KL_FLAGS_SEND_ROUTES);
    uint8_t buf[255] = { 0 };
    uint8_t len = 255;
    uint8_t source;
    uint8_t dest;
    if (recvfromAckTimeout(buf, &len, 2000, &source, &dest)) {
        Serial.print("RECEIVED ROUTES FROM: ");
        Serial.println(source);
        uint8_t source_route = getRouteTo(source);
        int16_t last_rssi = _driver.lastRssi();
        uint8_t existing_route = 0;
        for (uint8_t i = 1; i<len; i++) {
            uint8_t _dest = buf[i++];
            if (_dest == _thisAddress) {
                continue;
            }
            existing_route = getRouteTo(_dest);
            if (!existing_route
                || (existing_route != _dest && (
                    last_rssi > -80
                    || last_rssi > getRouteSignalStrength(_dest)) )) {
                    addRouteTo(_dest, source_route, last_rssi);
            }
        }
        printRoutingTable();
    }
}
*/

/*
void KnightLab_LoRaRouter::discoverAllRoutes()
{
    uint8_t visited[255] = {0};
    bool clean_pass = false;
    // send our own beacon to attach to the network
    uint8_t resp = doArp(0);
    Serial.println("Beginning fetch of routes from known nodes");
    printRoutingTable();
    do {
        clean_pass = true;
        for (uint8_t i=1; i<255; i++) {
            if (i == _thisAddress) {
                continue;
            }
            if (!visited[i] && getRouteTo(i)) {
                Serial.print("FETCHING ROUTES FROM: ");
                Serial.println(i);
                requestRoutes(i);
                visited[i] = 1;
                clean_pass = false;
            }
        }
    } while (!clean_pass);
}
*/
