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
   uint8_t prefix[8] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
   uint8_t addr0[8] = {0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x94, 0xb0};
   // for 2 node setup
   //uint8_t addr1[8] = {0x00, 0x12, 0x4b, 0x00, 0x03, 0xa5, 0x8d, 0x4e};
   // for 3 node setup
   uint8_t addr1[8] = {0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x94, 0xcb};
   uint8_t addr2[8] = {0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x8d, 0x4e};
   uint8_t i        = 0;
    
   //===== reset local variables
   memset(&dag_vars,0,sizeof(dag_vars_t));
   for (i=0; i<MAX_NODES; i++) {
       dag_vars.nodes[i].address.type = ADDR_NONE;
   }

   // Hardcode a 3 node DAG for development. node 0 is parent of node 1, and
   // node 1 is parent of node 2.
   // node 0
   node = &dag_vars.nodes[0];
   node->address.type       = ADDR_128B;
   memcpy(&node->address.addr_128b[0], &prefix[0], 8);
   memcpy(&node->address.addr_128b[8], &addr0[0], 8);
   node->neighbor           = NULL;
   dag_vars.nextNode        = 1;

   // node 1
   node = &dag_vars.nodes[1];
   node->address.type       = ADDR_128B;
   memcpy(&node->address.addr_128b[0], &prefix[0], 8);
   memcpy(&node->address.addr_128b[8], &addr1[0], 8);
   node->neighbor           = NULL;
   dag_vars.nextNode        = 2;

   dag_vars.neighbors[0].node             = &dag_vars.nodes[0];
   dag_vars.neighbors[0].parentPreference = MAXPREFERENCE;
   dag_vars.neighbors[0].next             = NULL;

   dag_vars.nodes[1].neighbor = &dag_vars.neighbors[0];

   // node 2
   node = &dag_vars.nodes[2];
   node->address.type       = ADDR_128B;
   memcpy(&node->address.addr_128b[0], &prefix[0], 8);
   memcpy(&node->address.addr_128b[8], &addr2[0], 8);
   node->neighbor           = NULL;
   dag_vars.nextNode        = 3;

   dag_vars.neighbors[1].node             = &dag_vars.nodes[1];
   dag_vars.neighbors[1].parentPreference = MAXPREFERENCE;
   dag_vars.neighbors[1].next             = NULL;

   dag_vars.nodes[2].neighbor = &dag_vars.neighbors[1];
}

/**
\brief Build a downward route to a node in the DAG.

The route includes the destination but does not include the origin.

\param[out] route Route to fill in, built from destination to root; unused hops remain NULL
\param[in] dest Target node for route; must be a 64-bit or 128-bit address
\return E_SUCCESS if find a route; E_FAIL otherwise
*/
owerror_t dag_buildRoute(dag_route_t* route, open_addr_t* dest) {
   dagnode_t *node;
   dagnbr_t  *nbr;
   open_addr_t dest_128b;
   uint8_t   i = 0;

   // Validate address
   if (dest->type==ADDR_128B) {
      dest_128b.type = ADDR_128B;
      memcpy(&dest_128b.addr_128b[0], &dest->addr_128b[0], 16);
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
      if (packetfunctions_sameAddress(&dag_vars.nodes[i].address, &dest_128b)) {
         route->hop[route->hopCount++] = &dag_vars.nodes[i];
         break;
      }
   }
   if (route->hopCount==0 || route->hop[0]->neighbor==NULL) {
      // Can't find destination or path to DAG root
      openserial_printError(
         COMPONENT_DAG,ERR_NO_ROUTE,
         (errorparameter_t)1,
         (errorparameter_t)0
      );
      return E_FAIL;
   }

   // Hop back to DAG root
   node = route->hop[0];
   nbr  = node->neighbor;
   for (i=route->hopCount; i<MAX_HOPS; i++) {
      while (nbr!=NULL) {
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
         } else {
             nbr = nbr->next;
         }
      }
      // Didn't find a parent
      if (route->hopCount==i) {
         openserial_printError(
            COMPONENT_DAG,ERR_NO_ROUTE,
            (errorparameter_t)2,
            (errorparameter_t)i
         );
         return E_FAIL;
      }
   }
   openserial_printError(
      COMPONENT_DAG,ERR_NO_ROUTE,
      (errorparameter_t)3,
      (errorparameter_t)0
   );
   return E_FAIL;
}

//=========================== private =========================================
