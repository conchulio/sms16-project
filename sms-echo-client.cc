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
#include <cstdlib>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define ADVERTISEMENT_OFFSET 50.0

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SmsEchoClientApplication");
NS_OBJECT_ENSURE_REGISTERED (SmsEchoClient);

FileSMSChunks::FileSMSChunks(unsigned int id, size_t size, bool i_have_full_file) : FileSMS(id, size) {
  // NS_LOG_INFO("Constructor called file " << id);
  file_size_in_chunks = (uint32_t) std::ceil(1000*size/((double) CHUNK_SIZE));
  size_of_last_chunk = (1000*size) % CHUNK_SIZE;
  for (size_t i = 0; i < file_size_in_chunks; i++) {
    if (i_have_full_file)
      chunks.push_back(true);
    else
      chunks.push_back(false);
  }
  // NS_LOG_INFO("chunks pointer at construction: " << chunks);
  // if (file_size_in_chunks == 0) {
  //   NS_LOG_INFO("File size is zero");
  // }
  // if (this->chunks == NULL) {
  //   NS_LOG_INFO("Calloc died");
  // }
  if (!i_have_full_file)
    num_of_received_chunks = 0;
  else
    num_of_received_chunks = file_size_in_chunks;
  // nodes_who_have_file = new std::vector<Ipv4Address>;
}

bool FileSMSChunks::is_full() {
  return num_of_received_chunks == file_size_in_chunks;
}

bool FileSMSChunks::seen_in_node(Ipv4Address node) {
  return std::find(nodes_who_have_file.begin(), nodes_who_have_file.end(), node) != nodes_who_have_file.end();
}

uint32_t FileSMSChunks::get_first_missing_chunk() {
  uint32_t first_missing_chunk = 0;
  for (;first_missing_chunk < file_size_in_chunks; first_missing_chunk++) {
    if (!chunks[first_missing_chunk]) {
      break;
    }
  }
  return first_missing_chunk;
}

uint16_t FileSMSChunks::get_size_of_chunk(uint32_t chunk_id) {
  if (chunk_id == file_size_in_chunks-1) {
    return size_of_last_chunk;
  } else {
    return CHUNK_SIZE;
  }
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
  // NS_LOG_INFO("File " << getFileId() << " seen in these nodes: ");
  for (size_t i = 0; i < nodes_who_have_file.size(); i++) {
    //  NS_LOG_INFO(nodes_who_have_file[i]);
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
  // NS_LOG_INFO(ss.str());
}

uint32_t SmsEchoClient::GetNumOfFullFiles() {
  uint32_t full_files = 0;
  for (uint32_t i = 0; i < files.size(); i++) {
    if(files[i].is_full())
      full_files += 1;
  }
  return full_files;
}

bool SmsEchoClient::add_new_chunk(uint32_t file_id, uint32_t file_size, uint32_t chunk_id, Ipv4Address sender) {
  std::pair<FileSMSChunks,int32_t> file_valid_pair = getFileById(file_id);
  if (file_valid_pair.second == -1) {
    // We haven't seen this file so far
    files.push_back(FileSMSChunks(file_id, file_size, false));
    files.back().add_node_to_seen_list(sender);
    files.back().chunks[chunk_id] = true;
    files.back().num_of_received_chunks+=1;
    NS_LOG_INFO("Got new chunk " << chunk_id << " for previously unknown file " << file_id);
  } else if (!files[file_valid_pair.second].chunks[chunk_id]) {
    // We already know about this file
    NS_LOG_INFO(address << " got new chunk " << chunk_id << " for file " << file_id << " file index in array " << file_valid_pair.second);
    files[file_valid_pair.second].add_node_to_seen_list(sender);
    files[file_valid_pair.second].chunks[chunk_id] = true;
    files[file_valid_pair.second].num_of_received_chunks+=1;
  } else {
    return false;
  }
  maximum_full_files_seen = MAX(maximum_full_files_seen,GetNumOfFullFiles());
  NS_LOG_INFO("Num of received chunks " << files[file_valid_pair.second].num_of_received_chunks);
  return true;
}

std::pair<FileSMSChunks,int32_t> SmsEchoClient::getFileById(uint32_t id) {
  for (size_t i = 0; i < files.size(); i++) {
    // NS_LOG_INFO("ID we look for " << id << " this file's id " << files[i].getFileId() << " i " << i);
    if (files[i].getFileId() == id) {
      // NS_LOG_INFO("Found it! ID we look for " << id << " this file's id " << files[i].getFileId() << " i " << i);
      return std::pair<FileSMSChunks,uint32_t>(files[i],i);
    }
  }
  return std::pair<FileSMSChunks,uint32_t>(FileSMSChunks(0,0,true),-1);
}

FileSMSChunks SmsEchoClient::getFileToRequest(Ipv4Address node_which_we_ask) {

  std::stringstream ss;
  ss << "Files with lowest chunks missing: ";
  uint32_t minimumChunksMissing = UINT_MAX;
  std::vector<FileSMSChunks> filesWithMinimumChunks;
  for (size_t i = 0; i < files.size(); i++) {
    if (!files[i].is_full() && files[i].seen_in_node(node_which_we_ask)) {
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
  ss << "all files which I have: ";
  for (size_t i = 0; i < files.size(); i++) {
    ss << "id: " << files[i].getFileId() << " is full? " << files[i].is_full() << ", ";
  }
  ss << "CHOSEN FILE TO REQUEST: ID: " << fileWithLowestPopularity.getFileId() << " popularity: " << fileWithLowestPopularity.get_popularity(seen_nodes.size()) << " index: " << getFileById(fileWithLowestPopularity.getFileId()).second << ";";
  NS_LOG_INFO(ss.str());
  return fileWithLowestPopularity;
}

uint8_t* SmsEchoClient::EncodeFilesForAdv() {
  // NS_LOG_INFO("Warning: Only 16 bits for id and size in order that the packet doesn't overflow");
  //                                                times 2 because of id and size being transmitted
  uint32_t full_files = GetNumOfFullFiles();
  maximum_full_files_seen = MAX(maximum_full_files_seen, full_files);
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
  maximum_full_files_seen = MAX(maximum_full_files_seen, num_advertised_files);
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
    std::stringstream ss;
    ss << "Files which I have: ";
    for (uint32_t i = 0; i < files.size(); i++) {
      if (files[i].is_full())
        ss << "File " << files[i].getFileId() << "; ";
    }
    ss << "Files which the other node has: ";
    for (uint32_t i = 0; i < received_files.size(); i++) {
      ss << "File " << received_files[i].getFileId() << "; ";
    }
    NS_LOG_INFO(ss.str());
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
    files.push_back(FileSMSChunks(filesToSet[i].getFileId(),filesToSet[i].getFileSize(),true));
    ss << "File " << files[i].getFileId() <<
      // " size: " << files[i].getFileSize() <<
      // " chunks: " << files[i].file_size_in_chunks <<
       "; ";
  }
  maximum_full_files_seen = MAX(maximum_full_files_seen,filesToSet.size());
  // NS_LOG_INFO(ss.str());
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
  m_sendEvent = EventId();
  m_requestEvent = EventId();
  m_replyEvent = EventId();
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

  // ScheduleTransmit (Seconds (0.+random_offset));
  m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(true)), &SmsEchoClient::Send, this);
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

  Simulator::Cancel(m_sendEvent);
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

// void
// SmsEchoClient::ScheduleTransmit (Time dt)
// {
//   NS_LOG_FUNCTION (this << dt);
//   m_sendEvent = Simulator::Schedule (dt, &SmsEchoClient::Send, this);
// }

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

  // ++m_sent;

  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
      << "s client " << address << " sent Advertisement to 255.255.255.255 port "<< m_peerPort);

  // if (m_sent < m_count)
  //   {
  // ScheduleTransmit (m_interval);
  //   }
}

/*
IDEA: conservative approach: with 24Mbps one full frame of 1500 bytes
takes 0.001s to transmit.

So:
timer for reply:    0s
timer for request:  (num_of_full_files_i_own+1) * 0.001s
  maybe even less or maybe
                    ln(num_of_full_files_i_own+2) * 0.001s
  or maybe something with sqrt(x)
timer for advertisement: 50+50*(1/(num_of_full_files_i_own+1))+(random number from -5 to 5)
*/

double SmsEchoClient::get_time_advertisement(bool start) {
  // All values in milliseconds
  // double offset = ADVERTISEMENT_OFFSET;
  // NS_LOG_INFO("maximum_full_files_seen: " << maximum_full_files_seen);
  double offset = maximum_full_files_seen + 10.0;
  if (start) {
    offset = 0;
  }
  double num_of_full_files_i_own = (double) GetNumOfFullFiles();
  // from -5.0 to 5.0
  double multiplier = 50;
  double random_component = ((double) std::rand() / RAND_MAX)*15;
  double to_seconds = 0.001;
  return to_seconds*(offset+multiplier*(1.0/num_of_full_files_i_own)+random_component);
}

double SmsEchoClient::get_time_request() {
  double num_of_full_files_i_own = (double) GetNumOfFullFiles();
  double to_seconds = 0.001;
  double random_component = ((double) std::rand() / RAND_MAX)*10;
  double reply = num_of_full_files_i_own+1.0+random_component;
  double time_for_advertisement = get_time_advertisement(false);
  if (to_seconds*reply > time_for_advertisement) {
    NS_LOG_WARN("This reply is scheduled with lower priority than an advertisement :o, reply " << to_seconds*reply << " time for advertisement " << time_for_advertisement);
  }
  // NS_LOG_INFO("Number of files I own: " << reply);
  return to_seconds*reply;
}

void SmsEchoClient::cancel_all_events() {
  Simulator::Cancel(m_sendEvent);
  Simulator::Cancel(m_replyEvent);
  Simulator::Cancel(m_requestEvent);
}

void SmsEchoClient::request_packet(Ipv4Address sender, FileSMSChunks file_to_request) {
  NS_LOG_INFO(address << " requesting file " << file_to_request.getFileId() << " chunk number " << file_to_request.get_first_missing_chunk() <<
    " number of chunk we already have " << file_to_request.num_of_received_chunks << " size of chunk array " << file_to_request.chunks.size());
  request_header request = {.packet_type = 1, .receiver_address = sender.Get(),
    .file_id = file_to_request.getFileId(), .chunk_id = file_to_request.get_first_missing_chunk()};
  Ptr<Packet> packet = Create<Packet> ((uint8_t*) &request, sizeof(request_header));
  // m_txTrace (packet);
  // socket->SendTo(packet, 0, from);
  m_socket_send->Send(packet);
}

void SmsEchoClient::reply(reply_header* reply, uint16_t chunk_size) {
  NS_LOG_INFO("Sending reply, file ID: " << reply->file_id << ", chunk_id: " << reply->chunk_id);
  size_t data_size = sizeof(reply_header) + chunk_size;
  uint8_t data[data_size];
  memcpy(data, reply, sizeof(reply_header));
  Ptr<Packet> packet = Create<Packet> (data, data_size);
  // m_txTrace (packet);
  m_socket_send->Send(packet);
  free(reply);
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
      Ipv4Address sender = InetSocketAddress::ConvertFrom(from).GetIpv4();
      // NS_LOG_INFO("Received something");
      if (InetSocketAddress::IsMatchingType (from))
        {
          // NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client " << address << " received " << packet->GetSize () << " bytes from " <<
          //              sender << " port " <<
          //              InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      size_t size_of_header_preview = MIN(packet->GetSize(),2*sizeof(uint8_t));
      uint8_t packet_content[size_of_header_preview];
      packet->CopyData(packet_content, size_of_header_preview);
      // NS_LOG_INFO("Packet size " << packet->GetSize());
      // char s[1];
      // sprintf(s,"%d", packet_content[0]);
      // NS_LOG_INFO("Packet content " << ((uint32_t) packet_content[0]));
      if (packet_content[0] == 0) {
        cancel_all_events();
        NS_LOG_INFO("Packet is an advertisement at time " << Simulator::Now ().GetSeconds () << "s client " <<
          address << " received " << packet->GetSize () << " bytes from " <<
          sender << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        addNodeToSeenList(sender);
        uint8_t num_of_files = packet_content[1];
        packet->RemoveAtStart(sizeof(uint8_t)*2);
        // Maybe not safe to put huge amounts of data on the stack...
        uint8_t raw_files[packet->GetSize ()];
        packet->CopyData(raw_files, packet->GetSize ());
        DecodeFilesForAdv(raw_files, num_of_files, sender);
        FileSMSChunks file_to_request = getFileToRequest(sender);
        if (file_to_request.is_full()) {
          NS_LOG_WARN("No more files to request for node " << address << " at time " << Simulator::Now().GetSeconds());
          // Maybe here we shouldn't advertise again and just shut up. Then the simulation would end automatically
          // m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);
          return;
        }
        m_requestEvent = Simulator::Schedule (Seconds(get_time_request()), &SmsEchoClient::request_packet, this, sender, file_to_request);
        m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);
        // TODO schedule next advertisement

      } else if (packet_content[0] == 1) {
        cancel_all_events();
        request_header request;
        uint8_t raw_packet[packet->GetSize ()];
        packet->CopyData(raw_packet, packet->GetSize ());
        memcpy(&request, raw_packet, sizeof(request_header));
        Ipv4Address receiver_address = Ipv4Address(request.receiver_address);
        // NS_LOG_INFO("Receiver address: " << receiver_address);
        if (!receiver_address.IsEqual(address)) {
          // NS_LOG_INFO("My address " << address << ", this packet isn't for me");
          m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);
          return;
        }
        NS_LOG_INFO("Packet is a request, requesting " << request.file_id << ", chunk " << request.chunk_id <<
          " at time " << Simulator::Now ().GetSeconds () << "s client " <<
          address << " received " << packet->GetSize () << " bytes from " <<
          sender << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        std::pair<FileSMSChunks,int32_t> file_by_id_and_valid = getFileById(request.file_id);
        FileSMSChunks file_requested = file_by_id_and_valid.first;
        if (file_by_id_and_valid.second == -1) {
          NS_LOG_WARN("File isn't valid, exiting now!");
          abort();
        }
        uint16_t chunk_size = file_requested.get_size_of_chunk(request.chunk_id);
        reply_header* reply = (reply_header*) malloc(sizeof(reply_header));
        // reply = &(reply_header) {.packet_type = 2, .original_requester = sender.Get(),
        //   .file_id = file_requested.getFileId(), .file_size = (uint32_t) file_requested.getFileSize(), .chunk_id = request.chunk_id};
        reply->packet_type = 2;
        reply->original_requester = sender.Get();
        reply->file_id = file_requested.getFileId();
        reply->file_size = (uint32_t) file_requested.getFileSize();
        reply->chunk_id = request.chunk_id;
        m_replyEvent = Simulator::Schedule (Seconds(0), &SmsEchoClient::reply, this, reply, chunk_size);
        m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);

      } else if (packet_content[0] == 2) {
        cancel_all_events();
        NS_LOG_INFO("Packet is a reply at time " << Simulator::Now ().GetSeconds () << "s client " <<
          address << " received " << packet->GetSize () << " bytes from " <<
          sender << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        reply_header reply;
        uint8_t raw_packet[packet->GetSize ()];
        packet->CopyData(raw_packet, packet->GetSize ());
        memcpy(&reply, raw_packet, sizeof(reply_header));
        add_new_chunk(reply.file_id, reply.file_size, reply.chunk_id, sender);
        Ipv4Address original_requester = Ipv4Address(reply.original_requester);
        if (original_requester == address) {
          // We are allowed to request again :)
          FileSMSChunks file_to_request = getFileToRequest(sender);
          if (file_to_request.is_full()) {
            NS_LOG_WARN("No more files to request for node " << address << " at time " << Simulator::Now().GetSeconds());
            m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);
            return;
          }
          // If we are the original_requester we request again immediately
          m_requestEvent = Simulator::Schedule (Seconds(0.), &SmsEchoClient::request_packet, this, sender, file_to_request);
        }
        m_sendEvent = Simulator::Schedule (Seconds (get_time_advertisement(false)), &SmsEchoClient::Send, this);
      } else {
        NS_LOG_WARN("Got some weird packet type :o at time " << Simulator::Now ().GetSeconds () << "s client " <<
          address << " received " << packet->GetSize () << " bytes from " <<
          sender << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
        abort();
      }
    }
}

} // Namespace ns3
