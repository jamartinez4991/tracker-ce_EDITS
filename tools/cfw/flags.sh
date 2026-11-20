#!/usr/bin/env bash
# flags.sh <feature>... | --merged | --stock
# Prints the -D flags for the given features plus everything they depend on (from features.tsv).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
tsv="$here/features.tsv"
[ $# -ge 1 ] || { echo "usage: flags.sh <feature>... | --merged | --stock" >&2; exit 2; }
if [ "$1" = "--stock" ]; then echo ""; exit 0; fi
if [ "$1" = "--merged" ]; then set -- $(awk -F'\t' 'NR>1 && $7=="y"{print $1}' "$tsv"); fi
awk -F'\t' -v want="$*" '
NR==1{next}
{ flags[$1]=$2; dep[$1]=$6; known[$1]=1 }
function add(f,   n,a,i){
  if(!(f in known)){ print "unknown feature: " f > "/dev/stderr"; bad=1; return }
  if(seen[f]++) return
  if(dep[f]!="-"){ n=split(dep[f],a,","); for(i=1;i<=n;i++) add(a[i]) }
  n=split(flags[f],a,","); for(i=1;i<=n;i++) out[a[i]]=1
}
END{
  n=split(want,w," "); for(i=1;i<=n;i++) add(w[i])
  if(bad) exit 1
  s=""; for(k in out) s=s "\n-D" k
  cmd="sort"; print substr(s,2) | cmd; close(cmd)
}' "$tsv" | tr '\n' ' ' | sed 's/ $//'; echo
