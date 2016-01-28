#include "sms-helpers.h"
#include "sms-echo-helper.h"
#include <iostream>
#include <set>
#include <fstream>

NS_LOG_COMPONENT_DEFINE("SMSProject");

int main(int argc, char* argv[]) {
    LogComponentEnable("SMSProject", LOG_LEVEL_INFO);
    LogComponentEnable("SmsEchoClientApplication", LOG_LEVEL_WARN);
    NS_LOG_UNCOND("sms16");

    NodeContainer c;
    c.Create(getNumberOfMobileNodes());

    installMobility(c);

    NetDeviceContainer netDevices;
    installWifi(c, netDevices);

    InternetStackHelper internet;
    internet.Install(c);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(netDevices);

    std::vector< std::vector<FileSMS> > nodeFileList;
    std::set< int > file_set;

    std::ofstream results("results.txt");
    results << "Files per node in the beginning: " << std::endl;
    uint32_t total_num_of_files_in_the_beginning = 0;
    for (size_t i = 0; i < c.GetN(); i++) {
        results << "Node " << i << std::endl;
        std::vector<FileSMS> files = getInitialFileList();
        total_num_of_files_in_the_beginning += files.size();
        for (size_t j = 0; j < files.size(); j++) {
          results << "File " << files[j].getFileId() << std::endl;
          file_set.insert(files[j].getFileId());
        }
        nodeFileList.push_back(files);
        // std::vector<FileSMS>::const_iterator it;
        // for (it = files.begin(); it != files.end(); ++it) {
          /* std::cout << "File " << it->getFileId() << " size: " << it->getFileSize() << std::endl; */
        // }
        // ...
        // The 'files' vector contains all the FileSMS objects that the node has
        // when the simulation starts. While in this example the information is
        // thrown away, the student must store it somewhere and use it appropriately.
        // ...
    }

    uint16_t port = 42;
    // SmsEchoServerHelper server (port);
    // ApplicationContainer apps_server = server.Install(c.Get(0));
    // apps_server.Start(Seconds(1.0));
    // apps_server.Stop(Seconds(10.0));
    Address serverAddress = Address(Ipv4Address("255.255.255.255"));

    uint32_t packetSize = 1024;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds(1.0);
    SmsEchoClientHelper client (serverAddress, port);
    client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    client.SetAttribute("Interval", TimeValue(interPacketInterval));
    client.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer apps = client.Install(c);
    for (uint32_t i = 0; i < c.GetN(); i++) {
      SmsEchoClient* smsApp = static_cast<SmsEchoClient*> (&(*(c.Get(i)->GetApplication(0))));
      smsApp->SetIPAdress(interfaces.Get(i).first->GetAddress(1,0).GetLocal());
      smsApp->SetFiles (nodeFileList[i]);
    }
    // Why does it start at two seconds?
    apps.Start(Seconds(2.0));
    // apps.Stop(Seconds(10.0)); // That's the Default
    apps.Stop(Seconds(getSimulationDuration()));
    // It takes around 90 seconds to distribute all files

    Simulator::Stop(Seconds(getSimulationDuration()));
    Simulator::Run();

    // TODO: statistics for final evaluation
    results << "Files per node in the end: " << std::endl;
    std::set< int > file_set_in_the_end;
    uint32_t total_number_of_full_files = 0;
    for (uint32_t i = 0; i < c.GetN(); i++) {
      results << "Node " << i << std::endl;
      SmsEchoClient* smsApp = static_cast<SmsEchoClient*> (&(*(c.Get(i)->GetApplication(0))));
      std::vector<FileSMSChunks> files_in_the_end = smsApp->files;
      for (uint32_t j = 0; j < files_in_the_end.size(); j++) {
        if (files_in_the_end[j].is_full()) {
          results << "File " << files_in_the_end[j].getFileId() << std::endl;
          total_number_of_full_files++;
          file_set_in_the_end.insert(files_in_the_end[j].getFileId());
        }
      }
    }

    NS_LOG_UNCOND("Stopped at time " << Simulator::Now ().GetSeconds () << " Unique files in the beginning: " << file_set.size() << " Total number of full files in the beginnig: " <<
    total_num_of_files_in_the_beginning << ", full files in the end: " << total_number_of_full_files << " unique files in the end " << file_set_in_the_end.size());

    Simulator::Destroy();
    return 0;
}
