/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "sms-echo-client.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SmsEchoClientApplication");
NS_OBJECT_ENSURE_REGISTERED (SmsEchoClient);

FileSMSChunks::FileSMSChunks(unsigned int id, size_t size) : FileSMS(id, size) {
  file_size_in_chunks = (uint32_t) std::ceil(1000*size/((double) CHUNK_SIZE));
  size_of_last_chunk = (1000*size) % CHUNK_SIZE;
  chunks = new bool[file_size_in_chunks];
  num_of_received_chunks = 0;
  // nodes_who_have_file = new std::vector<Ipv4Address>;
}

double FileSMSChunks::get_popularity(uint32_t total_number_of_nodes) {
  return nodes_who_have_file.size()/total_number_of_nodes;
}

uint32_t FileSMSChunks::get_num_of_missing_chunks () {
  return file_size_in_chunks - num_of_received_chunks;
}

void FileSMSChunks::add_node_to_seen_list(Ipv4Address node) {
  nodes_who_have_file.push_back(node);
}

TypeId
SmsEchoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SmsEchoClient")
    .SetParent<Application> ()
    .AddConstructor<SmsEchoClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SmsEchoClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&SmsEchoClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&SmsEchoClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SmsEchoClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&SmsEchoClient::SetDataSize,
                                         &SmsEchoClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&SmsEchoClient::m_txTrace))
  ;
  return tid;
}

void SmsEchoClient::SetFiles (std::vector<FileSMS> files) {
  this->files.clear();
  std::vector<FileSMS>::const_iterator it;
  for (uint32_t i = 0; i < files.size(); i++) {
    // NS_LOG_INFO("Before creating the object");
    this->files.push_back(FileSMSChunks(files[i].getFileId(),files[i].getFileSize()));
    // NS_LOG_INFO("File Size");
    // NS_LOG_INFO(this->files.size());
    NS_LOG_INFO("File " << this->files[i].getFileId() <<
      " size: " << this->files[i].getFileSize() <<
      " chunks: " << this->files[i].file_size_in_chunks);
  }
}

SmsEchoClient::SmsEchoClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_socket_send = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
}

SmsEchoClient::~SmsEchoClient()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket_send = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
SmsEchoClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
SmsEchoClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void
SmsEchoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SmsEchoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  if (m_socket == 0) {
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_peerPort);
      m_socket->Bind (local);
      // TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      // m_socket = Socket::CreateSocket (GetNode (), tid);
      // if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
      //   {
      //     m_socket->Bind();
      //     m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
      //   }
  }
  if (m_socket_send == 0) {
    m_socket_send = Socket::CreateSocket (GetNode (), tid);
    // if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
    if (true)
      {
        m_socket_send->Bind();
        m_socket_send->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
      }
  }

  m_socket_send->SetAllowBroadcast(true);
  m_socket->SetRecvCallback (MakeCallback (&SmsEchoClient::HandleRead, this));

  double random_offset = ((double) std::rand() / RAND_MAX);
  // std::cout << "Random offset is " << RAND_MAX << std::endl;
  ScheduleTransmit (Seconds (0.+random_offset));
}

void
SmsEchoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
  }

  if (m_socket_send != 0) {
      m_socket_send->Close ();
      m_socket_send = 0;
  }

  Simulator::Cancel (m_sendEvent);
}

void
SmsEchoClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
SmsEchoClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void
SmsEchoClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SmsEchoClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SmsEchoClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SmsEchoClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &SmsEchoClient::Send, this);
}

void
SmsEchoClient::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  // std::cout << files;

  uint8_t adv[] = {0};
  this->SetFill(adv, 1, 1);

  Ptr<Packet> p;
  p = Create<Packet> (m_data, m_dataSize);

  m_txTrace (p);
  m_socket_send->Send(p);

  ++m_sent;

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
      << "s client sent Advertisement to 255.255.255.255 port "<< m_peerPort);

  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
}

void
SmsEchoClient::HandleRead (Ptr<Socket> socket)
{
  // NS_LOG_INFO ("Received something");
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      uint8_t packetType[1];
      packet->CopyData(packetType, 1);
      if (packetType[0] == 0) {
        /* NS_LOG_INFO("Sending response"); */
        uint8_t response[] = {1};
        packet = Create<Packet> (response, 1);
        m_txTrace (packet);
        socket->SendTo(packet, 0, from);
      }
    }
}

} // Namespace ns3
