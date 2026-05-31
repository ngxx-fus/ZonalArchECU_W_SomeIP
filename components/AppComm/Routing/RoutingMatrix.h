#ifndef __ROUTING_MATRIX_H__
#define __ROUTING_MATRIX_H__

#include <stdint.h>

typedef enum {
    E_FRAME_EVENT  = 0,
    E_FRAME_CYCLIC = 1,
    E_FRAME_MIXED  = 2
} eFrameTxType_t;

typedef enum {
    DIR_X = 0, /* No involvement */
    DIR_S = 1, /* Source / Sender */
    DIR_D = 2  /* Destination / Receiver */
} eFrameDirection_t;

typedef struct {
    uint16_t ID;
    uint8_t  TxType;
    uint8_t  Relation[3]; /* Index: 0-FZECU, 1-BZECU, 2-CCU */
    char     Desc[75];
} RoutingMatrix_t;

/* Use extern to allow multiple inclusions without redefinition errors */
extern const RoutingMatrix_t RoutingTable[];
extern const uint8_t RoutingTableSize;

#endif /* __ROUTING_MATRIX_H__ */