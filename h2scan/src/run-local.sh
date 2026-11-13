#!/bin/bash
# usage: run.sh <70|71> <scenario 1..5> [seconds]  (run inside goblint-suite docker, /h = h2scan dir)
v=$1; n=$2; t=${3:-560}
cd /src
SECONDS=0
timeout $t ./goblint \
  --set lib.activated[+] apr --set ana.activated[+] threadJoins \
  --set ana.malloc.unique_address_count 1 \
  --disable sem.unknown_function.spawn --disable sem.unknown_function.call \
  --disable sem.unknown_function.invalidate.globals --disable sem.unknown_function.invalidate.args \
  --disable sem.unknown_function.read.args \
  --set ana.activated[-] expRelation \
  --disable ana.base.context.non-ptr --disable ana.base.context.int \
  --set ana.ctx_insens[+] base --set ana.ctx_insens[+] mallocWrapper \
  --disable ana.thread.context.create-edges \
  --set dbg.solver-stats-interval 60 \
  $(ls /h/i$v/*.i | grep -v harness.i | grep -v h2_bucket_beam.i | grep -v h2_workers.i) /h/handoff/h$v/h2_workers_once.i /h/handoff/h$v/beam_model.i /h/handoff/h$v/handoff$n.i > /h/handoff/out$v-$n.txt 2> /h/handoff/err$v-$n.txt
echo "v=$v scen=$n exit=$? wall=$SECONDS"
