#ifndef SHINGO_TABLE_H
#define SHINGO_TABLE_H

#include <stdint.h>
#include <cassert>
#include <map>
#include <sys/types.h>
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/timer.h"
#include "ns3/net-device.h"
#include "ns3/output-stream-wrapper.h"

#include "shingo-packet.h"
#include "shingo-queue.h"

namespace ns3 {
namespace shingo {
enum RouteFlags
{
  VALID = 0,     // !< VALID
  INVALID = 1,     // !< INVALID
  IN_SEARCH = 2,      //!< IN_SEARCH
  DISCOVER = 3
};


//経路エントリークラスの定義
class RoutingTableEntry
{
 public:
   //コンストラクタ
 RoutingTableEntry (Ptr<NetDevice> dev = 0, Ipv4Address dst = Ipv4Address (), uint32_t seqNo = 0,
                     Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (), uint32_t hops = 0, Ipv4Address nextHop = Ipv4Address (),
                     Time lifetime = Simulator::Now (), Time SettlingTime = Simulator::Now (), bool changedEntries = false);

  ~RoutingTableEntry ();

   //あて先IPアドレスの取得
 Ipv4Address GetDestination () const { return m_ipv4Route->GetDestination (); }
 
   //経路情報の取得
 Ptr<Ipv4Route>  GetRoute () const { return m_ipv4Route; }
 void SetRoute (Ptr<Ipv4Route> route) { m_ipv4Route = route; }

   //ネクストホップを設定する
 void SetNextHop (Ipv4Address nextHop) { m_ipv4Route->SetGateway (nextHop); }
   //ネクストホップを取得する
 Ipv4Address GetNextHop () const { return m_ipv4Route->GetGateway (); }

   //出力デバイスを設定する
 void SetOutputDevice (Ptr<NetDevice> device) { m_ipv4Route->SetOutputDevice (device); }
   //出力デバイスを取得する
 Ptr<NetDevice> GetOutputDevice () const { 
	return m_ipv4Route->GetOutputDevice (); 
	}

   //インターフェースアドレスを設定する
 void SetInterface (Ipv4InterfaceAddress iface) { m_iface = iface; }
   //インターフェースアドレスを取得する
 Ipv4InterfaceAddress GetInterface () const { return m_iface; }

  /**
   * Set sequence number
   * \param sequenceNumber the sequence number
   */
  void
  SetSeqNo (uint32_t sequenceNumber)
  {
    m_seqNo = sequenceNumber;
  }
  /**
   * Get sequence number
   * \returns the sequence number
   */
  uint32_t
  GetSeqNo () const
  {
    return m_seqNo;
  }

  void
  SetHop (uint32_t hopCount)
  {
    m_hops = hopCount;
  }
  /**
   * Get hop
   * \returns the hop count
   */
  uint32_t
  GetHop () const
  {
    return m_hops;
  }

   //経路エントリの維持時間を設定する
 void SetLifeTime (Time lifeTime) { m_lifeTime = lifeTime + Simulator::Now (); }
   //経路エントリの維持時間を取得する 
 Time GetLifeTime () const { return m_lifeTime - Simulator::Now (); }

  /**
   * Set settling time
   * \param settlingTime the settling time
   */
  void
  SetSettlingTime (Time settlingTime)
  {
    m_settlingTime = settlingTime;
  }
  /**
   * Get settling time
   * \returns the settling time
   */
  Time
  GetSettlingTime () const
  {
    return (m_settlingTime);
  }
  /**
   * Set route flags
   * \param flag the route flags
   */
  void
  SetFlag (RouteFlags flag)
  {
    m_flag = flag;
  }
  /**
   * Get route flags
   * \returns the route flags
   */
  RouteFlags
  GetFlag () const
  {
    return m_flag;
  }
  /**
   * Set entries changed indicator
   * \param entriesChanged
   */
  void
  SetEntriesChanged (bool entriesChanged)
  {
    m_entriesChanged = entriesChanged;
  }
  /**
   * Get entries changed
   * \returns the entries changed flag
   */
  bool
  GetEntriesChanged () const
  {
    return m_entriesChanged;
  }

 bool operator== (Ipv4Address const  destination) const {
  return (m_ipv4Route->GetDestination () == destination);
 }

  void
  Print (Ptr<OutputStreamWrapper> stream) const;
 
 private:
  /// Destination Sequence Number
  uint32_t m_seqNo;

  uint16_t m_hops;
   //経路エントリーの維持時間
 Time m_lifeTime;
   //経路情報
 Ptr<Ipv4Route> m_ipv4Route;

   //出力I/Fアドレス
 Ipv4InterfaceAddress m_iface;




  /// Routing flags: valid, invalid or in search
  RouteFlags m_flag;
  /// Time for which the node retains an update with changed metric before broadcasting it.
  /// A node does that in hope of receiving a better update.
  Time m_settlingTime;
  /// Flag to show if any of the routing table entries were changed with the routing update.
  uint32_t m_entriesChanged;

};

class RoutingTable
{
 public:
 RoutingTable ();

   //ルーティングテーブルに経路エントリーを追加する
 bool AddRoute (RoutingTableEntry & r);
   //ルーティングテーブルから経路エントリーを削除する
 bool DeleteRoute (Ipv4Address dst);

   //ルーティングテーブルからあて先dstの経路エントリーを検索する
 bool LookupRoute (Ipv4Address dst, RoutingTableEntry & rt);

  bool
  LookupRoute (Ipv4Address id, RoutingTableEntry & rt, bool forRouteInput);

   //ルーティングテーブルを更新する
 bool Update (RoutingTableEntry & rt);
  /**
   * Lookup list of addresses for which nxtHp is the next Hop address
   * \param nxtHp nexthop's address for which we want the list of destinations
   * \param dstList is the list that will hold all these destination addresses
   */
  void
  GetListOfDestinationWithNextHop (Ipv4Address nxtHp, std::map<Ipv4Address, RoutingTableEntry> & dstList);
  /**
   * Lookup list of all addresses in the routing table
   * \param allRoutes is the list that will hold all these addresses present in the nodes routing table
   */
  void
  GetListOfAllRoutes (std::map<Ipv4Address, RoutingTableEntry> & allRoutes);
  /**
   * Delete all route from interface with address iface
   * \param iface the interface
   */
  void
  DeleteAllRoutesFromInterface (Ipv4InterfaceAddress iface);

   //ルーティングテーブルのすべてのエントリーを削除する
 void Clear () { m_ipv4AddressEntry.clear (); }

   //ルーティングテーブルをファイルに出力する
 void Print (Ptr<OutputStreamWrapper> stream) const;

  /**
   * Delete all outdated entries if Lifetime is expired
   * \param removedAddresses is the list of addresses to purge
   */
  void
  Purge (std::map<Ipv4Address, RoutingTableEntry> & removedAddresses);
  uint32_t
  RoutingTableSize ();
  /**
  * Add an event for a destination address so that the update to for that destination is sent
  * after the event is completed.
  * \param address destination address for which this event is running.
  * \param id unique eventid that was generated.
  * \return true on success
  */
  bool
  AddIpv4Event (Ipv4Address address, EventId id);
  /**
  * Clear up the entry from the map after the event is completed
  * \param address destination address for which this event is running.
  * \return true on success
  */
  bool
  DeleteIpv4Event (Ipv4Address address);
  /**
  * Force delete an update waiting for settling time to complete as a better update to
  * same destination was received.
  * \param address destination address for which this event is running.
  * \return true on success
  */
  bool
  AnyRunningEvent (Ipv4Address address);

  /**
  * Force delete an update waiting for settling time to complete as a better update to
  * same destination was received.
  * \param address destination address for which this event is running.
  * \return true on finding out that an event is already running for that destination address.
  */
  bool
  ForceDeleteIpv4Event (Ipv4Address address);
  /**
    * Get the EcentId associated with that address.
    * \param address destination address for which this event is running.
    * \return EventId on finding out an event is associated else return NULL.
    */
  EventId
  GetEventId (Ipv4Address address);

  /**
   * Get hold down time (time until an invalid route may be deleted)
   * \returns the hold down time
   */
  Time Getholddowntime () const
  {
    return m_holddownTime;
  }
  /**
   * Set hold down time (time until an invalid route may be deleted)
   * \param t the hold down time
   */
  void Setholddowntime (Time t)
  {
    m_holddownTime = t;
  }


 private:
   //ルーティングテーブル
  // Fields
  /// an entry in the routing table.
  std::map<Ipv4Address, RoutingTableEntry> m_ipv4AddressEntry;
  /// an entry in the event table.
  std::map<Ipv4Address, EventId> m_ipv4Events;
  /// hold down time of an expired route
  Time m_holddownTime;

};


/**
 * \ingroup aodv
 * \brief Routing table entry
 */
class RoutingTableEntry2
{
public:
  /**
   * constructor
   *
   * \param dev the device
   * \param dst the destination IP address
   * \param vSeqNo verify sequence number flag
   * \param seqNo the sequence number
   * \param iface the interface
   * \param hops the number of hops
   * \param nextHop the IP address of the next hop
   * \param lifetime the lifetime of the entry
   */
  RoutingTableEntry2 (Ptr<NetDevice> dev = 0,Ipv4Address dst = Ipv4Address (), bool vSeqNo = false, uint32_t seqNo = 0,
                     Ipv4InterfaceAddress iface = Ipv4InterfaceAddress (), uint16_t  hops = 0,
                     Ipv4Address nextHop = Ipv4Address (), Time lifetime = Simulator::Now ());

  ~RoutingTableEntry2 ();

  ///\name Precursors management
  //\{
  /**
   * Insert precursor in precursor list if it doesn't yet exist in the list
   * \param id precursor address
   * \return true on success
   */
  bool InsertPrecursor (Ipv4Address id);
  /**
   * Lookup precursor by address
   * \param id precursor address
   * \return true on success
   */
  bool LookupPrecursor (Ipv4Address id);
  /**
   * \brief Delete precursor
   * \param id precursor address
   * \return true on success
   */
  bool DeletePrecursor (Ipv4Address id);
  /// Delete all precursors
  void DeleteAllPrecursors ();
  /**
   * Check that precursor list is empty
   * \return true if precursor list is empty
   */
  bool IsPrecursorListEmpty () const;
  /**
   * Inserts precursors in output parameter prec if they do not yet exist in vector
   * \param prec vector of precursor addresses
   */
  void GetPrecursors (std::vector<Ipv4Address> & prec) const;
  //\}

  /**
   * Mark entry as "down" (i.e. disable it)
   * \param badLinkLifetime duration to keep entry marked as invalid
   */
  void Invalidate (Time badLinkLifetime);

  // Fields
  /**
   * Get destination address function
   * \returns the IPv4 destination address
   */
  Ipv4Address GetDestination () const
  {
    return m_ipv4Route->GetDestination ();
  }
  /**
   * Get route function
   * \returns The IPv4 route
   */
  Ptr<Ipv4Route> GetRoute () const
  {
    return m_ipv4Route;
  }
  /**
   * Set route function
   * \param r the IPv4 route
   */
  void SetRoute (Ptr<Ipv4Route> r)
  {
    m_ipv4Route = r;
  }
  /**
   * Set next hop address
   * \param nextHop the next hop IPv4 address
   */
  void SetNextHop (Ipv4Address nextHop)
  {
    m_ipv4Route->SetGateway (nextHop);
  }
  /**
   * Get next hop address
   * \returns the next hop address
   */
  Ipv4Address GetNextHop () const
  {
    return m_ipv4Route->GetGateway ();
  }
  /**
   * Set output device
   * \param dev The output device
   */
  void SetOutputDevice (Ptr<NetDevice> dev)
  {
    m_ipv4Route->SetOutputDevice (dev);
  }
  /**
   * Get output device
   * \returns the output device
   */
  Ptr<NetDevice> GetOutputDevice () const
  {
    return m_ipv4Route->GetOutputDevice ();
  }
  /**
   * Get the Ipv4InterfaceAddress
   * \returns the Ipv4InterfaceAddress
   */
  Ipv4InterfaceAddress GetInterface () const
  {
    return m_iface;
  }
  /**
   * Set the Ipv4InterfaceAddress
   * \param iface The Ipv4InterfaceAddress
   */
  void SetInterface (Ipv4InterfaceAddress iface)
  {
    m_iface = iface;
  }
  /**
   * Set the valid sequence number
   * \param s the sequence number
   */
  void SetValidSeqNo (bool s)
  {
    m_validSeqNo = s;
  }
  /**
   * Get the valid sequence number
   * \returns the valid sequence number
   */
  bool GetValidSeqNo () const
  {
    return m_validSeqNo;
  }
  /**
   * Set the sequence number
   * \param sn the sequence number
   */
  void SetSeqNo (uint32_t sn)
  {
    m_seqNo = sn;
  }
  /**
   * Get the sequence number
   * \returns the sequence number
   */
  uint32_t GetSeqNo () const
  {
    return m_seqNo;
  }
  /**
   * Set the number of hops
   * \param hop the number of hops
   */
  void SetHop (uint16_t hop)
  {
    m_hops = hop;
  }
  /**
   * Get the number of hops
   * \returns the number of hops
   */
  uint16_t GetHop () const
  {
    return m_hops;
  }
  /**
   * Set the lifetime
   * \param lt The lifetime
   */
  void SetLifeTime (Time lt)
  {
    m_lifeTime = lt + Simulator::Now ();
  }
  /**
   * Get the lifetime
   * \returns the lifetime
   */
  Time GetLifeTime () const
  {
    return m_lifeTime - Simulator::Now ();
  }
  /**
   * Set the route flags
   * \param flag the route flags
   */
  void SetFlag (RouteFlags flag)
  {
    m_flag = flag;
  }
  /**
   * Get the route flags
   * \returns the route flags
   */
  RouteFlags GetFlag () const
  {
    return m_flag;
  }
  /**
   * Set the RREQ count
   * \param n the RREQ count
   */
  void SetRreqCnt (uint8_t n)
  {
    m_reqCount = n;
  }
  /**
   * Get the RREQ count
   * \returns the RREQ count
   */
  uint8_t GetRreqCnt () const
  {
    return m_reqCount;
  }
  /**
   * Increment the RREQ count
   */
  void IncrementRreqCnt ()
  {
    m_reqCount++;
  }
  /**
   * Set the unidirectional flag
   * \param u the uni directional flag
   */
  void SetUnidirectional (bool u)
  {
    m_blackListState = u;
  }
  /**
   * Get the unidirectional flag
   * \returns the unidirectional flag
   */
  bool IsUnidirectional () const
  {
    return m_blackListState;
  }
  /**
   * Set the blacklist timeout
   * \param t the blacklist timeout value
   */
  void SetBlacklistTimeout (Time t)
  {
    m_blackListTimeout = t;
  }
  /**
   * Get the blacklist timeout value
   * \returns the blacklist timeout value
   */
  Time GetBlacklistTimeout () const
  {
    return m_blackListTimeout;
  }
  /// RREP_ACK timer
  Timer m_ackTimer;

  /**
   * \brief Compare destination address
   * \param dst IP address to compare
   * \return true if equal
   */
  bool operator== (Ipv4Address const  dst) const
  {
    return (m_ipv4Route->GetDestination () == dst);
  }
  /**
   * Print packet to trace file
   * \param stream The output stream
   */
  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  /// Valid Destination Sequence Number flag
  bool m_validSeqNo;
  /// Destination Sequence Number, if m_validSeqNo = true
  uint32_t m_seqNo;
  /// Hop Count (number of hops needed to reach destination)
  uint16_t m_hops;
  /**
  * \brief Expiration or deletion time of the route
  *	Lifetime field in the routing table plays dual role:
  *	for an active route it is the expiration time, and for an invalid route
  *	it is the deletion time.
  */
  Time m_lifeTime;
  /** Ip route, include
   *   - destination address
   *   - source address
   *   - next hop address (gateway)
   *   - output device
   */
  Ptr<Ipv4Route> m_ipv4Route;
  /// Output interface address
  Ipv4InterfaceAddress m_iface;
  /// Routing flags: valid, invalid or in search
  RouteFlags m_flag;

  /// List of precursors
  std::vector<Ipv4Address> m_precursorList;
  /// When I can send another request
  Time m_routeRequestTimout;
  /// Number of route requests
  uint8_t m_reqCount;
  /// Indicate if this entry is in "blacklist"
  bool m_blackListState;
  /// Time for which the node is put into the blacklist
  Time m_blackListTimeout;
};

/**
 * \ingroup aodv
 * \brief The Routing table used by AODV protocol
 */
class RoutingTable2
{
public:
  /**
   * constructor
   * \param t the routing table entry lifetime
   */
  RoutingTable2 ();
//  RoutingTable2 (Time t);
  ///\name Handle lifetime of invalid route
  //\{
  Time GetBadLinkLifetime () const
  {
    return m_badLinkLifetime;
  }
  void SetBadLinkLifetime (Time t)
  {
    m_badLinkLifetime = t;
  }
  //\}
  /**
   * Add routing table entry if it doesn't yet exist in routing table
   * \param r routing table entry
   * \return true in success
   */
  bool AddRoute (RoutingTableEntry2 & r);
  /**
   * Delete routing table entry with destination address dst, if it exists.
   * \param dst destination address
   * \return true on success
   */
  bool DeleteRoute (Ipv4Address dst);
  /**
   * Lookup routing table entry with destination address dst
   * \param dst destination address
   * \param rt entry with destination address dst, if exists
   * \return true on success
   */
  bool LookupRoute (Ipv4Address dst, RoutingTableEntry2 & rt);
  /**
   * Lookup route in VALID state
   * \param dst destination address
   * \param rt entry with destination address dst, if exists
   * \return true on success
   */
  bool LookupValidRoute (Ipv4Address dst, RoutingTableEntry2 & rt);
  /**
   * Update routing table
   * \param rt entry with destination address dst, if exists
   * \return true on success
   */
  bool Update (RoutingTableEntry2 & rt);
  /**
   * Set routing table entry flags
   * \param dst destination address
   * \param state the routing flags
   * \return true on success
   */
  bool SetEntryState (Ipv4Address dst, RouteFlags state);
  /**
   * Lookup routing entries with next hop Address dst and not empty list of precursors.
   *
   * \param nextHop the next hop IP address
   * \param unreachable
   */
  void GetListOfDestinationWithNextHop (Ipv4Address nextHop, std::map<Ipv4Address, uint32_t> & unreachable);
  /**
   *   Update routing entries with this destination as follows:
   *  1. The destination sequence number of this routing entry, if it
   *     exists and is valid, is incremented.
   *  2. The entry is invalidated by marking the route entry as invalid
   *  3. The Lifetime field is updated to current time plus DELETE_PERIOD.
   *  \param unreachable routes to invalidate
   */
  void InvalidateRoutesWithDst (std::map<Ipv4Address, uint32_t> const & unreachable);
  /**
   * Delete all route from interface with address iface
   * \param iface the interface IP address
   */
  void DeleteAllRoutesFromInterface (Ipv4InterfaceAddress iface);
  /// Delete all entries from routing table
  void Clear ()
  {
    m_ipv4AddressEntry.clear ();
  }
  /// Delete all outdated entries and invalidate valid entry if Lifetime is expired
  void Purge ();
  /** Mark entry as unidirectional (e.g. add this neighbor to "blacklist" for blacklistTimeout period)
   * \param neighbor - neighbor address link to which assumed to be unidirectional
   * \param blacklistTimeout - time for which the neighboring node is put into the blacklist
   * \return true on success
   */
  bool MarkLinkAsUnidirectional (Ipv4Address neighbor, Time blacklistTimeout);
  /**
   * Print routing table
   * \param stream the output stream
   */
  void Print (Ptr<OutputStreamWrapper> stream) const;

private:
  /// The routing table
  std::map<Ipv4Address, RoutingTableEntry2> m_ipv4AddressEntry;
  /// Deletion time for invalid routes
  Time m_badLinkLifetime;
  /**
   * const version of Purge, for use by Print() method
   * \param table the routing table entry to purge
   */
  void Purge (std::map<Ipv4Address, RoutingTableEntry2> &table) const;
};

}
}

#endif /* SHINGO_TABLE_H */
