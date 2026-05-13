#!/bin/zsh

## @file flash2esp.sh
## @brief Script to automate building and flashing for dual ESP32 setup with Git branch management.

# --- Configuration ---
IDF_PATH="/home/fus/esp"
EXPORT_SCRIPT="$IDF_PATH/export.sh"

# Slot A Config
ESP32_A_DEV="/dev/ttyUSB0"
ESP32_A_BRANCH="Lab_BZECU"
ESP32_A_TARGET_DIR="/home/fus/Documents/ZonalArchECU_W_SomeIP_A"

# Slot B Config
ESP32_B_DEV="/dev/ttyUSB1"
ESP32_B_BRANCH="Lab_BZECU"
ESP32_A_TARGET_DIR="/home/fus/Documents/ZonalArchECU_W_SomeIP_B"



# --- Functions ---

#
# @brief Check if the device node exists.
# @param device Path to the device, e.g., /dev/ttyUSB0
#/
check_device() {
    if [[ ! -c "$1" ]]; then
        echo "Error: Device $1 not found!"
        return 1
    fi
    return 0
}

#
# @brief Manage git state before switching context.
#/
manage_git_state() {
    local target_branch=$1
    echo "Stashing uncommitted changes..."
    git stash
    
    echo "Switching to branch: $target_branch"
    git checkout "$target_branch" || { echo "Failed to checkout $target_branch"; exit 1; }
    
    # Optional: Pop stash if you want to bring changes to the target branch
    # git stash pop
}

#
# @brief Build and Flash process.
# @param device The serial port to flash to.
#/
process_flash() {
    local device=$1
    
    # Source ESP-IDF environment
    source "$EXPORT_SCRIPT" > /dev/null 2>&1

    # Build and Flash
    echo "Building and Flashing to $device..."
    idf.py -p "$device" build flash monitor
}

# --- Main Logic ---

# Parse Arguments
BUILD=false
FLASH=false
SLOT=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --build) BUILD=true; shift ;;
        --flash) FLASH=true; shift ;;
        --slot) SLOT=$2; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Validate Slot
if [[ -z "$SLOT" ]]; then
    echo "Usage: $0 --build --flash --slot [A|B|AB|BA]"
    exit 1
fi

# Execution Loop based on Slot
for (( i=0; i<${#SLOT}; i++ )); do
    CURRENT_SLOT="${SLOT:$i:1}"
    echo "--- Processing Slot $CURRENT_SLOT ---"

    case $CURRENT_SLOT in
        A)
            check_device "$ESP32_A_DEV" || continue
            manage_git_state "$ESP32_A_BRANCH"
            [[ "$FLASH" == true ]] && process_flash "$ESP32_A_DEV"
            ;;
        B)
            check_device "$ESP32_B_DEV" || continue
            manage_git_state "$ESP32_B_BRANCH"
            [[ "$FLASH" == true ]] && process_flash "$ESP32_B_DEV"
            ;;
        *)
            echo "Invalid slot: $CURRENT_SLOT"
            ;;
    esac
done

# Return to original state if needed
echo "Done!"
