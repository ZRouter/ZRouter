#!/bin/sh

out=0;

for sz in `ps -axo rss=` ; do
    out=$(( ${out} + ${sz} ));
done

echo "Memory used by userland programs: ${out}K"

sysctl vm.vmtotal
