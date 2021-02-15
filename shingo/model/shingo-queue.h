
#ifndef SHINGO_QUEUE_H
#define SHINGO_QUEUE_H

#include <vector>
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace shingo {
/**
 * \ingroup iarp
 * \brief IARP Queue Entry
 */
class QueueEntry
{
public:
  /// Unicast forward call back function typedef
  typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;
  /// Error callback function typedef
  typedef Ipv4RoutingProtocol::ErrorCallback ErrorCallback;
  /**
   * c-tor
   *
   * \param pa the packet to create the entry
   * \param h the Ipv4Header
   * \param ucb the UnicastForwardCallback function
   * \param ecb the ErrorCallback function
   */
  QueueEntry (Ptr<const Packet> pa = 0, Ipv4Header const & h = Ipv4Header (),
              UnicastForwardCallback ucb = UnicastForwardCallback (),
              ErrorCallback ecb = ErrorCallback ())
    : m_packet (pa),
      m_header (h),
      m_ucb (ucb),
      m_ecb (ecb),
      m_expire (Seconds (0))
  {
  }

  /**
   * Compare queue entries
   * \param o QueueEntry to compare
   * \return true if equal
   */
  bool operator== (QueueEntry const & o) const
  {
    return ((m_packet == o.m_packet) && (m_header.GetDestination () == o.m_header.GetDestination ()) && (m_expire == o.m_expire));
  }

  // Fields
  /**
   * Get unicast forward callback function
   * \returns the unicast forward callback
   */
  UnicastForwardCallback GetUnicastForwardCallback () const
  {
    return m_ucb;
  }
  /**
   * Set unicast forward callback function
   * \param ucb the unicast forward callback
   */
  void SetUnicastForwardCallback (UnicastForwardCallback ucb)
  {
    m_ucb = ucb;
  }
  /**
   * Get error callback function
   * \returns the error callback
   */
  ErrorCallback GetErrorCallback () const
  {
    return m_ecb;
  }
  /**
   * Set error callback function
   * \param ecb the error callback
   */
  void SetErrorCallback (ErrorCallback ecb)
  {
    m_ecb = ecb;
  }
  /**
   * Get packet
   * \returns the current packet
   */
  Ptr<const Packet> GetPacket () const
  {
    return m_packet;
  }
  /**
   * Set packet
   * \param p The current packet
   */
  void SetPacket (Ptr<const Packet> p)
  {
    m_packet = p;
  }
  /**
   * Get IP header
   * \returns the IPv4 header
   */
  Ipv4Header GetIpv4Header () const
  {
    return m_header;
  }
  /**
   * Set IP header
   * \param h The IPv4 header
   */
  void SetIpv4Header (Ipv4Header h)
  {
    m_header = h;
  }
  /**
   * Set expire time
   * \param exp
   */
  void SetExpireTime (Time exp)
  {
    m_expire = exp + Simulator::Now ();
  }
  /**
   * Get expire time
   * \returns the expire time
   */
  Time GetExpireTime () const
  {
    return m_expire - Simulator::Now ();
  }

private:
  /// Data packet
  Ptr<const Packet> m_packet;
  /// IP header
  Ipv4Header m_header;
  /// Unicast forward callback
  UnicastForwardCallback m_ucb;
  /// Error callback
  ErrorCallback m_ecb;
  /// Expire time for queue entry
  Time m_expire;
};
/**
 * \ingroup iarp
 * \brief IARP Packet queue
 *
 * When a route is not available, the packets are queued. Every node can buffer up to 5 packets per
 * destination. We have implemented a "drop front on full" queue where the first queued packet will be dropped
 * to accommodate newer packets.
 */
class PacketQueue
{
public:
  /// Default c-tor
  PacketQueue ()
  {
  }
  /**
   * Push entry in queue, if there is no entry with the same packet and destination address in queue.
   * \param entry QueueEntry to compare
   * \return true if successful
   */
  bool Enqueue (QueueEntry & entry);
  /**
   * Return first found (the earliest) entry for given destination
   * 
   * \param dst the destination IP address
   * \param entry the queue entry
   * \returns true if successful
   */
  bool Dequeue (Ipv4Address dst, QueueEntry & entry);
  /**
   * Remove all packets with destination IP address dst
   * \param dst the destination IP address
   */
  void DropPacketWithDst (Ipv4Address dst);
  /**
   * Finds whether a packet with destination dst exists in the queue
   * \param dst the destination IP address
   * \returns true if a packet found
   */
  bool Find (Ipv4Address dst);
  /**
   * Get count of packets with destination dst in the queue
   * \param dst the destination IP address
   * \returns the count
   */
  uint32_t
  GetCountForPacketsWithDst (Ipv4Address dst);
  /**
   * Get the number of entries
   * \returns the number of entries
   */
  uint32_t GetSize ();

  // Fields
  /**
   * Get maximum queue length
   * \returns the maximum queue length
   */
  uint32_t GetMaxQueueLen () const
  {
    return m_maxLen;
  }
  /**
   * Set maximum queue length
   * \param len the maximum queue length
   */
  void SetMaxQueueLen (uint32_t len)
  {
    m_maxLen = len;
  }
  /**
   * Get maximum packets per destination
   * \returns the maximum packets per destination
   */
  uint32_t GetMaxPacketsPerDst () const
  {
    return m_maxLenPerDst;
  }
  /**
   * Set maximum packets per destination
   * \param len The maximum packets per destination
   */
  void SetMaxPacketsPerDst (uint32_t len)
  {
    m_maxLenPerDst = len;
  }
  /**
   * Get queue timeout
   * \returns the queue timeout
   */
  Time GetQueueTimeout () const
  {
    return m_queueTimeout;
  }
  /**
   * Set queue timeout
   * \param t The queue timeout
   */
  void SetQueueTimeout (Time t)
  {
    m_queueTimeout = t;
  }

private:
  std::vector<QueueEntry> m_queue; ///< the queue
  /// Remove all expired entries
  void Purge ();
  /**
   * Notify that the packet is dropped from queue due to timeout
   * \param en the queue entry
   * \param reason the reason for the packet drop
   */
  void Drop (QueueEntry en, std::string reason);
  /// The maximum number of packets that we allow a routing protocol to buffer.
  uint32_t m_maxLen;
  /// The maximum number of packets that we allow per destination to buffer.
  uint32_t m_maxLenPerDst;
  /// The maximum period of time that a routing protocol is allowed to buffer a packet for, seconds.
  Time m_queueTimeout;
  /**
   * Determine if queue entries are equal
   * \param en the queue entry
   * \param dst the IPv4 destination address
   * \returns true if the entry is for the destination address
   */
  static bool IsEqual (QueueEntry en, const Ipv4Address dst)
  {
    return (en.GetIpv4Header ().GetDestination () == dst);
  }
};

/**
 * \ingroup aodv
 * \brief AODV route request queue
 *
 * Since AODV is an on demand routing we queue requests while looking for route.
 */
class RequestQueue
{
public:
  /**
   * constructor
   *
   * \param maxLen the maximum length
   * \param routeToQueueTimeout the route to queue timeout
   */
  RequestQueue (uint32_t maxLen, Time routeToQueueTimeout)
    : m_maxLen (maxLen),
      m_queueTimeout (routeToQueueTimeout)
  {
  }
  /**
   * Push entry in queue, if there is no entry with the same packet and destination address in queue.
   * \param entry the queue entry
   * \returns true if the entry is queued
   */
  bool Enqueue (QueueEntry & entry);
  /**
   * Return first found (the earliest) entry for given destination
   * 
   * \param dst the destination IP address
   * \param entry the queue entry
   * \returns true if the entry is dequeued
   */
  bool Dequeue (Ipv4Address dst, QueueEntry & entry);
  /**
   * Remove all packets with destination IP address dst
   * \param dst the destination IP address
   */
  void DropPacketWithDst (Ipv4Address dst);
  /**
   * Finds whether a packet with destination dst exists in the queue
   * 
   * \param dst the destination IP address
   * \returns true if an entry with the IP address is found
   */
  bool Find (Ipv4Address dst);
  /**
   * \returns the number of entries
   */
  uint32_t GetSize ();

  // Fields
  /**
   * Get maximum queue length
   * \returns the maximum queue length
   */
  uint32_t GetMaxQueueLen () const
  {
    return m_maxLen;
  }
  /**
   * Set maximum queue length
   * \param len The maximum queue length
   */
  void SetMaxQueueLen (uint32_t len)
  {
    m_maxLen = len;
  }
  /**
   * Get queue timeout
   * \returns the queue timeout
   */
  Time GetQueueTimeout () const
  {
    return m_queueTimeout;
  }
  /**
   * Set queue timeout
   * \param t The queue timeout
   */
  void SetQueueTimeout (Time t)
  {
    m_queueTimeout = t;
  }

private:
  /// The queue
  std::vector<QueueEntry> m_queue;
  /// Remove all expired entries
  void Purge ();
  /**
   * Notify that packet is dropped from queue by timeout
   * \param en the queue entry to drop
   * \param reason the reason to drop the entry
   */
  void Drop (QueueEntry en, std::string reason);
  /// The maximum number of packets that we allow a routing protocol to buffer.
  uint32_t m_maxLen;
  /// The maximum period of time that a routing protocol is allowed to buffer a packet for, seconds.
  Time m_queueTimeout;
  /**
   * Determine if queue matches a destination address
   * \param en The queue entry
   * \param dst The destination IPv4 address
   * \returns true if the queue entry matches the destination address
   */
  static bool IsEqual (QueueEntry en, const Ipv4Address dst)
  {
    return (en.GetIpv4Header ().GetDestination () == dst);
  }
};


}
}
#endif /* SHINGO_QUEUE_H */
