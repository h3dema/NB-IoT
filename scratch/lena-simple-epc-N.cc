/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 *
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */
#include "ns3/radio-bearer-stats-calculator.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <string>

// --- to create the directory (ns3 is using c++0x)
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

//#include "ns3/gtk-config-store.h"

#include "ns3/netanim-module.h"  // Required for NetAnim

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");



void print_positions(NodeContainer enbNodes, NodeContainer ueNodes, std::string fname)
{
  std::ofstream out(fname);
  if (!out.is_open()) {
      NS_LOG_UNCOND("Failed to open file: " << fname);
      return;
  }
  // print eNB positions
  out << "eNB Position:" << std::endl;
  Vector enbPos = enbNodes.Get(0)->GetObject<MobilityModel>()->GetPosition();
  out << "  (x=" << enbPos.x << ", y=" << enbPos.y << ")" << std::endl;
  
  // print the positions of the UE nodes
  out << "\nUE Positions:" << std::endl;
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
      Vector pos = ueNodes.Get(i)->GetObject<MobilityModel>()->GetPosition();
      out << "  UE " << i+1 << ": (x=" << pos.x << ", y=" << pos.y << ")" << std::endl;
  }
  out.close();
}


int
main (int argc, char *argv[])
{

  uint16_t numberOfeNB = 1;  // always one
  uint16_t numberOfNodes = 2;   // number of UE's
  
  double simTime = 50.2;     //Sec
  double distance = 60.0;
  double interPacketIntervaldl = 1 *1000;
  double interPacketIntervalul = 1*1000;   // 1Sseconds = 1000 Milliseconds
  bool useCa = false;
  uint16_t tc = 1;
  uint32_t packetsize = 64;
  uint8_t maxmcs = 9;

  int32_t t3324 = 50*1024;  // Milliseconds
  int64_t t3412 = 10*1024;  // Milliseconds
  int32_t edrx_cycle = 5*1024;  // Milliseconds
  uint16_t rrc_release_timer = 10*1000; // Milliseconds
  bool psm_enable = true;
  uint16_t EnableRandom = 7;

  uint8_t in_circle = 1;  // whether to place the nodes in a straight line (0) or in a circle (1)
  double circle_radius = 50.0;  // radius of the circle (eNB is the center)


  // -- create output folder
  char buffer[PATH_MAX];
  if (getcwd(buffer, sizeof(buffer)) == nullptr) {
      perror("getcwd() error");
      return 1;
  }

  std::string curr_dir(buffer);
  std::string output_dir = curr_dir + "/output";  // save to `output`
  struct stat st = {0};
  if (stat(output_dir.c_str(), &st) == -1) {
      if (mkdir(output_dir.c_str(), 0755) == 0) {
          std::cout << "Created directory: " << output_dir << std::endl;
      }
  }

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of UE", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketIntervaldl", "DL Inter packet interval [ms])", interPacketIntervaldl);
  cmd.AddValue("interPacketIntervalul", "UL Inter packet interval [ms])", interPacketIntervalul);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.AddValue("tc", "Test case number.", tc);
  cmd.AddValue("packetsize", "Packet size", packetsize);
  cmd.AddValue("t3324", " DRX timer", t3324); //<TODO: To connect maxmcs to lte-amc.cc>
  cmd.AddValue("t3412", "PSM timer", t3412);
  cmd.AddValue("edrx_cycle", "edrx Cycle", edrx_cycle);
  cmd.AddValue("rrc_release_timer", "Connected timer", rrc_release_timer);
  cmd.AddValue("psm_enable", "PSM enabling flag", psm_enable);
  cmd.AddValue("maxmcs", "Maximum MCS", maxmcs);
  cmd.AddValue("EnableRandom","EnableRandom", EnableRandom);
  cmd.Parse(argc, argv);


  //Config::SetDefault ("ns3::UeManager::m_rrcreleaseinterval", UintegerValue(20));


  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));

   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get(0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfeNB);  // just one for now
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  if (in_circle == 1) {

    // position eNB at the center of the circle
    MobilityHelper enbMobility;
    Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator>();
    enbPositionAlloc->Add(Vector(0.0, 0.0, 0.0));  // Center
    enbMobility.SetPositionAllocator(enbPositionAlloc);
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.Install(enbNodes);  
  
    // UE
    MobilityHelper ueMobility;
    Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
    ueMobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
            "X", DoubleValue(0.0),
            "Y", DoubleValue(0.0),
            "rho", DoubleValue(var->GetValue(0, circle_radius))
    );

    ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ueMobility.Install(ueNodes);
  }
  else {
    // eNB is the left most node pos=(0,0), and the rest are placed at increased distance
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for (uint16_t i = 1; i < 1 + numberOfNodes; i++)
      {
        positionAlloc->Add (Vector(distance * i, 0, 0));
      } 
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(enbNodes);
    mobility.Install(ueNodes);
  }

  // set position for remoteHostContainer
  MobilityHelper hostMobility;
  Ptr<ListPositionAllocator> hostPositionAlloc = CreateObject<ListPositionAllocator>();
  hostPositionAlloc->Add(Vector(circle_radius, circle_radius, 0.0));  // 
  hostMobility.SetPositionAllocator(hostPositionAlloc);
  hostMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  hostMobility.Install(enbNodes);  

  print_positions(enbNodes, ueNodes, output_dir + "/positions.txt");
  
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes, t3324, t3412, edrx_cycle, rrc_release_timer, psm_enable);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes, t3324, t3412,edrx_cycle);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  for (uint16_t i = 0; i < numberOfNodes; i++)
      {
        // Notice that there is only one eNB
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
        // side effect: the default EPS bearer will be activated
      }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketIntervaldl)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      dlClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
      dlClient.SetAttribute ("EnableRandom", UintegerValue (EnableRandom));
      dlClient.SetAttribute ("Percent", UintegerValue (100));
      dlClient.SetAttribute ("EnableDiagnostic", UintegerValue (0));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketIntervalul)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      ulClient.SetAttribute ("PacketSize", UintegerValue(packetsize));
      ulClient.SetAttribute ("EnableRandom", UintegerValue (EnableRandom));
      ulClient.SetAttribute ("Percent", UintegerValue (100));
      ulClient.SetAttribute ("EnableDiagnostic", UintegerValue (0));



      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
  }

  std::string str;
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  //lteHelper->EnableTraces();
 
  lteHelper->EnableUlRxPhyTraces();
  Ptr<PhyRxStatsCalculator> p1 = lteHelper->GetUlRxPhyStats();
  str = output_dir + "/UlRxPhy_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p1->SetAttribute("UlRxOutputFilename",StringValue( str));

  lteHelper->EnableUlTxPhyTraces();
  str = output_dir + "/UlTxPhy_";
  str.append(std::to_string(tc));
  str.append(".txt");
  Ptr<PhyTxStatsCalculator> p2 = lteHelper->GetUlTxPhyStats();
  p2->SetAttribute("UlTxOutputFilename",StringValue(str));

  lteHelper->EnableMacTraces();
  Ptr<MacStatsCalculator> m = lteHelper->GetMacStats();
  str = output_dir + "/UlMac_";
  str.append(std::to_string(tc));
  str.append(".txt");
  m->SetAttribute("UlOutputFilename",StringValue(str));
  str = output_dir + "/DlMac_";
  str.append(std::to_string(tc));
  str.append(".txt");
  m->SetAttribute("DlOutputFilename",StringValue(str));

  lteHelper->EnablePdcpTraces();
  Ptr<RadioBearerStatsCalculator> p = lteHelper->GetPdcpStats();  
  str = output_dir + "/UlPdcp_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p->SetAttribute("UlPdcpOutputFilename",StringValue(str));
  str = output_dir + "/DlPdcp_";
  str.append(std::to_string(tc));
  str.append(".txt");
  p->SetAttribute("DlPdcpOutputFilename",StringValue(str));
  
  lteHelper->EnableRlcTraces();
  Ptr<RadioBearerStatsCalculator> r = lteHelper->GetRlcStats();
  str = output_dir + "/UlRlc_";
  str.append(std::to_string(tc));
  str.append(".txt");
  r->SetAttribute("UlRlcOutputFilename",StringValue(str));
  str = output_dir + "/DlRlc_";
  str.append(std::to_string(tc));
  str.append(".txt");
  r->SetAttribute("DlRlcOutputFilename",StringValue(str));
   
  // NetAnim
  AnimationInterface anim(output_dir + "/netanim_output.xml");
  // UE in red
  for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
      anim.UpdateNodeDescription(ueNodes.Get(i), "UE " + std::to_string(i + 1));
      anim.UpdateNodeColor(ueNodes.Get(i), 255, 0, 0);  // Red
  }
  // eNB in blue
  anim.UpdateNodeDescription(enbNodes.Get(0), "eNB");
  anim.UpdateNodeColor(enbNodes.Get(0), 0, 0, 255);  // Blue
  // remoteHostContainer in green
  anim.UpdateNodeDescription(remoteHostContainer.Get(0), "Host");
  anim.UpdateNodeColor(remoteHostContainer.Get(0), 0, 255, 0);  // Blue
  

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

