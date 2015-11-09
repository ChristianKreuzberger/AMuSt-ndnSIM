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

#ifndef DASH_ALWAYS_NO_LOGIC
#define DASH_ALWAYS_NO_LOGIC

#include "adaptation-logic.hpp"
#include <climits>

namespace dash
{
namespace player
{

class SVCNoAdaptationLogic : public AdaptationLogic
{
public:
  SVCNoAdaptationLogic(MultimediaPlayer* mPlayer) : AdaptationLogic (mPlayer)
  {
    currentSegmentNumber = 0;
  }

  virtual std::string GetName() const
  {
    return "dash::player::SVCNoAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<SVCNoAdaptationLogic>(mPlayer);
  }

  virtual ISegmentURL*
  GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation, bool* hasDownloadedAllSegments);

  virtual bool
  hasMinBufferLevel(const dash::mpd::IRepresentation* rep);

  virtual void
  SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations);


protected:

  void orderRepresentationsByDepIds();
  unsigned int getNextNeededSegmentNumber(int layer);

  static SVCNoAdaptationLogic _staticLogic;
  unsigned int currentSegmentNumber;

  std::map<int /*level*/, IRepresentation*> m_orderdByDepIdReps;

  SVCNoAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCNoAdaptationLogic);
  }


};
}
}


#endif // DASH_ALWAYS_LOWEST_ADAPTATION_LOGIC
