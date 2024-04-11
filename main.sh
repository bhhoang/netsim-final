#!/usr/bin/env bash
distance=101.000000
numpackets=10
intervals=0.500000

for i in {2..30}; do
    echo "Running the experiment with $i nodes"
    ./ns3 run scratch/final.cc -- --numNodes=$i
done