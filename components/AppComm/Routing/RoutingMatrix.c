#include "RoutingMatrix.h"

const RoutingMatrix_t RoutingMatrix[] = {
  /* ID      TxType            FZECU  BZECU  CCU    DESCRIPTION */
    {0x154,  E_FRAME_EVENT,  { DIR_D, DIR_X, DIR_S }, "Control command for Front-ZECU"},
    {0x155,  E_FRAME_EVENT,  { DIR_X, DIR_D, DIR_S }, "Control command for Back-ZECU"},
    {0x156,  E_FRAME_EVENT,  { DIR_D, DIR_D, DIR_S }, "Global control for specific situations"},
    {0x181,  E_FRAME_EVENT,  { DIR_S, DIR_X, DIR_D }, "Emergency event from Front-ZECU"},
    {0x183,  E_FRAME_CYCLIC, { DIR_S, DIR_X, DIR_D }, "Periodic status update from Front-ZECU"},
    {0x191,  E_FRAME_EVENT,  { DIR_X, DIR_S, DIR_D }, "Emergency event from Back-ZECU"},
    {0x193,  E_FRAME_CYCLIC, { DIR_X, DIR_S, DIR_D }, "Periodic status update from Back-ZECU"}
};

const uint8_t RoutingMatrixSize = sizeof(RoutingMatrix) / sizeof(RoutingMatrix[0]);