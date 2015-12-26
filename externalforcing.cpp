/*!
 *
 * \file externalforcing.cpp
 * \brief Calculates external forcings
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include "cme.h"
#include "simulation.h"

/*===============================================================================================================================

 Calculate external forcings, e.g. tide

===============================================================================================================================*/
int CSimulation::nCalcExternalForcing(void)
{
   int nSize = m_VdTideData.size();

   if (nSize == 0)
   {
      // No tide data, still water level is fixed throughout simulation
      m_dThisIterStillWaterLevel = m_dOrigStillWaterLevel;
   }
   else
   {
      // We have tide data
      static int nTideDataCount = 0;

      // Wrap the tide data, i.e. start again with the first record if we do not have enough
      if (nTideDataCount > nSize-1)
         nTideDataCount = 0;

      m_dThisIterStillWaterLevel = m_dOrigStillWaterLevel + m_VdTideData[nTideDataCount];
      nTideDataCount++;
   }

   // Update min and max still water levels
   m_dMaxStillWaterLevel = tMax(m_dThisIterStillWaterLevel, m_dMaxStillWaterLevel);
   m_dMinStillWaterLevel = tMin(m_dThisIterStillWaterLevel, m_dMinStillWaterLevel);

   return RTN_OK;
}
