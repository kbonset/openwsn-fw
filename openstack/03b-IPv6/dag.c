// Copyright 2016, Onset Computer Corp.

#include "opendefs.h"
#include "dag.h"
#include "idmanager.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "neighbors.h"

//=========================== variables =======================================

dag_vars_t             dag_vars;

//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief Initialize this module.
*/
void dag_init() {
   dagnode_t* node;
   uint8_t addr0[8] = {0x00, 0x12, 0x4b, 0x00, 0x03, 0xa5, 0x94, 0xb0};
   uint8_t addr1[8] = {0x00, 0x12, 0x4b, 0x00, 0x03, 0xa5, 0x8d, 0x4e};
   uint8_t addr2[8] = {0x00, 0x12, 0x4b, 0x00, 0x03, 0xa5, 0x94, 0xcb};
   uint8_t i        = 0;
    
   //===== reset local variables
   memset(&dag_vars,0,sizeof(dag_vars_t));
   for (i=0; i<MAX_NODES; i++) {
       dag_vars.nodes[i].address.type = ADDR_NONE;
   }

   // Hardcode a 2 node DAG for development. node 0 is parent of node 1.
   node = &dag_vars.nodes[0];
   node->address.type       = ADDR_64B;
   memcpy(node->address.addr_64b, addr0, sizeof(addr0)/sizeof(uint8_t));
   node->neighbor           = NULL;
   dag_vars.nextNode       = 1;

   node = &dag_vars.nodes[1];
   node->address.type       = ADDR_64B;
   memcpy(node->address.addr_64b, addr1, sizeof(addr1)/sizeof(uint8_t));
   node->neighbor           = NULL;
   dag_vars.nextNode       = 2;

   dag_vars.neighbors[0].node             = &dag_vars.nodes[0];
   dag_vars.neighbors[0].parentPreference = MAXPREFERENCE;
   dag_vars.neighbors[0].next             = NULL;

   dag_vars.nodes[1].neighbor = &dag_vars.neighbors[0];
}

/**
\brief Build a downward route to a node in the DAG.

\param[out] route Route to fill in, built from destination to root; unused hops remain NULL
\param[in] dest Target node for route; must be a 64-bit or 128-bit address
\return E_SUCCESS if find a route; E_FAIL otherwise
*/
owerror_t dag_buildRoute(dag_route_t* route, open_addr_t* dest) {
   dagnode_t *node;
   dagnbr_t  *nbr;
   open_addr_t temp_prefix;
   open_addr_t dest_64b;
   uint8_t   i = 0;

   // Validate address
   if (dest->type==ADDR_128B) {
      packetfunctions_ip128bToMac64b(dest,&temp_prefix,&dest_64b);
   } else if (dest->type==ADDR_64B) {
      memcpy(&dest_64b, dest, sizeof(open_addr_t));
   } else {
      openserial_printError(
         COMPONENT_DAG,ERR_NO_ROUTE,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return E_FAIL;
   }
   
   // Initialize route
   for (i=0; i<MAX_HOPS; i++) {
      route->hop[i] = NULL;
   }
   route->hopCount = 0;

   // Find destination
   for (i=0; i<MAX_NODES; i++) {
      if (packetfunctions_sameAddress(&dag_vars.nodes[i].address, &dest_64b)) {
         route->hop[route->hopCount++] = &dag_vars.nodes[i];
         break;
      }
   }
   if (route->hopCount==0 || route->hop[0]->neighbor==NULL) {
      // Can't find destination or path to DAG root
      openserial_printError(
         COMPONENT_DAG,ERR_NO_ROUTE,
         (errorparameter_t)0,
         (errorparameter_t)1
      );
      return E_FAIL;
   }

   // Hop back to DAG root
   node = route->hop[0];
   nbr  = node->neighbor;
   for (i=route->hopCount; i<MAX_HOPS; i++) {
      for (; nbr!=NULL; nbr=nbr->next) {
         // Found next hop
         if (nbr->parentPreference==MAXPREFERENCE) {
            if (idmanager_isMyAddress(&nbr->node->address)) {
               // We're at DAG root; done
               return E_SUCCESS;
            }
            route->hop[route->hopCount++] = nbr->node;
            node = nbr->node;
            nbr  = node->neighbor;
            break;
         }
      }
      // Didn't find a parent
      if (route->hopCount==i) {
         openserial_printError(
            COMPONENT_DAG,ERR_NO_ROUTE,
            (errorparameter_t)i,
            (errorparameter_t)2
         );
         return E_FAIL;
      }
   }
   return E_SUCCESS;
}


//=========================== private =========================================
