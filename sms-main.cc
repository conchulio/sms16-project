#include "sms-helpers.h"

NS_LOG_COMPONENT_DEFINE("SMSProject");

int main(int argc, char* argv[]) {
    LogComponentEnable("SMSProject", LOG_LEVEL_INFO);

    NodeContainer c;
    c.Create(getNumberOfMobileNodes());

    installMobility(c);

    NetDeviceContainer netDevices;
    installWifi(c, netDevices);

    for (size_t i = 0; i < c.GetN(); i++) {
        std::vector<FileSMS> files = getInitialFileList();
        std::vector<FileSMS>::const_iterator it;
        for (it = files.begin(); it != files.end(); ++it) {
            NS_LOG_DEBUG("File " << it->getFileId() << " size: " << it->getFileSize());
        }
        // ...
        // The 'files' vector contains all the FileSMS objects that the node has
        // when the simulation starts. While in this example the information is
        // thrown away, the student must store it somewhere and use it appropriately.
        // ...
    }

    // ...

    Simulator::Stop(Seconds(getSimulationDuration()));
    Simulator::Run();

    // TODO: statistics for final evaluation

    Simulator::Destroy();
    return 0;
}
