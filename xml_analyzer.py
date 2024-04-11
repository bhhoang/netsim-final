#!/usr/bin/env python3

import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt
import sys

def parse_xml(xml_file):
    tree = ET.parse(xml_file)
    return tree

def analyze(data):
    totalFlows = 0
    lostFlows = 0
    totalClients = 0
    lostClients = []
    totalRatio = 0
    totalDelay = 0
    for flow in data.findall("FlowStats/Flow"):
        totalFlows += 1
        flowId = flow.get("flowId") 

        # Find the flow with the same id in Ipv4FlowClassifier
        for ipv4flow in data.findall("Ipv4FlowClassifier/Flow"):
            if ipv4flow.get("flowId") == flowId:
                break
        # Get the source and destination address and port
        srcAdd = ipv4flow.get("sourceAddress")
        srcPrt = ipv4flow.get("sourcePort")
        desAdd = ipv4flow.get("destinationAddress")
        desPrt = ipv4flow.get("destinationPort")
        #print(f"FlowID: {flowId} (UDP {srcAdd}/{srcPrt} --> {desAdd}/{desPrt})")
        if srcPrt != "9":
            totalClients += 1
        # Get total transmitted and received bytes of the flow
        txBytes = float(flow.get("txBytes"))
        rxBytes = float(flow.get("rxBytes"))

        # Duration = (timeLastPacket - timeFirstPacket) * 10^-9
        txDuration = (float(flow.get('timeLastTxPacket')[:-2]) - float(flow.get('timeFirstTxPacket')[:-2]))*1e-9
        rxDuration = (float(flow.get('timeLastRxPacket')[:-2]) - float(flow.get('timeFirstRxPacket')[:-2]))*1e-9

        # Bit rate = sent or received bytes * 8 / duration
        txBitrate = None if txDuration == 0 else (txBytes*8/txDuration)*1e-3
        rxBitrate = None if rxDuration == 0 else (rxBytes*8/rxDuration)*1e-3
        #print("\tTX bitrate: " + (f"{txBitrate:.2f} kbit/s" if txBitrate else "None"))
        #print("\tRX bitrate: " + (f"{rxBitrate:.2f} kbit/s" if rxBitrate else "None"))

        # Get the number of transmitted and received packets
        txPackets = int(flow.get("txPackets"))
        rxPackets = int(flow.get("rxPackets"))
        #print(f"\tTX Packets: {txPackets}")
        #print(f"\tRX Packets: {rxPackets}")

        # Calculate the mean delay
        delaySum = float(flow.get("delaySum")[:-2])
        delayMean = None if rxPackets == 0 else delaySum / rxPackets * 1e-9
        #print("\tMean Delay: " + (f"{delayMean * 1e3:.2f} ms" if delayMean else "None"))
        if delayMean:
            totalDelay += delayMean

        # Calculate the packet loss ratio
        lostPackets = int(flow.get("lostPackets"))
        if lostPackets != 0:
            lostFlows += 1
            lostClients.append(srcAdd)
        packetLossRatio = (txPackets - rxPackets) / txPackets * 100
        #print(f"\tPacket Loss Ratio: {packetLossRatio:.2f} %")
        # Return lost clients
        totalRatio += packetLossRatio
        
    return len(lostClients), float(totalRatio/totalFlows), float(totalDelay/totalFlows)
        #print(lostClients)

def plotter(list_criteria: list, ite: int, title: str, xlabel: str, ylabel: str, filename: str) -> None:
    plt.scatter(range(2,ite), list_criteria)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.savefig(filename)
    plt.close()

if __name__ == '__main__':
    # if len(sys.argv) != 2:
    #     #print("Usage: %s <xml_file>" % sys.argv[0])
    #     sys.exit(1)
    # data = parse_xml(sys.argv[1])
    distance = '101.000000'
    interval = '0.500000'
    transmit_pk = '10'
    it_nums = 31
    list_lostClients = []
    list_packetLossRatio = []
    list_avgDelay = []
    for i in range(2,it_nums):
        tree = parse_xml("FinalFlow-" + str(i) + "-" + distance + "-" + transmit_pk + '-' + interval + ".xml")
        lostClients, packetLossRatio, avgDelay = analyze(tree)
        #print("number of Nodes: " + str(i))
        #print(lostClients)
        list_lostClients.append(lostClients)
        list_packetLossRatio.append(packetLossRatio)
        list_avgDelay.append(avgDelay)
    plotter(list_lostClients, it_nums, "Lost Clients with respect to number of Nodes", "Number of Nodes", "Lost Clients", "LostClients.png")
    plotter(list_packetLossRatio, it_nums, "Packet Loss Ratio with respect to number of Nodes", "Number of Nodes", "Packet Loss Ratio (%)", "PacketLossRatio.png")
    plotter(list_avgDelay, it_nums, "Average Delay with respect to number of Nodes", "Number of Nodes", "Average Delay(s)", "AverageDelay.png")