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

#ifndef DASH_ADAPTATION_LOGIC_FACTORY
#define DASH_ADAPTATION_LOGIC_FACTORY

#define ENSURE_ADAPTATION_LOGIC_REGISTERED(x) dash::player::AdaptationLogicFactory::RegisterAdaptationLogic(x::GetName(), &(x::Create));


#include <iostream>
#include <string>
#include <map>
#include <memory>

namespace dash
{
namespace player
{
class MultimediaPlayer;
class AdaptationLogic;

typedef std::shared_ptr<AdaptationLogic> (*ALogicFunctionPointer)(MultimediaPlayer*);


class AdaptationLogicFactory
{
public:
  AdaptationLogicFactory() : AvailALogics()
  {
  }


  static std::shared_ptr<AdaptationLogicFactory> GetInstance()
  {
      static std::shared_ptr<AdaptationLogicFactory> instance = std::make_shared<AdaptationLogicFactory>();
      return instance;
  }

  /** registry of adaptation logics */
  std::map<std::string /* aLogicTypeName */, ALogicFunctionPointer /* ptr */> AvailALogics;
public:
  static
  void RegisterAdaptationLogic(std::string aLogicType,
                        ALogicFunctionPointer functionPointer)
  {
    if (GetInstance()->AvailALogics.find(aLogicType) == GetInstance()->AvailALogics.end())
    {
#if defined(DEBUG) || defined(NS3_LOG_ENABLE)
      std::cerr << "AdaptationLogicFactory():\tRegistering Adaptation Logic of Type " << aLogicType << std::endl;
#endif
      GetInstance()->AvailALogics[aLogicType] = functionPointer;
    }
  }


  /** create an instance of an adaptation logic based on the provided string */
  static std::shared_ptr<AdaptationLogic> Create(std::string adaptationLogicType, MultimediaPlayer* mPlayer)
  {
    if (GetInstance()->AvailALogics.find(adaptationLogicType) != GetInstance()->AvailALogics.end())
    {
      // call the function pointer (Create Function), and pass the mPlayer pointer
      ALogicFunctionPointer fptr = (GetInstance()->AvailALogics[adaptationLogicType]);

      return fptr(mPlayer);
    }
    else
    {
      std::cerr << "AdaptationLogicFactory():\tError - Could not find Adaptation Logic of Type " << adaptationLogicType << std::endl;
      return nullptr;
    }
  }
};



}
}


#endif // DASH_ADAPTATION_LOGIC_FACTORY
