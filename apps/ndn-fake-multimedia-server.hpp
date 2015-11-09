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

#ifndef NDN_FAKEMULTIMEDIASERVER_H
#define NDN_FAKEMULTIMEDIASERVER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief 
 *
 * 
 */
class FakeMultimediaServer : public App {
public:
  static TypeId
  GetTypeId(void);

  FakeMultimediaServer();

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop


  bool
  CompressString(std::string input, std::stringstream& outputStream);

  void
  ReturnManifestData(shared_ptr<const Interest> interest, std::string& fname); // return file-manifest data

  void
  ReturnVirtualPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo);

  void
  ReturnPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo, const char* payload, int payload_size);


  long
  GetFileSize(std::string filename);

  uint16_t
  GetFaceMTU(uint32_t faceId);

  size_t
  EstimateOverhead(std::string& fname);

  uint16_t m_MTU;


  ndn::time::milliseconds m_freshnessTime;

private:
  std::string m_prefix;
  std::string m_metaDataFile;
  std::string m_postfixManifest;
  std::string m_mpdFileName;

  std::string m_mpdFileContent;

  std::map<std::string,long> m_fileSizes;
  std::map<std::string,size_t> m_packetSizes;


  uint32_t m_maxPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_FakeMultimediaServer_H
