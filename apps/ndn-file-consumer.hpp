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

#ifndef NDN_FILECONSUMER_H
#define NDN_FILECONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"

#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include "ns3/point-to-point-module.h"

#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"



#define MAX_RTT 1000.0

namespace ns3 {
namespace ndn {




/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class FileConsumer : public App {
public:
  static TypeId
  GetTypeId();


  static bool
  DecompressFile ( std::string source, std::string filename );

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  FileConsumer();
  virtual ~FileConsumer();

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  enum SequenceStatus { NotRequested = 0, Requested = 1, TimedOut = 2, Received = 3 };

protected:
  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator

  virtual void
  OnData(shared_ptr<const Data> data);

  virtual void
  OnManifest(long fileSize);

  virtual void
  OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length);

  virtual void
  AfterData(bool manifest, bool timeout, uint32_t seq_nr); // triggered AFTER OnFileData and AFTER Manifest

  virtual void
  OnFileReceived(unsigned status, unsigned length);

  virtual void
  OnTimeout(uint32_t seq_nr);

  virtual void
  ScheduleNextSendEvent(double miliseconds=0);

  virtual void
  AfterSendPacket();

  virtual bool
  SendPacket();

  virtual bool
  SendManifestPacket();

  virtual bool
  SendFilePacket();

  virtual uint32_t
  GetNextSeqNo(); // returns the next sequence number that should be scheduled for downloaś

  virtual bool
  AreAllSeqReceived();

  virtual void
  CreateTimeoutEvent(uint32_t seqNo, uint32_t timeout);

  virtual void
  CheckSeqForTimeout(uint32_t seqNo);


  long
  GetFaceBitrate(uint32_t faceId);

  uint16_t
  GetFaceMTU(uint32_t faceId);


  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  long m_nextEventScheduleTime;
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

  std::string m_manifestPostfix;


  bool m_hasRequestedManifest;
  bool m_hasReceivedManifest;
  bool m_finishedDownloadingFile;


  std::string m_outFile;


  long m_fileSize;
  uint32_t m_curSeqNo;
  uint32_t m_maxSeqNo;
  uint32_t m_lastSeqNoReceived;

  uint32_t m_maxPayloadSize;


  std::vector<SequenceStatus> m_sequenceStatus;
  uint8_t* m_localDataCache;
  std::map<uint32_t,EventId> m_chunkTimeoutEvents;
  std::map<uint32_t,long> m_sequenceSendTime;

  long m_manifestRequestTime;


  double EstimatedRTT;
  double DeviationRTT;

  unsigned int m_initialRTT;
  unsigned int m_maxRTT;

  unsigned int m_packetsSent;
  unsigned int m_packetsReceived;
  unsigned int m_packetsTimeout;
  unsigned int m_packetsRetransmitted;

  void PacketStatsUpdateEvent();
  EventId m_packetStatsUpdateEvent;


protected: // callbacks/traces
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */> m_downloadStartedTrace;
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */,
            long /*fileSize*/> m_manifestReceivedTrace;
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */,
            double /* downloadSpeedInBytesPerSecond */, long /*milliSeconds */> m_downloadFinishedTrace;
  TracedCallback<Ptr<ns3::ndn::App> /* app */, shared_ptr<const Name> /* interestName */,
            unsigned int /* m_packetsSent */, unsigned int /* m_packetsReceived */,
            unsigned int /*m_packetsTimedout */, unsigned int /* m_packetsRetransmitted */,
            double /* EstimatedRTT */, double /* RTTVariation */
            > m_currentStatsTrace;

  double lastDownloadBitrate;

private:
  int64_t _start_time;
  int64_t _finished_time;
  shared_ptr<Name> _shared_interestName; // = make_shared<Name>(m_interestName);


  double
  CalculateDownloadSpeed();



};

} // namespace ndn
} // namespace ns3

#endif
