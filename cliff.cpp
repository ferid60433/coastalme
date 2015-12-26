/*!
 *
 * \file cliff.cpp
 * \brief CCliff routines
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
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
#include "cliff.h"

CCliff::CCliff(CCoast* pCoastIn, int const nCoast, int const nPointOnCoast, double const dRemainIn, double const dNotchElevIn, double const dNotchOverhangIn, double const dAccumWaveEnergyIn)
{
   m_bCliffCollapse =
   m_bAllSedimentGone  = false;

   pCoast              = pCoastIn;
   m_nCoast            = nCoast;
   m_nPointOnCoast     = nPointOnCoast;
   m_nCategory         = LF_CLIFF;
   m_dRemaining        = dRemainIn;
   m_dNotchBaseElev    = dNotchElevIn;
   m_dNotchOverhang    = dNotchOverhangIn;
   m_dTotWaveEnergy    = dAccumWaveEnergyIn;
}

CCliff::~CCliff(void)
{
}

bool CCliff::bHasCollapsed(void) const
{
   return m_bCliffCollapse;
}

void CCliff::SetCliffCollapse(bool const bStatus)
{
   m_bCliffCollapse = bStatus;
}

bool CCliff::bAllSedimentGone(void) const
{
   return m_bAllSedimentGone;
}

void CCliff::SetAllSedimentGone(void)
{
   m_bAllSedimentGone = true;
}

double CCliff::dGetNotchBaseElev(void) const
{
   return m_dNotchBaseElev;
}

void CCliff::SetNotchBaseElev(double const dNewElev)
{
   m_dNotchBaseElev = dNewElev;
}

void CCliff::SetRemaining(double const dLenIn)
{
   m_dRemaining = dLenIn;
}

double CCliff::dGetRemaining(void)
{
   return m_dRemaining;
}

void CCliff::SetNotchOverhang(double const dLenIn)
{
   m_dNotchOverhang = dLenIn;
}

double CCliff::dGetNotchOverhang(void)
{
   return m_dNotchOverhang;
}

//! Returns true if the notch has reached the edge of the cell, or if the notch overhang exceeds the critical notch overhang
bool CCliff::bReadyToCollapse(double const dThresholdOverhang)
{
   if ((m_dRemaining <= 0) || (m_dNotchOverhang >= dThresholdOverhang))
      return true;
   else
      return false;
}

// Deepen the erosional notch
double CCliff::dErodeNotch(double const dLenIn)
{
   // First constrain the depth of notch deepening, it can exceed the depth of sediment remaining
   double dToRemove = tMin(m_dRemaining, dLenIn);
   m_dRemaining -= dToRemove;
   m_dNotchOverhang += dToRemove;

   // Return the (possibly reduced) depth of notch deepening
   return dToRemove;
}

void CCliff::Display(void)
{
   cout << endl;
//    for (int n = 0; n < static_cast<int>(m_VPoints.size()); n++)
//       cout << "[" << m_VPoints[n].dGetX() << "][" << m_VPoints[n].dGetY() << "], ";
//    cout << endl;
//    cout.flush();
}
