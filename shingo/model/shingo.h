/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SHINGO_H
#define SHIGNO_H
//#pragma once 

#include "shingo-table.h"
#include "shingo-neighbor.h"
#include "shingo-dpd.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

namespace ns3
{
namespace shingo
{

class RoutingProtocol : public Ipv4RoutingProtocol
{
  public:
 static TypeId GetTypeId (void);
 static const uint32_t MY_PORT;

 RoutingProtocol ();
 virtual ~RoutingProtocol();
 virtual void DoDispose ();

   //パケットの転送経路を決定する処理関数
 Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, 
     Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

   //受信パケットの配送処理関数
 bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, 
  Ptr<const NetDevice> idev, UnicastForwardCallback ucb, 
  MulticastForwardCallback mcb, LocalDeliverCallback lcb, 
  ErrorCallback ecb);

   //I/Fが使用可能になったときの処理関数
 virtual void NotifyInterfaceUp (uint32_t interface);
 
   //I/Fが使用不可能になったときの処理関数
 virtual void NotifyInterfaceDown (uint32_t interface);

   //I/Fに新しいIPアドレスを指定したときの処理関数
 virtual void NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address);
 
   //I/FにIPアドレスを削除したときの処理関数
 virtual void NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address);

   //Ipv4のインスタンスを取得する関数
 virtual void SetIpv4 (Ptr<Ipv4> ipv4);

   //ルーティングテーブルを出力する関数
 virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

   //乱数系列のストリーム番号を割り当てる関数
 int64_t AssignStreams (int64_t stream);

 /**************IARP用********************/
  // Methods to handle protocol parameters
  /**
   * Set enable buffer flag
   * \param f The enable buffer flag
   */
  void SetEnableBufferFlag (bool f);
  /**
   * Get enable buffer flag
   * \returns the enable buffer flag
   */
  bool GetEnableBufferFlag () const;
  /**
   * Set weighted settling time (WST) flag
   * \param f the weighted settling time (WST) flag
   */
  void SetWSTFlag (bool f);
  /**
   * Get weighted settling time (WST) flag
   * \returns the weighted settling time (WST) flag
   */
  bool GetWSTFlag () const;
  /**
   * Set enable route aggregation (RA) flag
   * \param f the enable route aggregation (RA) flag
   */
  void SetEnableRAFlag (bool f);
  /**
   * Get enable route aggregation (RA) flag
   * \returns the enable route aggregation (RA) flag
   */
  bool GetEnableRAFlag () const;

  private:
   //経路更新の時間間隔
 Time UpdateInterval;

   //IPプロトコル
 Ptr<Ipv4> m_ipv4;

  /// Nodes IP address
  Ipv4Address m_mainAddress;

   //一つのIPインターフェースに一つのソケットを生成する。 
   //(socket, I/F address (IP + mask))のマップを定義する。
 std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;

  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketSubnetBroadcastAddresses;

   //ルーティングテーブルマネジャー
 RoutingTable m_routingTablemagr;

  /// waits for 24s since the last update to flush this route from its routing table.
  uint32_t Holdtimes;
  /// PeriodicUpdateInterval specifies the periodic time interval between which the a node broadcasts
  /// its entire routing table.
  Time m_periodicUpdateInterval;
  /// SettlingTime specifies the time for which a node waits before propagating an update.
  /// It waits for this time interval in hope of receiving an update with a better metric.
  Time m_settlingTime;
  /// Loopback device used to defer route requests until a route is found
  Ptr<NetDevice> m_lo;
  /// Main Routing table for the node
  RoutingTable m_routingTable;
  /// Advertised Routing table for the node
  RoutingTable m_advRoutingTable;
  /// The maximum number of packets that we allow a routing protocol to buffer.
  uint32_t m_maxQueueLen;
  /// The maximum number of packets that we allow per destination to buffer.
  uint32_t m_maxQueuedPacketsPerDst;
  /// The maximum period of time that a routing protocol is allowed to buffer a packet for.
  Time m_maxQueueTime;
  /// A "drop front on full" queue used by the routing layer to buffer packets to which it does not have a route.
  PacketQueue m_queue;
  /// Flag that is used to enable or disable buffering
  bool EnableBuffering;
  /// Flag that is used to enable or disable Weighted Settling Time
  bool EnableWST;
  /// This is the wighted factor to determine the weighted settling time
  double m_weightedFactor;
  /// This is a flag to enable route aggregation. Route aggregation will aggregate all routes for
  /// 'RouteAggregationTime' from the time an update is received by a node and sends them as a single update .
  bool EnableRouteAggregation;
  /// Parameter that holds the route aggregation time interval
  Time m_routeAggregationTime;
  /// Unicast callback for own packets
  UnicastForwardCallback m_scb;
  /// Error callback for own packets
  ErrorCallback m_ecb;

/***********************IERP******************************/
  /// Routing table
  RoutingTable2 m_routingTable2;

  /// Handle neighbors
  Neighbors m_nb;

// Protocol parameters.
  uint32_t m_rreqRetries;             ///< Maximum number of retransmissions of RREQ with TTL = NetDiameter to discover

  uint16_t m_rreqRateLimit;           ///< Maximum number of RREQ per second.

  /// Number of RREQs used for RREQ rate control
  uint16_t m_rreqCount;

  /// Request sequence number
  uint32_t m_seqNo;

  /// Broadcast ID
  uint32_t m_requestId;

  /// Handle duplicated RREQ
  IdCache m_rreqIdCache;

  uint16_t m_ttlStart;                ///< Initial TTL value for RREQ.

  uint16_t m_ttlIncrement;            ///< TTL increment for each attempt using the expanding ring search for RREQ dissemination.

  uint16_t m_ttlThreshold;            ///< Maximum TTL value for expanding ring search, TTL = NetDiameter is used beyond this value.

  uint32_t m_netDiameter;             ///< Net diameter measures the maximum possible number of hops between two nodes in the network
  uint16_t m_timeoutBuffer;           ///< Provide a buffer for the timeout.

  Time m_nodeTraversalTime;

  Time m_netTraversalTime;             ///< Estimate of the average net traversal time.

  Time m_pathDiscoveryTime;            ///< Estimate of maximum time needed to find route in network.

  Time m_activeRouteTimeout;          ///< Period of time during which the route is considered to be valid.

  Time m_myRouteTimeout;               ///< Value of lifetime field in RREP generating by this node.

  Time m_nextHopWait;                  ///< Period of our waiting for the neighbour's RREP_ACK
  Time m_blackListTimeout;             ///< Time for which the node is put into the blacklist

  bool m_destinationOnly;              ///< Indicates only the destination may respond to this RREQ.

  bool m_gratuitousReply;              ///< Indicates whether a gratuitous RREP should be unicast to the node

  /// Map IP address + RREQ timer.
  std::map<Ipv4Address, Timer> m_addressReqTimer;

  RequestQueue m_queue2;



  private:
  void
  Start ();
  /**
   * Queue packet until we find a route
   * \param p the packet to route
   * \param header the Ipv4Header
   * \param ucb the UnicastForwardCallback function
   * \param ecb the ErrorCallback function
   */
  void
  DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /// Look for any queued packets to send them out
  void
  LookForQueuedPackets (void);
  /**
   * Send packet from queue
   * \param dst - destination address to which we are sending the packet to
   * \param route - route identified for this packet
   */

  /**
   * If route exists and is valid, forward packet.
   *
   * \param p the packet to route
   * \param header the IP header
   * \param ucb the UnicastForwardCallback function
   * \param ecb the ErrorCallback function
   * \returns true if forwarded
   */ 
  bool Forwarding (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /**
   * Set lifetime field in routing table entry to the maximum of existing lifetime and lt, if the entry exists
   * \param addr - destination address
   * \param lt - proposed time for lifetime field in routing table entry for destination with address addr.
   * \return true if route to destination address addr exist
   */
  bool UpdateRouteLifeTime (Ipv4Address addr, Time lt);

  void
  SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route);
  /**
   * Send packet from queue
   * \param dst - destination address to which we are sending the packet to
   * \param route - route identified for this packet
   */
  void
  SendPacketFromQueue2 (Ipv4Address dst, Ptr<Ipv4Route> route);
  /**
   * Find socket with local interface address iface
   * \param iface the interface
   * \returns the socket
   */
  Ptr<Socket>
  FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;

  /**
   * Test whether the provided address is assigned to an interface on this node
   * \param src the source IP address
   * \returns true if the IP address is the node's IP address
   */
  bool IsMyOwnAddress (Ipv4Address src);

  /// Receive and process control packet
  void RecvShingo (Ptr<Socket> socket);

  // Receive dsdv control packets
  /**
   * Receive and process dsdv control packet
   * \param socket the socket for receiving dsdv control packets
   */
  void
  RecvIarp (Ptr<Packet> packet, Ipv4Address receiver, Ipv4Address sender);

  /// Receive RREQ
  void RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);

  /// Receive RREP
  void RecvReply (Ptr<Packet> p, Ipv4Address my,Ipv4Address src);
  /// Receive RREP_ACK
  void RecvReplyAck (Ipv4Address neighbor);

  /// Send packet
  void
  Send (Ptr<Ipv4Route>, Ptr<const Packet>, const Ipv4Header &);

  /// Send RREQ
  void SendRequest (Ipv4Address dst);

/// Send RREP
  void SendReply (RreqHeader const & rreqHeader, RoutingTableEntry2 const & toOrigin);
  /** Send RREP by intermediate node
   * \param toDst routing table entry to destination
   * \param toOrigin routing table entry to originator
   * \param gratRep indicates whether a gratuitous RREP should be unicast to destination
   */
  void SendReplyByIntermediateNode (RoutingTableEntry2 & toDst, RoutingTableEntry2 & toOrigin, bool gratRep);
  
  /// Send RREP_ACK
  void SendReplyAck (Ipv4Address neighbor);

  /**
   * Send packet to destination scoket
   * \param socket - destination node socket
   * \param packet - packet to send
   * \param destination - destination node IP address
   */
  /**
   * Repeated attempts by a source node at route discovery for a single destination
   * use the expanding ring search technique.
   * \param dst the destination IP address
   */
  void ScheduleRreqRetry (Ipv4Address dst);

  /**
   * Handle route discovery process
   * \param dst the destination IP address
   */
  void RouteRequestTimerExpire (Ipv4Address dst);

  void SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
  /**
   * Create loopback route for given header
   *
   * \param header the IP header
   * \param oif the device
   * \returns the route
   */
  Ptr<Ipv4Route>
  LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const;
  /**
   * Get settlingTime for a destination
   * \param dst - destination address
   * \return settlingTime for the destination if found
   */
  Time
  GetSettlingTime (Ipv4Address dst);
  /// Sends trigger update from a node
  void
  SendTriggeredUpdate ();
  /// Broadcasts the entire routing table for every PeriodicUpdateInterval
  void
  SendPeriodicUpdate ();
  /// Merge periodic updates
  void
  MergeTriggerPeriodicUpdates ();
  /// Notify that packet is dropped for some reason
  void
  Drop (Ptr<const Packet>, const Ipv4Header &, Socket::SocketErrno);

  /// RREQ rate limit timer
  Timer m_rreqRateLimitTimer;
  /// Reset RREQ count and schedule RREQ rate limit timer with delay 1 sec.
  void RreqRateLimitTimerExpire ();

  /**
   * Mark link to neighbor node as unidirectional for blacklistTimeout
   *
   * \param neighbor the IP address of the neightbor node
   * \param blacklistTimeout the black list timeout time
   */
  void AckTimerExpire (Ipv4Address neighbor, Time blacklistTimeout);

  /// Timer to trigger periodic updates from a node
  Timer m_periodicUpdateTimer;
  /// Timer used by the trigger updates in case of Weighted Settling Time is used
  Timer m_triggeredExpireTimer;

  /// Keep track of the last bcast time
  Time m_lastBcastTime;

  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

}
}
#endif /* SHINGO_H */

