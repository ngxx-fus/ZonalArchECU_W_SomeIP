#!/usr/bin/env zsh

## @file run_monitor.zsh
## @brief Compiles a Qt Designer UI file to Python and executes the main application.
## @details This script uses the isolated Conda environment tools directly via absolute paths,
## avoiding the need to manually activate the environment beforehand.

## @defgroup Config Environment and Path Configuration
## @{

## @var WORKSPACE
## @brief Absolute path to the project working directory.
WORKSPACE="/home/fus/Documents/Refactor_ZonalArchECU_Front/monitor/"

## @var PYTHON
## @brief Absolute path to the Python interpreter within the target Conda environment.
PYTHON="/home/fus/miniconda3/envs/python38/bin/python"

## @var UIC
## @brief Absolute path to the PySide6 UI compiler within the target Conda environment.
UIC="/home/fus/miniconda3/envs/python38/bin/pyside6-uic"

## @var FILE_UI
## @brief Source Qt Designer UI file.
FILE_UI="${WORKSPACE}monitor.ui"

## @var FILE_UI_CONVERTED_PYTHON
## @brief Output Python file generated from the UI file.
FILE_UI_CONVERTED_PYTHON="${WORKSPACE}monitor_ui.py"

## @var FILE_MAIN
## @brief The primary application entry point script.
FILE_MAIN="${WORKSPACE}MonitorMain.py"

## @}

## @brief Main execution routine.
## @details Validates file existence, converts the UI layout, and launches the monitor application.
function main() {
    # Step 1: Validate and compile the UI file
    if [[ -f "$FILE_UI" ]]; then
        echo "Compiling UI: $FILE_UI -> $FILE_UI_CONVERTED_PYTHON"
        "$UIC" "$FILE_UI" -o "$FILE_UI_CONVERTED_PYTHON"
        
        # Check the exit status of the compiler
        if [[ $? -ne 0 ]]; then
            echo "Fatal: pyside6-uic compilation failed."
            exit 1
        fi
    else
        echo "Fatal: Source UI file not found at $FILE_UI"
        exit 1
    fi

    # Step 2: Validate and execute the main Python application
    if [[ -f "$FILE_MAIN" ]]; then
        echo "Launching application: $FILE_MAIN"
        "$PYTHON" "$FILE_MAIN"
    else
        echo "Fatal: Main execution script not found at $FILE_MAIN"
        exit 1
    fi
}

# Execute the main function
main