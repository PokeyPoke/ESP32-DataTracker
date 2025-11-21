#!/bin/bash

# ESP32 DataTracker2 - Local Web Server for Firmware Flashing
# Double-click this file to start the web server

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR/docs"

# Find an available port (try 8000, 8080, 8888 in order)
PORT=8000
if lsof -Pi :8000 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    PORT=8080
fi
if lsof -Pi :8080 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    PORT=8888
fi

echo "========================================="
echo "  ESP32 DataTracker2 Firmware Flasher"
echo "========================================="
echo ""
echo "Starting local web server..."
echo "Open your browser to:"
echo ""
echo "  http://localhost:$PORT"
echo ""
echo "Press Ctrl+C to stop the server"
echo "========================================="
echo ""

# Start Python web server (try python3 first, then python)
if command -v python3 &> /dev/null; then
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    python -m http.server $PORT
else
    echo "ERROR: Python not found!"
    echo "Please install Python 3 to use this tool."
    read -p "Press Enter to exit..."
    exit 1
fi
