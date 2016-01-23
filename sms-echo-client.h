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

#ifndef SMS_ECHO_CLIENT_H
#define SMS_ECHO_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "sms-helpers.h"

#define CHUNK_SIZE 1450

namespace ns3 {

class Socket;
class Packet;

class FileSMSChunks : public FileSMS {
public:
  FileSMSChunks(unsigned int id, size_t size, bool i_have_full_file);

  std::vector<bool> chunks;
  uint16_t size_of_last_chunk;
  uint32_t file_size_in_chunks;
  uint32_t num_of_received_chunks;
  std::vector<Ipv4Address> nodes_who_have_file;

  uint32_t get_first_missing_chunk();
  uint32_t get_num_of_missing_chunks ();
  uint16_t get_size_of_chunk(uint32_t chunk_id);
  void add_node_to_seen_list(Ipv4Address node);
  bool seen_in_node(Ipv4Address node);
  double get_popularity(uint32_t total_number_of_nodes);
  bool is_full();
};

/**
 * \ingroup udpecho
 * \brief A Udp Echo client
 *
 * Every packet sent should be returned by the server and received here.
 */
class SmsEchoClient : public Application
{
public:
  static TypeId GetTypeId (void);

  SmsEchoClient ();

  virtual ~SmsEchoClient ();

  std::vector<FileSMSChunks> files;

  void cancel_all_events();
  double get_time_advertisement(bool start);
  double get_time_request();
  std::pair<FileSMSChunks,int32_t> getFileById(uint32_t id);
  bool add_new_chunk(uint32_t file_id, uint32_t file_size, uint32_t chunk_id, Ipv4Address sender);
  void SetFiles (std::vector<FileSMS> filesToSet);
  void SetIPAdress (Ipv4Address address);
  uint32_t GetNumOfFullFiles();
  void addNodeToSeenList(Ipv4Address sender);
  FileSMSChunks getFileToRequest(Ipv4Address node_which_we_ask);

  uint8_t* EncodeFilesForAdv();
  std::vector<FileSMSChunks> DecodeFilesForAdv(uint8_t* raw_array, uint8_t num_advertised_files, Ipv4Address sender);

  // uint32_t nodes_seen;

  // We would have to enable C++2011
  // enum packet_type : uint8_t {adv, req, resp};

  typedef struct adv_header {
    uint8_t packet_type;
  } adv_header;

  typedef struct request_header {
    uint8_t packet_type;
    uint32_t receiver_address;
    uint32_t file_id;
    uint32_t chunk_id;
  } request_header;

  typedef struct reply_header {
    uint8_t packet_type;
    uint32_t original_requester;
    uint32_t file_id;
    uint32_t file_size;
    uint32_t chunk_id;
  } reply_header;

  static const size_t reply_header_length = 13;

  /**
   * \param ip destination ipv4 address
   * \param port destination port
   */
  void SetRemote (Address ip, uint16_t port);
  void SetRemote (Ipv4Address ip, uint16_t port);
  void SetRemote (Ipv6Address ip, uint16_t port);

  /**
   * Set the data size of the packet (the number of bytes that are sent as data
   * to the server).  The contents of the data are set to unspecified (don't
   * care) by this call.
   *
   * \warning If you have set the fill data for the echo client using one of the
   * SetFill calls, this will undo those effects.
   *
   * \param dataSize The size of the echo data you want to sent.
   */
  void SetDataSize (uint32_t dataSize);

  /**
   * Get the number of data bytes that will be sent to the server.
   *
   * \warning The number of bytes may be modified by calling any one of the
   * SetFill methods.  If you have called SetFill, then the number of
   * data bytes will correspond to the size of an initialized data buffer.
   * If you have not called a SetFill method, the number of data bytes will
   * correspond to the number of don't care bytes that will be sent.
   *
   * \returns The number of data bytes.
   */
  uint32_t GetDataSize (void) const;

  /**
   * Set the data fill of the packet (what is sent as data to the server) to
   * the zero-terminated contents of the fill string string.
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the size of the fill string -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param fill The string to use as the actual echo data bytes.
   */
  void SetFill (std::string fill);

  /**
   * Set the data fill of the packet (what is sent as data to the server) to
   * the repeated contents of the fill byte.  i.e., the fill byte will be
   * used to initialize the contents of the data packet.
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the dataSize parameter -- this means that the PacketSize
   * attribute may be changed as a result of this call.
   *
   * \param fill The byte to be repeated in constructing the packet data.
   * \param dataSize The desired size of the resulting echo packet data.
   */
  void SetFill (uint8_t fill, uint32_t dataSize);

  /**
   * Set the data fill of the packet (what is sent as data to the server) to
   * the contents of the fill buffer, repeated as many times as is required.
   *
   * Initializing the packet to the contents of a provided single buffer is
   * accomplished by setting the fillSize set to your desired dataSize
   * (and providing an appropriate buffer).
   *
   * \warning The size of resulting echo packets will be automatically adjusted
   * to reflect the dataSize parameter -- this means that the PacketSize
   * attribute of the Application may be changed as a result of this call.
   *
   * \param fill The fill pattern to use when constructing packets.
   * \param fillSize The number of bytes in the provided fill pattern.
   * \param dataSize The desired size of the final echo data.
   */
  void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);

protected:
  virtual void DoDispose (void);

private:
  Ipv4Address address;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTransmit (Time dt);
  void request_packet(Ipv4Address sender, FileSMSChunks file_to_request);
  void reply(reply_header* request, uint16_t chunk_size);
  void Send (void);

  void HandleRead (Ptr<Socket> socket);
  void HandleRequest (Ptr<Socket> socket);

  uint32_t m_count;
  Time m_interval;
  uint32_t m_size;

  uint32_t m_dataSize;
  uint8_t *m_data;

  uint32_t m_sent;
  Ptr<Socket> m_socket;
  Ptr<Socket> m_socket_send;
  Address m_peerAddress;
  uint16_t m_peerPort;
  EventId m_sendEvent;
  EventId m_requestEvent;
  EventId m_replyEvent;
  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;

  uint32_t maximum_full_files_seen;

  // std::vector<FileSMSChunks> seen_files;
  std::vector<Ipv4Address> seen_nodes;
};

} // namespace ns3

#endif /* SMS_ECHO_CLIENT_H */
