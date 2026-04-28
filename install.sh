#!/bin/sh
set -e
 
command -v gcc      >/dev/null 2>&1 || { echo "error: gcc not found"    >&2; exit 1; }
command -v git      >/dev/null 2>&1 || { echo "error: git not found"    >&2; exit 1; }
pkg-config --exists json-c          || { echo "error: json-c not found" >&2; exit 1; }
 
git clone https://github.com/Teemsploit/SoberFFlagEditor.git
cd SoberFFlagEditor
gcc $(pkg-config --cflags --libs json-c) -O2 -o fflag SoberFFlagEditor.c
sudo install -Dm755 fflag /usr/local/bin/fflag
echo "done"
