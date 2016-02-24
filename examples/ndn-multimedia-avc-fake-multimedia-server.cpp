/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University 
 * Klagenfurt
 *
 * This file is part of amus-ndnSIM, based on ndnSIM. See AUTHORS for complete list of 
 * authors and contributors.
 *
 * amus-ndnSIM and ndnSIM are free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * amus-ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * amus-ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

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
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("10Mbps"));
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
  ndnHelper.setCsSize(0);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "100");
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/myprefix", "/localhost/nfd/strategy/best-route");

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  consumerHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::RateAndBufferBasedAdaptationLogic"));
  consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/FakeVid1/vid1.mpd" )));

  //consumerHelper.SetPrefix (std::string("/Server_" + boost::lexical_cast<std::string>(i%server.size ()) + "/layer0"));
  ApplicationContainer app1 = consumerHelper.Install (nodes.Get(2));

  ndn::AppHelper fakeDASHProducerHelper("ns3::ndn::FakeMultimediaServer");

  // This fake multimedia producer will reply to all requests starting with /myprefix/FakeVid1
  fakeDASHProducerHelper.SetPrefix("/myprefix/FakeVid1");
  fakeDASHProducerHelper.SetAttribute("MetaDataFile", StringValue("representations/netflix_vid1.csv"));
  // We just give the MPD file a name that makes it unique
  fakeDASHProducerHelper.SetAttribute("MPDFileName", StringValue("vid1.mpd"));

  fakeDASHProducerHelper.Install(nodes.Get(0));

  // We can install more then one fake multimedia producer on one node:

  // This fake multimedia producer will reply to all requests starting with /myprefix/FakeVid2
  fakeDASHProducerHelper.SetPrefix("/myprefix/FakeVid2");
  fakeDASHProducerHelper.SetAttribute("MetaDataFile", StringValue("representations/netflix_vid2.csv"));
  // We just give the MPD file a name that makes it unique
  fakeDASHProducerHelper.SetAttribute("MPDFileName", StringValue("vid2.mpd"));


  // This fake multimedia producer will reply to all requests starting with /myprefix/FakeVid3
  fakeDASHProducerHelper.SetPrefix("/myprefix/FakeVid3");
  fakeDASHProducerHelper.SetAttribute("MetaDataFile", StringValue("representations/netflix_vid3.csv"));
  // We just give the MPD file a name that makes it unique
  fakeDASHProducerHelper.SetAttribute("MPDFileName", StringValue("vid3.mpd"));

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndnGlobalRoutingHelper.AddOrigins("/myprefix", nodes.Get(0));
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(60.0));

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



