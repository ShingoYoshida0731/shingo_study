/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

//#pragma once

#include "shingo.h"

#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include <algorithm>
#include <limits>
#include <iomanip>
#include <iostream>

NS_LOG_COMPONENT_DEFINE ("ShingoProtocol");

namespace ns3
{

namespace shingo
{

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for MYRTP control traffic
const uint32_t shingo::RoutingProtocol::MY_PORT = 5555;

//------------------------------------------


//IARPの実装に使うtag
struct DeferredRouteOutputTag : public Tag
{
  /// Positive if output device is fixed in RouteOutput
  int32_t oif;

  /**
   * Constructor
   *
   * \param o outgoing interface (OIF)
   */
  DeferredRouteOutputTag (int32_t o = -1)
    : Tag (),
      oif (o)
  {
  }

  static TypeId
  GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::shingo::DeferredRouteOutputTag")
      .SetParent<Tag> ()
      .SetGroupName ("Shingo")
      .AddConstructor<DeferredRouteOutputTag> ()
    ;
    return tid;
  }

  TypeId
  GetInstanceTypeId () const
  {
    return GetTypeId ();
  }

  uint32_t
  GetSerializedSize () const
  {
    return sizeof(int32_t);
  }

  void
  Serialize (TagBuffer i) const
  {
    i.WriteU32 (oif);
  }

  void
  Deserialize (TagBuffer i)
  {
    oif = i.ReadU32 ();
  }

  void
  Print (std::ostream &os) const
  {
    os << "DeferredRouteOutputTag: output interface = " << oif;
  }
};


TypeId
RoutingProtocol::GetTypeId (void)
{
 static TypeId tid = TypeId ("ns3::shingo::RoutingProtocol")
     .SetParent<Ipv4RoutingProtocol> ()
     .SetGroupName("Shingo")
     .AddConstructor<RoutingProtocol> ()
/**********IARP**************/
     .AddAttribute ("PeriodicUpdateInterval","Periodic interval between exchange of full routing tables among nodes. ",
                   TimeValue (Seconds (15)),
                   MakeTimeAccessor (&RoutingProtocol::m_periodicUpdateInterval),
                   MakeTimeChecker ())
    .AddAttribute ("SettlingTime", "Minimum time an update is to be stored in adv table before sending out"
                      "in case of change in metric (in seconds)",
                   TimeValue (Seconds (5)),
                   MakeTimeAccessor (&RoutingProtocol::m_settlingTime),
                   MakeTimeChecker ())
    .AddAttribute ("MaxQueueLen", "Maximum number of packets that we allow a routing protocol to buffer.",
                   UintegerValue (500 /*assuming maximum nodes in simulation is 100*/),
                   MakeUintegerAccessor (&RoutingProtocol::m_maxQueueLen),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxQueuedPacketsPerDst", "Maximum number of packets that we allow per destination to buffer.",
                   UintegerValue (5),
                   MakeUintegerAccessor (&RoutingProtocol::m_maxQueuedPacketsPerDst),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxQueueTime","Maximum time packets can be queued (in seconds)",
                   TimeValue (Seconds (30)),
                   MakeTimeAccessor (&RoutingProtocol::m_maxQueueTime),
                   MakeTimeChecker ())
    .AddAttribute ("EnableBuffering","Enables buffering of data packets if no route to destination is available",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RoutingProtocol::SetEnableBufferFlag,
                                        &RoutingProtocol::GetEnableBufferFlag),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableWST","Enables Weighted Settling Time for the updates before advertising",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RoutingProtocol::SetWSTFlag,
                                        &RoutingProtocol::GetWSTFlag),
                   MakeBooleanChecker ())
    .AddAttribute ("Holdtimes","Times the forwarding Interval to purge the route.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&RoutingProtocol::Holdtimes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("WeightedFactor","WeightedFactor for the settling time if Weighted Settling Time is enabled",
                   DoubleValue (0.875),
                   MakeDoubleAccessor (&RoutingProtocol::m_weightedFactor),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("EnableRouteAggregation","Enables Weighted Settling Time for the updates before advertising",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RoutingProtocol::SetEnableRAFlag,
                                        &RoutingProtocol::GetEnableRAFlag),
                   MakeBooleanChecker ())
    .AddAttribute ("RouteAggregationTime","Time to aggregate updates before sending them out (in seconds)",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::m_routeAggregationTime),
                   MakeTimeChecker ());
/**********IARP*****************/
 return tid;
}

void
RoutingProtocol::SetEnableBufferFlag (bool f)
{
  EnableBuffering = f;
}
bool
RoutingProtocol::GetEnableBufferFlag () const
{
  return EnableBuffering;
}
void
RoutingProtocol::SetWSTFlag (bool f)
{
  EnableWST = f;
}
bool
RoutingProtocol::GetWSTFlag () const
{
  return EnableWST;
}
void
RoutingProtocol::SetEnableRAFlag (bool f)
{
  EnableRouteAggregation = f;
}
bool
RoutingProtocol::GetEnableRAFlag () const
{
  return EnableRouteAggregation;
}

RoutingProtocol::RoutingProtocol ()
  : m_routingTable (),
    m_advRoutingTable (),
    m_queue (),
    m_nb (Seconds(1)),
    m_rreqRetries (2),
    m_rreqRateLimit (10),
    m_rreqCount (0),
    m_seqNo (0),
    m_requestId (0),
    m_rreqIdCache (m_pathDiscoveryTime),
    m_ttlStart (1),
    m_ttlIncrement (2),
    m_ttlThreshold (7),
    m_netDiameter (35),
    m_timeoutBuffer (2),
    m_nodeTraversalTime (MilliSeconds (40)),
    m_netTraversalTime (Time ((2 * m_netDiameter) * m_nodeTraversalTime)),
    m_pathDiscoveryTime ( Time (2 * 0.24)), //m_netTraversalTime
    m_myRouteTimeout (Time (2 * std::max (m_pathDiscoveryTime, Seconds (3)))),
    m_nextHopWait (m_nodeTraversalTime + MilliSeconds (10)),
    m_blackListTimeout (Time (m_rreqRetries * m_netTraversalTime)),
    m_destinationOnly (false),
    m_gratuitousReply (true),
    m_queue2 (64, Seconds(30)),
    m_periodicUpdateTimer (Timer::CANCEL_ON_DESTROY)
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}

RoutingProtocol::~RoutingProtocol ()
{
}


void
RoutingProtocol::DoDispose ()
{  
 //終了時のオブジェクトの廃棄などの後処理を行う
  m_ipv4 = 0;
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter = m_socketAddresses.begin (); iter
       != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
 //ルーティングテーブルのエントリーを出力する処理を行う。
 //実際にはルーティングテーブルクラスのPrintメソッドにより処理する。

  *stream->GetStream () << "Node: " << m_ipv4->GetObject<Node> ()->GetId ()
                        << ", Time: " << Now ().As (unit)
                        << ", Local time: " << GetObject<Node> ()->GetLocalTime ().As (unit)
                        << ", IARP Routing table" << std::endl;
  m_routingTable.Print (stream);
  m_routingTable2.Print (stream);
  *stream->GetStream () << std::endl;
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
        NS_LOG_FUNCTION (this << stream);
        
        //乱数系列の割り当て処理
        m_uniformRandomVariable->SetStream (stream);
        return 1;
}

void
RoutingProtocol::Start ()
{
 //ルーティングプロトコルの開始処理
/***********IARP******************/
  m_queue.SetMaxPacketsPerDst (m_maxQueuedPacketsPerDst);
  m_queue.SetMaxQueueLen (m_maxQueueLen);
  m_queue.SetQueueTimeout (m_maxQueueTime);
  m_routingTable.Setholddowntime (Time (Holdtimes * m_periodicUpdateInterval));
  m_advRoutingTable.Setholddowntime (Time (Holdtimes * m_periodicUpdateInterval));
  m_scb = MakeCallback (&RoutingProtocol::Send,this);
  m_ecb = MakeCallback (&RoutingProtocol::Drop,this);
  m_periodicUpdateTimer.SetFunction (&RoutingProtocol::SendPeriodicUpdate,this);
  m_periodicUpdateTimer.Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,1000)));
/*********************************/
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, 
 Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
 NS_LOG_FUNCTION (this << header << " " << (oif ? oif->GetIfIndex () : 0));
 //パケットに転送経路を決定するための処理を行う
/**********IARP***********************/
  if (!p)
    {
      return LoopbackRoute (header,oif);
    }
  if (m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No IARP interfaces");
      Ptr<Ipv4Route> route;
      return route;
    }
  std::map<Ipv4Address, RoutingTableEntry> removedAddresses;
  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route;
  Ipv4Address dst = header.GetDestination ();
  NS_LOG_DEBUG ("Packet Size: " << p->GetSize ()
                                << ", Packet id: " << p->GetUid () << ", Destination address in Packet: " << dst);
  RoutingTableEntry rt;
  m_routingTable.Purge (removedAddresses);
  for (std::map<Ipv4Address, RoutingTableEntry>::iterator rmItr = removedAddresses.begin ();
       rmItr != removedAddresses.end (); ++rmItr)
    {
      rmItr->second.SetEntriesChanged (true);
      rmItr->second.SetSeqNo (rmItr->second.GetSeqNo () + 1);
      m_advRoutingTable.AddRoute (rmItr->second);
    }
  if (!removedAddresses.empty ())
    {
      Simulator::Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,1000)),&RoutingProtocol::SendTriggeredUpdate,this);
    }
  if (m_routingTable.LookupRoute (dst,rt))
    {
      if (EnableBuffering)
        {
          LookForQueuedPackets ();
        }
      if (rt.GetHop () == 1)
        {
          route = rt.GetRoute ();
          NS_ASSERT (route != 0);
          NS_LOG_DEBUG ("A route exists from " << route->GetSource ()
                                               << " to neighboring destination "
                                               << route->GetDestination ());
          if (oif != 0 && route->GetOutputDevice () != oif)
            {
              NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
              sockerr = Socket::ERROR_NOROUTETOHOST;
              return Ptr<Ipv4Route> ();
            }
          return route;
        }
      else
        {
          RoutingTableEntry newrt;
          if (m_routingTable.LookupRoute (rt.GetNextHop (),newrt))
            {
              route = newrt.GetRoute ();
              NS_ASSERT (route != 0);
              NS_LOG_DEBUG ("A route exists from " << route->GetSource ()
                                                   << " to destination " << dst << " via "
                                                   << rt.GetNextHop ());
              if (oif != 0 && route->GetOutputDevice () != oif)
                {
                  NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
                  sockerr = Socket::ERROR_NOROUTETOHOST;
                  return Ptr<Ipv4Route> ();
                }
              return route;
            }
        }
    }

//  if (EnableBuffering)
//    {
      uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice (oif) : -1);
      DeferredRouteOutputTag tag (iif);
      if (!p->PeekPacketTag (tag))
        {
          p->AddPacketTag (tag);
        }
//    }
  return LoopbackRoute (header,oif);
/*************************************/
}

void
RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p,
                                      const Ipv4Header & header,
                                      UnicastForwardCallback ucb,
                                      ErrorCallback ecb)
{
//printf("DefferdRouteOutput \n");
  NS_LOG_FUNCTION (this << p << header);
  NS_ASSERT (p != 0 && p != Ptr<Packet> ());
  QueueEntry newEntry (p,header,ucb,ecb);
  bool result = m_queue2.Enqueue (newEntry);
  if (result)
    {
      NS_LOG_LOGIC ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());
      RoutingTableEntry2 rt;
      bool result = m_routingTable2.LookupRoute (header.GetDestination (), rt);
      if (!result || ((rt.GetFlag () != IN_SEARCH) && result))
        {
          NS_LOG_LOGIC ("Send new RREQ for outbound packet to " << header.GetDestination ());
          SendRequest (header.GetDestination ());
        }
    }
}

bool
RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
      Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
      MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
//printf("RouteInput \n");
  NS_LOG_FUNCTION (m_mainAddress << " received packet " << p->GetUid ()
                                 << " from " << header.GetSource ()
                                 << " on interface " << idev->GetAddress ()
                                 << " to destination " << header.GetDestination ());
  if (m_socketAddresses.empty ())
    {
      NS_LOG_DEBUG ("No IARP interfaces");
      return false;
    }
  NS_ASSERT (m_ipv4 != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  int32_t iif = m_ipv4->GetInterfaceForDevice (idev);

  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();

  // ISRP is not a multicast routing protocol
  if (dst.IsMulticast ())
    {
      return false;
    }

  // Deferred route request
  if (EnableBuffering == true && idev == m_lo)
    {
      DeferredRouteOutputTag tag;
      if (p->PeekPacketTag (tag))
        {
          DeferredRouteOutput (p,header,ucb,ecb);
          return true;
        }
    }
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (origin == iface.GetLocal ())
        {
          return true;
        }
    }
  // LOCAL DELIVARY TO AIRP INTERFACES
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
        {
          if (dst == iface.GetBroadcast () || dst.IsBroadcast ())
            {
              Ptr<Packet> packet = p->Copy ();
              if (lcb.IsNull () == false)
                {
                  NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetLocal ());
                  lcb (p, header, iif);
                  // Fall through to additional processing
                }
              else
                {
                  NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
                  ecb (p, header, Socket::ERROR_NOROUTETOHOST);
                }
              if (header.GetTtl () > 1)
                {
                  NS_LOG_LOGIC ("Forward broadcast. TTL " << (uint16_t) header.GetTtl ());
                  RoutingTableEntry toBroadcast;
                  if (m_routingTable.LookupRoute (dst,toBroadcast,true))
                    {
                      Ptr<Ipv4Route> route = toBroadcast.GetRoute ();
                      ucb (route,packet,header);
                    }
                  else
                    {
                      NS_LOG_DEBUG ("No route to forward. Drop packet " << p->GetUid ());
                    }
                }
              return true;
            }
        }
    }

  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      if (lcb.IsNull () == false)
        {
          NS_LOG_LOGIC ("Unicast local delivery to " << dst);
          lcb (p, header, iif);
        }
      else
        {
          NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
          ecb (p, header, Socket::ERROR_NOROUTETOHOST);
        }
      return true;
    }

  // Check if input device supports IP forwarding
  if (m_ipv4->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return true;
    }

  RoutingTableEntry toDst;
  if (m_routingTable.LookupRoute (dst,toDst))
    {
      RoutingTableEntry ne;
      if (m_routingTable.LookupRoute (toDst.GetNextHop (),ne))
        {
          Ptr<Ipv4Route> route = ne.GetRoute ();
          NS_LOG_LOGIC (m_mainAddress << " is forwarding packet " << p->GetUid ()
                                      << " to " << dst
                                      << " from " << header.GetSource ()
                                      << " via nexthop neighbor " << toDst.GetNextHop ());
          ucb (route,p,header);
          return true;
        }
    }
/*  NS_LOG_LOGIC ("Drop packet " << p->GetUid ()
                               << " as there is no route to forward it.");
  return false;
*/
  return Forwarding (p, header, ucb, ecb);
}

bool
RoutingProtocol::Forwarding (Ptr<const Packet> p, const Ipv4Header & header,
                             UnicastForwardCallback ucb, ErrorCallback ecb)
{
  //printf("Forwarding\n");
  NS_LOG_FUNCTION (this);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();
  m_routingTable2.Purge ();
  RoutingTableEntry2 toDst;
  if (m_routingTable2.LookupRoute (dst, toDst))
    {
      if (toDst.GetFlag () == VALID)
        {
          Ptr<Ipv4Route> route = toDst.GetRoute ();
          NS_LOG_LOGIC (route->GetSource () << " forwarding to " << dst << " from " << origin << " packet " << p->GetUid ());

          /*
           *  Each time a route is used to forward a data packet, its Active Route
           *  Lifetime field of the source, destination and the next hop on the
           *  path to the destination is updated to be no less than the current
           *  time plus ActiveRouteTimeout.
           */
          UpdateRouteLifeTime (origin, m_activeRouteTimeout);
          UpdateRouteLifeTime (dst, m_activeRouteTimeout);
          UpdateRouteLifeTime (route->GetGateway (), m_activeRouteTimeout);
          /*
           *  Since the route between each originator and destination pair is expected to be symmetric, the
           *  Active Route Lifetime for the previous hop, along the reverse path back to the IP source, is also updated
           *  to be no less than the current time plus ActiveRouteTimeout
           */
          RoutingTableEntry2 toOrigin;
          m_routingTable2.LookupRoute (origin, toOrigin);
          UpdateRouteLifeTime (toOrigin.GetNextHop (), m_activeRouteTimeout);

          m_nb.Update (route->GetGateway (), m_activeRouteTimeout);
          m_nb.Update (toOrigin.GetNextHop (), m_activeRouteTimeout);

          ucb (route, p, header);
          return true;
        }
      else
        {
          if (toDst.GetValidSeqNo ())
            {
              //SendRerrWhenNoRouteToForward (dst, toDst.GetSeqNo (), origin);
              NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it.");
              return false;
            }
        }
    }
  NS_LOG_LOGIC ("route not found to " << dst << ". Send RERR message.");
  NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it.");
  //SendRerrWhenNoRouteToForward (dst, 0, origin);
  return false;
}


bool
RoutingProtocol::UpdateRouteLifeTime (Ipv4Address addr, Time lifetime)
{
//printf("UpdateROuteLifeTime \n");
  NS_LOG_FUNCTION (this << addr << lifetime);
  RoutingTableEntry2 rt;
  if (m_routingTable2.LookupRoute (addr, rt))
    {
      if (rt.GetFlag () == VALID)
        {
          NS_LOG_DEBUG ("Updating VALID route");
          rt.SetRreqCnt (0);
          rt.SetLifeTime (std::max (lifetime, rt.GetLifeTime ()));
          m_routingTable2.Update (rt);
          return true;
        }
    }
  return false;
}


Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif) const
{
//printf("LoopbackRoute \n");
  NS_ASSERT (m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  // rt->SetSource (hdr.GetSource ());
  //
  // Source address selection here is tricky.  The loopback route is
  // returned when IARP does not have a route; this causes the packet
  // to be looped back and handled (cached) in RouteInput() method
  // while a route is found. However, connection-oriented protocols
  // like TCP need to create an endpoint four-tuple (src, src port,
  // dst, dst port) and create a pseudo-header for checksumming.  So,
  // IARP needs to guess correctly what the eventual source address
  // will be.
  //
  // For single interface, single address nodes, this is not a problem.
  // When there are possibly multiple outgoing interfaces, the policy
  // implemented here is to pick the first available IARP interface.
  // If RouteOutput() caller specified an outgoing interface, that
  // further constrains the selection of source address
  //
  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
  if (oif)
    {
      // Iterate to find an address on the oif device
      for (j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = m_ipv4->GetInterfaceForAddress (addr);
          if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else
    {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid IARP source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (m_lo);
  return rt;
}


void
RoutingProtocol::RecvShingo (Ptr<Socket> socket)
{
  //printf("RecvShingo\n");
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();
/*
  if (m_socketAddresses.find (socket) != m_socketAddresses.end ())
    {//printf("1, ");
      receiver = m_socketAddresses[socket].GetLocal ();
    }
  else if (m_socketSubnetBroadcastAddresses.find (socket) != m_socketSubnetBroadcastAddresses.end ())
    {//printf("2, ");
      receiver = m_socketSubnetBroadcastAddresses[socket].GetLocal ();
    }
  else
    {
      NS_ASSERT_MSG (false, "Received a packet from an unknown socket");
    }
*/
  NS_LOG_DEBUG ("SHIGNO node " << this << " received a SHINGO packet from " << sender << " to " << receiver);
/*
  UpdateRouteToNeighbor (sender, receiver);
  TypeHeader tHeader (AODVTYPE_RREQ);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
*/
/*
printf("%d\n",packet->GetSize());
  switch (1)
    {
    case SHINGO_IARP:
      {
        RecvIarp (packet, receiver, sender);
        break;
      }
    }
*/

if(packet->GetSize() % 12 == 0)
  {
   RecvIarp (packet, receiver, sender);
  }
  else if (packet->GetSize() % 23 == 0)
        {
         RecvRequest (packet, receiver, sender);
        }
  else if (packet->GetSize() % 19 == 0)
        {
         RecvReply (packet, receiver, sender);
        }
  else //if (packet->GetSize() % 1 == 0)
        {
         RecvReplyAck (sender);
        }

}


void
RoutingProtocol::RecvIarp (Ptr<Packet> packet, Ipv4Address receiver, Ipv4Address sender)
{
/*
  Address sourceAddress;
  Ptr<Packet> advpacket = Create<Packet> ();
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();
*/
//printf("RecvIarp \n");
Ptr<Packet> advpacket = Create<Packet> ();
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
  uint32_t packetSize = packet->GetSize ();
  NS_LOG_FUNCTION (m_mainAddress << " received IARP packet of size: " << packetSize
                                 << " and packet id: " << packet->GetUid ());
  uint32_t count = 0;
  for (; packetSize > 0; packetSize = packetSize - 12)
    {
      count = 0;
      IarpHeader iarpHeader, tempIarpHeader;
      packet->RemoveHeader (iarpHeader);
      NS_LOG_DEBUG ("Processing new update for " << iarpHeader.GetDst ());
      /*Verifying if the packets sent by me were returned back to me. If yes, discarding them!*/
      for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
           != m_socketAddresses.end (); ++j)
        {
          Ipv4InterfaceAddress interface = j->second;
          if (iarpHeader.GetDst () == interface.GetLocal ())
            {
              if (iarpHeader.GetDstSeqno () % 2 == 1)
                {
                  NS_LOG_DEBUG ("Sent Iarp update back to the same Destination, "
                                "with infinite metric. Time left to send fwd update: "
                                << m_periodicUpdateTimer.GetDelayLeft ());
                  count++;
                }
              else
                {
                  NS_LOG_DEBUG ("Received update for my address. Discarding this.");
                  count++;
                }
            }
        }
      if (count > 0)
        {
          continue;
        }
      NS_LOG_DEBUG ("Received a IARP packet from "
                    << sender << " to " << receiver << ". Details are: Destination: " << iarpHeader.GetDst () << ", Seq No: "
                    << iarpHeader.GetDstSeqno () << ", HopCount: " << iarpHeader.GetHopCount ());
      RoutingTableEntry fwdTableEntry, advTableEntry;
      EventId event;
      bool permanentTableVerifier = m_routingTable.LookupRoute (iarpHeader.GetDst (),fwdTableEntry);
      if (permanentTableVerifier == false)
        {
          if (iarpHeader.GetDstSeqno () % 2 != 1)
            {
              NS_LOG_DEBUG ("Received New Route!");
              RoutingTableEntry newEntry (
                /*device=*/ dev, /*dst=*/
                iarpHeader.GetDst (), /*seqno=*/
                iarpHeader.GetDstSeqno (),
                /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                /*hops=*/ iarpHeader.GetHopCount (), /*next hop=*/
                sender, /*lifetime=*/
                Simulator::Now (), /*settlingTime*/
                m_settlingTime, /*entries changed*/
                true);
              newEntry.SetFlag (VALID);
              m_routingTable.AddRoute (newEntry);
              NS_LOG_DEBUG ("New Route added to both tables");
              m_advRoutingTable.AddRoute (newEntry);
            }
          else
            {
              // received update not present in main routing table and also with infinite metric
              NS_LOG_DEBUG ("Discarding this update as this route is not present in "
                            "main routing table and received with infinite metric");
            }
        }
      else
        {
          if (!m_advRoutingTable.LookupRoute (iarpHeader.GetDst (),advTableEntry))
            {
              RoutingTableEntry tr;
              std::map<Ipv4Address, RoutingTableEntry> allRoutes;
              m_advRoutingTable.GetListOfAllRoutes (allRoutes);
              for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
                {
                  NS_LOG_DEBUG ("ADV table routes are:" << i->second.GetDestination ());
                }
              // present in fwd table and not in advtable
              m_advRoutingTable.AddRoute (fwdTableEntry);
              m_advRoutingTable.LookupRoute (iarpHeader.GetDst (),advTableEntry);
            }
          if (iarpHeader.GetDstSeqno () % 2 != 1)
            {
              if (iarpHeader.GetDstSeqno () > advTableEntry.GetSeqNo ())
                {
                  // Received update with better seq number. Clear any old events that are running
                  if (m_advRoutingTable.ForceDeleteIpv4Event (iarpHeader.GetDst ()))
                    {
                      NS_LOG_DEBUG ("Canceling the timer to update route with better seq number");
                    }
                  // if its a changed metric *nomatter* where the update came from, wait  for WST
                  if (iarpHeader.GetHopCount () != advTableEntry.GetHop ())
                    {
                      advTableEntry.SetSeqNo (iarpHeader.GetDstSeqno ());
                      advTableEntry.SetLifeTime (Simulator::Now ());
                      advTableEntry.SetFlag (VALID);
                      advTableEntry.SetEntriesChanged (true);
                      advTableEntry.SetNextHop (sender);
                      advTableEntry.SetHop (iarpHeader.GetHopCount ());
                      NS_LOG_DEBUG ("Received update with better sequence number and changed metric.Waiting for WST");
                      Time tempSettlingtime = GetSettlingTime (iarpHeader.GetDst ());
                      advTableEntry.SetSettlingTime (tempSettlingtime);
                      NS_LOG_DEBUG ("Added Settling Time:" << tempSettlingtime.GetSeconds ()
                                                           << "s as there is no event running for this route");
                      event = Simulator::Schedule (tempSettlingtime,&RoutingProtocol::SendTriggeredUpdate,this);
                      m_advRoutingTable.AddIpv4Event (iarpHeader.GetDst (),event);
                      NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
                      // if received changed metric, use it but adv it only after wst
                      m_routingTable.Update (advTableEntry);
                      m_advRoutingTable.Update (advTableEntry);
                    }
                  else
                    {
                      // Received update with better seq number and same metric.
                      advTableEntry.SetSeqNo (iarpHeader.GetDstSeqno ());
                      advTableEntry.SetLifeTime (Simulator::Now ());
                      advTableEntry.SetFlag (VALID);
                      advTableEntry.SetEntriesChanged (true);
                      advTableEntry.SetNextHop (sender);
                      advTableEntry.SetHop (iarpHeader.GetHopCount ());
                      m_advRoutingTable.Update (advTableEntry);
                      NS_LOG_DEBUG ("Route with better sequence number and same metric received. Advertised without WST");
                    }
                }
              else if (iarpHeader.GetDstSeqno () == advTableEntry.GetSeqNo ())
                {
                  if (iarpHeader.GetHopCount () < advTableEntry.GetHop ())
                    {
                      /*Received update with same seq number and better hop count.
                       * As the metric is changed, we will have to wait for WST before sending out this update.
                       */
                      NS_LOG_DEBUG ("Canceling any existing timer to update route with same sequence number "
                                    "and better hop count");
                      m_advRoutingTable.ForceDeleteIpv4Event (iarpHeader.GetDst ());
                      advTableEntry.SetSeqNo (iarpHeader.GetDstSeqno ());
                      advTableEntry.SetLifeTime (Simulator::Now ());
                      advTableEntry.SetFlag (VALID);
                      advTableEntry.SetEntriesChanged (true);
                      advTableEntry.SetNextHop (sender);
                      advTableEntry.SetHop (iarpHeader.GetHopCount ());
                      Time tempSettlingtime = GetSettlingTime (iarpHeader.GetDst ());
                      advTableEntry.SetSettlingTime (tempSettlingtime);
                      NS_LOG_DEBUG ("Added Settling Time," << tempSettlingtime.GetSeconds ()
                                                           << " as there is no current event running for this route");
                      event = Simulator::Schedule (tempSettlingtime,&RoutingProtocol::SendTriggeredUpdate,this);
                      m_advRoutingTable.AddIpv4Event (iarpHeader.GetDst (),event);
                      NS_LOG_DEBUG ("EventCreated EventUID: " << event.GetUid ());
                      // if received changed metric, use it but adv it only after wst
                      m_routingTable.Update (advTableEntry);
                      m_advRoutingTable.Update (advTableEntry);
                    }
                  else
                    {
                      /*Received update with same seq number but with same or greater hop count.
                       * Discard that update.
                       */
                      if (!m_advRoutingTable.AnyRunningEvent (iarpHeader.GetDst ()))
                        {
                          /*update the timer only if nexthop address matches thus discarding
                           * updates to that destination from other nodes.
                           */
                          if (advTableEntry.GetNextHop () == sender)
                            {
                              advTableEntry.SetLifeTime (Simulator::Now ());
                              m_routingTable.Update (advTableEntry);
                            }
                          m_advRoutingTable.DeleteRoute (
                            iarpHeader.GetDst ());
                        }
                      NS_LOG_DEBUG ("Received update with same seq number and "
                                    "same/worst metric for, " << iarpHeader.GetDst () << ". Discarding the update.");
                    }
                }
              else
                {
                  // Received update with an old sequence number. Discard the update
                  if (!m_advRoutingTable.AnyRunningEvent (iarpHeader.GetDst ()))
                    {
                      m_advRoutingTable.DeleteRoute (iarpHeader.GetDst ());
                    }
                  NS_LOG_DEBUG (iarpHeader.GetDst () << " : Received update with old seq number. Discarding the update.");
                }
            }
          else
            {
              NS_LOG_DEBUG ("Route with infinite metric received for "
                            << iarpHeader.GetDst () << " from " << sender);
              // Delete route only if update was received from my nexthop neighbor
              if (sender == advTableEntry.GetNextHop ())
                {
                  NS_LOG_DEBUG ("Triggering an update for this unreachable route:");
                  std::map<Ipv4Address, RoutingTableEntry> dstsWithNextHopSrc;
                  m_routingTable.GetListOfDestinationWithNextHop (iarpHeader.GetDst (),dstsWithNextHopSrc);
                  m_routingTable.DeleteRoute (iarpHeader.GetDst ());
                  advTableEntry.SetSeqNo (iarpHeader.GetDstSeqno ());
                  advTableEntry.SetEntriesChanged (true);
                  m_advRoutingTable.Update (advTableEntry);
                  for (std::map<Ipv4Address, RoutingTableEntry>::iterator i = dstsWithNextHopSrc.begin (); i
                       != dstsWithNextHopSrc.end (); ++i)
                    {
                      i->second.SetSeqNo (i->second.GetSeqNo () + 1);
                      i->second.SetEntriesChanged (true);
                      m_advRoutingTable.AddRoute (i->second);
                      m_routingTable.DeleteRoute (i->second.GetDestination ());
                    }
                }
              else
                {
                  if (!m_advRoutingTable.AnyRunningEvent (iarpHeader.GetDst ()))
                    {
                      m_advRoutingTable.DeleteRoute (iarpHeader.GetDst ());
                    }
                  NS_LOG_DEBUG (iarpHeader.GetDst () <<
                                " : Discard this link break update as it was received from a different neighbor "
                                "and I can reach the destination");
                }
            }
        }
    }
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_advRoutingTable.GetListOfAllRoutes (allRoutes);
  if (EnableRouteAggregation && allRoutes.size () > 0)
    {
      Simulator::Schedule (m_routeAggregationTime,&RoutingProtocol::SendTriggeredUpdate,this);
    }
  else
    {
      Simulator::Schedule (MicroSeconds (m_uniformRandomVariable->GetInteger (0,1000)),&RoutingProtocol::SendTriggeredUpdate,this);
    }
}


void
RoutingProtocol::RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
//printf("RescRequest\n");
  NS_LOG_FUNCTION (this);
  RreqHeader rreqHeader;
  p->RemoveHeader (rreqHeader);

  // A node ignores all RREQs received from any node in its blacklist
  RoutingTableEntry2 toPrev;
  if (m_routingTable2.LookupRoute (src, toPrev))
    {
      if (toPrev.IsUnidirectional ())
        {
          NS_LOG_DEBUG ("Ignoring RREQ from node in blacklist");
          return;
        }
    }

  uint32_t id = rreqHeader.GetId ();
  Ipv4Address origin = rreqHeader.GetOrigin ();

  /*
   *  Node checks to determine whether it has received a RREQ with the same Originator IP Address and RREQ ID.
   *  If such a RREQ has been received, the node silently discards the newly received RREQ.
   */
  if (m_rreqIdCache.IsDuplicate (origin, id))
    {
      NS_LOG_DEBUG ("Ignoring RREQ due to duplicate");
      return;
    }

  // Increment RREQ hop count
  uint8_t hop = rreqHeader.GetHopCount () + 1;
  rreqHeader.SetHopCount (hop);

  if(hop > 8) return;

int check = 0;

  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_routingTable.GetListOfAllRoutes (allRoutes);
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
    {
      RoutingTableEntry rt;
      RoutingTableEntry2 rt2;
      rt = i->second;
/*
      if(i->second.GetDestination() == src &&  rreqHeader.GetHopCount () > i->second.GetHop())
         {
           return;
         }
*/
      if(i->second.GetDestination() == rreqHeader.GetDst())
        {
          check = 1;
          //rt2.SetFlag (DISCOVER);
          NS_LOG_LOGIC (src << " : sender " << receiver << " : receiver discover to " << rreqHeader.GetDst());
        }
    }


  NS_LOG_LOGIC (src << " : sender " <<receiver << " receive RREQ with hop count " << static_cast<uint32_t> (rreqHeader.GetHopCount ())
                         << " ID " << rreqHeader.GetId ()
                         << " to destination " << rreqHeader.GetDst ());

  /*
   *  When the reverse route is created or updated, the following actions on the route are also carried out:
   *  1. the Originator Sequence Number from the RREQ is compared to the corresponding destination sequence number
   *     in the route table entry and copied if greater than the existing value there
   *  2. the valid sequence number field is set to true;
   *  3. the next hop in the routing table becomes the node from which the  RREQ was received
   *  4. the hop count is copied from the Hop Count in the RREQ message;
   *  5. the Lifetime is set to be the maximum of (ExistingLifetime, MinimalLifetime), where
   *     MinimalLifetime = current time + 2*NetTraversalTime - 2*HopCount*NodeTraversalTime
   */
  RoutingTableEntry2 toOrigin;
  if (!m_routingTable2.LookupRoute (origin, toOrigin))
    {
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry2 newEntry (/*device=*/ dev, /*dst=*/ origin, /*validSeno=*/ true, /*seqNo=*/ rreqHeader.GetOriginSeqno (),
                                              /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0), /*hops=*/ hop,
                                              /*nextHop*/ src, /*timeLife=*/ Time ((2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime)));
      m_routingTable2.AddRoute (newEntry);
    }
  else
    {
      if (toOrigin.GetValidSeqNo ())
        {
          if (int32_t (rreqHeader.GetOriginSeqno ()) - int32_t (toOrigin.GetSeqNo ()) > 0)
            {
              toOrigin.SetSeqNo (rreqHeader.GetOriginSeqno ());
            }
        }
      else
        {
          toOrigin.SetSeqNo (rreqHeader.GetOriginSeqno ());
        }
      toOrigin.SetValidSeqNo (true);
      toOrigin.SetNextHop (src);
      toOrigin.SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver)));
      toOrigin.SetInterface (m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0));
      toOrigin.SetHop (hop);
      toOrigin.SetLifeTime (std::max (Time (2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime),
                                      toOrigin.GetLifeTime ()));
      m_routingTable2.Update (toOrigin);
      //m_nb.Update (src, Time (AllowedHelloLoss * HelloInterval));
    }


  RoutingTableEntry2 toNeighbor;
  if (!m_routingTable2.LookupRoute (src, toNeighbor))
    {
      NS_LOG_DEBUG ("Neighbor:" << src << " not found in routing table. Creating an entry");
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
      RoutingTableEntry2 newEntry (dev, src, false, rreqHeader.GetOriginSeqno (),
                                  m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),
                                  1, src, m_activeRouteTimeout);
      m_routingTable2.AddRoute (newEntry);
    }
  else
    {
      toNeighbor.SetLifeTime (m_activeRouteTimeout);
      toNeighbor.SetValidSeqNo (false);
      toNeighbor.SetSeqNo (rreqHeader.GetOriginSeqno ());
      toNeighbor.SetFlag (VALID);
      toNeighbor.SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver)));
      toNeighbor.SetInterface (m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0));
      toNeighbor.SetHop (1);
      toNeighbor.SetNextHop (src);
      m_routingTable2.Update (toNeighbor);
    }
  m_nb.Update (src, Time (2 * Seconds(1)));
/*
  NS_LOG_LOGIC (receiver << " receive RREQ with hop count " << static_cast<uint32_t> (rreqHeader.GetHopCount ())
                         << " ID " << rreqHeader.GetId ()
                         << " to destination " << rreqHeader.GetDst ());
*/
  NS_LOG_LOGIC (src << " : sender " <<receiver << " receive RREQ with hop count " << static_cast<uint32_t> (rreqHeader.GetHopCount ())
                         << " ID " << rreqHeader.GetId ()
                         << " to destination " << rreqHeader.GetDst ());



  //  A node generates a RREP if either:
  //  (i)  it is itself the destination,
  if (IsMyOwnAddress (rreqHeader.GetDst ()))
    {
      m_routingTable2.LookupRoute (origin, toOrigin);
      NS_LOG_DEBUG ("Send reply since I am the destination");
      SendReply (rreqHeader, toOrigin);
      return;
    }

  /*
   * (ii) or it has an active route to the destination, the destination sequence number in the node's existing route table entry for the destination
   *      is valid and greater than or equal to the Destination Sequence Number of the RREQ, and the "destination only" flag is NOT set.
   */
  RoutingTableEntry2 toDst;
  Ipv4Address dst = rreqHeader.GetDst ();
  if (m_routingTable2.LookupRoute (dst, toDst))
    {
      /*
       * Drop RREQ, This node RREP will make a loop.
       */
      if (toDst.GetNextHop () == src)
        {
          NS_LOG_DEBUG ("Drop RREQ from " << src << ", dest next hop " << toDst.GetNextHop ());
          return;
        }
      /*
       * The Destination Sequence number for the requested destination is set to the maximum of the corresponding value
       * received in the RREQ message, and the destination sequence value currently maintained by the node for the requested destination.
       * However, the forwarding node MUST NOT modify its maintained value for the destination sequence number, even if the value
       * received in the incoming RREQ is larger than the value currently maintained by the forwarding node.
       */
      if ((rreqHeader.GetUnknownSeqno () || (int32_t (toDst.GetSeqNo ()) - int32_t (rreqHeader.GetDstSeqno ()) >= 0))
          && toDst.GetValidSeqNo () )
        {
          if (!rreqHeader.GetDestinationOnly () && toDst.GetFlag () == VALID)
            {
              m_routingTable2.LookupRoute (origin, toOrigin);
              SendReplyByIntermediateNode (toDst, toOrigin, rreqHeader.GetGratuitousRrep ());
              return;
            }
          rreqHeader.SetDstSeqno (toDst.GetSeqNo ());
          rreqHeader.SetUnknownSeqno (false);
        }
    }

  SocketIpTtlTag tag;
  p->RemovePacketTag (tag);
  if (tag.GetTtl () < 2)
    {
      NS_LOG_DEBUG ("TTL exceeded. Drop RREQ origin " << src << " destination " << dst );
      return;
    }

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      //SocketIpTtlTag ttl;
      //ttl.SetTtl (tag.GetTtl () - 1);
      //packet->AddPacketTag (ttl);
      packet->AddHeader (rreqHeader);
      //TypeHeader tHeader (AODVTYPE_RREQ);
      //packet->AddHeader (tHeader);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;

//broadcast or unicast
if(check == 0)  //if(toDst.GetFlag () == IN_SEARCH)
  {
//printf("0 \n");
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      m_lastBcastTime = Simulator::Now ();
      Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, destination);
    }else if(check == 1)  //else if(toDst.GetFlag () == DISCOVER)
    {
//printf("1 \n");
//bordercast先のルートへのnexthopに送信したい
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_routingTable.GetListOfAllRoutes (allRoutes);
  //m_lastBcastTime = Simulator::Now ();
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
    {
      RoutingTableEntry rt;
      rt = i->second;
      if(i->second.GetDestination() == rreqHeader.GetDst ())
        {
         NS_LOG_LOGIC ("Hop: " << i->second.GetHop() << "dest" << i->second.GetDestination() << "next" << i->second.GetNextHop());
         destination = i->second.GetNextHop();
         Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, destination);
return;
        }
    }
    }

    }

}

bool
RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
//printf("IsMyOwnAddress \n");
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
//printf("false\n");
  return false;
}


void
RoutingProtocol::RecvReply (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
  //printf("RecvReply\n");
  NS_LOG_FUNCTION (this << " src " << sender);
  RrepHeader rrepHeader;
  p->RemoveHeader (rrepHeader);
  Ipv4Address dst = rrepHeader.GetDst ();
  NS_LOG_LOGIC ("RREP destination " << dst << " RREP origin " << rrepHeader.GetOrigin ());

  uint8_t hop = rrepHeader.GetHopCount () + 1;
  rrepHeader.SetHopCount (hop);

/*
  // If RREP is Hello message
  if (dst == rrepHeader.GetOrigin ())
    {
      //ProcessHello (rrepHeader, receiver);
      return;
    }
*/
  /*
   * If the route table entry to the destination is created or updated, then the following actions occur:
   * -  the route is marked as active,
   * -  the destination sequence number is marked as valid,
   * -  the next hop in the route entry is assigned to be the node from which the RREP is received,
   *    which is indicated by the source IP address field in the IP header,
   * -  the hop count is set to the value of the hop count from RREP message + 1
   * -  the expiry time is set to the current time plus the value of the Lifetime in the RREP message,
   * -  and the destination sequence number is the Destination Sequence Number in the RREP message.
   */
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (receiver));
  RoutingTableEntry2 newEntry (/*device=*/ dev, /*dst=*/ dst, /*validSeqNo=*/ true, /*seqno=*/ rrepHeader.GetDstSeqno (),
                                          /*iface=*/ m_ipv4->GetAddress (m_ipv4->GetInterfaceForAddress (receiver), 0),/*hop=*/ hop,
                                          /*nextHop=*/ sender, /*lifeTime=*/ rrepHeader.GetLifeTime ());
  RoutingTableEntry2 toDst;
  if (m_routingTable2.LookupRoute (dst, toDst))
    {
      /*
       * The existing entry is updated only in the following circumstances:
       * (i) the sequence number in the routing table is marked as invalid in route table entry.
       */
      if (!toDst.GetValidSeqNo ())
        {
          m_routingTable2.Update (newEntry);
        }
      // (ii)the Destination Sequence Number in the RREP is greater than the node's copy of the destination sequence number and the known value is valid,
      else if ((int32_t (rrepHeader.GetDstSeqno ()) - int32_t (toDst.GetSeqNo ())) > 0)
        {
          m_routingTable2.Update (newEntry);
        }
      else
        {
          // (iii) the sequence numbers are the same, but the route is marked as inactive.
          if ((rrepHeader.GetDstSeqno () == toDst.GetSeqNo ()) && (toDst.GetFlag () != VALID))
            {
              m_routingTable2.Update (newEntry);
            }
          // (iv)  the sequence numbers are the same, and the New Hop Count is smaller than the hop count in route table entry.
          else if ((rrepHeader.GetDstSeqno () == toDst.GetSeqNo ()) && (hop < toDst.GetHop ()))
            {
              m_routingTable2.Update (newEntry);
            }
        }
    }
  else
    {
      // The forward route for this destination is created if it does not already exist.
      NS_LOG_LOGIC ("add new route");
      m_routingTable2.AddRoute (newEntry);
    }

  // Acknowledge receipt of the RREP by sending a RREP-ACK message back
  if (rrepHeader.GetAckRequired ())
    {
      SendReplyAck (sender);
      rrepHeader.SetAckRequired (false);
    }

  NS_LOG_LOGIC ("receiver " << receiver << " origin " << rrepHeader.GetOrigin ());
  if (IsMyOwnAddress (rrepHeader.GetOrigin ()))
    {
      if (toDst.GetFlag () == IN_SEARCH || toDst.GetFlag () == DISCOVER)
        {
          m_routingTable2.Update (newEntry);
          m_addressReqTimer[dst].Remove ();
          m_addressReqTimer.erase (dst);
        }
      m_routingTable2.LookupRoute (dst, toDst);
      SendPacketFromQueue2 (dst, toDst.GetRoute ());
      return;
    }

  RoutingTableEntry2 toOrigin;
  if (!m_routingTable2.LookupRoute (rrepHeader.GetOrigin (), toOrigin) || toOrigin.GetFlag () == IN_SEARCH)
    {
      return; // Impossible! drop.
    }
  toOrigin.SetLifeTime (std::max (m_activeRouteTimeout, toOrigin.GetLifeTime ()));
  m_routingTable2.Update (toOrigin);

  // Update information about precursors
  if (m_routingTable2.LookupValidRoute (rrepHeader.GetDst (), toDst))
    {
      toDst.InsertPrecursor (toOrigin.GetNextHop ());
      m_routingTable2.Update (toDst);

      RoutingTableEntry2 toNextHopToDst;
      m_routingTable2.LookupRoute (toDst.GetNextHop (), toNextHopToDst);
      toNextHopToDst.InsertPrecursor (toOrigin.GetNextHop ());
      m_routingTable2.Update (toNextHopToDst);

      toOrigin.InsertPrecursor (toDst.GetNextHop ());
      m_routingTable2.Update (toOrigin);

      RoutingTableEntry2 toNextHopToOrigin;
      m_routingTable2.LookupRoute (toOrigin.GetNextHop (), toNextHopToOrigin);
      toNextHopToOrigin.InsertPrecursor (toDst.GetNextHop ());
      m_routingTable2.Update (toNextHopToOrigin);
    }
/*
  SocketIpTtlTag tag;
  p->RemovePacketTag (tag);
  if (tag.GetTtl () < 2)
    {
      NS_LOG_DEBUG ("TTL exceeded. Drop RREP destination " << dst << " origin " << rrepHeader.GetOrigin ());
      return;
    }
*/

  Ptr<Packet> packet = Create<Packet> ();
  //SocketIpTtlTag ttl;
  //ttl.SetTtl (tag.GetTtl () - 1);
  //packet->AddPacketTag (ttl);
  packet->AddHeader (rrepHeader);
  //TypeHeader tHeader (AODVTYPE_RREP);
  //packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), MY_PORT));
}
void
RoutingProtocol::RecvReplyAck (Ipv4Address neighbor)
{
  NS_LOG_FUNCTION (this);
  RoutingTableEntry2 rt;
  if (m_routingTable2.LookupRoute (neighbor, rt))
    {
      rt.m_ackTimer.Cancel ();
      rt.SetFlag (VALID);
      m_routingTable2.Update (rt);
    }
}


void
RoutingProtocol::SendReplyAck (Ipv4Address neighbor)
{
  NS_LOG_FUNCTION (this << " to " << neighbor);
  RrepAckHeader h;
  //TypeHeader typeHeader (SHINGO_RREP_ACK);
  Ptr<Packet> packet = Create<Packet> ();
  //SocketIpTtlTag tag;
  //tag.SetTtl (1);
  //packet->AddPacketTag (tag);
  packet->AddHeader (h);
  //packet->AddHeader (typeHeader);
  RoutingTableEntry toNeighbor;
  m_routingTable.LookupRoute (neighbor, toNeighbor);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toNeighbor.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (neighbor, MY_PORT));
}


//RecvIarp後動く、データ更新して送信するやつ
void
RoutingProtocol::SendTriggeredUpdate ()
{
  NS_LOG_FUNCTION (m_mainAddress << " is sending a triggered update");
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_advRoutingTable.GetListOfAllRoutes (allRoutes);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      IarpHeader iarpHeader;
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
        {
        //ホップ数設定
        if(i->second.GetHop() < 2){
          NS_LOG_LOGIC ("Destination: " << i->second.GetDestination ()
                                        << " SeqNo:" << i->second.GetSeqNo () << " HopCount:"
                                        << i->second.GetHop () + 1);
          RoutingTableEntry temp = i->second;
          if ((i->second.GetEntriesChanged () == true) && (!m_advRoutingTable.AnyRunningEvent (temp.GetDestination ())))
            {
              iarpHeader.SetDst (i->second.GetDestination ());
              iarpHeader.SetDstSeqno (i->second.GetSeqNo ());
              iarpHeader.SetHopCount (i->second.GetHop () + 1);
              temp.SetFlag (VALID);
              temp.SetEntriesChanged (false);
              m_advRoutingTable.DeleteIpv4Event (temp.GetDestination ());
              if (!(temp.GetSeqNo () % 2))
                {
                  m_routingTable.Update (temp);
                }
              packet->AddHeader (iarpHeader);
              //TypeHeader tHeader (SHINGO_IARP);
              //packet->AddHeader (tHeader);
              m_advRoutingTable.DeleteRoute (temp.GetDestination ());
              NS_LOG_DEBUG ("Deleted this route from the advertised table");
            }
          else
            {
              EventId event = m_advRoutingTable.GetEventId (temp.GetDestination ());
              NS_ASSERT (event.GetUid () != 0);
              NS_LOG_DEBUG ("EventID " << event.GetUid () << " associated with "
                                       << temp.GetDestination () << " has not expired, waiting in adv table");
            }
        }
        }
      if (packet->GetSize () >= 12 && iarpHeader.GetHopCount() <= 2) //ホップ数設定
        {
          RoutingTableEntry temp2;
          m_routingTable.LookupRoute (m_ipv4->GetAddress (1, 0).GetBroadcast (), temp2);
          iarpHeader.SetDst (m_ipv4->GetAddress (1, 0).GetLocal ());
          iarpHeader.SetDstSeqno (temp2.GetSeqNo ());
          iarpHeader.SetHopCount (temp2.GetHop () + 1);
          NS_LOG_DEBUG ("Adding my update as well to the packet");
          packet->AddHeader (iarpHeader);
          //TypeHeader tHeader (SHINGO_IARP);
          //packet->AddHeader (tHeader);
          // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
          Ipv4Address destination;
          if (iface.GetMask () == Ipv4Mask::GetOnes ())
            {
              destination = Ipv4Address ("255.255.255.255");
            }
          else
            {
              destination = iface.GetBroadcast ();
            }
          socket->SendTo (packet, 0, InetSocketAddress (destination, MY_PORT));
          NS_LOG_FUNCTION ("Sent Triggered Update from "
                           << iarpHeader.GetDst ()
                           << " with packet id : " << packet->GetUid () << " and packet Size: " << packet->GetSize ());
        }
      else
        {
          NS_LOG_FUNCTION ("Update not sent as there are no updates to be triggered");
        }
    }
}

//ブロードキャストで経路情報の更新するやつ?
void
RoutingProtocol::SendPeriodicUpdate ()
{
  std::map<Ipv4Address, RoutingTableEntry> removedAddresses, allRoutes;
  m_routingTable.Purge (removedAddresses);
  MergeTriggerPeriodicUpdates ();
  m_routingTable.GetListOfAllRoutes (allRoutes);
  if (allRoutes.empty ())
    {
      return;
    }
  NS_LOG_FUNCTION (m_mainAddress << " is sending out its periodic update");
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      Ptr<Packet> packet = Create<Packet> ();
      for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
        {
          IarpHeader iarpHeader;
          if (i->second.GetHop () == 0)
            {
              RoutingTableEntry ownEntry;
              iarpHeader.SetDst (m_ipv4->GetAddress (1,0).GetLocal ());
              iarpHeader.SetDstSeqno (i->second.GetSeqNo () + 2);
              iarpHeader.SetHopCount (i->second.GetHop () + 1);
              m_routingTable.LookupRoute (m_ipv4->GetAddress (1,0).GetBroadcast (),ownEntry);
              ownEntry.SetSeqNo (iarpHeader.GetDstSeqno ());
              m_routingTable.Update (ownEntry);
              packet->AddHeader (iarpHeader);
              //TypeHeader tHeader (SHINGO_IARP);
              //packet->AddHeader (tHeader);

              NS_LOG_DEBUG ("Forwarding the update for " << i->first);
              NS_LOG_DEBUG ("Forwarding details are, Destination: " << iarpHeader.GetDst ()
                                                                << ", SeqNo:" << iarpHeader.GetDstSeqno ()
                                                                << ", HopCount:" << iarpHeader.GetHopCount ()
                                                                << ", LifeTime: " << i->second.GetLifeTime ().GetSeconds ());
            }
          else if (i -> second.GetHop() <= 2) //ホップ数設定
            {
              iarpHeader.SetDst (i->second.GetDestination ());
              iarpHeader.SetDstSeqno ((i->second.GetSeqNo ()));
              iarpHeader.SetHopCount (i->second.GetHop () + 1);
              packet->AddHeader (iarpHeader);
              //TypeHeader tHeader (SHINGO_IARP);
              //packet->AddHeader (tHeader);

              NS_LOG_DEBUG ("Forwarding the update for " << i->first);
              NS_LOG_DEBUG ("Forfwarding details are, Destination: " << iarpHeader.GetDst ()
                                                                << ", SeqNo:" << iarpHeader.GetDstSeqno ()
                                                                << ", HopCount:" << iarpHeader.GetHopCount ()
                                                                << ", LifeTime: " << i->second.GetLifeTime ().GetSeconds ());
            }
/*
          NS_LOG_DEBUG ("Forwarding the update for " << i->first);
          NS_LOG_DEBUG ("Forwarding details are, Destination: " << iarpHeader.GetDst ()
                                                                << ", SeqNo:" << iarpHeader.GetDstSeqno ()
                                                                << ", HopCount:" << iarpHeader.GetHopCount ()
                                                                << ", LifeTime: " << i->second.GetLifeTime ().GetSeconds ());
*/
        }
      for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator rmItr = removedAddresses.begin (); rmItr
           != removedAddresses.end (); ++rmItr)
        {
          IarpHeader removedHeader;
          removedHeader.SetDst (rmItr->second.GetDestination ());
          removedHeader.SetDstSeqno (rmItr->second.GetSeqNo () + 1);
          removedHeader.SetHopCount (rmItr->second.GetHop () + 1);
          packet->AddHeader (removedHeader);
          //TypeHeader tHeader (SHINGO_IARP);
          //packet->AddHeader (tHeader);
          NS_LOG_DEBUG ("Update for removed record is: Destination: " << removedHeader.GetDst ()
                                                                      << " SeqNo:" << removedHeader.GetDstSeqno ()
                                                                      << " HopCount:" << removedHeader.GetHopCount ());
        }
      socket->Send (packet);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      socket->SendTo (packet, 0, InetSocketAddress (destination, MY_PORT));
      NS_LOG_FUNCTION ("PeriodicUpdate Packet UID is : " << packet->GetUid ());
    }
  m_periodicUpdateTimer.Schedule (m_periodicUpdateInterval + MicroSeconds (25 * m_uniformRandomVariable->GetInteger (0,1000)));
}


void
RoutingProtocol::SendRequest (Ipv4Address dst)
{
  //printf("SendRequest\n");
  NS_LOG_FUNCTION ( this << dst);
  // A node SHOULD NOT originate more than RREQ_RATELIMIT RREQ messages per second.

  if (m_rreqCount == m_rreqRateLimit)
    {
      Simulator::Schedule (m_rreqRateLimitTimer.GetDelayLeft () + MicroSeconds (100),
                           &RoutingProtocol::SendRequest, this, dst);
      return;
    }
  else
    {
      m_rreqCount++;
    }

//m_rreqCount++;
  // Create RREQ header
  RreqHeader rreqHeader;
  rreqHeader.SetDst (dst);
  //rreqHeader.SetRad(Ipv4Address("10.1.1.12"));

  RoutingTableEntry2 rt;
  // Using the Hop field in Routing Table to manage the expanding ring search
  uint16_t ttl = m_ttlStart;
/*
if(rt.GetFlag() == DISCOVER){

  if (m_routingTable2.LookupRoute (dst, rt))
    {
          ttl = rt.GetHop () + m_ttlIncrement;
          if (ttl > m_ttlThreshold)
            {
              ttl = m_netDiameter;
            }
        
      if (ttl == m_netDiameter)
        {
          rt.IncrementRreqCnt ();
        }
      if (rt.GetValidSeqNo ())
        {
          rreqHeader.SetDstSeqno (rt.GetSeqNo ());
        }
      else
        {
          rreqHeader.SetUnknownSeqno (true);
        }
      rt.SetHop (ttl);
      rt.SetFlag (DISCOVER);
      rt.SetLifeTime (m_pathDiscoveryTime);
      m_routingTable2.Update (rt);
    }
  else
    {
      rreqHeader.SetUnknownSeqno (true);
      Ptr<NetDevice> dev = 0;
      RoutingTableEntry2 newEntry ( dev, dst,  false,  0, Ipv4InterfaceAddress (), ttl,Ipv4Address (),  m_pathDiscoveryTime);


      // Check if TtlStart == NetDiameter
      if (ttl == m_netDiameter)
        {
          newEntry.IncrementRreqCnt ();
        }
      newEntry.SetFlag (DISCOVER);
      m_routingTable2.AddRoute (newEntry);
    }
} else{
*/


  if (m_routingTable2.LookupRoute (dst, rt))
    {
      if (rt.GetFlag () != IN_SEARCH)
        {
          ttl = std::min<uint16_t> (rt.GetHop () + m_ttlIncrement, m_netDiameter);
        }
      else
        {
          ttl = rt.GetHop () + m_ttlIncrement;
          if (ttl > m_ttlThreshold)
            {
              ttl = m_netDiameter;
            }
        }
      if (ttl == m_netDiameter)
        {
          rt.IncrementRreqCnt ();
        }
      if (rt.GetValidSeqNo ())
        {
          rreqHeader.SetDstSeqno (rt.GetSeqNo ());
        }
      else
        {
          rreqHeader.SetUnknownSeqno (true);
        }
      rt.SetHop (ttl);
      rt.SetFlag (IN_SEARCH);
      rt.SetLifeTime (m_pathDiscoveryTime);
      m_routingTable2.Update (rt);
    }
  else
    {
      rreqHeader.SetUnknownSeqno (true);
      Ptr<NetDevice> dev = 0;
      RoutingTableEntry2 newEntry (/*device=*/ dev, /*dst=*/ dst, /*validSeqNo=*/ false, /*seqno=*/ 0,
                                              /*iface=*/ Ipv4InterfaceAddress (),/*hop=*/ ttl,
                                              /*nextHop=*/ Ipv4Address (), /*lifeTime=*/ m_pathDiscoveryTime);
      // Check if TtlStart == NetDiameter
      if (ttl == m_netDiameter)
        {
          newEntry.IncrementRreqCnt ();
        }
      newEntry.SetFlag (IN_SEARCH);
      m_routingTable2.AddRoute (newEntry);
    }
//}
  if (m_gratuitousReply)
    {
      rreqHeader.SetGratuitousRrep (true);
    }
  if (m_destinationOnly)
    {
      rreqHeader.SetDestinationOnly (true);
    }

  m_seqNo++;
  rreqHeader.SetOriginSeqno (m_seqNo);
  m_requestId++;
  rreqHeader.SetId (m_requestId);

  // Send RREQ as subnet directed broadcast from each interface used by aodv
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;

      rreqHeader.SetOrigin (iface.GetLocal ());
      m_rreqIdCache.IsDuplicate (iface.GetLocal (), m_requestId);

      Ptr<Packet> packet = Create<Packet> ();
      SocketIpTtlTag tag;
      tag.SetTtl (ttl);
      packet->RemovePacketTag (tag);
      //packet->AddPacketTag (tag);
      packet->AddHeader (rreqHeader);
      //TypeHeader tHeader (AODVTYPE_RREQ);
      //packet->AddHeader (tHeader);

      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;

      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      NS_LOG_DEBUG ("Send RREQ with id " << rreqHeader.GetId () << " to socket");
      m_lastBcastTime = Simulator::Now ();
      Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))), &RoutingProtocol::SendTo, this, socket, packet, destination);
/*
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_routingTable.GetListOfAllRoutes (allRoutes);
  m_lastBcastTime = Simulator::Now ();
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
    {
      RoutingTableEntry rt;
      rt = i->second;
      int sec = 0;
      if(i->second.GetHop() == 5)
        {
         destination = i->second.GetDestination();
         NS_LOG_LOGIC ("Hop: " << i->second.GetHop() << "dest" << i->second.GetDestination() );
         destination = i->second.GetNextHop();
         Simulator::Schedule (Time (MilliSeconds (m_uniformRandomVariable->GetInteger (0, 10))+ MilliSeconds(1)), &RoutingProtocol::SendTo, this, socket, packet, destination);
break;
        }
      sec = sec + 5;
    }     
*/
    }
  ScheduleRreqRetry (dst);
}


void
RoutingProtocol::SendReply (RreqHeader const & rreqHeader, RoutingTableEntry2 const & toOrigin)
{
  //printf("SendRelly\n");
  NS_LOG_FUNCTION (this << toOrigin.GetDestination ());
  /*
   * Destination node MUST increment its own sequence number by one if the sequence number in the RREQ packet is equal to that
   * incremented value. Otherwise, the destination does not change its sequence number before generating the  RREP message.
   */
  if (!rreqHeader.GetUnknownSeqno () && (rreqHeader.GetDstSeqno () == m_seqNo + 1))
    {
      m_seqNo++;
    }
  RrepHeader rrepHeader ( /*prefixSize=*/ 0, /*hops=*/ 0, /*dst=*/ rreqHeader.GetDst (),
                                          /*dstSeqNo=*/ m_seqNo, /*origin=*/ toOrigin.GetDestination (), /*lifeTime=*/ m_myRouteTimeout);
  Ptr<Packet> packet = Create<Packet> ();
  //SocketIpTtlTag tag;
  //tag.SetTtl (toOrigin.GetHop ());
  //packet->AddPacketTag (tag);
  packet->AddHeader (rrepHeader);
  //TypeHeader tHeader (AODVTYPE_RREP);
  //packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), MY_PORT));
}

void
RoutingProtocol::SendReplyByIntermediateNode (RoutingTableEntry2 & toDst, RoutingTableEntry2 & toOrigin, bool gratRep)
{
  //printf("SendReplyByIntermedia \n");
  NS_LOG_FUNCTION (this);
  RrepHeader rrepHeader (/*prefix size=*/ 0, /*hops=*/ toDst.GetHop (), /*dst=*/ toDst.GetDestination (), /*dst seqno=*/ toDst.GetSeqNo (),
                                          /*origin=*/ toOrigin.GetDestination (), /*lifetime=*/ toDst.GetLifeTime ());
  /* If the node we received a RREQ for is a neighbor we are
   * probably facing a unidirectional link... Better request a RREP-ack
   */
  if (toDst.GetHop () == 1)
    {
      rrepHeader.SetAckRequired (true);
      RoutingTableEntry2 toNextHop;
      m_routingTable2.LookupRoute (toOrigin.GetNextHop (), toNextHop);
      toNextHop.m_ackTimer.SetFunction (&RoutingProtocol::AckTimerExpire, this);
      toNextHop.m_ackTimer.SetArguments (toNextHop.GetDestination (), m_blackListTimeout);
      toNextHop.m_ackTimer.SetDelay (m_nextHopWait);
    }
  toDst.InsertPrecursor (toOrigin.GetNextHop ());
  toOrigin.InsertPrecursor (toDst.GetNextHop ());
  m_routingTable2.Update (toDst);
  m_routingTable2.Update (toOrigin);

  Ptr<Packet> packet = Create<Packet> ();
  //SocketIpTtlTag tag;
  //tag.SetTtl (toOrigin.GetHop ());
  //packet->AddPacketTag (tag);
  packet->AddHeader (rrepHeader);
  //TypeHeader tHeader (AODVTYPE_RREP);
  //packet->AddHeader (tHeader);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (toOrigin.GetInterface ());
  NS_ASSERT (socket);
  socket->SendTo (packet, 0, InetSocketAddress (toOrigin.GetNextHop (), MY_PORT));

  // Generating gratuitous RREPs
  if (gratRep)
    {
      RrepHeader gratRepHeader (/*prefix size=*/ 0, /*hops=*/ toOrigin.GetHop (), /*dst=*/ toOrigin.GetDestination (),
                                                 /*dst seqno=*/ toOrigin.GetSeqNo (), /*origin=*/ toDst.GetDestination (),
                                                 /*lifetime=*/ toOrigin.GetLifeTime ());
      Ptr<Packet> packetToDst = Create<Packet> ();
      //SocketIpTtlTag gratTag;
      //gratTag.SetTtl (toDst.GetHop ());
      //packetToDst->AddPacketTag (gratTag);
      packetToDst->AddHeader (gratRepHeader);
      //TypeHeader type (AODVTYPE_RREP);
      //packetToDst->AddHeader (type);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (toDst.GetInterface ());
      NS_ASSERT (socket);
      NS_LOG_LOGIC ("Send gratuitous RREP " << packet->GetUid ());
      socket->SendTo (packetToDst, 0, InetSocketAddress (toDst.GetNextHop (), MY_PORT));
    }
}


void
RoutingProtocol::SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  socket->SendTo (packet, 0, InetSocketAddress (destination, MY_PORT));

}

void
RoutingProtocol::ScheduleRreqRetry (Ipv4Address dst)
{
//printf("ScheduleRreqRetry \n");
  NS_LOG_FUNCTION (this << dst);
  if (m_addressReqTimer.find (dst) == m_addressReqTimer.end ())
    {
      Timer timer (Timer::CANCEL_ON_DESTROY);
      m_addressReqTimer[dst] = timer;
    }
  m_addressReqTimer[dst].SetFunction (&RoutingProtocol::RouteRequestTimerExpire, this);
  m_addressReqTimer[dst].Remove ();
  m_addressReqTimer[dst].SetArguments (dst);
  RoutingTableEntry2 rt;
  m_routingTable2.LookupRoute (dst, rt);
  Time retry;
  if (rt.GetHop () < m_netDiameter)
    {
      retry = 2 * m_nodeTraversalTime * (rt.GetHop () + m_timeoutBuffer);
    }
  else
    {
      NS_ABORT_MSG_UNLESS (rt.GetRreqCnt () > 0, "Unexpected value for GetRreqCount ()");
      uint16_t backoffFactor = rt.GetRreqCnt () - 1;
      NS_LOG_LOGIC ("Applying binary exponential backoff factor " << backoffFactor);
      retry = m_netTraversalTime * (1 << backoffFactor);
    }
  m_addressReqTimer[dst].Schedule (retry);
  NS_LOG_LOGIC ("Scheduled RREQ retry in " << retry.GetSeconds () << " seconds");
}

void
RoutingProtocol::RouteRequestTimerExpire (Ipv4Address dst)
{
//printf("RouteRequestTimer \n");
  NS_LOG_LOGIC (this);
  RoutingTableEntry2 toDst;
  if (m_routingTable2.LookupValidRoute (dst, toDst))
    {
      SendPacketFromQueue2 (dst, toDst.GetRoute ());
      NS_LOG_LOGIC ("route to " << dst << " found");
      return;
    }
  /*
   *  If a route discovery has been attempted RreqRetries times at the maximum TTL without
   *  receiving any RREP, all data packets destined for the corresponding destination SHOULD be
   *  dropped from the buffer and a Destination Unreachable message SHOULD be delivered to the application.
   */
  if (toDst.GetRreqCnt () == m_rreqRetries)
    {
      NS_LOG_LOGIC ("route discovery to " << dst << " has been attempted RreqRetries (" << m_rreqRetries << ") times with ttl " << m_netDiameter);
      m_addressReqTimer.erase (dst);
      m_routingTable2.DeleteRoute (dst);
      NS_LOG_DEBUG ("Route not found. Drop all packets with dst " << dst);
      m_queue2.DropPacketWithDst (dst);
      return;
    }

  if (toDst.GetFlag () == IN_SEARCH || toDst.GetFlag () == DISCOVER)
    {
      NS_LOG_LOGIC ("Resend RREQ to " << dst << " previous ttl " << toDst.GetHop ());
      SendRequest (dst);
    }
  else
    {
      NS_LOG_DEBUG ("Route down. Stop search. Drop packet with destination " << dst);
      m_addressReqTimer.erase (dst);
      m_routingTable2.DeleteRoute (dst);
      m_queue2.DropPacketWithDst (dst);
    }
}

void
RoutingProtocol::AckTimerExpire (Ipv4Address neighbor, Time blacklistTimeout)
{
  NS_LOG_FUNCTION (this);
  m_routingTable2.MarkLinkAsUnidirectional (neighbor, blacklistTimeout);
}



void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  m_ipv4 = ipv4;
  // Create lo route. It is asserted that the only one interface up for now is loopback
  NS_ASSERT (m_ipv4->GetNInterfaces () == 1 && m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  m_lo = m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_lo != 0);
  // Remember lo route
  RoutingTableEntry rt (
    /*device=*/ m_lo,  /*dst=*/
    Ipv4Address::GetLoopback (), /*seqno=*/
    0,
    /*iface=*/ Ipv4InterfaceAddress (Ipv4Address::GetLoopback (),Ipv4Mask ("255.0.0.0")),
    /*hops=*/ 0,  /*next hop=*/
    Ipv4Address::GetLoopback (),
    /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  rt.SetFlag (INVALID);
  rt.SetEntriesChanged (false);
  m_routingTable.AddRoute (rt);
 Simulator::ScheduleNow (&RoutingProtocol::Start, this);
}


void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ()
                        << " interface is up");
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }
 // 該当するI/Fの専用ソケットを生成する
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShingo,this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MY_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetAttribute ("IpTtl",UintegerValue (1));
  m_socketAddresses.insert (std::make_pair (socket,iface));

 //経路エントリーを作成する
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*seqno=*/ 0,/*iface=*/ iface,/*hops=*/ 0,
                                    /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_routingTable.AddRoute (rt);
  if (m_mainAddress == Ipv4Address ())
    {
      m_mainAddress = iface.GetLocal ();
    }
  NS_ASSERT (m_mainAddress != Ipv4Address ());
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (i);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (i,0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No iarp interfaces");
      m_routingTable.Clear ();
      return;
    }
  m_routingTable.DeleteAllRoutesFromInterface (m_ipv4->GetAddress (i,0));
  m_advRoutingTable.DeleteAllRoutesFromInterface (m_ipv4->GetAddress (i,0));
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
        NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    {
      return;
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
  if (!socket)
    {
      if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
        {
          return;
        }
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
      NS_ASSERT (socket != 0);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShingo,this));
      // Bind to any IP address so that broadcasts can be received
      socket->BindToNetDevice (l3->GetNetDevice (i));
      socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MY_PORT));
      socket->SetAllowBroadcast (true);
      m_socketAddresses.insert (std::make_pair (socket,iface));
      Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
      RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (),/*seqno=*/ 0, /*iface=*/ iface,/*hops=*/ 0,
                                        /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
      m_routingTable.AddRoute (rt);
    }
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {
      m_socketAddresses.erase (socket);
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvShingo,this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MY_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket,iface));
        }
    }
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr) const
{
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j
       != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }
  Ptr<Socket> socket;
  return socket;
}


void
RoutingProtocol::Send (Ptr<Ipv4Route> route,
                       Ptr<const Packet> packet,
                       const Ipv4Header & header)
{
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  NS_ASSERT (l3 != 0);
  Ptr<Packet> p = packet->Copy ();
  l3->Send (p,route->GetSource (),header.GetDestination (),header.GetProtocol (),route);
}

void
RoutingProtocol::Drop (Ptr<const Packet> packet,
                       const Ipv4Header & header,
                       Socket::SocketErrno err)
{
  NS_LOG_DEBUG (m_mainAddress << " drop packet " << packet->GetUid () << " to "
                              << header.GetDestination () << " from queue. Error " << err);
}

void
RoutingProtocol::RreqRateLimitTimerExpire ()
{
  NS_LOG_FUNCTION (this);
  m_rreqCount = 0;
  m_rreqRateLimitTimer.Schedule (Seconds (1));
}

void
RoutingProtocol::LookForQueuedPackets ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4Route> route;
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_routingTable.GetListOfAllRoutes (allRoutes);
  for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
    {
      RoutingTableEntry rt;
      rt = i->second;
      if (m_queue.Find (rt.GetDestination ()))
        {
          if (rt.GetHop () == 1)
            {
              route = rt.GetRoute ();
              NS_LOG_LOGIC ("A route exists from " << route->GetSource ()
                                                   << " to neighboring destination "
                                                   << route->GetDestination ());
              NS_ASSERT (route != 0);
            }
          else
            {
              RoutingTableEntry newrt;
              m_routingTable.LookupRoute (rt.GetNextHop (),newrt);
              route = newrt.GetRoute ();
              NS_LOG_LOGIC ("A route exists from " << route->GetSource ()
                                                   << " to destination " << route->GetDestination () << " via "
                                                   << rt.GetNextHop ());
              NS_ASSERT (route != 0);
            }
          SendPacketFromQueue (rt.GetDestination (),route);
        }
    }
}

void
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst,
                                      Ptr<Ipv4Route> route)
{
  NS_LOG_DEBUG (m_mainAddress << " is sending a queued packet to destination " << dst);
  QueueEntry queueEntry;
  if (m_queue.Dequeue (dst,queueEntry))
    {
      DeferredRouteOutputTag tag;
      Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
      if (p->RemovePacketTag (tag))
        {
          if (tag.oif != -1 && tag.oif != m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ()))
            {
              NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
              return;
            }
        }
      UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
      Ipv4Header header = queueEntry.GetIpv4Header ();
      header.SetSource (route->GetSource ());
      header.SetTtl (header.GetTtl () + 1); // compensate extra TTL decrement by fake loopback routing
      ucb (route,p,header);
      if (m_queue.GetSize () != 0 && m_queue.Find (dst))
        {
          Simulator::Schedule (MilliSeconds (m_uniformRandomVariable->GetInteger (0,100)),
                               &RoutingProtocol::SendPacketFromQueue,this,dst,route);
        }
    }
}

void
RoutingProtocol::SendPacketFromQueue2 (Ipv4Address dst, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this);
  QueueEntry queueEntry;
  while (m_queue2.Dequeue (dst, queueEntry))
    {
      DeferredRouteOutputTag tag;
      Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
      if (p->RemovePacketTag (tag)
          && tag.oif != -1
          && tag.oif != m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ()))
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          return;
        }
      UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
      Ipv4Header header = queueEntry.GetIpv4Header ();
      header.SetSource (route->GetSource ());
      header.SetTtl (header.GetTtl () + 1); // compensate extra TTL decrement by fake loopback routing
      ucb (route, p, header);
    }
}


Time
RoutingProtocol::GetSettlingTime (Ipv4Address address)
{
  NS_LOG_FUNCTION ("Calculating the settling time for " << address);
  RoutingTableEntry mainrt;
  Time weightedTime;
  m_routingTable.LookupRoute (address,mainrt);
  if (EnableWST)
    {
      if (mainrt.GetSettlingTime () == Seconds (0))
        {
          return Seconds (0);
        }
      else
        {
          NS_LOG_DEBUG ("Route SettlingTime: " << mainrt.GetSettlingTime ().GetSeconds ()
                                               << " and LifeTime:" << mainrt.GetLifeTime ().GetSeconds ());
          weightedTime = Time (m_weightedFactor * mainrt.GetSettlingTime ().GetSeconds () + (1.0 - m_weightedFactor)
                               * mainrt.GetLifeTime ().GetSeconds ());
          NS_LOG_DEBUG ("Calculated weightedTime:" << weightedTime.GetSeconds ());
          return weightedTime;
        }
    }
  return mainrt.GetSettlingTime ();
}

void
RoutingProtocol::MergeTriggerPeriodicUpdates ()
{
  NS_LOG_FUNCTION ("Merging advertised table changes with main table before sending out periodic update");
  std::map<Ipv4Address, RoutingTableEntry> allRoutes;
  m_advRoutingTable.GetListOfAllRoutes (allRoutes);
  if (allRoutes.size () > 0)
    {
      for (std::map<Ipv4Address, RoutingTableEntry>::const_iterator i = allRoutes.begin (); i != allRoutes.end (); ++i)
        {
          RoutingTableEntry advEntry = i->second;
          if ((advEntry.GetEntriesChanged () == true) && (!m_advRoutingTable.AnyRunningEvent (advEntry.GetDestination ())))
            {
              if (!(advEntry.GetSeqNo () % 2))
                {
                  advEntry.SetFlag (VALID);
                  advEntry.SetEntriesChanged (false);
                  m_routingTable.Update (advEntry);
                  NS_LOG_DEBUG ("Merged update for " << advEntry.GetDestination () << " with main routing Table");
                }
              m_advRoutingTable.DeleteRoute (advEntry.GetDestination ());
            }
          else
            {
              NS_LOG_DEBUG ("Event currently running. Cannot Merge Routing Tables");
            }
        }
    }
}




}
}

