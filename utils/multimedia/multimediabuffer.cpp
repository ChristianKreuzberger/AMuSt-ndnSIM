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


#include "multimediabuffer.hpp"

using namespace dash::player;

MultimediaBuffer::MultimediaBuffer(unsigned int maxBufferedSeconds)
{
  this->maxBufferedSeconds= (double) maxBufferedSeconds;

  toBufferSegmentNumber = 0;
  toConsumeSegmentNumber = 0;
}

bool MultimediaBuffer::addToBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation, float experiencedDownloadBitrate)
{
  //check if we receive a segment with a too large number
  if(toBufferSegmentNumber < segmentNumber)
    return false;

  //fprintf(stderr, "toBufferSegmentNumber=%d < segmentNumber=%d\n",toBufferSegmentNumber,segmentNumber);

  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  // Check if segment has depIds
  if(usedRepresentation->GetDependencyId ().size() > 0)
  {
    // if so find the correct map
    MBuffer::iterator it = buff.find (segmentNumber);
    if(it == buff.end ())
      return false;

    BufferRepresentationEntryMap map = it->second;

    for(std::vector<std::string>::const_iterator k = usedRepresentation->GetDependencyId ().begin ();
        k !=  usedRepresentation->GetDependencyId ().end (); k++)
    {
      //depId not found we can not add this layer
      if(map.find ((*k)) == map.end ())
      {
        //fprintf(stderr, "Could not find '%s' in map\n", (*k).c_str());
        return false;
      }
    }
  }
  else
  {
    //check if segment with layer == 0 fits in buffer
    if(isFull(usedRepresentation->GetId (),duration))
      return false;
  }

  // Add segment to buffer
  //fprintf(stderr, "Inserted something for Segment %d in Buffer\n", segmentNumber);

  MultimediaBuffer::BufferRepresentationEntry entry;
  entry.repId = usedRepresentation->GetId ();
  entry.segmentDuration = duration;
  entry.segmentNumber = segmentNumber;
  entry.depIds = usedRepresentation->GetDependencyId ();
  entry.bitrate_bit_s = usedRepresentation->GetBandwidth ();
  entry.experienced_bitrate_bit_s = (unsigned int) experiencedDownloadBitrate;

  buff[segmentNumber][usedRepresentation->GetId ()] = entry;
  toBufferSegmentNumber++;
  return true;
}

bool MultimediaBuffer::enoughSpaceInLayeredBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  if(isFull(usedRepresentation->GetId (),duration))
    return false;
 
  return true;
}


bool MultimediaBuffer::enoughSpaceInTotalBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation)
{
  //determine segment duration
  double duration = (double) usedRepresentation->GetSegmentList()->GetDuration();
  duration /= (double) usedRepresentation->GetSegmentList()->GetTimescale ();

  if(isFull(duration))
    return false;
 
  return true;
}


bool MultimediaBuffer::isFull(std::string repId, double additional_seconds)
{
  if(maxBufferedSeconds < additional_seconds+getBufferedSeconds(repId))
    return true;
  return false;
}


bool MultimediaBuffer::isFull(double additional_seconds)
{
  if(maxBufferedSeconds < additional_seconds+getBufferedSeconds())
    return true;
  return false;
}

bool MultimediaBuffer::isEmpty()
{
  if(getBufferedSeconds() <= 0.0)
    return true;
  return false;
}

/** get buffered seconds from all segments */
double MultimediaBuffer::getBufferedSeconds()
{
  double bufferSize = 0.0;

  for(MBuffer::iterator it = buff.begin (); it != buff.end (); ++it)
    bufferSize += it->second.begin()->second.segmentDuration;

  //fprintf(stderr, "BufferSize for lowestRep = %f\n", bufferSize);
  return bufferSize;
}

/** get buffered seconds only from segments belonging to the representation id repId */
double MultimediaBuffer::getBufferedSeconds(std::string repId)
{
  double bufferSize = 0.0;
  for(MBuffer::iterator it = buff.begin (); it != buff.end (); ++it)
  {
    BufferRepresentationEntryMap::iterator k = it->second.find(repId);
    if(k != it->second.end())
    {
      bufferSize += k->second.segmentDuration;
    }
  }
  //fprintf(stderr, "BufferSize for rep[%s] = %f\n", repId.c_str (),bufferSize);
  return bufferSize;
}

unsigned int MultimediaBuffer::getHighestBufferedSegmentNr(std::string repId)
{
  //std::map should be orderd based on operator<, so iterate from the back
  for(MBuffer::reverse_iterator it = buff.rbegin (); it != buff.rend (); ++it)
  {
    BufferRepresentationEntryMap::iterator k = it->second.find(repId);
    if(k != it->second.end())
    {
      return k->second.segmentNumber;
    }
  }
  return 0;
}


MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::consumeFromBuffer()
{
  BufferRepresentationEntry entryConsumed;

  if(isEmpty())
    return entryConsumed;

  MBuffer::iterator it = buff.find (toConsumeSegmentNumber);
  if(it == buff.end ())
  {
    fprintf(stderr, "Could not find SegmentNumber. This should never happen\n");
    return entryConsumed;
  }

  entryConsumed = getHighestConsumableRepresentation(toConsumeSegmentNumber);
  buff.erase (it);
  toConsumeSegmentNumber++;
  return entryConsumed;
}

MultimediaBuffer::BufferRepresentationEntry MultimediaBuffer::getHighestConsumableRepresentation(int segmentNumber)
{
  BufferRepresentationEntry consumableEntry;

  // find the correct entry map
  MBuffer::iterator it = buff.find (segmentNumber);
  if(it == buff.end ())
  {
    return consumableEntry;
  }

  BufferRepresentationEntryMap map = it->second;

  //find entry with most depIds.
  unsigned int most_depIds = 0;
  for(BufferRepresentationEntryMap::iterator k = map.begin (); k != map.end (); ++k)
  {
    if(most_depIds <= k->second.depIds.size())
    {
      consumableEntry = k->second;
      most_depIds = k->second.depIds.size();
    }
  }
  return consumableEntry;
}

double MultimediaBuffer::getBufferedPercentage()
{
  return getBufferedSeconds() / maxBufferedSeconds;
}

double MultimediaBuffer::getBufferedPercentage(std::string repId)
{
  return getBufferedSeconds (repId) / maxBufferedSeconds;
}

