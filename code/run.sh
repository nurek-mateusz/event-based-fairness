#!/usr/bin/env bash
set -ex

# This is the master script for the capsule. When you click "Reproducible Run", the code in this file will execute.

# inform about starting the scripts
echo "### BEGIN OF EXECUTION ###"

# create a directory for results (for local runs)
mkdir -p ../results

# compile the code
make

# run simulation
./opinion-dynamics exponential 0.3 0.2 0.00563145983483561 0.0 3600 ../data/events.csv ../data/surveys.csv ../results/marijuana-0.0.csv marijuana
# inform about the end of analyses
echo "### END OF EXECUTION ###"
