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

#ifndef ADAPTATIONLOGICSVCBUFFERBASED_HPP
#define ADAPTATIONLOGICSVCBUFFERBASED_HPP

#include "adaptation-logic.hpp"
#include <cmath>

#define BUFFER_MIN_SIZE 16 // in seconds
#define BUFFER_ALPHA 8 // in seconds

#define BUFFER_MIN_SIZE_CONSERVATIVE 16 // in seconds
#define BUFFER_ALPHA_CONSERVATIVE 8 // in seconds

#define BUFFER_MIN_SIZE_AGGRESSIVE 8 // in seconds
#define BUFFER_ALPHA_AGGRESSIVE 2 // in seconds

#define BUFFER_MIN_SIZE_NORMAL 8 // in seconds
#define BUFFER_ALPHA_NORMAL 4 // in seconds

namespace dash
{
namespace player
{
class AbstractSVCBufferBasedAdaptationLogic : public AdaptationLogic
{
public:
  AbstractSVCBufferBasedAdaptationLogic(MultimediaPlayer* mPlayer, double buffer_alpha, double buffer_min_size) : AdaptationLogic (mPlayer)
  {
    alpha = buffer_alpha;
    gamma = buffer_min_size;
  }

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations);

  virtual ISegmentURL*
  GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation,  bool* hasDownloadedAllSegments);

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);


protected:
  AbstractSVCBufferBasedAdaptationLogic()
  {
  }

  void orderRepresentationsByDepIds();
  unsigned int desired_buffer_size(int i, int i_curr);
  unsigned int getNextNeededSegmentNumber(int layer);

  std::map<int /*level*/, IRepresentation*> m_orderdByDepIdReps;

  double alpha;
  int gamma; //BUFFER_MIN_SIZE

  double segment_duration;

  enum AdaptationPhase
  {
    Steady = 0,
    Growing = 1,
    Upswitching = 2
  };

  AdaptationPhase lastPhase;
  AdaptationPhase allowedPhase;
};


class SVCBufferBasedAdaptationLogicAggressive : AbstractSVCBufferBasedAdaptationLogic
{
  public:
  virtual std::string GetName() const
  {
    return "dash::player::SVCBufferBasedAdaptationLogicAggressive";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<AbstractSVCBufferBasedAdaptationLogic>(mPlayer, BUFFER_ALPHA_AGGRESSIVE, BUFFER_MIN_SIZE_AGGRESSIVE);
  }

  protected:
  static SVCBufferBasedAdaptationLogicAggressive _staticLogic;

  SVCBufferBasedAdaptationLogicAggressive()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferBasedAdaptationLogicAggressive);
  }

};



class SVCBufferBasedAdaptationLogicConservative : AbstractSVCBufferBasedAdaptationLogic
{
  public:
  virtual std::string GetName() const
  {
    return "dash::player::SVCBufferBasedAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<AbstractSVCBufferBasedAdaptationLogic>(mPlayer, BUFFER_ALPHA_CONSERVATIVE, BUFFER_MIN_SIZE_CONSERVATIVE);
  }

  protected:
  static SVCBufferBasedAdaptationLogicConservative _staticLogic;

  SVCBufferBasedAdaptationLogicConservative()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferBasedAdaptationLogicConservative);
  }
};



class SVCBufferBasedAdaptationLogicNormal : AbstractSVCBufferBasedAdaptationLogic
{
  public:
  virtual std::string GetName() const
  {
    return "dash::player::SVCBufferBasedAdaptationLogicNormal";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<AbstractSVCBufferBasedAdaptationLogic>(mPlayer, BUFFER_ALPHA_NORMAL, BUFFER_MIN_SIZE_NORMAL);
  }

  protected:
  static SVCBufferBasedAdaptationLogicNormal _staticLogic;

  SVCBufferBasedAdaptationLogicNormal()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferBasedAdaptationLogicNormal);
  }
};


}
}
#endif // ADAPTATIONLOGICSVCBUFFERBASED_HPP
