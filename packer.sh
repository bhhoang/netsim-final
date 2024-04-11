#!/usr/bin/env bash
distance=101.000000
numpackets=10
intervals=0.500000
# Tar flow files
# Tar all the file with format FinalFlow-i-<distance>-<numpackets>-<intervals>.xml to FinalFlow.tar.gz
for i in {2..30}; do
    tar -zcvf FinalFlow.tar.gz $(ls -l|awk '/FinalFlow-'$i'-'$distance'-'$numpackets'-'$intervals'.xml/{print $NF}')
done

# Tar the .pcap files
tar -zcvf FinalPcap.tar.gz *.pcap

# Tar the .png files
tar -zcvf FinalPng.tar.gz *.png