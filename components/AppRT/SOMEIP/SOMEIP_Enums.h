#ifndef __CONSTS_AND_ENUMS_H__
#define __CONSTS_AND_ENUMS_H__

#include <stdint.h>

typedef enum SOMEIP_MessageType_e {
    /* Standard SOME/IP Message Types */
    eSOMEIP_MSG_REQUEST              = 0x00, ///< Request expecting response
    // ----> Researching/seeking/findingout --->Structure (all fields)
    // ----> Ref to the PDF/DOC/PAPER
    // ----> When/Where?
    eSOMEIP_MSG_REQUEST_NO_RETURN    = 0x01, ///< Fire & Forget
    eSOMEIP_MSG_NOTIFICATION         = 0x02, ///< Event/Notification
    
    /* Response Message Types */
    eSOMEIP_MSG_RESPONSE             = 0x80, ///< Normal Response
    eSOMEIP_MSG_ERROR                = 0x81, ///< Error Response

    /// SKIPPED ////////////////////////////////////////////////////////////
    /* SOME/IP-TP Message Types */
    eSOMEIP_MSG_TP_REQUEST           = 0x20, ///< Segmented Request
    eSOMEIP_MSG_TP_REQUEST_NO_RETURN = 0x21, ///< Segmented Fire & Forget
    eSOMEIP_MSG_TP_NOTIFICATION      = 0x22, ///< Segmented Notification

    /* SOME/IP-TP Response Message Types */
    eSOMEIP_MSG_TP_RESPONSE          = 0xA0, ///< Segmented Response
    eSOMEIP_MSG_TP_ERROR             = 0xA1, ///< Segmented Error Response
} SOMEIP_MessageType_e;

typedef enum SOMEIP_ReturnCode_e {
    /* Standard Return Codes */
    eSOMEIP_E_OK                      = 0x00, ///< No error occurred
    eSOMEIP_E_NOT_OK                  = 0x01, ///< Unspecified error
    eSOMEIP_E_UNKNOWN_SERVICE         = 0x02, ///< Unknown service requested
    eSOMEIP_E_UNKNOWN_METHOD          = 0x03, ///< Unknown method requested

    /* Deprecated / Obsolete */
    eSOMEIP_E_NOT_READY               = 0x04, ///< Service not ready
    eSOMEIP_E_NOT_REACHABLE           = 0x05, ///< Service unreachable
    eSOMEIP_E_TIMEOUT                 = 0x06, ///< Timeout occurred

    /* Protocol and Message Errors */
    eSOMEIP_E_WRONG_PROTOCOL_VERSION  = 0x07, ///< Wrong protocol version
    eSOMEIP_E_WRONG_INTERFACE_VERSION = 0x08, ///< Wrong interface version
    eSOMEIP_E_MALFORMED_MESSAGE       = 0x09, ///< Malformed message received
    eSOMEIP_E_WRONG_MESSAGE_TYPE      = 0x0A  ///< Wrong message type
} SOMEIP_ReturnCode_e;

#endif /*__CONSTS_AND_ENUMS_H__*/