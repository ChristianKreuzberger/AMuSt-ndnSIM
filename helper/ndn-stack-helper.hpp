/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDN_STACK_HELPER_H
#define NDN_STACK_HELPER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ptr.h"
#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"

#include "ndn-face-container.hpp"
#include "ndn-fib-helper.hpp"
#include "ndn-strategy-choice-helper.hpp"

namespace ns3 {

class Node;

namespace ndn {

class NetDeviceFace;
class L3Protocol;

/**
 * @ingroup ndn
 * @defgroup ndn-helpers Helpers
 */
/**
 * @ingroup ndn-helpers
 * @brief Helper class to install NDN stack and configure its parameters
 */
class StackHelper : boost::noncopyable {
public:
  /**
   * \brief Create a new NdnStackHelper with a default NDN_FLOODING forwarding stategy
   */
  StackHelper();

  /**
   * \brief Destroy the NdnStackHelper
   */
  virtual ~StackHelper();

  /**
   * @brief Set parameters of NdnL3Protocol
   */
  void
  SetStackAttributes(const std::string& attr1 = "", const std::string& value1 = "",
                     const std::string& attr2 = "", const std::string& value2 = "",
                     const std::string& attr3 = "", const std::string& value3 = "",
                     const std::string& attr4 = "", const std::string& value4 = "");

  /**
   * @brief Set maximum size for NFD's Content Store (in number of packets)
   */
  void
  setCsSize(size_t maxSize);

  /**
   * @brief Set ndnSIM 1.0 content store implementation and its attributes
   * @param contentStoreClass string, representing class of the content store
   * @note ndnSIM 1.0 content store implementation have limited support for Interest selectors
   *       Do not use these implementations if your scenario relies on proper selector processing.
   */
  void
  SetOldContentStore(const std::string& contentStoreClass, const std::string& attr1 = "",
                  const std::string& value1 = "", const std::string& attr2 = "",
                  const std::string& value2 = "", const std::string& attr3 = "",
                  const std::string& value3 = "", const std::string& attr4 = "",
                  const std::string& value4 = "");

  typedef Callback<shared_ptr<NetDeviceFace>, Ptr<Node>, Ptr<L3Protocol>, Ptr<NetDevice>>
    NetDeviceFaceCreateCallback;

  /**
   * @brief Add callback to create and configure instance of the face, based on supplied Ptr<Node>
   *and Ptr<NetDevice>
   *
   * It is possible to set up several callbacks for different NetDevice types.
   *
   * If none of the callbacks fit the TypeId of NetDevice, a default callback is used
   *(DefaultNetDeviceCallback)
   */
  void
  AddNetDeviceFaceCreateCallback(TypeId netDeviceType, NetDeviceFaceCreateCallback callback);

  /**
   * @brief Update callback to create and configure instance of the face, based on supplied
   *Ptr<Node> and Ptr<NetDevice>
   *
   * It is possible to set up several callbacks for different NetDevice types.
   *
   * Using this method, it is possible to override Face creation for PointToPointNetDevices
   */
  void
  UpdateNetDeviceFaceCreateCallback(TypeId netDeviceType, NetDeviceFaceCreateCallback callback);

  /**
   * @brief Remove callback to create and configure instance of the face, based on supplied
   * Ptr<Node> and Ptr<NetDevice>
   */
  void
  RemoveNetDeviceFaceCreateCallback(TypeId netDeviceType, NetDeviceFaceCreateCallback callback);

  /**
  * \brief Install Ndn stack on the node
  *
  * This method will assert if called on a node that already has Ndn object
  * installed on it
  *
  * \param nodeName The name of the node on which to install the stack.
  *
  * \returns list of installed faces in the form of a smart pointer
  * to NdnFaceContainer object
  */
  Ptr<FaceContainer>
  Install(const std::string& nodeName) const;

  /**
   * \brief Install Ndn stack on the node
   *
   * This method will assert if called on a node that already has Ndn object
   * installed on it
   *
   * \param node The node on which to install the stack.
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  Install(Ptr<Node> node) const;

  /**
   * \brief Install Ndn stack on each node in the input container
   *
   * The program will assert if this method is called on a container with a node
   * that already has an ndn object aggregated to it.
   *
   * \param c NodeContainer that holds the set of nodes on which to install the
   * new stacks.
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  Install(const NodeContainer& c) const;

  /**
   * \brief Install Ndn stack on all nodes in the simulation
   *
   * \returns list of installed faces in the form of a smart pointer
   * to FaceContainer object
   */
  Ptr<FaceContainer>
  InstallAll() const;

  /**
   * \brief Set flag indicating necessity to install default routes in FIB
   */
  void
  SetDefaultRoutes(bool needSet);

  static KeyChain&
  getKeyChain();

   /**
   * \brief Update Ndn stack on a given node (Add faces for new devices)
   *
   * \param node The node on which to update the stack.
   */
  void
  Update(Ptr<Node> node);

  /**
   *\brief Update Ndn stack on given nodes (Add faces for new devices)
   *
   * \param c The nodes on which to update the stack.
   */
  void
  Update(const NodeContainer& c);

  /**
   *\brief Update Ndn stack on a given node name (Add faces for new devices)
   *
   * \param nodeName The name of the node on which to update the stack.
   */
  void
  Update(const std::string& nodeName);

  /**
   *\brief Update Ndn stack on all the nodes (Add faces for new devices)
   */
  void
  UpdateAll();

  /**
   *\brief Disable the RIB manager of NFD
   */
  void
  disableRibManager();

  /**
   * \brief Disable Face Manager
   */
  void
  disableFaceManager();

  /**
   * \brief Disable Strategy Choice Manager
   */
  void
  disableStrategyChoiceManager();

  /**
   * \brief Disable Status Server
   */
  void
  disableStatusServer();

private:
  shared_ptr<NetDeviceFace>
  DefaultNetDeviceCallback(Ptr<Node> node, Ptr<L3Protocol> ndn, Ptr<NetDevice> netDevice) const;

  shared_ptr<NetDeviceFace>
  PointToPointNetDeviceCallback(Ptr<Node> node, Ptr<L3Protocol> ndn,
                                Ptr<NetDevice> netDevice) const;
  shared_ptr<NetDeviceFace>
  createAndRegisterFace(Ptr<Node> node, Ptr<L3Protocol> ndn, Ptr<NetDevice> device) const;

  bool m_isRibManagerDisabled;
  bool m_isFaceManagerDisabled;
  bool m_isStatusServerDisabled;
  bool m_isStrategyChoiceManagerDisabled;

public:
  void
  setCustomNdnCxxClocks();

private:
  ObjectFactory m_ndnFactory;
  ObjectFactory m_contentStoreFactory;

  bool m_needSetDefaultRoutes;
  size_t m_maxCsSize;

  typedef std::list<std::pair<TypeId, NetDeviceFaceCreateCallback>> NetDeviceCallbackList;
  NetDeviceCallbackList m_netDeviceCallbacks;
};

} // namespace ndn
} // namespace ns3

#endif /* NDN_STACK_HELPER_H */
