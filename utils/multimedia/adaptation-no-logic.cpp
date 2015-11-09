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

#include "adaptation-no-logic.hpp"
#include "multimedia-player.hpp"


namespace dash
{
namespace player
{

ENSURE_ADAPTATION_LOGIC_INITIALIZED(SVCNoAdaptationLogic)

ISegmentURL*
SVCNoAdaptationLogic::GetNextSegment(unsigned int *requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool *hasDownloadedAllSegments)
{

  unsigned int max_layer = m_orderdByDepIdReps.size ()-1;
  unsigned int next_segment_number = 0;
  bool segment_number_set = false;
  unsigned int chosen_layer = 0;

  // determine next layer to download segment from
  double buffer_level_lowest_layer = m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[0]->GetId());
  //fprintf(stderr, "buffer_level_lowest_layer = %f\n", buffer_level_lowest_layer);

  if(buffer_level_lowest_layer == 0.0)
  {
    next_segment_number = getNextNeededSegmentNumber(0);
    chosen_layer = 0;
    segment_number_set = true;
  }
  else // find layer with lowest buffered seconds
  {
    unsigned int l = 1;
    while (l <= max_layer)
    {
      if(buffer_level_lowest_layer > m_multimediaPlayer->GetBufferLevel (m_orderdByDepIdReps[l]->GetId()))
      {
        next_segment_number = getNextNeededSegmentNumber(l);
        chosen_layer = l;
        segment_number_set = true;
        break;
      }
      l++;
    }
  }

  if(segment_number_set == false)
    next_segment_number = getNextNeededSegmentNumber(0);

  if (next_segment_number < getTotalSegments ())
  {
    *requested_segment_number = next_segment_number;
    *usedRepresentation = m_orderdByDepIdReps[chosen_layer];
    *hasDownloadedAllSegments = false;
    return m_orderdByDepIdReps[chosen_layer]->GetSegmentList()->GetSegmentURLs().at(next_segment_number);
  }

  *hasDownloadedAllSegments = true;
  return NULL;
}

bool SVCNoAdaptationLogic::hasMinBufferLevel(const dash::mpd::IRepresentation* rep)
{
  if(m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[0]->GetId()) > 0.0)
     return true;

  return false;
}

void SVCNoAdaptationLogic::SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations)
{
  AdaptationLogic::SetAvailableRepresentations (availableRepresentations);
  orderRepresentationsByDepIds();
}

unsigned int SVCNoAdaptationLogic::getNextNeededSegmentNumber(int layer)
{
  // check buffer
  if (m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[layer]->GetId()) == 0)
  {
    // empty buffer -> just request next Segment that should be consumed
      return this->m_multimediaPlayer->nextSegmentNrToConsume();
  }
  else
  {
    // get highest buffed segment number for level +1
    return m_multimediaPlayer->getHighestBufferedSegmentNr (m_orderdByDepIdReps[layer]->GetId()) + 1;
  }
}

void SVCNoAdaptationLogic::orderRepresentationsByDepIds()
{
  std::map<std::string/*RepId*/, IRepresentation*> reps = *m_availableRepresentations;
  std::map<std::string/*RepId*/, IRepresentation*> selectedReps;

  //fprintf(stderr, "m_availableRepresentations.size()=%d\n",m_availableRepresentations->size ());
  //fprintf(stderr, "reps.size()=%d\n",reps.size ());

  m_orderdByDepIdReps.clear ();
  int level = 0;

  while(reps.size () > 0)
  {
    //find rep with the lowest depIds.
    std::map<std::string/*RepId*/, IRepresentation*>::iterator lowest = reps.begin ();
    for(std::map<std::string/*RepId*/, IRepresentation*>::iterator it = reps.begin (); it!=reps.end (); ++it)
    {
      if(lowest->second->GetDependencyId().size() < lowest->second->GetDependencyId().size())
        lowest = it;
    }
    std::vector<std::string> v = lowest->second->GetDependencyId();
    // check if all required DepIds are in m_orderdByDepIdReps
    for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); it++ )
    {
      if(selectedReps.find(*it) == selectedReps.end ())
      {
        fprintf(stderr, "Error in ordering Representations based on DepIds\n");
        return;
      }
    }
    m_orderdByDepIdReps[level++] = lowest->second;
    selectedReps[lowest->first] = lowest->second;
    reps.erase (lowest);
  }

  //for(std::map<int /*level*/, IRepresentation*>::iterator it = m_orderdByDepIdReps.begin (); it!=m_orderdByDepIdReps.end (); it++)
  //  fprintf(stderr, "layer[%d]=%s\n",it->first, it->second->GetId().c_str());
}

}
}
