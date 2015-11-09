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

#include "ndn-file-consumer-wdw.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"



#include <math.h>


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumerWdw");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumerWdw);

TypeId
FileConsumerWdw::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumerWdw")
      .SetGroupName("Ndn")
      .SetParent<FileConsumerCbr>()
      .AddConstructor<FileConsumerWdw>()
    ;

  return tid;
}

FileConsumerWdw::FileConsumerWdw() : FileConsumerCbr()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerWdw::~FileConsumerWdw()
{
}



void
FileConsumerWdw::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  FileConsumerCbr::StartApplication();
  // the constructor of FileConsumerCbr should have calculated the maximum window size, so we are going to set it here
  m_maxWindowSize = m_windowSize;


  m_cwndSSThresh = 1000000;


  ignoreTimeoutsCounter = 0;
  ignoreGoodCounter = 0;

  m_cwndPhase = SlowStart;
}



void
FileConsumerWdw::IncrementWindow()
{
  NS_LOG_FUNCTION_NOARGS();
  double diff = ((double)m_maxWindowSize - m_windowSize)/2.0;

  if (diff > 0)
  {
    m_windowSize += diff;
  }
}


void
FileConsumerWdw::DecrementWindow()
{
  NS_LOG_FUNCTION_NOARGS();

  if (m_windowSize > 10)
    m_windowSize = m_windowSize / 2;

}



void
FileConsumerWdw::AfterData(bool manifest, bool timeout, uint32_t seq_nr)
{
  if (manifest)
  {
    SendPacket();
  } else
  {
    if (timeout && ignoreTimeoutsCounter == 0)
    {
      // had a timeout, and it's the first one of many...
      DecrementWindow();

      // ignore further timeouts for now
      ignoreTimeoutsCounter = m_windowSize;

      // we need to back off, therefore rescheduling the next send event
      ScheduleNextSendEvent(1000.0 / m_windowSize);
    }


    // if this was not a timeout, increase the window!
    if (!timeout)
    {
      if (ignoreGoodCounter == 0)
      {
        ignoreGoodCounter = m_windowSize;
        IncrementWindow();
      }

      // Schedule Next Event earlier, if necessary (most likely yes, because we increased the window)
      if (m_nextEventScheduleTime > Simulator::Now().GetMilliSeconds() + (1000.0 / m_windowSize))
      {
        ScheduleNextSendEvent(1000.0 / m_windowSize);
      }
    }

    if (ignoreGoodCounter > 0)
    {
      ignoreGoodCounter--;
    }

    if (ignoreTimeoutsCounter > 0)
    {
      ignoreTimeoutsCounter--;
    }


  }


}



} // namespace ndn
} // namespace ns3
