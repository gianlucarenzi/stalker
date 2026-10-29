#!/bin/bash

# This script sets up the environment for running the generate_cpl.py script
# and then executes it.

# --- Configuration ---
# Path to the directory containing the KiCad Python modules
PYTHON_MODULE_PATH="/usr/lib/python3/dist-packages"
PYTHON3=$(which python3)

# Get the directory where this script is located
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

# Absolute path to the generate_cpl.py script (assumed to be in the same directory)
PYTHON_SCRIPT="$SCRIPT_DIR/generate_cpl.py"
# --- End Configuration ---

# Check if the user provided a path to the .kicad_pcb file
if [ -z "$1" ]; then
    echo "Error: You must provide the path to the .kicad_pcb file."
    echo "Usage: $0 /path/to/your/project.kicad_pcb"
    exit 1
fi

KICAD_PCB_FILE="$1"

# The pcbnew module comes from a system KiCad 5 (e.g. Debian 11) when there
# is one, otherwise from the KiCad 5 AppImage ("kicad5 --python", v2.2.0+)
if PYTHONPATH=$PYTHON_MODULE_PATH ${PYTHON3} -c "import pcbnew" >/dev/null 2>&1; then
    echo "► Using system pcbnew, PYTHONPATH: $PYTHON_MODULE_PATH"
    RUN_PYTHON=(env PYTHONPATH="$PYTHON_MODULE_PATH" "${PYTHON3}")
elif command -v kicad5 >/dev/null 2>&1; then
    echo "► Using pcbnew from the KiCad 5 AppImage (kicad5 --python)"
    RUN_PYTHON=(kicad5 --python)
else
    echo "Error: no pcbnew module found. Install KiCad 5 or put the KiCad 5"
    echo "AppImage (v2.2.0 or later) in the PATH as 'kicad5'."
    exit 1
fi
echo "► Executing script: $PYTHON_SCRIPT"
echo "► Processing file: $KICAD_PCB_FILE"
echo "--------------------------------------------------"

"${RUN_PYTHON[@]}" "$PYTHON_SCRIPT" "$KICAD_PCB_FILE"
RET=$?

echo "--------------------------------------------------"
if [ $RET -ne 0 ]; then
    echo "✘ Script execution failed."
    exit $RET
fi
echo "✔ Script execution finished."

