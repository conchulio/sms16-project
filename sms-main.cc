#include "sms-helpers.h"
#include "sms-echo-helper.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE("SMSProject");

int main(int argc, char* argv[]) {
    LogComponentEnable("SMSProject", LOG_LEVEL_INFO);
    LogComponentEnable("SmsEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("SmsEchoServerApplication", LOG_LEVEL_INFO);
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

    for (size_t i = 0; i < c.GetN(); i++) {
        std::vector<FileSMS> files = getInitialFileList();
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
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));

    Simulator::Stop(Seconds(getSimulationDuration()));
    Simulator::Run();

    // TODO: statistics for final evaluation

    Simulator::Destroy();
    return 0;
}
