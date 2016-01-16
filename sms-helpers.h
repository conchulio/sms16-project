#ifndef SMS_HELPERS_H
#define SMS_HELPERS_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/random-variable.h"
#include "cstdio"

#include <algorithm>
#include <vector>

using namespace ns3;
using std::size_t;


class FileSMS
{
public:
    FileSMS(unsigned int id, size_t size);
    unsigned int getFileId() const;
    size_t getFileSize() const;
    bool operator==(const FileSMS &other) const;
    bool operator!=(const FileSMS &other) const;

private:
    unsigned int mId;
    size_t mFileSize;
};


/**
 * Returns the total number of mobile nodes running in the simulation
 */
unsigned int getNumberOfMobileNodes();

/**
 * Returns the duration of the simulation, i.e. the amount of seconds
 * after which the simulation must be stopped
 */
double getSimulationDuration();

/**
 * Installs the mobility component on all the nodes in NodeContainer c
 */
void installMobility(NodeContainer &c);

/**
 * Install a WiFiNetDevice on each node in NodeContainer c and sets the WiFi parameters.
 * The resulting NetDevices will be stored inside the NetDeviceContainer passed as parameter
 * (the function will overwrite the NetDeviceContainer).
 */
void installWifi(NodeContainer &c, NetDeviceContainer &devices);

/**
 * This function returns the files that are available in a mobile node at the beginning of the simulation.
 * The student has to call getInitialFileList exactly once for each mobile node in the simulation.
 * Each FileSMS object has an id and a size in KB. Different files may have different sizes.
 * The popularity of different files may differ. The popularity of a file is unknown at the beginning of the simulation.
 */
std::vector<FileSMS> getInitialFileList();
#endif // SMS_HELPERS_H
