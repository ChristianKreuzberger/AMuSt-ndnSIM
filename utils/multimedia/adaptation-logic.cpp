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

#include "adaptation-logic.hpp"
#include "multimedia-player.hpp"


namespace dash
{
namespace player
{
ENSURE_ADAPTATION_LOGIC_INITIALIZED(AdaptationLogic)


AdaptationLogic::AdaptationLogic(MultimediaPlayer* mPlayer)
{
  this->m_multimediaPlayer = mPlayer;
}



AdaptationLogic::~AdaptationLogic()
{
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
  std::cerr << "Adaptation Logic deconstructing..." << std::endl;
#endif
}


void
AdaptationLogic::SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations)
{
  this->m_availableRepresentations = availableRepresentations;
}



ISegmentURL*
AdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool *hasDownloadedAllSegments)
{
  *requested_segment_number = 0;
  *usedRepresentation = NULL;
  *hasDownloadedAllSegments = false;
  return NULL;
}


IRepresentation*
AdaptationLogic::GetLowestRepresentation()
{
    return (this->m_availableRepresentations->begin()->second);
}

// we assume that in all represntation the same amount of segments exists..
unsigned int AdaptationLogic::getTotalSegments()
{
  return m_availableRepresentations->begin()->second->GetSegmentList ()->GetSegmentURLs().size();
}

bool AdaptationLogic::hasMinBufferLevel(const dash::mpd::IRepresentation* rep)
{
  return true;
}



}
}
