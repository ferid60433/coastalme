/*!
 *
 * \file fluxorientation.cpp
 * \brief Calculates flux orientation along a coast
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

 Calculates a tangent to the coastline: this assumed to be the orientation of energy/sediment flux along a coast. It is specified as an angle (in degrees) measured clockwise from north. Based on a routine by Martin Hurst

 ===============================================================================================================================*/
void CSimulation::DoFluxOrientation(int const nCoast)
{
   int
      nCoastSize = m_VCoast[nCoast].nGetCoastlineSize();

   for (int nCoastPoint = 0; nCoastPoint < nCoastSize; nCoastPoint++)
   {
      double
         dXDiff,
         dYDiff;

      if (nCoastPoint == 0)
      {
         // For the point at the start of the coastline: use the straight line from 'this' point to the next point
         C2DPoint PtThis = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint);              // In external CRS
         C2DPoint PtAfter = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint+1);           // In external CRS

         dXDiff = PtAfter.dGetX() - PtThis.dGetX();
         dYDiff = PtAfter.dGetY() - PtThis.dGetY();
      }
      else if (nCoastPoint == nCoastSize-1)
      {
         // For the point at the end of the coastline: use the straight line from the point before to 'this' point
         C2DPoint PtBefore = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint-1);          // In external CRS
         C2DPoint PtThis = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint);              // In external CRS

         dXDiff = PtThis.dGetX() - PtBefore.dGetX();
         dYDiff = PtThis.dGetY() - PtBefore.dGetY();
      }
      else
      {
         // For coastline points not at the start or end of the coast: start with a straight line which links the coastline points before and after 'this' coastline point
         C2DPoint PtBefore = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint-1);           // In external CRS
         C2DPoint PtAfter = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint+1);            // In external CRS

         dXDiff = PtAfter.dGetX() - PtBefore.dGetX();
         dYDiff = PtAfter.dGetY() - PtBefore.dGetY();
      }

      // Convert to azimuths
      if (bFPIsEqual(dYDiff, 0, TOLERANCE))
      {
         // The linking line runs either W-E or E-W
         if (dXDiff > 0)
            // It runs W to E
            m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, 90);
         else
            // It runs E to W
            m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, 270);
      }
      else if (bFPIsEqual(dXDiff, 0, TOLERANCE))
      {
         // The linking line runs N-S or S-N
         if (dYDiff > 0)
            // It runs S to N
            m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, 0);
         else
            // It runs N to S
            m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, 180);
      }
      else
      {
         // The linking line runs neither W-E nor N-S so we have to work a bit harder to find its azimuth
         double dAzimuth;
         if (dXDiff > 0)
            dAzimuth = (180 / PI) * (PI * 0.5 - atan(dYDiff / dXDiff));
         else
            dAzimuth = (180 / PI) * (PI * 1.5 - atan(dYDiff / dXDiff));

         m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, dAzimuth);
      }

//      LogStream << m_ulIter << ": nCoastPoint = " << nCoastPoint << " FluxOrientation = " << m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint) << endl;
   }
}
