#!/usr/bin/env bash
set -ex

# This is the master script for the capsule. When you click "Reproducible Run", the code in this file will execute.

# inform about starting the scripts
echo "### BEGIN OF EXECUTION ###"

# create a directory for results (for local runs)
mkdir -p ../results

# List of topics
topics=("euthanasia" "fssocsec" "fswelfare" "jobguar" "marijuana" "toomucheqrights")

# run simulation in a loop for each topic and each X
for topic in "${topics[@]}"
do
    for X in $(seq 0.1 0.1 0.9 | tr ',' '.')
    do
        for iteration in {1..10}
        do
            make
            ./opinion-dynamics exponential 0.3 0.2 0.00563145983483561 ${X} 3600 ../data/events.csv ../data/surveys.csv ../results/${topic}-${X}-iter${iteration}.csv ${topic}
        done
    done
done

# inform about the end of analyses
echo "### END OF EXECUTION ###"
