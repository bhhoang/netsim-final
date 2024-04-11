// Disable RTS/CTS
// Increase number of nodes in communication range 2,15,30
// Since using grid => use constant position mobility model
// Configure wifi using YansWifiPhyHelper and YansWifiChannelHelper
// Tracing
// Run
// Destroy

#include <iostream>

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE("NetSimFinal");
 

void
ReceivePacket(Ptr<Socket> socket)
{
    while (socket->Recv())
    {
        NS_LOG_UNCOND("Received one packet!");
        NS_LOG_UNCOND("At time " << Simulator::Now().GetSeconds() << "s");
    }
}
 
static void
GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
{
    if (pktCount > 0) // Send if there are packets to send
    {
        socket->Send(Create<Packet>(pktSize));
        Simulator::Schedule(pktInterval,
                            &GenerateTraffic,
                            socket,
                            pktSize,
                            pktCount - 1,
                            pktInterval); // Send the next packet after pktInterval seconds
    }
    else
    {
        socket->Close();
    }
}
 
int
main(int argc, char* argv[])
{
    std::string phyMode("DsssRate1Mbps");
    double distance = 101;      // m
    uint32_t packetSize = 1024; // bytes
    uint32_t numPackets = 10;
    uint32_t numNodes = 30;
    uint32_t sinkNode = 0;
    //uint32_t sourceNode = 2; //
    double interval = 0.5; // seconds
    bool verbose = false;
    bool tracing = true;
 
    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("distance", "distance (m)", distance);
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue("numPackets", "number of packets generated", numPackets);
    cmd.AddValue("interval", "interval (seconds) between packets", interval);
    cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);
    cmd.AddValue("tracing", "turn on ascii and pcap tracing", tracing);
    cmd.AddValue("numNodes", "number of nodes", numNodes);
    cmd.AddValue("sinkNode", "Receiver node number", sinkNode);
    // cmd.AddValue("sourceNode", "Sender node number", sourceNode);
    cmd.Parse(argc, argv);

    // Disable RTS/CTS for frames below 2000 bytes
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2000"));
    // => If packet size is below 2000 bytes, do not use RTS/CTS

    // Convert to time object
    Time interPacketInterval = Seconds(interval);
 
    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
 
    NodeContainer c;
    c.Create(numNodes);
 
    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents(); // Turn on all Wifi logging
    }
 
    YansWifiPhyHelper wifiPhy;
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set("RxGain", DoubleValue(-10));
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
 
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());
 
    // Add an upper mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);
 
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(distance),
                                  "DeltaY",
                                  DoubleValue(distance),
                                  "GridWidth",
                                  UintegerValue(6),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);
 
    // Enable OLSR
    OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;
 
    Ipv4ListRoutingHelper list;
    list.Add(staticRouting, 0);
    list.Add(olsr, 10);
 
    // Internet Stack Install
    InternetStackHelper internet;
    internet.SetRoutingHelper(list); // has effect on the next Install ()
    internet.Install(c);
 
    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);
 
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(sinkNode), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));
 
    // Loop through all nodes to set up the sources, except the sinkNode
    for (uint32_t nodeId = 0; nodeId < c.GetN(); ++nodeId)
    {
        if (nodeId != sinkNode) // Check so we don't set the sinkNode as a source
        {
            Ptr<Socket> source = Socket::CreateSocket(c.Get(nodeId), tid);
            InetSocketAddress remote = InetSocketAddress(i.GetAddress(sinkNode, 0), 80);
            source->Connect(remote);

            // Schedule the traffic generation from this node
            Simulator::Schedule(Seconds(30.0) + Seconds(nodeId), // Stagger start times slightly
                                &GenerateTraffic,
                                source,
                                packetSize,
                                numPackets,
                                interPacketInterval); 
        }
    }

    if (tracing)
    {
        AsciiTraceHelper ascii;
        wifiPhy.EnableAsciiAll(ascii.CreateFileStream("final.tr"));
        wifiPhy.EnablePcap("final", devices);
        // Trace routing tables
        Ptr<OutputStreamWrapper> routingStream =
            Create<OutputStreamWrapper>("final.routes", std::ios::out);
        Ipv4RoutingHelper::PrintRoutingTableAllEvery(Seconds(2), routingStream);
        Ptr<OutputStreamWrapper> neighborStream =
            Create<OutputStreamWrapper>("final.neighbors", std::ios::out);
        Ipv4RoutingHelper::PrintNeighborCacheAllEvery(Seconds(2), neighborStream);

    }
 
    // Flow monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    double totalDeliveryRatio = 0.0;
    double totalAverageDelay = 0.0;
    uint32_t totalPacketsTx = 0;
    uint32_t totalPacketsRx = 0;
    uint32_t totalFlows = 0;


    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Delivery Ratio: " << i->second.rxPackets * 1.0 / i->second.txPackets
                  << "\n";
        std::cout << "  Average Delay: " << i->second.delaySum.GetSeconds() / i->second.rxPackets
                  << "\n";

        totalDeliveryRatio += i->second.rxPackets * 1.0 / i->second.txPackets;
        totalFlows++;
        totalPacketsTx += i->second.txPackets;
        totalPacketsRx += i->second.rxPackets;
        if (!std::isnan(i->second.delaySum.GetSeconds() / i->second.rxPackets)){
            totalAverageDelay += i->second.delaySum.GetSeconds() / i->second.rxPackets;
        }
    }
    std::cout << "\n" << std::endl;
    std::cout << "--- Flow Monitor Results ---\n"
                << std::endl;
    std::cout << "============================\n" << std::endl;
    std::cout << "Total received packets: " << totalPacketsRx << "\n";
    std::cout << "Total flows: " << totalFlows << "\n";
    std::cout << "Average delivery ratio: " << totalDeliveryRatio / totalFlows << "\n";
    std::cout << "Average delay: " << totalAverageDelay / totalFlows << "\n";
    std::cout << "Total transmitted packets: " << totalPacketsTx << "\n";

    // Create filename with format "FinalFlow-<numNodes>-<distance>-<numPackets>-<interval>.xml"
    std::string filename = "FinalFlow-" + std::to_string(numNodes) + "-" + std::to_string(distance) + "-" + std::to_string(numPackets) + "-" + std::to_string(interval) + ".xml";
    monitor->SerializeToXmlFile(filename, true, true);
    // View the topology in NetAnim
    AnimationInterface anim ("FinalTopology.xml");

    // anim.SetConstantPosition (c.Get(0), 0, distance);
    // anim.SetConstantPosition (c.Get(1), distance, 5);
    Simulator::Destroy();
 
    return 0;
}
