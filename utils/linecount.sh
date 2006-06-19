#!/bin/sh
find ../src | grep -E '\.(cpp|h)$' | grep -v 'lua/' | xargs -P 1 wc -l

# euans unreadable find arg version
#find ../src -type d -iname lua -prune -o -iname \*.cpp -print -o -iname \*.h -print | xargs -P 1 wc -l