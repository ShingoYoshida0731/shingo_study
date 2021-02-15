#include "shingo-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"

namespace ns3
{
namespace shingo
{

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t)
  : m_type (t),
    m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shingo::TypeHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shingo")
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case SHINGO_IARP:
    case SHINGO_RREQ:
    case SHINGO_RREP:
    case SHINGO_RREP_ACK:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  //printf("%d \n", dist);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case SHINGO_IARP:
      {
        os << "IARP";
        break;
      }
    case SHINGO_RREQ:
      {
        os << "RREQ";
        break;
      }
    case SHINGO_RREP:
      {
        os << "RREP";
        break;
      }
    case SHINGO_RREP_ACK:
      {
        os << "RREP_ACK";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}


NS_OBJECT_ENSURE_REGISTERED (IarpHeader);

IarpHeader::IarpHeader (Ipv4Address dst, uint32_t hopCount, uint32_t dstSeqNo)
  : m_dst (dst),
    m_hopCount (hopCount),
    m_dstSeqNo (dstSeqNo)
{
}

IarpHeader::~IarpHeader ()
{
}

TypeId
IarpHeader::GetTypeId () //パケットヘッダのタイプIDを取得する
{
 static TypeId tid = TypeId ("ns3::shingo::IarpHeader")
  .SetParent<Header> ()
  .SetGroupName ("Iarp")
  .AddConstructor<IarpHeader> ()
 ;
 return tid;
}

TypeId
IarpHeader::GetInstanceTypeId () const //タイプIDのインスタンスを取得する
{
 return GetTypeId ();
}

uint32_t
IarpHeader::GetSerializedSize () const //パケットタイプヘッダのカプセル化サイズを取得する
{
 return 12;
}

void
IarpHeader::Serialize (Buffer::Iterator i) const //パケットタイプヘッダのカプセル化処理
{
  WriteTo (i, m_dst);
  i.WriteHtonU32 (m_hopCount);
  i.WriteHtonU32 (m_dstSeqNo);

}

uint32_t
IarpHeader::Deserialize (Buffer::Iterator start) //パケットタイプデヘッダのカプセル化処理
{
  Buffer::Iterator i = start;

  ReadFrom (i, m_dst);
  m_hopCount = i.ReadNtohU32 ();
  m_dstSeqNo = i.ReadNtohU32 ();

  uint32_t dist = i.GetDistanceFrom (start);
  //printf("%d \n", dist);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
IarpHeader::Print (std::ostream &os) const
{
  os << "DestinationIpv4: " << m_dst
     << " Hopcount: " << m_hopCount
     << " SequenceNumber: " << m_dstSeqNo;
}

std::ostream &
operator<< (std::ostream & os, IarpHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// RREQ
//-----------------------------------------------------------------------------
RreqHeader::RreqHeader (uint8_t flags, uint8_t reserved, uint8_t hopCount, uint32_t requestID, Ipv4Address dst,
                        uint32_t dstSeqNo, Ipv4Address origin, uint32_t originSeqNo/*, Ipv4Address rad*/)
  : m_flags (flags),
    m_reserved (reserved),
    m_hopCount (hopCount),
    m_requestID (requestID),
    m_dst (dst),
    m_dstSeqNo (dstSeqNo),
    m_origin (origin),
    m_originSeqNo (originSeqNo)/*,
    m_rad(rad)*/
{
}

NS_OBJECT_ENSURE_REGISTERED (RreqHeader);

TypeId
RreqHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shingo::RreqHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shingo")
    .AddConstructor<RreqHeader> ()
  ;
  return tid;
}

TypeId
RreqHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
RreqHeader::GetSerializedSize () const
{
  return 23;
}

void
RreqHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_flags);
  i.WriteU8 (m_reserved);
  i.WriteU8 (m_hopCount);
  i.WriteHtonU32 (m_requestID);
  WriteTo (i, m_dst);
  i.WriteHtonU32 (m_dstSeqNo);
  WriteTo (i, m_origin);
  i.WriteHtonU32 (m_originSeqNo);
  //WriteTo (i, m_rad);
}

uint32_t
RreqHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_flags = i.ReadU8 ();
  m_reserved = i.ReadU8 ();
  m_hopCount = i.ReadU8 ();
  m_requestID = i.ReadNtohU32 ();
  ReadFrom (i, m_dst);
  m_dstSeqNo = i.ReadNtohU32 ();
  ReadFrom (i, m_origin);
  m_originSeqNo = i.ReadNtohU32 ();
  //ReadFrom (i, m_rad);

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
RreqHeader::Print (std::ostream &os) const
{
  os << "RREQ ID " << m_requestID << " destination: ipv4 " << m_dst
     << " sequence number " << m_dstSeqNo << " source: ipv4 "
     << m_origin << " sequence number " << m_originSeqNo
     << " flags:" << " Gratuitous RREP " << (*this).GetGratuitousRrep ()
     << " Destination only " << (*this).GetDestinationOnly ()
     << " Unknown sequence number " << (*this).GetUnknownSeqno ();
}

std::ostream &
operator<< (std::ostream & os, RreqHeader const & h)
{
  h.Print (os);
  return os;
}

void
RreqHeader::SetGratuitousRrep (bool f)
{
  if (f)
    {
      m_flags |= (1 << 5);
    }
  else
    {
      m_flags &= ~(1 << 5);
    }
}

bool
RreqHeader::GetGratuitousRrep () const
{
  return (m_flags & (1 << 5));
}

void
RreqHeader::SetDestinationOnly (bool f)
{
  if (f)
    {
      m_flags |= (1 << 4);
    }
  else
    {
      m_flags &= ~(1 << 4);
    }
}

bool
RreqHeader::GetDestinationOnly () const
{
  return (m_flags & (1 << 4));
}

void
RreqHeader::SetUnknownSeqno (bool f)
{
  if (f)
    {
      m_flags |= (1 << 3);
    }
  else
    {
      m_flags &= ~(1 << 3);
    }
}

bool
RreqHeader::GetUnknownSeqno () const
{
  return (m_flags & (1 << 3));
}

bool
RreqHeader::operator== (RreqHeader const & o) const
{
  return (m_flags == o.m_flags && m_reserved == o.m_reserved
          && m_hopCount == o.m_hopCount && m_requestID == o.m_requestID
          && m_dst == o.m_dst && m_dstSeqNo == o.m_dstSeqNo
          && m_origin == o.m_origin && m_originSeqNo == o.m_originSeqNo /*&& m_rad == o.m_rad*/);
}


//-----------------------------------------------------------------------------
// RREP
//-----------------------------------------------------------------------------

RrepHeader::RrepHeader (uint8_t prefixSize, uint8_t hopCount, Ipv4Address dst,
                        uint32_t dstSeqNo, Ipv4Address origin, Time lifeTime)
  : m_flags (0),
    m_prefixSize (prefixSize),
    m_hopCount (hopCount),
    m_dst (dst),
    m_dstSeqNo (dstSeqNo),
    m_origin (origin)
{
  m_lifeTime = uint32_t (lifeTime.GetMilliSeconds ());
}

NS_OBJECT_ENSURE_REGISTERED (RrepHeader);

TypeId
RrepHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shingo::RrepHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shingo")
    .AddConstructor<RrepHeader> ()
  ;
  return tid;
}

TypeId
RrepHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
RrepHeader::GetSerializedSize () const
{
  return 19;
}

void
RrepHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_flags);
  i.WriteU8 (m_prefixSize);
  i.WriteU8 (m_hopCount);
  WriteTo (i, m_dst);
  i.WriteHtonU32 (m_dstSeqNo);
  WriteTo (i, m_origin);
  i.WriteHtonU32 (m_lifeTime);
}

uint32_t
RrepHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_flags = i.ReadU8 ();
  m_prefixSize = i.ReadU8 ();
  m_hopCount = i.ReadU8 ();
  ReadFrom (i, m_dst);
  m_dstSeqNo = i.ReadNtohU32 ();
  ReadFrom (i, m_origin);
  m_lifeTime = i.ReadNtohU32 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
RrepHeader::Print (std::ostream &os) const
{
  os << "destination: ipv4 " << m_dst << " sequence number " << m_dstSeqNo;
  if (m_prefixSize != 0)
    {
      os << " prefix size " << m_prefixSize;
    }
  os << " source ipv4 " << m_origin << " lifetime " << m_lifeTime
     << " acknowledgment required flag " << (*this).GetAckRequired ();
}

void
RrepHeader::SetLifeTime (Time t)
{
  m_lifeTime = t.GetMilliSeconds ();
}

Time
RrepHeader::GetLifeTime () const
{
  Time t (MilliSeconds (m_lifeTime));
  return t;
}

void
RrepHeader::SetAckRequired (bool f)
{
  if (f)
    {
      m_flags |= (1 << 6);
    }
  else
    {
      m_flags &= ~(1 << 6);
    }
}

bool
RrepHeader::GetAckRequired () const
{
  return (m_flags & (1 << 6));
}

void
RrepHeader::SetPrefixSize (uint8_t sz)
{
  m_prefixSize = sz;
}

uint8_t
RrepHeader::GetPrefixSize () const
{
  return m_prefixSize;
}

bool
RrepHeader::operator== (RrepHeader const & o) const
{
  return (m_flags == o.m_flags && m_prefixSize == o.m_prefixSize
          && m_hopCount == o.m_hopCount && m_dst == o.m_dst && m_dstSeqNo == o.m_dstSeqNo
          && m_origin == o.m_origin && m_lifeTime == o.m_lifeTime);
}

void
RrepHeader::SetHello (Ipv4Address origin, uint32_t srcSeqNo, Time lifetime)
{
  m_flags = 0;
  m_prefixSize = 0;
  m_hopCount = 0;
  m_dst = origin;
  m_dstSeqNo = srcSeqNo;
  m_origin = origin;
  m_lifeTime = lifetime.GetMilliSeconds ();
}

std::ostream &
operator<< (std::ostream & os, RrepHeader const & h)
{
  h.Print (os);
  return os;
}


RrepAckHeader::RrepAckHeader ()
  : m_reserved (0)
{
}

NS_OBJECT_ENSURE_REGISTERED (RrepAckHeader);

TypeId
RrepAckHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::shingo::RrepAckHeader")
    .SetParent<Header> ()
    .SetGroupName ("Shingo")
    .AddConstructor<RrepAckHeader> ()
  ;
  return tid;
}

TypeId
RrepAckHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
RrepAckHeader::GetSerializedSize () const
{
  return 1;
}

void
RrepAckHeader::Serialize (Buffer::Iterator i ) const
{
  i.WriteU8 (m_reserved);
}

uint32_t
RrepAckHeader::Deserialize (Buffer::Iterator start )
{
  Buffer::Iterator i = start;
  m_reserved = i.ReadU8 ();
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
RrepAckHeader::Print (std::ostream &os ) const
{
}

bool
RrepAckHeader::operator== (RrepAckHeader const & o ) const
{
  return m_reserved == o.m_reserved;
}

std::ostream &
operator<< (std::ostream & os, RrepAckHeader const & h )
{
  h.Print (os);
  return os;
}



}
}
