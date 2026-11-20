#!/usr/bin/env bash
# newwarn.sh <stock.log> <other.log>  -> warnings present in <other> but not in the stock build log.
# Warnings are compared by (file, message) with line numbers stripped, so shifted code is not "new".
norm() { grep 'warning:' "$1" | sed -E 's#^.*/((core|platforms|synth|util|editor-fx-sdk|synth-plugin-sdk)/)#\1#; s/:[0-9]+(:[0-9]+)?:/:/' | sort -u; }
comm -13 <(norm "$1") <(norm "$2")
