#!/bin/bash

directory="../src"

if [ -d "$directory" ]; then
    echo "TODO's in $directory"
    grep -rli --include="*" "TODO" "$directory" | xargs -I {} sh -c 'echo "File: {}"; grep -i "TODO" {}'
else
    echo "Directory does not exist"
fi
