#include "Network.h"
#include "malloc.h"

/* GENERIC PAYLOAD UTILS *********************************************************************************************************/

void NewGenericPayload(GenericPayload_t* GP_Ptr, uint16_t Size) {
    /* Set new size for payload data */
    GP_Ptr->Size = Size;
    /* Allocate memory for payload data */
    if (GP_Ptr->Size > 0) {
        
        GP_Ptr->Ptr = malloc(GP_Ptr->Size);
        
        /* Ensure pointer is null if allocation fails */
        if (GP_Ptr->Ptr == NULL) {
            GP_Ptr->Size = 0;
        }
    }
}

void DeleteGenericPayload(GenericPayload_t* GP_Ptr) {
    /* Check and free allocated memory */
    if (GP_Ptr->Ptr != NULL) {
        free(GP_Ptr->Ptr);
        GP_Ptr->Ptr = NULL;
    }
    GP_Ptr->Size = 0;
}

uint16_t CopyGenericPayload(GenericPayload_t* GP_Ptr_Src, GenericPayload_t* GP_Ptr_Dst) {
    uint16_t bytesToCopy = 0;

    /* Validate source and destination pointers */
    if ((GP_Ptr_Src->Ptr != NULL) && (GP_Ptr_Dst->Ptr != NULL)) {
        /* Determine the safe amount of data to copy */
        bytesToCopy = (GP_Ptr_Src->Size < GP_Ptr_Dst->Size) ? GP_Ptr_Src->Size : GP_Ptr_Dst->Size;
        
        /* Perform memory copy */
        memcpy(GP_Ptr_Dst->Ptr, GP_Ptr_Src->Ptr, bytesToCopy);
    }
    
    return bytesToCopy;
}

/* NETWORK UTILS ****************************************************************************************************************/

/// @brief Convert a 4-byte array to an IPv4 string
/// @param IPv4Addr Input array of 4 bytes
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertByteArr2IPvToAddress(uint8_t IPv4Addr[], char IPv4Str[]) {
    if (IPv4Addr == NULL || IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* this is a comment - format bytes into string x.x.x.x */
    sprintf(IPv4Str, "%u.%u.%u.%u", IPv4Addr[0], IPv4Addr[1], IPv4Addr[2], IPv4Addr[3]);

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 4-byte array
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output array of 4 bytes
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToByteArr(char IPv4Str[], uint8_t IPv4Addr[]) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* this is a comment - parse and validate 4 segments */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* assign results with casting */
    for (int32_t i = 0; i < 4; i++) {
        IPv4Addr[i] = (uint8_t)ip[i];
    }

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 64-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint64_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt64(char IPv4Str[], uint64_t * IPv4Addr) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* parse string to temporary integers */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* this is a comment - shift and combine into 64-bit (Network Byte Order) */
    *IPv4Addr = ((uint64_t)(uint8_t)ip[0] << 24) | 
                ((uint64_t)(uint8_t)ip[1] << 16) | 
                ((uint64_t)(uint8_t)ip[2] << 8)  | 
                ((uint64_t)(uint8_t)ip[3]);

    return STAT_OKE;
}

/// @brief Convert a 64-bit integer to an IPv4 string
/// @param IPv4Addr Input 64-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt64ToIPv4Address(uint64_t IPv4Addr, char IPv4Str[]) {
    if (IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* extract bytes from integer using masking and shifting */
    uint8_t b0 = (uint8_t)((IPv4Addr >> 24) & 0xFF);
    uint8_t b1 = (uint8_t)((IPv4Addr >> 16) & 0xFF);
    uint8_t b2 = (uint8_t)((IPv4Addr >> 8)  & 0xFF);
    uint8_t b3 = (uint8_t)(IPv4Addr & 0xFF);

    /* this is a comment - write bytes to string buffer */
    sprintf(IPv4Str, "%u.%u.%u.%u", b0, b1, b2, b3);

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 32-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint32_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt32(char IPv4Str[], uint32_t * IPv4Addr) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* parse and validate 4 segments from input string [cite: 35, 46] */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* combine octets into 32-bit value in Network Byte Order */
    *IPv4Addr = ((uint32_t)(uint8_t)ip[0] << 24) | 
                ((uint32_t)(uint8_t)ip[1] << 16) | 
                ((uint32_t)(uint8_t)ip[2] << 8)  | 
                ((uint32_t)(uint8_t)ip[3]);

    return STAT_OKE;
}

/// @brief Convert a 32-bit integer to an IPv4 string
/// @param IPv4Addr Input 32-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt32ToIPv4Address(uint32_t IPv4Addr, char IPv4Str[]) {
    if (IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* extract bytes via bit shifting to handle endianness [cite: 141, 142] */
    uint8_t b0 = (uint8_t)((IPv4Addr >> 24) & 0xFF);
    uint8_t b1 = (uint8_t)((IPv4Addr >> 16) & 0xFF);
    uint8_t b2 = (uint8_t)((IPv4Addr >> 8)  & 0xFF);
    uint8_t b3 = (uint8_t)(IPv4Addr & 0xFF);

    /* format extracted bytes into dotted-decimal string */
    sprintf(IPv4Str, "%u.%u.%u.%u", b0, b1, b2, b3);

    return STAT_OKE;
}

/// @brief Convert IPv4 octets to a 32-bit integer in Big-endian (Network) order
/// @param Octet0 First octet (e.g., 192)
/// @param Octet1 Second octet (e.g., 168)
/// @param Octet2 Third octet (e.g., 1)
/// @param Octet3 Fourth octet (e.g., 10)
/// @return uint32_t The packed 32-bit IP address
uint32_t IPv4ToUint32(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3) {
    /* Shift octets into a 32-bit container in Big-endian order */
    return ((uint32_t)Octet0 << 24) | 
           ((uint32_t)Octet1 << 16) | 
           ((uint32_t)Octet2 << 8)  | 
           ((uint32_t)Octet3);
}

/// @brief Convert 6 MAC address octets to a 64-bit integer
/// @param Octet0 High byte of MAC (Most Significant Byte)
/// @param Octet1 Second byte
/// @param Octet2 Third byte
/// @param Octet3 Fourth byte
/// @param Octet4 Fifth byte
/// @param Octet5 Low byte of MAC (Least Significant Byte)
/// @return uint64_t The packed MAC address
uint64_t MACToUint64(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3, uint8_t Octet4, uint8_t Octet5) {
    /* Pack 48-bit MAC address into a 64-bit container in Big-endian order */
    return ((uint64_t)Octet0 << 40) |
           ((uint64_t)Octet1 << 32) |
           ((uint64_t)Octet2 << 24) |
           ((uint64_t)Octet3 << 16) |
           ((uint64_t)Octet4 << 8)  |
           ((uint64_t)Octet5);
}

/// @brief Convert a byte array to a 32-bit unsigned integer (Big Endian)
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to convert (max 4)
/// @return The converted 32-bit unsigned integer
uint32_t ConvertByteArrayToInt32_BigEndian(const uint8_t* ByteArr, uint8_t Size) {
    /* Initialize result and set processing limit to 4 bytes */
    uint32_t result = 0;
    uint8_t limit = (Size > 4) ? 4 : Size;

    /* Shift and OR bytes into integer starting from most significant byte */
    for (uint8_t i = 0; i < limit; i++) {
        result = (result << 8) | ByteArr[i];
    }

    return result;
}

/// @brief Convert a byte array to a 64-bit unsigned integer (Big Endian)
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to convert (max 8)
/// @return The converted 64-bit unsigned integer
uint64_t ConvertByteArrayToInt64_BigEndian(const uint8_t* ByteArr, uint8_t Size) {
    /* Initialize result and set processing limit to 8 bytes */
    uint64_t result = 0;
    uint8_t limit = (Size > 8) ? 8 : Size;

    /* Shift and OR bytes into integer starting from most significant byte */
    for (uint8_t i = 0; i < limit; i++) {
        result = (result << 8) | ByteArr[i];
    }

    return result;
}

/// @brief Convert a byte array to a 32-bit unsigned integer (Little Endian)
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to convert (max 4)
/// @return The converted 32-bit unsigned integer
uint32_t ConvertByteArrayToInt32_LittleEndian(const uint8_t* ByteArr, uint8_t Size) {
    /* Initialize result and set processing limit to 4 bytes */
    uint32_t result = 0;
    uint8_t limit = (Size > 4) ? 4 : Size;

    /* Shift and OR bytes into integer starting from least significant byte */
    for (uint8_t i = 0; i < limit; i++) {
        result |= ((uint32_t)ByteArr[i] << (8 * i));
    }

    return result;
}

/// @brief Convert a byte array to a 64-bit unsigned integer (Little Endian)
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to convert (max 8)
/// @return The converted 64-bit unsigned integer
uint64_t ConvertByteArrayToInt64_LittleEndian(const uint8_t* ByteArr, uint8_t Size) {
    /* Initialize result and set processing limit to 8 bytes */
    uint64_t result = 0;
    uint8_t limit = (Size > 8) ? 8 : Size;

    /* Shift and OR bytes into integer starting from least significant byte */
    for (uint8_t i = 0; i < limit; i++) {
        result |= ((uint64_t)ByteArr[i] << (8 * i));
    }

    return result;
}

/// @brief Reverse the byte order of an array in-place (Endianness conversion)
/// @param Arr Generic pointer to the data array
/// @param Size Total number of bytes to reverse
void ReverseByteEndian(GenericPtr_t Arr, EthSize_t Size){
    Byte_t      TempByte;
    EthSize_t   i;
    if(Size < 2 || Arr.Byte == NULL) return;
    for(i = 0; i < (Size>>1); ++i){
        TempByte              =   Arr.Byte[0+i];
        Arr.Byte[0+i]         =   Arr.Byte[(Size-1)-i];
        Arr.Byte[(Size-1)-i]  =   TempByte;
    }
}

/* EOF **************************************************************************************************************************/
