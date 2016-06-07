#include "opendefs.h"
#include "forwarding.h"
#include "iphc.h"
#include "dag.h"
#include "openqueue.h"
#include "openserial.h"
#include "idmanager.h"
#include "packetfunctions.h"
#include "neighbors.h"
#include "icmpv6.h"
#include "icmpv6rpl.h"
#include "openudp.h"
#include "opentcp.h"
#include "debugpins.h"
#include "scheduler.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

void      forwarding_getNextHop(
   open_addr_t*         destination,
   open_addr_t*         addressToWrite
);
owerror_t forwarding_send_internal_RoutingTable(
   OpenQueueEntry_t*    msg,
   ipv6_header_iht*     ipv6_outer_header,
   ipv6_header_iht*     ipv6_inner_header,
   rpl_option_ht*       rpl_option,
   uint32_t*             flow_label,
   uint8_t              fw_SendOrfw_Rcv
);
owerror_t forwarding_send_internal_SourceRouting(
   OpenQueueEntry_t*    msg,
   ipv6_header_iht*     ipv6_outer_header,
   ipv6_header_iht*     ipv6_inner_header,
   rpl_option_ht*       rpl_option
);
owerror_t forwarding_send_down_SourceRouting(
   OpenQueueEntry_t*    msg,
   ipv6_header_iht*     ipv6_outer_header,
   ipv6_header_iht*     ipv6_inner_header,
   rpl_option_ht*       rpl_option,
   dag_route_t*         srcRoute
);
void      forwarding_createRplOption(
   rpl_option_ht*       rpl_option,
   uint8_t              flags
);
void      forwarding_optimize128bForIphc(
   open_addr_t* src128b,
   open_addr_t* dst128b,
   open_addr_t* srcOpt,
   open_addr_t* dstOpt
);


//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void forwarding_init() {
}

/**
\brief Send a packet originating at this mote up to the DAG root.

This function is called by an upper layer, and only concerns packets originated
at this mote.

\param[in,out] msg Packet to send.
*/
owerror_t forwarding_send_up(OpenQueueEntry_t* msg) {
    ipv6_header_iht      ipv6_outer_header;
    ipv6_header_iht      ipv6_inner_header;
    rpl_option_ht        rpl_option;
    open_addr_t*         myprefix;
    open_addr_t*         myadd64;
    uint32_t             flow_label = 0;

    open_addr_t          temp_dest_host;
    open_addr_t*         p_dest;
    open_addr_t*         p_src;  
    open_addr_t          temp_src_host; 
    uint8_t              sam;
    uint8_t              m;
    uint8_t              dam;

    // take ownership over the packet
    msg->owner                = COMPONENT_FORWARDING;

    m   = IPHC_M_NO;

    // retrieve my prefix and EUI64
    myprefix                  = idmanager_getMyID(ADDR_PREFIX);
    myadd64                   = idmanager_getMyID(ADDR_64B);

    // set source address (me)
    msg->l3_sourceAdd.type=ADDR_128B;
    memcpy(&(msg->l3_sourceAdd.addr_128b[0]),myprefix->prefix,8);
    memcpy(&(msg->l3_sourceAdd.addr_128b[8]),myadd64->addr_64b,8);

    // initialize IPv6 header
    memset(&ipv6_outer_header,0,sizeof(ipv6_header_iht));
    memset(&ipv6_inner_header,0,sizeof(ipv6_header_iht));

    // Set hop limit to the default in-network value as this packet is being
    // sent from upper layer. This is done here as send_internal() is used by
    // forwarding of packets as well which carry a hlim. This value is required
    // to be set to a value as the following function can decrement it.
    ipv6_inner_header.hop_limit     = IPHC_DEFAULT_HOP_LIMIT;

    // create the RPL hop-by-hop option

    forwarding_createRplOption(
      &rpl_option,      // rpl_option to fill in
      0x00              // flags
    );

    forwarding_optimize128bForIphc(&(msg->l3_sourceAdd), &(msg->l3_destinationAdd),
                                   &temp_src_host, &temp_dest_host);

    if (temp_src_host.type==ADDR_128B) {
        // Expecting same prefix since originating at DAG root
        openserial_printError(
            COMPONENT_FORWARDING,ERR_PREFIXES_DIFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return E_FAIL;
    }

    if (temp_src_host.type==ADDR_16B) {
        sam = IPHC_SAM_16B;
        dam = IPHC_DAM_16B;
        p_src  = &temp_src_host;
        p_dest = &temp_dest_host;
    } else if (temp_src_host.type==ADDR_64B) {
        sam = IPHC_SAM_64B;
        dam = IPHC_DAM_64B;
        p_src  = &temp_src_host;
        p_dest = &temp_dest_host;
    } else {
        //not the same prefix. so the packet travels to another network
        //check if this is a source routing pkt. in case it is then the DAM is elided as it is in the SrcRouting header.
        if (packetfunctions_isBroadcastMulticast(&(msg->l3_destinationAdd))==FALSE){
            sam = IPHC_SAM_128B;
            dam = IPHC_DAM_128B;
            p_dest = &(msg->l3_destinationAdd);
            p_src = &(msg->l3_sourceAdd);
            
            ipv6_outer_header.src.type = ADDR_128B;
            memcpy(&ipv6_outer_header.src,p_src,sizeof(open_addr_t));
            ipv6_outer_header.hop_limit = IPHC_DEFAULT_HOP_LIMIT;
        } else {
           // this is DIO, source address elided, multicast bit is set
            sam = IPHC_SAM_ELIDED;
            m   = IPHC_M_YES;
            dam = IPHC_DAM_ELIDED;
            p_dest = &(msg->l3_destinationAdd);
            p_src = &(msg->l3_sourceAdd);
        }
    }
    //IPHC inner header and NHC IPv6 header will be added at here
    iphc_prependIPv6Header(msg,
                IPHC_TF_ELIDED,
                flow_label,                   // value_flowlabel
                IPHC_NH_INLINE,
                msg->l4_protocol, 
                IPHC_HLIM_64,
                ipv6_outer_header.hop_limit,  // irrelevant since IPHC_HLIM_64
                IPHC_CID_NO,
                IPHC_SAC_STATELESS,
                sam,
                m,
                IPHC_DAC_STATELESS,
                dam,
                p_dest,
                p_src,            
                PCKTSEND  
                );
    // both of them are compressed
    ipv6_outer_header.next_header_compressed = TRUE;

    return forwarding_send_internal_RoutingTable(
        msg,
        &ipv6_outer_header,
        &ipv6_inner_header,
        &rpl_option,
        &flow_label,
        PCKTSEND
    );
}


/**
\brief Send a packet originating at the DAG root down into the mesh.

This function is called by an upper layer, and only concerns packets originated
at this mote.

\param[in,out] msg Packet to send.
\param[in] srcRoute Path to destination, ordered from destination to 1st hop
*/
owerror_t forwarding_send_down(OpenQueueEntry_t* msg, dag_route_t* srcRoute) {
    ipv6_header_iht      ipv6_outer_header;
    ipv6_header_iht      ipv6_inner_header;
    rpl_option_ht        rpl_option;
    open_addr_t*         myprefix;
    open_addr_t*         myadd64;
    uint32_t             flow_label = 0;

    open_addr_t          temp_dest_host;
    open_addr_t*         p_dest;
    open_addr_t*         p_src;  
    open_addr_t          temp_src_host; 
    uint8_t              sam;
    uint8_t              m;
    uint8_t              dam;

    // take ownership over the packet
    msg->owner                = COMPONENT_FORWARDING;

    m   = IPHC_M_NO;

    // retrieve my prefix and EUI64
    myprefix                  = idmanager_getMyID(ADDR_PREFIX);
    myadd64                   = idmanager_getMyID(ADDR_64B);

    // set source address (me)
    msg->l3_sourceAdd.type=ADDR_128B;
    memcpy(&(msg->l3_sourceAdd.addr_128b[0]),myprefix->prefix,8);
    memcpy(&(msg->l3_sourceAdd.addr_128b[8]),myadd64->addr_64b,8);

    // initialize IPv6 header
    memset(&ipv6_outer_header,0,sizeof(ipv6_header_iht));
    memset(&ipv6_inner_header,0,sizeof(ipv6_header_iht));

    // Set hop limit to the default in-network value as this packet is being
    // sent from upper layer. This is done here as send_internal() is used by
    // forwarding of packets as well which carry a hlim. This value is required
    // to be set to a value as the following function can decrement it.
    ipv6_inner_header.hop_limit     = IPHC_DEFAULT_HOP_LIMIT;

    // Create the RPL hop-by-hop option. Lower layers require it to be defined.
    forwarding_createRplOption(
      &rpl_option,      // rpl_option to fill in
      0x00              // flags
    );
    // However, we trick iphc_sendFromForwarding() into not writing the hop-by-hop
    // header, which is not used for a downward route.
    rpl_option.optionType = 0;

    forwarding_optimize128bForIphc(&(msg->l3_sourceAdd), &(msg->l3_destinationAdd),
                                   &temp_src_host, &temp_dest_host);

    if (temp_src_host.type==ADDR_128B) {
        // Expecting same prefix since originating at DAG root
        openserial_printError(
            COMPONENT_FORWARDING,ERR_PREFIXES_DIFFER,
            (errorparameter_t)0,
            (errorparameter_t)0
        );
        return E_FAIL;
    }

    if (temp_src_host.type==ADDR_16B) {
        sam = IPHC_SAM_16B;
        dam = IPHC_DAM_16B;
    } else if (temp_src_host.type==ADDR_64B) {
        sam = IPHC_SAM_64B;
        dam = IPHC_DAM_64B;
    }
    p_src  = &temp_src_host;
    p_dest = &temp_dest_host;

    //IPHC inner header and NHC IPv6 header will be added at here
    iphc_prependIPv6Header(msg,
                IPHC_TF_ELIDED,
                flow_label, // value_flowlabel
                IPHC_NH_INLINE,
                msg->l4_protocol, 
                IPHC_HLIM_64,
                ipv6_outer_header.hop_limit,
                IPHC_CID_NO,
                IPHC_SAC_STATELESS,
                sam,
                m,
                IPHC_DAC_STATELESS,
                dam,
                p_dest,
                p_src,            
                PCKTSEND  
                );
    // both of them are compressed
    ipv6_outer_header.next_header_compressed = TRUE;

    if (srcRoute->hopCount > 1) {
        return forwarding_send_down_SourceRouting(
            msg,
            &ipv6_outer_header,
            &ipv6_inner_header,
            &rpl_option,
            srcRoute
        );
    } else {
        return forwarding_send_internal_RoutingTable(
            msg,
            &ipv6_outer_header,
            &ipv6_inner_header,
            &rpl_option,
            &flow_label,
            PCKTSEND
        );
    }
}

/**
\brief Send a packet originating at this mote.

This function is called by an upper layer, and only concerns packets originated
at this mote.

\param[in,out] msg Packet to send.
*/
owerror_t forwarding_send(OpenQueueEntry_t* msg) {
   dag_route_t route;
    
   if (idmanager_getIsDAGroot()==TRUE) {
      // Allow sending application messages down the mesh.
      if (msg->l4_protocol==IANA_UDP) {
         if (dag_buildRoute(&route, &msg->l3_destinationAdd) == E_SUCCESS) {
            return forwarding_send_down(msg, &route);
         } else {
            return E_FAIL;
         }
      } else if (msg->l4_protocol==IANA_ICMPv6) {
         // Really this more like send *out*, rather than up, for RPL DIOs.
         return forwarding_send_up(msg);
      }
   } else {
      return forwarding_send_up(msg);
   }
   return E_FAIL;
}

/**
\brief Indicates a packet has been sent.

\param[in,out] msg   The packet just sent.
\param[in]     error The outcome of sending it.
*/
void forwarding_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   
   // take ownership
   msg->owner = COMPONENT_FORWARDING;
   
   if (msg->creator==COMPONENT_RADIO || msg->creator==COMPONENT_FORWARDING) {
      // this is a relayed packet
      
      // free packet
      openqueue_freePacketBuffer(msg);
   } else {
      // this is a packet created by this mote
      
      // indicate sendDone to upper layer
      switch(msg->l4_protocol) {
         case IANA_TCP:
            opentcp_sendDone(msg,error);
            break;
         case IANA_UDP:
            openudp_sendDone(msg,error);
            break;
         case IANA_ICMPv6:
            icmpv6_sendDone(msg,error);
            break;
         default:
            
            // log error
            openserial_printCritical(
               COMPONENT_FORWARDING,
               ERR_WRONG_TRAN_PROTOCOL,
               (errorparameter_t)msg->l4_protocol,
               (errorparameter_t)0
            );
            
            // free packet
            openqueue_freePacketBuffer(msg);
      }
   }
}

/**
\brief Indicates a packet was received.

\param[in,out] msg               The packet just sent.
\param[in]     ipv6_header       The information contained in the IPv6 header.
\param[in]     ipv6_hop_header   The hop-by-hop header present in the packet.
\param[in]     rpl_option        The hop-by-hop options present in the packet.
*/
void forwarding_receive(
      OpenQueueEntry_t*      msg,
      ipv6_header_iht*       ipv6_outer_header,
      ipv6_header_iht*       ipv6_inner_header,
      rpl_option_ht*         rpl_option
    ) {
    uint8_t flags;
    uint16_t senderRank;
   
    // take ownership
    msg->owner                     = COMPONENT_FORWARDING;
   
    // determine L4 protocol
    // get information from ipv6_header
    msg->l4_protocol            = ipv6_inner_header->next_header;
    msg->l4_protocol_compressed = ipv6_inner_header->next_header_compressed;
   
    // populate packets metadata with L3 information
    memcpy(&(msg->l3_destinationAdd),&ipv6_inner_header->dest, sizeof(open_addr_t));
    memcpy(&(msg->l3_sourceAdd),     &ipv6_inner_header->src,  sizeof(open_addr_t));
   
    if (
        (
            idmanager_isMyAddress(&(msg->l3_destinationAdd))
            ||
            packetfunctions_isBroadcastMulticast(&(msg->l3_destinationAdd))
        )
        &&
        ipv6_outer_header->next_header!=IANA_IPv6ROUTE
    ) {
        if (ipv6_outer_header->src.type != ADDR_NONE || ipv6_outer_header->rhe_length){
            packetfunctions_tossHeader(msg,ipv6_outer_header->header_length + ipv6_outer_header->rhe_length);
        }
        // this packet is for me, no source routing header // toss iphc inner header
        packetfunctions_tossHeader(msg,ipv6_inner_header->header_length);
        // indicate received packet to upper layer
        switch(msg->l4_protocol) {
        case IANA_TCP:
            opentcp_receive(msg);
            break;
        case IANA_UDP:
            openudp_receive(msg);
            break;
        case IANA_ICMPv6:
            icmpv6_receive(msg);
            break;
        default:
            // log error
            openserial_printError(
                COMPONENT_FORWARDING,ERR_WRONG_TRAN_PROTOCOL,
                (errorparameter_t)msg->l4_protocol,
                (errorparameter_t)1
            );
            
            // free packet
            openqueue_freePacketBuffer(msg);
        }
    } else {
        // this packet is not for me: relay
      
        // change the creator of the packet
        msg->creator = COMPONENT_FORWARDING;
      
        if (ipv6_outer_header->next_header!=IANA_IPv6ROUTE) {
            flags = rpl_option->flags;
            senderRank = rpl_option->senderRank;
            if ((flags & O_FLAG)!=0){
                // wrong direction
                // log error
                openserial_printError(
                    COMPONENT_FORWARDING,
                    ERR_WRONG_DIRECTION,
                    (errorparameter_t)flags,
                    (errorparameter_t)senderRank
                );
            }
            if (senderRank < neighbors_getMyDAGrank()){
                // loop detected
                // set flag
                rpl_option->flags |= R_FLAG;
                // log error
                openserial_printError(
                    COMPONENT_FORWARDING,
                    ERR_LOOP_DETECTED,
                    (errorparameter_t) senderRank,
                    (errorparameter_t) neighbors_getMyDAGrank()
                );
            }
            forwarding_createRplOption(rpl_option, rpl_option->flags);
            // resend as if from upper layer
            if (
                forwarding_send_internal_RoutingTable(
                    msg,
                    ipv6_outer_header,
                    ipv6_inner_header,
                    rpl_option,
                    &(ipv6_outer_header->flow_label),
                    PCKTFORWARD 
                )==E_FAIL
            ) {
                openqueue_freePacketBuffer(msg);
            }
        } else {
            // source routing header present
            if (
                forwarding_send_internal_SourceRouting(
                    msg,
                    ipv6_outer_header,
                    ipv6_inner_header,
                    rpl_option
                )==E_FAIL
            ) {
                // log error
                openserial_printError(
                    COMPONENT_FORWARDING,
                    ERR_INVALID_FWDMODE,
                    (errorparameter_t)0,
                    (errorparameter_t)0
                );
            }
        }
    }
}

//=========================== private =========================================

/**
\brief Retrieve the next hop's address from routing table.

\param[in]  destination128b  Final IPv6 destination address.
\param[out] addressToWrite64b Location to write the EUI64 of next hop to.
*/
void forwarding_getNextHop(open_addr_t* destination128b, open_addr_t* addressToWrite64b) {
   uint8_t         i;
   open_addr_t     temp_prefix64btoWrite;
   
   if (packetfunctions_isBroadcastMulticast(destination128b)) {
      // IP destination is broadcast, send to 0xffffffffffffffff
      addressToWrite64b->type = ADDR_64B;
      for (i=0;i<8;i++) {
         addressToWrite64b->addr_64b[i] = 0xff;
      }
   } else if (neighbors_isStableNeighbor(destination128b)) {
      // IP destination is 1-hop neighbor, send directly
      packetfunctions_ip128bToMac64b(destination128b,&temp_prefix64btoWrite,addressToWrite64b);
   } else {
      // destination is remote, send to preferred parent
      neighbors_getPreferredParentEui64(addressToWrite64b);
   }
}

/**
\brief Send a packet using the routing table to find the next hop.

\param[in,out] msg             The packet to send.
\param[in]     ipv6_header     The packet's IPv6 header.
\param[in]     rpl_option      The hop-by-hop option to add in this packet.
\param[in]     flow_label      The flowlabel to add in the 6LoWPAN header.
\param[in]     fw_SendOrfw_Rcv The packet is originating from this mote
   (PCKTSEND), or forwarded (PCKTFORWARD).
*/
owerror_t forwarding_send_internal_RoutingTable(
      OpenQueueEntry_t*      msg,
      ipv6_header_iht*       ipv6_outer_header,
      ipv6_header_iht*       ipv6_inner_header,
      rpl_option_ht*         rpl_option,
      uint32_t*              flow_label,
      uint8_t                fw_SendOrfw_Rcv
   ) {
   
   // retrieve the next hop from the routing table
   forwarding_getNextHop(&(msg->l3_destinationAdd),&(msg->l2_nextORpreviousHop));
   if (msg->l2_nextORpreviousHop.type==ADDR_NONE) {
      openserial_printError(
         COMPONENT_FORWARDING,
         ERR_NO_NEXTHOP,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return E_FAIL;
   }
   
   if (ipv6_outer_header->src.type != ADDR_NONE){
      packetfunctions_tossHeader(msg,ipv6_outer_header->header_length);
   }
   
   // send to next lower layer
   return iphc_sendFromForwarding(
      msg,
      ipv6_outer_header,
      ipv6_inner_header,
      rpl_option,
      flow_label,
      NULL,  // no rh3
      0,
      fw_SendOrfw_Rcv
   );
}

/**
\brief Send a packet using the source rout to find the next hop.

\note This is always called for packets being forwarded.

How to process the routing header is detailed in
http://tools.ietf.org/html/rfc6554#page-9.

\param[in,out] msg             The packet to send.
\param[in]     ipv6_header     The packet's IPv6 header.
*/
owerror_t forwarding_send_internal_SourceRouting(
    OpenQueueEntry_t* msg,
    ipv6_header_iht*  ipv6_outer_header,
    ipv6_header_iht*  ipv6_inner_header,
    rpl_option_ht*    rpl_option
    ) {
    uint8_t              temp_8b;
    uint8_t              type;
    uint8_t              next_type;
    uint8_t              size;
    uint8_t              next_size;
    uint8_t              hlen;
    open_addr_t          firstAddr;
    open_addr_t          nextAddr;
    open_addr_t          temp_prefix;
    open_addr_t          temp_addr64;
    
    uint8_t              rpi_length;
    uint8_t              flags;
    uint16_t             senderRank;
    
    uint8_t              RH_copy[127];
    uint8_t              RH_length;
    
    uint8_t				 RH3_length;

    uint8_t sizeRH=0;

    memset(&RH_copy[0],0,127);
    RH3_length = 0;
    RH_length = 0;
    memcpy(&msg->l3_destinationAdd,&ipv6_inner_header->dest,sizeof(open_addr_t));
    memcpy(&msg->l3_sourceAdd,&ipv6_inner_header->src,sizeof(open_addr_t));
    
    // initial first Address by compression reference
    firstAddr.type = ADDR_128B;
    if (ipv6_outer_header->src.type != ADDR_NONE){
        if (rpl_option->rplInstanceID == 0){
            icmpv6rpl_getRPLDODAGid(&firstAddr.addr_128b[0]);
        }
    } else {
        memcpy(&firstAddr,&ipv6_inner_header->src,sizeof(open_addr_t));
    }
    
    hlen = 0;
    
    temp_8b = *((uint8_t*)(msg->payload)+hlen);
    type    = *((uint8_t*)(msg->payload)+hlen+1);
    
    //copy and toss any unknown 6LoRHE
    while((temp_8b&FORMAT_6LORH_MASK) == ELECTIVE_6LoRH){
        sizeRH = temp_8b & IPINIP_LEN_6LORH_MASK;
    	memcpy(&RH_copy[RH_length], msg->payload, sizeRH+2);
    	packetfunctions_tossHeader(msg, sizeRH+2);
    	RH_length += 2 + sizeRH;
        temp_8b = *((uint8_t*)(msg->payload)+hlen);
        type    = *((uint8_t*)(msg->payload)+hlen+1);
    }

    hlen += 2;

    // get the first address
    switch(type){
    case RH3_6LOTH_TYPE_0:
        memcpy(&firstAddr.addr_128b[15],msg->payload+hlen,1);
        hlen += 1;
        break;
    case RH3_6LOTH_TYPE_1:
        memcpy(&firstAddr.addr_128b[14],msg->payload+hlen,2);
        hlen += 2;
        break;
    case RH3_6LOTH_TYPE_2:
        memcpy(&firstAddr.addr_128b[12],msg->payload+hlen,4);
        hlen += 4;
        break;
    case RH3_6LOTH_TYPE_3:
        memcpy(&firstAddr.addr_128b[8],msg->payload+hlen,8);
        hlen += 8;
        break;
    case RH3_6LOTH_TYPE_4:
        memcpy(&firstAddr.addr_128b[0],msg->payload+hlen,16);
        hlen += 16;
        break;
    }
    
    packetfunctions_ip128bToMac64b(&firstAddr,&temp_prefix,&temp_addr64);
    if (
        packetfunctions_sameAddress(&temp_prefix,idmanager_getMyID(ADDR_PREFIX)) &&
        packetfunctions_sameAddress(&temp_addr64,idmanager_getMyID(ADDR_64B))
    ){
        size = temp_8b & RH3_6LOTH_SIZE_MASK;
        if (size > 0){
            // there are at least 2 entries in the header, 
            // the router removes the first entry and decrements the Size (by 1) 
            size -= 1;
            packetfunctions_tossHeader(msg,hlen);
            packetfunctions_reserveHeaderSize(msg,2);
            msg->payload[0] = CRITICAL_6LORH | size;
            msg->payload[1] = type;
            // get next hop
            memcpy(&nextAddr,&firstAddr,sizeof(open_addr_t));
            switch(type){
            case RH3_6LOTH_TYPE_0:
                memcpy(&nextAddr.addr_128b[15],msg->payload+2,1);
                break;
            case RH3_6LOTH_TYPE_1:
                memcpy(&nextAddr.addr_128b[14],msg->payload+2,2);
                break;
            case RH3_6LOTH_TYPE_2:
                memcpy(&nextAddr.addr_128b[12],msg->payload+2,4);
                break;
            case RH3_6LOTH_TYPE_3:
                memcpy(&nextAddr.addr_128b[8],msg->payload+2,8);
                break;
            case RH3_6LOTH_TYPE_4:
                memcpy(&nextAddr.addr_128b[0],msg->payload+2,16);
                break;
            }
            packetfunctions_ip128bToMac64b(
                &nextAddr,
                &temp_prefix,
                &msg->l2_nextORpreviousHop
            );
        } else {
            temp_8b   = *((uint8_t*)(msg->payload)+hlen);
            next_type = *((uint8_t*)(msg->payload)+hlen+1);
            if (
                (temp_8b & FORMAT_6LORH_MASK) == CRITICAL_6LORH &&
                next_type<=RH3_6LOTH_TYPE_4
            ) {
                // there is another RH3-6LoRH following, check the type
                if (next_type >= type){
                    packetfunctions_tossHeader(msg,hlen);
                    // get next hop
                    memcpy(&nextAddr,&firstAddr,sizeof(open_addr_t));
                    switch(next_type){
                    case RH3_6LOTH_TYPE_0:
                        memcpy(&nextAddr.addr_128b[15],msg->payload+2,1);
                        break;
                    case RH3_6LOTH_TYPE_1:
                        memcpy(&nextAddr.addr_128b[14],msg->payload+2,2);
                        break;
                    case RH3_6LOTH_TYPE_2:
                        memcpy(&nextAddr.addr_128b[12],msg->payload+2,4);
                        break;
                    case RH3_6LOTH_TYPE_3:
                        memcpy(&nextAddr.addr_128b[8],msg->payload+2,8);
                        break;
                    case RH3_6LOTH_TYPE_4:
                        memcpy(&nextAddr.addr_128b[0],msg->payload+2,16);
                        break;
                    }
                    packetfunctions_ip128bToMac64b(
                        &nextAddr,
                        &temp_prefix,
                        &msg->l2_nextORpreviousHop
                    );
                } else {
                    hlen += 2;
                    switch(next_type){
                    case RH3_6LOTH_TYPE_0:
                        memcpy(&firstAddr.addr_128b[15],msg->payload+hlen,1);
                        hlen += 1;
                        break;
                    case RH3_6LOTH_TYPE_1:
                        memcpy(&firstAddr.addr_128b[14],msg->payload+hlen,2);
                        hlen += 2;
                        break;
                    case RH3_6LOTH_TYPE_2:
                        memcpy(&firstAddr.addr_128b[12],msg->payload+hlen,4);
                        hlen += 4;
                        break;
                    case RH3_6LOTH_TYPE_3:
                        memcpy(&firstAddr.addr_128b[8],msg->payload+hlen,8);
                        hlen += 8;
                        break;
                    }
                    next_size = temp_8b & RH3_6LOTH_SIZE_MASK;
                    packetfunctions_tossHeader(msg,hlen);
                    if (next_size>0){
                        next_size -= 1;
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0] = CRITICAL_6LORH | next_size;
                        msg->payload[1] = next_type;
                    }
                    // add first address
                    switch(type){
                    case RH3_6LOTH_TYPE_0:
                        packetfunctions_reserveHeaderSize(msg,1);
                        msg->payload[0] = firstAddr.addr_128b[15];
                        break;
                    case RH3_6LOTH_TYPE_1:
                        packetfunctions_reserveHeaderSize(msg,2);
                        memcpy(&msg->payload[0],&firstAddr.addr_128b[14],2);
                        break;
                    case RH3_6LOTH_TYPE_2:
                        packetfunctions_reserveHeaderSize(msg,4);
                        memcpy(&msg->payload[0],&firstAddr.addr_128b[12],4);
                        break;
                    case RH3_6LOTH_TYPE_3:
                        packetfunctions_reserveHeaderSize(msg,8);
                        memcpy(&msg->payload[0],&firstAddr.addr_128b[8],8);
                        break;
                    case RH3_6LOTH_TYPE_4:
                        packetfunctions_reserveHeaderSize(msg,16);
                        memcpy(&msg->payload[0],&firstAddr.addr_128b[0],16);
                        break;
                    }
                    packetfunctions_reserveHeaderSize(msg,2);
                    msg->payload[0] = CRITICAL_6LORH | 0;
                    msg->payload[1] = type;
                    packetfunctions_ip128bToMac64b(
                        &firstAddr,
                        &temp_prefix,
                        &msg->l2_nextORpreviousHop
                    );
                }
            } else {
                // there is no next RH3-6loRH, remove current one
                packetfunctions_tossHeader(msg,hlen);
                packetfunctions_ip128bToMac64b(
                    &msg->l3_destinationAdd,
                    &temp_prefix,
                    &msg->l2_nextORpreviousHop
                );
            }
        }
    } else {
        // log error
        openserial_printError(
            COMPONENT_IPHC,
            ERR_6LOWPAN_UNSUPPORTED,
            (errorparameter_t)16,
            (errorparameter_t)(temp_addr64.addr_64b[7])
        );
    }
    // copy RH3s before toss them
    if (
        ipv6_outer_header->src.type != ADDR_NONE &&
        ipv6_outer_header->hopByhop_option != NULL
    ){
        // check the length of RH3s
        RH3_length += ipv6_outer_header->hopByhop_option-msg->payload;
        memcpy(&RH_copy[RH_length],msg->payload,RH3_length);
        packetfunctions_tossHeader(msg,RH3_length);
        RH_length += RH3_length;
        
        // retrieve hop-by-hop header (includes RPL option)
        rpi_length = iphc_retrieveIPv6HopByHopHeader(
                          msg,
                          rpl_option
                     );
     
        // toss the headers
        packetfunctions_tossHeader(
            msg,
            rpi_length
        );
      
        flags = rpl_option->flags;
        senderRank = rpl_option->senderRank;
        if ((flags & O_FLAG)!=O_FLAG){
            // wrong direction
            // log error
            openserial_printError(
                COMPONENT_FORWARDING,
                ERR_WRONG_DIRECTION,
                (errorparameter_t)flags,
                (errorparameter_t)senderRank
            );
        }
        if (senderRank > neighbors_getMyDAGrank()){
            // loop detected
            // set flag
            rpl_option->flags |= R_FLAG;
            // log error
            openserial_printError(
                COMPONENT_FORWARDING,
                ERR_LOOP_DETECTED,
                (errorparameter_t) senderRank,
                (errorparameter_t) neighbors_getMyDAGrank()
            );
        }
        forwarding_createRplOption(rpl_option, rpl_option->flags);
        // toss the IP in IP 6LoRH
        packetfunctions_tossHeader(msg, ipv6_outer_header->header_length);
    } else {
        RH3_length = 0;
    }
    
    // send to next lower layer
    return iphc_sendFromForwarding(
        msg,
        ipv6_outer_header,
        ipv6_inner_header,
        rpl_option,
        &ipv6_outer_header->flow_label,
        &RH_copy[0],
        RH_length,
        PCKTFORWARD
    );
}

/**
\brief Send a packet, using a source route to find the next hop.

We assume the packet is sent by the DAG root. Processing is detailed in
https://datatracker.ietf.org/doc/draft-ietf-roll-routing-dispatch/

\param[in,out] msg                Packet to send
\param[in]     ipv6_outer_header  Packet's IP-in-IP header for use by the PAN
\param[in]     ipv6_inner_header  Packet's original IPv6 header; mostly empty
\param[in]     rpl_option         RPL params
\param[in]     srcRoute           Hops to destination
*/
owerror_t forwarding_send_down_SourceRouting(
      OpenQueueEntry_t* msg,
      ipv6_header_iht*  ipv6_outer_header,
      ipv6_header_iht*  ipv6_inner_header,
      rpl_option_ht*    rpl_option,
      dag_route_t*      srcRoute
   ) {
   open_addr_t       compressRef;
   open_addr_t       temp_prefix;
   uint8_t           i;
   uint8_t           matchlen;
   uint8_t           matchType;     // type (length) of addresses in the header
   uint8_t           addrIndex;     // start index to copy from the source hop
   uint8_t           rh3Type;       // RH3 header match class currently active
   uint8_t           rh3buf[40];    // stores SRH-6LoRH as it is built
   uint8_t           typeSize;      // value for size field in RH3 header (one less than hop count)
   uint8_t           typeStart;     // start position in rh3buf for the current RH3 type
   uint8_t           typePos;       // position in rh3buf for the next byte to write
   
   // Initial compression reference is me (DAG root)
   compressRef.type = ADDR_128B;
   memcpy(&compressRef.addr_128b[0],&msg->l3_sourceAdd.addr_128b[0],16);

   rh3Type    = 0xff;  // means type not defined yet
   memset(&rh3buf[0],0,40);
   typeSize   = 0;     
   typeStart  = 0;     
   typePos    = 0;     // position for next write for the active RH3 type

   // Loop through source route, starting from next hop.
   // Skip srcRoute[0]; it is the destination.
   for (i=srcRoute->hopCount-1; i>0; i--) {

      // Find length of address match between hop and compress reference
      for (matchlen=0; matchlen<16; matchlen++) {
         if (srcRoute->hop[i]->address.addr_128b[matchlen] != compressRef.addr_128b[matchlen]) {
            break;
         }
      }
      if (matchlen==16) {
         // Means hop is the same as compress reference.
         openserial_printError(
            COMPONENT_FORWARDING,
            ERR_INVALID_ROUTE,
            (errorparameter_t)0,
            (errorparameter_t)0);
         return E_FAIL;
      }

      if (matchlen==15) {
         matchType = RH3_6LOTH_TYPE_0;
         addrIndex = 15;
      } else if (matchlen==14) {
         matchType = RH3_6LOTH_TYPE_1;
         addrIndex = 14;
      } else if (matchlen>=12) {
         matchType = RH3_6LOTH_TYPE_2;
         addrIndex = 12;
      } else if (matchlen>=8) {
         matchType = RH3_6LOTH_TYPE_3;
         addrIndex = 8;
      } else {
         matchType = RH3_6LOTH_TYPE_4;
         addrIndex = 0;
      }

      if (matchType!=rh3Type) {
         typeStart         = typePos;
         typeSize          = 0;
      } else {
         typeSize++;
      }
      // Write/Update 2 header bytes
      rh3buf[typeStart] = CRITICAL_6LORH|typeSize;
      if (matchType!=rh3Type) {
         typePos++;                      // skip past byte just written
         rh3buf[typePos++] = matchType;
      }
      // Write rightmost address bytes for the header type
      memcpy(&rh3buf[typePos],&srcRoute->hop[i]->address.addr_128b[addrIndex],16-addrIndex);
      typePos += 16-addrIndex;

      if (matchType != rh3Type) {
         rh3Type = matchType;
      }
      compressRef = srcRoute->hop[i]->address;
   }
   
   // set layer 2 next hop
   packetfunctions_ip128bToMac64b(
      &srcRoute->hop[srcRoute->hopCount-1]->address,
      &temp_prefix,
      &msg->l2_nextORpreviousHop);
   
   // send to next lower layer
   return iphc_sendFromForwarding(
      msg,
      ipv6_outer_header,
      ipv6_inner_header,
      rpl_option,
      &ipv6_outer_header->flow_label,
      &rh3buf[0],
      typePos,
      PCKTSEND
   );
}

/**
\brief Create a RPL option.

\param[out] rpl_option A pointer to the structure to fill in.
\param[in]  flags      The flags to indicate in the RPL option.
*/
void forwarding_createRplOption(rpl_option_ht* rpl_option, uint8_t flags) {
    uint8_t I,K;
    rpl_option->optionType         = RPL_HOPBYHOP_HEADER_OPTION_TYPE;
    rpl_option->rplInstanceID      = icmpv6rpl_getRPLIntanceID();
    rpl_option->senderRank         = neighbors_getMyDAGrank();
   
    if (rpl_option->rplInstanceID == 0){
       I = 1;
    } else {
       I = 0;
    }
    
    if ((rpl_option->senderRank & 0x00FF) == 0){
        K = 1;
    } else {
        K = 0;
    }
    
    rpl_option->flags = (flags & ~I_FLAG & ~K_FLAG) | (I<<1) | K;
}

/**
\brief Optimize the endpoint addresses from 128-bit down to 64-bit or 16-bit,
       for use with RFC 6282 header compression.

Uses 64-bit if prefixes match; 16-bit if furthermore the 64-bit host addresses are
in the form 0000:00FF:FE00:xxxx, where 'x' is any hex character.

\param[in] src128b Source 128-bit address
\param[in] dst128b Destination 128-bit address
\param[out] srcOpt Optimized source address
\param[out] dstOpt Optimized destination address
*/
void forwarding_optimize128bForIphc(open_addr_t* src128b, open_addr_t* dst128b,
                                    open_addr_t* srcOpt, open_addr_t* dstOpt) {
   open_addr_t src_prefix;
   open_addr_t dst_prefix;
   open_addr_t temp_src;
   open_addr_t temp_dst;
   uint8_t addr16b[6] = {0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00};
   uint8_t matchlen;
   
   packetfunctions_ip128bToMac64b(src128b,&src_prefix,&temp_src);
   packetfunctions_ip128bToMac64b(dst128b,&dst_prefix,&temp_dst);
   
   if (packetfunctions_sameAddress(&src_prefix,&dst_prefix)) {

      for (matchlen=0; matchlen<6; matchlen++) {
         if (temp_src.addr_64b[matchlen] != addr16b[matchlen]
               || temp_src.addr_64b[matchlen] != temp_dst.addr_64b[matchlen])
            break;
      }
      if (matchlen==6) {
         packetfunctions_mac64bToMac16b(&temp_src, srcOpt);
         packetfunctions_mac64bToMac16b(&temp_dst, dstOpt);
      } else {
         packetfunctions_ip128bToMac64b(src128b,&src_prefix,srcOpt);
         packetfunctions_ip128bToMac64b(dst128b,&dst_prefix,dstOpt);
      }
   }
}
