#!/bin/bash

WSLPATH=$(wslpath -a "$(pwd)")
SERVER_EXE="${WSLPATH}/server/server.exe"
CLIENT_EXE="${WSLPATH}/client/client.exe"

if ! command -v wine &> /dev/null; then
    echo "wine is not installed. Installing..."
    sudo apt update && sudo apt install -y wine
fi

wine "${SERVER_EXE}" &
SERVER_PID=$!

sleep 2

wine "${CLIENT_EXE}"

kill $SERVER_PID