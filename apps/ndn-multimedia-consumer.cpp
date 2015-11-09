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

#include "ndn-multimedia-consumer.hpp"
#include "ns3/core-module.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/system-path.h"

#include "ns3/boolean.h"


#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"


#include "utils/multimedia/multimedia-player.hpp"



NS_LOG_COMPONENT_DEFINE("ndn.MultimediaConsumer");

using namespace dash::mpd;

namespace ns3 {
namespace ndn {

typedef MultimediaConsumer<FileConsumerCbr> MultimediaConsumerCbr;
NS_OBJECT_ENSURE_REGISTERED(MultimediaConsumerCbr);

typedef MultimediaConsumer<FileConsumerWdw> MultimediaConsumerWdw;
NS_OBJECT_ENSURE_REGISTERED(MultimediaConsumerWdw);

template <typename Parent> std::string MultimediaConsumer<Parent>::alphabet = std::string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");

template<class Parent>
TypeId
MultimediaConsumer<Parent>::GetTypeId(void)
{
  static TypeId tid =
    TypeId((super::GetTypeId ().GetName () + "::MultimediaConsumer").c_str())
      .SetGroupName("Ndn")
      .template SetParent<super>()
      .template AddConstructor<MultimediaConsumer>()
      .template AddAttribute("MpdFileToRequest", "URI to the MPD File to Request", StringValue("/"),
                    MakeNameAccessor(&MultimediaConsumer<Parent>::m_mpdInterestName), MakeNameChecker())
      .template AddAttribute("ScreenWidth", "Width of the screen", UintegerValue(1920),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenWidth), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("ScreenHeight", "Height of the screen", UintegerValue(1080),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenHeight), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("MaxBufferedSeconds", "Maximum amount of buffered seconds allowed", UintegerValue(30),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_maxBufferedSeconds), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("DeviceType", "PC, Laptop, Tablet, Phone, Game Console", StringValue("PC"),
                    MakeStringAccessor(&MultimediaConsumer<Parent>::m_deviceType), MakeStringChecker())
      .template AddAttribute("AllowUpscale", "Define whether or not the client has capabilities to upscale content with lower resolutions", BooleanValue(true),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowUpscale), MakeBooleanChecker ())
      .template AddAttribute("AllowDownscale", "Define whether or not the client has capabilities to downscale content with higher resolutions", BooleanValue(false),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowDownscale), MakeBooleanChecker ())
      .template AddAttribute("AdaptationLogic", "Defines the adaptation logic to be used; ",
                          StringValue("dash::player::AlwaysLowestAdaptationLogic"),
                    MakeStringAccessor (&MultimediaConsumer<Parent>::m_adaptationLogicStr), MakeStringChecker ())
      .template AddAttribute("StartRepresentationId", """Defines the representation ID of the representation to start streaming; "
                          "can be either an ID from the MPD file or one of the following keywords: "
                          "lowest, auto (lowest = the lowest representation available, auto = use adaptation logic to decide)",
                          StringValue("auto"),
                    MakeStringAccessor (&MultimediaConsumer<Parent>::m_startRepresentationId), MakeStringChecker ())
      .template AddAttribute("TraceNotDownloadedSegments", "Defines wether to trace or not to trace not downloaded segments", BooleanValue(false),
                    MakeBooleanAccessor(&MultimediaConsumer<Parent>::traceNotDownloadedSegments), MakeBooleanChecker())
      .template AddAttribute("StartUpDelay", "Defines the time to wait before trying to start playback", DoubleValue(2.0),
                    MakeDoubleAccessor(&MultimediaConsumer<Parent>::startupDelay), MakeDoubleChecker<double>())
      .AddTraceSource("PlayerTracer", "Trace Player consumes of multimedia data",
                      MakeTraceSourceAccessor(&MultimediaConsumer<Parent>::m_playerTracer))
                    ;

  return tid;
}

template<class Parent>
MultimediaConsumer<Parent>::MultimediaConsumer() : super()
{
  NS_LOG_FUNCTION_NOARGS();
  mpd = NULL;
  mPlayer = NULL;
}


template<class Parent>
MultimediaConsumer<Parent>::~MultimediaConsumer()
{
}



///////////////////////////////////////////////////
//             Application Methods               //
///////////////////////////////////////////////////

// Start Application - initialize variables etc...
template<class Parent>
void
MultimediaConsumer<Parent>::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  NS_LOG_DEBUG("Starting Multimedia Consumer - Device Type: " << m_deviceType);
  NS_LOG_DEBUG("Screen Resolution: " << m_screenWidth << "x" << m_screenHeight);
  NS_LOG_DEBUG("MPD File: " << m_mpdInterestName);
  NS_LOG_DEBUG("SuperClass: " << super::GetTypeId ().GetName ());

  uint32_t node_id = super::GetNode ()->GetId();

  std::stringstream ss_tempDir;
  ss_tempDir << "/" << node_id;

  boost::random::random_device rng;
  boost::random::uniform_int_distribution<> index_dist(0, MultimediaConsumer<Parent>::alphabet.size() - 1);
  for(int i = 0; i < 8; ++i)
     ss_tempDir << MultimediaConsumer<Parent>::alphabet[index_dist(rng)];

  m_tempDir = ns3::SystemPath::MakeTemporaryDirectoryName() + ss_tempDir.str();

  NS_LOG_DEBUG("Temporary Directory: " << m_tempDir);
  ns3::SystemPath::MakeDirectories(m_tempDir);

  m_tempMpdFile = m_tempDir + "/mpd.xml.gz";

  m_mpdParsed = false;
  m_initSegmentIsGlobal = false;
  m_hasInitSegment = false;
  m_hasDownloadedAllSegments = false;
  m_hasStartedPlaying = false;
  m_freezeStartTime = 0;
  totalConsumedSegments = 0;
  requestedRepresentation = NULL;
  requestedSegmentURL = NULL;

  m_currentDownloadType = MPD;
  m_startTime = Simulator::Now().GetMilliSeconds();

  NS_LOG_DEBUG("Trying to instantiate MultimediaPlayer(" << m_adaptationLogicStr << ")");

  mPlayer = new dash::player::MultimediaPlayer(m_adaptationLogicStr, m_maxBufferedSeconds);

  NS_ASSERT_MSG(mPlayer->GetAdaptationLogic() != nullptr,
          "Could not initialize adaptation logic...");

  super::SetAttribute("FileToRequest", StringValue(m_mpdInterestName.toUri()));
  super::SetAttribute("WriteOutfile", StringValue(m_tempMpdFile));

  // do base stuff
  super::StartApplication();
}


// Stop Application - Cancel any outstanding events
template<class Parent>
void
MultimediaConsumer<Parent>::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  m_consumerLoopTimer.Cancel();
  Simulator::Cancel(m_consumerLoopTimer);

  m_downloadEventTimer.Cancel();
  Simulator::Cancel(m_downloadEventTimer);

  /*OK LOG ALL NOT RECEIVED FILES FROM MPD*/
  if(traceNotDownloadedSegments)
  {
    //check if mpd and player exists
    if(mpd != NULL && mPlayer != NULL)
    {
      //first consume everything from buffer
      while(consume() > 0.0);

      //ok check how many segments we have not consumed
      while(totalConsumedSegments < mPlayer->GetAdaptationLogic()->getTotalSegments())
      {
        m_playerTracer(this, totalConsumedSegments++,  "0",
                       0, 0, 0, std::vector<std::string>());
      }
    }
  }

  // clean up mpd/DASH specific stuff
  if (mpd != NULL)
    delete mpd;

  if (mPlayer != NULL)
    delete mPlayer;

  mpd = NULL;
  mPlayer = NULL;

  // cleanup base stuff
  super::StopApplication();
}



template<class Parent>
void
MultimediaConsumer<Parent>::OnMpdFile()
{

  // check if file was gziped, if not, we use it as is
  if (m_tempMpdFile.find(".gz") != std::string::npos)
  {
    // file was compressed, decompress it
    NS_LOG_DEBUG("GZIP MPD File " << m_tempMpdFile << " received. Decompressing...");
    std::string newFileName = m_tempMpdFile.substr(0, m_tempMpdFile.find(".gz"));

    if(FileConsumer::DecompressFile(m_tempMpdFile, newFileName))
      m_tempMpdFile = newFileName;
  }


  NS_LOG_DEBUG("MPD File " << m_tempMpdFile << " received. Parsing now...");


  dash::IDASHManager *manager;
  manager = CreateDashManager();
  mpd = manager->Open((char*)m_tempMpdFile.c_str());

  // We don't need the manager anymore...
  manager->Delete();
  manager = NULL;

  if (mpd == NULL)
  {
    NS_LOG_ERROR("Error parsing mpd " << m_tempMpdFile);
    return;
  }

  // we are assuming there is only 1 period, get the first one
  IPeriod *currentPeriod = mpd->GetPeriods().at(0);

  // get base URLs
  m_baseURL = "";
  std::vector<dash::mpd::IBaseUrl*> baseUrls = mpd->GetBaseUrls ();

  if (baseUrls.size() > 0)
  {
    if (baseUrls.size() == 1)
    {
      m_baseURL = baseUrls.at(0)->GetUrl();
    }
    else
    {
      int randUrl = rand() % baseUrls.size();
      NS_LOG_DEBUG("Mutliple base URLs available, selecting a random one... ");
      m_baseURL = baseUrls.at(randUrl)->GetUrl();
    }

    if(m_baseURL.substr (0,7).compare ("http://") == 0)
      m_baseURL = m_baseURL.substr(6,m_baseURL.length ());

    NS_LOG_DEBUG("Base URL: " << m_baseURL);
  }
  else
  {
    NS_LOG_ERROR("No Base URL provided in MPD file... exiting.");
    return;
  }

  // Get the adaptation sets, though we are only takeing the first one
  std::vector<IAdaptationSet *> allAdaptationSets = currentPeriod->GetAdaptationSets();

  // we are assuming that allAdaptationSets.size() == 1
  if (allAdaptationSets.size() == 0)
  {
    NS_LOG_ERROR("No adaptation sets found in MPD file... exiting.");
    return;
  }

  // use first adaptation set
  IAdaptationSet* adaptationSet = allAdaptationSets.at(0);

  // check if the adaptation set has an init segment
  // alternatively, the init segment is representation-specific
  NS_LOG_DEBUG("Checking for init segment in adaptation set...");
  std::string initSegment = "";

  if (adaptationSet->GetSegmentBase () && adaptationSet->GetSegmentBase ()->GetInitialization ())
  {
    NS_LOG_DEBUG("Adaptation Set has INIT Segment");
    // get URL to init segment
    initSegment = adaptationSet->GetSegmentBase ()->GetInitialization ()->GetSourceURL ();
    // TODO: request init segment
    m_initSegmentIsGlobal = true;
    m_hasInitSegment = true;
  }
  else
  {
    NS_LOG_DEBUG("Adaptation Set does not have INIT Segment");
    m_hasInitSegment = false;
    /*if (adaptationSet->GetRepresentation().at(0)->GetSegmentBase())
    {
      std::cerr << "Alternative: " << adaptationSet->GetRepresentation().at(0)->GetSegmentBase()->GetInitialization()->GetSourceURL() << std::endl;
    }*/
  }



  // get all representations
  std::vector<IRepresentation*> reps = adaptationSet->GetRepresentation();

  NS_LOG_DEBUG("MPD file contains " << reps.size() << " Representations: ");
  NS_LOG_DEBUG("Start Representation: " << m_startRepresentationId);

    // calculate segment duration
  // reps.at(0)->GetSegmentList()->GetDuration();
  NS_LOG_DEBUG("Period Duration:" << reps.at(0)->GetSegmentList()->GetDuration());

  bool startRepresentationSelected = false;

  std::string firstRepresentationId = "";
  std::string bestRepresentationBasedOnBandwidth = "";

  //double downloadSpeed = super::CalculateDownloadSpeed() * 8;
  mPlayer->SetLastDownloadBitRate(super::lastDownloadBitrate);


  NS_LOG_DEBUG("Download Speed of MPD file was : " << super::lastDownloadBitrate << " bits per second");
  m_isLayeredContent = false;

  m_availableRepresentations.clear();
  for (IRepresentation* rep : reps)
  {
    unsigned int width = rep->GetWidth();
    unsigned int height = rep->GetHeight();

    // if not allowed to upscale, skip this representation
    if (!m_allowUpscale && width < this->m_screenWidth && height < this->m_screenHeight)
    {
      continue;
    }

    // if not allowed to downscale and width/height are too large, skip this representation
    if (!m_allowDownscale && width > this->m_screenWidth && height > this->m_screenHeight)
    {
      continue;
    }

    std::string repId = rep->GetId();

    if (firstRepresentationId == "")
      firstRepresentationId = repId;

    // else: Use this representation and add it to available representations
    std::vector<std::string> dependencies = rep->GetDependencyId ();

    unsigned int requiredDownloadSpeed = rep->GetBandwidth();

    if (dependencies.size() > 0) // we found out that this is layered content
      m_isLayeredContent = true;

    NS_LOG_DEBUG("ID = " << repId << ", DepId=" <<
        dependencies.size() << ", width=" << width << ", height=" << height << ", bwReq=" << requiredDownloadSpeed);


    if (!startRepresentationSelected && m_startRepresentationId != "lowest")
    {
      if (m_startRepresentationId == "auto")
      {
        // do we have enough bandwidth available?
        if (super::lastDownloadBitrate > requiredDownloadSpeed)
        {
          // yes we do!
          bestRepresentationBasedOnBandwidth = repId;
        }
      }
      else if (rep->GetId() == m_startRepresentationId)
      {
        NS_LOG_DEBUG("The last representation is the start representation!");
        startRepresentationSelected = true;
      }
    }

    m_availableRepresentations[repId] = rep;
  }

  /** TODO: What if there are several representations bith the same bitrate but different spatial resolutions? How to pick the right one... 
      We need to have a utility value for each representation
  */

  // check m_startRepresentationId
  if (m_startRepresentationId == "lowest")
  {
    NS_LOG_DEBUG("Using Lowest available representation; ID = " << firstRepresentationId);
    m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  } else if (m_startRepresentationId == "auto")
  {
    // select representation based on bandwidth
    if (bestRepresentationBasedOnBandwidth != "")
    {
      NS_LOG_DEBUG("Using best representation based on bandwidth; ID = " << bestRepresentationBasedOnBandwidth);
      m_startRepresentationId = bestRepresentationBasedOnBandwidth;
      startRepresentationSelected = true;
    }
  }

  // was there a start representation selected?
  if (!startRepresentationSelected)
  {
    // IF NOT, default to lowest
    NS_LOG_DEBUG("No start representation selected, default to lowest available representation; ID = " << firstRepresentationId);
    m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  }

  m_curRepId = m_startRepresentationId;

  // okay, check init segment
  if (initSegment == "" && m_hasInitSegment == true)
  {
    NS_LOG_DEBUG("Using init segment of representation " << m_startRepresentationId);
    initSegment = m_availableRepresentations[m_startRepresentationId]->GetSegmentBase()->GetInitialization()->GetSourceURL();
    NS_LOG_DEBUG("Init Segment URL = " << initSegment);
  }


  m_mpdParsed = true;
  mPlayer->SetAvailableRepresentations(&m_availableRepresentations);


  // trigger MPD parsed after x seconds
  unsigned long curTime = Simulator::Now().GetMilliSeconds();
  NS_LOG_DEBUG("MPD received after " << (curTime - m_startTime) << " ms");

  if (initSegment == "")
  {
    NS_LOG_DEBUG("No init Segment selected.");
    // schedule streaming of first segment
    m_currentDownloadType = Segment;
    ScheduleDownloadOfSegment();
  } else
  {
    // Schedule streaming of init segment
    m_initSegment = initSegment;
    m_currentDownloadType = InitSegment;
    ScheduleDownloadOfInitSegment();
  }

  // we received the MDP, so we can now start the timer for playing
  SchedulePlay(startupDelay);
}



template<class Parent>
void
MultimediaConsumer<Parent>::OnMultimediaFile()
{
  NS_LOG_DEBUG("On Multimedia File: " << super::m_interestName);

  if (!super::m_active)
    return;

  // get the current representation id
  // and check if this was an init segment
  if (m_currentDownloadType == InitSegment)
  {
    // init segment
    if (m_initSegmentIsGlobal)
    {
      m_downloadedInitSegments.push_back("GlobalAdaptationSet");
      NS_LOG_DEBUG("Global Init Segment received");
    } else
    {
      m_downloadedInitSegments.push_back(m_curRepId);
      NS_LOG_DEBUG("Init Segment received (rep=" << m_curRepId << ")");
    }

  }
  else
  {
    // normal segment

    //fprintf(stderr, "lastBitrate = %f\n", super::lastDownloadBitrate);
    mPlayer->SetLastDownloadBitRate(super::lastDownloadBitrate);

    // check if there is enough space in buffer 
    if(mPlayer->EnoughSpaceInBuffer(requestedSegmentNr, requestedRepresentation, m_isLayeredContent))
    {
      if(mPlayer->AddToBuffer(requestedSegmentNr, requestedRepresentation, super::lastDownloadBitrate, m_isLayeredContent))
        NS_LOG_DEBUG("Segment Accepted for Buffering");
      else
        NS_LOG_DEBUG("Segment Rejected for Buffering");
    }
    else
    {
      // try again in 1 second, and again and again... but do not donwload anything in the meantime
      Simulator::Schedule(Seconds(1.0), &MultimediaConsumer<Parent>::OnMultimediaFile, this);
      return;
    }
  }

  m_currentDownloadType = Segment;
  ScheduleDownloadOfSegment();
}


template<class Parent>
void
MultimediaConsumer<Parent>::OnFileReceived(unsigned status, unsigned length)
{
  // make sure that the file is being properly retrieved by the super class first!
  super::OnFileReceived(status, length);

  if (!m_mpdParsed)
  {
    OnMpdFile();
  } else
  {
    OnMultimediaFile();
  }

}



template<class Parent>
void
MultimediaConsumer<Parent>::OnData(shared_ptr<const Data> data)
{
  if(requestedSegmentURL == NULL)
  {
    super::OnData(data);
    return;
  }

  std::string interestName = data->getName().toUri();

  if(boost::starts_with(interestName, m_baseURL+(requestedSegmentURL->GetMediaURI())))
  {
    super::OnData(data);
  }
  // else
  // ignore
}


template<class Parent>
void
MultimediaConsumer<Parent>::ScheduleDownloadOfInitSegment()
{
  // wait some time (10 milliseconds) before requesting the next first segment
  // basically, we simulate that parsing the MPD takes 10 ms on the client
  // this assumption might not be true generally speaking, but not waiting at all
  // is worse.
  Simulator::Schedule(Seconds(0.01), &MultimediaConsumer<Parent>::DownloadInitSegment, this);
}


template<class Parent>
void
MultimediaConsumer<Parent>::DownloadInitSegment()
{
  NS_LOG_DEBUG("Downloading init segment... " << m_baseURL + m_initSegment << ";");
  super::StopApplication();
  super::SetAttribute("FileToRequest", StringValue(m_baseURL + m_initSegment));
  super::SetAttribute("WriteOutfile", StringValue(""));
  super::StartApplication();
}



template<class Parent>
void
MultimediaConsumer<Parent>::ScheduleDownloadOfSegment()
{
  // wait 1 ms (dummy time) before downloading next segment - this prevents some issues
  // with start/stop application and interests coming in late.
  m_downloadEventTimer.Cancel();
  m_downloadEventTimer = Simulator::Schedule(Seconds(0.001), &MultimediaConsumer<Parent>::DownloadSegment, this);
}


template<class Parent>
void
MultimediaConsumer<Parent>::DownloadSegment()
{
  // Not needed on Buffer insert we will wait!
  /*if (mPlayer->GetBufferLevel() >= m_maxBufferedSeconds)
  {
    NS_LOG_DEBUG("Player Buffer=" << mPlayer->GetBufferLevel() << ", MaxBuffer= " << m_maxBufferedSeconds << ", pausing download...");
    // we can wait before we need to downlaod something again - but for how long?
    // seems that half of segment-length should be good

    Simulator::Schedule(Seconds(1.0), &MultimediaConsumer<Parent>::DownloadSegment, this);
    return;
  }*/

  // get segment number and rep id
  requestedRepresentation = NULL;
  requestedSegmentNr = 0;

  requestedSegmentURL = mPlayer->GetAdaptationLogic()->GetNextSegment(&requestedSegmentNr, &requestedRepresentation, &m_hasDownloadedAllSegments);

  if(m_hasDownloadedAllSegments) // DONE
  {
    NS_LOG_DEBUG("No more segments available for download!\n");
    return;
  }

  if (requestedSegmentURL == NULL) //IDLE
  {
    NS_LOG_DEBUG("IDLE\n");
    m_downloadEventTimer = Simulator::Schedule(Seconds(1.0), &MultimediaConsumer<Parent>::DownloadSegment, this);
    return;
  }

  super::StopApplication();
  super::SetAttribute("FileToRequest", StringValue(m_baseURL + requestedSegmentURL->GetMediaURI()));
  super::SetAttribute("WriteOutfile", StringValue(""));
  super::SetAttribute("StartWindowSize", StringValue("10"));
  super::StartApplication();
}





template<class Parent>
void
MultimediaConsumer<Parent>::SchedulePlay(double wait_time)
{
  m_consumerLoopTimer.Cancel();
  m_consumerLoopTimer = Simulator::Schedule(Seconds(wait_time), &MultimediaConsumer<Parent>::DoPlay, this);
}



template<class Parent>
void
MultimediaConsumer<Parent>::DoPlay()
{
  double consumed_sec = consume();

  if(consumed_sec > 0) // we play
  {
    SchedulePlay(consumed_sec);
  }
  else if(consumed_sec == 0.0 && m_hasDownloadedAllSegments)
  {
    //we finished streaming just return
    return;
  }
  else //we stall
  {
     //restart timer
     SchedulePlay(); // with default parm.

    //check if we should abort the download
    if(requestedRepresentation != NULL && !m_hasDownloadedAllSegments && requestedRepresentation->GetDependencyId().size() > 0) // means we are downloading something with dependencies
    {
      //check buffer state
      if(!mPlayer->GetAdaptationLogic()->hasMinBufferLevel(requestedRepresentation))
      {
        //abort download ...
        NS_LOG_DEBUG("Aborting to download a segment with repId = " << requestedRepresentation->GetId().c_str());
        super::StopApplication();
        mPlayer->SetLastDownloadBitRate(0.0);//set dl_bitrate to zero.
        ScheduleDownloadOfSegment();
      }
    }
  }
}

template<class Parent>
double
MultimediaConsumer<Parent>::consume()
{
  unsigned int buffer_level = mPlayer->GetBufferLevel();

  NS_LOG_DEBUG("Cur Buffer Level = " << buffer_level);

  // did we finish streaming yet?
  if (buffer_level == 0 && m_hasDownloadedAllSegments == true)
     return 0.0;

  dash::player::MultimediaBuffer::BufferRepresentationEntry entry = mPlayer->ConsumeFromBuffer();
  double consumedSeconds = entry.segmentDuration;
  if ( consumedSeconds > 0)
  {
    NS_LOG_DEBUG("Consumed Segment " << entry.segmentNumber << ", with Rep " << entry.repId << " for " << entry.segmentDuration << "seconds");
    int64_t freezeTime = 0;
    if (!m_hasStartedPlaying)
    {
      // we havent started yet, so we can measure the start-up delay until now
      m_hasStartedPlaying = true;
      int64_t startUpDelay = Simulator::Now().GetMilliSeconds() - m_startTime;
      // LOG STARTUP DELAY HERE
      freezeTime = startUpDelay;
      NS_LOG_DEBUG("Started consuming ... (Start-Up Delay: " << startUpDelay << " milliseconds)");
    }
    else if (m_freezeStartTime != 0)
    {
      // we had a freeze/stall, but we can continue playing now
      // measure:
      freezeTime = (Simulator::Now().GetMilliSeconds() - m_freezeStartTime);
      m_freezeStartTime = 0;
      NS_LOG_DEBUG("Freeze Of " << freezeTime << " milliseconds is over!");
    }

    //fprintf(stderr,  "Current Buffer Level: %f\n", mPlayer->GetBufferLevel());
    m_playerTracer(this, entry.segmentNumber, entry.repId,entry.experienced_bitrate_bit_s, freezeTime, (unsigned int) (mPlayer->GetBufferLevel()), entry.depIds);

    NS_LOG_DEBUG("Consuming " << consumedSeconds << " seconds from buffer...");
    totalConsumedSegments++;
    return consumedSeconds;
  }
  else
  {
    // could not consume, means buffer is empty
    if (m_freezeStartTime == 0 && m_hasStartedPlaying == true)
    {
      // this actually means that we have a stall/free (m_hasStartedPlaying == false would mean that this is part of start up delay)
      // set m_freezeStartTime
      m_freezeStartTime = Simulator::Now().GetMilliSeconds();
    }

    // continue trying to consume... - these are unsmooth seconds
    return 0.0; // default parameter
  }
}


} // namespace ndn
} // namespace ns3
