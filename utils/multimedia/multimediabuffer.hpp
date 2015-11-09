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

#ifndef MULTIMEDIABUFFER_HPP
#define MULTIMEDIABUFFER_HPP

#include <map>
#include <vector>
#include "libdash.h"

namespace dash
{
namespace player
{

class MultimediaBuffer
{
public:

  struct BufferRepresentationEntry
  {
    unsigned int segmentNumber;
    double segmentDuration;
    unsigned int bitrate_bit_s; // the segments representation bitrate (as advertised by mpd)
    unsigned int experienced_bitrate_bit_s; // the experiended download bitrate for this segment

    std::vector<std::string>  depIds;
    std::string repId;

    BufferRepresentationEntry()
    {
      segmentNumber = 0;
      segmentDuration = 0.0;
      bitrate_bit_s = 0.0;
      experienced_bitrate_bit_s = 0.0;
      repId = "InvalidSegment";
    }

    BufferRepresentationEntry(const BufferRepresentationEntry& other)
    {
      repId = other.repId;
      segmentNumber = other.segmentNumber;
      segmentDuration = other.segmentDuration;
      bitrate_bit_s = other.bitrate_bit_s;
      experienced_bitrate_bit_s = other.experienced_bitrate_bit_s;
      depIds = other.depIds;
    }
  };

  MultimediaBuffer(unsigned int maxBufferedSeconds);

  bool addToBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation, float experiencedDownloadBitrate);
  bool enoughSpaceInLayeredBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation);
  bool enoughSpaceInTotalBuffer(unsigned int segmentNumber, const dash::mpd::IRepresentation* usedRepresentation);
  BufferRepresentationEntry consumeFromBuffer();
  bool isFull(std::string repId, double additional_seconds = 0.0);
  bool isFull(double additional_seconds = 0.0);
  bool isEmpty();
  double getBufferedSeconds(); //returns the buffered seconds for the "lowest" representation
  double getBufferedSeconds(std::string repId); //returns the buffered seconds for representation = repId
  unsigned int getHighestBufferedSegmentNr(std::string repId);
  unsigned int nextSegmentNrToBeConsumed(){return toConsumeSegmentNumber;}

  double getBufferedPercentage();
  double getBufferedPercentage(std::string repId);

protected:
  double maxBufferedSeconds;

  unsigned int toBufferSegmentNumber;
  unsigned int toConsumeSegmentNumber;

  typedef std::map
  <std::string, /*Representation*/
  BufferRepresentationEntry
  >BufferRepresentationEntryMap;

  typedef std::map
  <int, /*SegmentNumber*/
  BufferRepresentationEntryMap
  >MBuffer;

  MBuffer buff;

  BufferRepresentationEntry getHighestConsumableRepresentation(int segmentNumber);

};
}
}

#endif // MULTIMEDIABUFFER_HPP
