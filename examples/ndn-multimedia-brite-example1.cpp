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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"

#include "ns3/ndnSIM/apps/ndn-app.hpp"

#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-brite-helper.hpp"

namespace ns3 {


void
FileDownloadedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, double downloadSpeed, long milliSeconds)
{
  std::cout << "Trace: File finished downloading: " << Simulator::Now().GetMilliSeconds () << " "<< *interestName <<
     " Download Speed: " << downloadSpeed/1000.0 << " Kilobit/s in " << milliSeconds << " ms" << std::endl;
}

void
FileDownloadedManifestTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, long fileSize)
{
  std::cout << "Trace: Manifest received: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << " File Size: " << fileSize << std::endl;
}

void
FileDownloadStartedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName)
{
  std::cout << "Trace: File started downloading: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << std::endl;
}


int
main(int argc, char* argv[])
{
  std::string confFile = "brite.conf";

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue ("briteConfFile", "BRITE configuration file", confFile);
  cmd.Parse(argc, argv);

  // Create NDN Stack
  ndn::StackHelper ndnHelper;

  // Create Brite Topology Helper
  ndn::NDNBriteHelper bth (confFile);
  bth.AssignStreams (3);
  // tell the topology helper to build the topology 
  bth.BuildBriteTopology ();

  // Separate clients, servers and routers
  NodeContainer client;
  NodeContainer server;
  NodeContainer router;

  uint32_t sumLeafNodes = 0;


  for (uint32_t i = 0; i < bth.GetNAs(); i++)
  {
    std::cout << "Number of nodes for AS: " << bth.GetNNodesForAs(i) << ", non leaf nodes: " << bth.GetNNonLeafNodesForAs(i) << std::endl;
    for(uint32_t node=0; node < bth.GetNNonLeafNodesForAs(i); node++)
    {
      std::cout << "Node " << node << " has " << bth.GetNonLeafNodeForAs(i,node)->GetNDevices() << " devices " << std::endl;
      //container.Add (briteHelper->GetNodeForAs (ASnumber,node));
      router.Add(bth.GetNonLeafNodeForAs(i,node));
    }
  }

  for (uint32_t i = 0; i < bth.GetNAs(); i++)
  {
    uint32_t numLeafNodes = bth.GetNLeafNodesForAs(i);

    std::cout << "AS " << i << " has " << numLeafNodes << "leaf nodes! " << std::endl;

    for (uint32_t j= 0; j < numLeafNodes; j++)
    {
      if (i * 100 + j % 50 == 0) // some strategy to decide whether this node should be a server or a client
      {
        server.Add(bth.GetLeafNodeForAs(i,j));
      }
      else
      {
        client.Add(bth.GetLeafNodeForAs(i,j));
      }
    }

    sumLeafNodes+= numLeafNodes;
  }

  std::cout << "Total Number of leaf nodes: " << sumLeafNodes << std::endl;


  // clients do not really need a large content store, but it could be beneficial to give them some
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "100");
  ndnHelper.Install (client);

  // servers do not need a content store at all, they have an app to do that
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1");
  ndnHelper.Install (server);

  // what really needs a content store is the routers, which we don't have many
  ndnHelper.setCsSize(10000);
  ndnHelper.Install(router);

  //ndnHelper.SetDefaultRoutes(true);

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/myprefix", "/localhost/nfd/strategy/best-route");
  // ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/broadcast");


  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();


  // Installing multimedia consumer
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr::MultimediaConsumer");
  consumerHelper.SetAttribute("AllowUpscale", BooleanValue(true));
  consumerHelper.SetAttribute("AllowDownscale", BooleanValue(false));
  consumerHelper.SetAttribute("ScreenWidth", UintegerValue(1920));
  consumerHelper.SetAttribute("ScreenHeight", UintegerValue(1080));
  consumerHelper.SetAttribute("StartRepresentationId", StringValue("auto"));
  consumerHelper.SetAttribute("MaxBufferedSeconds", UintegerValue(30));
  consumerHelper.SetAttribute("StartUpDelay", StringValue("0.1"));

  consumerHelper.SetAttribute("AdaptationLogic", StringValue("dash::player::SVCBufferBasedAdaptationLogic"));

  // Randomize Client File Selection
  Ptr<UniformRandomVariable> r = CreateObject<UniformRandomVariable>();
  for(uint32_t i=0; i<client.size (); i++)
  {
    // TODO: Make some logic to decide which file to request
    consumerHelper.SetAttribute("MpdFileToRequest", StringValue(std::string("/myprefix/SVC/BBB/BBB-III.mpd" )));
    ApplicationContainer consumer = consumerHelper.Install (client[i]);

    std::cout << "Client " << i << " is Node " << client[i]->GetId() << std::endl;

    // Start and stop the consumer
    consumer.Start (Seconds(1.0)); // TODO: Start at randomized time
    consumer.Stop (Seconds(600.0));
  }


  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/FileDownloadFinished",
                               MakeCallback(&FileDownloadedTrace));


   // Producer
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");

  // Producer will reply to all requests starting with /myprefix
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/someuser/multimediaData/"));
  producerHelper.Install(server); // install to servers

  ndnGlobalRoutingHelper.AddOrigins("/myprefix", server);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();
  // ndn::GlobalRoutingHelper::CalculateRoutes();


  Simulator::Stop(Seconds(1000.0));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation ended" << std::endl;

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}


