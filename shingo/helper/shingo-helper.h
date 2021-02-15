/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SHINGO_HELPER_H
#define SHINGO_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"

namespace ns3
{

class ShingoHelper : public Ipv4RoutingHelper
{
 public:
 ShingoHelper();

 ShingoHelper* Copy (void) const;
 virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
 void SetAttribute (std::string name, const AttributeValue &value);
 int64_t AssignStreams (NodeContainer c, int64_t stream);

 
 private:
 ObjectFactory m_agentFactory;
};

}
#endif /* SHINGO_HELPER_H */

