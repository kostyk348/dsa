#!/bin/bash
# DSA Steam Launcher — installs DSA audio into a Steam game directory
# and launches the game.
#
# Usage:
#   ./steam-with-dsa.sh install <appid>     # install DSA into game dir
#   ./steam-with-dsa.sh uninstall <appid>   # restore original FMOD libs
#   ./steam-with-dsa.sh launch <appid>      # launch game with DSA
#
# Game directories (Steam library paths):
#   ~/.steam/steam/steamapps/common/<name>/
#   ~/.local/share/Steam/steamapps/common/<name>/

set -euo pipefail

DSA_DIR="$(cd "$(dirname "$0")" && pwd)"
FMOD_SRC="$DSA_DIR/libfmod.so"
STUDIO_SRC="$DSA_DIR/libfmodstudio.so"

# Known appid → game directory mappings
declare -A GAME_DIRS
GAME_DIRS[2861150]="Slay the Spire 2"
GAME_DIRS[1019280]="100 Orange Juice"

# Search Steam library paths
find_game_dir() {
    local appid="$1"
    local name="${GAME_DIRS[$appid]:-}"
    if [ -z "$name" ]; then
        # Try to find by searching common paths for the appid
        for base in "$HOME/.steam/steam/steamapps/common" "$HOME/.local/share/Steam/steamapps/common"; do
            for d in "$base"/*/; do
                if [ -f "${d}steam_appid.txt" ] && [ "$(cat "${d}steam_appid.txt")" = "$appid" ]; then
                    echo "${d%/}"
                    return 0
                fi
            done
        done
        return 1
    fi
    for base in "$HOME/.steam/steam/steamapps/common" "$HOME/.local/share/Steam/steamapps/common"; do
        if [ -d "$base/$name" ]; then
            echo "$base/$name"
            return 0
        fi
    done
    return 1
}

cmd_install() {
    local appid="$1"
    local dir
    dir="$(find_game_dir "$appid")" || { echo "Game dir not found for appid $appid"; exit 1; }
    echo "[dsa] Installing DSA into $dir"

    # Backup originals
    for lib in libfmod.so libfmod.so.14 libfmod.so.14.6 libfmodstudio.so libfmodstudio.so.14 libfmodstudio.so.14.6; do
        if [ -f "$dir/$lib" ] && [ ! -f "$dir/$lib.dsa.bak" ]; then
            cp "$dir/$lib" "$dir/$lib.dsa.bak"
            echo "[dsa]   Backed up $lib → $lib.dsa.bak"
        fi
    done

    # Copy DSA libs
    cp "$FMOD_SRC" "$dir/libfmod.so"
    cp "$STUDIO_SRC" "$dir/libfmodstudio.so"
    # Create symlinks for versioned names
    ln -sf libfmod.so "$dir/libfmod.so.14" 2>/dev/null || true
    ln -sf libfmod.so "$dir/libfmod.so.14.6" 2>/dev/null || true
    ln -sf libfmodstudio.so "$dir/libfmodstudio.so.14" 2>/dev/null || true
    ln -sf libfmodstudio.so "$dir/libfmodstudio.so.14.6" 2>/dev/null || true
    echo "[dsa] DSA installed. Launch via Steam normally."
}

cmd_uninstall() {
    local appid="$1"
    local dir
    dir="$(find_game_dir "$appid")" || { echo "Game dir not found for appid $appid"; exit 1; }
    echo "[dsa] Restoring originals in $dir"

    for lib in libfmodstudio.so.14.6 libfmodstudio.so.14 libfmodstudio.so libfmod.so.14.6 libfmod.so.14 libfmod.so; do
        if [ -f "$dir/$lib.dsa.bak" ]; then
            mv "$dir/$lib.dsa.bak" "$dir/$lib"
            echo "[dsa]   Restored $lib"
        fi
    done
    echo "[dsa] Originals restored."
}

cmd_launch() {
    local appid="$1"
    shift
    echo "[dsa] Launching Steam app $appid with DSA..."
    exec steam "steam://rungameid/$appid" "$@"
}

case "${1:-help}" in
    install)
        cmd_install "$2"
        ;;
    uninstall)
        cmd_uninstall "$2"
        ;;
    launch)
        cmd_launch "$2"
        ;;
    *)
        echo "DSA Steam Launcher"
        echo ""
        echo "Commands:"
        echo "  install <appid>     Install DSA into game directory"
        echo "  uninstall <appid>   Restore original FMOD libs"
        echo "  launch <appid>      Launch game with DSA"
        echo ""
        echo "Known games:"
        echo "  2861150  Slay the Spire 2"
        echo "  1019280  100 Orange Juice"
        echo ""
        echo "After 'install', just launch the game normally from Steam."
        exit 0
        ;;
esac
