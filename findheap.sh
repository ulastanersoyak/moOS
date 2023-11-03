#!/bin/bash

directory="./src"

if [ -d "$directory" ]; then
    echo "TODO's in $directory"
    grep -rli --include="*" "kcalloc" "$directory" | xargs -I {} sh -c 'echo "File: {}"; grep -i "kcalloc" {}'
else
    echo "Directory does not exist"
fi
