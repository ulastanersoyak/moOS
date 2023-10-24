#!/bin/bash
if [ -z "$1" ]; then
  echo "provide commit msg"
  exit -1
fi

make clean
git add .
git commit -m "$1"
git push
