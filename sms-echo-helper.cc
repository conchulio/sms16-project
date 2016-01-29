/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "sms-echo-helper.h"
#include "sms-echo-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

// SmsEchoServerHelper::SmsEchoServerHelper (uint16_t port)
// {
//   m_factory.SetTypeId (SmsEchoServer::GetTypeId ());
//   SetAttribute ("Port", UintegerValue (port));
// }
//
// void
// SmsEchoServerHelper::SetAttribute (
//   std::string name,
//   const AttributeValue &value)
// {
//   m_factory.Set (name, value);
// }
//
// ApplicationContainer
// SmsEchoServerHelper::Install (Ptr<Node> node) const
// {
//   return ApplicationContainer (InstallPriv (node));
// }
//
// ApplicationContainer
// SmsEchoServerHelper::Install (std::string nodeName) const
// {
//   Ptr<Node> node = Names::Find<Node> (nodeName);
//   return ApplicationContainer (InstallPriv (node));
// }
//
// ApplicationContainer
// SmsEchoServerHelper::Install (NodeContainer c) const
// {
//   ApplicationContainer apps;
//   for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
//     {
//       apps.Add (InstallPriv (*i));
//     }
//
//   return apps;
// }
//
// Ptr<Application>
// SmsEchoServerHelper::InstallPriv (Ptr<Node> node) const
// {
//   Ptr<Application> app = m_factory.Create<SmsEchoServer> ();
//   node->AddApplication (app);
//
//   return app;
// }

// We use this constructor
SmsEchoClientHelper::SmsEchoClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (SmsEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  this->nodeFileList = nodeFileList;
}

SmsEchoClientHelper::SmsEchoClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (SmsEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

SmsEchoClientHelper::SmsEchoClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (SmsEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
SmsEchoClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
SmsEchoClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<SmsEchoClient>()->SetFill (fill);
}

void
SmsEchoClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<SmsEchoClient>()->SetFill (fill, dataLength);
}

void
SmsEchoClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<SmsEchoClient>()->SetFill (fill, fillLength, dataLength);
}

void SmsEchoClientHelper::SetFiles (Ptr<Application> app, std::vector<FileSMS> files) {
  app->GetObject<SmsEchoClient>()->SetFiles(files);
}

ApplicationContainer
SmsEchoClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SmsEchoClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

// We use this method
ApplicationContainer
SmsEchoClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }
  // for (uint32_t i = 0; i < c.GetN(); i++) {
  //   apps.Add (InstallPriv (c.Get(i), nodeFileList[i]));
  // }

  return apps;
}

Ptr<SmsEchoClient>
SmsEchoClientHelper::InstallPriv (Ptr<Node> node//, std::vector<FileSMS> fileList
) const
{
  Ptr<SmsEchoClient> app = m_factory.Create<SmsEchoClient> ();
  node->AddApplication (app);
  // app.fileList = nodeFileList[]

  return app;
}

} // namespace ns3
