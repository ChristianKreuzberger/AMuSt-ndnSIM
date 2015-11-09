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

#include "multimedia-player.hpp"



namespace dash
{
namespace player
{
/*MultimediaPlayer::MultimediaPlayer() : MultimediaPlayer("dash::player::AdaptationLogic")
{
  m_lastBitrate = 0.0;
}*/

MultimediaPlayer::~MultimediaPlayer()
{
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
  std::cerr << "Deleting MultimediaPlayer;";
#endif
  m_adaptLogic = nullptr;

  if(m_buffer)
    delete(m_buffer);
}

MultimediaPlayer::MultimediaPlayer(std::string AdaptationLogicStr, unsigned int maxBufferedSeconds)
{
  m_buffer = new MultimediaBuffer(maxBufferedSeconds);
  m_lastBitrate = 0;
  std::shared_ptr<AdaptationLogic> aLogic = AdaptationLogicFactory::Create(AdaptationLogicStr, this);

  if (aLogic == nullptr)
  {
    std::cerr << "MultimediaPlayer():\tFailed initializing adaptation logic '" << AdaptationLogicStr << "'" << std::endl;
  }
  else
  {
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
    std::cerr << "MultimediaPlayer():\tInitialized adaptation logic of type " << aLogic->GetName() << std::endl;
#endif
    m_adaptLogic = aLogic;
  }
}



std::shared_ptr<AdaptationLogic>&
MultimediaPlayer::GetAdaptationLogic()
{
  return m_adaptLogic;
}



void
MultimediaPlayer::SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations)
{
  this->m_availableRepresentations = availableRepresentations;
  m_adaptLogic->SetAvailableRepresentations(availableRepresentations);
}


bool
MultimediaPlayer::AddToBuffer(unsigned int segmentNr, const dash::mpd::IRepresentation *usedRepresentation, float experiencedDownloadBitrate, bool isLayeredContent)
{
  // ignore isLayeredContent for now
  return m_buffer->addToBuffer (segmentNr, usedRepresentation, experiencedDownloadBitrate);
}

bool
MultimediaPlayer::EnoughSpaceInBuffer(unsigned int segmentNr, const dash::mpd::IRepresentation *usedRepresentation, bool isLayeredContent)
{
  if (isLayeredContent) // e.g., SVC
    return m_buffer->enoughSpaceInLayeredBuffer (segmentNr, usedRepresentation);
  else // e.g., AVC
    return m_buffer->enoughSpaceInTotalBuffer (segmentNr, usedRepresentation);
}


double
MultimediaPlayer::GetBufferLevel(std::string repId)
{
  if(repId.compare ("NULL") == 0)
    return m_buffer->getBufferedSeconds ();
  else
    return m_buffer->getBufferedSeconds (repId);
}

double
MultimediaPlayer::GetBufferPercentage(std::string repId)
{
  if(repId.compare ("NULL") == 0)
    return m_buffer->getBufferedPercentage ();
  else
    return m_buffer->getBufferedPercentage (repId);
}

unsigned int
MultimediaPlayer::getHighestBufferedSegmentNr(std::string repId)
{
  return m_buffer->getHighestBufferedSegmentNr (repId);
}

MultimediaBuffer::BufferRepresentationEntry MultimediaPlayer::ConsumeFromBuffer()
{
  return m_buffer->consumeFromBuffer ();
}

unsigned int
MultimediaPlayer::nextSegmentNrToConsume ()
{
  return m_buffer->nextSegmentNrToBeConsumed ();
}


void
MultimediaPlayer::SetLastDownloadBitRate(double bitrate)
{
  this->m_lastBitrate = bitrate;
}


double
MultimediaPlayer::GetLastDownloadBitRate()
{
  return this->m_lastBitrate;
}


}
}
