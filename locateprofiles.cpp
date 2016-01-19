/*!
 *
 * \file locateprofiles.cpp
 * \brief Creates the profiles which are normal to the coastline
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

// TODO Calculate the spacing of the coastline normals so that they are closer where the coastline is convex, less close where it is concave

/*===============================================================================================================================

 Create profiles normal to the coastline

===============================================================================================================================*/
int CSimulation::nCreateCoastlineProfiles(void)
{
   // For each coastline, find the points along the coastline from which to start a normal profile
   for (unsigned int i = 0; i < m_VCoast.size(); i++)
   {
      // The normal profiles differ in position by a random amount each iteration
      double dSpacing = m_dCoastNormalAvgSpacing;
      if (m_dCoastNormalRandSpaceFact > 0)
      {
         // We have a random factor in the coastline normal spacing, note that dDeltaSpacing can be -ve
         double dDeltaSpacing = dGetRand0Gaussian() * m_dCoastNormalRandSpaceFact;
         dSpacing += dDeltaSpacing;

         // Make sure that this normal is at least one cell distant from the last normal
         dSpacing = tMax(dSpacing, m_dCellSide);
      }

      int nCoastSize = m_VCoast[i].nGetCoastlineSize();
      int nProfile = -1;
      int nCoastPoint = 0;
      double dTmpLen = 0;

      while (++nCoastPoint < nCoastSize)
      {
         int nCoastPointBefore = tMax(nCoastPoint-1, 0);
         dTmpLen += m_VCoast[i].dGetCoastlineSegmentLength(nCoastPoint, nCoastPointBefore);
         if (dTmpLen >= dSpacing)
         {
            // OK, create the normal profile here, at this point on the coastline. Make the start of the profile the centroid of the actual cell that is marked as coast (not the cell under the smoothed vector coast, they may well be different)
            C2DPoint PtStart;                                     // PtStart has coordinates in external CRS
            PtStart.SetX(dGridXToExtCRSX(m_VCoast[i].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetX()));
            PtStart.SetY(dGridYToExtCRSY(m_VCoast[i].pPtiGetCellMarkedAsCoastline(nCoastPoint)->nGetY()));

            C2DPoint PtEnd;
            if (nGetCoastNormalEndPoint(i, nCoastPoint, nCoastSize, &PtStart, m_dCoastNormalLength, &PtEnd) != RTN_OK)
               continue;

            // Create a new profile
            m_VCoast[i].AppendProfile(nCoastPoint);
            nProfile++;

            // Finally, create the profile's coastline-normal vector (external CRS)
            vector<C2DPoint> VNormal;
            VNormal.push_back(PtStart);
            VNormal.push_back(PtEnd);
            m_VCoast[i].pGetProfile(nProfile)->SetProfile(&VNormal);

            // Ready for next time round
            dTmpLen = 0;

//             LogStream << setiosflags(ios::fixed);
//             LogStream << m_ulIter << ": profile " << nProfile << endl;
//             LogStream << "Start is [" << PtStart.dGetX() << "][" << PtStart.dGetY() << "]" << endl;
//             LogStream << "End   is [" << PtEnd.dGetX() << "][" << PtEnd.dGetY() << "]" << endl;
//             LogStream << endl;
//         if (++nCoastPoint >= m_VCoast[i].nGetCoastlineSize()-1)
//            break;
         }
      }

      // Did we fail to create any normals? This can happen, for example if the coastline is very short, so just give warning and carry on. Even if profiles have been created, note that we have no checked whether these are valid profiles
      if (nProfile == -1)
      {
         LogStream << WARN << "iteration " << m_ulIter << ": no profiles created for coastline " << i << endl;
      }
   }

   return RTN_OK;
}

/*==============================================================================================================================

 Finds the end point of a coastline-normal line, given the start point

===============================================================================================================================*/
int CSimulation::nGetCoastNormalEndPoint(int const nCoast, int const nStartPoint, int const nCoastSize, C2DPoint* const pPtStart, double const dLineLength, C2DPoint* pPtEnd)
{
   // TODO Could use pre-calculated azimuths here maybe

   // If at start or end of coast, need special treatment for points before and points after
   int nCoastPointBefore = tMax(nStartPoint-1, 0);
   int nCoastPointAfter = tMin(nStartPoint+1, nCoastSize-1);

   // Get the y = a * x + b equation of the straight line linking the coastline points before and after 'this' coastline point
   C2DPoint PtBefore = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPointBefore);           // PtBefore has coordinates in external CRS
   C2DPoint PtAfter = *m_VCoast[nCoast].pPtGetVectorCoastlinePoint(nCoastPointAfter);             // PtAfter has coordinates in external CRS

   // For this linking line, slope a = (y2 - y1) / (x2 - x1)
   double dYDiff = PtAfter.dGetY() - PtBefore.dGetY();
   double dXDiff = PtAfter.dGetX() - PtBefore.dGetX();

   double dXEnd1 = 0, dXEnd2 = 0, dYEnd1 = 0, dYEnd2 = 0;

   if (bFPIsEqual(dYDiff, 0, TOLERANCE))
   {
      // The linking line runs W-E or E-W, so a straight line at right angles to this runs N-S or S-N. Calculate the two possible end points for this coastline-normal profile
      dXEnd1 = dXEnd2 = pPtStart->dGetX();
      dYEnd1 = pPtStart->dGetY() + dLineLength;
      dYEnd2 = pPtStart->dGetY() - dLineLength;
   }
   else if (bFPIsEqual(dXDiff, 0, TOLERANCE))
   {
      // The linking line runs N-S or S-N, so a straight line at right angles to this runs W-E or E-W. Calculate the two possible end points for this coastline-normal profile
      dYEnd1 = dYEnd2 = pPtStart->dGetY();
      dXEnd1 = pPtStart->dGetX() + dLineLength;
      dXEnd2 = pPtStart->dGetX() - dLineLength;
   }
   else
   {
      // The linking line runs neither W-E nor N-S so we have to work a bit harder to find the end-point of the coastline-normal profile
      double dA = dYDiff / dXDiff;

      // Now calculate the equation of the straight line which is perpendicular to this linking line
      double dAPerp = -1 / dA;
      double dBPerp = pPtStart->dGetY() - (dAPerp * pPtStart->dGetX());

      // Calculate the end point of the profile: first do some substitution then rearrange as a quadratic equation i.e. in the form Ax^2 + Bx + C = 0 (see http://math.stackexchange.com/questions/228841/how-do-i-calculate-the-intersections-of-a-straight-line-and-a-circle)
      double dQuadA = 1 + (dAPerp * dAPerp);
      double dQuadB = 2 * ((dBPerp * dAPerp) - (dAPerp * pPtStart->dGetY()) - pPtStart->dGetX());
      double dQuadC = ((pPtStart->dGetX() * pPtStart->dGetX()) + (pPtStart->dGetY() * pPtStart->dGetY()) + (dBPerp * dBPerp) - (2 * pPtStart->dGetY() * dBPerp) - (dLineLength * dLineLength));

      // Solve for x and y using the quadratic formula x = (−B ± sqrt(B^2 − 4AC)) / 2A
      double dDiscriminant = (dQuadB * dQuadB) - (4 * dQuadA * dQuadC);
      if (dDiscriminant < 0)
      {
         LogStream << ERR << "iteration " << m_ulIter << ": discriminant < 0 when finding profile end point on coastline " << nCoast << ", from coastline point " << nStartPoint << "), ignored" << endl;
         return RTN_ERR_BADENDPOINT;
      }

      dXEnd1 = (-dQuadB + sqrt(dDiscriminant)) / (2 * dQuadA);
      dYEnd1 = (dAPerp * dXEnd1) + dBPerp;
      dXEnd2 = (-dQuadB - sqrt(dDiscriminant)) / (2 * dQuadA);
      dYEnd2 = (dAPerp * dXEnd2) + dBPerp;
   }

   // We have two possible solutions, so decide which of the two endpoints to use then create the profile end-point (coordinates in external CRS)
   bool bSeaHand = m_VCoast[nCoast].nGetSeaHandedness();            // Assumes handedness is either 0 or 1 (i.e. not -1)
   *pPtEnd = PtChooseEndPoint(bSeaHand, &PtBefore, &PtAfter, dXEnd1, dYEnd1, dXEnd2, dYEnd2);

   // Check that pPtEnd is not off the grid
   if (! bIsWithinGrid( static_cast<int>(dRound(dExtCRSXToGridX(pPtEnd->dGetX()))), static_cast<int>(dRound(dExtCRSYToGridY(pPtEnd->dGetY())))))
   {
//      LogStream << WARN << "iteration " << m_ulIter << ": profile endpoint is outside grid for coastline " << nCoast << ",  from coastline point " << nStartPoint << "), ignored" << endl;
      return RTN_ERR_OFFGRIDENDPOINT;
   }

   return RTN_OK;
}

/*==============================================================================================================================

 Choose which end point to use for the coastline-normal profile

===============================================================================================================================*/
C2DPoint CSimulation::PtChooseEndPoint(bool const bHand, C2DPoint* const PtBefore, C2DPoint* const PtAfter, double const dXEnd1, double const dYEnd1, double const dXEnd2, double const dYEnd2)
{
   C2DPoint PtChosen;

   if (bHand == RIGHT_HANDED)
   {
      // The sea is to the right of the linking line. So which way is the linking line oriented? First check the N-S component
      if (PtAfter->dGetY() > PtBefore->dGetY())
      {
         // We are going N to S and the sea is to the right, so the normal endpoint is to the E
         if (dXEnd1 > dXEnd2)
         {
            PtChosen.SetX(dXEnd1);
            PtChosen.SetY(dYEnd1);
         }
         else
         {
            PtChosen.SetX(dXEnd2);
            PtChosen.SetY(dYEnd2);
         }
      }
      else if (bFPIsEqual(PtAfter->dGetY(), PtBefore->dGetY(), TOLERANCE))
//      else if (PtAfter->dGetY() == PtBefore->dGetY())
      {
         // No N-S component i.e. the linking line is exactly W-E. So check the W-E component
         if (PtAfter->dGetX() > PtBefore->dGetX())
         {
            // We are going E to W and the sea is to the right, so the normal endpoint is to the N (note that the origin of the grid is to the top left)
            if (dYEnd1 < dYEnd2)
            {
               PtChosen.SetX(dXEnd1);
               PtChosen.SetY(dYEnd1);
            }
            else
            {
               PtChosen.SetX(dXEnd2);
               PtChosen.SetY(dYEnd2);
            }
         }
         else     // Do not check for (PtAfter->dGetX() == PtBefore->dGetX()), since this would mean the two points are co-incident
         {
            // We are going W to E and the sea is to the right, so the normal endpoint is to the S (note that the origin of the grid is to the top left)
            if (dYEnd1 > dYEnd2)
            {
               PtChosen.SetX(dXEnd1);
               PtChosen.SetY(dYEnd1);
            }
            else
            {
               PtChosen.SetX(dXEnd2);
               PtChosen.SetY(dYEnd2);
            }
         }
      }
      else
      {
         // We are going S to N and the sea is to the right, so the normal endpoint is to the W
         if (dXEnd1 < dXEnd2)
         {
            PtChosen.SetX(dXEnd1);
            PtChosen.SetY(dYEnd1);
         }
         else
         {
            PtChosen.SetX(dXEnd2);
            PtChosen.SetY(dYEnd2);
         }
      }
   }
   else
   {
      // The sea is to the left of the linking line. So which way is the linking line oriented? First check the N-S component
      if (PtAfter->dGetY() > PtBefore->dGetY())
      {
         // We are going N to S and the sea is to the left, so the normal endpoint is to the W
         if (dXEnd1 < dXEnd2)
         {
            PtChosen.SetX(dXEnd1);
            PtChosen.SetY(dYEnd1);
         }
         else
         {
            PtChosen.SetX(dXEnd2);
            PtChosen.SetY(dYEnd2);
         }
      }
      else if (bFPIsEqual(PtAfter->dGetY(), PtBefore->dGetY(), TOLERANCE))
//      else if (PtAfter->dGetY() == PtBefore->dGetY())
      {
         // No N-S component i.e. the linking line is exactly W-E. So check the W-E component
         if (PtAfter->dGetX() > PtBefore->dGetX())
         {
            // We are going E to W and the sea is to the left, so the normal endpoint is to the S (note that the origin of the grid is to the top left)
            if (dYEnd1 > dYEnd2)
            {
               PtChosen.SetX(dXEnd1);
               PtChosen.SetY(dYEnd1);
            }
            else
            {
               PtChosen.SetX(dXEnd2);
               PtChosen.SetY(dYEnd2);
            }
         }
         else     // Do not check for (PtAfter->dGetX() == PtBefore->dGetX()), since this would mean the two points are co-incident
         {
            // We are going W to E and the sea is to the left, so the normal endpoint is to the N (note that the origin of the grid is to the top left)
            if (dYEnd1 < dYEnd2)
            {
               PtChosen.SetX(dXEnd1);
               PtChosen.SetY(dYEnd1);
            }
            else
            {
               PtChosen.SetX(dXEnd2);
               PtChosen.SetY(dYEnd2);
            }
         }
      }
      else
      {
         // We are going S to N and the sea is to the left, so the normal endpoint is to the E
         if (dXEnd1 > dXEnd2)
         {
            PtChosen.SetX(dXEnd1);
            PtChosen.SetY(dYEnd1);
         }
         else
         {
            PtChosen.SetX(dXEnd2);
            PtChosen.SetY(dYEnd2);
         }
      }
   }

   return PtChosen;
}

/*==============================================================================================================================

 Puts the coastline-normal profiles onto the raster grid, i.e. rasterizes multi-line vector objects onto the raster grid. Note that this doesn't work if the vector has already been interpolated to fit on the grid i.e. if distances between vector points are just one cell apart

===============================================================================================================================*/
int CSimulation::nAllCoastlineNormalProfilesToGrid(void)
{
   int nValidProfiles = 0;

   // Do once for every coastline object
   for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
   {
      // How many profiles on this coast?
      int nProfiles = m_VCoast[i].nGetNumProfiles();
      if (nProfiles < 1)
      {
         // This can happen if the coastline is very short, so just give a warning and carry on with the next coastline segment
         LogStream << WARN << "interation " << m_ulIter << ": coastline " << i << " has no profiles" << endl;
         continue;
      }

      // Now do this loop for every profile in the list
      vector<int> nVInvalid;
      for (int j = 0; j < nProfiles; j++)
      {
         int nPoints = m_VCoast[i].pGetProfile(j)->nGetNumVecPointsInProfile();
         if (nPoints < 2)
         {
            // Need at least two points in the profile
            cerr << ERR << "iteration " << m_ulIter << ": profile " << j << " of coastline " << i << " is too short to rasterize: at least two points needed" << endl;
            return RTN_ERR_LINETOGRID;
         }

         // All is OK, so go for it: set up temporary vectors to hold the x-y coords (in grid CRS) of the cells which we will mark
         vector<C2DIPoint> VCellsToMark;

         int nRet = nRasterizeCoastlineNormalProfile(m_VCoast[i].pGetProfile(j)->pVGetPoints(), &VCellsToMark);
         if (nRet == RTN_OK)
         {
            // This profile is fine
            nValidProfiles++;

            for (unsigned int k = 0; k < VCellsToMark.size(); k++)
            {
               // So mark each cell in the raster grid
               m_pRasterGrid->Cell[VCellsToMark[k].nGetX()][VCellsToMark[k].nGetY()].SetAsNormalProfile(true);

               // Store the raster-grid coordinates in the profile object
               m_VCoast[i].pGetProfile(j)->SetCellInProfile(VCellsToMark[k].nGetX(), VCellsToMark[k].nGetY());

               // And also store the external coordinates in the profile object
               m_VCoast[i].pGetProfile(j)->SetCellInProfileExtCRS(dGridXToExtCRSX(VCellsToMark[k].nGetX()), dGridYToExtCRSY(VCellsToMark[k].nGetY()));
            }
         }
         else
         {
            // This profile is invalid, so mark it and remove once we are out of the loop
            nVInvalid.push_back(j);
         }
      }

      // Any invalid profiles?
      int nInvalid = nVInvalid.size();
      if (nInvalid > 0)
      {
         // Yes, so remove the invalid profiles. Have to do this in reverse sequence since a vector iterator becomes invalid for items after the removed item
         for (int n = nInvalid-1; n >= 0; n--)
            m_VCoast[i].RemoveProfile(nVInvalid[n]);
      }
   }

   if (nValidProfiles == 0)
   {
      // Problem! No valid profiles. However, carry on
      cerr << WARN << "iteration " << m_ulIter << ": no valid profiles" << endl;
   }

   return RTN_OK;
}

/*==============================================================================================================================

 Rasterizes a coastline-normal profile i.e. returns an output vector of cells which are 'under' the vector line. The profile is removed if a rasterized cell is dry land or coast

===============================================================================================================================*/
int CSimulation::nRasterizeCoastlineNormalProfile(vector<C2DPoint>* const pVPointsIn, vector<C2DIPoint>* pVIPointsOut)
{
   int nX, nY;

   pVIPointsOut->clear();

   for (unsigned int k = 0; k < (pVPointsIn->size() - 1); k++)
   {
      // In rastergrid coordinates. They can't be unsigned, since they could be -ve i.e. off the grid
      int nX1 = nExtCRSXToGridX(pVPointsIn->at(k).dGetX());
      int nY1 = nExtCRSYToGridY(pVPointsIn->at(k).dGetY());
      int nX2 = nExtCRSXToGridX(pVPointsIn->at(k+1).dGetX());
      int nY2 = nExtCRSYToGridY(pVPointsIn->at(k+1).dGetY());

      // Interpolate between cells by a simple DDA line algorithm, see http://en.wikipedia.org/wiki/Digital_differential_analyzer_(graphics_algorithm) Note that Bresenham's algorithm gave occasional gaps
      int nLength = tMax(tAbs(nX1 - nX2), tAbs(nY2 - nY1));
      double dXInc = static_cast<double>(nX2 - nX1) / static_cast<double>(nLength);
      double dYInc = static_cast<double>(nY2 - nY1) / static_cast<double>(nLength);
      double dX = nX1;
      double dY = nY1;

      // Process each interpolated point
      for (int m = 0; m < nLength; m++)
      {
         nX = static_cast<int>(dRound(dX));
         nY = static_cast<int>(dRound(dY));

         // Make sure the interpolated point is within the raster grid
         if (! bIsWithinGrid(nX, nY))
         {
            // It isn't, so remove this profile point
//             LogStream << m_ulIter << ": in nRasterizeCoastlineNormalProfile(), REMOVING [" << nX << "][" << nY << "] since is outside [" << m_nXGridMax << "][" << m_nYGridMax << "] grid" << endl;
             return RTN_ERR_LINETOGRID;
         }

         // Once we are clear of the coastline (i.e. when m > 0), check if this profile hits land (NOTE used to get problems here since if the coastline vector has been heavily smoothed, this can result is 'false positives' profiles marked as invalid which are not actually invalid, because the profile hits land when m = 0. This results in some cells being flagged as profile cells which are actually inland)
         if (m > 0)
         {
            if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
            {
               // We've hit dry land so remove this profile point
//               LogStream << m_ulIter << ": in nRasterizeCoastlineNormalProfile(), REMOVING [" << nX << "][" << nY << "] since has hit land" << endl;
               return RTN_ERR_LINETOGRID;
            }

            if (m_pRasterGrid->Cell[nX][nY].bIsCoastline())
            {
               // We've hit a coastline so remove this profile point
//               LogStream << m_ulIter << ": in nRasterizeCoastlineNormalProfile(), REMOVING [" << nX << "][" << nY << "] since has hit coastline" << endl;
               return RTN_ERR_LINETOGRID;
            }
         }

         // This point is fine, so add it to the output vector
         pVIPointsOut->push_back(C2DIPoint(nX, nY));         // In raster-grid co-ordinates

         // And increment for next time
         dX += dXInc;
         dY += dYInc;
      }
   }

   // How long is the output vector?
   if (pVIPointsOut->size() < 3)
   {
      // For shore platform coastline-normal profiles, we cannot have very short normal profiles, with less than 3 cells (since we cannot calculate along-profile slope properly for such short profiles)
      return RTN_ERR_LINETOGRID;
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Checks all coastline-normal profiles for intersection

===============================================================================================================================*/
int CSimulation::nCheckAllProfilesForIntersection(void)
{
   // Do once for every coastline object
   for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
   {
      // Do once for every profile except the last
      for (int j = 0; j < m_VCoast[i].nGetNumProfiles() - 1; j++)
      {
         double
            dIntersectX = 0,
            dIntersectY = 0;
         bool bIntersect = bCheckForIntersection(m_VCoast[i].pGetProfile(j)->pVGetPoints(), m_VCoast[i].pGetProfile(j+1)->pVGetPoints(), dIntersectX, dIntersectY);

         if (bIntersect)
            LogStream << m_ulIter << ": coast = " << i << " profiles = " << j << " and " << j+1 << " INTERSECT at " << dIntersectX << ", " << dIntersectY << endl;
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Checks a pair of coastline-normal profiles for intersection. From Andre LeMothe's "Tricks of the Windows Game Programming Gurus" via http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect

===============================================================================================================================*/
bool CSimulation::bCheckForIntersection(vector<C2DPoint>* const pVProfile1, vector<C2DPoint>* const pVProfile2, double& dX, double& dY)
{
   // Returns true if the lines intersect. If the lines do intersect then the intersection point is returned in dX and dY

   if ((pVProfile1->size() > 2) || (pVProfile2->size() > 2))
   {
      LogStream << "ERROR, TOO LONG" << endl;
      return false;
   }

   // In external coordinates
   double
      dX1 = pVProfile1->at(0).dGetX(),
      dY1 = pVProfile1->at(0).dGetY(),
      dX2 = pVProfile1->at(1).dGetX(),
      dY2 = pVProfile1->at(1).dGetY();

   double
      dX3 = pVProfile2->at(0).dGetX(),
      dY3 = pVProfile2->at(0).dGetY(),
      dX4 = pVProfile2->at(1).dGetX(),
      dY4 = pVProfile2->at(1).dGetY();

   // Uses Cramer's Rule to solve the equations
    double
      dDiffX1 = dX2 - dX1,
      dDiffY1 = dY2 - dY1,
      dDiffX2 = dX4 - dX3,
      dDiffY2 = dY4 - dY3;

    double
      dS = 0,
      dT = 0,
      dTmp = 0;

    dTmp = -dDiffX2 * dDiffY1 + dDiffX1 * dDiffY2;
    if (dTmp != 0)
      dS = (-dDiffY1 * (dX1 - dX3) + dDiffX1 * (dY1 - dY3)) / dTmp;

    dTmp = -dDiffX2 * dDiffY1 + dDiffX1 * dDiffY2;
    if (dTmp != 0)
      dT = (dDiffX2 * (dY1 - dY3) - dDiffY2 * (dX1 - dX3)) / dTmp;

    if (dS >= 0 && dS <= 1 && dT >= 0 && dT <= 1)
    {
      // Collision detected
      dX = dX1 + (dT * dDiffX1);
      dY = dY1 + (dT * dDiffY1);

      return true;
    }

    // No collision
    return false;
}
