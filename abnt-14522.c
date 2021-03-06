/*
 * abnt-14522.c
 *
 *  Created on: 1 de out de 2019
 *      Author: Luiz Paulo
 */

/////////////////////////////////////////////////////////////////////////
////////////////////////////// Includes /////////////////////////////////
/////////////////////////////////////////////////////////////////////////
#include "abnt-14522.h"

/////////////////////////////////////////////////////////////////////////
///////////////////////// Macros and Defines ////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//////////////////// Private Function Prototypes ////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//////////// State Related Enum and Functions Prototypes ////////////////
/////////////////////////////////////////////////////////////////////////

///**
// * Motor States Enum
// */
//typedef enum
//{
//    ABNT_START_ST = 0, ABNT_LAST_ST
//} abnt_states;
//
//static void _abntStartSt(void);

/**
 * Array of Motor state functions
 */
//static const fp _abntStates[ABNT_LAST_ST] = { _abntStartSt };

/////////////////////////////////////////////////////////////////////////
///////////////////////// Private Structure /////////////////////////////
/////////////////////////////////////////////////////////////////////////
///**
// * Private structure
// */
//typedef struct
//{
//    abnt_states state;				///< Module's state
//} abnt_t;
//
//static abnt_t _abnt;

/////////////////////////////////////////////////////////////////////////
///////////////////////// Private Variables /////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
///////////////////////////// Functions /////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//void abntInit(void)
//{
//
//    // Clear the main structure
//    memset(&_abnt, 0x00, sizeof(_abnt));
//}

//void abntService(void)
//{
//
//    // Check if the state is between the working states
//    if (_abnt.state >= ABNT_LAST_ST)
//    {
//
//        // If it is an erroneous state, init the module again
//        abntInit();
//    }
//
//    // Process the current state
//    _abntStates[_abnt.state]();
//
//}

bool_t abntToByte(abnt14522_t abnt, uint8_t *byte)
{
    uint8_t exclusive = 0xFF;

    // 1 octet
    (*byte) = abnt.remainingSeconds & 0xFF;
    exclusive ^= *(byte++);

    // 2 octet
    (*byte) = ((abnt.remainingSeconds >> 8) & 0x0F)
            | (abnt.bDemandRepo ? 0x10 : 0x00)
            | (abnt.bReactiveInterval ? 0x20 : 0x00)
            | (abnt.bUferCap ? 0x40 : 0x00) | (abnt.bUferInd ? 0x80 : 0x00);
    exclusive ^= *(byte++);

    // 3 octet
    (*byte) = (abnt.horoType & 0x0F) | ((abnt.taxType << 4) & 0x0F)
            | (abnt.bTaxActive ? 0x80 : 0x00);
    exclusive ^= *(byte++);

    // 4 and 5 octet
    (*byte) = abnt.activePulses & 0xFF;
    exclusive ^= *(byte++);
    (*byte) = (abnt.activePulses >> 8) & 0x0F;
    exclusive ^= *(byte++);

    // 6 and 7 octet
    (*byte) = abnt.reactivePulses & 0xFF;
    exclusive ^= *(byte++);
    (*byte) = (abnt.reactivePulses >> 8) & 0x0F;
    exclusive ^= *(byte++);

    // 8 octet exclusive or
    (*byte) = exclusive;

    return TRUE;

}

bool_t abnOffset(abnt14522_t *t, int16_t remaining, int16_t activePulses,
                 int16_t reactivePulses)
{
    t->remainingSeconds += remaining;
    t->activePulses += activePulses;
    t->reactivePulses += reactivePulses;


    return TRUE;
}

void _abntStartSt(void)
{

}

//////////////////////////// END OF FILE ////////////////////////////////
/////////////////////////////////////////////////////////////////////////
