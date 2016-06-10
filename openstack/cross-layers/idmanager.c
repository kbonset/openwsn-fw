#include "opendefs.h"
#include "idmanager.h"
#include "eui64.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "neighbors.h"
#include "schedule.h"

//=========================== variables =======================================

idmanager_vars_t idmanager_vars;

//=========================== prototypes ======================================

uint8_t idmanager_atoh(char c);

//=========================== public ==========================================

void idmanager_init() {
#ifdef ADDR64B
   uint8_t i,j;
   char *addr64b;
#endif
   
   // reset local variables
   memset(&idmanager_vars, 0, sizeof(idmanager_vars_t));
   // this is used to not wakeup in non-activeslot
   idmanager_vars.slotSkip             = FALSE;
   
   // isDAGroot
#ifdef DAGROOT
   idmanager_vars.isDAGroot            = TRUE;
#else
   idmanager_vars.isDAGroot            = FALSE;
#endif
   
   // myPANID
   idmanager_vars.myPANID.type         = ADDR_PANID;
   idmanager_vars.myPANID.panid[0]     = 0xca;
   idmanager_vars.myPANID.panid[1]     = 0xfe;
   
   // myPrefix
   idmanager_vars.myPrefix.type        = ADDR_PREFIX;
#ifdef DAGROOT
   idmanager_vars.myPrefix.prefix[0]   = 0xbb;
   idmanager_vars.myPrefix.prefix[1]   = 0xbb;
   idmanager_vars.myPrefix.prefix[2]   = 0x00;
   idmanager_vars.myPrefix.prefix[3]   = 0x00;
   idmanager_vars.myPrefix.prefix[4]   = 0x00;
   idmanager_vars.myPrefix.prefix[5]   = 0x00;
   idmanager_vars.myPrefix.prefix[6]   = 0x00;
   idmanager_vars.myPrefix.prefix[7]   = 0x00;
#else
   memset(&idmanager_vars.myPrefix.prefix[0], 0x00, sizeof(idmanager_vars.myPrefix.prefix));
#endif
   
   // my64bID
   idmanager_vars.my64bID.type         = ADDR_64B;
#ifdef ADDR64B
   // Read from string in format: 'hhhh:hhhh:hhhh:hhhh'

   // Stringify the ADDR64B macro value.
   #define STRINGIFY(s) #s
   #define ADDR64B_STR(s) STRINGIFY(s)

   addr64b = ADDR64B_STR(ADDR64B);
   j       = 0;                // compensates byte index for colons in string
   for (i=0; i<19; i+=2) {
      // skip over colon
      if ((i+1) % 5 == 0) {
         i++;
         j++;
      }
      idmanager_vars.my64bID.addr_64b[(i-j)/2] = (idmanager_atoh(addr64b[i]) << 4)
                                                + idmanager_atoh(addr64b[i+1]);
   }
#else   
   eui64_get(idmanager_vars.my64bID.addr_64b);
#endif
   
   // my16bID
   packetfunctions_mac64bToMac16b(&idmanager_vars.my64bID,&idmanager_vars.my16bID);
}

bool idmanager_getIsDAGroot() {
   bool res;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   res=idmanager_vars.isDAGroot;
   ENABLE_INTERRUPTS();
   return res;
}

void idmanager_setIsDAGroot(bool newRole) {
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   idmanager_vars.isDAGroot = newRole;
   neighbors_updateMyDAGrankAndNeighborPreference();
   schedule_startDAGroot();
   ENABLE_INTERRUPTS();
}

bool idmanager_getIsSlotSkip() {
   bool res;
   INTERRUPT_DECLARATION();
   
   DISABLE_INTERRUPTS();
   res=idmanager_vars.slotSkip;
   ENABLE_INTERRUPTS();
   return res;
}

open_addr_t* idmanager_getMyID(uint8_t type) {
   open_addr_t* res;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   switch (type) {
     case ADDR_16B:
        res= &idmanager_vars.my16bID;
        break;
     case ADDR_64B:
        res= &idmanager_vars.my64bID;
        break;
     case ADDR_PANID:
        res= &idmanager_vars.myPANID;
        break;
     case ADDR_PREFIX:
        res= &idmanager_vars.myPrefix;
        break;
     case ADDR_128B:
        // you don't ask for my full address, rather for prefix, then 64b
     default:
        openserial_printCritical(COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)type,
              (errorparameter_t)0);
        res= NULL;
        break;
   }
   ENABLE_INTERRUPTS();
   return res;
}

owerror_t idmanager_setMyID(open_addr_t* newID) {
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   switch (newID->type) {
     case ADDR_16B:
        memcpy(&idmanager_vars.my16bID,newID,sizeof(open_addr_t));
        break;
     case ADDR_64B:
        memcpy(&idmanager_vars.my64bID,newID,sizeof(open_addr_t));
        break;
     case ADDR_PANID:
        memcpy(&idmanager_vars.myPANID,newID,sizeof(open_addr_t));
        break;
     case ADDR_PREFIX:
        memcpy(&idmanager_vars.myPrefix,newID,sizeof(open_addr_t));
        break;
     case ADDR_128B:
        //don't set 128b, but rather prefix and 64b
     default:
        openserial_printCritical(COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)newID->type,
              (errorparameter_t)1);
        ENABLE_INTERRUPTS();
        return E_FAIL;
   }
   ENABLE_INTERRUPTS();
   return E_SUCCESS;
}

bool idmanager_isMyAddress(open_addr_t* addr) {
   open_addr_t temp_my128bID;
   bool res;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   switch (addr->type) {
     case ADDR_16B:
        res= packetfunctions_sameAddress(addr,&idmanager_vars.my16bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_64B:
        res= packetfunctions_sameAddress(addr,&idmanager_vars.my64bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_128B:
        // build temporary my128bID
        temp_my128bID.type = ADDR_128B;
        memcpy(&temp_my128bID.addr_128b[0],&idmanager_vars.myPrefix.prefix,8);
        memcpy(&temp_my128bID.addr_128b[8],&idmanager_vars.my64bID.addr_64b,8);

        res= packetfunctions_sameAddress(addr,&temp_my128bID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_PANID:
        res= packetfunctions_sameAddress(addr,&idmanager_vars.myPANID);
        ENABLE_INTERRUPTS();
        return res;
     case ADDR_PREFIX:
        res= packetfunctions_sameAddress(addr,&idmanager_vars.myPrefix);
        ENABLE_INTERRUPTS();
        return res;
     default:
        openserial_printCritical(COMPONENT_IDMANAGER,ERR_WRONG_ADDR_TYPE,
              (errorparameter_t)addr->type,
              (errorparameter_t)2);
        ENABLE_INTERRUPTS();
        return FALSE;
   }
}

void idmanager_triggerAboutRoot() {
   uint8_t         number_bytes_from_input_buffer;
   uint8_t         input_buffer[9];
   open_addr_t     myPrefix;
   uint8_t         dodagid[16];
   
   //=== get command from OpenSerial
   number_bytes_from_input_buffer = openserial_getInputBuffer(input_buffer,sizeof(input_buffer));
   if (number_bytes_from_input_buffer!=sizeof(input_buffer)) {
      openserial_printError(COMPONENT_IDMANAGER,ERR_INPUTBUFFER_LENGTH,
            (errorparameter_t)number_bytes_from_input_buffer,
            (errorparameter_t)0);
      return;
   };
   
   //=== handle command
   
   // take action (byte 0)
   switch (input_buffer[0]) {
     case ACTION_YES:
        idmanager_setIsDAGroot(TRUE);
        idmanager_vars.slotSkip = FALSE;
        break;
     case ACTION_NO:
        idmanager_setIsDAGroot(FALSE);
        idmanager_vars.slotSkip = TRUE;
        break;
     case ACTION_TOGGLE:
        if (idmanager_getIsDAGroot()) {
           idmanager_setIsDAGroot(FALSE);
           idmanager_vars.slotSkip = TRUE;
        } else {
           idmanager_setIsDAGroot(TRUE);
           idmanager_vars.slotSkip = FALSE;
        }
        break;
   }
   
   // store prefix (bytes 1-8)
   myPrefix.type = ADDR_PREFIX;
   memcpy(
      myPrefix.prefix,
      &input_buffer[1],
      sizeof(myPrefix.prefix)
   );
   idmanager_setMyID(&myPrefix);
   
   // indicate DODAGid to RPL
   memcpy(&dodagid[0],idmanager_vars.myPrefix.prefix,8);  // prefix
   memcpy(&dodagid[8],idmanager_vars.my64bID.addr_64b,8); // eui64
   icmpv6rpl_writeDODAGid(dodagid);
   
   return;
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_id() {
   debugIDManagerEntry_t output;
   
   output.isDAGroot = idmanager_vars.isDAGroot;
   memcpy(output.myPANID,idmanager_vars.myPANID.panid,2);
   memcpy(output.my16bID,idmanager_vars.my16bID.addr_16b,2);
   memcpy(output.my64bID,idmanager_vars.my64bID.addr_64b,8);
   memcpy(output.myPrefix,idmanager_vars.myPrefix.prefix,8);
   
   openserial_printStatus(STATUS_ID,(uint8_t*)&output,sizeof(debugIDManagerEntry_t));
   return TRUE;
}


//=========================== private =========================================

/**
\brief Converts a hex character to a base 16 digit; 0 if not a hex character
 */
uint8_t idmanager_atoh(char c) {
  if (c>='0' && c<='9')
     return c - '0';
  else if (c>='A' && c<='F')
     return c - 'A' + 10;
  else if (c>='a' && c<='f')
     return c - 'a' + 10;
  else
     return 0;
}