#!/bin/bash
exec >> ~/rombundler-log.txt 2>&1
cd "$(dirname "$0")/../.."
exec "$(dirname "$0")/rombundler"