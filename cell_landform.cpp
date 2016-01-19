/*!
 *
 * \file cell_landform.cpp
 * \brief CCellLandform routines
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
#include "cell_landform.h"

CCellLandform::CCellLandform()
{
   m_nCategory          = LF_NONE;
   m_nCoast             =
   m_nPointOnCoast      = -1;

   m_dAccumWaveEnergy   = 0;
}

CCellLandform::~CCellLandform(void)
{
}

//! Set the landform category
void CCellLandform::SetCategory(int const nClassIn)
{
    m_nCategory = nClassIn;
}

//! Get the landform category
int CCellLandform::nGetCategory(void) const
{
   return  m_nCategory;
}

//! Set the coast number
void CCellLandform::SetCoast(int const nCoastIn)
{
   m_nCoast = nCoastIn;
}

//! Get the coast number
int CCellLandform::nGetCoast(void) const
{
   return m_nCoast;
}

//! Set the number of the point on the coastline
void CCellLandform::SetPointOnCoast(int const nPointOnCoastIn)
{
   m_nPointOnCoast = nPointOnCoastIn;
}

//! Set the number of the point on the coastline
int CCellLandform::nGetPointOnCoast(void) const
{
   return m_nPointOnCoast;
}

//! Set accumulated wave energy
void CCellLandform::SetAccumWaveEnergy(double const dEnergyIn)
{
   m_dAccumWaveEnergy = dEnergyIn;
}

//! Get accumulated wave energy
double CCellLandform::dGetAccumWaveEnergy(void) const
{
   return m_dAccumWaveEnergy;
}

//! Set cliff notch base elevation
void CCellLandform::SetCliffNotchBaseElev(double const dElevIn)
{
   m_uLFData.m_sCliffData.m_dNotchBaseElev = dElevIn;
}

//! Get cliff notch base elevation
double CCellLandform::dGetCliffNotchBaseElev(void) const
{
   return m_uLFData.m_sCliffData.m_dNotchBaseElev;
}

//! Set the cliff notch overhang which remains on this cell
void CCellLandform::SetCliffNotchOverhang(double const dLenIn)
{
   m_uLFData.m_sCliffData.m_dNotchOverhang = dLenIn;
}

//! Get the cliff notch overhang which remains on this cell
double CCellLandform::dGetCliffNotchOverhang(void) const
{
   return m_uLFData.m_sCliffData.m_dNotchOverhang;
}

//! Set the cliff depth remaining on this cell
void CCellLandform::SetCliffRemaining(double const dLenIn)
{
   m_uLFData.m_sCliffData.m_dRemaining = dLenIn;
}

//! Get the cliff depth remaining on this cell
double CCellLandform::dGetCliffRemaining(void) const
{
   return m_uLFData.m_sCliffData.m_dRemaining;
}


