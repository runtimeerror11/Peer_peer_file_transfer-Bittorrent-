#!/bin/bash
# Usage: ./run_tracker.sh <tracker_info.txt> <tracker_id>
g++ maint.cpp helpert.cpp commands_t.cpp -o tracker
if [ $? -eq 0 ]; then
    ./tracker "$1" "$2"
else
    echo "Tracker compilation failed."
fi

# Cretaing executable file -->
# chmod +x run_tracker.sh

#Run the tracker -->
# ./run_tracker.sh tracker_info.txt 1