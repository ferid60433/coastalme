/*!
 *
 * \file erodecoast.cpp
 * \brief Erodes the coast, extrapolating from erosion on the coastline-normal profiles
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
#include "2dpoint.h"
#include "simulation.h"
#include "hermitecubic.h"

/*===============================================================================================================================

 Erodes all coastlines by first calculating erosion on coastline-normal profiles, then extrapolating this to cells between the profiles

===============================================================================================================================*/
int CSimulation::nErodeAllCoasts(void)
{
   static bool bForward = true;
   int nRet = 0;

   // On each coast, calc potential erosion on every coastline-normal profile
   for (int nCoast = 0; nCoast < static_cast<int>(m_VCoast.size()); nCoast++)
   {
      int nNumProfiles = m_VCoast[nCoast].nGetNumProfiles();

      // Do each profile, if the user has set m_bErodeCoastAlternateDir to true, then go along the coast in alternate directions each timestep
      int nProfile = 0;
      for ((bForward ? nProfile = 0 : nProfile = (nNumProfiles-1)); (bForward ? nProfile < nNumProfiles :  nProfile >= 0); (bForward ? nProfile++ : nProfile--))
      {
         // Calculate potential erosion along the length of the profile
         nRet = nCalcPotentialErosionOnProfile(nCoast, nProfile, m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile(), m_VCoast[nCoast].pGetProfile(nProfile)->VPtiGetCellsInProfile(), m_VCoast[nCoast].pGetProfile(nProfile)->VPtGetCellsInProfileExtCRS());
         if (nRet != RTN_OK)
            return nRet;
      }
   }

   // On each coast, calculate potential erosion between the coastline-normal profiles
   for (int nCoast = 0; nCoast < static_cast<int>(m_VCoast.size()); nCoast++)
   {
      int nNumProfiles = m_VCoast[nCoast].nGetNumProfiles();

      // Do each profile, if the user has set m_bErodeCoastAlternateDir to true, then go along the coast in alternate directions each timestep
      int nProfile = 0;
      for ((bForward ? nProfile = 0 : nProfile = (nNumProfiles-1)); (bForward ? nProfile < nNumProfiles :  nProfile >= 0); (bForward ? nProfile++ : nProfile--))
      {
         // Calculate potential erosion for sea cells between this profile and the next profile (or up to the edge of the grid) on these cells
         for (int nDirection = DIRECTION_FORWARD; nDirection <= DIRECTION_BACKWARD; nDirection++)
         {
            int nRet = nCalcPotentialErosionOneSideOfProfile(nCoast, nProfile, m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile(), nDirection, m_VCoast[nCoast].pGetProfile(nProfile)->VPtiGetCellsInProfile());
            if (nRet != RTN_OK)
               return nRet;
         }
      }
   }

   // If desired, swap direction for next timestep
   if (m_bErodeCoastAlternateDir)
      bForward = !bForward;

   // Now calculate actual erosion on all sea cells (both on profiles, and between profiles)
   nRet = nCalcActualErosionOnAllSeaCells();
   if (nRet != RTN_OK)
      return nRet;

   return RTN_OK;
}

/*==============================================================================================================================

 Calculates potential (i.e. unconstrained) erosional lowering on a single coastline-normal profile for given wave conditions and water level, due to platform mass wasting. The initial shore platform profile is eroded due to wave action. This routine uses a behavioural rule to modify the original surface elevation profile geometry, in which:

 erosion rate/slope = f(d/Db)

 based on Walkden & Hall (2005). Note that actual erosion depth as calculated by nErodeSeaCells() may be less than this 'potential' erosion depth, since it is constrained by sediment availablity.

 Originally coded in Matlab, by Andres Payo

==============================================================================================================================*/
int CSimulation::nCalcPotentialErosionOnProfile(int const nCoast, int const nProfile, int const nProfSize, vector<C2DIPoint>* pVPtiGridProfile, vector<C2DPoint>* pVPtExtCRSProfile)
{
   // Set up a vector for the coastline-normal profile elevations. The length of this vector line is given by the number of cells 'under' the profile. Thus each point on the vector relates to a single cell in the grid. We assume that all points on the profile vector are equally spaced
   // The elevation of each of these points on the vector is just the elevation of the centroid of the cell that is 'under' the vector. However we cannot always be confident that this is the 'true' elevation of the point on the vector since (unless the profile runs planview N-S or W-E) the vector does not always run exactly through the centroid of the cell. So we use a weighting which indicates confidence in this elevation

   // Get the number of the coast point at which this profile starts
   int nCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNumCoastPoint();

   // Calculate the length of the profile in external CRS units (was set by user, but do again here for safety)
   double dProfileLenXY = hypot((pVPtExtCRSProfile->at(nProfSize - 1).dGetX() - pVPtExtCRSProfile->at(0).dGetX()), (pVPtExtCRSProfile->at(nProfSize - 1).dGetY() - pVPtExtCRSProfile->at(0).dGetY()));

   // Next calculate the distance between profile points, again in external CRS units. Assume that the sample points are equally spaced along the profile
   double dSpacingXY = dProfileLenXY / (nProfSize - 1);

   vector<double> dVProfileZ(nProfSize, 0);                 // Initial (pre-erosion) sediment-top elevs of cells 'under' the profile
   vector<double> dVProfileDistXY(nProfSize,  0);           // Along-profile distance measured from the coast, in external CRS units
   vector<double> dVProfileSlope(nProfSize, 0);

   for (int i = 0; i < nProfSize; i++)
   {
      int nX = pVPtiGridProfile->at(i).nGetX();
      int nY = pVPtiGridProfile->at(i).nGetY();

      // Get the Z (elevation) value from each cell 'under' the profile
      dVProfileZ[i] = m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev();

      // And store the X-Y distance from the start of the profile
      dVProfileDistXY[i] = i * dSpacingXY;
   }

   for (int i = 1; i < nProfSize - 1; i++)
   {
      // Get the Z differences (already in external CRS units)
      double dZDiff = dVProfileZ[i - 1] - dVProfileZ[i + 1];

      // Calculate dZ/dXY, the Z slope (i.e. rise over run) in the XY direction
      dVProfileSlope[i] = dZDiff / (dSpacingXY + dSpacingXY);
   }

   // Sort out the two end points
   dVProfileSlope[0] = dVProfileSlope[1];
   dVProfileSlope[nProfSize - 1] = dVProfileSlope[nProfSize - 2];

   // Smooth the vector of slopes using a running mean
   dVProfileSlope = dVSmoothProfileSlope(&dVProfileSlope);

   // Get the breaking depth for this profile
   double dDepthofBreaking = m_VCoast[nCoast].dGetDepthOfBreaking(nCoastPoint);
//   LogStream << m_ulIter << ": ON PROFILE nProfile = " << nProfile << " dDepthofBreaking = " << dDepthofBreaking << endl;

   // Initialize the coastline-normal vector with depth / m_dWaveBreakingDepth. Note that water depth is always zero at point zero (where the profile crosses the coastline), so the first inundated point on the profile is point 1
   vector<double>
      dVProfileDepthOverDB(nProfSize, 0),          // Depth / wave breaking depth at the coastline-normal sample points
      dVProfileErosionPotential(nProfSize, 0);     // Erosion potential at the coastline-normal sample points

   for (int i = 0; i < nProfSize; i++)
   {
      // Now calculate depth over DB
      dVProfileDepthOverDB[i] = m_dThisIterStillWaterLevel - dVProfileZ[i];
      dVProfileDepthOverDB[i] /= dDepthofBreaking;

      // Constrain dDepthOverDB[i] to be between 0 (can get small -ve due to rounding errors) and m_dDepthOverDBMax
      dVProfileDepthOverDB[i] = tMax(dVProfileDepthOverDB[i], 0.0);
      dVProfileDepthOverDB[i] = tMin(dVProfileDepthOverDB[i], m_dDepthOverDBMax);

      // And then use the look-up table to find the value of erosion potential at this point on the profile
      dVProfileErosionPotential[i] = dLookUpErosionPotential(dVProfileDepthOverDB[i]);

      // If erosion potential (a -ve value) is tiny, set it to zero
      if (dVProfileErosionPotential[i] > EROSIONPOTENTIALTOLERANCE)
         dVProfileErosionPotential[i] = 0;
   }

   // Set up more arrays for points along the profile
   vector<double>
      dVRecessionXY(nProfSize, 0),
      dVBeachProtection(nProfSize, m_dBeachProtectionFactor),
      dVChangeElevZ(nProfSize, 0);                 // Change in elevation

   // Calculate the lowering at each point along the coastline-normal profile
   for (int i = 0; i < nProfSize; i++)
   {
      int nX = pVPtiGridProfile->at(i).nGetX();
      int nY = pVPtiGridProfile->at(i).nGetY();

      // First store the local slope, this is just for output display purposes
      m_pRasterGrid->Cell[nX][nY].SetLocalSlope(dVProfileSlope[i]);

      if (i > 0)
      {
         // For the actual erosion calcs: start at point 1 since this is the first inundated point
         //
         // dRecession = dForce * (dBeachProtection / dR) * dErosionPotential * dSlope * dTime
         // where:
         //     dVRecession [m] is the landward migration distance defined in the profile relative (XY) CRS
         //     dForce is given by Equation 4 in Walkden & Hall, 2005
         //     dVBeachProtection [1] is beach protection factor [1, 0] = [no protection, fully protected]
         //     dVR  [m^(9/4)s^(2/3)] is the material strength and some hydrodynamic constant
         //     dVProfileErosionPotential [?] is the erosion potential at each point along the profile
         //     dVSlope [1] is the along-profile slope
         //     m_dTimeStep * 3600 [s] is the time interval in seconds
         //
         // dRecession is horizontal recession (along the XY direction):
         //
         // dVRecessionXY[i] = (dForce * dVBeachProtection[i] * dVErosionPotentialFunc[i] * dVSlope[i] * m_dTimeStep * 3600) / dVR[i]
         //
         // XY recession must be -ve or zero. If it is +ve then it represents accretion not erosion, which must be described by a different set of equations. So we also need to constrain XY recession to be <= 0
         dVRecessionXY[i] = tMin(m_VCoast[nCoast].dGetWaveEnergy(nCoastPoint) * dVBeachProtection[i] * dVProfileErosionPotential[i] * dVProfileSlope[i] / m_dR, 0.0);

         // OK, we have the horizontal recession (along the XY direction) so now calculate the vertical lowering dZ = dXY * tan(a), where tan(a) is the slope in the XY direction. Expressed as a single equation:
         //
         // dDeltaZ = tMin(dForce * (dBeachProtection / dR) * dErosionPotential * dSlope * dSlope * dTime, 0.0);
         double dDeltaZ = dVRecessionXY[i] * dVProfileSlope[i];

         if (dDeltaZ < 0)
         {
            // dDeltaZ is zero or -ve: if dDeltaZ is zero then do nothing, if -ve then remove dDeltaZ from this cell
            dVChangeElevZ[i] = dDeltaZ;

            // Check if already eroded this iteration (shouldn't be) TODO is where profiles cross, fix this
            if (m_pRasterGrid->Cell[nX][nY].bPotentialErosion())
            {
//                LogStream << setiosflags(ios::fixed);
//                LogStream << m_ulIter << ": [" << nX << "][" << nY << "] in profile " << nProfile << " already had potential erosion this iteration" << endl;
            }

            else
            {
               // Set the potential (unconstrained) erosion (a +ve value) for this cell, and increment the total potential erosion
               m_pRasterGrid->Cell[nX][nY].SetPotentialErosion(-dDeltaZ);

               // Set the distance-from-profile value to one, this is used in the weighting function when a cell has more than one erosion value calculated
               m_pRasterGrid->Cell[nX][nY].SetWeight(1);

               // Update this-iteration totals
               m_ulThisIterNPotentialErosionCells++;
               m_dThisIterPotentialErosion -= dDeltaZ;

               // Increment the check values
               m_ulTotPotErosionOnProfiles++;
               m_dTotPotErosionOnProfiles -= dDeltaZ;
            }
         }
      }
   }

   // If desired, save this coastline-normal profile for checking purposes
   if (m_bOutputProfileData)
   {
      int nRet = nSaveProfile(nProfile, nCoast, nProfSize, &dVProfileDistXY, &dVProfileZ, &dVProfileDepthOverDB, &dVProfileErosionPotential, &dVProfileSlope, &dVRecessionXY, &dVChangeElevZ, pVPtiGridProfile);
      if (nRet != RTN_OK)
         return nRet;
   }

   return RTN_OK;
}

/*==============================================================================================================================

 Calculates potential erosion on cells on one side of a given coastline-normal profile, up to the next profile or up to the edge of the grid

==============================================================================================================================*/
int CSimulation::nCalcPotentialErosionOneSideOfProfile(int const nCoast, int const nProfile, int const nProfSize, int const nDirection, vector<C2DIPoint>* const pVPtiGridProfile)
{
   // Start at the coast end of this coastline-normal profile, then move one cell forward along the coast, then construct a parallel profile from this new coastline start cell. Calculate erosion along this parallel profile in the same way as above. Move another cell forward along the coastline, do the same. Keep going until hit another profile, or the grid edge
   // TODO for first profile, must do this for cells between edge and this profile
   int
      nCoastProfileStart = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNumCoastPoint(),
      nProfileStartX = pVPtiGridProfile->at(0).nGetX(),
      nProfileStartY = pVPtiGridProfile->at(0).nGetY(),
      nCoastMax = m_VCoast[nCoast].nGetCoastlineSize(),
      nDistFromProfile = 0,
      nParCoastXLast = nProfileStartX,
      nParCoastYLast = nProfileStartY;

   while (true)
   {
      // Increment the distance from the coastline-normal profile
      nDistFromProfile++;

      // Start the parallel profile nDistFromProfile cells along the coastline from the coastline-normal profile, direction depending on nDirection
      int nThisPointOnCoast = nCoastProfileStart;
      if (nDirection == DIRECTION_FORWARD)
         nThisPointOnCoast += nDistFromProfile;
      else
         nThisPointOnCoast -= nDistFromProfile;

      // Have we reached the beginning of the coast?
      if ((nDirection == DIRECTION_BACKWARD) && (nThisPointOnCoast < 0))
         // TODO how to deal with the remaining cells?
         break;

      // Have we reached the end of the coast?
      if ((nDirection == DIRECTION_FORWARD) && (nThisPointOnCoast >= nCoastMax))
         // TODO how to deal with the remaining cells?
         break;

      // All is OK, so get the grid co-ordinates of this point, which is the coastline start point for the parallel profile
      int
         nParCoastX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisPointOnCoast)->nGetX(),
         nParCoastY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisPointOnCoast)->nGetY();

      if ((nParCoastX == nParCoastXLast) && (nParCoastY == nParCoastYLast))
      {
         // Should not happen, but could do due to rounding errors
         LogStream << WARN << m_ulIter << ": coastline rounding problem on coast " << nCoast << " profile " << nProfile << " at [" << nParCoastX << "][" << nParCoastY << "]" << endl;

         // So move on to the next point along the coastline in this direction
         continue;
      }

      // Is this coastline start point the start point of an adjacent coastline-normal vector?
      if (m_pRasterGrid->Cell[nParCoastX][nParCoastY].bGetProfile())
         break;

      // OK construct a parallel profile
      vector<C2DIPoint> PtiVGridParProfile;        // Integer coords (grid CRS) of cells under the parallel profile
      vector<C2DPoint> PtVExtCRSParProfile;        // Co-ords (external CRS) of cells under the parallel profile
      ConstructParallelProfile(nDistFromProfile, nProfileStartX, nProfileStartY, nParCoastX, nParCoastY, nProfSize, pVPtiGridProfile, &PtiVGridParProfile, &PtVExtCRSParProfile);

      int nParProfSize = PtiVGridParProfile.size();
      // We have a parallel profile which starts at the coast, but is it long enough to be useful? May have been cut short because it extended outside the grid, or we hit an adjacent profile
      if (nParProfSize < 3)
      {
         // We cannot use this parallel profile, it is too short to calculate along-profile slope, so abandon it and move on to the next parallel profile in this direction
         nParCoastXLast = nParCoastX;
         nParCoastYLast = nParCoastY;
//         LogStream << m_ulIter << ": parallel profile ABANDONED, starts at [" << nParCoastX << "][" << nParCoastY << "] coastline point " << nThisPointOnCoast << ", length = " << nParProfSize << endl;
         continue;
      }

      // This parallel profile is OK, so calculate potential erosion along it. First calculate the length of the parallel profile in external CRS units (was set by user, but do again here)
      double dParProfileLenXY = hypot((PtVExtCRSParProfile[nParProfSize - 1].dGetX() - PtVExtCRSParProfile[0].dGetX()), (PtVExtCRSParProfile[nParProfSize - 1].dGetY() - PtVExtCRSParProfile[0].dGetY()));

      // Next calculate the distance between profile points, again in external CRS units. Assume that the sample points are equally spaced along the parallel profile
      double dParSpacingXY = dParProfileLenXY / (nParProfSize - 1);

      vector<double>
         dVParProfileZ(nProfSize, 0),                    // Initial (pre-erosion) sed-top elevs of cells 'under' the parallel profile
         dVParProfileDistXY(nParProfSize, 0);            // Along-profile distance measured from the coast, in external CRS units

      for (int i = 0; i < nParProfSize; i++)
      {
         int nXPar = PtiVGridParProfile[i].nGetX();
         int nYPar = PtiVGridParProfile[i].nGetY();

         // If it is within the grid, get the Z (elevation) value from each cell 'under' the parallel profile
         if (bIsWithinGrid(nXPar, nYPar))
            dVParProfileZ[i] = m_pRasterGrid->Cell[nXPar][nYPar].dGetSedimentTopElev();

         // And store the X-Y distance from the start of the profile
         dVParProfileDistXY[i] = i * dParSpacingXY;
      }

      vector<double>
         dVParProfileSlope(nProfSize, 0);

      for (int i = 1; i < nParProfSize-1; i++)
      {
         // Get the Z differences (already in external CRS units)
         double dZDiff = dVParProfileZ[i-1] - dVParProfileZ[i+1];

         // Calculate dZ/dXY, the Z slope (i.e. rise over run) in the XY direction
         dVParProfileSlope[i] = dZDiff / (dParSpacingXY + dParSpacingXY);
      }

      // Sort out the two end points
      dVParProfileSlope[0] = dVParProfileSlope[1];
      dVParProfileSlope[nParProfSize-1] = dVParProfileSlope[nParProfSize-2];

      // Smooth the vector of slopes using a running mean
      dVParProfileSlope = dVSmoothProfileSlope(&dVParProfileSlope);

      // Initialize the parallel profile vector with depth / m_dWaveBreakingDepth. Note that water depth is always zero at point zero (where the profile crosses the coastline), so the first inundated point on the profile is point 1
      vector<double>
         dVParProfileDepthOverDB(nParProfSize, 0),          // Depth / wave breaking depth at the parallel profile sample points
         dVParProfileErosionPotential(nParProfSize, 0);     // Erosion potential at the parallel profile sample points

      double dDepthofBreaking = m_VCoast[nCoast].dGetDepthOfBreaking(nThisPointOnCoast);
//      LogStream << m_ulIter << ": BETWEEN PROFILES nProfile = " << nProfile << " nThisPointOnCoast = " << nThisPointOnCoast << " dDepthofBreaking = " << dDepthofBreaking << endl;

      for (int i = 0; i < nParProfSize; i++)
      {
         // Now calculate depth over DB
         dVParProfileDepthOverDB[i] = m_dThisIterStillWaterLevel - dVParProfileZ[i];
         dVParProfileDepthOverDB[i] /= dDepthofBreaking;

         // Constrain dDepthOverDB[i] to be between 0 (can get small -ve due to rounding errors) and m_dDepthOverDBMax
         dVParProfileDepthOverDB[i] = tMax(dVParProfileDepthOverDB[i], 0.0);
         dVParProfileDepthOverDB[i] = tMin(dVParProfileDepthOverDB[i], m_dDepthOverDBMax);

         // And then use the look-up table to find the value of erosion potential at this point on the profile
         dVParProfileErosionPotential[i] = dLookUpErosionPotential(dVParProfileDepthOverDB[i]);

         // If erosion potential (a -ve value) is tiny, set it to zero
         if (dVParProfileErosionPotential[i] > EROSIONPOTENTIALTOLERANCE)
            dVParProfileErosionPotential[i] = 0;
      }

      // Set up more arrays for points along the parallel profile
      vector<double>
         dVParBeachProtection(nParProfSize, m_dBeachProtectionFactor),
         dVParRecessionXY(nParProfSize, 0),
         dVParChangeElevZ(nProfSize, 0);                       // Change in elevation due to erosion

      // Calculate the erosional lowering at each point along the parallel profile
      for (int i = 0; i < nParProfSize; i++)
      {
         int
            nXPar = PtiVGridParProfile[i].nGetX(),
            nYPar = PtiVGridParProfile[i].nGetY();

         // First store the local slope, this is just for output display purposes
         m_pRasterGrid->Cell[nXPar][nYPar].SetLocalSlope(dVParProfileSlope[i]);

         if (i > 0)
         {
            // For the actual erosion calcs: start at point 1 since this is the first inundated point
            //
            // dRecession = dForce * (dBeachProtection / dR) * dErosionPotential * dSlope * dTime
            // where:
            //     dVRecession [m] is the landward migration distance defined in the profile relative (XY) CRS
            //     dForce is given by Equation 4 in Walkden & Hall, 2005
            //     dVBeachProtection [1] is beach protection factor [1, 0] = [no protection, fully protected]
            //     dVR  [m^(9/4)s^(2/3)] is the material strength and some hydrodynamic constant
            //     dVProfileErosionPotential [?] is the erosion potential at each point along the profile
            //     dVSlope [1] is the along-profile slope
            //     m_dTimeStep * 3600 [s] is the time interval in seconds
            //
            // dRecession is horizontal recession (along the XY direction):
            //
            // dVRecessionXY[i] = (dForce * dVBeachProtection[i] * dVErosionPotentialFunc[i] * dVSlope[i] * m_dTimeStep * 3600) / dVR[i]
            //
            //
            // XY recession must be -ve or zero. If it is +ve then it represents accretion not erosion, which must be described by a different set of equations. So we also need to constrain XY recession to be <= 0
            dVParRecessionXY[i] = tMin(m_VCoast[nCoast].dGetWaveEnergy(nThisPointOnCoast) * dVParBeachProtection[i] * dVParProfileErosionPotential[i] * dVParProfileSlope[i] / m_dR, 0.0);

            // OK, we have the horizontal recession (along the XY direction) so now calculate the vertical lowering dZ = dXY * tan(a), where tan(a) is the slope in the XY direction. Expressed as a single equation:
            //
            // dDeltaZ = tMin(dForce * (dBeachProtection / dR) * dErosionPotential * dSlope * dSlope * dTime, 0.0);
            double dDeltaZ = dVParRecessionXY[i] * dVParProfileSlope[i];

            if (dDeltaZ < 0)
            {
               // dDeltaZ is zero or -ve: if dDeltaZ is zero then do nothing, if -ve then remove dDeltaZ from this cell
               dVParChangeElevZ[i] = dDeltaZ;

               // Has this cell already been eroded during this iteration?
               if (m_pRasterGrid->Cell[nXPar][nYPar].bPotentialErosion())
               {
                  // It has, so use a weighted average of this erosion depth, and the previously-calculated erosion depth, where closeness to a coastline-normal profile gives greater weight
                  double dLastWeight = m_pRasterGrid->Cell[nXPar][nYPar].dGetWeight();

                  if (dLastWeight != DBL_NODATA)
                  {
                     // It has already been eroded, so get the previous value of potential erosion (is +ve)
                     double dPrevPotErosion = m_pRasterGrid->Cell[nXPar][nYPar].dGetPotentialErosion();

                     // Calculate the weighted value of potential erosion
                     double dThisWeight = (1.0 / nDistFromProfile);
                     double dWeightedPotErosion = (dLastWeight * dPrevPotErosion) + (dThisWeight * (-dDeltaZ));
                     dWeightedPotErosion /= (dThisWeight + dLastWeight);

//                     LogStream << "Previous value = " << dPrevPotErosion << ",  weighted value = " << dWeightedPotErosion << endl << endl;
                     m_pRasterGrid->Cell[nXPar][nYPar].SetPotentialErosion(dWeightedPotErosion);

                     // Update this-iteration totals
                     m_ulThisIterNPotentialErosionCells++;
                     m_dThisIterPotentialErosion += dWeightedPotErosion;

                     // Increment the check values
                     m_ulTotPotErosionBetweenProfiles++;
                     m_dTotPotErosionBetweenProfiles += dWeightedPotErosion;
                  }
               }

               else
               {
                  // Not already eroded, so just set the potential (unconstrained) erosion (a +ve value) for this cell and mark the cell as eroded
                  m_pRasterGrid->Cell[nXPar][nYPar].SetPotentialErosion(-dDeltaZ);

                  // Update this-iteration totals
                  m_ulThisIterNPotentialErosionCells++;
                  m_dThisIterPotentialErosion -= dDeltaZ;

                  // Increment the check values
                  m_ulTotPotErosionBetweenProfiles++;
                  m_dTotPotErosionBetweenProfiles -= dDeltaZ;
               }
            }
         }
      }

//      LogStream << m_ulIter << ": coast = " << nCoast << " profile = " << nProfile << " nDirection = " << nDirection << " dist from profile = " <<  nDistFromProfile << endl;

      // If desired, save this parallel coastline-normal profiles for checking purposes
      if (m_bOutputParallelProfileData)
      {
         int nRet = nSaveParProfile(nCoast, nProfile, nParProfSize, nDirection, nDistFromProfile, &dVParProfileDistXY, &dVParProfileZ, &dVParProfileDepthOverDB, &dVParProfileErosionPotential, &dVParProfileSlope, &dVParRecessionXY, &dVParChangeElevZ, pVPtiGridProfile);
         if (nRet != RTN_OK)
            return nRet;
      }

      // Update for next time round the loop
      nParCoastXLast = nParCoastX;
      nParCoastYLast = nParCoastY;
   }

   return RTN_OK;
}

/*==============================================================================================================================

 Calculates actual (i.e. unconstrained) erosion on all sea cells, include the cells which are in the coastline-normal profiles

==============================================================================================================================*/
int CSimulation::nCalcActualErosionOnAllSeaCells(void)
{
   for (int nX = 0; nX < m_nXGridMax; nX++)
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         if (m_pRasterGrid->Cell[nX][nY].bPotentialErosion())
            DoActualErosionOnCell(nX, nY);
      }

   return RTN_OK;
}

/*==============================================================================================================================

 Calculates actual (constrained by available sediment) erosion on a single sea cell

==============================================================================================================================*/
void CSimulation::DoActualErosionOnCell(int const nX, int const nY)
{
   // Get the potential depth of potential erosion
   double dThisPotentialErosion = m_pRasterGrid->Cell[nX][nY].dGetPotentialErosion();

   // TODO get working with multiple layers i.e. discard any layers with zero thickness. At present, just considering the top layer only
   int nThisLayer = m_nLayers - 1;

   // Find out how much sediment we have available on this cell
   double dExistingAvailableFine = m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->dGetFine();
   double dExistingAvailableSand = m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->dGetSand();
   double dExistingAvailableCoarse = m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->dGetCoarse();

   // Now partition the total lowering for this cell between the three size fractions: do this by relative erodibility
   int nFineWeight = (dExistingAvailableFine > 0 ? 1 : 0);
   int nSandWeight = (dExistingAvailableSand > 0 ? 1 : 0);
   int nCoarseWeight = (dExistingAvailableCoarse > 0 ? 1 : 0);

   double dTotErodibility = (nFineWeight * m_dFineErodibility) + (nSandWeight * m_dSandErodibility) + (nCoarseWeight * m_dCoarseErodibility);
   double dTotActualErosion = 0;

   if (nFineWeight)
   {
      // Erode some fine-sized sediment
      double dFineLowering = (nFineWeight * m_dFineErodibility * dThisPotentialErosion) / dTotErodibility;

      // Make sure we don't get -ve amounts left on the cell
      double dFine = tMin(dExistingAvailableFine, dFineLowering);
      double dRemaining = dExistingAvailableFine - dFine;

      dTotActualErosion += dFine;

      // Set the value for this layer
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->SetFine(dRemaining);

      // And increment the per-iteration total
      m_dThisIterActualFineErosion += dFine;
   }

   if (nSandWeight)
   {
      // Erode some sand-sized sediment
      double dSandLowering = (nSandWeight * m_dSandErodibility * dThisPotentialErosion) / dTotErodibility;

      // Make sure we don't get -ve amounts left on the source cell
      double dSand = tMin(dExistingAvailableSand, dSandLowering);
      double dRemaining = dExistingAvailableSand - dSand;

      dTotActualErosion += dSand;

      // Set the value for this layer
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->SetSand(dRemaining);

      // And increment the per-iteration total
      m_dThisIterActualSandErosion += dSand;
   }

   if (nCoarseWeight)
   {
      // Erode some coarse-sized sediment
      double dCoarseLowering = (nCoarseWeight * m_dCoarseErodibility * dThisPotentialErosion) / dTotErodibility;

      // Make sure we don't get -ve amounts left on the source cell
      double dCoarse = tMin(dExistingAvailableCoarse, dCoarseLowering);
      double dRemaining = dExistingAvailableCoarse - dCoarse;

      dTotActualErosion += dCoarse;

      // Set the value for this layer
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nThisLayer)->pGetUnconsolidatedSediment()->SetCoarse(dRemaining);

      // And increment the per-iteration total
      m_dThisIterActualCoarseErosion += dCoarse;
   }

   // Set the actual erosion value for this cell
   m_pRasterGrid->Cell[nX][nY].SetActualErosion(dTotActualErosion);

   // Recalculate the elevation of every layer
   m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

   // Update per-iteration totals
   if (dTotActualErosion > 0)
   {
      m_ulThisIterNActualErosionCells++;
      m_dThisIterActualErosion += dTotActualErosion;
   }


   // TODO Do consolidated sediment too

}

/*==============================================================================================================================

 Creates a look-up table for erosion potential, given depth over DB

==============================================================================================================================*/
bool CSimulation::bCreateErosionPotentialLookUp(vector<double> *VdDepthOverDBIn, vector<double> *VdErosionPotentialIn, vector<double>  *VdErosionPotentialFirstDerivIn)
{
   // Set up a temporary vector to hold the incremental DepthOverDB values
   vector<double> VdDepthOverDB;
   double dTempDOverDB = 0;

   while (dTempDOverDB <= 1.1)   // Arbitrary max value, we will adjust this later
   {
      VdDepthOverDB.push_back(dTempDOverDB);          // These are the incremental sample values of DepthOverDB
      dTempDOverDB += DODBINCREMENT;

      m_VdErosionPotential.push_back(0);              // This will hold the corresponding value of erosion potential for each sample point
   }

   int nSize = VdDepthOverDB.size();
   vector<double>
   dVDeriv(nSize, 0),         // First derivative at the sample points: calculated by the spline function but not subsequently used
           dVDeriv2(nSize, 0.),       // Second derivative at the sample points, ditto
           dVDeriv3(nSize, 0.);       // Third derivative at the sample points, ditto

   // Calculate the value of erosion potential (is a -ve value) for each of the sample values of DepthOverDB, and store it for use in the look-up function
   hermite_cubic_spline_value(VdDepthOverDBIn->size(), &(VdDepthOverDBIn->at(0)), &(VdErosionPotentialIn->at(0)), &(VdErosionPotentialFirstDerivIn->at(0)), nSize, &(VdDepthOverDB[0]), &(m_VdErosionPotential[0]), &(dVDeriv[0]), &(dVDeriv2[0]), &(dVDeriv3[0]));

   // Tidy the erosion potential look-up data: cut off values (after the first) for which erosion potential is no longer -ve
   int nLastVal = -1;

   for (int n = 1; n < nSize - 1; n++)
      if (m_VdErosionPotential[n] > 0)
      {
         nLastVal = n;
         break;
      }

   if (nLastVal > 0)
   {
      // Erosion potential is no longer -ve at this value of DepthOverDB, so set the maximum value of DepthOverDB that will be used in the simulation (any DepthOverDB value greater than this produces zero erosion potential)
      m_dDepthOverDBMax = VdDepthOverDB[nLastVal];
      m_VdErosionPotential.erase(m_VdErosionPotential.begin() + nLastVal + 1, m_VdErosionPotential.end());
      m_VdErosionPotential.back() = 0;
   }

   else
      // Erosion potential is unbounded, i.e. it is still -ve when we have reached the end of the look-up vector
      return false;

   // All OK
   return true;
}

/*==============================================================================================================================

 The erosion potential lookup: it returns a value for erosion potential given a value of Depth Over DB

==============================================================================================================================*/
double CSimulation::dLookUpErosionPotential(double const dDepthOverDB) const
{
   // If dDepthOverDB exceeds the maximum value which we calculated earlier, erosion potential is zero
   if (dDepthOverDB > m_dDepthOverDBMax)
      return 0;

   // OK, dDepthOverDB is less than the maximum so look up a corresponding value for erosion potential. The look-up index is dDepthOverDB divided by (the Depth Over DB increment used when creating the look-up vector). But since this look-up index may not be an integer, split the look-up index into integer and fractional parts and deal with each separately
   double dIntPart = 0;
   double dFractPart = modf(dDepthOverDB * INVDODBINCREMENT, &dIntPart);
   unsigned int nIndex = static_cast<unsigned int>(dIntPart);

   // Use the integer part of the look-up index to get a preliminary value of erosion potential
   double dErosionPotential = m_VdErosionPotential[nIndex];

   if (dFractPart > 0)
   {
      // If the fractional part is non-zero, do a linear interpolation between this and the next value of erosion potential in the look-up table (note that the final value of erosion potential in the look-up table is always zero)
      double dInterpolated = dFractPart * (dErosionPotential - m_VdErosionPotential[nIndex + 1]);
      dErosionPotential -= dInterpolated;    // Because erosion potential is -ve
   }

   return dErosionPotential;
}


/*==============================================================================================================================

 Returns a vector containing the dZ/dXY slope (tan a) given an x-y vector and a z vector. The x and y input vectors are in raster-grid units and the z vector is in real-world units

===============================================================================================================================*/
vector<double> CSimulation::dVCalcProfileSlope(vector<C2DPoint> *pVXY, vector<double> *pVZ)
{
   unsigned int nSize = pVXY->size();
   vector<double> dVSlope(nSize, 0);

   // Calculate the between-point differences between this point and the preceding point: in the XY direction (i.e. along the length of the profile) and in the Z direction. Can't do this for the first point of course, this remains zero
   for (unsigned int i = 1; i < nSize - 1; i++)
   {
      // First the XY distances (already in external CRS)
      double dX1 = pVXY->at(i - 1).dGetX();
      double dY1 = pVXY->at(i - 1).dGetY();

      double dX2 = pVXY->at(i + 1).dGetX();
      double dY2 = pVXY->at(i + 1).dGetY();

      // Calc the XY distance between the (i-1) and (i) points
      double dXYDiff = hypot((dX1 - dX2), (dY1 - dY2));

      // Do the same for Z differences (already in external CRS units)
      double dZDiff = pVZ->at(i - 1) - pVZ->at(i + 1);

      // Calculate dZ/dXY, the Z slope (i.e. rise over run) in the XY direction
      if (dXYDiff != 0)
         dVSlope[i] = dZDiff / dXYDiff;
   }

   return dVSlope;
}


/*==============================================================================================================================

 Construct a parallel coastline-normal profile

===============================================================================================================================*/
void CSimulation::ConstructParallelProfile(int const nDistFromProfile, int const nProfileStartX, int const nProfileStartY, int const nParCoastX, int const nParCoastY, int const nProfSize, vector<C2DIPoint>* const pVPtiGridProfile, vector<C2DIPoint>* pPtiVGridParProfile, vector<C2DPoint>* pPtVExtCRSParProfile)
{
   // OK, we have the coastline start point for the parallel profile. Now construct a temporary profile, parallel to the coastline-normal profile, starting from this point
   int
      nXOffset = nParCoastX - nProfileStartX,
      nYOffset = nParCoastY - nProfileStartY;

   // Put the coastline start point at the beginning of the temporary profile co-ords vectors
   pPtiVGridParProfile->push_back(C2DIPoint(nParCoastX, nParCoastY));
   pPtVExtCRSParProfile->push_back(C2DPoint(dGridXToExtCRSX(nParCoastX), dGridYToExtCRSY(nParCoastY)));

   // Starting at the cell which is just seaward of the coastline, append co-ord values for the temporary profile
   for (int nProfilePoint = 1; nProfilePoint < nProfSize; nProfilePoint++)
   {
      // Get the grid co-ordinates of the cell which is this distance seaward, from the coastline-normal profile
      int nXProf = pVPtiGridProfile->at(nProfilePoint).nGetX();
      int nYProf = pVPtiGridProfile->at(nProfilePoint).nGetY();

      // Now calculate the grid co-ordinates of this cell, which is potentially in the parallel profile
      int nXPar = nXProf + nXOffset;
      int nYPar = nYProf + nYOffset;

      // Is this cell within the grid? If not, cut short the profile
      if (! bIsWithinGrid(nXPar, nYPar))
         return;

      // Have we hit an adjacent coastline-normal profile?
      if (m_pRasterGrid->Cell[nXPar][nYPar].bGetProfile())
         return;

      // OK, append the cell details
      pPtiVGridParProfile->push_back(C2DIPoint(nXPar, nYPar));
      pPtVExtCRSParProfile->push_back(C2DPoint(dGridXToExtCRSX(nXPar), dGridYToExtCRSY(nYPar)));

      // And set the inverse of the distance-from-profile value for this point on the parallel profile, this is used in the weighting function when a cell has more than one erosion value calculated
      assert(nDistFromProfile > 0);
      double dWeight = 1.0 / nDistFromProfile;
      m_pRasterGrid->Cell[nXPar][nYPar].SetWeight(dWeight);
   }
}
