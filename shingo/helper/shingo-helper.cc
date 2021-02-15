/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "shingo-helper.h"
#include "ns3/shingo.h"

#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3
{

ShingoHelper::ShingoHelper() : Ipv4RoutingHelper ()
{
 m_agentFactory.SetTypeId ("ns3::shingo::RoutingProtocol");
}

ShingoHelper* 
ShingoHelper::Copy (void) const 
{
 return new ShingoHelper (*this); 
}

Ptr<Ipv4RoutingProtocol> 
ShingoHelper::Create (Ptr<Node> node) const
{
 Ptr<shingo::RoutingProtocol> agent = m_agentFactory.Create<shingo::RoutingProtocol> ();
 node->AggregateObject (agent);
 return agent;
}

void 
ShingoHelper::SetAttribute (std::string name, const AttributeValue &value)
{
 m_agentFactory.Set (name, value);
}

int64_t
ShingoHelper::AssignStreams (NodeContainer c, int64_t stream)
{
 int64_t currentStream = stream;
 Ptr<Node> node;
 for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i) {
  node = (*i);
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
  NS_ASSERT_MSG (ipv4, "Ipv4 not installed on node");
  Ptr<Ipv4RoutingProtocol> shingo = ipv4->GetRoutingProtocol ();
  NS_ASSERT_MSG (shingo, "Ipv4 routing not installed on node");
  Ptr<shingo::RoutingProtocol> myshingo = DynamicCast<shingo::RoutingProtocol> (shingo);
  if (shingo) {
   currentStream += myshingo->AssignStreams (currentStream);
   continue;
  }
  // shingo may also be in a list
  Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (shingo);
  if (list) {
   int16_t priority;
   Ptr<Ipv4RoutingProtocol> listShingo;
   Ptr<shingo::RoutingProtocol> listshingo;
   for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++) {
    listShingo = list->GetRoutingProtocol (i, priority);
    listshingo = DynamicCast<shingo::RoutingProtocol> (listShingo);
    if (listshingo) {
     currentStream += listshingo->AssignStreams (currentStream);
     break;
    }
   }
  }
 }
 return (currentStream - stream);
}

}

