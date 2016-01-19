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
#include <climits>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SmsEchoClientApplication");
NS_OBJECT_ENSURE_REGISTERED (SmsEchoClient);

FileSMSChunks::FileSMSChunks(unsigned int id, size_t size, bool i_have_full_file) : FileSMS(id, size) {
  file_size_in_chunks = (uint32_t) std::ceil(1000*size/((double) CHUNK_SIZE));
  size_of_last_chunk = (1000*size) % CHUNK_SIZE;
  chunks = new bool[file_size_in_chunks];
  if (!i_have_full_file)
    num_of_received_chunks = 0;
  else
    num_of_received_chunks = file_size_in_chunks;
  // nodes_who_have_file = new std::vector<Ipv4Address>;
}

bool FileSMSChunks::is_full() {
  return num_of_received_chunks == file_size_in_chunks;
}

double FileSMSChunks::get_popularity(uint32_t total_number_of_nodes) {
  return nodes_who_have_file.size()/((double) total_number_of_nodes);
}

uint32_t FileSMSChunks::get_num_of_missing_chunks() {
  return file_size_in_chunks - num_of_received_chunks;
}

void FileSMSChunks::add_node_to_seen_list(Ipv4Address node) {
  // bool not_in_list_yet = true;
  for (size_t i = 0; i < nodes_who_have_file.size(); i++) {
    if (nodes_who_have_file[i].IsEqual(node)) {
      // not_in_list_yet = false;
      return;
    }
  }
  nodes_who_have_file.push_back(node);
  NS_LOG_INFO("File " << getFileId() << " seen in these nodes: ");
  for (size_t i = 0; i < nodes_who_have_file.size(); i++) {
     NS_LOG_INFO(nodes_who_have_file[i]);
  }
}

void SmsEchoClient::addNodeToSeenList(Ipv4Address sender) {
  for (size_t i = 0; i < seen_nodes.size(); i++) {
    if (seen_nodes[i].IsEqual(sender)) {
      // not_in_list_yet = false;
      return;
    }
  }
  seen_nodes.push_back(sender);
  std::stringstream ss;
  ss << "Seen nodes in " << address << ": ";
  // for (size_t i = 0; i < seen_nodes.size(); i++) {
  //    ss << seen_nodes[i] << ", ";
  // }
  ss << seen_nodes.size();
  NS_LOG_INFO(ss.str());
}

uint32_t SmsEchoClient::GetNumOfFullFiles() {
  uint32_t full_files = 0;
  for (uint32_t i = 0; i < files.size(); i++) {
    if(files[i].is_full())
      full_files += 1;
  }
  return full_files;
}

FileSMSChunks SmsEchoClient::getFileToRequest() {
  std::stringstream ss;
  ss << "Files with lowest chunks missing: ";
  uint32_t minimumChunksMissing = UINT_MAX;
  std::vector<FileSMSChunks> filesWithMinimumChunks;
  for (size_t i = 0; i < files.size(); i++) {
    if (!files[i].is_full()) {
      if (files[i].get_num_of_missing_chunks() < minimumChunksMissing) {
        filesWithMinimumChunks.clear();
        filesWithMinimumChunks.push_back(files[i]);
        minimumChunksMissing = files[i].get_num_of_missing_chunks();
      } else if (files[i].get_num_of_missing_chunks() == minimumChunksMissing) {
        filesWithMinimumChunks.push_back(files[i]);
      }
    }
  }
  FileSMSChunks fileWithLowestPopularity = FileSMSChunks(0,0,true);
  double lowest_popularity = INFINITY;
  for (size_t i = 0; i < filesWithMinimumChunks.size(); i++) {
    FileSMSChunks file = filesWithMinimumChunks[i];
    ss << "ID: " << file.getFileId() << ", chunks missing: " << file.get_num_of_missing_chunks() << ", popularity: " << file.get_popularity(seen_nodes.size()) << "; ";
    if (file.get_popularity(seen_nodes.size()) < lowest_popularity) {
      fileWithLowestPopularity = file;
      lowest_popularity = file.get_popularity(seen_nodes.size());
    }
  }
  ss << "CHOSEN FILE TO REQUEST: ID: " << fileWithLowestPopularity.getFileId() << " popularity: " << fileWithLowestPopularity.get_popularity(seen_nodes.size()) << "; ";
  NS_LOG_INFO(ss.str());
  // If the file returned file is full it means that it's invalid because the client knows no more files to request.
  return fileWithLowestPopularity;
}

uint8_t* SmsEchoClient::EncodeFilesForAdv() {
  // NS_LOG_INFO("Warning: Only 16 bits for id and size in order that the packet doesn't overflow");
  //                                                times 2 because of id and size being transmitted
  uint32_t full_files = GetNumOfFullFiles();
  NS_LOG_INFO("Total files " << files.size() << " full files " << full_files);
  uint16_t* array = (uint16_t*) malloc(full_files*2*sizeof(uint16_t));
  // uint16_t array[files.size()*2];
  uint32_t i = 0;
  uint32_t j = 0;
  while (i < full_files) {
    if (!files[j].is_full()) {
      j++;
      // NS_LOG_INFO(j);
      continue;
    }
    array[i*2] = (uint16_t) files[j].getFileId();
    array[i*2+1] = (uint16_t) files[j].getFileSize();
    j++;
    i++;
    // NS_LOG_INFO(i);
  }
  return (uint8_t*) array;
}

std::vector<FileSMSChunks> SmsEchoClient::DecodeFilesForAdv(uint8_t* raw_array, uint8_t num_advertised_files, Ipv4Address sender) {
  uint16_t* array = (uint16_t*) raw_array;
  std::vector<FileSMSChunks> received_files;
  for (size_t i = 0; i < num_advertised_files; i++) {
    received_files.push_back(FileSMSChunks(array[i*2], array[i*2+1], false));
  }
  std::stringstream ss;
  for (size_t i = 0; i < received_files.size(); i++) {
    bool already_known_file = false;
    for (size_t j = 0; j < files.size(); j++) {
      if (files[j] == received_files[i]) {
        files[j].add_node_to_seen_list(sender);
        already_known_file = true;
        break;
      }
    }
    if (!already_known_file) {
      ss << "File " << received_files[i].getFileId() <<
        " size: " << received_files[i].getFileSize() <<
        " chunks: " << received_files[i].file_size_in_chunks << "; ";
      files.push_back(received_files[i]);
      files.back().add_node_to_seen_list(sender);
    }

  }
  if (!ss.str().empty()) {
    NS_LOG_INFO("Unknown files seen " << ss.str());
  } else {
    NS_LOG_INFO("No new files seen");
  }
  return received_files;
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

void SmsEchoClient::SetFiles (std::vector<FileSMS> filesToSet) {
  files.clear();
  std::vector<FileSMS>::const_iterator it;
  NS_LOG_INFO("Node " << address);
  std::stringstream ss;
  for (uint32_t i = 0; i < filesToSet.size(); i++) {
    // NS_LOG_INFO("Before creating the object");
    files.push_back(FileSMSChunks(filesToSet[i].getFileId(),filesToSet[i].getFileSize(),true));
    // NS_LOG_INFO("File Size");
    // NS_LOG_INFO(this->files.size());
    ss << "File " << files[i].getFileId() <<
      // " size: " << files[i].getFileSize() <<
      // " chunks: " << files[i].file_size_in_chunks <<
       "; ";
  }
  NS_LOG_INFO(ss.str());
}

void SmsEchoClient::SetIPAdress (Ipv4Address address) {
  this->address = address;
  // addNodeToSeenList(this->address);
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
  }
  m_socket->SetRecvCallback (MakeCallback (&SmsEchoClient::HandleRead, this));

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
  m_socket_send->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());

  double random_offset = ((double) std::rand() / RAND_MAX);
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

  uint8_t adv[] = {0};
  uint8_t num_files[] = {(uint8_t) GetNumOfFullFiles()};
  uint8_t* encoded_files = this->EncodeFilesForAdv();
  size_t full_length_of_packet = sizeof(adv)*2+sizeof(uint16_t)*files.size()*2;
  uint8_t* full_packet = (uint8_t*) malloc(full_length_of_packet);
  memcpy(full_packet, adv, sizeof(adv));
  memcpy(full_packet+sizeof(adv), num_files, sizeof(num_files));
  memcpy(full_packet+sizeof(adv)+sizeof(num_files), encoded_files, sizeof(uint16_t)*GetNumOfFullFiles()*2);
  free(encoded_files);
  SetFill(full_packet, full_length_of_packet, full_length_of_packet);
  free(full_packet);
  // NS_LOG_INFO("Sent stuff!!!!");
  Ptr<Packet> p;
  p = Create<Packet> (m_data, m_dataSize);

  m_txTrace (p);
  m_socket_send->Send(p);

  ++m_sent;

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
      << "s client " << address << " sent Advertisement to 255.255.255.255 port "<< m_peerPort);

  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
}

// Handles everything that's broadcast
void
SmsEchoClient::HandleRead (Ptr<Socket> socket)
{
  // NS_LOG_INFO ("Received something");
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      // NS_LOG_INFO("Received something");
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client " << address << " received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      size_t size_of_header_preview = MIN(packet->GetSize(),2*sizeof(uint8_t));
      uint8_t packet_content[size_of_header_preview];
      packet->CopyData(packet_content, size_of_header_preview);
      // NS_LOG_INFO("Packet size " << packet->GetSize());
      // char s[1];
      // sprintf(s,"%d", packet_content[0]);
      NS_LOG_INFO("Packet content " << ((uint32_t) packet_content[0]));
      if (packet_content[0] == 0) {
        NS_LOG_INFO("Packet is an advertisement");
        addNodeToSeenList(InetSocketAddress::ConvertFrom (from).GetIpv4 ());
        uint8_t num_of_files = packet_content[1];
        packet->RemoveAtStart(sizeof(uint8_t)*2);
        // Maybe not safe to put huge amounts of data on the stack...
        uint8_t raw_files[packet->GetSize ()];
        packet->CopyData(raw_files, packet->GetSize ());
        DecodeFilesForAdv(raw_files, num_of_files, InetSocketAddress::ConvertFrom (from).GetIpv4 ());
        request_header req_h = {.packet_type = 1, .receiver_address = InetSocketAddress::ConvertFrom(from).GetIpv4().Get(), .file_id = 0, .chunk_id = 0};
        packet = Create<Packet> ((uint8_t*) &req_h, sizeof(request_header));
        m_txTrace (packet);
        // socket->SendTo(packet, 0, from);
        m_socket_send->Send(packet);
      } else if (packet_content[0] == 1) {
        NS_LOG_INFO("Packet is a request");
        FileSMSChunks file_to_request = getFileToRequest();
        // NS_LOG_INFO();
        request_header request;
        uint8_t raw_packet[packet->GetSize ()];
        packet->CopyData(raw_packet, packet->GetSize ());
        memcpy(&request, raw_packet, sizeof(request_header));
        Ipv4Address receiver_address = Ipv4Address(request.receiver_address);
        NS_LOG_INFO("Receiver address: " << receiver_address);
        if (!receiver_address.IsEqual(address)) {
          NS_LOG_INFO("My address " << address << ", this packet isn't for me");
          return;
        }
        // typedef struct {
        //   uint8_t packet_type;
        //   uint32_t original_requester;
        //   uint32_t file_id;
        //   uint32_t chunk_id;
        // } reply_header;
        reply_header header;
        header.packet_type = 2;
        header.original_requester = InetSocketAddress::ConvertFrom(from).GetIpv4().Get();
        // Retrieve chunk
        // TODO
        size_t chunk_size;
        if (true) {
          chunk_size = CHUNK_SIZE;
        }
        // Send requested chunk
        size_t data_size = sizeof(reply_header) + chunk_size;
        uint8_t data[data_size];
        memcpy(data, &header, sizeof(reply_header));
        packet = Create<Packet> (data, data_size);
        m_txTrace (packet);
        m_socket_send->Send(packet);
      } else if (packet_content[0] == 2) {
        NS_LOG_INFO("Packet is a reply");
      } else {
        NS_LOG_INFO("Got some packet type :o");
      }
    }
}

// Handles everything that's unicast
// void
// SmsEchoClient::HandleRequest (Ptr<Socket> socket) {
//   NS_LOG_FUNCTION(this << socket);
//   Ptr<Packet> packet;
//   Address from;
//   while (packet = socket->RecvFrom(from)) {
//       if (InetSocketAddress::IsMatchingType (from)) {
//           NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client " << address << " received " << packet->GetSize () << " bytes from " <<
//                        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
//                        InetSocketAddress::ConvertFrom (from).GetPort ());
//       }
//       size_t req_size = sizeof(SmsEchoClient::request_header);
//       uint8_t req_buffer[req_size];
//       SmsEchoClient::request_header request;
//       packet->CopyData(req_buffer, req_size);
//       memcpy(&request, req_buffer, req_size);
//       if (request.packet_type == 1) {
//         SmsEchoClient::reply_header header;
//         header.packet_type = 2;
//         header.original_requester = InetSocketAddress::ConvertFrom(from).GetIpv4().Get();
//         // Retrieve chunk
//         // TODO
//         size_t chunk_size;
//         if (true) {
//           chunk_size = CHUNK_SIZE;
//         }
//         // Send requested chunk
//         NS_LOG_WARN("Ah, sizeof(struct) gives you a wrong size because of alignment of the struct in memory!");
//         size_t data_size = sizeof(SmsEchoClient::reply_header) + chunk_size;
//         uint8_t* data = new uint8_t[data_size];
//         memcpy(data, &header, sizeof(SmsEchoClient::reply_header));
//         packet = Create<Packet> (data, data_size);
//         m_txTrace (packet);
//         m_socket_send->Send(packet);
//         delete data;
//       }
//   }
// }

} // Namespace ns3
