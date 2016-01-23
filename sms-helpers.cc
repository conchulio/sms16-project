#include "sms-helpers.h"

FileSMS::FileSMS(unsigned int id, size_t size)
  : mId(id)
    , mFileSize(size)
{
}

FileSMS::~FileSMS() {}

unsigned int FileSMS::getFileId() const {
  return mId;
}

size_t FileSMS::getFileSize() const {
  return mFileSize;
}

bool FileSMS::operator==(const FileSMS &other) const {
  return mId == other.mId;
}

bool FileSMS::operator!=(const FileSMS &other) const {
  return !(*this == other);
}

/**
 * Returns the total number of mobile nodes running in the simulation
 */
unsigned int getNumberOfMobileNodes() {
    return 25; // This is just an example, the number may be different
}

/**
 * Returns the duration of the simulation, i.e. the amount of seconds
 * after which the simulation must be stopped
 */
double getSimulationDuration() {
    return 900.0; // This is just an example, the number may be different
}

/**
 * Installs the mobility component on all the nodes in NodeContainer c
 */
void installMobility(NodeContainer &c) {
    MobilityHelper mobility;

    // These are just examples, the parameters may be different
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(5),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));

    mobility.Install(c);
}

/**
 * Install a WiFiNetDevice on each node in NodeContainer c and sets the WiFi parameters.
 * The resulting NetDevices will be stored inside the NetDeviceContainer passed as parameter
 * (the function will overwrite the NetDeviceContainer).
 */
void installWifi(NodeContainer &c, NetDeviceContainer &devices) {
    // Modulation and wifi channel bit rate
    std::string phyMode("OfdmRate24Mbps");

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange", DoubleValue(25.0));
    wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                   "m0", DoubleValue(1.0),
                                   "m1", DoubleValue(1.0),
                                   "m2", DoubleValue(1.0));
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a non-QoS upper mac
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");

    // Disable rate control
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue(phyMode),
                                 "ControlMode", StringValue(phyMode));

    devices = wifi.Install(wifiPhy, wifiMac, c);
    wifiPhy.EnablePcap ("sms16", devices);
}

/**
 * This function returns the files that are available in a mobile node at the beginning of the simulation.
 * The student has to call getInitialFileList exactly once for each mobile node in the simulation.
 * Each FileSMS object has an id and a size in KB. Different files may have different sizes.
 * The popularity of different files may differ. The popularity of a file is unknown at the beginning of the simulation.
 */
std::vector<FileSMS> getInitialFileList() {
    // This is just an example, the implementation used in the evaluation may differ in:
    //   - The value of totalFileCount (size of the entire catalog)
    //   - File popularity distribution
    //   - Max number of files per node
    //   - File size (keep in mind that different files may have different sizes)
    //   - Something else

    static const unsigned int totalFileCount = 100;
    static const unsigned int maxFileCountPerNode = 10;
    std::vector<FileSMS> files;

    // selecting number of files the node will store
    UniformVariable numOfFilesRand;
    unsigned int numOfFiles = numOfFilesRand.GetInteger(1, maxFileCountPerNode);
    files.reserve(numOfFiles);

    // selecting file id and file size
    UniformVariable fileSizeRand;
    ZipfVariable zipfRandom = ZipfVariable(totalFileCount, 1.1);
    for (unsigned int i = 0; i < numOfFiles; i++) {
        FileSMS file(zipfRandom.GetInteger(), 1000);
        std::vector<FileSMS>::const_iterator iter = std::find(files.begin(), files.end(), file);
        while (iter != files.end()) {
            file = FileSMS(zipfRandom.GetInteger(), 1000);
            iter = std::find(files.begin(), files.end(), file);
        }
        files.push_back(file);
    }

    return files;
}
