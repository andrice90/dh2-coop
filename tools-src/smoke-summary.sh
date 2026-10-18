#!/bin/bash
# Digest of one smoke-maps.ps1 phase dir (host = game-dh2coop.log, clients = game2/game3-dh2coop.log).
# Usage: tools-src/smoke-summary.sh re/notes/coop-local/smoke/MAP3A/walk
d="$1"
strip() { sed 's/^\[[^]]*\] \[[a-z]*\] \[t[0-9]*\] //' "$1"; }
ts() { sed -n 's/^\[[0-9-]* \([0-9:.]*\)\].*/\1/p' "$1" | sed -n '1p;$p' | paste -sd' '; }
h="$d/game-dh2coop.log"
echo "== $d  status=$(tr -d '\357\273\277\r\n' < "$d/status.txt" 2>/dev/null)"
[ -f "$h" ] || { echo "no host log"; exit 0; }
echo "host log time $(ts "$h")  lines=$(wc -l < "$h")"
echo "run-output: $(grep -h 'welcomed pid\|exited\|did not log\|timed out\|closed after\|failed' "$d/run-output.txt" 2>/dev/null | tr -d '\357\273\277' | cut -c1-110 | paste -sd'|' | cut -c1-400)"
hid=$(strip "$h" | sed -n "s/^animals: sync role=host id=\([0-9]*\) pos=(\(-\?[0-9.]*\),.*/\1 \2/p" | sort -u -k1,1n)
echo "host animals unique=$(echo "$hid" | grep -c .) parked(-10000)=$(echo "$hid" | grep -c ' -10000')"
echo "host animal states: $(strip "$h" | sed -n "s/^animals: sync role=host id=[0-9]* .*state='\([^']*\)'.*/\1/p" | sort | uniq -c | sort -rn | head -8 | awk '{printf "%s=%s ",$2,$1}')"
hostfires() { strip "$1" | sed -n "s/^story: \[host\] trigger FIRE id=[0-9]* name='\([^']*\)'.*/\1/p; s/^story: \[host\] ran '\([^']*\)'.*/\1/p" | sort -u; }
echo "host trigger fires (unique): $(hostfires "$h" | paste -sd' ')"
echo "host not-found: $(strip "$h" | sed -n "s/^story: coop_trigger '\([^']*\)' not found/\1/p" | sort -u | paste -sd' ')"
maps() { strip "$1" | grep '^cBspTree::Load\|^cSpServer::StartGame' | sed 's/ in [0-9]*ms//; s/ (bsp loaded)//' | cut -c1-70 | paste -sd'|'; }
echo "host maps: $(maps "$h")"
echo "host deaths: $(strip "$h" | grep -c 'animals: death')  mission msgs: $(strip "$h" | grep -c 'events: host mission message')"
strip "$h" | grep 'events: host mission message\|events: commit kind=2 \|events: commit kind=4 ' | cut -c1-200 | head -6
for cl in game2 game3 game4; do
  c="$d/$cl-dh2coop.log"; [ -f "$c" ] || continue
  echo "-- client $cl log time $(ts "$c") lines=$(wc -l < "$c")"
  echo "client maps: $(maps "$c")"
  echo "welcomed=$(strip "$c" | grep -c 'coop: welcomed')  received: $(strip "$c" | sed -n 's/^animals: client received \([0-9]*\) animals.*/\1/p' | paste -sd',')"
  echo "mirror freeze npc=true ids: $(strip "$c" | sed -n "s/^animals: mirror freeze id=\([0-9]*\) ok=[a-z]* npc=true.*/\1/p" | sort -un | paste -sd' ')"
  echo "mirror freeze npc=false: $(strip "$c" | sed -n "s/^animals: mirror freeze id=\([0-9]*\) ok=[a-z]* npc=false.*/\1/p" | sort -un | wc -l)  ok=false: $(strip "$c" | grep -c 'mirror freeze id=[0-9]* ok=false')"
  hf=$(hostfires "$h")
  cf=$(strip "$c" | sed -n "s/^story: applied host fire '\([^']*\)'.*/\1/p; s/^story: host fire '\([^']*\)' (id [0-9]*) already ran here.*/\1/p" | sort -u)
  echo "client applied host fire (unique): $(echo "$cf" | paste -sd' ')"
  echo "host fired, client did not apply: $(comm -23 <(echo "$hf") <(echo "$cf") | paste -sd' ')"
  echo "client applied arm: $(strip "$c" | grep -c 'story: applied host trigger')  deferred fires: $(strip "$c" | grep -c 'story: defer host fire')"
  echo "client PlaySA applied=$(strip "$c" | grep -c 'story: applied PlaySA') propanim applied=$(strip "$c" | grep -c 'story: applied prop anim')  host PlaySA=$(strip "$h" | grep -c 'story: PlaySA id=') propanim=$(strip "$h" | grep -c 'story: prop anim id=')"
  echo "cutscene lines host=$(strip "$h" | grep -i 'cutscene' | grep -vc '^hook') client=$(strip "$c" | grep -i 'cutscene' | grep -vc '^hook')  (hold/done/over/defer/skip)"
  echo "client mission applied: $(strip "$c" | grep -c 'events: applied host mission end')  swallowed: $(strip "$c" | grep -c 'events: client mission message swallowed')"
  strip "$c" | grep 'events: following host\|events: host map\|events: synced with host' | cut -c1-220 | head -6
done
echo "-- problems (host/client counts)"
for pat in 'unhandled exception' 'first-chance' 'Could not forward' 'SetBodyState.*-> -1' 'no such trigger' 'not a cObjTarget' 'failed' 'faulted' 'prologue mismatch' 'unexpected bytes' 'unreadable' 'waiting for'; do
  line="$pat:"
  for f in "$d"/game*-dh2coop.log; do
    n=$(strip "$f" | grep -ci -- "$pat"); line="$line $(basename $f -dh2coop.log)=$n"
  done
  echo "$line"
done
for f in "$d"/game*-dh2coop.log; do
  echo "[$(basename $f -dh2coop.log)] error/warn lines (distinct):"
  strip "$f" | grep -i '^\[error\]\|first-chance\|warn' >/dev/null
  sed -n 's/^\[[^]]*\] \[\(error\|warning\)\] \[t[0-9]*\] \(.*\)/\1 \2/p' "$f" | sed 's/0x[0-9a-f]\+/0xN/g; s/-\?[0-9]\+\(\.[0-9]\+\)\?/N/g' | sort | uniq -c | sort -rn | head -8 | cut -c1-200
done
echo "-- first-chance sites (nvd3dum.dll is known noise at map load) and crashes"
for f in "$d"/game*-dh2coop.log; do
  b=$(basename "$f" -dh2coop.log)
  echo "[$b] first-chance: $(strip "$f" | sed -n 's/^first-chance exception \(0x[0-9a-f]*\) at \([^ ]*\) eip.*/\1@\2/p' | sort | uniq -c | sort -rn | awk '{printf "%s x%s; ", $2, $1}' | cut -c1-300)"
  strip "$f" | grep 'unhandled exception' | cut -c1-200 | head -2
done
echo "-- map loads (cBspTree::Load per tree: restart / next-level flows)"
for f in "$d"/game*-dh2coop.log; do
  echo "[$(basename "$f" -dh2coop.log)] $(sed -n "s/^\[[0-9-]* \([0-9:]*\)[0-9.]*\] .*cBspTree::Load('Bsp.\([A-Za-z0-9]*\)\.bsp'.*/\1 \2/p" "$f" | paste -sd'|')"
done
echo "host level flow: $(strip "$h" | grep 'coop: host restarts\|coop: host skips' | cut -c1-120 | paste -sd'|')"
for cl in "$d"/game[234]-dh2coop.log; do [ -f "$cl" ] || continue
  echo "[$(basename "$cl" -dh2coop.log)] following: $(strip "$cl" | sed -n "s/^events: following host to '\([^']*\)'.*/\1/p" | paste -sd'>') synced: $(strip "$cl" | grep -c 'events: synced with host')"
done
echo "-- log families (top 6 by count, digits folded; flood check)"
for f in "$d"/game*-dh2coop.log; do
  echo "[$(basename "$f" -dh2coop.log)] lines=$(wc -l < "$f"): $(strip "$f" | sed 's/[-0-9.]\+/N/g' | cut -d' ' -f1-3 | sort | uniq -c | sort -rn | head -6 | awk '{c=$1; $1=""; printf "%s x%s |", $0, c}' | cut -c1-400)"
done
echo "-- debug.log"
for f in "$d"/game*-debug.log; do
  echo "[$(basename $f -debug.log)] LUA nil=$(grep -c "attempt to call a nil" "$f") forward=$(grep -ci 'could not forward' "$f") fatal/assert=$(grep -ci 'fatal\|assert\|exception\|access violation' "$f") lastlines: $(tail -n 2 "$f" | tr -d '\r' | cut -c1-90 | paste -sd'|')"
done
ls "$d" 2>/dev/null | grep -i 'dmp' | sed 's/^/DUMP: /'
