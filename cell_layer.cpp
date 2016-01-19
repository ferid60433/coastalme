/*!
 *
 * \file cell_layer.cpp
 * \brief CCellLayer routines
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
 * \copyright GNU General Public License
 *
 */

/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "cell_layer.h"

CCellLayer::CCellLayer(void)
{
   m_dVolSedFraction       =
   m_dMechResistance       =
   m_dConsolidationStatus  = 0;
}

CSedimentLayer* CCellLayer::pGetUnconsolidatedSediment(void)
{
   return &m_UnconsolidatedSediment;
}

CSedimentLayer* CCellLayer::pGetConsolidatedSediment(void)
{
   return &m_ConsolidatedSediment;
}

void CCellLayer::RemoveCliff(void)
{
   double
      dNow = 0,
      dLost = 0;

   dNow  = m_UnconsolidatedSediment.dGetFine();
   dLost = m_UnconsolidatedSediment.dGetNotchFineLost();
   m_UnconsolidatedSediment.SetFine(dNow - dLost);
   m_UnconsolidatedSediment.SetNotchFineLost(0);

   dNow  = m_UnconsolidatedSediment.dGetSand();
   dLost = m_UnconsolidatedSediment.dGetNotchSandLost();
   m_UnconsolidatedSediment.SetSand(dNow - dLost);
   m_UnconsolidatedSediment.SetNotchSandLost(0);

   dNow  = m_UnconsolidatedSediment.dGetCoarse();
   dLost = m_UnconsolidatedSediment.dGetNotchCoarseLost();
   m_UnconsolidatedSediment.SetCoarse(dNow - dLost);
   m_UnconsolidatedSediment.SetNotchCoarseLost(0);

   dNow  = m_ConsolidatedSediment.dGetFine();
   dLost = m_ConsolidatedSediment.dGetNotchFineLost();
   m_ConsolidatedSediment.SetFine(dNow - dLost);
   m_ConsolidatedSediment.SetNotchFineLost(0);

   dNow  = m_ConsolidatedSediment.dGetSand();
   dLost = m_ConsolidatedSediment.dGetNotchSandLost();
   m_ConsolidatedSediment.SetSand(dNow - dLost);
   m_ConsolidatedSediment.SetNotchSandLost(0);

   dNow  = m_ConsolidatedSediment.dGetCoarse();
   dLost = m_ConsolidatedSediment.dGetNotchCoarseLost();
   m_ConsolidatedSediment.SetCoarse(dNow - dLost);
   m_ConsolidatedSediment.SetNotchCoarseLost(0);
}

double CCellLayer::dGetUnconsolidatedThickness(void) const
{
   return (m_UnconsolidatedSediment.dGetFine() + m_UnconsolidatedSediment.dGetSand() + m_UnconsolidatedSediment.dGetCoarse());
}

double CCellLayer::dGetConsolidatedThickness(void) const
{
   return (m_ConsolidatedSediment.dGetFine() + m_ConsolidatedSediment.dGetSand() + m_ConsolidatedSediment.dGetCoarse());
}

double CCellLayer::dGetTotalThickness(void) const
{
   return (m_UnconsolidatedSediment.dGetFine() + m_UnconsolidatedSediment.dGetSand() + m_UnconsolidatedSediment.dGetCoarse() + m_ConsolidatedSediment.dGetFine() + m_ConsolidatedSediment.dGetSand() + m_ConsolidatedSediment.dGetCoarse());
}

double CCellLayer::dGetNotchUnconsolidatedLost(void) const
{
   return (m_UnconsolidatedSediment.dGetNotchFineLost() + m_UnconsolidatedSediment.dGetNotchSandLost() + m_UnconsolidatedSediment.dGetNotchCoarseLost());
}

double CCellLayer::dGetNotchConsolidatedLost(void) const
{
   return (m_ConsolidatedSediment.dGetNotchFineLost() + m_ConsolidatedSediment.dGetNotchSandLost() + m_ConsolidatedSediment.dGetNotchCoarseLost());
}

double CCellLayer::dGetVolSedFraction(void) const
{
   return m_dVolSedFraction;
}

void CCellLayer::SetVolSedFraction(double const dNewVolSedFraction)
{
   m_dVolSedFraction = dNewVolSedFraction;
}

double CCellLayer::dGetMechResistance(void) const
{
   return m_dMechResistance;
}

void CCellLayer::SetMechResistance(double const dNewMechResistance)
{
   m_dMechResistance = dNewMechResistance;
}

double CCellLayer::dGetConsolidationStatus(void) const
{
   return m_dConsolidationStatus;
}

void CCellLayer::SetConsolidationStatus(double const dNewConsolidationStatus)
{
   m_dConsolidationStatus = dNewConsolidationStatus;
}

