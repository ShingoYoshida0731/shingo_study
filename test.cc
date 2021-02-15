#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ipv4-header.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"

#include "ns3/shingo-helper.h"
#include "ns3/applications-module.h"

#include "ns3/netanim-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("test");

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      NS_LOG_UNCOND ("そーーーしーーーーん");
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

int main(int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double rss = -80;  // -dBm
  double interval = 0.5; // seconds
  uint32_t packetSize = 1012; // bytes
  uint32_t numPackets = 10;

 //LogComponentEnable("ShingoProtocol",LOG_LEVEL_LOGIC);
 //LogComponentEnable("AodvRoutingProtocol",LOG_LEVEL_ALL);

  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer a;
  a.Create (1);

  //Node設定
  NodeContainer s;
  s.Create (575);

  //NodeContainer si;
  //si.Create(1);

  //wifi設定
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  wifiPhy.Set ("RxGain", DoubleValue (0) );

  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));

  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(100));
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  //Nodeにwifiを適用
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices1 = wifi.Install (wifiPhy, wifiMac, a);
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, s);

  //NetDeviceContainer devices2 = wifi.Install (wifiPhy, wifiMac, si);
  
  //Node(Olsr)の位置
/*
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0),
                                 "MinY", DoubleValue (0),
                                 "DeltaX", DoubleValue (70.0),
                                 "DeltaY", DoubleValue (70.0),
                                 "GridWidth", UintegerValue (7),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (s);
*/


  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (300, 300.0, 0.0));
  //positionAlloc->Add (Vector (500, 500, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel", 
//				"Bounds", RectangleValue (Rectangle (90, 100, 90, 170)),
//				"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=2]"),
//				"Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  
  mobility.Install (a);

//   MobilityHelper mobility;
   mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                     "X", StringValue ("300.0"),
                                     "Y", StringValue ("300.0"),
                                     "Rho", StringValue ("ns3::UniformRandomVariable[Min=11|Max=310]"));
//   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel", 
				"Bounds", RectangleValue (Rectangle (0, 500, 0, 500)),
				"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.3]"),
				"Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.3]"));
  
   mobility.Install (s);


  //protocol設定
  ShingoHelper shingo;
  AodvHelper aodv;
  DsdvHelper dsdv;
  OlsrHelper olsr;

  InternetStackHelper internet;
  //internet.SetRoutingHelper(aodv);
  internet.SetRoutingHelper(shingo);
  internet.Install(s);
  internet.Install(a);
  //internet.Install(si);

//protocolのトレースファイル作成
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routingtable-shingo.tr", std::ios::out);
  aodv.PrintRoutingTableAllEvery(Seconds(2), routingStream);

/*
  //protocol設定
  ShingoHelper shingo;
  //AodvHelper aodv;
  //DsdvHelper dsdv;

  InternetStackHelper internet;
  internet.SetRoutingHelper(shingo);
  internet.Install(s);
  internet.Install(a);
  //internet.Install(si);

//protocolのトレースファイル作成
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routingtable-shingo.tr", std::ios::out);
  shingo.PrintRoutingTableAllEvery(Seconds(2), routingStream);
*/
  //IPaddress設定
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.0.0.0", "255.255.252.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices1);
  i = ipv4.Assign (devices);
//i = ipv4.Assign (devices2);

  //送信Node設定
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source = Socket::CreateSocket (s.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  //送信Node設定
  //TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source1 = Socket::CreateSocket (s.Get (1), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source1->SetAllowBroadcast (true);
  source1->Connect (remote);

  //送信Node設定
  //TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source2 = Socket::CreateSocket (s.Get (2), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source2->SetAllowBroadcast (true);
  source2->Connect (remote);

  //送信Node設定
  //TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source3 = Socket::CreateSocket (s.Get (3), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source3->SetAllowBroadcast (true);
  source3->Connect (remote);

  //送信Node設定
  //TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source4 = Socket::CreateSocket (s.Get (4), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source4->SetAllowBroadcast (true);
  source4->Connect (remote);

  //送信Node設定
  //TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source5 = Socket::CreateSocket (s.Get (5), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.0.0.1"), 80);
  source5->SetAllowBroadcast (true);
  source5->Connect (remote);

  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (7), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);
/*  Simulator::ScheduleWithContext (source1->GetNode ()->GetId (),
                                  Seconds (7.3), &GenerateTraffic,
                                  source1, packetSize, numPackets, interPacketInterval);
*/  Simulator::ScheduleWithContext (source2->GetNode ()->GetId (),
                                  Seconds (7.6), &GenerateTraffic,
                                  source2, packetSize, numPackets, interPacketInterval);
/*  Simulator::ScheduleWithContext (source3->GetNode ()->GetId (),
                                  Seconds (7.9), &GenerateTraffic,
                                  source3, packetSize, numPackets, interPacketInterval);
*/  Simulator::ScheduleWithContext (source4->GetNode ()->GetId (),
                                  Seconds (8.2), &GenerateTraffic,
                                  source4, packetSize, numPackets, interPacketInterval);
/*  Simulator::ScheduleWithContext (source5->GetNode ()->GetId (),
                                  Seconds (8.5), &GenerateTraffic,
                                  source5, packetSize, numPackets, interPacketInterval);
*/
  TypeId tid3 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink3 = Socket::CreateSocket (a.Get (0), tid3);
  InetSocketAddress local3 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink3->Bind (local3);
  recvSink3->SetRecvCallback (MakeCallback (&ReceivePacket));




  //トレースファイル作成
  //wifiPhy.EnableAsciiAll (as.CreateFileStream("test.tr"));

  //pcapファイル
  wifiPhy.EnablePcapAll ("exp");
  
  //netanim設定
  std::string xf = "test.xml";
  AnimationInterface anim (xf);
  anim.EnablePacketMetadata (true);
  Simulator::Stop(Seconds(15));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

