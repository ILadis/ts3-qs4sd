#!/bin/bash

export SELF_SRC="$(realpath ${BASH_SOURCE})"
export SELF_DIR="$(dirname "${SELF_SRC}")"

export PRIVILIGED_PATH="${SELF_DIR}"
export PLUGIN_PATH="${SELF_DIR}/plugins"

export LIVE_RELOAD=1
export LOG_LEVEL=DEBUG

if [ ! -f "${SELF_DIR}/service/PluginLoader" ]; then
  mkdir -p "${SELF_DIR}/service"
  wget 'https://github.com/SteamDeckHomebrew/decky-loader/releases/download/v2.10.1/PluginLoader' \
    -qO "${SELF_DIR}/service/PluginLoader"
fi

# For testing on Steam Deck setup port forwarding between
# development machine and Steam Deck via SSH:
#
# $ ssh -T -L 8080:127.0.0.1:8080 deck
# $ ssh -T -R 1337:127.0.0.1:1337 deck

chmod +x "${SELF_DIR}/service/PluginLoader"
exec "${SELF_DIR}/service/PluginLoader"