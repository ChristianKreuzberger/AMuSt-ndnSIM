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

#include "adaptation-logic-svc-buffer-based.hpp"

#include "multimedia-player.hpp"

namespace dash
{
namespace player
{


ISegmentURL*
AbstractSVCBufferBasedAdaptationLogic::GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation, bool *hasDownloadedAllSegments)
{
  unsigned int i = 0;
  // set i_curr to the maximum allowed level
  unsigned int i_curr = m_orderdByDepIdReps.size ()-1;
  unsigned int next_segment_number = -1;

  // find the "best" layer, that is CURRENTLY buffered
  while(m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[i_curr]->GetId()) == 0.0 && i_curr > 0)
  {
    i_curr--;
  }

  // Steady Phase
  while (i <= i_curr)
  {
    if (m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[i]->GetId()) < desired_buffer_size(i, i_curr))
    {
      // i is the next, need segment number though
      next_segment_number = getNextNeededSegmentNumber(i);

      if (next_segment_number < getTotalSegments ())
      {
        //fprintf(stderr, "Steady Phase: The next segment: SegNr=%d, Level=%d\n", next_segment_number, i);
        *requested_segment_number = next_segment_number;
        *usedRepresentation = m_orderdByDepIdReps[i];
        *hasDownloadedAllSegments = false;
        return m_orderdByDepIdReps[i]->GetSegmentList()->GetSegmentURLs().at(next_segment_number);
      }
      else
        *hasDownloadedAllSegments = true;
    }

    i++;
  }

  i = 0;
  // Growing Phase
  while (i <= i_curr)
  {
    if (m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[i]->GetId()) < desired_buffer_size(i, i_curr+2))
    {
      // i is the next, need segment number though
      next_segment_number = getNextNeededSegmentNumber(i);

      if (next_segment_number < getTotalSegments())
      {
        //fprintf(stderr, "Growing Phase: The next segment: SegNr=%d, Level=%d\n", next_segment_number, i);
        *requested_segment_number = next_segment_number;
        *usedRepresentation = m_orderdByDepIdReps[i];
        *hasDownloadedAllSegments = false;
        return m_orderdByDepIdReps[i]->GetSegmentList()->GetSegmentURLs().at(next_segment_number);
      }
      else
        *hasDownloadedAllSegments = true;
    }
    i++;
  }

  //fprintf(stderr, "Growing Phase done for i_curr=%d\n", i_curr);

  // Quality Increase Phase
  if (i <=  m_orderdByDepIdReps.size ()-1)
  {
    i_curr++;
    next_segment_number = getNextNeededSegmentNumber(i);

    if (next_segment_number < getTotalSegments())
    {
      //fprintf(stderr, "Quality Increase: The next segment: SegNr=%d, Level=%d\n", next_segment_number, i);

      *requested_segment_number = next_segment_number;
      *usedRepresentation = m_orderdByDepIdReps[i];
      *hasDownloadedAllSegments = false;
      return m_orderdByDepIdReps[i]->GetSegmentList()->GetSegmentURLs().at(next_segment_number);
    }
    else
      *hasDownloadedAllSegments = true;
  }

  //fprintf(stderr, "IDLE....\n");
  return NULL;
}

void
AbstractSVCBufferBasedAdaptationLogic::SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations)
{
  AdaptationLogic::SetAvailableRepresentations (availableRepresentations);
  orderRepresentationsByDepIds();

  //calc typical segment duration (we assume all reps have the same duration..)
  segment_duration = (double) m_orderdByDepIdReps.begin()->second->GetSegmentList()->GetDuration();
  segment_duration /= (double) m_orderdByDepIdReps.begin()->second->GetSegmentList()->GetTimescale ();
}

//this functions classifies reps into layers depending on the DepIds.
void AbstractSVCBufferBasedAdaptationLogic::orderRepresentationsByDepIds()
{
  //fprintf(stderr, "AbstractSVCBufferBasedAdaptationLogic::orderRepresentationsByDepIds()\n");

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

unsigned int AbstractSVCBufferBasedAdaptationLogic::desired_buffer_size(int i, int i_curr)
{
  return gamma + (int)std::ceil((i_curr - i) * alpha);
}

unsigned int AbstractSVCBufferBasedAdaptationLogic::getNextNeededSegmentNumber(int layer)
{
  // check buffer
  if (m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[layer]->GetId()) == 0)
  {
    // empty buffer, check if level = 0
    if (layer == 0)
    {
      // level 0 and buffer empty -> Means we need to fetch the next segmant that should be consumed
      return this->m_multimediaPlayer->nextSegmentNrToConsume();
    }
    else
    {
      // level != 0 and buffer empty? means we "should" be in quality increase phase
      // request next to consume + gamma

      //get the duration of a typical segment..
      return this->m_multimediaPlayer->nextSegmentNrToConsume() + floor((double)gamma/segment_duration);
    }
  }
  else
  {
    // get highest buffed segment number for level +1
    return m_multimediaPlayer->getHighestBufferedSegmentNr (m_orderdByDepIdReps[layer]->GetId()) + 1;
  }
}

bool AbstractSVCBufferBasedAdaptationLogic::hasMinBufferLevel(const dash::mpd::IRepresentation* rep)
{

  std::string repId = rep->GetId ();

  //determine layer of rep
  int layer = -1;
  for(std::map<int /*level*/, IRepresentation*>::iterator it = m_orderdByDepIdReps.begin (); it != m_orderdByDepIdReps.end (); ++it)
  {
    if(repId.compare (it->second->GetId ()) == 0)
    {
      layer = it->first;
      break;
    }
  }

  if(layer == -1)
  {
    fprintf(stderr, "Error could not determine layer for RepId\n");
    return true;
  }
  else if( layer == 0) // nerver stop download for layer 0
    return true;

  if (m_multimediaPlayer->GetBufferLevel(m_orderdByDepIdReps[layer - 1]->GetId()) < desired_buffer_size(layer-1, layer))
    return false;

  return true;
}

}
}
