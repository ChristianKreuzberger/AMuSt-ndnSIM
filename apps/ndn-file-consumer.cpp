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

#include "ndn-file-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"


#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"

#include "ns3/wifi-net-device.h"

#include <math.h>


#include <fstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>





#define ALPHA 0.125
#define BETA 0.25
#define MINIMUM_DEVIATION 5.0
#define MINIMUM_TIMEOUT 10.0


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumer);

TypeId
FileConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<FileConsumer>()
      .AddAttribute("FileToRequest", "Name of the File to Request", StringValue("/"),
                    MakeNameAccessor(&FileConsumer::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for Interests", StringValue("2s"),
                    MakeTimeAccessor(&FileConsumer::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FileConsumer::m_manifestPostfix), MakeStringChecker())
      .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)", StringValue(""),
                    MakeStringAccessor(&FileConsumer::m_outFile), MakeStringChecker())
      .AddAttribute("MaxEstimatedRTT", "The maximum RTT the RTTEstimator should have (in ms)", UintegerValue(500),
                    MakeUintegerAccessor(&FileConsumer::m_maxRTT),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("InitialRTT", "The initial RTT the RTTEstimator should have (in ms)", UintegerValue(500),
                    MakeUintegerAccessor(&FileConsumer::m_initialRTT),
                    MakeUintegerChecker<uint32_t>())
      .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                      MakeTraceSourceAccessor(&FileConsumer::m_downloadFinishedTrace))
      .AddTraceSource("ManifestReceived", "Trace called every time a manifest is received",
                      MakeTraceSourceAccessor(&FileConsumer::m_manifestReceivedTrace))
      .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                      MakeTraceSourceAccessor(&FileConsumer::m_downloadStartedTrace))
      .AddTraceSource("CurrentPacketStats", "Trace current packets statistics (once per second)",
                      MakeTraceSourceAccessor(&FileConsumer::m_currentStatsTrace));
    ;

  return tid;
}

FileConsumer::FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
  m_localDataCache = NULL;
}

FileConsumer::~FileConsumer()
{
}


void
FileConsumer::PacketStatsUpdateEvent()
{
  m_currentStatsTrace(this, _shared_interestName, m_packetsSent, m_packetsReceived, m_packetsTimeout, m_packetsRetransmitted, EstimatedRTT, DeviationRTT);

  if (!m_active || m_finishedDownloadingFile == true)
    return;

  m_packetStatsUpdateEvent = Simulator::Schedule(Seconds(1), &FileConsumer::PacketStatsUpdateEvent, this);
}

///////////////////////////////////////////////////
//             Application Methods               //
///////////////////////////////////////////////////

// Start Application - initialize variables etc...
void
FileConsumer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  // initialize variables
  m_hasReceivedManifest = false;
  m_hasRequestedManifest = false;
  m_finishedDownloadingFile = false;

  m_fileSize = 0;
  m_curSeqNo = -1;
  m_maxSeqNo = -1;
  m_lastSeqNoReceived = -1;

  DeviationRTT = 0.0;
  EstimatedRTT = m_initialRTT;

  m_sequenceStatus.clear();
  m_sequenceStatus.resize(1); // set initial size to 1 to cover the manifest


  m_packetsReceived = m_packetsSent = m_packetsTimeout = m_packetsRetransmitted = 0;

  // initialize random variable generator
  m_rand = CreateObject<UniformRandomVariable>();

  if (!m_outFile.empty())
  {
    // create outfile
    FILE* fp = fopen(m_outFile.c_str(), "w");
    fclose(fp);
  }
  // create buffer (set to NULL for now)
  m_localDataCache = NULL;

  // set start time
  _start_time = Simulator::Now().GetMilliSeconds ();

  _shared_interestName = make_shared<Name>(m_interestName);

  m_downloadStartedTrace(this, _shared_interestName);

  // Start requester - schedule "SendPacket" method immediately (this will request the file manifest)
  ScheduleNextSendEvent();
  PacketStatsUpdateEvent();
}


// Stop Application - Cancel any outstanding events
void
FileConsumer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);
  Simulator::Cancel(m_packetStatsUpdateEvent);

  //cancel all timeouts
  for(std::map<uint32_t,EventId>::iterator it = m_chunkTimeoutEvents.begin();
      it != m_chunkTimeoutEvents.end(); it++)
  {
    Simulator::Cancel(it->second);
  }

  m_sequenceStatus.clear();

  if (m_localDataCache != NULL)
  {
    free(m_localDataCache);
    m_localDataCache = NULL;
  }

  m_outFile = "";

  // clear m_rand
  m_rand = nullptr;

  // cleanup base stuff
  App::StopApplication();
}


bool
FileConsumer::SendPacket()
{
  // check if active
  if (!m_active)
    return false;

  NS_LOG_FUNCTION_NOARGS();

  bool okay = false;

  // did we request or receive the manifest yet?
  if (!m_hasRequestedManifest)
    okay = SendManifestPacket();
  else // if we did, then we can start streaming
    okay = SendFilePacket();

  if (okay)
    AfterSendPacket();

  return okay;
}



void
FileConsumer::AfterSendPacket()
{
    m_packetsSent++;
}

///////////////////////////////////////////////////
//          Process outgoing packets             //
///////////////////////////////////////////////////

bool
FileConsumer::SendManifestPacket()
{
  if (!m_active)
    return false;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> interestNameWithManifest = make_shared<Name>(m_interestName);

  // create the interest name: m_interestName + manifest string (postfix)
  interestNameWithManifest->append(m_manifestPostfix);

  //fprintf(stderr, "interestNameWithManifest=%s\n", interestNameWithManifest->toUri().c_str());

  // create an interest, set nonce
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue()*1000);
  interest->setName(*interestNameWithManifest);

  m_manifestRequestTime = Simulator::Now().GetMilliSeconds();

  // set the interest lifetime
  long timeout = 1.0 * EstimatedRTT + 4.0 * DeviationRTT; // , where u = 1 and q = 4

  m_interestLifeTime = ns3::Time::FromDouble(timeout, ns3::Time::MS);

  m_sequenceStatus[0] = Requested;

  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);
  CreateTimeoutEvent(0, m_interestLifeTime.GetMilliSeconds());

  // log that we created the interest
  NS_LOG_INFO("> Creating INTEREST for " << interest->getName());
  m_hasRequestedManifest = true;

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  return true;

}


bool
FileConsumer::SendFilePacket()
{
  if (!m_active)
    return false;


  unsigned seq = GetNextSeqNo();

  NS_LOG_DEBUG("Requesting Sequence " << seq);

  if (seq > m_maxSeqNo || m_fileSize == 0)
    return false;

  // check if this is a retransmission
  if (m_sequenceStatus[seq] == TimedOut)
    m_packetsRetransmitted++;

  m_sequenceStatus[seq] = Requested;
  m_sequenceSendTime[seq] = Simulator::Now().GetMilliSeconds();

  NS_LOG_FUNCTION_NOARGS();


  // set the interest lifetime
  double timeout = 1.0 * EstimatedRTT + 4.0 * DeviationRTT; // , where u = 1 and q = 4

  if (timeout < MINIMUM_TIMEOUT)
    timeout = MINIMUM_TIMEOUT;


  m_interestLifeTime = ns3::Time::FromDouble(timeout, ns3::Time::MS);

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);

  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();

  // m_rand returns a double value - multiply by 1000, else (int)nonce is always 0 !!!
  interest->setNonce(m_rand->GetValue()*1000);
  interest->setName(*nameWithSequence);

  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  CreateTimeoutEvent(seq, m_interestLifeTime.GetMilliSeconds());

  NS_LOG_INFO("> File INTEREST (Seq: " << seq << "): " << interest->getName());


  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_curSeqNo++;

  return true;
}



// find a sequence number that has not been requested yet or timed out
uint32_t
FileConsumer::GetNextSeqNo()
{
  // start by counting from 1 (seqNo = 0 is the manifest)
  uint32_t seqNo = 1;

  for (; seqNo <= m_maxSeqNo; seqNo ++)
  {
    auto seqStatus = m_sequenceStatus[seqNo];
    if (seqStatus == NotRequested || seqStatus == TimedOut)
    {
      return seqNo;
    }
  }

  return m_maxSeqNo+1;
}



bool
FileConsumer::AreAllSeqReceived()
{
  uint32_t seqNo = 0;

  for (; seqNo <= m_maxSeqNo; seqNo ++)
  {
    auto seqStatus = m_sequenceStatus[seqNo];
    if (seqStatus != Received)
    {
      return false;
    }
  }

  return true;
}



void
FileConsumer::CreateTimeoutEvent(uint32_t seqNo, uint32_t timeout)
{
  if (m_chunkTimeoutEvents.find( seqNo ) != m_chunkTimeoutEvents.end())
  {
    Simulator::Cancel(m_chunkTimeoutEvents[seqNo]);
    m_chunkTimeoutEvents[seqNo].Cancel();
  }

  // schedule the timeout event 1 miliseconds after the interest lifetime is over (just in case, we don't want events to trigger at the same time)
  m_chunkTimeoutEvents[seqNo] = Simulator::Schedule(MilliSeconds(timeout+1), &FileConsumer::CheckSeqForTimeout, this, seqNo);
}




void
FileConsumer::CheckSeqForTimeout(uint32_t seqNo)
{
  if (m_hasReceivedManifest == false && seqNo == 0)
  {
    // means this timeout is about the manifest
    m_sequenceStatus[0] = TimedOut;
    m_hasRequestedManifest = false;
    m_chunkTimeoutEvents[seqNo].Cancel();
    SendPacket();
    return;
  }

  if (m_sequenceStatus[seqNo] != Received)
  {
    // means this sequence has timed out
    m_sequenceStatus[seqNo] = TimedOut;
    NS_LOG_DEBUG("Timeout occured for seq " << seqNo);
    m_chunkTimeoutEvents[seqNo].Cancel();

    m_packetsTimeout++;

    // update estimated rtt
    EstimatedRTT = EstimatedRTT * 2;
    if (EstimatedRTT > m_maxRTT)
      EstimatedRTT = m_maxRTT;


    // call ontimeout
    OnTimeout(seqNo);
  }
}



void
FileConsumer::OnTimeout(uint32_t seq_nr)
{
  AfterData(false, true, seq_nr);
}


///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
FileConsumer::OnData(shared_ptr<const Data> data)
{
  // check if app is active
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  m_packetsReceived++;

  // Log some infos
  NS_LOG_FUNCTION(this << data);
  NS_LOG_INFO("< DATA for " << data->getName());


  // get the hopcount
  int hopCount = -1;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  // get interest name
  ndn::Name interestName = data->getName();

  // Check whether this is a Manifest Packet or a Data Packet
  // Manifest packets will end with m_manifestPostfix
  // only check if we haven't received the manifest yet
  if (!m_hasReceivedManifest)
  {
    ndn::Name  lastPostfix = interestName.getSubName(interestName.size() -1 );

    NS_LOG_INFO("< LastPostfix = " << lastPostfix);

    if (lastPostfix == m_manifestPostfix)
    {
      // this means we have just received the manifest
      // get the content value: cast unit8_t* to long* and then dereference it
      //long fileSize =  *((long*)data->getContent().value());

      uint8_t *buffer = ((uint8_t*)data->getContent().value());
      long fileSize;
      int maxPayload;
      memcpy(&fileSize, buffer, sizeof(long));
      memcpy(&maxPayload, buffer+sizeof(long), sizeof(unsigned));


      NS_LOG_DEBUG("FileConsumer: Received Manifest! FileSize=" << fileSize << ", MaxPayload=" << maxPayload);
      m_hasReceivedManifest = true;
      m_fileSize = fileSize;
      m_maxPayloadSize = maxPayload;

      if (m_fileSize == -1)
      {
        NS_LOG_UNCOND("ERROR: FileConsumer: File not found on server: " << interestName);
        m_fileSize = 0;
        m_curSeqNo = 0;
        m_maxSeqNo = 0;
      } else
      {
        m_curSeqNo = 0;
        m_maxSeqNo = ceil((double)m_fileSize/(double)m_maxPayloadSize);
        NS_LOG_DEBUG("FileConsumer: Resulting Max Seq Nr = " << m_maxSeqNo);

        // Trigger OnManifest
        m_chunkTimeoutEvents[0].Cancel();
        OnManifest(fileSize);
        AfterData(true, false, 0);
      }

      return;
    }
  }
  // else: this is a normal data packet, process it as such a data packet

  // Get seq_nr from Interest Name
  uint32_t seqNo = interestName.at(-1).toSequenceNumber();
  m_lastSeqNoReceived = seqNo;

  // make sure that we mark this sequence as received
  m_sequenceStatus[seqNo] = Received;

  if (m_chunkTimeoutEvents.find( seqNo ) != m_chunkTimeoutEvents.end())
  {
    // cancel timeout event
    Simulator::Cancel(m_chunkTimeoutEvents[seqNo]);
    m_chunkTimeoutEvents[seqNo].Cancel();
    m_chunkTimeoutEvents.erase(seqNo);
  } // else: don't bother, probably was a duplicate

  // trigger OnFileData
  NS_LOG_DEBUG("SeqNo: " << seqNo);
  NS_LOG_DEBUG("Contentvaluesize: " << data->getContent().value_size());
  OnFileData(seqNo, data->getContent().value(), data->getContent().value_size());

  // check if everything has been received
  if (!m_finishedDownloadingFile && AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
  }

  AfterData(false, false, seqNo);
}


void
FileConsumer::AfterData(bool manifest, bool timeout, uint32_t seq_nr)
{
  if (AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
  } else {
    ScheduleNextSendEvent();
  }
}


long
FileConsumer::GetFaceBitrate(uint32_t faceId)
{
  Ptr<ns3::PointToPointNetDevice> nd1 = GetNode ()->GetDevice(faceId)->GetObject<ns3::PointToPointNetDevice>();

  if (nd1 == NULL)
  {
      return 54000000; // 54 Mbit
  } else
  {
    DataRateValue dv;
    nd1->GetAttribute("DataRate", dv);
    DataRate d = dv.Get();
    return d.GetBitRate();
  }
}


uint16_t
FileConsumer::GetFaceMTU(uint32_t faceId)
{
  Ptr<ns3::PointToPointNetDevice> nd1 = GetNode ()->GetDevice(faceId)->GetObject<ns3::PointToPointNetDevice>();

   if (nd1 == NULL)
  {
    // try getting Wifi net device
    Ptr<ns3::WifiNetDevice> nd2 = GetNode ()->GetDevice(faceId)->GetObject<ns3::WifiNetDevice>();

    if (nd2 != NULL)
    {
      return nd2->GetMtu();
    } else {
      Ptr<ns3::NetDevice> nd3 = GetNode ()->GetDevice(faceId)->GetObject<ns3::NetDevice>();

      return nd3->GetMtu();
    }

  } else {
    return nd1->GetMtu();
  }

}


void
FileConsumer::OnManifest(long fileSize)
{
  m_sequenceStatus[0] = Received;
  // reserve elements in sequence status
  m_sequenceStatus.resize(m_maxSeqNo+1);


  if (!m_outFile.empty())
  {
    // create m_localDataCache
    m_localDataCache = (uint8_t*)malloc(sizeof(uint8_t) * fileSize);
  }


  long SampleRTT  = Simulator::Now().GetMilliSeconds() - m_manifestRequestTime;


  EstimatedRTT = SampleRTT;
  DeviationRTT = SampleRTT / 2;

  // call trace source
  m_manifestReceivedTrace(this, _shared_interestName, fileSize);
}


void
FileConsumer::OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length)
{
  NS_LOG_FUNCTION(this << seq_nr << length);
  // write outfile if defined
  if (!m_outFile.empty())
  {

    // FILE * fp = fopen(m_outFile.c_str(), "ab");
    if (seq_nr < m_maxSeqNo)
    {
      memcpy(m_localDataCache + (seq_nr-1)*m_maxPayloadSize, data, length);
      //fwrite(data, sizeof(uint8_t), length, fp);
    } else
    {
      memcpy(m_localDataCache + (seq_nr-1)*m_maxPayloadSize, data, m_fileSize % m_maxPayloadSize);
      //fwrite(data, sizeof(uint8_t), m_fileSize % m_maxPayloadSize, fp);
    }
    //fclose(fp);
  }


  long SampleRTT  = Simulator::Now().GetMilliSeconds() - m_sequenceSendTime[seq_nr];
  m_sequenceSendTime.erase(seq_nr);
//  delete m_sequenceSendTime[seq_nr];


  // 90% of estimated + 10% of measured RTT
  EstimatedRTT = (1-BETA) * EstimatedRTT + BETA * SampleRTT;

  double absDiff = abs(SampleRTT - EstimatedRTT);

  if (absDiff < MINIMUM_DEVIATION)
    absDiff = MINIMUM_DEVIATION;

  DeviationRTT = (1-ALPHA)* DeviationRTT + ALPHA * absDiff;

}

void
FileConsumer::ScheduleNextSendEvent(double miliseconds)
{
  NS_LOG_FUNCTION(this << miliseconds);

  if (miliseconds <= 0.00)
    miliseconds = 0.01;

  //NS_LOG_UNCOND("Curtime: " << Simulator::Now().GetMilliSeconds());
  //NS_LOG_UNCOND("Event TS: " << m_sendEvent.GetTs());

  m_sendEvent.Cancel();
  // Schedule Next Send Event Now
  m_sendEvent = Simulator::Schedule(NanoSeconds(miliseconds*1000000.0), &FileConsumer::SendPacket, this);

  m_nextEventScheduleTime = Simulator::Now().GetMilliSeconds() + miliseconds;
}


double
FileConsumer::CalculateDownloadSpeed()
{
  _finished_time = Simulator::Now().GetMilliSeconds ();
  lastDownloadBitrate = ((double)(m_fileSize *8)) / ( ((double)(_finished_time - _start_time))/1000.0 );
  return lastDownloadBitrate;
}


void
FileConsumer::OnFileReceived(unsigned status, unsigned length)
{
    // TODO: Stop Event Loop
  Simulator::Cancel(m_sendEvent);

  if (m_finishedDownloadingFile)
    return;

  m_finishedDownloadingFile = true;
  // do nothing here
  NS_LOG_DEBUG("Finally received the whole file!");

  // write file to disk
  if (!m_outFile.empty())
  {
    FILE* fp = fopen(m_outFile.c_str(), "w");
    fwrite(m_localDataCache, sizeof(uint8_t), m_fileSize, fp);
    fclose(fp);
  }

  double downloadSpeed = CalculateDownloadSpeed();
  NS_LOG_DEBUG("Download finished after " << (_finished_time - _start_time) << "ms; AvgSpeed = " << downloadSpeed << " bytes per second.");

  // call trace source
  this->m_downloadFinishedTrace(this, _shared_interestName, downloadSpeed, (_finished_time - _start_time));

  // kill all remaining timeout events
  for (uint32_t seqNo = 0; seqNo < m_maxSeqNo; seqNo++)
  {
    Simulator::Cancel(m_chunkTimeoutEvents[seqNo]);
  }

  Simulator::Cancel(m_sendEvent);
  m_chunkTimeoutEvents.clear();

  // clear m_sequenceSendTime
  m_sequenceSendTime.clear();

  // do not clear m_sequenceStatus here, it might still be triggered...
}


bool
FileConsumer::DecompressFile ( std::string source, std::string filename )
{
  std::ifstream file( source.c_str(), std::ios_base::in | std::ios_base::binary ); //Creates the input stream

  //Tests if the file is being opened correctly.
  if ( !file )
  {
   std::cerr<< "Can't open file: " << source << std::endl;
   return false;
  }

  try
  {
    std::ofstream out( filename.c_str(), std::ios_base::out |  std::ios_base::binary ); //Creates the output stream
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in; //Sets the filters to be used in the input stream
    in.push( boost::iostreams::gzip_decompressor() ); //Decompress being used on the file in the input stream
    in.push( file );
    boost::iostreams::copy( in, out ); //Copy the decompressed file to the output stream
  }
  catch(std::exception& e)
  {
    //std::cerr << e.what() << std::endl;
    NS_LOG_DEBUG(e.what() << " Assuming file was not zipped!");
    return false;
  }
  return true;
}






} // namespace ndn
} // namespace ns3
