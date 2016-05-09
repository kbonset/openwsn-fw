#ifndef __DAG_H
#define __DAG_H

/**
\addtogroup IPv6
\{
\addtogroup DAG
\{
*/

#include "opendefs.h"

//=========================== define ==========================================

#define MAX_NODES 10
#define MAX_NEIGHBORS 20
#define MAX_HOPS 7

//=========================== typedef =========================================

typedef struct dagnode dagnode_t;
typedef struct dagnbr dagnbr_t;

/**
\brief A mote in the context of a list of potential parents for another mote.
 */
struct dagnbr {
    dagnode_t* node;
    uint8_t parentPreference; 
    dagnbr_t* next;
};

/**
\brief A mote in the DAG; includes a linked list of potential parent neighbors
 */
struct dagnode {
    open_addr_t address;   /**< Address always is 128 bits */
    dagnbr_t* neighbor;
};

/**
\brief A route, typically to a destination mote
 */
typedef struct {
    dagnode_t* hop[MAX_HOPS];
    uint8_t hopCount;
} dag_route_t;

//=========================== variables =======================================

typedef struct {
    dagnode_t nodes[MAX_NODES];          /**< List of nodes in the DAG */
    uint8_t nextNode;                    /**< Index of next node to use in 'nodes' */
    dagnbr_t neighbors[MAX_NEIGHBORS];   /**< Source list of neighbors; referenced
                                              in neighbor list for a mote */
} dag_vars_t;

//=========================== prototypes ======================================

void      dag_init(void);
owerror_t dag_buildRoute(dag_route_t* route, open_addr_t* dest);

/**
\}
\}
*/

#endif
