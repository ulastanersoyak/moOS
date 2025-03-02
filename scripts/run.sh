#!/bin/bash

cd "$(dirname "$0")/.." || exit 1
make clean
make run
