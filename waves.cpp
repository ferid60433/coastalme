/*!
 *
 * \file waves.cpp
 * \brief Simulates wave propagation
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

 Simulates wave propagation along all coastline-normal profiles, based on a routine by Martin Hurst

===============================================================================================================================*/
int CSimulation::nDoAllPropagateWaves(void)
{
   // Calculate some wave properties based on the wave period following Airy wave theory
   m_dC_0 = (G * m_dWavePeriod) / (2 * PI);          // Deep water (offshore) wave celerity (m/s)
   m_dL_0 = m_dC_0 * m_dWavePeriod;                  // Deep water (offshore) wave length (m)

   for (int nCoast = 0; nCoast < static_cast<int>(m_VCoast.size()); nCoast++)
   {
      int
         nCoastSize = m_VCoast[nCoast].nGetCoastlineSize(),
         nNumProfiles = m_VCoast[nCoast].nGetNumProfiles();

      // Calculate wave properties at every point along each valid profile, and for the cells under the profiles
      for (int nProfile = 0; nProfile < nNumProfiles; nProfile++)
         CalcWaveProperties(nCoast, nCoastSize, nProfile);

      // Next, interpolate these wave properties for all remaining coastline points
      for (int nProfile = 0; nProfile < nNumProfiles; nProfile++)
         InterpolateWavePropertiesToCoastline(nCoast, nCoastSize, nProfile, nNumProfiles);

      // Calculate wave energy at every point on the coastline
      for (int n = 0; n < nCoastSize; n++)
      {
         // Equation 4 from Walkden & Hall, 2005
         double dErosiveWaveForce = pow(m_VCoast[nCoast].dGetBreakingWaveHeight(n), 3.25) * pow(m_dWavePeriod, 0.75);

         // Calculate total wave energy at each coast point during this timestep
         m_VCoast[nCoast].SetWaveEnergy(n, dErosiveWaveForce * m_dTimeStep * 3600);
      }

      // Finally, interpolate wave properties for the cells between the profiles
      for (int nProfile = 0; nProfile < nNumProfiles; nProfile++)
         InterpolateWavePropertiesToCells(nCoast, nCoastSize, nProfile, nNumProfiles);
   }

   return RTN_OK;
}


/*===============================================================================================================================

 Calculates wave properties along a coastline-normal profile. This assumes shore-parallel contours, because flux orientation does not change as we move seawards along the profile

 This is a modified version of a routine by Martin Hurst, from the COVE model

===============================================================================================================================*/
void CSimulation::CalcWaveProperties(int const nCoast, int const nCoastSize, int const nProfile)
{
   int nCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNumCoastPoint();

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

   // Determine dAlpha_0, the angle between the coast and offshore wave crest approach
   double dAlpha_0 = 0;
   if (m_dOffshoreWaveOrientation <= dFluxOrientationNext)
      dAlpha_0 = dFluxOrientationThis - m_dOffshoreWaveOrientation - 90;
   else if (m_dOffshoreWaveOrientation > (dFluxOrientationThis + 270))
      dAlpha_0 = (dFluxOrientationThis + 270) - m_dOffshoreWaveOrientation;
   else
      dAlpha_0 = 270 - (m_dOffshoreWaveOrientation - dFluxOrientationThis);

  // Calculate dAlpha_0_Prev, this is for the previous coast point
  double dAlpha_0_Prev = 0;
  if (nCoastPoint > 0)
   {
      if (m_dOffshoreWaveOrientation <= m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1))
         dAlpha_0_Prev = dFluxOrientationPrev - m_dOffshoreWaveOrientation - 90;
      else if (m_dOffshoreWaveOrientation > (m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1) + 270))
         dAlpha_0_Prev = dFluxOrientationPrev + 270 - m_dOffshoreWaveOrientation;
      else
         dAlpha_0_Prev = 270 - (m_dOffshoreWaveOrientation - dFluxOrientationPrev);
   }
   else
      dAlpha_0_Prev = dAlpha_0;

   // Calculate dAlpha_0_Next, this is for the next coast point
   double dAlpha_0_Next = 0;
   if (nCoastPoint < nCoastSize-1)
   {
      if (m_dOffshoreWaveOrientation <= m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1))
         dAlpha_0_Next = dFluxOrientationNext - m_dOffshoreWaveOrientation - 90;
      else if (m_dOffshoreWaveOrientation > (m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1) + 270))
         dAlpha_0_Next = dFluxOrientationNext + 270 - m_dOffshoreWaveOrientation;
      else
         dAlpha_0_Next = 270 - (m_dOffshoreWaveOrientation - m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1));
   }
   else
      dAlpha_0_Next = dAlpha_0;

   // Following Ashton and Murray (2006), if we have high-angle waves then use the flux orientation of the previous node (updrift), if transitioning from diffusive to antidiffusive use flux maximising angle (45 degrees)
   if (dAlpha_0 <= -90 || dAlpha_0 >= 90)
      // Wave is oriented offshore, so no transport
      dAlpha_0 = 0;
   else if ((dAlpha_0_Prev > 0) && (dAlpha_0 > 0))
   {
      // dAlpha_0 and dAlpha_0_Prev are both +ve
      if ((dAlpha_0_Prev < 45) && (dAlpha_0 > 45))
         dAlpha_0 = 45;
      else if (dAlpha_0 > 45)
         dAlpha_0 = dAlpha_0_Prev;
   }
   else if ((dAlpha_0_Next < 0) && (dAlpha_0 < 0))
   {
      // dAlpha_0 and dAlpha_0_Next are both -ve
      if ((dAlpha_0_Next > -45) && (dAlpha_0 < -45))
         dAlpha_0 = -45;
      else if (dAlpha_0 < -45)
         dAlpha_0 = dAlpha_0_Next;
   }
   else if (dAlpha_0 > 45 && dAlpha_0_Prev > 0)
   {
      // For high-angle waves use updrift orientation
      if (m_dOffshoreWaveOrientation <= dFluxOrientationThis)
         dAlpha_0 = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1) - m_dOffshoreWaveOrientation - 90;
      else if (m_dOffshoreWaveOrientation > (dFluxOrientationThis+270))
         dAlpha_0 = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1) + 270 - m_dOffshoreWaveOrientation;
      else
         dAlpha_0 = 270 - (m_dOffshoreWaveOrientation - m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint-1));
   }
   else if (dAlpha_0 < -45 && dAlpha_0_Next < 0)
   {
      // For high-angle waves use updrift orientation
      if (m_dOffshoreWaveOrientation <= dFluxOrientationThis)
         dAlpha_0 = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1) - m_dOffshoreWaveOrientation - 90;
      else if (m_dOffshoreWaveOrientation > (dFluxOrientationThis + 270))
         dAlpha_0 = m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1) + 270 - m_dOffshoreWaveOrientation;
      else
         dAlpha_0 = 270 - (m_dOffshoreWaveOrientation - m_VCoast[nCoast].dGetFluxOrientation(nCoastPoint+1));
   }

   // DFM TEST Is needed because of occasional problem with Martin's code above
   dAlpha_0 = tMin(dAlpha_0, 90.0);
   dAlpha_0 = tMax(dAlpha_0, -90.0);

   bool bBreaking = false;
   int
      nProfileSize = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile(),
      nBreakingDist = 0;
   double
      dBreakingWaveHeight = 0,
      dBreakingWaveOrientation = 0,
      dBreakingWaveOrientationExtCRS = 0,
      dBreakingDepth = 0;

   // Now go landwards along the profile, calculate wave height and wave angle for every inundated point on the profile (don't do point zero, this is on the coastline) until the waves start to break
   for (int nProfilePoint = (nProfileSize-1); nProfilePoint > 0; nProfilePoint--)
   {
      int nX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX();
      int nY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY();

      double
         dWaterDepth = m_pRasterGrid->Cell[nX][nY].dGetWaterDepth(),                      // Water depth for the cell 'under' this point in the profile
         dWaveHeight = 0,
         dAlpha = 0;

      if ((! bBreaking) && (dAlpha_0 != 0) && (dWaterDepth > 0))
      {
         // If we are not in the active (breaking) zone, and the waves are not oriented offshore, and water depth is not zero, then calculate wave height and angle
         double dL = m_dL_0 * sqrt(tanh((2 * PI * dWaterDepth) / m_dL_0));                // Wavelength (m) in intermediate-shallow waters
//         double dL = m_dL_0 * pow(tanh(pow(pow(2 * PI / m_dWavePeriod, 2) * dWaterDepth / G, 0.75)), 2.0 / 3.0);  //Fenton & McKee (1990) formulation
         double dC = m_dC_0 * tanh((2 * PI * dWaterDepth) / dL);                          // Wave speed (m/s) set by dWaterDepth, dL and m_dC_0
         double dk = 2 * PI / dL;                                                         // Wave number (1/m)
         double dn = ((2 * dWaterDepth * dk) / (sinh(2 * dWaterDepth * dk)) + 1) / 2;     // Shoaling factor
         double dKs = sqrt(m_dC_0 / (dn * dC * 2));                                       // Shoaling coefficient
         dAlpha = (180 / PI) * asin((dC / m_dC_0) * sin((PI / 180) * dAlpha_0));          // Calculate wave angle
         double dKr = sqrt(cos((PI / 180) * dAlpha_0) / cos((PI / 180) * dAlpha));        // Refraction coefficient
         dWaveHeight = m_dOffshoreWaveHeight * dKs * dKr;                                 // Calculate wave height

//          assert(dWaveHeight >= 0);

         // Now see if the wave breaks
         if (dWaveHeight > (dWaterDepth * ACTIVE_ZONE_RATIO))
         {
            // It does! We are in the active zone
            bBreaking = true;
            dBreakingWaveHeight = dWaveHeight;                          // Gets updated as we move landward, the last value is the one we need
            dBreakingWaveOrientation = dFluxOrientationThis + dAlpha;   // Ditto
            dBreakingDepth = dWaterDepth;                               // Ditto

            // Convert orientation of breaking waves to CRS which was used for input
            dBreakingWaveOrientationExtCRS = dKeepWithin360(dBreakingWaveOrientation - 180);

            nBreakingDist = nProfilePoint;
         }
      }

      if (bBreaking)
      {
         // We are in the active zone, so set these for the cell 'under' the profile point
         m_pRasterGrid->Cell[nX][nY].SetInActiveZone(true);
         m_pRasterGrid->Cell[nX][nY].SetWaveHeight(dBreakingWaveHeight);
         m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(dBreakingWaveOrientationExtCRS);
      }
      else
      {
         // We are not in the active zone
         m_pRasterGrid->Cell[nX][nY].SetWaveHeight(m_dOffshoreWaveHeight);
         m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(m_dOffshoreWaveOrientationIn);
      }
   }

//   LogStream << m_ulIter << ": nProfile = " << nProfile << " " << (dAlpha_0 == 0 ? "OFFSHORE" : "onshore") << " dBreakingWaveHeight = " << dBreakingWaveHeight << " dFluxOrientationThis = " << dFluxOrientationThis << " dBreakingWaveOrientation = " << dBreakingWaveOrientation << " dBreakingDepth = " << dBreakingDepth << endl;

   // Set the breaking wave height, breaking wave angle, and depth of breaking for the coast point
   m_VCoast[nCoast].SetBreakingWaveHeight(nCoastPoint, dBreakingWaveHeight);
   m_VCoast[nCoast].SetBreakingWaveAngle(nCoastPoint, dBreakingWaveOrientation);
   m_VCoast[nCoast].SetDepthOfBreaking(nCoastPoint, dBreakingDepth);
   m_VCoast[nCoast].SetBreakingDistance(nCoastPoint, nBreakingDist);
}


/*===============================================================================================================================

 Interpolates wave properties from profiles to the in-between points along a coastline

===============================================================================================================================*/
void CSimulation::InterpolateWavePropertiesToCoastline(int const nCoast, int const nCoastSize, int const nProfile, int const nNumProfiles)
{
   int
      nThisCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNumCoastPoint();

   // For the breaking wave stuff, to go into the in-between coastline points
   int nThisBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nThisCoastPoint);
   double
      dThisBreakingWaveHeight = m_VCoast[nCoast].dGetBreakingWaveHeight(nThisCoastPoint),
      dThisBreakingWaveAngle = m_VCoast[nCoast].dGetBreakingWaveAngle(nThisCoastPoint),
      dThisBreakingDepth = m_VCoast[nCoast].dGetDepthOfBreaking(nThisCoastPoint);

   // -----------------------------------------------------------------------------------------------------------------
   // For the last profile on a coast, just put the profile's wave properties unchanged into every coast point up to the end of the coast
   if (nProfile == (nNumProfiles-1))
   {
      for (int n = nThisCoastPoint+1; n < nCoastSize; n++)
      {
         // Set the breaking wave height, breaking wave angle, and depth of breaking for the coast point
         m_VCoast[nCoast].SetBreakingWaveHeight(n, dThisBreakingWaveHeight);
         m_VCoast[nCoast].SetBreakingWaveAngle(n, dThisBreakingWaveAngle);
         m_VCoast[nCoast].SetDepthOfBreaking(n, dThisBreakingDepth);
         m_VCoast[nCoast].SetBreakingDistance(n, nThisBreakingDist);
      }

      return;
   }

   // -----------------------------------------------------------------------------------------------------------------
   // For all other profiles, weight the wave properties for this profile, and for the next profile along the coast, so that they change smoothly between the two profiles
   int
      nNextCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile+1)->nGetNumCoastPoint(),
      nDistBetween = nNextCoastPoint - nThisCoastPoint - 1;

   if (nDistBetween <= 0)
      // Nothing to do
      return;

   int nNextBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nNextCoastPoint);
   double
      dNextBreakingWaveHeight = m_VCoast[nCoast].dGetBreakingWaveHeight(nNextCoastPoint),
      dNextBreakingWaveAngle = m_VCoast[nCoast].dGetBreakingWaveAngle(nNextCoastPoint),
      dNextBreakingDepth = m_VCoast[nCoast].dGetDepthOfBreaking(nNextCoastPoint);

   for (int n = nThisCoastPoint+1; n < nNextCoastPoint; n++)
   {
      int
         nDist = n - nThisCoastPoint;

      double
         dThisWeight = (nDistBetween - nDist) / static_cast<double>(nDistBetween),
         dNextWeight = 1 - dThisWeight,
         dBreakingWaveHeight = (dThisWeight * dThisBreakingWaveHeight) + (dNextWeight * dNextBreakingWaveHeight),
         dBreakingWaveOrientation = (dThisWeight * dThisBreakingWaveAngle) + (dNextWeight * dNextBreakingWaveAngle),
         dBreakingDepth = (dThisWeight * dThisBreakingDepth) + (dNextWeight * dNextBreakingDepth),
         dBreakingDist = (dThisWeight * nThisBreakingDist) + (dNextWeight * nNextBreakingDist);

      // Set the breaking wave height, breaking wave angle, and depth of breaking for this coast point
      m_VCoast[nCoast].SetBreakingWaveHeight(n, dBreakingWaveHeight);
      m_VCoast[nCoast].SetBreakingWaveAngle(n, dBreakingWaveOrientation);
      m_VCoast[nCoast].SetDepthOfBreaking(n, dBreakingDepth);
      m_VCoast[nCoast].SetBreakingDistance(n, dRound(dBreakingDist));
   }

   // -----------------------------------------------------------------------------------------------------------------
   // Additionally, for the first profile along the coast, put this profile's wave properties unchanged into every coast point back to the start of the coast
   if (nProfile == 0)
   {
      for (int n = nThisCoastPoint-1; n >= 0; n--)
      {
         // Set the breaking wave height, breaking wave angle, and depth of breaking for the coast point
         m_VCoast[nCoast].SetBreakingWaveHeight(n, dThisBreakingWaveHeight);
         m_VCoast[nCoast].SetBreakingWaveAngle(n, dThisBreakingWaveAngle);
         m_VCoast[nCoast].SetDepthOfBreaking(n, dThisBreakingDepth);
         m_VCoast[nCoast].SetBreakingDistance(n, nThisBreakingDist);
      }
   }
}


/*===============================================================================================================================

 Interpolates wave properties from profiles to the cells between the profiles

===============================================================================================================================*/
void CSimulation::InterpolateWavePropertiesToCells(int const nCoast, int const nCoastSize, int const nProfile, int const nNumProfiles)
{
   int
      nThisProfileCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNumCoastPoint(),
      nThisCoastX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisProfileCoastPoint)->nGetX(),
      nThisCoastY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisProfileCoastPoint)->nGetY();

   // -----------------------------------------------------------------------------------------------------------------
   // For the last profile on a coast, just put the profile's wave properties unchanged into every cell up to the edge of the grid then return
   if (nProfile == (nNumProfiles-1))
   {
//      LogStream << m_ulIter << ": interpolating wave properties for cells between profile " << nProfile << " and coastline END" << endl;

      int nProfileLen = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile();

      // These are for the x and y offsets between the point on the profile and the point on the coast
      vector<int>
         nVXOffSet,
         nVYOffSet;

      // Get offsets between the coast point and every other point on the profile
      for (int nProfilePoint = 1; nProfilePoint < nProfileLen; nProfilePoint++)
      {
         int
            nThisX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
            nThisY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY(),
            nXOffSet = nThisX - nThisCoastX,
            nYOffSet = nThisY - nThisCoastY;

         nVXOffSet.push_back(nXOffSet);
         nVYOffSet.push_back(nYOffSet);
      }

      // Move forwards along the coast until we reach its end
      for (int nCoastPoint = (nThisProfileCoastPoint+1); nCoastPoint < nCoastSize; nCoastPoint++)
      {
         // Again move landwards along the profile until we hit the last inundated point
         bool bInActiveZone = false;
         int nBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nCoastPoint);
         for (int nProfilePoint = (nProfileLen-1); nProfilePoint > 0; nProfilePoint--)
         {
            // Calc the x and y coords for the 'target' cell
            int
               nX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetX() + nVXOffSet[nProfilePoint-1],
               nY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetY() + nVYOffSet[nProfilePoint-1];

            // Safety checks
            if (! bIsWithinGrid(nX, nY))
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] outside " << m_nXGridMax << " x " << m_nYGridMax << " grid" << endl;
               continue;
            }

            if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] dry land" << endl;
               continue;
            }

//            LogStream << m_ulIter << ": [" << nX << "][" << nY << "] is inside grid, will update" << endl;

            // Get the x and y coords of the point on the profile
            int
               nThisProfileX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
               nThisProfileY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY();

            // Get the wave orientation and height values from the cells under the profile point
            double
               dProfileWaveHeight = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveHeight(),
               dProfileWaveOrientation = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveOrientation();

            // Check for pre-existing values
            double dOldWaveOrientation = m_pRasterGrid->Cell[nX][nY].dGetWaveOrientation();
            double dOldWaveHeight = m_pRasterGrid->Cell[nX][nY].dGetWaveHeight();
            if (dOldWaveOrientation != m_dOffshoreWaveOrientation)
               dProfileWaveOrientation = (dProfileWaveOrientation + dOldWaveOrientation) / 2;
            if (dOldWaveHeight != m_dOffshoreWaveHeight)
               dProfileWaveHeight = (dProfileWaveHeight + dOldWaveHeight) / 2;

            m_pRasterGrid->Cell[nX][nY].SetWaveHeight(dProfileWaveHeight);
            m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(dProfileWaveOrientation);

            if (nProfilePoint == nBreakingDist)
               bInActiveZone = true;

            if (bInActiveZone)
               m_pRasterGrid->Cell[nX][nY].SetInActiveZone(true);
         }
      }

      return;
   }

   // -----------------------------------------------------------------------------------------------------------------
   // For all other profiles, weight the wave properties for this profile, and for the next profile along the coast, so that they change smoothly between the two profiles
   int
      nNextProfileCoastPoint = m_VCoast[nCoast].pGetProfile(nProfile+1)->nGetNumCoastPoint(),
      nDistBetween = nNextProfileCoastPoint - nThisProfileCoastPoint - 1;

   if (nDistBetween <= 0)
      // Nothing to do
      return;

   int
      nNextCoastX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nNextProfileCoastPoint)->nGetX(),
      nNextCoastY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nNextProfileCoastPoint)->nGetY(),
      nProfileLen = tMin(m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile(), m_VCoast[nCoast].pGetProfile(nProfile+1)->nGetNCellsInProfile());

   // First go foward along the coast (i.e. start from 'this' profile then work towards the 'next' profile), then go backwards (i.e. start from the 'next' profile then work towards 'this' profile)
   for (int nDirection = DIRECTION_FORWARD; nDirection <= DIRECTION_BACKWARD; nDirection++)
   {
      // Sort out the 'start' profile
      int
         nStartProfile = nProfile,
         nStartCoastX = nThisCoastX,
         nStartCoastY = nThisCoastY;
      if (nDirection == DIRECTION_BACKWARD)
      {
         nStartProfile = nProfile+1;
         nStartCoastX = nNextCoastX,
         nStartCoastY = nNextCoastY;
      }

      // These are for the x and y offsets between the point on the profile and the point on the coast, along the 'start' profile
      vector<int>
         nVXOffSet,
         nVYOffSet;

      // Get offsets between the coast point and every other point on the profile
      for (int nProfilePoint = 1; nProfilePoint < nProfileLen; nProfilePoint++)
      {
         int
            nThisX = m_VCoast[nCoast].pGetProfile(nStartProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
            nThisY = m_VCoast[nCoast].pGetProfile(nStartProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY(),
            nXOffSet = nThisX - nStartCoastX,
            nYOffSet = nThisY - nStartCoastY;

         nVXOffSet.push_back(nXOffSet);
         nVYOffSet.push_back(nYOffSet);
      }

      bool bForward = true;
      if (nDirection == DIRECTION_BACKWARD)
         bForward = false;
      int n;

      for ((bForward ? n = (nThisProfileCoastPoint+1) : n = (nNextProfileCoastPoint-1)); (bForward ? n < nNextProfileCoastPoint : n > nThisProfileCoastPoint); (bForward ? n++ : n--))
      {
         int nDist;
         if (bForward)
            nDist = n - nThisProfileCoastPoint;
         else
            nDist = nNextProfileCoastPoint - n;

//         LogStream << m_ulIter << ": profile = " << nProfile << (bForward ? " FORWARD" : " BACKWARD") << " n = " << n << " nDist = " << nDist << endl;

         double
            dThisWeight = (nDistBetween - nDist) / static_cast<double>(nDistBetween),
            dNextWeight = 1 - dThisWeight;

         // Move along the two profiles, going landwards until we hit the last inundated point
         bool bInActiveZone = false;
         int nBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(n);
         for (int nProfilePoint = (nProfileLen-1); nProfilePoint > 0; nProfilePoint--)
         {
            // Calc the x and y offset of the 'target' cell (the one to be interpolated)
            int
               nX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(n)->nGetX() + nVXOffSet[nProfilePoint-1],
               nY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(n)->nGetY() + nVYOffSet[nProfilePoint-1];

            // Safety checks
            if (! bIsWithinGrid(nX, nY))
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] outside " << m_nXGridMax << " x " << m_nYGridMax << " grid" << endl;
               continue;
            }

            if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] dry land" << endl;
               continue;
            }

//            LogStream << m_ulIter << ": [" << nX << "][" << nY << "] is inside grid, will update" << endl;

            // Now get the values for points on the 'next' and 'this' profiles
            int
               nThisProfileX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
               nThisProfileY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY(),
               nNextProfileX = m_VCoast[nCoast].pGetProfile(nProfile+1)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
               nNextProfileY = m_VCoast[nCoast].pGetProfile(nProfile+1)->PtiGetCellInProfile(nProfilePoint)->nGetY();

//             assert(bIsWithinGrid(nThisProfileX, nThisProfileY));
//             assert(bIsWithinGrid(nNextProfileX, nNextProfileY));

            // Get the wave angle and height values from the corresponding cells under the two profiles
            double
               dThisProfileWaveHeight = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveHeight(),
               dNextProfileWaveHeight = m_pRasterGrid->Cell[nNextProfileX][nNextProfileY].dGetWaveHeight(),
               dWaveHeight = (dThisWeight * dThisProfileWaveHeight) + (dNextWeight * dNextProfileWaveHeight);

            double
               dThisProfileWaveAngle = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveOrientation(),
               dNextProfileWaveAngle = m_pRasterGrid->Cell[nNextProfileX][nNextProfileY].dGetWaveOrientation(),
               dWaveOrientation = (dThisWeight * dThisProfileWaveAngle) + (dNextWeight * dNextProfileWaveAngle);

//             assert(dThisProfileWaveHeight >= 0);

            if (nDirection == DIRECTION_FORWARD)
            {
               // Just set the distance weighted values
               m_pRasterGrid->Cell[nX][nY].SetWaveHeight(dWaveHeight);
               m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(dWaveOrientation);
            }
            else
            {
               // Going backward: have we already calculated a wave height value for this cell when going forwards?
               double dTempWaveHeight = m_pRasterGrid->Cell[nX][nY].dGetWaveHeight();
//                assert(dTempWaveHeight >= 0);
//                assert(dWaveHeight >= 0);
               if (dTempWaveHeight != m_dOffshoreWaveHeight)
                  // We have, so average the two
                  dWaveHeight = (dTempWaveHeight + dWaveHeight) / 2;
               m_pRasterGrid->Cell[nX][nY].SetWaveHeight(dWaveHeight);

               // Have we already calculated a wave angle value for this cell when going forwards?
               double dTempWaveAngle = m_pRasterGrid->Cell[nX][nY].dGetWaveOrientation();
               if (dTempWaveAngle != m_dOffshoreWaveOrientation)
                  dWaveOrientation = (dTempWaveAngle + dWaveOrientation) / 2;
               m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(dWaveOrientation);
            }

            // In both directions
            if (nProfilePoint == nBreakingDist)
               bInActiveZone = true;

            if (bInActiveZone)
               m_pRasterGrid->Cell[nX][nY].SetInActiveZone(true);
         }
      }
   }

   // -----------------------------------------------------------------------------------------------------------------
   // For the first profile along the coast, also put this profile's wave properties unchanged into every cell up to the edge of the grid
   if (nProfile == 0)
   {
//      LogStream << m_ulIter << ": interpolating wave properties for cells between profile " << nProfile << " and coastline START" << endl;

      int nProfileLen = m_VCoast[nCoast].pGetProfile(nProfile)->nGetNCellsInProfile();

      // These are for the x and y offsets between the point on the profile and the point on the coast
      vector<int>
         nVXOffSet,
         nVYOffSet;

      // Get offsets between the coast point and every other point on the profile
      for (int nProfilePoint = 1; nProfilePoint < nProfileLen; nProfilePoint++)
      {
         int
            nThisX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
            nThisY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY(),
            nXOffSet = nThisX - nThisCoastX,
            nYOffSet = nThisY - nThisCoastY;

         nVXOffSet.push_back(nXOffSet);
         nVYOffSet.push_back(nYOffSet);
      }

      // Move backwards along the coast until we reach the beginning
      for (int nCoastPoint = nThisProfileCoastPoint+1; nCoastPoint > 0; nCoastPoint--)
      {
         // Again, move landwards along the profile until we hit the last inundated point
         bool bInActiveZone = false;
         int nBreakingDist = m_VCoast[nCoast].nGetBreakingDistance(nCoastPoint);
         for (int nProfilePoint = (nProfileLen-1); nProfilePoint > 0 ; nProfilePoint--)
         {
            // Calc the x and y coords for the 'target' cell
            int
               nX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetX() + nVXOffSet[nProfilePoint-1],
               nY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetY() + nVYOffSet[nProfilePoint-1];

            // Safety checks
            if (! bIsWithinGrid(nX, nY))
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] outside " << m_nXGridMax << " x " << m_nYGridMax << " grid" << endl;
               continue;
            }

            if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
            {
//               LogStream << m_ulIter << ": [" << nX << "][" << nY << "] dry land" << endl;
               continue;
            }

//            LogStream << m_ulIter << ": [" << nX << "][" << nY << "] is inside grid, will update" << endl;

            // And get the x and y coords of the point on the profile
            int
               nThisProfileX = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetX(),
               nThisProfileY = m_VCoast[nCoast].pGetProfile(nProfile)->PtiGetCellInProfile(nProfilePoint)->nGetY();

//             assert(bIsWithinGrid(nThisProfileX, nThisProfileY));

            // Get the wave angle and height values from the cells under the profile point
            double
               dProfileWaveHeight = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveHeight(),
               dProfileWaveOrientation = m_pRasterGrid->Cell[nThisProfileX][nThisProfileY].dGetWaveOrientation();

            // Check for pre-existing values
            double dOldWaveOrientation = m_pRasterGrid->Cell[nX][nY].dGetWaveOrientation();
            double dOldWaveHeight = m_pRasterGrid->Cell[nX][nY].dGetWaveHeight();
            if (dOldWaveOrientation != m_dOffshoreWaveOrientation)
               dProfileWaveOrientation = (dProfileWaveOrientation + dOldWaveOrientation) / 2;
            if (dOldWaveHeight != m_dOffshoreWaveHeight)
               dProfileWaveHeight = (dProfileWaveHeight + dOldWaveHeight) / 2;

            m_pRasterGrid->Cell[nX][nY].SetWaveHeight(dProfileWaveHeight);
            m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(dProfileWaveOrientation);

            if (nProfilePoint == nBreakingDist)
               bInActiveZone = true;

            if (bInActiveZone)
               m_pRasterGrid->Cell[nX][nY].SetInActiveZone(true);
         }
      }
   }
}
