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
  echo '| map / run | alive H/C | welcome, sync | animals H/C (mirrored) | trig host arm/fire, client arm/fire/sent, defer | first-chance (not nvd3dum) | crash | fwd H/C | lines/s H / C avg(peak) | -1 | refused/mismatch | host mission msg | build | clients 3-4 |'
  echo '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|'
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
# first-chance sites of one log as code@module+offset; $2 = run | shutdown, $3 = the run's status (shutdown only when ok)
fcsites() {
  tr -d '\r' < "$1" | awk -v want="$2" -v ok="$3" '
    match($0, /^\[[0-9-]+ [0-9:]+\.[0-9]+/) { t = substr($0, RSTART + 12, 12); split(t, a, ":"); s = a[1] * 3600 + a[2] * 60 + a[3]; last = s }
    /first-chance exception/ { n++; ts[n] = s; line[n] = $0 }
    END {
      for (i = 1; i <= n; i++) {
        cls = (ok == "ok" && last - ts[i] <= 3) ? "shutdown" : "run"
        if (cls != want) continue
        if (match(line[i], /exception 0x[0-9a-f]+ at [^ ]+/)) { x = substr(line[i], RSTART + 10, RLENGTH - 10); sub(/ at /, "@", x); print x }
      }
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
  # cobras cannot be frozen like animals (mirror freeze ok=false is expected for them): counted apart from real failures
  cob=$(norm "$h" | sed -n 's/^tracker: cobras at map load ids=//p' | head -1 | tr ',' '\n' | grep -c .)
  bad=$((mirF - cob)); [ "$bad" -lt 0 ] && bad=0
  # triggers: host armed = "trigger event activate" (solo/host role), fired = "trigger FIRE" on the host (or "ran" for a client's request)
  ha=$(norm "$h" | sed -n "s/^story: \[\(host\|solo\)\] trigger event activate id=[0-9]* name='\([^']*\)'.*/\2/p" | sort -u | wc -l)
  hf=$(norm "$h" | sed -n "s/^story: \[\(host\|solo\)\] trigger FIRE id=[0-9]* name='\([^']*\)'.*/\2/p; s/^story: \[host\] ran '\([^']*\)'.*/\1/p" | sort -u | wc -l)
  # client side: the map-load arms and fires run locally while it is still "solo" (before the join), later ones as [client]
  ca=$(norm "$c1" 2>/dev/null | sed -n "s/^story: applied host trigger '\([^']*\)'.*/\1/p; s/^story: \[\(client\|solo\)\] trigger event activate id=[0-9]* name='\([^']*\)'.*/\2/p" | sort -u | wc -l)
  cf=$(norm "$c1" 2>/dev/null | sed -n "s/^story: applied host fire '\([^']*\)'.*/\1/p; s/^story: host fire '\([^']*\)' (id [0-9]*) already ran here.*/\1/p; s/^story: \[\(client\|solo\)\] trigger FIRE id=[0-9]* name='\([^']*\)'.*/\2/p" | sort -u | wc -l)
  cs=$(cnt '^story: client fire ' "$c1")
  df=$(cnt '^story: defer host fire' "$c1")
  # host mission message (code 6 = "Mission failed": the unattended host hunter was killed), seconds since the host log began
  mline=$(norm "$h" | grep -m1 '^events: host mission message')
  if [ -n "$mline" ]; then
    mt=$(firstts 'events: host mission message' "$h"); t00=$(sed -n 's/^\[[0-9-]* \([0-9:.]*\)\].*/\1/p' "$h" | head -1)
    mis="code $(echo "$mline" | sed -n 's/.*code=\([-0-9]*\).*/\1/p') '$(echo "$mline" | sed -n "s/.*text='\(.\{0,16\}\).*/\1/p")' +$(awk -v a="$(sec "$mt")" -v b="$(sec "$t00")" 'BEGIN { printf "%.0f", a - b }')s"
  else mis="-"; fi
  # first-chance exceptions of every tree. nvd3dum.dll AVs at map load are known noise (counted, not listed). A site within
  # 3 s of the end of a log whose run ended ok is listed apart as "at shutdown" (the run script closes the windows then).
  fcrun=""; fcsd=""; nv=0
  for f in "$h" $clients; do
    b=$(basename "$f" -dh2coop.log)
    nv=$((nv + $(fcsites "$f" run "$st" | grep -ci nvd3dum) + $(fcsites "$f" shutdown "$st" | grep -ci nvd3dum)))
    fcrun="$fcrun$(fcsites "$f" run "$st" | grep -vi nvd3dum | sed "s|^|$b:|")"$'\n'
    fcsd="$fcsd$(fcsites "$f" shutdown "$st" | grep -vi nvd3dum | sed "s|^|$b:|")"$'\n'
  done
  fcrun=$(echo "$fcrun" | grep . | sort | uniq -c | sort -rn | awk '{ printf "%s x%s; ", $2, $1 }' | cut -c1-300)
  fcsd=$(echo "$fcsd" | grep . | sort | uniq -c | sort -rn | awk '{ printf "%s x%s; ", $2, $1 }' | cut -c1-300)
  fc="${fcrun:-none}"
  [ -n "$fcsd" ] && fc="$fc [at shutdown: ${fcsd%; }]"
  [ "$nv" != 0 ] && fc="$fc (+$nv nvd3dum)"
  unhs=""; for f in "$h" $clients; do n=$(cnt 'unhandled exception' "$f"); [ "$n" != 0 ] && unhs="$unhs $(basename "$f" -dh2coop.log):$n"; done
  dmp=$(ls "$d" 2>/dev/null | grep -i 'dmp' | paste -sd' ')
  crash="${unhs# }${dmp:+ $dmp}"; [ -z "$crash" ] && crash=no
  fwh=$(grep -ci 'could not forward' "$d/game-debug.log" 2>/dev/null); fwc=$(grep -ci 'could not forward' "$d/game2-debug.log" 2>/dev/null)
  rh=$(rate "$h"); rc=$(rate "$c1")
  n1=0; rf=0; for f in "$d"/game*-dh2coop.log; do n1=$((n1 + $(cnt 'SetBodyState.* -> -1' "$f"))); rf=$((rf + $(norm "$f" | grep -ci 'refused\|mismatch'))); done
  # build: build.txt of the run dir (the harness writes "main <hash>") + the asi build id the host logged
  bld=$(tr -d '\r\357\273\277\n' < "$d/build.txt" 2>/dev/null)
  asi=$(norm "$h" | sed -n 's/.* build=\([0-9a-f]\{6,\}\) password.*/\1/p' | head -1)
  bld="${bld:-?}${asi:+ (asi $asi)}"
  # further clients (3-4 player runs): welcome seconds, sync, received animals, first-chance not at nvd3dum, alive state
  extra="-"
  for cf3 in "$d"/game3-dh2coop.log "$d"/game4-dh2coop.log; do
    [ -f "$cf3" ] || continue
    r=$(basename "$cf3" -dh2coop.log)
    w3=$(tr -d '\r' < "$d/run-output.txt" 2>/dev/null | sed -n "s/.*$r welcomed pid=[0-9]* after \([0-9.]*\)s.*/\1/p" | head -1)
    sy3=$(cnt 'events: synced with host' "$cf3"); rc3=$(norm "$cf3" | sed -n 's/^animals: client received \([0-9]*\) animals.*/\1/p' | sort -n | tail -1)
    a3=$(tail -n 1 "$d/alive.txt" 2>/dev/null | tr -d '\r' | sed -n "s/.* $r=\([^ ]*\).*/\1/p")
    fc3=$(fcsites "$cf3" run "$st" | grep -vi nvd3dum | sort | uniq -c | awk '{ printf "%s x%s; ", $2, $1 }')
    [ "$extra" = "-" ] && extra=""
    extra="$extra$r: ${a3:-?}, welcome ${w3:-?}s, synced x$sy3, received ${rc3:-0}, rate $(rate "$cf3")${fc3:+, first-chance $fc3} | "
  done
  extra="${extra% | }"
  printf '| %s | %s/%s%s%s | %ss, sync %s | %s/%s (%s+%s npc, %s cobra, %s bad) | %s/%s, %s/%s/%s, defer %s | %s | %s | %s/%s | %s / %s | %s | %s | %s | %s | %s |\n' \
    "$name" "${ah:-?}" "${ac:-?}" "${ex:+ [$ex]}" "${st:+ <$st>}" "${wel:-?}" "$sync" "${hsnap:-0}" "${rcv:-0}" "$mirA" "$mirN" "$cob" "$bad" \
    "$ha" "$hf" "$ca" "$cf" "$cs" "$df" "$fc" "$crash" "${fwh:-0}" "${fwc:-0}" "$rh" "$rc" "$n1" "$rf" "$mis" "$bld" "$extra"
done
