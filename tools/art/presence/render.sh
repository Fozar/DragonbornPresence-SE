#!/usr/bin/env bash
# Renders every icon of presence.html to assets/icons/<key>.png (512x512) with headless Edge/Chrome.
#   tools/art/presence/render.sh [key...]     (no keys = all)
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
out="$here/../../../assets/icons"
browser="${BROWSER:-/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe}"
page="file:///$(cygpath -m "$here")/presence.html"
mkdir -p "$out"

keys=("$@")
if [ ${#keys[@]} -eq 0 ]; then
  mapfile -t keys < <("$browser" --headless=new --allow-file-access-from-files --virtual-time-budget=3000 \
      --dump-dom "$page#list" 2>/dev/null | sed -n '/<pre id="keys">/,/<\/pre>/p' | sed 's/<[^>]*>//g' | grep -E '^[a-z_]+$')
fi

files=()
for key in "${keys[@]}"; do
  "$browser" --headless=new --hide-scrollbars --allow-file-access-from-files --window-size=512,512 \
      --virtual-time-budget=3000 --screenshot="$(cygpath -w "$out/$key.png")" "$page#$key" >/dev/null 2>&1
  files+=("$(cygpath -w "$out/$key.png")")
done
"${PYTHON:-python}" "$(cygpath -w "$here/optimize.py")" "${files[@]}"   # needs Pillow
echo "${#keys[@]} icons -> $(cygpath -w "$out")"
