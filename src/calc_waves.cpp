/*!
 *
 * \file calc_waves.cpp
 * \brief Simulates wave propagation, including wave shadowing
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2017
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
// #include <assert.h>
#include <cmath>
#include <unistd.h>
#include <stdio.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ios;

#include <fstream>
#include <iomanip>
using std::setprecision;
using std::setiosflags;
using std::resetiosflags;
using std::setw;

#include <algorithm>
using std::sort;
using std::remove;

#include <utility>
using std::pair;
using std::make_pair;

#include <stack>
using std::stack;

#include "cme.h"
#include "coast.h"
#include "simulation.h"

#include "hermite_cubic.h"
#include "linearinterp.h"


/*===============================================================================================================================

 Simulates wave propagation along all coastline-normal profiles, based on a routine by Martin Hurst

===============================================================================================================================*/
int CSimulation::nDoAllPropagateWaves(void)
{
   // Remove the csv files where the profile data will be stored
   chdir("temp");
   remove(WAVEHEIGHTX.c_str());
   remove(WAVEHEIGHTY.c_str());
   remove(ACTIVEZONE.c_str());
   chdir("..");
   
   // Do this for each coast
   for (int nCoast = 0; nCoast < static_cast<int>(m_VCoast.size()); nCoast++)
   {
      int
         nCoastSize = m_VCoast[nCoast].nGetCoastlineSize(),
         nNumProfiles = m_VCoast[nCoast].nGetNumProfiles();

      // Calculate wave properties at every point along each valid profile, and for the cells under the profiles. Do this in the original (curvature-related) profile sequence
      for (int nProfile = 0; nProfile < nNumProfiles; nProfile++)
         CalcWavePropertiesOnProfile(nCoast, nCoastSize, nProfile);
         
      // Next, interpolate these wave properties for all remaining coastline points. Do this in along-coastline sequence
      for (int n = 0; n < nNumProfiles; n++)
         InterpolateWavePropertiesToCoastline(nCoast, n, nNumProfiles);

      // Calculate wave energy at every point on the coastline
      for (int nCoastPoint = 0; nCoastPoint < nCoastSize; nCoastPoint++)
      {
         // Equation 4 from Walkden & Hall, 2005
         double dBreakingWaveHeight = m_VCoast[nCoast].dGetBreakingWaveHeight(nCoastPoint);
         if (dBreakingWaveHeight != DBL_NODATA)
         {
            double dErosiveWaveForce = pow(dBreakingWaveHeight, WALKDEN_HALL_PARAM_1) * pow(m_dWavePeriod, WALKDEN_HALL_PARAM_2);

            // Calculate total wave energy at each coast point during this timestep
            double dWaveEnergy = dErosiveWaveForce * m_dTimeStep * 3600;
            m_VCoast[nCoast].SetWaveEnergy(nCoastPoint, dWaveEnergy);
         }
      }
    }
    
   // Interpolate wave properties for the cells between the profiles using gdal_grid
   chdir("temp");
   InterpolateWavePropertiesToCellsGdal(WAVEENERGYFLUX);
   InterpolateWavePropertiesToCellsGdal(ACTIVEZONE);
   system("./clean.sh");
   chdir("..");
   
   // Find the shadow zones and then modify waves in and adjacent to them
   int nRet = nDoAllShadowZones();
   if (nRet != RTN_OK)
      return nRet;
   
   // Finally, fill in artefactual 'holes' in active zone and wave property patterns
   CalcD50AndFillWaveCalcHoles();      
   
   return RTN_OK;
}


/*===============================================================================================================================

 Calculates the angle between the deep water wave direction and a normal to the coastline tangent. If wave direction has a component which is down-coast (i.e. in the direction with increasing coast point numbers), then the angle returned is -ve. If wave direction has a component which is up-coast (i.e. in the direction with decreasing coast point numbers), then the angle returned is +ve. If waves are in an off-shore direction, DBL_NODATA is returned

===============================================================================================================================*/
double CSimulation::dCalcWaveAngleToCoastNormal(double const dCoastAngle, int const nSeaHand)
{
   double dWaveToNormalAngle = 0;

   if (nSeaHand == LEFT_HANDED)
      // Left-handed coast
      dWaveToNormalAngle = fmod((m_dDeepWaterWaveOrientation - dCoastAngle + 360), 360) - 90;
   else
      // Right-handed coast
      dWaveToNormalAngle = fmod((m_dDeepWaterWaveOrientation - dCoastAngle + 360), 360) - 270;

   if ((dWaveToNormalAngle >= 90) || (dWaveToNormalAngle <= -90))
      dWaveToNormalAngle = DBL_NODATA;

   return dWaveToNormalAngle;
}


/*===============================================================================================================================

 Interpolates wave properties from profiles to the coastline points between two profiles. Do this by weighting the wave properties so that they change smoothly between the two profiles

===============================================================================================================================*/
void CSimulation::InterpolateWavePropertiesToCoastline(int const nCoast, int const nProfIndex, int const nNumProfiles)
{
   int nProfile = m_VCoast[nCoast].nGetProfileAtAlongCoastlinePosition(nProfIndex);
   CGeomProfile* pProfile = m_VCoast[nCoast].pGetProfile(nProfile);

   // Only do this for profiles without problems, including the start-of-coast profile (but not the end-of-coast profile)
   if (! pProfile->bOKIncStartOfCoast())
      return;

   int nThisCoastPoint = pProfile->nGetNumCoastPoint();

   // For the breaking wave stuff, to go into the in-between coastline points
   int const nThisBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nThisCoastPoint);
   double
      dThisBreakingWaveHeight = m_VCoast[nCoast].dGetBreakingWaveHeight(nThisCoastPoint),       // This could be DBL_NODATA
      dThisBreakingWaveOrientation = m_VCoast[nCoast].dGetBreakingWaveOrientation(nThisCoastPoint),
      dThisBreakingDepth = m_VCoast[nCoast].dGetDepthOfBreaking(nThisCoastPoint);

   // Get the number of the next profile along the coast. If this next profile has a problem, go to the one after that, etc
   int nNextProfile = -1;
   for (int nNextProfIndex = nProfIndex+1; nNextProfIndex < nNumProfiles; nNextProfIndex++)
   {
      nNextProfile = m_VCoast[nCoast].nGetProfileAtAlongCoastlinePosition(nNextProfIndex);

      if (m_VCoast[nCoast].pGetProfile(nNextProfile)->bOKIncStartAndEndOfCoast())
         break;
   }

   int const
      nNextCoastPoint = m_VCoast[nCoast].pGetProfile(nNextProfile)->nGetNumCoastPoint(),
      nDistBetween = nNextCoastPoint - nThisCoastPoint - 1;

   if (nDistBetween <= 0)
      // Nothing to do
      return;

   int nNextBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nNextCoastPoint);
   double
      dNextBreakingWaveHeight = m_VCoast[nCoast].dGetBreakingWaveHeight(nNextCoastPoint),          // This could be DBL_NODATA
      dNextBreakingWaveOrientation = m_VCoast[nCoast].dGetBreakingWaveOrientation(nNextCoastPoint),
      dNextBreakingDepth = m_VCoast[nCoast].dGetDepthOfBreaking(nNextCoastPoint);

   // If both this profile and the next profile are not in the active zone, then do no more
   if ((dThisBreakingWaveHeight == DBL_NODATA) && (dNextBreakingWaveHeight == DBL_NODATA))
      return;

   // OK, at least one of the two profiles is in the active zone
   if (dThisBreakingWaveHeight == DBL_NODATA)
   {
      // The next profile must be in the active zone, so use values from the next profile
      for (int n = nThisCoastPoint; n < nNextCoastPoint; n++)
      {
         // Set the breaking wave height, breaking wave angle, and depth of breaking for this coast point
         m_VCoast[nCoast].SetBreakingWaveHeight(n, dNextBreakingWaveHeight);
         m_VCoast[nCoast].SetBreakingWaveOrientation(n, dNextBreakingWaveOrientation);
         m_VCoast[nCoast].SetDepthOfBreaking(n, dNextBreakingDepth);
         m_VCoast[nCoast].SetBreakingDistance(n, nNextBreakingDist);
      }

      return;
   }

   if (dNextBreakingWaveHeight == DBL_NODATA)
   {
      // This profile must be in the active zone, so use values from this profile
      for (int n = nThisCoastPoint+1; n <= nNextCoastPoint; n++)
      {
         // Set the breaking wave height, breaking wave angle, and depth of breaking for this coast point
         m_VCoast[nCoast].SetBreakingWaveHeight(n, dThisBreakingWaveHeight);
         m_VCoast[nCoast].SetBreakingWaveOrientation(n, dThisBreakingWaveOrientation);
         m_VCoast[nCoast].SetDepthOfBreaking(n, dThisBreakingDepth);
         m_VCoast[nCoast].SetBreakingDistance(n, nThisBreakingDist);
      }

      return;
   }

   // The values for both this profile point and the next profile point are fine, so do a weighted interpolation between this profile and the next profile
   for (int n = nThisCoastPoint+1; n < nNextCoastPoint; n++)
   {
      int nDist = n - nThisCoastPoint;

      double
         dBreakingWaveHeight = 0,
         dBreakingWaveOrientation = 0,
         dBreakingDepth = 0,
         dBreakingDist = 0;

      if ((dNextBreakingDepth > 0) && (dThisBreakingDepth > 0))
      {
         double
            dThisWeight = (nDistBetween - nDist) / static_cast<double>(nDistBetween),
            dNextWeight = 1 - dThisWeight;

//          assert(dThisWeight >= 0);
//          assert(dNextWeight >= 0);

         dBreakingWaveHeight = (dThisWeight * dThisBreakingWaveHeight) + (dNextWeight * dNextBreakingWaveHeight),
         dBreakingWaveOrientation = (dThisWeight * dThisBreakingWaveOrientation) + (dNextWeight * dNextBreakingWaveOrientation),
         dBreakingDepth = (dThisWeight * dThisBreakingDepth) + (dNextWeight * dNextBreakingDepth),
         dBreakingDist = (dThisWeight * nThisBreakingDist) + (dNextWeight * nNextBreakingDist);
      }
      else if (dThisBreakingDepth > 0)
      {
         dBreakingWaveHeight = dNextBreakingWaveHeight,
         dBreakingWaveOrientation = dNextBreakingWaveOrientation,
         dBreakingDepth = dNextBreakingDepth,
         dBreakingDist = nNextBreakingDist;
      }
      else if (dNextBreakingDepth > 0)
      {
         dBreakingWaveHeight = dThisBreakingWaveHeight,
         dBreakingWaveOrientation = dThisBreakingWaveOrientation,
         dBreakingDepth = dThisBreakingDepth,
         dBreakingDist = nThisBreakingDist;
      }

      // Set the breaking wave height, breaking wave angle, and depth of breaking for this coast point
      m_VCoast[nCoast].SetBreakingWaveHeight(n, dBreakingWaveHeight);
      m_VCoast[nCoast].SetBreakingWaveOrientation(n, dBreakingWaveOrientation);
      m_VCoast[nCoast].SetDepthOfBreaking(n, dBreakingDepth);
      m_VCoast[nCoast].SetBreakingDistance(n, static_cast<int>(dRound(dBreakingDist)));
   }
}


/*===============================================================================================================================

 Calculates tangents to a coastline: the tangent is assumed to be the orientation of energy/sediment flux along a coast. The tangent is specified as an angle (in degrees) measured clockwise from north. Based on a routine by Martin Hurst

 ===============================================================================================================================*/
void CSimulation::CalcCoastTangents(int const nCoast)
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
         CGeom2DPoint PtThis = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint);              // In external CRS
         CGeom2DPoint PtAfter = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint+1);           // In external CRS

         dXDiff = PtAfter.dGetX() - PtThis.dGetX();
         dYDiff = PtAfter.dGetY() - PtThis.dGetY();
      }
      else if (nCoastPoint == nCoastSize-1)
      {
         // For the point at the end of the coastline: use the straight line from the point before to 'this' point
         CGeom2DPoint PtBefore = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint-1);          // In external CRS
         CGeom2DPoint PtThis = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint);              // In external CRS

         dXDiff = PtThis.dGetX() - PtBefore.dGetX();
         dYDiff = PtThis.dGetY() - PtBefore.dGetY();
      }
      else
      {
         // For coastline points not at the start or end of the coast: start with a straight line which links the coastline points before and after 'this' coastline point
         CGeom2DPoint PtBefore = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint-1);           // In external CRS
         CGeom2DPoint PtAfter = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPoint+1);            // In external CRS

         dXDiff = PtAfter.dGetX() - PtBefore.dGetX();
         dYDiff = PtAfter.dGetY() - PtBefore.dGetY();
      }

      // Calculate angle between line and north point, measured clockwise (the azimuth)
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
         // The linking line runs neither W-E nor N-S so we have to work a bit harder to find the angle between it and the azimuth
         double dAzimuthAngle;
         if (dXDiff > 0)
            dAzimuthAngle= (180 / PI) * (PI * 0.5 - atan(dYDiff / dXDiff));
         else
            dAzimuthAngle = (180 / PI) * (PI * 1.5 - atan(dYDiff / dXDiff));

         m_VCoast[nCoast].SetFluxOrientation(nCoastPoint, dAzimuthAngle);
      }

//      LogStream << m_ulTimestep << ": nCoastPoint = " << nCoastPoint << " FluxOrientation = " << m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint) << endl;
   }
}


/*===============================================================================================================================
 
 Function used to sort coastline curvature values for shadow zone calcs
 
===============================================================================================================================*/
bool bCurvaturePairCompareAscending(const pair<int, double> &prLeft, const pair<int, double> &prRight)
{
   // Sort in ascending order (i.e. most convex first)
   return prLeft.second < prRight.second;
}


/*===============================================================================================================================
 
 Find wave shadow zones and modifies waves in and near them
 
===============================================================================================================================*/
int CSimulation::nDoAllShadowZones(void)
{
   int const nSearchDist = tMax(m_nXGridMax, m_nYGridMax);
   
   // Do this once for each coastline
   for (unsigned int nCoast = 0; nCoast < m_VCoast.size(); nCoast++)
   {
      int nCoastSize = m_VCoast[nCoast].nGetCoastlineSize();
      
      // Create a vector of pairs: the first value of the pair is the coastline point, the second is the coastline's detailed curvature at that point
      vector<pair<int, double> > prVCurvature;

      // Omit the first and last few points on the coast next to the edge of the grid: this is necessary because some high curvature can develop at the grid edges, and we are not interested in shadow zones there
      for (int nCoastPoint = GRID_MARGIN; nCoastPoint < (nCoastSize - GRID_MARGIN); nCoastPoint++)
      {
         double dCurvature = m_VCoast[nCoast].dGetDetailedCurvature(nCoastPoint);
         prVCurvature.push_back(make_pair(nCoastPoint, dCurvature));
      }
      
      // Calculate the standard deviation of the detailed curvature vector
      double dStdCurvature = m_VCoast[nCoast].dGetDetailedCurvatureSTD();
      
      // If we have a coast with almost identical curvature everywhere (e.g. a straight line), then there will be no shadow zones, so just return      
      if (tAbs(dStdCurvature) < TOLERANCE)
         return RTN_OK;
      
      // TODO if the waves are off-shore, should we again do nothing?
      // TODO What about shadow zones created if we have more than one coast?
      
      // Sort this pair vector in ascending order, so that the most convex points are first
      sort(prVCurvature.begin(), prVCurvature.end(), bCurvaturePairCompareAscending);
      
      int nLastCapePoint = -1;
      unsigned int nThisCapePointIndex = 0;
      
      vector<int> VnCapePoint;
      vector<int> VnCapeX;
      vector<int> VnCapeY;
      vector<double> VdXDiff;
      vector<double> VdYDiff;
      
      // The first stage: look for capes and shadow zone boundary lines from these capes
      int const MAX_CAPES = 20;
      for (int nCape = 0; nCape < MAX_CAPES; nCape++)
      {         
         if (VnCapeX.size() >= MAX_NUM_SHADOW_ZONES)
            break;
         
         nThisCapePointIndex += nCape;
         int nThisCapePoint = prVCurvature[nThisCapePointIndex].first;   

         // Make sure that we do not do more than one search from the same cape i.e. from points close or adjacent
         if (nLastCapePoint >= 0)
         {
            // To locate shadow zones, we go to a cape then draw a line through this cape parallel to the deep-water wave direction, in the inland direction. We go a short distance along this line in order to get clear of the cape, then we trace this line until it either hits the edge of the grid, or it hits a coast. If it hits a coast then this delineates the boundary of a shadow zone
            bool bFound = false;
            
            while (true)
            {
               if ((nThisCapePoint < (nLastCapePoint - CAPE_POINT_MIN_SPACING)) || (nThisCapePoint = (nLastCapePoint + CAPE_POINT_MIN_SPACING)))
               {
                  // This cape point is sufficiently far from the last cape point
                  bFound = true;
                  break;
               }
               
               // Too close, so try another cape point               
               nThisCapePointIndex++;
               
               // Safety check
               if (nThisCapePointIndex > prVCurvature.size())
                  break;
                  
               nThisCapePoint = prVCurvature[nThisCapePointIndex].first;   
            }
            
            if (! bFound)
               break;
         }
 
         // Save for next time
         nLastCapePoint = nThisCapePoint;
         
         // Decide whether to use the deep-water breaking wave orientation at the cape, or the local wave orientation at the cape. Seems to work better using deep-water orientation (as in COVE)
         double dWaveOrientation;
         if (USE_DEEP_WATER_FOR_SHADOW_LINE)
            // Use the deep-water wave orientation
            dWaveOrientation = m_dDeepWaterWaveOrientation;
         else
         {
            // Get the local breaking wave orientation at this cape
            dWaveOrientation = m_VCoast[nCoast].dGetBreakingWaveOrientation(nThisCapePoint);
            if (dWaveOrientation == DBL_NODATA)
               // Waves not breaking at this cape, so use the deep sea wave orientation
               dWaveOrientation = m_dDeepWaterWaveOrientation;
         }
         
         // Get the location (grid CRS) of this cape
         CGeom2DIPoint PtiCape = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisCapePoint);    // grid CRS
         int 
            nCapeX = PtiCape.nGetX(),
            nCapeY = PtiCape.nGetY();
            
//          LogStream << m_ulTimestep << ": shadow zone stage 1, nCape = " << nCape << ", searching for shadow zone intersection from cape at [" << nCapeX << "][" << nCapeY << "] {" << dGridCentroidXToExtCRSX(nCapeX) << ", " << dGridCentroidYToExtCRSY(nCapeY) << "}" << endl;            
            
         // Now calculate the location (in grid CRS) of a point which is dSearchDist distant and is on a line parallel to the wave direction at the cape point and passing through the cape point. This distant point will almost certainly be off the grid. Note that the values of sin and cos take care of which quadrant the end point is in
         double
            dXDiff = nSearchDist * sin((PI / 180) * dWaveOrientation),
            dYDiff = nSearchDist * cos((PI / 180) * dWaveOrientation);
         
         // Get the position of the distant end point 
         int
            nEndX = nCapeX + dRound(dXDiff),
            nEndY = nCapeY - dRound(dYDiff);

//          LogStream << m_ulTimestep << ": shadow zone stage 1, nCape = " << nCape << ", cape point [" << nCapeX << "][" << nCapeY << "] {" << dGridCentroidXToExtCRSX(nCapeX) << ", " << dGridCentroidYToExtCRSY(nCapeY) << "}, wave orientation = " << dWaveOrientation << " degrees, end point [" << nEndX << "][" << nEndY << "] {" << dGridCentroidXToExtCRSX(nEndX) << ", " << dGridCentroidYToExtCRSY(nEndY) << "}" << endl;
         
         // Check if endpoint is on correct side
         int nCoastSeaHand = m_VCoast[nCoast].nGetSeaHandedness();
         CGeom2DIPoint
            PtiPrev = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisCapePoint-1),      // grid CRS
            PtiNext = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisCapePoint+1);      // grid CRS
         double
            dLinkingLineXDiff = PtiNext.nGetX() - PtiPrev.nGetX(),
            dLinkingLineYDiff = PtiNext.nGetY() - PtiPrev.nGetY(),
            dDistantXDiff = nEndX - nCapeX,
            dDistantYDiff = nEndY - nCapeY;
            
         // Next, we weed out infeasible shadow zones
         if (nCoastSeaHand == LEFT_HANDED)
         {
            if (dLinkingLineYDiff > 0)
            {
               // The linking line has a N to S component, the sea is on the left so the end point must be to the E
               if (dDistantXDiff > 0)
                  continue;
            }
            else if (dLinkingLineYDiff < 0)
            {
               // The linking line has a S to N component, the sea is on the left so the end point must be to the W
               if (dDistantXDiff < 0)
                  continue;
            }
               
            if (dLinkingLineXDiff > 0)
            {
               // The linking line has a W to E component, the sea is on the left so the end point must be to the N
               if (dDistantYDiff > 0)
                  continue;
            }
            else if (dLinkingLineXDiff < 0)
            {
               // The linking line has a E to W component, the sea is on the left so the end point must be to the S
               if (dDistantYDiff < 0)
                  continue;
            }
         }
         else     // RIGHT_HANDED coast
         {
            if (dLinkingLineYDiff > 0)
            {
               // The linking line has a N to S component, the sea is on the right so the end point must be to the W
               if (dDistantXDiff < 0)
                  continue;
            }
            else if (dLinkingLineYDiff < 0)
            {
               // The linking line has a S to N component, the sea is on the right so the end point must be to the E
               if (dDistantXDiff > 0)
                  continue;
            }
               
            if (dLinkingLineXDiff > 0)
            {
               // The linking line has a W to E component, the sea is on the right so the end point must be to the S
               if (dDistantYDiff > 0)
                  continue;
            }
            else if (dLinkingLineXDiff < 0)
            {
               // The linking line has a E to W component, the sea is on the right so the end point must be to the N
               if (dDistantYDiff < 0)
                  continue;
            }
         }
         
         // OK this is a possible shadow zone, so save the details
         VnCapePoint.push_back(nThisCapePoint);
         VnCapeX.push_back(nCapeX);
         VnCapeY.push_back(nCapeY);
         VdXDiff.push_back(dXDiff);
         VdYDiff.push_back(dYDiff);
      }
      
//       LogStream << m_ulTimestep << ": after shadow zone stage 1, we have " << VnCapeX.size() << " possible shadow zones" << endl;

      // The second stage: try to find endpoints for the possible shadow zones
      vector<int> VnCoastPoint(VnCapeX.size(), INT_NODATA);
      vector<int> VnEndX(VnCapeX.size(), INT_NODATA);
      vector<int> VnEndY(VnCapeX.size(), INT_NODATA);
      CGeomILine LiTmp;
      vector<CGeomILine> VILiBoundary(VnCapeX.size(), LiTmp);
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
      {         
         // Work along the cells which are 'under' this shadow zone line, and see if we hit a coast cell; stop at the edge of the grid. Interpolate between cells by a simple DDA line algorithm, see http://en.wikipedia.org/wiki/Digital_differential_analyzer_(graphics_algorithm) Note that Bresenham's algorithm gave occasional gaps
         double
            dXInc = VdXDiff[nZone],
            dYInc = -VdYDiff[nZone],
            dLength = tMax(tAbs(VdXDiff[nZone]), tAbs(VdYDiff[nZone]));
         
         dXInc /= dLength;
         dYInc /= dLength;
         
         double
            dX = VnCapeX[nZone] + dXInc,
            dY = VnCapeY[nZone] + dYInc;
            
         CGeomILine ILiShadowBoundary;   
         
         // Process each interpolated point
         bool bHaveLeftCoast = false;
         int 
            nSinceHitSea = 0,
            nInland = 0;
         CGeom2DIPoint PtiHitSea;
         for (int nLen = 0; nLen <= static_cast<int>(dRound(dLength)); nLen++)
         {
            int
               nX = static_cast<int>(dX),
               nY = static_cast<int>(dY);
               
            // Is the interpolated point within the raster grid?
            if (! bIsWithinGrid(nX, nY))
            {
               if (! bHaveLeftCoast)
                  break;
               
               if (CREATE_SHADOW_ZONE_IF_HITS_GRID_EDGE)
               {
                  // We are creating shadow zones if we hit the grid edge
                  if (nSinceHitSea > SHADOW_LINE_MIN_SINCE_HIT_SEA)
                  {
                     // We are clear of the cape point, but is the in-sea portion of the shadow zone line trivially short?
                     int nShadowBoundarySize = ILiShadowBoundary.nGetSize();
                     CGeom2DIPoint 
                        PtiCape(VnCapeX[nZone], VnCapeY[nZone]),
                        PtiGridEdge = ILiShadowBoundary[nShadowBoundarySize-1];
                     double dDistance = dGetDistanceBetween(&PtiHitSea, &PtiGridEdge) * m_dCellSide; 
                     
                     if (dDistance < MIN_SEA_LENGTH_OF_SHADOW_ZONE_LINE)
                     {  
                        // Too short, so forget about it
                        LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", shadow zone with grid-edge point [" << PtiGridEdge.nGetX() << "][" << PtiGridEdge.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiGridEdge.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiGridEdge.nGetY()) << "} and cape point [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "} TOO SHORT, crossed coast at [" << PtiHitSea.nGetX() << "][" << PtiHitSea.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiHitSea.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiHitSea.nGetY()) << "} so the in-sea portion has length " << dDistance << " m but the minimum in-sea length is " << MIN_SEA_LENGTH_OF_SHADOW_ZONE_LINE << " m" << endl;     
                        
                        break;
                     }
                     
                     // We've found a grid-edge shadow zone
                     VnEndX[nZone] = PtiGridEdge.nGetX();
                     VnEndY[nZone] = PtiGridEdge.nGetY();
                     VILiBoundary[nZone] = ILiShadowBoundary;
                     
                     // There is no coastline index for the end point, so we have to calculate a kind of 'virtual' coastline index for use in the next stage
                     int nCoastSize = m_VCoast[nCoast].nGetCoastlineSize();
                     CGeom2DIPoint PtCoastStart = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(0);
                     CGeom2DIPoint PtCoastEnd = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastSize-1);
                     
                     if (PtiGridEdge.nGetX() == 0)
                     {
                        // Shadow line hit W edge of the grid 
                        if (PtCoastStart.nGetX() == 0)
                        {
                           // The coastline starts from the W edge of the grid, so get the distance (in cells) between the coast start point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = -tAbs(PtiGridEdge.nGetY() - PtCoastStart.nGetY());
                        }
                        else if (PtCoastEnd.nGetX() == 0)
                        {
                           // The coastline ends at the W edge of the grid, so get the distance (in cells) between the coast end point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = nCoastSize + tAbs(PtiGridEdge.nGetY() - PtCoastEnd.nGetY());                           
                        }                        
                     }
                     else if (PtiGridEdge.nGetX() == m_nXGridMax-1)
                     {
                        // Shadow line hit E edge of the grid 
                        if (PtCoastStart.nGetX() == m_nXGridMax-1)
                        {
                           // The coastline starts from the E edge of the grid, so get the distance (in cells) between the coast start point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = -tAbs(PtiGridEdge.nGetY() - PtCoastStart.nGetY());
                        }
                        else if (PtCoastEnd.nGetX() == m_nXGridMax-1)
                        {
                           // The coastline ends at the E edge of the grid, so get the distance (in cells) between the coast end point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = nCoastSize + tAbs(PtiGridEdge.nGetY() - PtCoastEnd.nGetY());                           
                        }                        
                     }
                     else if (PtiGridEdge.nGetY() == 0)
                     {
                        // Shadow line hit N edge of the grid 
                        if (PtCoastStart.nGetY() == 0)
                        {
                           // The coastline starts from the N edge of the grid, so get the distance (in cells) between the coast start point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = -tAbs(PtiGridEdge.nGetX() - PtCoastStart.nGetX());
                        }
                        else if (PtCoastEnd.nGetY() == 0)
                        {
                           // The coastline ends at the N edge of the grid, so get the distance (in cells) between the coast end point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = nCoastSize + tAbs(PtiGridEdge.nGetX() - PtCoastEnd.nGetX());                           
                        }                                                
                     }
                     else if (PtiGridEdge.nGetY() == m_nYGridMax-1)
                     {
                        // Shadow line hit S edge of the grid 
                        if (PtCoastStart.nGetY() == m_nYGridMax-1)
                        {
                           // The coastline starts from the S edge of the grid, so get the distance (in cells) between the coast start point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = -tAbs(PtiGridEdge.nGetX() - PtCoastStart.nGetX());
                        }
                        else if (PtCoastEnd.nGetY() == m_nYGridMax-1)
                        {
                           // The coastline ends at the S edge of the grid, so get the distance (in cells) between the coast end point and the point where the shadow line hit the grid edge
                           VnCoastPoint[nZone] = nCoastSize + tAbs(PtiGridEdge.nGetX() - PtCoastEnd.nGetX());                           
                        } 
                     }
                        
//                      LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", line from cape point " << VnCapePoint[nZone] << " at [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "} hit the grid edge at [" << PtiGridEdge.nGetX() << "][" << PtiGridEdge.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiGridEdge.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiGridEdge.nGetY()) << "}, is 'virtual' coastline point " << VnCoastPoint[nZone] << endl;
                     
                     break;
                  }
               }
               else
               {
                  // We are not creating shadow zones if we hit the grid edge
//                   LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << " [" << nX << "][" << nY << "] not within grid for shadow zone" << endl;
                  break;
               }
            }
            
//             LogStream << m_ulTimestep << ": nZone = " << nZone << " nLen = " << nLen << ", stage 2 searching for shadow zone intersection at [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} from cape at [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "}" << endl;      

            ILiShadowBoundary.Append(nX, nY);
            
            if (! bHaveLeftCoast)
               nInland++;
            else
            {         
               nSinceHitSea++;
               
               if (nSinceHitSea > SHADOW_LINE_MIN_SINCE_HIT_SEA)
               {
                  // We are clear of the cape point, have we hit a coast?
                  bool bHitCoast = false;
                  int
                     nCoastX,
                     nCoastY;
                     
                  //  Note that two diagonal(ish) raster lines can cross each other without any intersection, so must also test an adjacent cell for intersection (does not matter which adjacent cell)
                  if (m_pRasterGrid->m_Cell[nX][nY].bIsCoastline())
                  {
                     bHitCoast = true;
                     nCoastX = nX;
                     nCoastY = nY;
                  }
                  else if ((bIsWithinGrid(nX, nY+1) && m_pRasterGrid->m_Cell[nX][nY+1].bIsCoastline()))
                  {
                     bHitCoast = true;
                     nCoastX = nX;
                     nCoastY = nY+1;                        
                  }
                  
                  if (bHitCoast)
                  {
                     double dLandDistance = nInland * m_dCellSide;
                     if (dLandDistance > MAX_LAND_LENGTH_OF_SHADOW_ZONE_LINE)
                     {  
                        // Too short, so forget about it
                        LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", shadow zone with coast point [" << nCoastX << "][" << nCoastY << "] {" << dGridCentroidXToExtCRSX(nCoastX) << ", " << dGridCentroidYToExtCRSY(nCoastY) << "} and cape point [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "} TOO LONG OVERLAND, crossed coast at [" << PtiHitSea.nGetX() << "][" << PtiHitSea.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiHitSea.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiHitSea.nGetY()) << "} the overland portion has length " << dLandDistance << " m but the maximum overland length is " << MAX_LAND_LENGTH_OF_SHADOW_ZONE_LINE << " m" << endl;     
                        
                        break;
                     }
                     
                     // We've hit a coastline which is a sufficient distance from the cape, but is the in-sea portion of the shadow zone line trivially short?
                     CGeom2DIPoint 
                        PtiCape(VnCapeX[nZone], VnCapeY[nZone]),
                        PtiCoast(nCoastX, nCoastY);
                     double dSeaDistance = dGetDistanceBetween(&PtiHitSea, &PtiCoast) * m_dCellSide; 
                     if (dSeaDistance < MIN_SEA_LENGTH_OF_SHADOW_ZONE_LINE)
                     {  
                        // Too short, so forget about it
                        LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", shadow zone with coast point [" << nCoastX << "][" << nCoastY << "] {" << dGridCentroidXToExtCRSX(nCoastX) << ", " << dGridCentroidYToExtCRSY(nCoastY) << "} and cape point [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "} TOO SHORT, crossed coast at [" << PtiHitSea.nGetX() << "][" << PtiHitSea.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiHitSea.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiHitSea.nGetY()) << "} so the in-sea portion has length " << dSeaDistance << " m but the minimum in-sea length is " << MIN_SEA_LENGTH_OF_SHADOW_ZONE_LINE << " m" << endl;     
                        
                        break;
                     }
                     
                     // We've found a shadow zone
                     VnEndX[nZone] = nCoastX;
                     VnEndY[nZone] = nCoastY;
                     VILiBoundary[nZone] = ILiShadowBoundary;
                     
                     // Get the coastline index of the end point
                     CGeom2DIPoint PtiEnd(nCoastX, nCoastY);
                     int nEndPoint = m_VCoast[nCoast].nGetCoastPointGivenCell(&PtiEnd);
                     VnCoastPoint[nZone] = nEndPoint;
                     
//                      LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", line from cape point " << VnCapePoint[nZone] << " at [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "} hit coast at [" << nCoastX << "][" << nCoastY << "] {" << dGridCentroidXToExtCRSX(nCoastX) << ", " << dGridCentroidYToExtCRSY(nCoastY) << "}, this is coastline point " << nEndPoint << endl;
                     
                     break;
                  }
               }
            }
            
            // Is this the first encounter with a sea cell?
            if (! bHaveLeftCoast && (m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea()))
            {
               // It is, so we have got clear of the cape
               bHaveLeftCoast = true;
               
               PtiHitSea.SetX(nX);
               PtiHitSea.SetY(nY);               

//                LogStream << m_ulTimestep << ": shadow zone stage 2, nZone = " << nZone << ", nLen = " << nLen << ", have hit sea at [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} going from cape at [" << VnCapeX[nZone] << "][" << VnCapeY[nZone] << "] {" << dGridCentroidXToExtCRSX(VnCapeX[nZone]) << ", " << dGridCentroidYToExtCRSY(VnCapeY[nZone]) << "}" << endl;   
            }
            
            // Increment for next time
            dX += dXInc;
            dY += dYInc;
         }
      }
      
      int nEndPoints = 0;
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
      {
         if (VnCoastPoint[nZone] != INT_NODATA)
            nEndPoints++;
      }
//       LogStream << m_ulTimestep << ": after shadow zone stage 2, we have " << nEndPoints << " possible shadow zones" << endl;
//       for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
//          LogStream << nZone << "\t" << VnCoastPoint[nZone] << "\t" << VnCapePoint[nZone] << endl;

      // The third stage: look for and remove 'nested' shadow zone boundaries
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
      {
         if ((VnCapePoint[nZone] != INT_NODATA) && (VnCoastPoint[nZone] != INT_NODATA))
         {
            for (unsigned int nOtherZone = 0; nOtherZone < VnCapeX.size(); nOtherZone++)
            {
               if (nOtherZone == nZone)
                  continue;

               if ((VnCapePoint[nOtherZone] == INT_NODATA) || (VnCoastPoint[nOtherZone] == INT_NODATA))
                  continue;
               
               if (VnCapePoint[nZone] > VnCoastPoint[nZone])
               {
                  // The cape point is down-coast from the coast point   
                  if ((bIsBetween(VnCoastPoint[nOtherZone], VnCoastPoint[nZone], VnCapePoint[nZone])) && (bIsBetween(VnCapePoint[nOtherZone], VnCoastPoint[nZone], VnCapePoint[nZone])))
                  {
                     // The nOtherZone shadow zone is nested within the nZone shadow zone
                     VnCoastPoint[nOtherZone] = INT_NODATA;
                     VnCapePoint[nOtherZone] = INT_NODATA;
                     
//                      LogStream << m_ulTimestep << ": shadow zone stage 3 (down-coast), zone " << nOtherZone << " is nested within zone " << nZone << " and will be ignored" << endl;
                  }
               }
               else
               {
                  // The cape point is up-coast from the coast point               
                  if ((bIsBetween(VnCoastPoint[nOtherZone], VnCapePoint[nZone], VnCoastPoint[nZone])) && (bIsBetween(VnCapePoint[nOtherZone], VnCapePoint[nZone], VnCoastPoint[nZone])))
                  {
                     // The nOtherZone shadow zone is nested within the nZone shadow zone
                     VnCoastPoint[nOtherZone] = INT_NODATA;
                     VnCapePoint[nOtherZone] = INT_NODATA;
                     
//                      LogStream << m_ulTimestep << ": shadow zone stage 3 (up-coast), zone " << nOtherZone << " is nested within zone " << nZone << " and will be ignored" << endl;                     
                  }
               }
            }
         }         
      }
      
      nEndPoints = 0;
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
      {
         if (VnCoastPoint[nZone] != INT_NODATA)
            nEndPoints++;
      }
      
      LogStream << m_ulTimestep << ": after shadow zone stage 3, we have " << nEndPoints << " definite shadow zones" << endl;
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
         LogStream << nZone << "\t" << VnCoastPoint[nZone] << "\t" << VnCapePoint[nZone] << endl;

      // The fourth stage: for non-nested shadow zones, store the boundary, flood fill the shadow zone, then change wave properties by sweeping the shadow zone and the area downdrift from the shadow zone
      for (unsigned int nZone = 0; nZone < VnCapeX.size(); nZone++)
      {
         if ((VnCapePoint[nZone] != INT_NODATA) && (VnCoastPoint[nZone] != INT_NODATA) )
         {
//             LogStream << m_ulTimestep << " shadow zone stage 4, processing nZone " << nZone << endl;
            
            // Store the shadow zone boundary, with the coast point first
            CGeom2DPoint
               PtCoast(dGridCentroidXToExtCRSX(VnEndX[nZone]), dGridCentroidYToExtCRSY(VnEndY[nZone])),
               PtCape(dGridCentroidXToExtCRSX(VnCapeX[nZone]), dGridCentroidYToExtCRSY(VnCapeY[nZone]));
            m_VCoast[nCoast].AppendShadowZoneBoundary(CGeomLine(&PtCoast, &PtCape));
            
            // Mark the cells as shadow zone boundary
            int nShadowLineLen = VILiBoundary[nZone].nGetSize();
            for (int nn = 0; nn < nShadowLineLen; nn++)
            {
               int 
                  nTmpX = VILiBoundary[nZone][nn].nGetX(),
                  nTmpY = VILiBoundary[nZone][nn].nGetY();                        
               m_pRasterGrid->m_Cell[nTmpX][nTmpY].SetShadowZoneBoundary();
               
               // If this is a sea cell, mark the shadow zone boundary cell as being in the shadow zone, but not yet processed
               if (m_pRasterGrid->m_Cell[nTmpX][nTmpY].bIsInContiguousSea())
                  m_pRasterGrid->m_Cell[nTmpX][nTmpY].SetShadowZoneCode(IN_SHADOW_ZONE_NOT_YET_DONE);
               
//                LogStream << m_ulTimestep << ": shadow zone stage 4, nZone = " << nZone << ", [" << nTmpX << "][" << nTmpY << "] {" << dGridCentroidXToExtCRSX(nTmpX) << ", " << dGridCentroidYToExtCRSY(nTmpY) << "} marked as shadow zone boundary" << endl;               
            }
            
            // Flood fill the shadow zone
            CGeom2DIPoint
               PtiCape(VnCapeX[nZone], VnCapeY[nZone]),
               PtiCoast(VnEndX[nZone], VnEndY[nZone]);
            int nRet = nFloodFillShadowZone(nCoast, VnCapePoint[nZone], &PtiCape, VnCoastPoint[nZone], &PtiCoast);
            if (nRet != RTN_OK)
            {
               // Could not do a flood fill of the shadow zone
               if (nRet == RTN_ERR_SHADOW_ZONE_FLOOD_START_POINT)
               {
                  // Could not find start point for flood fill. How serious this is depends on the length of the shadow zone line
                  if (nShadowLineLen < MAX_LEN_SHADOW_LINE_TO_IGNORE)
                  {
                     LogStream << m_ulTimestep << ": " << WARN << "abandoning shadow zone " << nZone << " but continuing simulation because this is a small shadow zone (shadow line length = " << nShadowLineLen << " cells)" << endl;
                     
                     return RTN_OK;
                  }
                  else
                  {
                     LogStream << m_ulTimestep << ": " << ERR << "could not find flood fill start point for shadow zone " << nZone << " (shadow line length = " << nShadowLineLen << " cells)" << endl;
                     return nRet;
                  }
               }
               else
                  return nRet;
            }
            
            // Sweep the shadow zone, changing wave orientation and height
            int nLengthOfSweep = 0;
            nRet = nSweepShadowZone(nCoast, VnCapePoint[nZone], &PtiCape, VnCoastPoint[nZone], &PtiCoast, nLengthOfSweep);
            if (nRet != RTN_OK)
               return nRet;
            
            // Sweep the coast downdrift of the shadow zone, changing wave height in order to conserve energy
            nRet = nSweepDownDriftFromShadowZone(nCoast, VnCapePoint[nZone], &PtiCape, VnCoastPoint[nZone], &PtiCoast, nLengthOfSweep);
            if (nRet != RTN_OK)
               return nRet;
         }
      }
   }
   
   return RTN_OK;
}


/*===============================================================================================================================
 
 Flood fills a shadow zone
 
===============================================================================================================================*/
int CSimulation::nFloodFillShadowZone(int const nCoast, int const nCapePoint, CGeom2DIPoint const* pPtiCape, int const nCoastPoint, CGeom2DIPoint const* pPtiCoast)
{
   int 
      nCoastSeaHand = m_VCoast[nCoast].nGetSeaHandedness(),
      nShadowZoneCoastToCapeSeaHand;
      
   if (nCapePoint > nCoastPoint)
   {
      // The cape point is down-coast from the coast point
      if (nCoastSeaHand == LEFT_HANDED)
         nShadowZoneCoastToCapeSeaHand = LEFT_HANDED;
      else
         nShadowZoneCoastToCapeSeaHand = RIGHT_HANDED;
   }
   else
   {
      // The cape point is up-coast from the coast point
      if (nCoastSeaHand == LEFT_HANDED)
         nShadowZoneCoastToCapeSeaHand = RIGHT_HANDED;
      else
         nShadowZoneCoastToCapeSeaHand = LEFT_HANDED;
   }   

   bool bStartPointOK = false;
   CGeom2DIPoint PtiFloodFillStart;
   for (int nOffset = FLOOD_FILL_START_OFFSET; nOffset > 0; nOffset--)
   {
      if (bStartPointOK)
         break;
      
      double dWeight = 0.05;
      while (! bStartPointOK)
      {
         if (dWeight >= 1)
            break;
         
         // Find a start point for the flood fill. Because shadow zones are generally triangular, start by choosing a low weighting so that the start point is close to the coast (i.e. not in the narrow apex of the triangle near the cape point). Then go nOffset cells inward, toward the centre of the shadow zone triangle, seems to be about right
         CGeom2DIPoint PtiWeightAvg = PtiWeightedAverage(pPtiCoast, pPtiCape, dWeight);       
         PtiFloodFillStart = PtiGetPerpendicular(&PtiWeightAvg, pPtiCape, nOffset, nShadowZoneCoastToCapeSeaHand);
            
         // Safety check
         if (! bIsWithinGrid(&PtiFloodFillStart))
         {
            LogStream << m_ulTimestep << ": " << ERR << "shadow zone flood fill start point [" << PtiFloodFillStart.nGetX() << "][" << PtiFloodFillStart.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiFloodFillStart.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiFloodFillStart.nGetY()) << "} is outside grid" << endl;
            
            return RTN_ERR_SHADOW_ZONE_FLOOD_FILL_NOGRID;
         }

         if (m_pRasterGrid->m_Cell[PtiFloodFillStart.nGetX()][PtiFloodFillStart.nGetY()].bIsInContiguousSea())
         {
            // Start point is a sea cell, all OK
            LogStream << m_ulTimestep << ": with dWeight = " << dWeight << ", and nOffset = " << nOffset << ", shadow zone flood fill start point [" << PtiFloodFillStart.nGetX() << "][" << PtiFloodFillStart.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiFloodFillStart.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiFloodFillStart.nGetY()) << "} is OK for shadow zone line from coast point [" << pPtiCoast->nGetX() << "][" << pPtiCoast->nGetY() << "] {" << dGridCentroidXToExtCRSX(pPtiCoast->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCoast->nGetY()) << "} to cape point [" << pPtiCape->nGetX() << "][" << pPtiCape->nGetY() << "] {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}" << endl;     

            bStartPointOK = true;
         }
         else
         {
            // Start point is not a sea cell
            LogStream << m_ulTimestep << ": with dWeight = " << dWeight << ", shadow zone flood fill start point [" << PtiFloodFillStart.nGetX() << "][" << PtiFloodFillStart.nGetY() << "] {" << dGridCentroidXToExtCRSX(PtiFloodFillStart.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiFloodFillStart.nGetY()) << "} is NOT a sea cell for shadow zone line from coast point [" << pPtiCoast->nGetX() << "][" << pPtiCoast->nGetY() << "] {" << dGridCentroidXToExtCRSX(pPtiCoast->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCoast->nGetY()) << "} to cape point [" << pPtiCape->nGetX() << "][" << pPtiCape->nGetY() << "] {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}" << endl;  
            
            dWeight += 0.05;   
         }
      }
   }
   
   if (! bStartPointOK)   
   {
      LogStream << m_ulTimestep << ": " << ERR << "could not find shadow zone flood fill start point" << endl;
      
      return RTN_ERR_SHADOW_ZONE_FLOOD_START_POINT;            
   }
   
   // All OK, so create an empty stack
   stack<CGeom2DIPoint> PtiStack;

   // We have a flood fill start point so push this point onto the stack
   PtiStack.push(PtiFloodFillStart);

   // Then do the flood fill: loop until there are no more cell co-ords on the stack
   while (! PtiStack.empty())
   {
      CGeom2DIPoint Pti = PtiStack.top();
      PtiStack.pop();

      int
         nX = Pti.nGetX(),
         nY = Pti.nGetY();

      while ((nX >= 0) && m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea() && (! m_pRasterGrid->m_Cell[nX][nY].bIsinShadowZone()) && (! m_pRasterGrid->m_Cell[nX][nY].bIsShadowZoneBoundary()) && (! m_pRasterGrid->m_Cell[nX][nY].bIsCoastline()))
         nX--;

      nX++;

      bool
         bSpanAbove = false,
         bSpanBelow = false;

      while ((nX < m_nXGridMax) && m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea() && (! m_pRasterGrid->m_Cell[nX][nY].bIsinShadowZone()) && (! m_pRasterGrid->m_Cell[nX][nY].bIsShadowZoneBoundary()) && (! m_pRasterGrid->m_Cell[nX][nY].bIsCoastline()))
      {
         // Mark the cell as being in the shadow zone, but not yet processed
         m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(IN_SHADOW_ZONE_NOT_YET_DONE);
         
//          LogStream << m_ulTimestep << ": [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} marked as shadow zone" << endl;

         if ((! bSpanAbove) && (nY > 0) && m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea() && (! m_pRasterGrid->m_Cell[nX][nY-1].bIsinShadowZone()) && (! m_pRasterGrid->m_Cell[nX][nY-1].bIsShadowZoneBoundary()) && (! m_pRasterGrid->m_Cell[nX][nY-1].bIsCoastline()))
         {
            PtiStack.push(CGeom2DIPoint(nX, nY-1));
            bSpanAbove = true;
         }
         else if (bSpanAbove && (nY > 0) && ((! m_pRasterGrid->m_Cell[nX][nY-1].bIsInContiguousSea()) || m_pRasterGrid->m_Cell[nX][nY-1].bIsinShadowZone() || m_pRasterGrid->m_Cell[nX][nY-1].bIsShadowZoneBoundary() || m_pRasterGrid->m_Cell[nX][nY-1].bIsCoastline()))
         {
            bSpanAbove = false;
         }

         if ((! bSpanBelow) && m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea() && (nY < m_nYGridMax+1) && (! m_pRasterGrid->m_Cell[nX][nY+1].bIsinShadowZone()) && (! m_pRasterGrid->m_Cell[nX][nY+1].bIsShadowZoneBoundary()) && (! m_pRasterGrid->m_Cell[nX][nY+1].bIsCoastline()))
         {
            PtiStack.push(CGeom2DIPoint(nX, nY+1));
            bSpanBelow = true;
         }
         else if (bSpanBelow && (nY < m_nYGridMax-1) && ((! m_pRasterGrid->m_Cell[nX][nY+1].bIsInContiguousSea()) || m_pRasterGrid->m_Cell[nX][nY+1].bIsinShadowZone() || m_pRasterGrid->m_Cell[nX][nY+1].bIsShadowZoneBoundary() || m_pRasterGrid->m_Cell[nX][nY+1].bIsCoastline()))
         {
            bSpanBelow = false;
         }

         nX++;
      }
   }
      
   return RTN_OK;
}


/*===============================================================================================================================
 
 Sweep the shadow zone, changing wave orientation and height
 
===============================================================================================================================*/
int CSimulation::nSweepShadowZone(int const nCoast, int const nCapePoint, CGeom2DIPoint const* pPtiCape, int const nCoastPoint, CGeom2DIPoint const* pPtiCoast, int& nCoastSwept)
{
   int 
      nCoastSize = m_VCoast[nCoast].nGetCoastlineSize(),
      nCoastSeaHand = m_VCoast[nCoast].nGetSeaHandedness(),
      nShadowZoneCoastToCapeSeaHand;
      
   if (nCoastSeaHand == LEFT_HANDED)
      nShadowZoneCoastToCapeSeaHand = RIGHT_HANDED;
   else
      nShadowZoneCoastToCapeSeaHand = LEFT_HANDED;
   
   // We are going to sweep the coastline starting from the coast point of the shadow zone line, going toward the cape point. Which direction is this?
   bool bSweepDownCoast = true;
   if (nCoastPoint > nCapePoint)
      bSweepDownCoast = false;   
   
   int nThisEndPoint = nCoastPoint;
   
   CGeom2DIPoint PtCoastStart = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(0);
   CGeom2DIPoint PtCoastEnd = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastSize-1);

   bool bSomeChanged = false;
   nCoastSwept = 0;
   int nLengthSwept = 0;
   while (true)
   {
      nLengthSwept++;
      
      if (bSweepDownCoast)
      {
         nThisEndPoint++;
         if (nThisEndPoint > nCoastSize-1)
            break;
      }
      else
      {
         nThisEndPoint--;
         if (nThisEndPoint < 0)
            break;
      }
      
      if (nThisEndPoint == nCapePoint)
         break;
      
      // Construct a line from the cape to the new end point
      CGeom2DIPoint PtiThisEnd;
      if (nThisEndPoint < 0)
      {
         // The shadow line hit the grid edge from which the coastline begins. We need to move along this edge till we get to the start of the coastline
         if (pPtiCoast->nGetX() == 0)
         {
            // Shadow line hit the W edge of the grid 
            PtiThisEnd.SetX(0);

            if (pPtiCoast->nGetY() > PtCoastStart.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nLengthSwept);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nLengthSwept);
         }
         else if (pPtiCoast->nGetX() == m_nXGridMax-1)
         {
            // Shadow line hit the E edge of the grid 
            PtiThisEnd.SetX(m_nXGridMax-1);

            if (pPtiCoast->nGetY() > PtCoastStart.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nLengthSwept);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nLengthSwept);
         }
         else if (pPtiCoast->nGetY() == 0)
         {
            // Shadow line hit the N edge of the grid 
            PtiThisEnd.SetY(0);

            if (pPtiCoast->nGetX() > PtCoastStart.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nLengthSwept);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nLengthSwept);
         }
         else if (pPtiCoast->nGetY() == m_nYGridMax-1)
         {
            // Shadow line hit the S edge of the grid 
            PtiThisEnd.SetY(m_nYGridMax-1);

            if (pPtiCoast->nGetX() > PtCoastStart.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nLengthSwept);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nLengthSwept);
         }
      }
      else if (nThisEndPoint >= nCoastSize)
      {
         // The shadow line hit the grid edge at which the coastline ends. We need to move along this edge till we get to the end of the coastline
         if (pPtiCoast->nGetX() == 0)
         {
            // Shadow line hit the W edge of the grid 
            PtiThisEnd.SetX(0);

            if (pPtiCoast->nGetY() > PtCoastEnd.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nLengthSwept);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nLengthSwept);
         }
         else if (pPtiCoast->nGetX() == m_nXGridMax-1)
         {
            // Shadow line hit the E edge of the grid 
            PtiThisEnd.SetX(m_nXGridMax-1);

            if (pPtiCoast->nGetY() > PtCoastEnd.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nLengthSwept);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nLengthSwept);
         }
         else if (pPtiCoast->nGetY() == 0)
         {
            // Shadow line hit the N edge of the grid 
            PtiThisEnd.SetY(0);

            if (pPtiCoast->nGetX() > PtCoastEnd.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nLengthSwept);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nLengthSwept);
         }
         else if (pPtiCoast->nGetY() == m_nYGridMax-1)
         {
            // Shadow line hit the S edge of the grid 
            PtiThisEnd.SetY(m_nYGridMax-1);

            if (pPtiCoast->nGetX() > PtCoastEnd.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nLengthSwept);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nLengthSwept);
         }
      }
      else
      {
         // The shadow line hit the coastline, not a grid edge
         PtiThisEnd = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisEndPoint);    // grid CRS
         
         if (bSomeChanged)
            nCoastSwept++;
         
         bSomeChanged = false;
      }
      
      int 
         nThisEndX = PtiThisEnd.nGetX(),
         nThisEndY = PtiThisEnd.nGetY(),
         nXDist = nThisEndX - pPtiCape->nGetX(),
         nYDist = nThisEndY - pPtiCape->nGetY();
      double 
         dX = pPtiCape->nGetX(),
         dY = pPtiCape->nGetY(),
         dDist = dGetDistanceBetween(pPtiCape, &PtiThisEnd),
         dXIncr = nXDist / dDist,
         dYIncr = nYDist / dDist;
      
      for (int n = 0; n < static_cast<int>(dRound(dDist)); n++)
      {
         dX += dXIncr;
         dY += dYIncr;
         
         int
            nX = static_cast<int>(dRound(dX)),
            nY = static_cast<int>(dRound(dY));    
         
         // Safety check
         if (! bIsWithinGrid(nX, nY))
            continue;
         
         int nZoneCode = m_pRasterGrid->m_Cell[nX][nY].nGetShadowZoneCode();
            
         if (nZoneCode != IN_SHADOW_ZONE_NOT_YET_DONE)
         {
//             if (nZoneCode == NOT_IN_SHADOW_ZONE)
//                // This cell is not in the shadow zone, so forget it
//                LogStream << m_ulTimestep << ": nThisEndPoint = " << nThisEndPoint << ", n = " << n << ", nLengthSwept = " << nLengthSwept << ", cape point {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}, end point {" << dGridCentroidXToExtCRSX(pPtiCoast->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCoast->nGetY()) << "}, this point [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} NOT IN SHADOW ZONE" << endl;            
//             
//             else if (nZoneCode == IN_SHADOW_ZONE_DONE)
//                // This cell is not in the shadow zone, so forget it
//                LogStream << m_ulTimestep << ": nThisEndPoint = " << nThisEndPoint << ", n = " << n << ", nLengthSwept = " << nLengthSwept << ", cape point {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}, end point {" << dGridCentroidXToExtCRSX(pPtiCoast->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCoast->nGetY()) << "}, this point [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} IN SHADOW ZONE, ALREADY DONE" << endl;            
            
            continue;
         }
         
         // We are in the shadow zone and will be processing this cell, so mark it
         m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(IN_SHADOW_ZONE_DONE);
         bSomeChanged = true;
         
         // Next calculate wave angle here: first calculate dOmega, the signed angle subtended between this end point and the cape point, and this end point and the end of the shadow line
         double dOmega = 180 * dAngleSubtended(pPtiCape, &PtiThisEnd, pPtiCoast) / PI;
         
         // If dOmega is 90 degrees or more in either direction, set both wave angle and wave height to zero
         if (tAbs(dOmega) >= 90)
         {
            m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(0);
            m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(0);
         }
         else
         {
            // Adapted from equation 12 in Hurst et al. 
            double dDeltaShadowWaveAngle = 1.5 * dOmega;
            
            // Get the pre-existing (i.e. shore-parallel) wave orientation
            double dWaveOrientation = m_pRasterGrid->m_Cell[nX][nY].dGetWaveOrientation();
               
            double dShadowWaveOrientation;
               
            if (nShadowZoneCoastToCapeSeaHand == LEFT_HANDED)
               dShadowWaveOrientation = dWaveOrientation + dDeltaShadowWaveAngle;
            else
               dShadowWaveOrientation = dWaveOrientation - dDeltaShadowWaveAngle;

            // Set the shadow zone wave orientation
            m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dKeepWithin360(dShadowWaveOrientation));  

            // Now calculate wave height within the shadow zone, use equation 13 from Hurst et al. 
            double dKp = 0.5 * cos(dOmega * PI / 180);
            
            // Get the pre-existing (i.e. shore-parallel) wave height
            double dWaveHeight = m_pRasterGrid->m_Cell[nX][nY].dGetWaveHeight();
            
            // Set the shadow zone wave height
            m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dKp * dWaveHeight);  
            
//             LogStream << m_ulTimestep << ": nThisEndPoint = " << nThisEndPoint << ", n = " << n << ", nLengthSwept = " << nLengthSwept << ", cape point {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}, end point {" << dGridCentroidXToExtCRSX(pPtiCoast->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCoast->nGetY()) << "}, this point [" << nX << "][" << nY << "] {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "}, angle subtended = " << dOmega << " degrees, m_dDeepWaterWaveOrientation = " << m_dDeepWaterWaveOrientation << " degrees, dDeltaShadowWaveAngle = " << dDeltaShadowWaveAngle << " degrees, dWaveOrientation = " << dWaveOrientation << " degrees, dShadowWaveOrientation = " << dShadowWaveOrientation << " degrees, dWaveHeight = " << dWaveHeight << " m, dKp = " << dKp << ", shadow zone wave height = " << dKp * dWaveHeight << " m" << endl;                
         }
      }
   }

   return RTN_OK;
}


/*===============================================================================================================================
 
 Sweeps the coast downdrift from the shadow zone, changing wave height in order to conserve energy
 
===============================================================================================================================*/
int CSimulation::nSweepDownDriftFromShadowZone(int const nCoast, int const nCapePoint, CGeom2DIPoint const* pPtiCape, int const nEndPoint, CGeom2DIPoint const* pPtiCoast, int const nSweepLength)
{
   int nCoastSize = m_VCoast[nCoast].nGetCoastlineSize();
   
   // Which direction do we need to sweep?
   bool bSweepDownCoast = false;
   if (nCapePoint < nEndPoint)
      bSweepDownCoast = true;   
   
   CGeom2DIPoint PtCoastStart = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(0);
   CGeom2DIPoint PtCoastEnd = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastSize-1);

   int nThisEndPoint = nEndPoint;   
   for (int nSweep = 0; nSweep < nSweepLength; nSweep++)
   {            
      if (bSweepDownCoast)
      {
         if (nSweep > 0)
            nThisEndPoint++;
         
//          if (nThisEndPoint > nCoastSize-1)
//             break;
      }
      else
      {
         if (nSweep > 0)
            nThisEndPoint--;
         
//          if (nThisEndPoint < 0)
//             break;
      }
      
      CGeom2DIPoint PtiThisEnd;
      if (nThisEndPoint < 0)
      {
         // The shadow line hit the grid edge from which the coastline begins. We need to move along this edge
         if (pPtiCoast->nGetX() == 0)
         {
            // Shadow line hit the W edge of the grid 
            PtiThisEnd.SetX(0);

            if (pPtiCoast->nGetY() > PtCoastStart.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nSweep);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nSweep);
         }
         else if (pPtiCoast->nGetX() == m_nXGridMax-1)
         {
            // Shadow line hit the E edge of the grid 
            PtiThisEnd.SetX(m_nXGridMax-1);

            if (pPtiCoast->nGetY() > PtCoastStart.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nSweep);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nSweep);
         }
         else if (pPtiCoast->nGetY() == 0)
         {
            // Shadow line hit the N edge of the grid 
            PtiThisEnd.SetY(0);

            if (pPtiCoast->nGetX() > PtCoastStart.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nSweep);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nSweep);
         }
         else if (pPtiCoast->nGetY() == m_nYGridMax-1)
         {
            // Shadow line hit the S edge of the grid 
            PtiThisEnd.SetY(m_nYGridMax-1);

            if (pPtiCoast->nGetX() > PtCoastStart.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nSweep);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nSweep);
         }
      }
      else if (nThisEndPoint >= nCoastSize)
      {
         LogStream << "nThisEndPoint = " << nThisEndPoint << endl;

         // The shadow line hit the grid edge at which the coastline ends
         if (pPtiCoast->nGetX() == 0)
         {
            // Shadow line hit the W edge of the grid 
            PtiThisEnd.SetX(0);

            if (pPtiCoast->nGetY() > PtCoastEnd.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nSweep);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nSweep);
         }
         else if (pPtiCoast->nGetX() == m_nXGridMax-1)
         {
            // Shadow line hit the E edge of the grid 
            PtiThisEnd.SetX(m_nXGridMax-1);

            if (pPtiCoast->nGetY() > PtCoastEnd.nGetY())
               PtiThisEnd.SetY(pPtiCoast->nGetY() + nSweep);
            else
               PtiThisEnd.SetY(pPtiCoast->nGetY() - nSweep);
         }
         else if (pPtiCoast->nGetY() == 0)
         {
            // Shadow line hit the N edge of the grid 
            PtiThisEnd.SetY(0);

            if (pPtiCoast->nGetX() > PtCoastEnd.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nSweep);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nSweep);
         }
         else if (pPtiCoast->nGetY() == m_nYGridMax-1)
         {
            // Shadow line hit the S edge of the grid 
            PtiThisEnd.SetY(m_nYGridMax-1);

            if (pPtiCoast->nGetX() > PtCoastEnd.nGetX())
               PtiThisEnd.SetX(pPtiCoast->nGetX() + nSweep);
            else
               PtiThisEnd.SetX(pPtiCoast->nGetX() - nSweep);
         }
      }
      else
      {
         // The shadow line hit the coastline, not a grid edge
         PtiThisEnd = *m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisEndPoint);    // grid CRS
      }

      int 
         nThisEndX = PtiThisEnd.nGetX(),
         nThisEndY = PtiThisEnd.nGetY(),
         nXDist = nThisEndX - pPtiCape->nGetX(),
         nYDist = nThisEndY - pPtiCape->nGetY();
      double 
         dX = pPtiCape->nGetX(),
         dY = pPtiCape->nGetY(),
         dDist = dGetDistanceBetween(pPtiCape, &PtiThisEnd),
         dXIncr = nXDist / dDist,
         dYIncr = nYDist / dDist;
      
//       LogStream << m_ulTimestep << ": sweeping downdrift, nThisEndPoint = " << nThisEndPoint  << ", nSweep = " << nSweep << ", nSweepLength = " << nSweepLength << ", cape point {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}, this end point {" << dGridCentroidXToExtCRSX(nThisEndX) << ", " << dGridCentroidYToExtCRSY(nThisEndY) << "}" << endl;
         
      for (int n = 0; n < static_cast<int>(dRound(dDist)); n++)
      {
         dX += dXIncr;
         dY += dYIncr;
         
         int
            nX = static_cast<int>(dRound(dX)),
            nY = static_cast<int>(dRound(dY));    
         
         // Safety check
         if (! bIsWithinGrid(nX, nY))
            continue;
   
         // Get the pre-existing (i.e. shore-parallel) wave height
         double dWaveHeight = m_pRasterGrid->m_Cell[nX][nY].dGetWaveHeight();
         if (dWaveHeight == DBL_NODATA)
         {
            // Is not a sea cell
            continue;
         }
         
         int nZoneCode = m_pRasterGrid->m_Cell[nX][nY].nGetShadowZoneCode();        
         
         if ((nZoneCode == IN_SHADOW_ZONE_NOT_YET_DONE) || (nZoneCode == IN_SHADOW_ZONE_DONE))
         {
            // This cell is in the shadow zone, so don't change it
            continue;
         }

         if (nZoneCode == DOWNDRIFT_OF_SHADOW_ZONE)
            // We have already processed this cell
            continue;

         // OK, we are downdrift of the shadow zone area and have not yet processed this cell, so mark it
         m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(DOWNDRIFT_OF_SHADOW_ZONE);
         
         // Equation 14 from Hurst et al. NOTE could not get this to work, so used the equation below instead
//         double dKp = 0.5 * (1.0 - sin((PI * 90.0 * nSweep) / (180.0 * nSweepLength)));                  
         double dKp = 0.5 + (0.5 * sin((PI * nSweep) / (2.0 * nSweepLength)));         
            
         // Set the modified wave height
         m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dKp * dWaveHeight);  
            
//          LogStream << m_ulTimestep << ": nThisEndPoint = " << nThisEndPoint << ", cape point {" << dGridCentroidXToExtCRSX(pPtiCape->nGetX()) << ", " << dGridCentroidYToExtCRSY(pPtiCape->nGetY()) << "}, nSweep = " << nSweep << ", nThisEndPoint = " << nThisEndPoint << ",  end point {" << dGridCentroidXToExtCRSX(PtiThisEnd.nGetX()) << ", " << dGridCentroidYToExtCRSY(PtiThisEnd.nGetY()) << "}, n = " << n << ", this point [" << nX << "][" << nY << "] = {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "}, DOWNDRIFT SWEEP original dWaveHeight = " << dWaveHeight << " m, dKp = " << dKp << ", modified wave height = " << dKp * dWaveHeight << " m" << endl;                
      }
   }

   return RTN_OK;
}


/*===============================================================================================================================

 Calculates an average d50 for each polygon, and also fills in 'holes' in active zone and wave calcs i.e. orphan cells which should have been included in the active zone but which have been omitted because of rounding problems

===============================================================================================================================*/
void CSimulation::CalcD50AndFillWaveCalcHoles(void)
{
   vector<int> VnPolygonD50Count(m_nGlobalPolygonID+1, 0);
   vector<double> VdPolygonD50(m_nGlobalPolygonID+1, 0);

   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         if (m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
         {
            // This is a sea cell, is it in the active zone?
            if (m_pRasterGrid->m_Cell[nX][nY].bIsInActiveZone())
            {
               // It is, so does it have unconsolidated sediment on it?
               double dTmpd50 = m_pRasterGrid->m_Cell[nX][nY].dGetUnconsD50();
               if (dTmpd50 != DBL_NODATA)
               {
                  // It does, so which polygon is it in?
                  int nID = m_pRasterGrid->m_Cell[nX][nY].nGetPolygonID();
                  if (nID != INT_NODATA)
                  {
                     VnPolygonD50Count[nID]++;
                     VdPolygonD50[nID] += dTmpd50;
                  }
               }
            }

            // Now fill in wave calc holes, start by looking at the cell's N-S and W-E neighbours
            int
               nXTmp,
               nYTmp,
               nActive = 0,
               nShadowOrDownDrift = 0,
               nDownDrift = 0,
               nRead = 0;
            double
               dWaveHeight = 0,
               dWaveAngle = 0;

            // North
            nXTmp = nX;
            nYTmp = nY-1;
            if ((bIsWithinGrid(nXTmp, nYTmp)) && (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInContiguousSea()))
            {
               nRead++;
               dWaveHeight += m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveHeight();
               dWaveAngle += (m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveOrientation());

               if (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInActiveZone())
                  nActive++;
               
               int nZoneCode = m_pRasterGrid->m_Cell[nXTmp][nYTmp].nGetShadowZoneCode();
               if (nZoneCode != NOT_IN_SHADOW_ZONE) 
                  nShadowOrDownDrift++;
               if (nZoneCode == DOWNDRIFT_OF_SHADOW_ZONE)
                  nDownDrift++;               
            }

            // East
            nXTmp = nX+1;
            nYTmp = nY;
            if ((bIsWithinGrid(nXTmp, nYTmp)) && (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInContiguousSea()))
            {
               nRead++;
               dWaveHeight += m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveHeight();
               dWaveAngle += (m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveOrientation());

               if (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInActiveZone())
                  nActive++;
               
               int nZoneCode = m_pRasterGrid->m_Cell[nXTmp][nYTmp].nGetShadowZoneCode();
               if (nZoneCode != NOT_IN_SHADOW_ZONE) 
                  nShadowOrDownDrift++;
               if (nZoneCode == DOWNDRIFT_OF_SHADOW_ZONE)
                  nDownDrift++;               
            }

            // South
            nXTmp = nX;
            nYTmp = nY+1;
            if ((bIsWithinGrid(nXTmp, nYTmp)) && (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInContiguousSea()))
            {
               nRead++;
               dWaveHeight += m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveHeight();
               dWaveAngle += (m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveOrientation());

               if (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInActiveZone())
                  nActive++;
               
               int nZoneCode = m_pRasterGrid->m_Cell[nXTmp][nYTmp].nGetShadowZoneCode();
               if (nZoneCode != NOT_IN_SHADOW_ZONE) 
                  nShadowOrDownDrift++;
               if (nZoneCode == DOWNDRIFT_OF_SHADOW_ZONE)
                  nDownDrift++;               
            }

            // West
            nXTmp = nX-1;
            nYTmp = nY;
            if ((bIsWithinGrid(nXTmp, nYTmp)) && (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInContiguousSea()))
            {
               nRead++;
               dWaveHeight += m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveHeight();
               dWaveAngle += (m_pRasterGrid->m_Cell[nXTmp][nYTmp].dGetWaveOrientation());

               if (m_pRasterGrid->m_Cell[nXTmp][nYTmp].bIsInActiveZone())
                  nActive++;
               
               int nZoneCode = m_pRasterGrid->m_Cell[nXTmp][nYTmp].nGetShadowZoneCode();
               if (nZoneCode != NOT_IN_SHADOW_ZONE) 
                  nShadowOrDownDrift++;
               if (nZoneCode == DOWNDRIFT_OF_SHADOW_ZONE)
                  nDownDrift++;               
            }

            if (nRead > 0)
            {
               // Calculate the average of neighbours
               dWaveHeight /= nRead;
               dWaveAngle /= nRead;
               dWaveAngle = dKeepWithin360(dWaveAngle);
               
               // If this sea cell has four active-zone neighbours, then it must also be in the active zone
               if (nActive == 4)
                  m_pRasterGrid->m_Cell[nX][nY].SetInActiveZone(true);
               
               // If this cell has the (default) deep-water wave height, but its neighbours have a different average wave height, then give it the average of its neighbours
               if ((m_pRasterGrid->m_Cell[nX][nY].dGetWaveHeight() == m_dDeepWaterWaveHeight) && (! bFPIsEqual(m_dDeepWaterWaveHeight, dWaveHeight, TOLERANCE)))
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dWaveHeight);

               // If this cell has the (default) deep-water wave angle, but its neighbours have a different average wave angle, then give it the average of its neighbours
               if ((m_pRasterGrid->m_Cell[nX][nY].dGetWaveOrientation() == m_dDeepWaterWaveOrientation) && (! bFPIsEqual(m_dDeepWaterWaveOrientation, dWaveAngle, TOLERANCE)))
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dWaveAngle);
               
               // If this sea cell is marked as IN_SHADOW_ZONE_NOT_YET_DONE then give it the average of its neighbours
               int nZoneCode = m_pRasterGrid->m_Cell[nX][nY].nGetShadowZoneCode();
               if (nZoneCode == IN_SHADOW_ZONE_NOT_YET_DONE)
               {
                  m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(IN_SHADOW_ZONE_DONE);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dWaveHeight);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dWaveAngle);
                  continue;
               }
               
               // If this sea cell has four neighbours which are marked as downdrift of the shadow zone, then it must also be downdrift of the shadow zone, give it the average of its neighbours
               if (nDownDrift == 4)
               {
                  m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(DOWNDRIFT_OF_SHADOW_ZONE);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dWaveHeight);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dWaveAngle);   
                  continue;
               }
               
               // If this sea cell has four neighbours which are not in the shadow zone, but is not itself marked as in the shadow zone or down drift from the shadow zone, then assume it is in the shadow zone and give it the average of its neighbours
               if ((nShadowOrDownDrift == 4) && (nZoneCode == NOT_IN_SHADOW_ZONE))
               {
                  m_pRasterGrid->m_Cell[nX][nY].SetShadowZoneCode(IN_SHADOW_ZONE_DONE);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dWaveHeight);
                  m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dWaveAngle);                  
               }
            }
         }
      }
   }

   // Calculate the average d50 for every polygon
   for (int nCoast = 0; nCoast < static_cast<int>(m_VCoast.size()); nCoast++)
   {
      for (int nPoly = 0; nPoly < m_VCoast[nCoast].nGetNumPolygons(); nPoly++)
      {
         CGeomCoastPolygon* pPolygon = m_VCoast[nCoast].pGetPolygon(nPoly);
         int nID = pPolygon->nGetGlobalID();

         if (VnPolygonD50Count[nID] > 0)
            VdPolygonD50[nID] /= VnPolygonD50Count[nID];

         pPolygon->SetAvgUnconsD50(VdPolygonD50[nID]);
      }
   }
}


/*===============================================================================================================================

 Calculates wave properties along a coastline-normal profile using either the COVE approach or the external CShore model
 
===============================================================================================================================*/
void CSimulation::CalcWavePropertiesOnProfile(int const nCoast, int const nCoastSize, int const nProfile)
{
   CGeomProfile* pProfile = m_VCoast[nCoast].pGetProfile(nProfile);

   // Calculate some wave properties based on the wave period following Airy wave theory
   m_dC_0 = (m_dG * m_dWavePeriod) / (2 * PI);           // Deep water (offshore) wave celerity (m/s)
   m_dL_0 = m_dC_0 * m_dWavePeriod;                      // Deep water (offshore) wave length (m)

   // Only do this for profiles without problems. Still do start- and end-of-coast profiles however
   if (! pProfile->bOKIncStartAndEndOfCoast())
      return;

   int
      nSeaHand = m_VCoast[nCoast].nGetSeaHandedness(),
      nCoastPoint = pProfile->nGetNumCoastPoint();

   // Get the flux orientation (the orientation of a line which is tangential to the coast) at adjacent coastline points (special treatment for the end points)
   double
      dFluxOrientationThis = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint),
      dFluxOrientationPrev = 0,
      dFluxOrientationNext = 0;

   if (nCoastPoint == 0)
   {
      dFluxOrientationPrev = dFluxOrientationThis;
      dFluxOrientationNext = m_VCoast[nCoast].dGetFluxOrientation(1);
   }
   else if (nCoastPoint == nCoastSize-1)
   {
      dFluxOrientationPrev = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-2);
      dFluxOrientationNext = dFluxOrientationThis;
   }
   else
   {
      dFluxOrientationPrev = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1);
      dFluxOrientationNext = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1);
   }
   
   // Calculate the angle between the deep water wave direction and a normal to the coast tangent
   double dWaveToNormalAngle = dCalcWaveAngleToCoastNormal(dFluxOrientationThis, nSeaHand);

   // Are the waves off-shore?
   if (dWaveToNormalAngle == DBL_NODATA)
   {
      // They are so, do nothing (each cell under the profile has already been initialised with deep water wave height and wave direction)
//      LogStream << m_ulTimestep << ": THIS profile = " << nProfile << " sea is to " << (m_VCoast[nCoast].nGetSeaHandedness() == RIGHT_HANDED ? "right" : "left") << " dWaveToNormalAngle = " << dWaveToNormalAngle << " which is off-shore" << endl;
      return;
   }

//   LogStream << m_ulTimestep << ": THIS profile = " << nProfile << " sea is to " << (m_VCoast[nCoast].nGetSeaHandedness() == RIGHT_HANDED ? "right" : "left") << " dWaveToNormalAngle = " << dWaveToNormalAngle << " which is " << (dWaveToNormalAngle < 0 ? "DOWN" : "UP") << "-coast" << endl;

   // Calculate the angle between the deep water wave direction and a normal to the coast tangent for the previous coast point
   double dWaveToNormalAnglePrev;
   if (nCoastPoint > 0)
      dWaveToNormalAnglePrev = dCalcWaveAngleToCoastNormal(dFluxOrientationPrev, nSeaHand);
   else
      dWaveToNormalAnglePrev = dWaveToNormalAngle;

//   LogStream << "\tPrevious profile, dWaveToNormalAnglePrev = " << dWaveToNormalAnglePrev << " which is " << (dWaveToNormalAnglePrev < 0 ? "DOWN" : "UP") << "-coast" << endl;

   // Calculate the angle between the deep water wave direction and a normal to the coast tangent for the next coast point
   double dWaveToNormalAngleNext;
   if (nCoastPoint < nCoastSize-1)
      dWaveToNormalAngleNext = dCalcWaveAngleToCoastNormal(dFluxOrientationNext, nSeaHand);
   else
      dWaveToNormalAngleNext = dWaveToNormalAngle;

//   LogStream << "\tNext profile, dWaveToNormalAngleNext = " << dWaveToNormalAngleNext << " which is " << (dWaveToNormalAngleNext < 0 ? "DOWN" : "UP") << "-coast" << endl;

   // Following Ashton and Murray (2006), if we have high-angle waves then use the flux orientation of the previous (up-coast) profile, if transitioning from diffusive to antidiffusive use flux maximizing angle (45 degrees)
   if ((dWaveToNormalAngle > 0) && (dWaveToNormalAnglePrev != DBL_NODATA) && (dWaveToNormalAnglePrev > 0))
   {
      if (dWaveToNormalAngle > 45)
      {
         if (dWaveToNormalAnglePrev < 45)
         {
            dWaveToNormalAngle = 45;
//            LogStream << "\tA1" << endl;
         }
         else
         {
            dWaveToNormalAngle = dWaveToNormalAnglePrev;
//            LogStream << "\tA2" << endl;
         }
      }
   }
   else if ((dWaveToNormalAngle < 0) && (dWaveToNormalAngleNext != DBL_NODATA) && (dWaveToNormalAngleNext < 0))
   {
      if (dWaveToNormalAngle < -45)
      {
         if (dWaveToNormalAngleNext > -45)
         {
            dWaveToNormalAngle = -45;
//            LogStream << "\tB1" << endl;
         }
         else
         {
            dWaveToNormalAngle = dWaveToNormalAngleNext;
//            LogStream << "\tB2" << endl;
         }
      }
   }
   else if ((dWaveToNormalAngle > 45) && (dWaveToNormalAnglePrev != DBL_NODATA) && (dWaveToNormalAnglePrev > 0))
   {
      // The wave direction here has an up-coast (decreasing indices) component: so for high-angle waves use the orientation from the up-coast (previous) profile
//      LogStream << "\tCCC" << endl;

      dWaveToNormalAngle = dFluxOrientationPrev;
   }
   else if ((dWaveToNormalAngle < -45)  && (dWaveToNormalAngleNext != DBL_NODATA) && (dWaveToNormalAngleNext < 0))
   {
      // The wave direction here has a down-coast (increasing indices) component: so for high-angle waves use the orientation from the down-coast (next) profile
//      LogStream << "\tDDD" << endl;

      dWaveToNormalAngle = dFluxOrientationNext;
   }

   // Initialize the wave properties at breaking for this profile
   bool bBreaking = false;
   int
      nProfileSize = pProfile->nGetNumCellsInProfile(),
      nBreakingDist = 0;
   double
      dBreakingWaveHeight = 0,
      dBreakingWaveOrientation = 0,
      dBreakingDepth = 0,
      dWaveHeight = DBL_NODATA,
      dWaveOrientation = DBL_NODATA;
   vector<bool>
      VbWaveIsBreaking(nProfileSize,0);
   vector<double> 
      VdWaveHeight(nProfileSize,0),
      VdWaveDirection(nProfileSize,0);
        
   if (m_nWavePropagationModel == MODEL_CSHORE)
   {
      // We are using CShore to propagate the waves, so create an input file for this profile
      double 
         dCShoreTimeStep = 3600,     // In seconds, not important because we are not using CShore to erode the profile, just to get the hydrodynamics
         dSurgeLevel = 0.0,          // Not used, but in the future we might include surge in the calculations
         dWaveFriction = 0.015;      // TODO Let the user define the wave friction factor
        
      // Set up vectors for the coastline-normal profile elevations. The length of this vector line is given by the number of cells 'under' the profile. Thus each point on the vector relates to a single cell in the grid. This assumes that all points on the profile vector are equally spaced (not quite true, depends on the orientation of the line segments which comprise the profile)   
      vector<double>
         VdProfileZ,                  // Initial (pre-erosion) elevation of both consolidated and unconsolidated sediment for cells 'under' the profile, in CShore units
         VdProfileDistXY;             // Along-profile distance measured from the seaward limit, in CShore units
     
      // The elevation of each of these profile points is the elevation of the centroid of the cell that is 'under' the point. However we cannot always be confident that this is the 'true' elevation of the point on the vector since (unless the profile runs planview N-S or W-E) the vector does not always run exactly through the centroid of the cell
      GetThisProfilePointsElevationVectors(nCoast, nProfile, nProfileSize, VdProfileDistXY, VdProfileZ);
      
      // For some reason, VdProfileDistXY is not populated, then exit this routine
      if (VdProfileDistXY.empty())
      {
         cout << "VdProfileDistXY is empty" << endl;
         return;
      }
     
      // Move to the CShore folder
      chdir("cshore");
   
      // Create the CShore input file  
      CreateCShoreinfile(dCShoreTimeStep, m_dWavePeriod, m_dDeepWaterWaveHeight, dWaveToNormalAngle, dSurgeLevel, dWaveFriction, VdProfileDistXY, VdProfileZ);
     
      // Run CShore for this profile
      system("./cshore.out > scr.txt"); 
     
      // Fetch the CShore results
      vector<double> 
         VdFreeSurfaceStd(VdProfileDistXY.size(), 0),          // This is converted to Hrms by Hrms = sqr(8)*FreeSurfaceStd
         VdSinWaveAngleRadians(VdProfileDistXY.size(), 0),     // This is converted to deg by asin(VdSinWaveAngleRadians)*(180/pi)
         VdFractionBreakingWaves(VdProfileDistXY.size(), 0);   // Is 0 if no wave breaking, and 1 if all waves breaking
       
      string 
         strOSETUP = "OSETUP",
         strOYVELO = "OYVELO",
         strOPARAM = "OPARAM";
      
      dLookUpCShoreouputs(strOSETUP, 4, 4, VdProfileDistXY, VdFreeSurfaceStd);
      dLookUpCShoreouputs(strOYVELO, 4, 2, VdProfileDistXY, VdSinWaveAngleRadians);
      dLookUpCShoreouputs(strOPARAM, 4, 3, VdProfileDistXY, VdFractionBreakingWaves);
      
      // Clean up the CShore outputs
      system("./clean.sh");
      chdir("..");
     
      // Convert CShore outputs to wave height and wave direction and update wave profile attributes
      for (int nProfilePoint = (nProfileSize-1); nProfilePoint >= 0; nProfilePoint--)
      {
         int
            nX = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetX(),
            nY = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetY();
         
         VdWaveHeight.at(nProfilePoint) = sqrt(8) * VdFreeSurfaceStd.at(nProfilePoint);
         
         double dAlpha = asin(VdSinWaveAngleRadians.at(nProfilePoint)) * (180/PI);
         if (nSeaHand == LEFT_HANDED)
            VdWaveDirection.at(nProfilePoint) = dKeepWithin360(dAlpha + 90 + dFluxOrientationThis);
         else
            VdWaveDirection.at(nProfilePoint) = dKeepWithin360(dAlpha + 270 + dFluxOrientationThis);
      
         if ((VdFractionBreakingWaves.at(nProfilePoint) >= 0.99) & (! bBreaking)) 
         {
            bBreaking = true;
            dBreakingWaveHeight = dWaveHeight;
            dBreakingWaveOrientation = dWaveOrientation;
            dBreakingDepth = m_pRasterGrid->m_Cell[nX][nY].dGetSeaDepth();  // Water depth for the cell 'under' this point in the profile
            nBreakingDist = nProfilePoint;
         }
           
         VbWaveIsBreaking.at(nProfilePoint) = bBreaking;
      }
      
    }
    
    else if (m_nWavePropagationModel == MODEL_COVE)
    {
      // We are using COVE's linear wave theory to propoagate the waves
      double dDepthLookupMax = m_dWaveDepthRatioForWaveCalcs * m_dDeepWaterWaveHeight;
         
      // Go landwards along the profile, calculating wave height and wave angle for every inundated point on the profile (don't do point zero, this is on the coastline) 
      // until the waves start to break  after breaking wave height is assumed to decrease linearly to zero at the shoreline and wave angle is equalt to wave angle at breaking
      for (int nProfilePoint = (nProfileSize-1); nProfilePoint > 0; nProfilePoint--)
      {
         int
            nX = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetX(),
            nY = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetY();

         // Safety check
         if (! m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
            continue;

         double dSeaDepth = m_pRasterGrid->m_Cell[nX][nY].dGetSeaDepth();  // Water depth for the cell 'under' this point in the profile
         
         if (dSeaDepth > dDepthLookupMax) 
         {
            // Sea depth is too large relative to wave height to feel the bottom, so do nothing aince each cell under the profile has already been initialised with deep water wave height and wave direction
            dWaveHeight       = m_dDeepWaterWaveHeight;
            dWaveOrientation  = m_dDeepWaterWaveOrientation;
         }
         else  
         {
            if (! bBreaking)
            {
               // Start calculating wave properties using linear wave theory
               double dL = m_dL_0 * sqrt(tanh((2 * PI * dSeaDepth) / m_dL_0));               // Wavelength (m) in intermediate-shallow waters
               double dC = m_dC_0 * tanh((2 * PI * dSeaDepth) / dL);                         // Wave speed (m/s) set by dSeaDepth, dL and m_dC_0
               double dk = 2 * PI / dL;                                                      // Wave number (1/m)
               double dn = ((2 * dSeaDepth * dk) / (sinh(2 * dSeaDepth * dk)) + 1) / 2;      // Shoaling factor
               double dKs = sqrt(m_dC_0 / (dn * dC * 2));                                    // Shoaling coefficient
               double dAlpha = (180 / PI) * asin((dC / m_dC_0) * sin((PI / 180) * dWaveToNormalAngle));  // Calculate angle between wave direction and the normal to the coast tangent
               double dKr = sqrt(cos((PI / 180) * dWaveToNormalAngle) / cos((PI / 180) * dAlpha));       // Refraction coefficient
               dWaveHeight = m_dDeepWaterWaveHeight * dKs * dKr;                             // Calculate wave height, based on the previous (more seaward) wave height
               if (nSeaHand == LEFT_HANDED)
                  dWaveOrientation = dKeepWithin360(dAlpha + 90 + dFluxOrientationThis);
               else
                  dWaveOrientation = dKeepWithin360(dAlpha + 270 + dFluxOrientationThis);
         
               // Test to see if the wave breaks at this depth
               if (dWaveHeight > (dSeaDepth * WAVEHEIGHT_OVER_WATERDEPTH_AT_BREAKING))
               {
                  // It does
                  bBreaking = true;
                  dBreakingWaveHeight = dWaveHeight;
                  dBreakingWaveOrientation = dWaveOrientation;
                  dBreakingDepth = dSeaDepth;
                  nBreakingDist = nProfilePoint;
               }
            }
            else   
            {
               // Wave has already broken
               dWaveOrientation = dBreakingWaveOrientation;   // wave orientation remains equal to wave orientation at breaking
               dWaveHeight      = dBreakingWaveHeight * (nProfilePoint/nBreakingDist); // wave height decreases linearly to zero at shoreline 
            }
         }
     
         // Save current wave attributes
         VdWaveDirection[nProfilePoint] = dWaveOrientation;
         VdWaveHeight[nProfilePoint] = dWaveHeight;
         VbWaveIsBreaking[nProfilePoint] = bBreaking;
      }
   }
    
   // Open the csv files for each wave attribute that will be interpolated
   chdir("temp");

   ofstream OutStreamWAVEHEIGHTX;
   OutStreamWAVEHEIGHTX.open(WAVEHEIGHTX.c_str(), ios::out | ios::app);
   if (! OutStreamWAVEHEIGHTX)
   {
      // Error, cannot open file
      cerr << ERR << "cannot open " << WAVEHEIGHTX << " for output" << endl;
      return;
   }
   
   ofstream OutStreamWAVEHEIGHTY;
   OutStreamWAVEHEIGHTY.open(WAVEHEIGHTY.c_str(), ios::out | ios::app);
   if (! OutStreamWAVEHEIGHTY)
   {
      // Error, cannot open file
      cerr << ERR << "cannot open " << WAVEHEIGHTY << " for output" << endl;
      return;
   }
    
   ofstream OutStreamACTIVEZONE;
   OutStreamACTIVEZONE.open(ACTIVEZONE.c_str(), ios::out | ios::app);
   if (! OutStreamACTIVEZONE)
   {
      // Error, cannot open file
      cerr << ERR << "cannot open " << ACTIVEZONE << " for output" << endl;
      return;
   }
   
   // Go landwards along the profile, fetching the calculated wave height and wave angle for every inundated point on this profile
   for (int nProfilePoint = (nProfileSize-1); nProfilePoint > 0; nProfilePoint--)
   {
      int
         nX = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetX(),
         nY = pProfile->pPtiGetCellInProfile(nProfilePoint)->nGetY();

      // Safety check
      if (! m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
         continue;
      
      // Fetch the wave attributes calculated for this profile: wave height, wave angle, and whether is in the active zone
      dWaveHeight = VdWaveHeight[nProfilePoint];
      double dWaveOrientation = VdWaveDirection[nProfilePoint];
      bBreaking = VbWaveIsBreaking[nProfilePoint];
      
      // Update RasterGrid wave properties
      m_pRasterGrid->m_Cell[nX][nY].SetInActiveZone(bBreaking);
      m_pRasterGrid->m_Cell[nX][nY].SetWaveHeight(dWaveHeight);
      m_pRasterGrid->m_Cell[nX][nY].SetWaveOrientation(dWaveOrientation);
      
      // Write the profile wave attributes to CSV files for interpolation between profiles
      OutStreamWAVEHEIGHTX << nX << "," << nY << "," << dWaveHeight * sin(dWaveOrientation * PI/180) << endl;
      OutStreamWAVEHEIGHTY << nX << "," << nY << "," << dWaveHeight * cos(dWaveOrientation * PI/180) << endl;
      OutStreamACTIVEZONE  << nX << "," << nY << "," << bBreaking << endl;
      
      if (m_ulTimestep == 4)
         LogStream << m_ulTimestep << ": cell [" << nX << "][" << nY << "] = {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} written to " << ACTIVEZONE << " as breaking" << endl;

      
   }

   OutStreamWAVEHEIGHTY.close();
   OutStreamWAVEHEIGHTX.close();
   OutStreamACTIVEZONE.close();
   
   // Back to main folder
   chdir(".."); 
   
   // Update wave attributes along the coastline object
   if (nBreakingDist > 0)
   {
      // This coast point is in the active zone, so set breaking wave height, breaking wave angle, and depth of breaking for the coast point
      m_VCoast[nCoast].SetBreakingWaveHeight(nCoastPoint, dBreakingWaveHeight);
      m_VCoast[nCoast].SetBreakingWaveOrientation(nCoastPoint, dBreakingWaveOrientation);
      m_VCoast[nCoast].SetDepthOfBreaking(nCoastPoint, dBreakingDepth);
      m_VCoast[nCoast].SetBreakingDistance(nCoastPoint, nBreakingDist);
//       LogStream << "nCoastPoint = " << nCoastPoint << " in active zone" << endl;
   }
   else
   {
      // This coast point is not in the active zone
      m_VCoast[nCoast].SetBreakingWaveHeight(nCoastPoint, DBL_NODATA);
      m_VCoast[nCoast].SetBreakingWaveOrientation(nCoastPoint, DBL_NODATA);
      m_VCoast[nCoast].SetDepthOfBreaking(nCoastPoint, DBL_NODATA);
      m_VCoast[nCoast].SetBreakingDistance(nCoastPoint, INT_NODATA);
//       LogStream << "nCoastPoint = " << nCoastPoint << " NOT in active zone" << endl;
   }

   return;
}


/*===============================================================================================================================

 Create the CShore input file

===============================================================================================================================*/
int CSimulation::CreateCShoreinfile(double dtimestep, double dWavePeriod, double dHrms, double dWaveAngle , double dSurgeLevel, double dWaveFriction, vector<double>& vdXdist, vector<double>& vdBottomElevation)
{
   // Initialize inifile from infileTemplate
   system("cp infileTemplate infile");       // The infileTemplate must be in the working directory
   std::string strFName = "infile";   
   
   // We have all the inputs in the CShore format, so we can create the input file
   std::ofstream file;
   file.open(strFName.c_str(), ios::out | ios::app);
   if (file.fail())
      // Error, cannot open file
      cerr << ERR << "cannot open " << strFName << " for output" << endl;

   file << setiosflags(ios::fixed) << setprecision(2); file << setw(11) << 0.0;
   file << setiosflags(ios::fixed) << setprecision(4); file << setw(11) << dWavePeriod << setw(11) << dHrms << setw(11) <<  dWaveAngle << std::endl;
   file << setiosflags(ios::fixed) << setprecision(2); file << setw(11) << dtimestep;
   file << setiosflags(ios::fixed) << setprecision(4); file << setw(11) << dWavePeriod << setw(11) << dHrms << setw(11) <<  dWaveAngle << std::endl;
   
   file << setiosflags(ios::fixed) << setprecision(2); file << setw(11) << 0.0;
   file << setiosflags(ios::fixed) << setprecision(4); file << setw(11) << dSurgeLevel << std::endl;
   file << setiosflags(ios::fixed) << setprecision(2); file << setw(11) << dtimestep;
   file << setiosflags(ios::fixed) << setprecision(4); file << setw(11) << dSurgeLevel << std::endl;
   
   file << setw(8) << vdXdist.size() << "                        -> NBINP" << std::endl;
   file << setiosflags(ios::fixed) << setprecision(4);
   for (unsigned i = 0; i < vdXdist.size(); i++)
   {
      file << setw(11) << vdXdist.at(i) << setw(11) << 	vdBottomElevation.at(i)<< setw(11) << dWaveFriction << std::endl;
   }
   return RTN_OK;
}


/*===============================================================================================================================

 Get profile horizontal distance and bottom elevation vectors in CShore units

===============================================================================================================================*/
int CSimulation::GetThisProfilePointsElevationVectors(int const nCoast, int const nProfile, int const &nProfSize, vector<double>& VdDistXY, vector<double>& vdVZ)
{
   CGeomProfile* pProfile = m_VCoast[nCoast].pGetProfile(nProfile);
   double 
      dXDist,
      dYDist,
      dProfileDistXY = 0;
   int 
      nX = 0,
      nY = 0,
      nX1 = 0,
      nY1 = 0;

   for (int i = nProfSize-1; i >= 0; i--)
   {
      nX = pProfile->pPtiVGetCellsInProfile()->at(i).nGetX(),
      nY = pProfile->pPtiVGetCellsInProfile()->at(i).nGetY();

      // Calculate the horizontal distance relative to the most seaward point
      if (i == nProfSize-1)
         dProfileDistXY = 0;
      else
      {
         dXDist = dGridCentroidXToExtCRSX(nX1) - dGridCentroidXToExtCRSX(nX),
         dYDist = dGridCentroidYToExtCRSY(nY1) - dGridCentroidYToExtCRSY(nY),
         dProfileDistXY = dProfileDistXY + hypot(dXDist, dYDist);
      }
      
      // Update the cell indexes. Initial cell is now the previous one
      nX1 = nX;
      nY1 = nY; 

      // Get the number of the highest layer with non-zero thickness
      int const nTopLayer = m_pRasterGrid->m_Cell[nX][nY].nGetTopNonZeroLayerAboveBasement();

      // Safety check
      if (nTopLayer == INT_NODATA)
         return RTN_ERR_NO_TOP_LAYER;

      if (nTopLayer == NO_NONZERO_THICKNESS_LAYERS)
         // TODO We are down to basement, decide what to do
         return RTN_OK;

      // Get the elevation for both consolidated and unconsolidated sediment on this cell
      double VdProfileZ = m_pRasterGrid->m_Cell[nX][nY].dGetSedimentTopElev() - m_dThisTimestepSWL;
      vdVZ.push_back(VdProfileZ); 
      //vdVZ.at(i) = VdProfileZ;

      // And store the X-Y plane distance from the start of the profile
      VdDistXY.push_back(dProfileDistXY);
   }
   
   return RTN_OK;
}


/*==============================================================================================================================

 The CShore lookup: it returns a vector with the the interpolated values on column # of the CShore output file. The interpolation may be simple linear or a more advanced hermite cubic method

==============================================================================================================================*/
int CSimulation::dLookUpCShoreouputs(string const strCShoreFilename, unsigned const nExpectedColumns, unsigned const nCShorecolumn, vector<double> const VdDistXY, vector<double>& dVMyInterpolatedValues)
{
   // TODO Make this a user input
   // Select the interpolation method to be used: 0 for Simple linear or 1 for hermite cubic
   int InterpMethodOption = CSHORE_INTERPOLATION_LINEAR;
//    int InterpMethodOption = CSHORE_INTERPOLATION_HERMITE_CUBIC;
   
   // Read in the first column (contains XY distance relative to seaward limit) and CShore column from the CShore output file
   std::ifstream InStream; 
   InStream.open(strCShoreFilename.c_str(), ios::in);
    
   // Did it open OK?
   if (! InStream.is_open())
   {
      // Error: cannot open CShore file for input
      cerr << ERR << "cannot open " << strCShoreFilename << " for input" << endl;
      return false;
   }
   
   // Opened OK
   unsigned int nExpectedRows; 
   double 
      dValue,
      dDummy;
   vector<double> VdValue;
   vector< vector < double > > VdData;

   // Read in the header line
   InStream >> dDummy;           // Always 1
   InStream >> nExpectedRows;    // Used for sanity check
   InStream >> dDummy;           // Always 3600
   
   // Read the remaining lines
   while (InStream >> dValue)
   {
      VdValue.push_back(dValue);
      
      if (VdValue.size() == nExpectedColumns)
      {
         VdData.push_back(VdValue);
         VdValue.clear();
      }
   }
   
   // Set up the vectors to hold the input data
   vector<double> 
      vdXYDistCShore,
      VdValuesCShore;
   
   for (unsigned int i = 0; i < VdData.size(); i++)
   {
      vdXYDistCShore.push_back(VdData[i][0]);
      VdValuesCShore.push_back(VdData[i][nCShorecolumn-1]);
   }

   // Check that we have read all the expected nExpectedRows
   unsigned nReadRows = vdXYDistCShore.size();
   if (nReadRows != nExpectedRows)
   {
      // Error: we expect nExpected CShore output rows but actually read nReadRows
      cerr << ERR << "expected CShore output rows " << nExpectedRows << "but read " << nReadRows << " for file " << strCShoreFilename << endl;
      return false;
   }

   // Change the origin of the across-shore distance from the CShore convention to the one used here (i.e. with the origin at the shoreline)
   vector<double>  vdXYDistCME(nReadRows,0);
   for (unsigned i=0; i < nReadRows; i++)
      vdXYDistCME.at(i) = vdXYDistCShore.at(nReadRows-1)-vdXYDistCShore.at(i);
     
   // Reverse the cshore XYdistance and value vectors (i.e. first point is at the shoreline and must be in strictly ascending order)
   std::reverse(vdXYDistCME.begin(),vdXYDistCME.end());
   std::reverse(VdValuesCShore.begin(),VdValuesCShore.end());
   
   // Now we have everything ready to do the interpolation 
   if (InterpMethodOption == CSHORE_INTERPOLATION_HERMITE_CUBIC)
   {
      // Using the hermite cubic approach: calculate the first derivative of CShore values (needed for the hermite interpolant)
      vector<double> VdValuesCShoreDeriv(nReadRows, 0);
      double dX;  // Horizontal distance increment between two adjacent points (not always the same distance because it depend on profile-cells centroid location)
      for (unsigned int i = 1; i < nReadRows-1; i++)
      {
         dX = vdXYDistCME.at(i+1) - vdXYDistCME.at(i-1);   // is always positive 
         VdValuesCShoreDeriv.at(i) = (VdValuesCShore.at(i+1) - VdValuesCShore.at(i-1) ) / (2 * dX);
      }
      
      VdValuesCShoreDeriv.at(0) = VdValuesCShoreDeriv.at(1);
      VdValuesCShoreDeriv.at(nReadRows-1) = VdValuesCShoreDeriv.at(nReadRows-2);
   
      // Interpolate the CShore values at the  
      int nSize = VdDistXY.size();
      vector<double>
         VdDistXYCopy(VdDistXY.begin(), VdDistXY.end()),
	    //dVInter(nSize,0.),
         VdDeriv(nSize, 0.),        // First derivative at the sample points: calculated by the spline function but not subsequently used
         VdDeriv2(nSize, 0.),       // Second derivative at the sample points, ditto
         VdDeriv3(nSize, 0.);       // Third derivative at the sample points, ditto

      // Calculate the value of erosion potential (is a -ve value) for each of the sample values of DepthOverDB, and store it for use in the look-up function
      hermite_cubic_spline_value(nReadRows, &(vdXYDistCME.at(0)), &(VdValuesCShore.at(0)), &(VdValuesCShoreDeriv.at(0)), nSize, &(VdDistXYCopy[0]), &(dVMyInterpolatedValues[0]), &(VdDeriv[0]), &(VdDeriv2[0]), &(VdDeriv3[0]));
   }
   else
   {
      // Using the simple linear approach
      vector<double> VdDistXYCopy(VdDistXY.begin(), VdDistXY.end());
      dVMyInterpolatedValues = interp1(vdXYDistCME, VdValuesCShore, VdDistXYCopy);
   }
   
   return RTN_OK;
}


/*===============================================================================================================================

 Interpolates wave properties from a profile to the cells between the profile using gdal_grid. Uses the WaveAttribute CSV files generated by CalcWavePropertiesOnProfile to produce a tiff --> xyz and update the raster grid attributes. All operations are done in the temp directory

===============================================================================================================================*/
int CSimulation::InterpolateWavePropertiesToCellsGdal(string strFileName)
{
   // Create the Virtual Raster file
   /* <OGRVRTDataSource>
        <OGRVRTLayer name= "temp">
                <SrcDataSource>temp.csv</SrcDataSource>
                <GeometryField encoding="PointFromColumns" x="field_1" y="field_2" z="field_3"/>
        </OGRVRTLayer>
   </OGRVRTDataSource>*/
    
   if (strFileName.compare(WAVEENERGYFLUX) == 0)
   {
      string strCopycsvX = "cp " + WAVEHEIGHTX + " temp.csv";
      system(strCopycsvX.c_str());
      
      system("gdal_grid -l temp temp.vrt tempX.tiff -a linear::radius=-1 -ot Float32 --config GDAL_NUM_THREADS ALL_CPUS -q");
      system("gdal_translate -of XYZ -tr 1 1 tempX.tiff tempX.xyz -q");
      
      string strCopyxyz1 = "cp tempX.xyz " + WAVEHEIGHTX.substr(0, WAVEHEIGHTX.length()-4) + ".xyz" ;
      system(strCopyxyz1.c_str());
      
      string strCopycsvY = "cp " + WAVEHEIGHTY + " temp.csv";
      system(strCopycsvY.c_str());
      
      system("gdal_grid -l temp temp.vrt tempY.tiff -a linear::radius=-1 -ot Float32 --config GDAL_NUM_THREADS ALL_CPUS -q");
      system("gdal_translate -of XYZ -tr 1 1 tempY.tiff tempY.xyz -q");
      
      string strCopyxyz2 = "cp tempY.xyz " + WAVEHEIGHTY.substr(0, WAVEHEIGHTY.length()-4) + ".xyz" ;
      system(strCopyxyz2.c_str());
   }
   
   else if (strFileName.compare(ACTIVEZONE) == 0)
   {
      string strCopycsv = "cp " + strFileName + " temp.csv";
      system(strCopycsv.c_str());
      
      system("gdal_grid -l temp temp.vrt temp.tiff -a nearest::radius=-1 -ot Int16 --config GDAL_NUM_THREADS ALL_CPUS -q");
      system("gdal_translate -of XYZ -tr 1 1 temp.tiff temp.xyz -q");   // Translate generated tiff file into xyz and save it in the cshore/out folder 
      
      string strCopyxyz3 = "cp temp.xyz " + ACTIVEZONE.substr(0, ACTIVEZONE.length()-4) + ".xyz";
      system(strCopyxyz3.c_str()); // Save the xyz file for the record 
   }

   // Update the raster grid with the interpolated values  
   bReadWaveAttributesXYZ(strFileName);
 
   return RTN_OK;
}


/*==============================================================================================================================

 Reads the xyz files with the interpolated wave properties and updates the raster grid values

==============================================================================================================================*/
bool CSimulation::bReadWaveAttributesXYZ(string strFileName)
{
   if (strFileName.compare(ACTIVEZONE) == 0)
   {
      // Create an ifstream object
      string strMyCshoreXYZ = "temp.xyz";
      std::ifstream InStream;

      // Try to open the xyz file for input
      InStream.open(strMyCshoreXYZ.c_str(), ios::in);

      // Did it open OK?
      if (! InStream.is_open())
      {
         // Error: cannot open xyz file for input
         cerr << m_ulTimestep << ": " << ERR << "cannot open " << strMyCshoreXYZ << " for input" << endl;
         return false;
      }

      // Opened OK
      char szRec[BUF_SIZE] = "";
      string strRec;

      // Now read the data from the file, line by line
      while (InStream.getline(szRec, BUF_SIZE, '\n'))
      {
         strRec = szRec;

         // Trim off leading and trailing whitespace
         strRec = strTrimLeft(&strRec);
         strRec = strTrimRight(&strRec);

         // If it is a blank line, ignore it
         if (strRec.empty())
            continue;

         // Split the string
         vector<string> strTmp = strSplit(&strRec, SPACE);
         
         // Get the grid coordinates
         std::string::size_type sz;
         int 
            nX = std::stoi(strTmp[0], &sz),
            nY = std::stoi(strTmp[1], &sz);

         // Is this a sea cell?
         if (m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
         {
            // It is, so convert the string to a bool and set the cell's active zone value
            bool bActiveZone = std::stoi(strTmp[2], &sz);
            m_pRasterGrid->m_Cell[nX][nY].SetInActiveZone(bActiveZone);
            
            if (m_ulTimestep == 4)
               LogStream << m_ulTimestep << ": cell [" << nX << "][" << nY << "] = {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "} set as active zone" << endl;
         }
      }
      
      // Close file
      InStream.close();
   }
   else if (strFileName.compare(WAVEENERGYFLUX) == 0)
   {
      // Create two ifstream objects
      string 
         strMyCshoreXYZx = "tempX.xyz",
         strMyCshoreXYZy = "tempY.xyz";
      std::ifstream InStreamX;
      std::ifstream InStreamY;

      // Try to open the two files for input
      InStreamX.open(strMyCshoreXYZx.c_str(), ios::in);
      InStreamY.open(strMyCshoreXYZy.c_str(), ios::in);
      
      // Did the first file open OK?
      if (! InStreamX.is_open())
      {
         // Error: cannot open xyz file for input
         cerr << m_ulTimestep << ": " << ERR << "cannot open " << strMyCshoreXYZx << " for input" << endl;
         return false;
      }
      
      // Did the secoond file open OK
      if (! InStreamY.is_open())
      {
         // Error: cannot xyz  file for input
         cerr << m_ulTimestep << ": " << ERR << "cannot open " << strMyCshoreXYZy << " for input" << endl;
         return false;
      }

      // Both files opened OK
      char 
         szRecX[BUF_SIZE] = "",
         szRecY[BUF_SIZE] = "";
      
      // Now read the data from both files, line by line (assumes both files are the same size)
      while (InStreamX.getline(szRecX, BUF_SIZE, '\n') && InStreamY.getline(szRecY, BUF_SIZE, '\n'))
      {
         string 
            strRecX = szRecX,
            strRecY = szRecY;

         // Trim off leading and trailing whitespace
         strRecX = strTrimLeft(&strRecX);
         strRecX = strTrimRight(&strRecX);
         strRecY = strTrimLeft(&strRecY);
         strRecY = strTrimRight(&strRecY);

         // If either is a blank line then ignore both lines
         if (strRecX.empty() || strRecY.empty())
            continue;

         // Split both strings
         vector<string> 
            strTmpX = strSplit(&strRecX, SPACE),
            strTmpY = strSplit(&strRecY, SPACE);
         
         // Get the grid coordinates from both files
         std::string::size_type sz;
         int 
            nXx = std::stoi(strTmpX[0], &sz),
            nYx = std::stoi(strTmpX[1], &sz),
            nXy = std::stoi(strTmpY[0], &sz),
            nYy = std::stoi(strTmpY[1], &sz);

         // Check that both points are at the same grid location
         if ((nXx != nXy) || (nYx != nYy))
            return false;
         
         // They are the same location so calculate the wave height and direction 
         double 
            dWaveHeightX = strtod(strTmpX[2].c_str(), NULL),
            dWaveHeightY = strtod(strTmpY[2].c_str(), NULL),
            dWaveHeight = sqrt((dWaveHeightX * dWaveHeightX) + (dWaveHeightY * dWaveHeightY)),
            dWaveDir = atan2(dWaveHeightX, dWaveHeightY) * (180/PI);
         
         // Update the cell's wave attributes
         m_pRasterGrid->m_Cell[nXx][nYx].SetWaveHeight(dWaveHeight);       
         m_pRasterGrid->m_Cell[nXx][nYx].SetWaveOrientation(dKeepWithin360(dWaveDir));
      }
      
      // Close file
      InStreamX.close();
      InStreamY.close();
   }
   
   return true;
}