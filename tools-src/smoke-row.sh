#!/bin/bash
# Markdown table row for one smoke run dir written by smoke-maps.ps1 (job smoke-all-maps, re/notes/smoke-all-maps.md).
# Usage: tools-src/smoke-row.sh re/notes/coop-local/smoke/MAP3A/base [more dirs...]    (one row per dir)
#        SMOKE_HEADER=1 tools-src/smoke-row.sh                                         (table header only)
# Dir contents: game-dh2coop.log (host), game2/3/4-dh2coop.log (clients), *-debug.log, status.txt, alive.txt,
# run-output.txt. Columns:
#   alive H/C   host / client process at the end (alive.txt: the watchdog's last poll); [..] = exit code line when one died
#   welcome     seconds from client start to "coop: welcomed" (run-output.txt); sync = "events: synced with host", s after welcome
#   animals     host snapshot size / client "received N animals" (mirror freeze lines: animals + npcs, bad = ok=false)
#   trig        host armed/fired (unique trigger names), client armed/applied-fire/own fire sent, defer = "defer host fire" lines
#   first-chance  exceptions that are not nvd3dum.dll (tree:code@module+offset xcount); crash = unhandled exception / dump
#   fwd         "Could not forward animation" in debug.log, host/client
#   lines/s     dh2coop.log lines per second, host / client: average (peak second)
#   -1          SetBodyState ... -> -1 lines (all logs);  refused/mismatch lines (all logs)
if [ -n "$SMOKE_HEADER" ]; then
  echo '| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch |'
  echo '|---|---|---|---|---|---|---|---|---|---|---|'
  exit 0
fi
norm() { tr -d '\r\357\273\277' < "$1" | sed 's/^\[[^]]*\] \[[a-z]*\] \[t[0-9]*\] //'; }
# lines per second: avg and peak (per wall-clock second of the [yyyy-mm-dd hh:mm:ss.mmm] stamp), without the first
# 10 s of the log (hook installs and the map load put ~120 lines into the first second of every process)
rate() {
  tr -d '\r' < "$1" | awk '
    match($0, /^\[[0-9-]+ [0-9:]+/) {
      t = substr($0, RSTART + 12, 8); split(t, a, ":"); s = a[1] * 3600 + a[2] * 60 + a[3]
      c[s]++; if (min == "" || s < min) min = s; if (s > max) max = s
    }
    END {
      if (min == "") { print "-"; exit }
      for (s = min + 10; s <= max; s++) { n += c[s]; if (c[s] > pk) pk = c[s] }
      span = max - min - 9; if (span < 1) { print "-"; exit }
      printf "%.1f(%d)", n / span, pk
    }'
}
sec() { awk -F: '{ print $1 * 3600 + $2 * 60 + $3 }' <<< "$1"; }
firstts() { sed -n "s/^\[[0-9-]* \([0-9:.]*\)\].*$1.*/\1/p" "$2" | head -1; }
cnt() { local n; n=$(norm "$2" 2>/dev/null | grep -c -- "$1"); echo "${n:-0}"; }
for d in "$@"; do
  d="${d%/}"
  h="$d/game-dh2coop.log"
  name="${d#re/notes/coop-local/smoke/}"
  [ -f "$h" ] || { echo "| $name | no host log |"; continue; }
  clients=$(ls "$d"/game[234]-dh2coop.log 2>/dev/null)
  c1="$d/game2-dh2coop.log"
  st=$(tr -d '\357\273\277\r\n' < "$d/status.txt" 2>/dev/null)
  alive=$(tail -n 1 "$d/alive.txt" 2>/dev/null | tr -d '\r')
  ah=$(echo "$alive" | sed -n 's/.*game=\([^ ]*\).*/\1/p'); ac=$(echo "$alive" | sed -n 's/.* game2=\([^ ]*\).*/\1/p')
  ex=$(tr -d '\r\357\273\277' < "$d/run-output.txt" 2>/dev/null | sed -n 's/.*\(game[0-9]* pid=[0-9]* exited[^|]*(code [-0-9a-fx]*)\).*/\1/p' | head -2 | tr '\n' ' ')
  wel=$(tr -d '\r' < "$d/run-output.txt" 2>/dev/null | sed -n 's/.*game2 welcomed pid=[0-9]* after \([0-9.]*\)s.*/\1/p' | head -1)
  tw=$(firstts 'coop: welcomed as player' "$c1"); tsy=$(firstts 'events: synced with host' "$c1")
  if [ -n "$tsy" ] && [ -n "$tw" ]; then sync=$(awk -v a="$(sec "$tsy")" -v b="$(sec "$tw")" 'BEGIN { printf "+%.1fs", a - b }'); else sync=NO; fi
  hsnap=$(norm "$h" | sed -n 's/^animals: host snapshot \([0-9]*\) animals.*/\1/p' | sort -n | tail -1)
  rcv=$(norm "$c1" 2>/dev/null | sed -n 's/^animals: client received \([0-9]*\) animals.*/\1/p' | sort -n | tail -1)
  mirA=$(norm "$c1" 2>/dev/null | sed -n 's/^animals: mirror freeze id=\([0-9]*\) ok=true npc=false.*/\1/p' | sort -un | wc -l)
  mirN=$(norm "$c1" 2>/dev/null | sed -n 's/^animals: mirror freeze id=\([0-9]*\) ok=true npc=true.*/\1/p' | sort -un | wc -l)
  mirF=$(cnt 'mirror freeze id=[0-9]* ok=false' "$c1")
  # triggers: host armed = "trigger event activate" (solo/host role), fired = "trigger FIRE" on the host (or "ran" for a client's request)
  ha=$(norm "$h" | sed -n "s/^story: \[\(host\|solo\)\] trigger event activate id=[0-9]* name='\([^']*\)'.*/\2/p" | sort -u | wc -l)
  hf=$(norm "$h" | sed -n "s/^story: \[\(host\|solo\)\] trigger FIRE id=[0-9]* name='\([^']*\)'.*/\2/p; s/^story: \[host\] ran '\([^']*\)'.*/\1/p" | sort -u | wc -l)
  ca=$(norm "$c1" 2>/dev/null | sed -n "s/^story: applied host trigger '\([^']*\)'.*/\1/p; s/^story: \[client\] trigger event activate id=[0-9]* name='\([^']*\)'.*/\1/p" | sort -u | wc -l)
  cf=$(norm "$c1" 2>/dev/null | sed -n "s/^story: applied host fire '\([^']*\)'.*/\1/p; s/^story: host fire '\([^']*\)' (id [0-9]*) already ran here.*/\1/p" | sort -u | wc -l)
  cs=$(cnt '^story: client fire ' "$c1")
  df=$(cnt '^story: defer host fire' "$c1")
  # first-chance exceptions of every tree (nvd3dum.dll AVs at map load are known noise)
  fc=$(for f in "$h" $clients; do norm "$f" | sed -n 's/^first-chance exception \(0x[0-9a-f]*\) at \([^ ]*\) eip.*/\1@\2/p' | sed "s|^|$(basename "$f" -dh2coop.log):|"; done | grep -vi 'nvd3dum' | sort | uniq -c | sort -rn | awk '{ printf "%s x%s; ", $2, $1 }' | cut -c1-300)
  [ -z "$fc" ] && fc=none
  nv=0; for f in "$h" $clients; do nv=$((nv + $(cnt 'first-chance exception.*nvd3dum' "$f"))); done
  [ "$nv" != 0 ] && fc="$fc (+$nv nvd3dum)"
  unhs=""; for f in "$h" $clients; do n=$(cnt 'unhandled exception' "$f"); [ "$n" != 0 ] && unhs="$unhs $(basename "$f" -dh2coop.log):$n"; done
  dmp=$(ls "$d" 2>/dev/null | grep -i 'dmp' | paste -sd' ')
  crash="${unhs# }${dmp:+ $dmp}"; [ -z "$crash" ] && crash=no
  fwh=$(grep -ci 'could not forward' "$d/game-debug.log" 2>/dev/null); fwc=$(grep -ci 'could not forward' "$d/game2-debug.log" 2>/dev/null)
  rh=$(rate "$h"); rc=$(rate "$c1")
  n1=0; rf=0; for f in "$d"/game*-dh2coop.log; do n1=$((n1 + $(cnt 'SetBodyState.* -> -1' "$f"))); rf=$((rf + $(norm "$f" | grep -ci 'refused\|mismatch'))); done
  printf '| %s | %s/%s%s%s | %ss, sync %s | %s/%s (%s+%s npc, %s bad) | %s/%s, %s/%s/%s, defer %s | %s | %s | %s/%s | %s / %s | %s | %s |\n' \
    "$name" "${ah:-?}" "${ac:-?}" "${ex:+ [$ex]}" "${st:+ <$st>}" "${wel:-?}" "$sync" "${hsnap:-0}" "${rcv:-0}" "$mirA" "$mirN" "$mirF" \
    "$ha" "$hf" "$ca" "$cf" "$cs" "$df" "$fc" "$crash" "${fwh:-0}" "${fwc:-0}" "$rh" "$rc" "$n1" "$rf"
done
