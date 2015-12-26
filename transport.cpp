/*!
 *
 * \file transport.cpp
 * \brief Simulates along-shore sediment transport
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

 Simulates all along-shore sediment transport using the CERC equation

===============================================================================================================================*/
int CSimulation::nDoAllAlongshoreSedimentTransport(void)
{
   /*
   The CERC equation (Komar and Inman, 1970; USACE, 1984) is commonly implemented in one-line coastal models, in which the depth-integrated alongshore volumetric sediment transport is a function of breaking wave height Hb and angle αb:

   Qls = Kls * Hb^(5/2) * sin(2 * αb)

   where Kls is a transport coefficient. For the transport of quartz-density sand-sized material Kls = 0.4 is used (Komar, 1998) but reported values vary widely. This equation describes the immersive weight transport of sand (i.e. sand transport in suspension).

   In Coast objects: m_dVBreakingWaveHeight, m_dVBreakingWaveAngle;
   */

   int nRet = RTN_OK;

   return nRet;
}
