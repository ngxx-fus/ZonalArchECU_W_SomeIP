import time

class NodeAuth:
    """
    Python implementation of the Zonal ZECU/CCU Authentication library.
    """
    
    # Masks to simulate C-style fixed-width integers
    MASK_32 = 0xFFFFFFFF
    MASK_64 = 0xFFFFFFFFFFFFFFFF
    GOLDEN_RATIO_PRIME = 0x9E3779B97F4A7C15
    
    def __init__(self):
        self._core_key = 0
        
    def set_core_key(self, key: int) -> None:
        """
        Sets the core authentication key. Must match the embedded side.
        """
        self._core_key = key & self.MASK_32
        
    def _calculate_mac(self, challenge: int) -> int:
        """
        Internal 64-bit MAC calculation mirroring the C implementation.
        """
        # key_64 = ((uint64_t)core_key << 32) | core_key;
        key_64 = ((self._core_key << 32) | self._core_key) & self.MASK_64
        
        # mac = challenge ^ key_64;
        mac = (challenge & self.MASK_64) ^ key_64
        
        # mac = (mac << 13) | (mac >> 51); /* Circular shift left */
        mac = ((mac << 13) & self.MASK_64) | (mac >> 51)
        
        # mac *= 0x9E3779B97F4A7C15ULL;
        mac = (mac * self.GOLDEN_RATIO_PRIME) & self.MASK_64
        
        # mac ^= (mac >> 27);
        mac ^= (mac >> 27)
        
        return mac

    def generate_challenge(self, target_id: int, timestamp: int = None) -> int:
        """
        Generates a 64-bit challenge. If timestamp is None, uses current system time.
        """
        if timestamp is None:
            timestamp = int(time.time())
            
        target_id &= self.MASK_32
        timestamp &= self.MASK_32
        
        # challenge = ((uint64_t)timestamp << 32) | (uint64_t)target_id;
        challenge = ((timestamp << 32) | target_id) & self.MASK_64
        return challenge

    def sign_challenge(self, challenge: int) -> int:
        """
        Signs the challenge. Returns the 64-bit signature.
        Raises ValueError if core key is not initialized.
        """
        if self._core_key == 0:
            raise ValueError("STAT_ERR_INVALID_STATE: Core key not set.")
            
        return self._calculate_mac(challenge)

    def verify_signature(self, original_challenge: int, received_signature: int) -> bool:
        """
        Verifies the signature. Returns True if valid (STAT_OKE), False otherwise (STAT_ERR_PERMISSION).
        """
        if self._core_key == 0:
            raise ValueError("STAT_ERR_INVALID_STATE: Core key not set.")
            
        expected_signature = self._calculate_mac(original_challenge)
        
        # Match exactly with the received 64-bit signature
        return expected_signature == (received_signature & self.MASK_64)