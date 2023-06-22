#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-module.h"
#include "ns3/config-store-module.h"
#include "ns3/antenna-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("3gppChannelNumerologiesExample");

int
main (int argc, char *argv[])
{

  // set simulation time and mobility
  double simTime = 5; // seconds
  double udpAppStartTime = 0.4; //seconds

  //other simulation parameters default values
  uint16_t numerology = 0;

  uint16_t gNbNum = 1;
  uint16_t ueNumPergNb = 6;

  double centralFrequency = 6e9;
  double bandwidth = 50e6;
  double txPower = 23;
  double lambda = 1000;
  uint32_t udpPacketSize = 1500;
  bool udpFullBuffer = true;
  uint8_t fixedMcs = 28;
  bool useFixedMcs = true;
  // Where we will store the output files.
  std::string simTag = "default";
  std::string outputDir = "./";


  CommandLine cmd;

  cmd.AddValue ("numerology",
                "The numerology to be used.",
                numerology);
  cmd.AddValue ("frequency",
                "The system frequency",
                centralFrequency);
  cmd.AddValue ("udpFullBuffer",
                "Whether to set the full buffer traffic; if this parameter is set then the udpInterval parameter"
                "will be neglected",
                udpFullBuffer);
  cmd.AddValue ("simTime",
                "Total simulation time",
                simTime);  
                        
  cmd.Parse (argc, argv);

  // setup the nr simulation
  Ptr<NrHelper> nrHelper = CreateObject<NrHelper> ();

  /*
   * Spectrum division. We create one operation band with one component carrier
   * (CC) which occupies the whole operation band bandwidth. The CC contains a
   * single Bandwidth Part (BWP). This BWP occupies the whole CC band.
   * Both operational bands will use the StreetCanyon channel modeling.
   */
  CcBwpCreator ccBwpCreator;
  const uint8_t numCcPerBand = 1;  // in this example, both bands have a single CC
  BandwidthPartInfo::Scenario scenario = BandwidthPartInfo::UMa_LoS;
  if (ueNumPergNb  > 1)
    {
      scenario = BandwidthPartInfo::InH_OfficeOpen;
    }


  // Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
  // a single BWP per CC
  CcBwpCreator::SimpleOperationBandConf bandConf (centralFrequency,
                                                  bandwidth,
                                                  numCcPerBand,
                                                  scenario);

  // By using the configuration created, it is time to make the operation bands
  OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc (bandConf);

  /*
   * Initialize channel and pathloss, plus other things inside band1. If needed,
   * the band configuration can be done manually, but we leave it for more
   * sophisticated examples. For the moment, this method will take care
   * of all the spectrum initialization needs.
   */
  nrHelper->InitializeOperationBand (&band);

  BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps ({band});

  /*
   * Continue setting the parameters which are common to all the nodes, like the
   * gNB transmit power or numerology.
   */
  nrHelper->SetGnbPhyAttribute ("TxPower", DoubleValue (txPower));
  nrHelper->SetGnbPhyAttribute ("Numerology", UintegerValue (numerology));

  // Scheduler
  nrHelper->SetSchedulerTypeId (TypeId::LookupByName ("ns3::NrMacSchedulerTdmaRR"));
  nrHelper->SetSchedulerAttribute ("FixedMcsDl", BooleanValue (useFixedMcs));
  nrHelper->SetSchedulerAttribute ("FixedMcsUl", BooleanValue (useFixedMcs));

  if (useFixedMcs)
    {
      nrHelper->SetSchedulerAttribute ("StartingMcsDl", UintegerValue (fixedMcs));
      nrHelper->SetSchedulerAttribute ("StartingMcsUl", UintegerValue (fixedMcs));
    }

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (999999999));

  // Antennas for all the UEs
  nrHelper->SetUeAntennaAttribute ("NumRows", UintegerValue (2));
  nrHelper->SetUeAntennaAttribute ("NumColumns", UintegerValue (4));
  nrHelper->SetUeAntennaAttribute ("AntennaElement", PointerValue (CreateObject<IsotropicAntennaModel> ()));

  // Antennas for all the gNbs
  nrHelper->SetGnbAntennaAttribute ("NumRows", UintegerValue (4));
  nrHelper->SetGnbAntennaAttribute ("NumColumns", UintegerValue (8));
  nrHelper->SetGnbAntennaAttribute ("AntennaElement", PointerValue (CreateObject<ThreeGppAntennaModel> ()));

  // Beamforming method
  Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
  idealBeamformingHelper->SetAttribute ("BeamformingMethod", TypeIdValue (DirectPathBeamforming::GetTypeId ()));
  nrHelper->SetBeamformingHelper (idealBeamformingHelper);

  Config::SetDefault ("ns3::ThreeGppChannelModel::UpdatePeriod",TimeValue (MilliSeconds (0)));
  nrHelper->SetPathlossAttribute ("ShadowingEnabled", BooleanValue (false));

  // Error Model: UE and GNB with same spectrum error model.
  nrHelper->SetUlErrorModel ("ns3::NrEesmIrT1");
  nrHelper->SetDlErrorModel ("ns3::NrEesmIrT1");

  // Both DL and UL AMC will have the same model behind.
  nrHelper->SetGnbDlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
  nrHelper->SetGnbUlAmcAttribute ("AmcModel", EnumValue (NrAmc::ShannonModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel


  // Create EPC helper
  Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper> ();
  nrHelper->SetEpcHelper (epcHelper);
  // Core latency
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (2)));

  // gNb routing between Bearer and bandwidh part
  uint32_t bwpIdForBearer = 0;
  nrHelper->SetGnbBwpManagerAlgorithmAttribute ("GBR_CONV_VOICE", UintegerValue (bwpIdForBearer));

  // Initialize nrHelper
  nrHelper->Initialize ();


  /*
   *  Create the gNB and UE nodes according to the network topology
   */
  NodeContainer gNbNodes;
  NodeContainer ueNodes;
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> utPositionAlloc = CreateObject<ListPositionAllocator> ();

  const double gNbHeight = 10;
  const double ueHeight = 1.5;
  int uesCoordsX[] = {10, 1000, 3000, -10, -1000, -3000};
  int ues_y = 0;

    gNbNodes.Create (gNbNum);
    ueNodes.Create (ueNumPergNb * gNbNum);

    // setting position for gNb and ues
	bsPositionAlloc->Add(Vector(0, 0, gNbHeight));
	for (auto ues_x: uesCoordsX)	utPositionAlloc->Add(Vector(ues_x, ues_y, ueHeight));

    
  mobility.SetPositionAllocator (bsPositionAlloc);
  mobility.Install (gNbNodes);

  mobility.SetPositionAllocator (utPositionAlloc);
  mobility.Install (ueNodes);


  // Install nr net devices
  NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice (gNbNodes,
                                                             allBwps);

  NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice (ueNodes,
                                                           allBwps);

  int64_t randomStream = 1;
  randomStream += nrHelper->AssignStreams (gNbNetDev, randomStream);
  randomStream += nrHelper->AssignStreams (ueNetDev, randomStream);


  // When all the configuration is done, explicitly call UpdateConfig ()

  for (auto it = gNbNetDev.Begin (); it != gNbNetDev.End (); ++it) {
      DynamicCast<NrGnbNetDevice> (*it)->UpdateConfig ();
    }

  for (auto it = ueNetDev.Begin (); it != ueNetDev.End (); ++it) {
      DynamicCast<NrUeNetDevice> (*it)->UpdateConfig ();
    }

  // create the internet and install the IP stack on the UEs
  // get SGW/PGW and create a single RemoteHost
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // connect a remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("10Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.05)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  internet.Install (ueNodes);


  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

  // Set the default gateway for the UEs
  for (uint32_t j = 0; j < ueNodes.GetN (); ++j)
    {
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (j)->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // attach UEs to the closest eNB
  nrHelper->AttachToClosestEnb (ueNetDev, gNbNetDev);

  // assign IP address to UEs, and install UDP downlink applications
  uint16_t dlPort = 1234;

  ApplicationContainer serverApps;

  // The sink will always listen to the specified ports
  UdpServerHelper dlPacketSinkHelper (dlPort);
  serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get (0)));

  UdpClientHelper dlClient;
  dlClient.SetAttribute ("RemotePort", UintegerValue (dlPort));
  dlClient.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));
  dlClient.SetAttribute ("MaxPackets", UintegerValue (0xFFFFFFFF));
  if (udpFullBuffer)
    {
      double bitRate = 30000000; // 30 Mbps will saturate the NR system of 20 MHz with the NrEesmIrT1 error model
      bitRate /= ueNumPergNb;    // Divide the cell capacity among UEs
      if (bandwidth > 20e6)
        {
          bitRate *=  bandwidth / 20e6;
        }
      lambda = bitRate / static_cast<double> (udpPacketSize * 8);
    }
  dlClient.SetAttribute ("Interval", TimeValue (Seconds (1.0 / lambda)));

  // The bearer that will carry low latency traffic
  EpsBearer bearer (EpsBearer::GBR_CONV_VOICE);

  Ptr<EpcTft> tft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf;
  dlpf.localPortStart = dlPort;
  dlpf.localPortEnd = dlPort;
  tft->Add (dlpf);

  /*
   * Let's install the applications!
   */
  ApplicationContainer clientApps;

  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      Ptr<Node> ue = ueNodes.Get (i);
      Ptr<NetDevice> ueDevice = ueNetDev.Get (i);
      Address ueAddress = ueIpIface.GetAddress (i);

      // The client, who is transmitting, is installed in the remote host,
      // with destination address set to the address of the UE
      dlClient.SetAttribute ("RemoteAddress", AddressValue (ueAddress));
      clientApps.Add (dlClient.Install (remoteHost));

      // Activate a dedicated bearer for the traffic type
      nrHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, tft);
    }

  // start server and client apps
  serverApps.Start (Seconds (udpAppStartTime));
  clientApps.Start (Seconds (udpAppStartTime));
  serverApps.Stop (Seconds (simTime));
  clientApps.Stop (Seconds (simTime));

  FlowMonitorHelper flowmonHelper;
  NodeContainer endpointNodes;
  endpointNodes.Add (remoteHost);
  endpointNodes.Add (ueNodes);

  Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install (endpointNodes);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();


  // Print per-flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

  double averageFlowThroughput = 0.0;
  double averageFlowDelay = 0.0;
  double averagePacketLoss = 0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        // Measure the duration of the flow from receiver's perspective
        double rxDuration = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();

        averageFlowThroughput += i->second.rxBytes * 8.0 / rxDuration / 1000 / 1000;
        averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;
        
        averagePacketLoss += (i->second.txPackets - i->second.rxPackets);
      }
      
  std::cout << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size () << "\n";
  std::cout << "  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";
  std::cout << "  Mean Packet Loss: " << averagePacketLoss / stats.size() << "\n";
  
  Simulator::Destroy ();
  return 0;
}

