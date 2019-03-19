// KnightLab_LoRaRouter.cpp

// Based on Mike McCauley's RHReliableDatagram but too many core changes to subclass

#include <KnightLab_LoRaRouter.h>

KnightLab_LoRaRouter::RoutedMessage KnightLab_LoRaRouter::_tmpMessage;
KnightLab_LoRaRouter::RoutedMessage KnightLab_LoRaRouter::_arpMessage;

////////////////////////////////////////////////////////////////////
// Constructors
KnightLab_LoRaRouter::KnightLab_LoRaRouter(RHGenericDriver& driver, uint8_t thisAddress)
    : RHDatagram(driver, thisAddress)
{
    //_max_hops = RH_DEFAULT_MAX_HOPS;
    _retransmissions = 0;
    _lastSequenceNumber = 0;
    _timeout = RH_DEFAULT_TIMEOUT;
    _retries = RH_DEFAULT_RETRIES;
    //_last_received_from_id = 0;
    // 0 indicates no messages received from that node. Thus _lastSequenceNumber increment is
    // modified not to use 0
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
    //uint8_t i;
    //for (i = 0; i < RH_ROUTING_TABLE_SIZE; i++)
	//if (_routes[i].dest == dest && _routes[i].state != Invalid)
	//    return &_routes[i];
    //return NULL;
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
    _tmpMessage.header.flags = flags;
    memcpy(_tmpMessage.data, buf, len);
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
        // ACK
        if (_flags & RH_FLAGS_ACK) {
            if (_to == RH_BROADCAST_ADDRESS) { // A broadcast ACK is an "I am here" message
                if (!getRouteTo(_from)) {
                    addRouteTo(_from, _from);
                    // TODO: track the rssi value in case we find a better route later
                }
            } 
            return false;
        } // Never ACK an ACK
        // ARP
        if (_flags & KL_FLAGS_ARP) {
            setHeaderFlags(RH_FLAGS_NONE, KL_FLAGS_ARP); // Clear the ARP flag before acking
            if (_to == _thisAddress) {
                //acknowledge(_id, _from, &_thisAddress, 1);
                //acknowledge(_id, _from);
                acknowledge(_id, RH_BROADCAST_ADDRESS); // let everybody know I'm here, not just the requestor
            } else if (_to == RH_BROADCAST_ADDRESS && getRouteTo(buf[0])> 0) {
                acknowledge(_id, _from);
                //acknowledge(_id, _from, &(_seenIds[_from]), 1);
                // we need to flatten this structure in order for an arp ack to access routing info
                //acknowledgeArp(_id, _from);
            }
            return false; // done processing ARP
        } else if (_to ==_thisAddress) {
            acknowledge(_id, _from);
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
    //if (RHReliableDatagram::recvfromAck((uint8_t*)&_tmpMessage, &tmpMessageLen, &_from, &_to, &_id, &_flags))
    if (recvfromAckHelper((uint8_t*)&_tmpMessage, &tmpMessageLen, &_from, &_to, &_id, &_flags))
    {
	// Here we simulate networks with limited visibility between nodes
	// so we can test routing
/*
#ifdef RH_TEST_NETWORK
	if (
#if RH_TEST_NETWORK==1
	    // This network looks like 1-2-3-4
	       (_thisAddress == 1 && _from == 2)
	    || (_thisAddress == 2 && (_from == 1 || _from == 3))
	    || (_thisAddress == 3 && (_from == 2 || _from == 4))
	    || (_thisAddress == 4 && _from == 3)
#elif RH_TEST_NETWORK==2
	       // This network looks like 1-2-4
	       //                         | | |
	       //                         --3--
	       (_thisAddress == 1 && (_from == 2 || _from == 3))
	    ||  _thisAddress == 2
	    ||  _thisAddress == 3
	    || (_thisAddress == 4 && (_from == 2 || _from == 3))
#elif RH_TEST_NETWORK==3
	       // This network looks like 1-2-4
	       //                         |   |
	       //                         --3--
	       (_thisAddress == 1 && (_from == 2 || _from == 3))
	    || (_thisAddress == 2 && (_from == 1 || _from == 4))
	    || (_thisAddress == 3 && (_from == 1 || _from == 4))
	    || (_thisAddress == 4 && (_from == 2 || _from == 3))
#elif RH_TEST_NETWORK==4
	       // This network looks like 1-2-3
	       //                           |
	       //                           4
	       (_thisAddress == 1 && _from == 2)
	    ||  _thisAddress == 2
	    || (_thisAddress == 3 && _from == 2)
	    || (_thisAddress == 4 && _from == 2)
#endif
)
	{
	    // OK
	}
	else
	{
		Serial.print("Ignoring message from ");
		Serial.print(_from);
		Serial.println(" for testing purposes.");
	    return false; // Pretend we got nothing
	}
#endif
*/
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
		 && getRouteTo(_tmpMessage.header.dest) != _from // a precaution against bouncing but is this needed?
		 && _tmpMessage.header.hops++ < _max_hops)
         //&& getRouteTo(_tmpMessage.header.dest)) // don't forward if we don't already have a route
	{
		/**
		 * In this condition, we've added a check that the route to the destination is not
		 * the same as the node this message just came from in order to avoid merely bouncing
		 * messages. A future route discovery protocol might require us to rethink this. -SBB
		 */
		Serial.print("Routing message to: ");
		Serial.print(_tmpMessage.header.dest);
		Serial.print(" via ");
		Serial.println(getRouteTo(_tmpMessage.header.dest));
        Serial.print("WITH FLAGS: ");
        Serial.println(_tmpMessage.header.flags);
	    // Maybe it has to be routed to the next hop
	    // REVISIT: if it fails due to no route or unable to deliver to the next hop, 
	    // tell the originator. BUT HOW?
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
    while ((timeLeft = timeout - (millis() - starttime)) > 0)
    {
    if (waitAvailableTimeout(timeLeft))
    {
        if (recvfromAck(buf, len, from, to, id, flags))
        return true;
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

void KnightLab_LoRaRouter::acknowledge(uint8_t id, uint8_t from)
{
    setHeaderId(id);
    setHeaderFlags(RH_FLAGS_ACK);
    // We would prefer to send a zero length ACK,
    // but if an RH_RF22 receives a 0 length message with a CRC error, it will never receive
    // a 0 length message again, until its reset, which makes everything hang :-(
    // So we send an ACK of 1 octet
    // REVISIT: should we send the RSSI for the information of the sender?
    uint8_t ack = '!';
    Serial.print("Sending ACK ID: ");
    Serial.print(id);
    Serial.print("; TO: ");
    Serial.println(from);
    sendto(&ack, sizeof(ack), from);
    waitPacketSent();
}

void KnightLab_LoRaRouter::clearRoutingTable()
{
	for (uint8_t i=0; i<sizeof(_static_routes); i++) {
		_static_routes[i] = 0;
	}
}

//void KnightLab_LoRaRouter::initializeAllRoutes()
//{
//	for (uint8_t i=0; i<sizeof(_static_routes); i++) {
//		_static_routes[i] = i;
//	}
//}

uint8_t KnightLab_LoRaRouter::getSequenceNumber() {
	return _lastE2ESequenceNumber;
}

/**
 * Request route to dest from send_to. If send_to is broadcast, the first responder with a route
 * will be utilized. Nodes should not respond unless they have a valid route.
 */
uint8_t KnightLab_LoRaRouter::doArp(uint8_t dest) {
    if (!routeHelper(&dest, 1, dest, true)) {
        routeHelper(&dest, 1, 255, true);
    };
    return getRouteTo(dest);
}

void KnightLab_LoRaRouter::addRouteTo(uint8_t dest, uint8_t next_hop)
{
    Serial.print("Adding route TO: ");
    Serial.print(dest);
    Serial.print(" VIA: ");
    Serial.println(next_hop);
	_static_routes[dest] = next_hop;
    Serial.println("ROUTING TABLE: ");
    for (uint8_t i=1; i<255; i++) {
        if (_static_routes[i]) {
            Serial.print(i);
            Serial.print(" -> ");
            Serial.println(_static_routes[i]);
        }
    }
}

bool KnightLab_LoRaRouter::recvfrom(uint8_t* buf, uint8_t* len, uint8_t* from, uint8_t* to, uint8_t* id, uint8_t* flags)
{
    Serial.println("KnightLab_LoRaRouter::recvfrom");
    bool ret = RHDatagram::recvfrom(buf, len, from, to, id, flags);

    #if RH_TEST_NETWORK==1
	    // This network looks like 1-2-3-4
        uint8_t _from = *from;
        if ( !(
	       (_thisAddress == 1 && _from == 2)
	    || (_thisAddress == 2 && (_from == 1 || _from == 3))
	    || (_thisAddress == 3 && (_from == 2 || _from == 4))
	    || (_thisAddress == 4 && _from == 3))) {
            return false;
        }
    #endif

    /*
    if (!getRouteTo(*from)) {
        addRouteTo(*from, *from);
    }
    */

    Serial.print("Received message ID: ");
    Serial.print(*id);
    Serial.print("; FROM: ");
    Serial.print(*from);
    Serial.print("; LEN: ");
    Serial.print(*len);
    Serial.print("; TO: ");
    Serial.print(*to);
    Serial.print("; FLAGS: ");
    Serial.println(*flags, HEX);
    return ret;
}

/// c-v from RHReliableDatagram
/**
 * typically handled in a reliable datagram subclass.
 */
bool KnightLab_LoRaRouter::routeHelper(uint8_t* buf, uint8_t len, uint8_t address, bool arp)
{
    // Assemble the message
    uint8_t thisSequenceNumber = ++_lastSequenceNumber;
    // Don't use 0 so we can tell if we've received any messages from a given node
    if (thisSequenceNumber == 0) {
        thisSequenceNumber = ++_lastSequenceNumber;
    }
    uint8_t retries = 0;
    //while (retries++ <= _retries)
    do {
    setHeaderId(thisSequenceNumber);
    if (arp) {
        setHeaderFlags(KL_FLAGS_ARP, RH_FLAGS_ACK); // Set the ARP flag, clear the ACK flag
    } else {
        setHeaderFlags(RH_FLAGS_NONE, RH_FLAGS_ACK); // Clear the ACK flag
    }
    Serial.print("Sending message LEN: ");
    Serial.print(len);
    Serial.print("; TO: ");
    Serial.print(address);
    Serial.print("; ARP: ");
    Serial.println(arp);
    sendto(buf, len, address);
    waitPacketSent();
    Serial.println("DONE");
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
        uint8_t from, to, id, flags;
        if (recvfrom(0, 0, &from, &to, &id, &flags)) // Discards the message
        {
            // Now have a message: is it our ACK?
            if (   (from == address || address == RH_BROADCAST_ADDRESS)
                    && to == _thisAddress
                    && (flags & RH_FLAGS_ACK)
                    && (id == thisSequenceNumber)) {
                // Its the ACK we are waiting for
                // if it's an ack to an arp, we should record the route
                if (arp) {
                    addRouteTo(buf[0], from);
                }
                return true;
            } else if ( arp && to == RH_BROADCAST_ADDRESS) {
                // this is an "I am here" broadcast response to our arp request
                addRouteTo(from, from);
                return true;
            } else if (   !(flags & RH_FLAGS_ACK)
                && (id == _seenIds[from])) {
                // This is a request we have already received. ACK it again
                acknowledge(id, from);
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
			Serial.print("No route to dest: ");
			Serial.println(message->header.dest);
			if (discover_routes) {
                Serial.println("Attempting to discover route");
                route = doArp(message->header.dest);
            }
            if (!route) {
	    		return RH_ROUTER_ERROR_NO_ROUTE;
            }
		}
		next_hop = route;
    }
    if (!routeHelper((uint8_t*)message, messageLen, next_hop)) {
		return RH_ROUTER_ERROR_UNABLE_TO_DELIVER;
	}
    return RH_ROUTER_ERROR_NONE;
}