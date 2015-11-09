/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University 
 * Klagenfurt
 * Maintained by Christian Kreuzberger (christian.kreuzberger@itec.aau.at)
 *
 *
 * This program is based on the ns-3 version (brite-topology-helper). See 
 * https://www.nsnam.org for more information about ns-3. See 
 * https://www.nsnam.org/docs/models/html/brite.html for more information about brite
 * and ns-3.
 * This program is a custom extension for ndnSIM 2.0/2.1. See http://ndnsim.net and 
 * https://github.com/named-data/ndnSIM for more information regarding ndnSIM.
 * This program relies on BRITE (see http://www.cs.bu.edu/brite/) to be compiled.
 * 
 *
 * Acknowledgements:
 * This work was partially funded by the Austrian Science Fund (FWF) under the 
 * CHIST-ERA project CONCERT (A Context-Adaptive Content Ecosystem Under Uncertainty), 
 * project number I1402.
 *
 * 
 * License:
 * This program is free software: you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_BRITE_HELPER
#define NDN_BRITE_HELPER

#include <string>
#include <vector>

#include "ns3/channel.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ndnSIM/helper/ndn-face-container.hpp"
#include "ns3/random-variable-stream.h"



//located in BRITE source directory
#include "Brite.h"

namespace ns3 {
namespace ndn {


/**
 * \defgroup brite BRITE Topology Generator for ndnSIM
 *
 * Create topologies with BRITE for ndnSIM.
 */

/**
 * \ingroup brite
 * \brief Interface with BRITE, the Boston university Representative Internet
 *        Topology gEnerator (see http://www.cs.bu.edu/brite/ )
 *
 * This helper class creates an interface with BRITE and allows the user to
 * easily create ns-3 topologies from BRITE generated graphs. This class
 * accepts a BRITE configuration and seed file, much like the stand-alone
 * BRITE software. Using these files, BRITE generates a graph which is
 * stored herein. ns-3 examples can then grab the BRITE generated nodes and
 * edges from this helper and create ns-3 specific topologies.
 *
 */

class NDNBriteHelper
{
public:
  /**
   * Construct a NDNBriteHelper
   *
   * \param confFile a BRITE configuration file
   * \param seedFile a BRITE seed file
   * \param newseedFile a BRITE seed file with newly generated values
   */
  NDNBriteHelper (std::string confFile,
                       std::string seedFile,
                       std::string newseedFile);

  /**
   * Construct a NDNBriteHelper using NS3 to generate seed values
   * need by BRITE
   *
   */
  NDNBriteHelper (std::string confFile);

  ~NDNBriteHelper ();

  /**
   * Assigns stream number to UniformRandomVariable used to
   * generate brite seed file
   *
   * \param streamNumber the stream number to assign
   *
   */
  void AssignStreams (int64_t streamNumber);

  /**
   *  Create NS3 topology using information generated from BRITE.
   */
  void BuildBriteTopology ();

  /**
   * Create NS3 topology using information generated from BRITE and configure topology for MPI use.
   *
   * \param systemCount The number of MPI instances to be used in the simulation.
   *
   */
  void BuildBriteTopology (const uint32_t systemCount);

  /**
   * Returns the number of router leaf nodes for a given AS
   *
   * \param asNum the AS number
   * \returns the number of leaf nodes in the specified AS
   *
   */
  uint32_t GetNLeafNodesForAs (uint32_t asNum);

  /**
   * Returns a given router leaf node from a given AS
   *
   * \param asNum the AS number
   * \param leafNum the leaf number
   * \returns the specified node
   */
  Ptr<Node> GetLeafNodeForAs (uint32_t asNum, uint32_t leafNum);

  /**
   * Returns the number of router non-leaf nodes for a given AS
   *
   * \param asNum the AS number
   * \returns the number of non-leaf nodes in the specified AS
   *
   */
  uint32_t GetNNonLeafNodesForAs (uint32_t asNum);

  /**
   * Returns a given router non-leaf node from a given AS
   *
   * \param asNum the AS number
   * \param nodeNum the node number
   * \returns the specified node
   */
  Ptr<Node> GetNonLeafNodeForAs (uint32_t asNum, uint32_t nodeNum);


  /**
   * Returns the total number of nodes for a given AS
   *
   * \param asNum the AS number
   * \returns the total number of nodes in the given AS
   */
  uint32_t GetNNodesForAs (uint32_t asNum);

  /**
   * Returns a given router node for a given AS
   *
   * \param asNum the AS number
   * \return the specified node
   *
   */
  Ptr<Node> GetNodeForAs (uint32_t asNum, uint32_t nodeNum);

  /**
    * Returns the number of AS created in the topology
    *
    * \returns the number of AS created in the topology
    */
  uint32_t GetNAs (void) const;

  /**
    * Returns the system number for the MPI instance that this AS is assigned to.  Will always return 0 if MPI not used
    *
    * \returns The system number that the specified AS number belongs to
    *
    * param asNum The AS Number
    */
  uint32_t GetSystemNumberForAs (uint32_t asNum) const;

  /**
    * Returns the number of nodes created within
    * the topology
    *
    * \returns the total number of nodes within the brite topology
    */
  uint32_t GetNNodesTopology () const;

  /**
    * Returns the number of edges created within
    * the topology
    *
    * \returns the total number of edges within the brite topology
    */
  uint32_t GetNEdgesTopology () const;

private:
  //brite values are unitless however all examples provided use mbps to specify rate
  //this constant value is used to convert the mbps provided by brite to bps.
  static const int mbpsToBps = 1000000;

  /**
   * \brief Node information from BRITE
   *
   * The BRITE code generates a graph and returns
   * information on the nodes generated. This is
   * stored here in a struct.
   */
  struct BriteNodeInfo
  {
    int nodeId;
    double xCoordinate;
    double yCoordinate;
    int inDegree;
    int outDegree;
    int asId;
    std::string type;
  };

  /**
   * \brief Edge information from BRITE
   *
   * The BRITE code generates a graph and returns
   * information on the edges generated. This is
   * stored here in a struct.
   */
  struct BriteEdgeInfo
  {
    int edgeId;
    int srcId;
    int destId;
    double length;
    double delay;
    double bandwidth;
    int asFrom;
    int asTo;
    std::string type;
  };

  //stores all of the nodes used in the BRITE generated topology
  NodeContainer m_nodes;

  /**
    * \internal
    */
  void BuildBriteNodeInfoList (void);

  /**
    * \internal
    */
  void BuildBriteEdgeInfoList (void);

  /**
    * \internal
    */
  void ConstructTopology (void);

  /**
    * \internal
    */
  void GenerateBriteTopology (void);

  /// brite configuration file to use
  std::string m_confFile;

  /// brite seed file to use
  std::string m_seedFile;

  /// brite seed file to generate for next run
  std::string m_newSeedFile;

  /// stores the number of AS in the BRITE generated topology
  uint32_t m_numAs;

  /// stores the netdevices created for each AS
  std::vector<NetDeviceContainer*> m_netDevices;

  /// stores the leaf router nodes for each AS
  std::vector<NodeContainer*> m_asLeafNodes;

  /// stores all non leaf router nodes for each AS
  std::vector<NodeContainer*> m_asNonLeafNodes;

  /// stores all of the nodes in the brite topology by AS number
  std::vector<NodeContainer*> m_nodesByAs;

  /// stores the MPI system number each AS assigned to.  All assigned to 0 if MPI not used.
  std::vector<int> m_systemForAs;

  /// the Brite topology
  brite::Topology* m_topology;

  /// stores the number of nodes created in the BRITE topology
  uint32_t m_numNodes;

  /// stores the number of edges created in the BRITE topology
  uint32_t m_numEdges;

  /**
   * The BRITE code generates multiple nodes and edges. Each
   * node and edge is stored in a BriteNodeInfo or BriteEdgeInfo
   * struct, and each instance is stored in a vector.
   * @{
   */
  typedef std::vector<BriteNodeInfo> BriteNodeInfoList;
  typedef std::vector<BriteEdgeInfo> BriteEdgeInfoList;

  BriteNodeInfoList m_briteNodeInfoList;
  BriteEdgeInfoList m_briteEdgeInfoList;
  /**@}*/

  /// used to create the links within the topology
  PointToPointHelper m_britePointToPointHelper;

  /// random variable stream for brite seed file
  Ptr<UniformRandomVariable> m_uv;
};

} // namespace ndn
} // namespace ns3

#endif /* BRITE_TOPOLOGY_HELPER_H */

