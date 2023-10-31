#!/bin/bash

directory="./src"

if [ -d "$directory" ]; then
    echo "TODO's in $directory"
    grep -rli --include="*" "TODO" "$directory"
else
    echo "Directory does not exist"
fi
