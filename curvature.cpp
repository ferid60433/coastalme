/*!
 *
 * \file curvature.cpp
 * \brief Calculates curvature of 2D vectors
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
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

 Calculates curvature of a coastline

===============================================================================================================================*/
void CSimulation::DoCoastCurvature(int const nCoast, int const nHandedness)
{
   // TODO May need to improve this
   int
      nCoastSize = m_VCoast[nCoast].nGetCoastlineSize(),
      nPoints = 0;
   double dTotKappa = 0;

   for (int nThis = 0; nThis < nCoastSize; nThis++)
   {
      double dKappa = 0;
      if ((nThis >= m_nCoastCurvatureInterval) && (nThis < (nCoastSize - m_nCoastCurvatureInterval)))
      {
         dKappa = dCalcCurvature(nHandedness, m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nThis - m_nCoastCurvatureInterval), m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nThis), m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nThis + m_nCoastCurvatureInterval));
         dTotKappa += dKappa;
         nPoints++;
      }

      m_VCoast[nCoast].SetCurvature(nThis, dKappa);
   }

   // And fill in the end points with the average curvature values
   double dAvgKappa = dTotKappa / nPoints;
   for (int nThis = 0; nThis < m_nCoastCurvatureInterval; nThis++)
   {
      m_VCoast[nCoast].SetCurvature(nThis, dAvgKappa);
      m_VCoast[nCoast].SetCurvature(nCoastSize - nThis - 1, dAvgKappa);
   }
}

/*===============================================================================================================================

 Calculates curvature from three points on a line. Is Algorithm HK2003 from S. Hermann and R. Klette. Multigrid analysis of curvature estimators. In Proc. Image Vision Computing New Zealand, pages 108–112, Massey University, 2003.) from "A Comparative Study on 2D Curvature Estimators", Simon Hermann and Reinhard Klette

===============================================================================================================================*/
double CSimulation::dCalcCurvature(int const nHandedness, C2DPoint* const PtBefore, C2DPoint* const PtThis, C2DPoint* const PtAfter)
{
   double
      dXBefore = PtBefore->dGetX(),
      dYBefore = PtBefore->dGetY(),
      dXThis = PtThis->dGetX(),
      dYThis = PtThis->dGetY(),
      dXAfter = PtAfter->dGetX(),
      dYAfter = PtAfter->dGetY();


   double dBefore = hypot((dXThis - dXBefore), (dYThis - dYBefore));
   double dAfter = hypot((dXThis - dXAfter), (dYThis - dYAfter));

   double dThetaBefore = atan2(dXBefore - dXThis, dYBefore - dYThis);
   double dThetaAfter = atan2(dXThis - dXAfter, dYThis - dYAfter);

   double dThetaMean = (dThetaBefore + dThetaAfter) / 2.0;

   double dDeltaBefore = abs(dThetaBefore - dThetaMean);
   double dDeltaAfter = abs(dThetaAfter - dThetaMean);

   double dKappa = (dDeltaAfter / (2.0 * dAfter)) + (dDeltaBefore / (2.0 * dBefore));

   // Now decide whether the three points are convex (+ve) or concave (-ve). This algorithm is rather crude
   double dEndLineMidX = (dXBefore + dXAfter)/2;
   double dEndLineMidY = (dYBefore + dYAfter)/2;
   bool bConcave = (dXThis < dEndLineMidX) | (dYThis < dEndLineMidY);
   int nShape;
   if (nHandedness == LEFT_HANDED)
      nShape = (bConcave ? -1 : 1);
   else
      nShape = (bConcave ? 1 : -1);

   return nShape * dKappa;
}

