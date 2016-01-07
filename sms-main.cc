#include "sms-helpers.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE("SMSProject");

void ReceivePacket (Ptr<Socket> socket)
{
  std::cout << "Enter ReceivePacket" << std::endl;
  while (socket->Recv ())
  {
    std::cout << "Received one packet!" << std::endl;
  }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
    uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
  {
    socket->Send (Create<Packet> (pktSize));
    Simulator::Schedule (pktInterval, &GenerateTraffic, 
        socket, pktSize,pktCount-1, pktInterval);
  }
  else
  {
    socket->Close ();
  }
}

int main(int argc, char* argv[]) {
    //puts("Hello World");
    LogComponentEnable("SMSProject", LOG_LEVEL_INFO);

    NodeContainer c;
    c.Create(getNumberOfMobileNodes());

    installMobility(c);

    NetDeviceContainer netDevices;
    installWifi(c, netDevices);

    InternetStackHelper internet;
    internet.Install(c);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(netDevices);

    std::vector< std::vector<FileSMS> > nodeFileList;

    for (size_t i = 0; i < c.GetN(); i++) {
        std::vector<FileSMS> files = getInitialFileList();
        nodeFileList.push_back(files);
        std::vector<FileSMS>::const_iterator it;
        for (it = files.begin(); it != files.end(); ++it) {
          /* std::cout << "File " << it->getFileId() << " size: " << it->getFileSize() << std::endl; */
        }
        // ...
        // The 'files' vector contains all the FileSMS objects that the node has
        // when the simulation starts. While in this example the information is
        // thrown away, the student must store it somewhere and use it appropriately.
        // ...
    }

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    std::vector< Ptr<Node> >::const_iterator it;
    /* for (it = c.Begin(); it != c.End(); ++it) { */
    /*   /1* std::cout << "Create sockets for node: " << (*it)->GetId() << std::endl; *1/ */
    /*   Ptr<Socket> recv = Socket::CreateSocket(*it, tid); */
    /*   InetSocketAddress bindAddress = InetSocketAddress(Ipv4Address::GetAny(), 42); */
    /*   recv->Bind(bindAddress); */
    /*   recv->SetRecvCallback(MakeCallback(&ReceivePacket)); */

    /*   Ptr<Socket> send = Socket::CreateSocket(*it, tid); */
    /*   InetSocketAddress destAddress = InetSocketAddress(Ipv4Address("255.255.255.255"), 42); */
    /*   send->SetAllowBroadcast(true); */
    /*   send->Connect(destAddress); */

    /*   Simulator::ScheduleWithContext(send->GetNode()->GetId(), */
    /*       Seconds(0.1), &GenerateTraffic, */
    /*       send, 1000, 100, Seconds(0.1)); */
    /* } */

    Ptr<Socket> recv = Socket::CreateSocket(c.Get(0), tid);
    InetSocketAddress bindAddress = InetSocketAddress(Ipv4Address::GetAny(), 42);
    recv->Bind(bindAddress);
    recv->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> send = Socket::CreateSocket(c.Get(1), tid);
    InetSocketAddress destAddress = InetSocketAddress(
        Ipv4Address::ConvertFrom(c.Get(0)->GetDevice(0)->GetAddress()), 42);
    send->SetAllowBroadcast(true);
    send->Connect(destAddress);

    Simulator::ScheduleWithContext(send->GetNode()->GetId(),
        Seconds(0.1), &GenerateTraffic,
        send, 1000, 100, Seconds(0.1));

    Simulator::Stop(Seconds(getSimulationDuration()));
    Simulator::Run();

    // TODO: statistics for final evaluation

    Simulator::Destroy();
    return 0;
}
