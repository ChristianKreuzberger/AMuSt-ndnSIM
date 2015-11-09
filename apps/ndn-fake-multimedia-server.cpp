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

#include "ndn-fake-multimedia-server.hpp"
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


#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/stream.hpp>



NS_LOG_COMPONENT_DEFINE("ndn.FakeMultimediaServer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FakeMultimediaServer);




TypeId
FakeMultimediaServer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FakeMultimediaServer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<FakeMultimediaServer>()
      .AddAttribute("Prefix", "Prefix, for which FakeMultimediaServer serves the data", StringValue("/"),
                    MakeStringAccessor(&FakeMultimediaServer::m_prefix), MakeStringChecker())
      .AddAttribute("MetaDataFile", "A CSV file containing meta data of representations that FakeMultimediaServer will serve",
                    StringValue("./representations.csv"),
                    MakeStringAccessor(&FakeMultimediaServer::m_metaDataFile), MakeStringChecker())
      .AddAttribute("MPDFileName", "The virtual name of the MPD file that FakeMultimediaServer will serve under Prefix/MPDFileName",
                    StringValue("MyVideo.mpd"),
                    MakeStringAccessor(&FakeMultimediaServer::m_mpdFileName), MakeStringChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FakeMultimediaServer::m_postfixManifest), MakeStringChecker())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&FakeMultimediaServer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&FakeMultimediaServer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&FakeMultimediaServer::m_keyLocator), MakeNameChecker());
  return tid;
}

FakeMultimediaServer::FakeMultimediaServer()
{
  NS_LOG_FUNCTION_NOARGS();
}



// inherited from Application base class.
void
FakeMultimediaServer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  // read m_metaDataFile and create fake representations
  std::ifstream infile(m_metaDataFile.c_str());
  if (!infile.is_open())
  {
    fprintf(stderr, "FakeMultimediaServer: Error opening %s\n", m_metaDataFile.c_str());
    return;
  }


  typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;

  std::string line;
  std::vector<std::string> vecLine;

  fprintf(stderr, "Reading Multimedia Specifics from %s\n", m_metaDataFile.c_str());

  int segment_duration = 0;
  int number_of_segments = 0;

  // get first line: segmentDuration
  std::getline(infile,line);
  std::string prefix("segmentDuration=");
  if (!line.compare(0, prefix.size(), prefix))
    segment_duration = atoi(line.substr(prefix.size()).c_str());

  std::getline(infile,line);
  prefix = "numberOfSegments=";
  if (!line.compare(0, prefix.size(), prefix))
    number_of_segments = atoi(line.substr(prefix.size()).c_str());

  fprintf(stderr,"duration=%d,number=%d\n", segment_duration,number_of_segments);

  // get header and ignore
  std::getline(infile,line);


  std::ostringstream mpdData;

  mpdData << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl 
          << "<MPD xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
          << "xmlns=\"urn:mpeg:DASH:schema:MPD:2011\" xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011\"" << std::endl
          << "profiles=\"urn:mpeg:dash:profile:isoff-main:2011\" type=\"static\"" << std::endl;

  int totalVideoDuration = number_of_segments * segment_duration;

  int videoDurationInSeconds = totalVideoDuration % 60;
  totalVideoDuration -= videoDurationInSeconds;
  totalVideoDuration = totalVideoDuration / 60;
  int videoDurationInMinutes = totalVideoDuration % 60;
  totalVideoDuration -= videoDurationInMinutes;
  int videoDurationInHours = totalVideoDuration / 60;

  


  mpdData << "mediaPresentationDuration=\"PT" << videoDurationInHours << "H" << videoDurationInMinutes << "M" << videoDurationInSeconds << "S\" "; 
  mpdData << "minBufferTime=\"PT2.0S\">" << std::endl;
  mpdData << "<BaseURL>" << m_prefix << "/</BaseURL>" << std::endl
          << "<Period start=\"PT0S\">" << std::endl << "<AdaptationSet bitstreamSwitching=\"true\">" << std::endl;




  
  while (std::getline(infile,line))
  {
    if (line.length() > 2)
    {
      Tokenizer tok(line);
      vecLine.assign(tok.begin(), tok.end());
      // reprId,screenWidth,screenHeight,bitrate
      std::string repr_id = vecLine.at(0);
      std::string screen_width = vecLine.at(1);
      std::string screen_height = vecLine.at(2);
      std::string bitrate = vecLine.at(3); 
      int iBitrate = atoi(bitrate.c_str());

      mpdData << "<Representation id=\"" << repr_id << "\" codecs=\"avc1\" mimeType=\"video/mp4\"" <<
                 " width=\"" << screen_width << "\" height=\"" << screen_height << "\" startWithSAP=\"1\" bandwidth=\"" << (iBitrate*1000) << "\">" << std::endl;
      mpdData << "<SegmentList duration=\"" << segment_duration << "\">" << std::endl;

      
      int iSegmentSize = (double)iBitrate/8.0 * (double)segment_duration * 1024; // in byte
      for (int i = 0; i < number_of_segments; i++)
      {
        std::ostringstream segmentFileName;
        segmentFileName << "/repr_" << repr_id << "_seg_" << i << ".264";
        m_fileSizes[segmentFileName.str()] = iSegmentSize;
        mpdData << "<SegmentURL media=\"" <<  "repr_" << repr_id << "_seg_" << i << ".264" << "\"/> " << std::endl;
        //fprintf(stderr, "SegmentName=%s,Size=%d\n", segmentFileName.str().c_str(), iSegmentSize);
      }

      mpdData << "</SegmentList>" << std::endl << "</Representation>" << std::endl;
    }
  }


  mpdData << "</AdaptationSet></Period></MPD>" << std::endl;

  //fprintf(stderr, "MPD='%s'\n", mpdData.str().c_str());
  
  // compress
  std::stringstream compressedMpdData;
  CompressString(mpdData.str(), compressedMpdData);

  m_mpdFileContent = compressedMpdData.str();

  infile.close();

  std::ostringstream mpdFileName;
  mpdFileName << "/" << m_mpdFileName;
  // store file size for mpd file name
  m_fileSizes[mpdFileName.str()] = m_mpdFileContent.size();

  m_MTU = GetFaceMTU(0);

  m_freshnessTime = ::ndn::time::milliseconds(m_freshness.GetMilliSeconds());
}



uint16_t
FakeMultimediaServer::GetFaceMTU(uint32_t faceId)
{
  Ptr<ns3::PointToPointNetDevice> nd1 = GetNode ()->GetDevice(faceId)->GetObject<ns3::PointToPointNetDevice>();
  return nd1->GetMtu();

}



void
FakeMultimediaServer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}




void
FakeMultimediaServer::OnInterest(shared_ptr<const Interest> interest)
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



#ifdef DEBUG
  // check if file exists and the sanity of the sequence number requested
  long fileSize = GetFileSize(fname);

  if (fileSize == -1)
    return; // file does not exist, just quit

  if (seqNo > ceil(fileSize / m_maxPayloadSize))
    return; // sequence not available
#endif


  // handle manifest or data
  if (isManifest)
  {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Manifest for file " << fname);
    ReturnManifestData(interest, fname);
  } else
  {
    if (fname.compare(1, std::string::npos, m_mpdFileName) == 0)
    {
      // we are processing the MPD here... this is important
      // return m_mpdFileContent
      NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Real Payload for file " << fname);
      NS_LOG_DEBUG("FileName: " << fname << ", SeqNo:" << seqNo);
      ReturnPayloadData(interest, fname, seqNo, m_mpdFileContent.c_str(), m_mpdFileContent.size());
    } else {  
      NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Virtual Payload for file " << fname);
      NS_LOG_DEBUG("FileName: " << fname << ", SeqNo:" << seqNo);
      ReturnVirtualPayloadData(interest, fname, seqNo);
    }
  }
}


bool
FakeMultimediaServer::CompressString(std::string input, std::stringstream& outputStream)
{
  boost::iostreams::filtering_streambuf< boost::iostreams::input> in;
  in.push( boost::iostreams::gzip_compressor());
  std::stringstream data;
  data << input;
  in.push(data);
  boost::iostreams::copy(in, outputStream);
  return true;
}



void
FakeMultimediaServer::ReturnManifestData(shared_ptr<const Interest> interest, std::string& fname)
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
FakeMultimediaServer::ReturnPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo, const char* payload, int payload_size)
{
  auto data = make_shared<Data>();
  data->setName(interest->getName());
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  int start_byte_no = seqNo * m_maxPayloadSize;

  if (start_byte_no > payload_size)
  {
    fprintf(stderr, "ERROR: Requested seqNo=%d (resulting in byte=%d), but payload_size=%d\n", seqNo, start_byte_no, payload_size);
    return;
  }

  int actual_payload_length = m_maxPayloadSize;
  if ((start_byte_no + actual_payload_length) > payload_size)
  {
    actual_payload_length -= (start_byte_no + actual_payload_length) - payload_size;
  }

  if (actual_payload_length <= 0)
  {
    fprintf(stderr, "ERROR: actual_payload_length < 0 (=%d)...\n", actual_payload_length);
    return;
  }


  auto buffer = make_shared< ::ndn::Buffer>(m_maxPayloadSize);

  memcpy(buffer->get(), &payload[start_byte_no], actual_payload_length);

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


void
FakeMultimediaServer::ReturnVirtualPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo)
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
FakeMultimediaServer::EstimateOverhead(std::string& fname)
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
long FakeMultimediaServer::GetFileSize(std::string filename)
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
