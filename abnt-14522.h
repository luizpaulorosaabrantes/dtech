/*
 * abnt-14522.h
 *
 *  Created on: 1 de out de 2019
 *      Author: Luiz Paulo
 */

#ifndef ABNT_14522_H_
#define ABNT_14522_H_

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////
////////////////////////////// Includes /////////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
///////////////////////// Macros and Defines ////////////////////////////
/////////////////////////////////////////////////////////////////////////
#define bool_t unsigned char
#define TRUE   1
#define FALSE  0


typedef enum
{
    PONTA = 1, FORA_PONTA = 2, RESERVADO = 8
} horoSazonal_t;

typedef enum
{
    AZUL = 0, VERDE = 1, IRRIGANTES = 2, OUTRAS = 3
} tipoTarifa_t;

typedef struct
{
    uint16_t remainingSeconds;
    bool_t bDemandRepo;
    bool_t bReactiveInterval;
    bool_t bUferCap;
    bool_t bUferInd;
    horoSazonal_t horoType;
    tipoTarifa_t taxType;
    bool_t bTaxActive;
    uint16_t activePulses;
    uint16_t reactivePulses;
} abnt14522_t;

/////////////////////////////////////////////////////////////////////////
//////////////////// External Function Prototypes ///////////////////////
/////////////////////////////////////////////////////////////////////////
extern void abntInit(void);
extern void abntService(void);
extern bool_t abntToByte(abnt14522_t abnt, uint8_t *byte);
extern bool_t abnOffset(abnt14522_t *t,int16_t remaining, int16_t activePulses,
                     int16_t reactivePulses);

#ifdef __cplusplus
}
#endif

#endif /* ABNT_14522_H_ */


/** @} */
//////////////////////////// END OF FILE ////////////////////////////////
/////////////////////////////////////////////////////////////////////////
