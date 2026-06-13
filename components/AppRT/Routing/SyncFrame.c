#include "SyncFrame.h"
#include "../AppConfig/All.h"

const ECUInfo_t ECUInfo = {
    .Fields = {
        .ECUName            = ECU_NAME_STR,
        .EthMACAddress      = SRC_MAC_ADDR_HEX,
        .IPv4Address        = SRC_IP_ADDR_HEX,
        .IPv4DefPort        = DEFAULT_PORT_VAL
    }
};
