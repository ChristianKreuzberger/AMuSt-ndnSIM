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

#include "ndn-fake-fileserver.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "ns3/data-rate.h"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/point-to-point-module.h"



#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <sys/types.h>
#include <sys/stat.h>

#include <math.h>
#include <fstream>


#include <algorithm>    // copy
#include <iterator>     // ostream_operator

#include <boost/tokenizer.hpp>



NS_LOG_COMPONENT_DEFINE("ndn.FakeFileServer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FakeFileServer);




TypeId
FakeFileServer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FakeFileServer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<FakeFileServer>()
      .AddAttribute("Prefix", "Prefix, for which FakeFileServer serves the data", StringValue("/"),
                    MakeStringAccessor(&FakeFileServer::m_prefix), MakeStringChecker())
      .AddAttribute("MetaDataFile", "A CSV file containing meta data of all files which FakeFileServer will serve",
                    StringValue("./filelist.csv"),
                    MakeStringAccessor(&FakeFileServer::m_metaDataFile), MakeStringChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FakeFileServer::m_postfixManifest), MakeStringChecker())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&FakeFileServer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&FakeFileServer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&FakeFileServer::m_keyLocator), MakeNameChecker());
  return tid;
}

FakeFileServer::FakeFileServer()
{
  NS_LOG_FUNCTION_NOARGS();
}



// inherited from Application base class.
void
FakeFileServer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  // read m_metaDataFile and fill m_fileSizes
  std::ifstream infile(m_metaDataFile.c_str());
  if (!infile.is_open())
  {
    fprintf(stderr, "FakeFileServer: Error opening %s\n", m_metaDataFile.c_str());
    return;
  }


  typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

  std::string line;
  std::vector<std::string> vecLine;

  fprintf(stderr, "Reading list of file sizes from %s\n", m_metaDataFile.c_str());


  while (std::getline(infile,line))
  {
    if (line.length() > 2)
    {
      Tokenizer tok(line);
      vecLine.assign(tok.begin(), tok.end());
      m_fileSizes["/" + vecLine.at(0)] = atoi(vecLine.at(1).c_str());
    }
  }


  infile.close();

  m_MTU = GetFaceMTU(0);

  m_freshnessTime = ::ndn::time::milliseconds(m_freshness.GetMilliSeconds());
}



uint16_t
FakeFileServer::GetFaceMTU(uint32_t faceId)
{
  Ptr<ns3::PointToPointNetDevice> nd1 = GetNode ()->GetDevice(faceId)->GetObject<ns3::PointToPointNetDevice>();
  return nd1->GetMtu();

}



void
FakeFileServer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}




void
FakeFileServer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());

  // get last postfix
  ndn::Name  lastPostfix = dataName.getSubName(dataName.size() -1 );
  NS_LOG_INFO("> LastPostfix = " << lastPostfix);

  bool isManifest = false;
  uint32_t seqNo = -1;

  if (lastPostfix == m_postfixManifest)
  {
    isManifest = true;
  }
  else
  {
    seqNo = dataName.at(-1).toSequenceNumber();
    seqNo = seqNo - 1; // Christian: the client thinks seqNo = 1 is the first one, for the server it's better to start at 0
  }

  // remove the last postfix
  dataName = dataName.getPrefix(-1);

  // extract filename and get path to file
  std::string fname = dataName.toUri();  // get the uri from interest

  // measure how much overhead this actually this
  int diff = EstimateOverhead(fname);
  // set new payload size to this value (minus 4 bytes to be safe for sequence numbers, ethernet headers, etc...)
  m_maxPayloadSize = m_MTU - diff - 4;

  //NS_LOG_UNCOND("NewPayload = " << m_maxPayloadSize << " (Overhead: " << diff << ") ");

  fname = fname.substr(m_prefix.length(), fname.length()); // remove the prefix

  // handle manifest or data
  if (isManifest)
  {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Manifest for file " << fname);
    ReturnManifestData(interest, fname);
  } else
  {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Payload for file " << fname);
    NS_LOG_DEBUG("FileName: " << fname << ", SeqNo:" << seqNo);

#ifdef DEBUG
    // check if file exists and the sanity of the sequence number requested
    long fileSize = GetFileSize(fname);

    if (fileSize == -1)
      return; // file does not exist, just quit

    if (seqNo > ceil(fileSize / m_maxPayloadSize))
      return; // sequence not available
#endif
    // else:
    ReturnVirtualPayloadData(interest, fname, seqNo);
  }
}



void
FakeFileServer::ReturnManifestData(shared_ptr<const Interest> interest, std::string& fname)
{
  long fileSize = GetFileSize(fname);

  auto data = make_shared<Data>();
  data->setName(interest->getName());
  data->setFreshnessPeriod(m_freshnessTime);

  // create a local buffer variable, which contains a long and an unsigned
  uint8_t buffer[sizeof(long) + sizeof(unsigned)];
  memcpy(buffer, &fileSize, sizeof(long));
  memcpy(buffer+sizeof(long), &m_maxPayloadSize, sizeof(unsigned));

  // create content with the file size in it
  data->setContent(reinterpret_cast<const uint8_t*>(buffer), sizeof(long) + sizeof(unsigned));

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);


  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}



void
FakeFileServer::ReturnVirtualPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo)
{
  auto data = make_shared<Data>();
  data->setName(interest->getName());

  data->setFreshnessPeriod(m_freshnessTime);

  auto buffer = make_shared< ::ndn::Buffer>(m_maxPayloadSize);
  data->setContent(buffer);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);


  // to create real wire encoding
  Block tmp = data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}






size_t
FakeFileServer::EstimateOverhead(std::string& fname)
{
  if (m_packetSizes.find(fname) != m_packetSizes.end())
  {
    return m_packetSizes[fname];
  }

  uint32_t interestLength = fname.length();
  // estimate the payload size for now
  int estimatedMaxPayloadSize = m_MTU - interestLength - 30; // the -30 is something we saw in results, it's just to estimate...

  auto data = make_shared<Data>();
  data->setName(fname + "/1"); // to simulate that there is at least one chunk
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  auto buffer = make_shared< ::ndn::Buffer>(estimatedMaxPayloadSize);
  data->setContent(buffer);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);


  // to create real wire encoding
  Block tmp = data->wireEncode();

  m_packetSizes[fname] = tmp.size() - estimatedMaxPayloadSize;

  return tmp.size() - estimatedMaxPayloadSize;
}



// GetFileSize either from m_fileSizes map or from disk
long FakeFileServer::GetFileSize(std::string filename)
{
  // check if is already in m_fileSizes
  if (m_fileSizes.find(filename) != m_fileSizes.end())
  {
    return m_fileSizes[filename];
  }
  fprintf(stderr, "Error finding file %s\n", filename.c_str());
  // else: query disk for file size

  return -1;
}





} // namespace ndn
} // namespace ns3
