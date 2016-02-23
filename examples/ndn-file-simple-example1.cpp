/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015-2016 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University 
 * Klagenfurt
 *
 * This file is part of AMuSt-ndnSIM, based on ndnSIM. See AUTHORS for complete list of 
 * authors and contributors.
 *
 * AMuSt-ndnSIM and ndnSIM are free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * AMuSt-ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * amus-ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

/** This example is based on ndn-simple.cpp, as seen in the ndnSIM 2.1 tutorial here: 
 * http://ndnsim.net/2.1/examples.html
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"

namespace ns3 {

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3); // 3 nodes, connected: 0 <---> 1 <---> 2

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/myprefix", "/localhost/nfd/strategy/best-route");

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::FileConsumer");
  consumerHelper.SetAttribute("FileToRequest", StringValue("/myprefix/file1.img"));

  consumerHelper.Install(nodes.Get(0)); // install to some node from nodelist

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/someuser/somedata/"));
  producerHelper.Install(nodes.Get(2)); // install to some node from nodelist

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndnGlobalRoutingHelper.AddOrigins("/myprefix", nodes.Get(2));
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(600.0));

  Simulator::Run();
  Simulator::Destroy();

  NS_LOG_UNCOND("Simulation Finished.");

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
