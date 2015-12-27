/*!
 *
 * \file locatecoast.cpp
 * \brief Finds the coastline on the raster grid
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
#include "iline.h"
#include "cliff.h"
#include "simulation.h"

/*===============================================================================================================================

 Locate the vector coastline(s), set up the vector coastline-normal profiles, and put both these onto the raster grid

===============================================================================================================================*/
int CSimulation::nLocateCoastlineAndProfiles(void)
{
   // First, remove any existing coastline(s) from the raster grid, and clear the vector coastline(s) and all associated vector coastline-normal profiles
   ClearRasterAndVectorCoastlines();

   // Find the coastline on the raster grid, mark raster cells, then create the vector coastline
   int nRet = nTraceCoastline();
   if (nRet != RTN_OK)
      return nRet;

   // Assign a coastal landform object to every point on the coastline
   nRet = nAssignCoastalLandforms();
   if (nRet != RTN_OK)
      return nRet;

   // Create the vector coastline normal profiles
   nRet = nCreateCoastlineProfiles();
   if (nRet != RTN_OK)
      return nRet;

   // DFM TEST Check if the profiles intersect
   nRet = nCheckAllProfilesForIntersection();
   if (nRet != RTN_OK)
      return nRet;

   // Put normal profiles onto the raster grid, but only if the profile is long enough, and doesn't cross the coastline or hit dry land
   nRet = nAllCoastlineNormalProfilesToGrid();
   if (nRet != RTN_OK)
      return nRet;

   return RTN_OK;
}

/*===============================================================================================================================

 Remove any pre-existing coastline(s) and coastline-normal profiles from the raster grid, also clear the vector coastline and profiles

===============================================================================================================================*/
void CSimulation::ClearRasterAndVectorCoastlines(void)
{
   // TODO Change this so that coastline grid cells are stored in a C2DPoint vector, then just 'walk' this vector to clear only the coastline grid cells, rather than go through the whole grid
   for (int nX = 0; nX < m_nXGridMax; nX++)
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         m_pRasterGrid->Cell[nX][nY].SetAsCoastline(false);
         m_pRasterGrid->Cell[nX][nY].SetAsNormalProfile(false);
      }

   // Clear all vector coastlines and profiles
   m_VCoast.clear();
}

/*===============================================================================================================================

 This locates coastline start and finish points on the edges of the raster grid, does some consistency checking, then uses these start and finish points to trace the coastline(s). Coastlines may then be smoothed

===============================================================================================================================*/
int CSimulation::nTraceCoastline(void)
{
   // Vectors to hold multiple coast edge points i.e. points where the vector coastline will cross the edge of the raster grid. These points (and the coastline itself) are assumed to be just above the still water level
   vector<bool> VbMatched;
   vector<int> VnMatchedWith;
   vector<int> VnHandedness;
   vector<int> VnSearchDirection;
   vector<int> VnStartEdge;
   vector<C2DIPoint> VPtiEdgePoint;

   // DFM TEST RANDOM Do the grid-edge search in a different sequence each time, to see if this makes any difference. If it doesn't then eventually remove it
   int nDirections[] = {ORIENTATION_NORTH, ORIENTATION_EAST, ORIENTATION_SOUTH, ORIENTATION_WEST};

   if (m_bRandomCoastEdgeSearch)
      Rand1Shuffle(nDirections, 4);

/*   cout << "-----------------------" << endl;
   cout << "Iteration " << m_ulIter << " edge search order is ";
   for (int i = 0; i < 4; i++)
      cout << (nDirections[i] == 1 ? "NORTH" : "") << (nDirections[i] == 2 ? "EAST" : "") << (nDirections[i] == 3 ? "SOUTH" : "") << (nDirections[i] == 4 ? "WEST" : "") << " ";
   cout << endl; */

   for (int i = 0; i < 4; i++)
      FindEdgePoints(nDirections[i], &VbMatched, &VnMatchedWith, &VnHandedness, &VnSearchDirection, &VnStartEdge, &VPtiEdgePoint);

   if (! bCheckEdgePoints(&VPtiEdgePoint))
      return RTN_ERR_FINDCOAST;

   if (! bDoTracing(&VbMatched, &VnMatchedWith, &VnHandedness, &VnSearchDirection, &VnStartEdge, &VPtiEdgePoint))
      return RTN_ERR_FINDCOAST;

   return RTN_OK;
}


/*==============================================================================================================================

 Searches a single grid edge for coastline start points (defined to be just above still water level)

===============================================================================================================================*/
void CSimulation::FindEdgePoints(int const nEdgeToSearch, vector<bool>* VbMatched, vector <int>* VnMatchedWith, vector<int>* VnHandedness, vector<int>* VnSearchDirection, vector<int>* VnStartEdge, vector<C2DIPoint>* VPtiEdgePoint)
{
   int
      nX,
      nY,
      nXLast,
      nYLast,
      nStart,
      nEnd,
      nHandDH,
      nHandUH,
      nSearchDirection,
      nStartEdge;
   double
      dLastElev = DBL_MAX;

   switch (nEdgeToSearch)
   {
   case ORIENTATION_WEST:
      // Search the LH (W) edge, moving N to S along this edge for cells which are just above the still water level (note that the origin of the grid is at top left)
      nX = nXLast = 0;
      nStart = 0;
      nEnd = m_nYGridMax;
      nHandDH = LEFT_HANDED;
      nHandUH = RIGHT_HANDED;
      nSearchDirection = ORIENTATION_EAST;
      nStartEdge = ORIENTATION_WEST;
      break;

   case ORIENTATION_NORTH:
      // Search the top (N) edge, from W to E (miss out corners)
      nY = nYLast = 0;
      nStart = 1;
      nEnd = m_nXGridMax-1;
      nHandDH = RIGHT_HANDED;
      nHandUH = LEFT_HANDED;
      nSearchDirection = ORIENTATION_SOUTH;
      nStartEdge = ORIENTATION_NORTH;
      break;

   case ORIENTATION_EAST:
      // Search the RH (E) edge, search N to S
      nX = nXLast = m_nXGridMax-1;
      nStart = 0;
      nEnd = m_nYGridMax;
      nHandDH = RIGHT_HANDED;
      nHandUH = LEFT_HANDED;
      nSearchDirection = ORIENTATION_WEST;
      nStartEdge = ORIENTATION_EAST;
      break;

   case ORIENTATION_SOUTH:
      // Search the bottom (S) edge, search W to E (miss out corners)
      nY = nYLast = m_nYGridMax-1;
      nStart = 1;
      nEnd = m_nXGridMax-1;
      nHandDH = LEFT_HANDED;
      nHandUH = RIGHT_HANDED;
      nSearchDirection = ORIENTATION_NORTH;
      nStartEdge = ORIENTATION_SOUTH;
      break;
   }

   // Now do the search
   for (int n = nStart; n < nEnd; n++)
   {
      switch (nEdgeToSearch)
      {
      case ORIENTATION_WEST:
      case ORIENTATION_EAST:
         nY = n;
         break;

      case ORIENTATION_NORTH:
      case ORIENTATION_SOUTH:
         nX = n;
         break;
      }

      double dThisElev = m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev();
      if ((dLastElev != DBL_MAX) && (dLastElev <= m_dThisIterStillWaterLevel) && (dThisElev > m_dThisIterStillWaterLevel))
      {
         // Found dry land while going uphill
         C2DIPoint Pti(nX, nY);
         if (find(VPtiEdgePoint->begin(), VPtiEdgePoint->end(), Pti) == VPtiEdgePoint->end())
         {
            // Not already found, so add it
//            cout << "Uphill: COAST AT [nX][nY] is [" << nX << "][" << nY << "] with dThisElev = " << dThisElev << ", [nXLast][nYLast] is [" << nXLast << "][" << nYLast << "] with dLastElev = " << dLastElev << endl;
            VPtiEdgePoint->push_back(Pti);
            VnHandedness->push_back(nHandDH);
            VnSearchDirection->push_back(nSearchDirection);
            VnStartEdge->push_back(nStartEdge);
            VbMatched->push_back(false);
            VnMatchedWith->push_back(-1);
         }
      }
      else if ((dLastElev != DBL_MAX) && (dLastElev > m_dThisIterStillWaterLevel) && (dThisElev <= m_dThisIterStillWaterLevel))
      {
         // Found dry land while going downhill
         C2DIPoint Pti(nXLast, nYLast);
         if (find(VPtiEdgePoint->begin(), VPtiEdgePoint->end(), Pti) == VPtiEdgePoint->end())
         {
            // Not already found, so add it
//            cout << "Downhill: [nX][nY] is [" << nX << "][" << nY << "] with dThisElev = " << dThisElev << ", COAST AT [nXLast][nYLast] is [" << nXLast << "][" << nYLast << "] with dLastElev = " << dLastElev << endl;
            VPtiEdgePoint->push_back(Pti);
            VnHandedness->push_back(nHandUH);
            VnSearchDirection->push_back(nSearchDirection);
            VnStartEdge->push_back(nStartEdge);
            VbMatched->push_back(false);
            VnMatchedWith->push_back(-1);
         }
      }

      dLastElev = dThisElev;
      nXLast = nX;
      nYLast = nY;
   }
}


/*==============================================================================================================================

 Do some sanity checking of the coastline endpoints

===============================================================================================================================*/
bool CSimulation::bCheckEdgePoints(vector<C2DIPoint>* VPtiEdgePoint)
{
   unsigned int nSize = VPtiEdgePoint->size();

   // Did we find any coastline endpoints?
   if (nSize == 0)
   {
      // Did not find any coastline endpoints after searching all four edges of the grid
      cerr << ERR << " no coastline endpoints found, is the still water level too high?" << endl;
      return false;
   }

   // Do we have an odd number of coastline endpoints?
   if (nSize % 2)
   {
      // Odd number, give warning
      LogStream << setiosflags(ios::fixed);
      LogStream << WARN << "iteration " << m_ulIter << ": odd number (" << nSize << ") of coastline endpoints found" << endl;
      LogStream << "EndPoints are:" << endl;
      for (unsigned int j = 0; j < nSize; j++)
         LogStream << j << " [" << VPtiEdgePoint->at(j).nGetX() << "][" << VPtiEdgePoint->at(j).nGetY() << "]" << endl;
      LogStream << "Grid size is " << m_nXGridMax << " x " << m_nYGridMax << endl;
      LogStream << "---------------------" << endl;
   }

/*   cout << endl << "Iteration " << m_ulIter << " at end of bCheckEdgePoints(), VPtiEdgePoint->size() = " << VPtiEdgePoint->size() << endl;
   for (unsigned int m = 0; m < VPtiEdgePoint->size(); m++)
   {
      cout << "[" << VPtiEdgePoint->at(m).dGetX() << "][" << VPtiEdgePoint->at(m).dGetY() << "]" << endl;
   }
   cout << endl;
   cout.flush(); */

   return true;
}

/*==============================================================================================================================

 This routine traces the coastline (which is defined to be just above still water level) on the grid using the 'wall follower' rule for maze traversal (http://en.wikipedia.org/wiki/Maze_solving_algorithm#Wall_follower)

===============================================================================================================================*/
bool CSimulation::bDoTracing(vector<bool>* VbMatched, vector <int>* VnMatchedWith, vector<int>* VnHandedness, vector<int>* VnSearchDirection, vector<int>* VnStartEdge, vector<C2DIPoint>* VPtiEdgePoint)
{
   bool bAtCoast = false;
   int nX, nY;
   int nThisCoast = -1;                      // Index to coastlines searched
   int nValidCoast = -1;                     // Index to the number of valid coastlines found
   int nThisEdgePoint = 0;
   CILine LTempGridCRS;                      // Temporary coastline, integers line (in grid coordinates)

   do
   {
      // Do this for every coastline edge point that hasn't already been matched up with another edgepoint
      if (VbMatched->at(nThisEdgePoint))
      {
         nThisEdgePoint++;
         continue;
      }

      // OK we will try to trace a coastline from this endpoint
      nX = VPtiEdgePoint->at(nThisEdgePoint).nGetX();
      nY = VPtiEdgePoint->at(nThisEdgePoint).nGetY();

      int
         nSearchDirection = VnSearchDirection->at(nThisEdgePoint),
         nHandedness = VnHandedness->at(nThisEdgePoint),
         nStartX = VPtiEdgePoint->at(nThisEdgePoint).nGetX(),
         nStartY = VPtiEdgePoint->at(nThisEdgePoint).nGetY();

      // Clear the temporary coastline
      LTempGridCRS.Clear();
      nThisCoast++;

      // ========================================================================================================================
      // Start at this endpoint and trace the rest of the coastline using the 'wall follower' rule for maze traversal
      bool
         bHasLeftStartEdge = false,
         bSearchedTooLong = false;
      int
         nRoundTheLoop = 0;
      do
      {
         // Safety device
         if (++nRoundTheLoop > ROUNDLOOPMAX)
         {
            bSearchedTooLong = true;
            break;
         }

         // Have we left the start edge?
         if (! bHasLeftStartEdge)
         {
            if (((nStartX == 0) && (nX > 0)) ||
                ((nStartX == (m_nXGridMax-1)) && (nX < m_nXGridMax-1)) ||
                ((nStartY == 0) && (nY > 0)) ||
                ((nStartY == (m_nYGridMax-1)) && (nY < m_nYGridMax-1)))
               bHasLeftStartEdge = true;
         }

         // Leave the loop if the vector coastline has left the start edge, then we find a coast cell which is at an edge (note that this edge could be the same edge from which this coastline started)
         if (bHasLeftStartEdge && bAtCoast)
         {
            if ((nX <= 0) || (nX >= m_nXGridMax-1) || (nY <= 0) || (nY >= m_nYGridMax-1))
               break;
         }

         // A sanity check: has the coastline become too long?
         if (LTempGridCRS.nGetSize() > m_nCoastMax)
         {
            // We have a problem, the vector coastline is unreasonably big
            LogStream << ERR << "iteration " << m_ulIter << ": size of temporary coastline " << nThisCoast << " (" << LTempGridCRS.nGetSize() << ") exceeds maximum (" << m_nCoastMax << ")" << endl;
            return RTN_ERR_FINDCOAST;
         }

         // OK now sort out the next iteration of the search
         bAtCoast = false;
         KeepWithinGrid(nX, nY);       // Safety check, is rarely needed but keep it anyway

         int
            nXSeaward,
            nYSeaward,
            nSeawardNewDirection,
            nXStraightOn,
            nYStraightOn,
            nXAntiSeaward,
            nYAntiSeaward,
            nAntiSeawardNewDirection,
            nXGoBack,
            nYGoBack,
            nGoBackNewDirection;

         C2DIPoint Pti(nX, nY);

         // Set up the variables
         switch (nHandedness)
         {
            case RIGHT_HANDED:
               // The sea is to the right-hand side of the coast as we traverse it, so we need to keep heading right to find the coast
               switch (nSearchDirection)
               {
                  case ORIENTATION_NORTH:
                     // The sea is towards the RHS (E) of the coast, so first try to go right (to the E)
                     nXSeaward = nX+1;
                     nYSeaward = nY;
                     nSeawardNewDirection = ORIENTATION_EAST;

                     // If can't do this, try to go straight on (to the N)
                     nXStraightOn = nX;
                     nYStraightOn = nY-1;

                     // If can't do either of these, try to go anti-seaward i.e. towards the LHS (W)
                     nXAntiSeaward = nX-1;
                     nYAntiSeaward = nY;
                     nAntiSeawardNewDirection = ORIENTATION_WEST;

                     // As a last resort, go back (to the S)
                     nXGoBack = nX;
                     nYGoBack = nY+1;
                     nGoBackNewDirection = ORIENTATION_SOUTH;

                     break;

                  case ORIENTATION_EAST:
                     // The sea is towards the RHS (S) of the coast, so first try to go right (to the S)
                     nXSeaward = nX;
                     nYSeaward = nY+1;
                     nSeawardNewDirection = ORIENTATION_SOUTH;

                     // If can't do this, try to go straight on (to the E)
                     nXStraightOn = nX+1;
                     nYStraightOn = nY;

                     // If can't do either of these, try to go anti-seaward i.e. towards the LHS (N)
                     nXAntiSeaward = nX;
                     nYAntiSeaward = nY-1;
                     nAntiSeawardNewDirection = ORIENTATION_NORTH;

                     // As a last resort, go back (to the W)
                     nXGoBack = nX-1;
                     nYGoBack = nY;
                     nGoBackNewDirection = ORIENTATION_WEST;

                     break;

                  case ORIENTATION_SOUTH:
                     // The sea is towards the RHS (W) of the coast, so first try to go right (to the W)
                     nXSeaward = nX-1;
                     nYSeaward = nY;
                     nSeawardNewDirection = ORIENTATION_WEST;

                     // If can't do this, try to go straight on (to the S)
                     nXStraightOn = nX;
                     nYStraightOn = nY+1;

                     // If can't do either of these, try to go anti-seaward i.e. towards the LHS (E)
                     nXAntiSeaward = nX+1;
                     nYAntiSeaward = nY;
                     nAntiSeawardNewDirection = ORIENTATION_EAST;

                     // As a last resort, go back (to the N)
                     nXGoBack = nX;
                     nYGoBack = nY-1;
                     nGoBackNewDirection = ORIENTATION_NORTH;

                     break;

                  case ORIENTATION_WEST :
                     // The sea is towards the RHS (N) of the coast, so first try to go right (to the N)
                     nXSeaward = nX;
                     nYSeaward = nY-1;
                     nSeawardNewDirection = ORIENTATION_NORTH;

                     // If can't do this, try to go straight on (to the W)
                     nXStraightOn = nX-1;
                     nYStraightOn = nY;

                     // If can't do either of these, try to go anti-seaward i.e. towards the LHS (S)
                     nXAntiSeaward = nX;
                     nYAntiSeaward = nY+1;
                     nAntiSeawardNewDirection = ORIENTATION_SOUTH;

                     // As a last resort, go back (to the E)
                     nXGoBack = nX+1;
                     nYGoBack = nY;
                     nGoBackNewDirection = ORIENTATION_EAST;

                     break;
               }
               break;

            case LEFT_HANDED:
               // The sea is to the left-hand side of the coast as we traverse it, so we need to keep heading left to find the coast
               switch (nSearchDirection)
               {
                  case ORIENTATION_NORTH:
                     // The sea is towards the LHS (W) of the coast, so first try to go left (to the W)
                     nXSeaward = nX-1;
                     nYSeaward = nY;
                     nSeawardNewDirection = ORIENTATION_WEST;

                     // If can't do this, try to go straight on (to the N)
                     nXStraightOn = nX;
                     nYStraightOn = nY-1;

                     // If can't do either of these, try to go anti-seaward i.e. towards the RHS (E)
                     nXAntiSeaward = nX+1;
                     nYAntiSeaward = nY;
                     nAntiSeawardNewDirection = ORIENTATION_EAST;

                     // As a last resort, go back (to the S)
                     nXGoBack = nX;
                     nYGoBack = nY+1;
                     nGoBackNewDirection = ORIENTATION_SOUTH;

                     break;

                  case ORIENTATION_EAST :
                     // The sea is towards the LHS (N) of the coast, so first try to go left (to the N)
                     nXSeaward = nX;
                     nYSeaward = nY-1;
                     nSeawardNewDirection = ORIENTATION_NORTH;

                     // If can't do this, try to go straight on (to the E)
                     nXStraightOn = nX+1;
                     nYStraightOn = nY;

                     // If can't do either of these, try to go anti-seaward i.e. towards the RHS (S)
                     nXAntiSeaward = nX;
                     nYAntiSeaward = nY+1;
                     nAntiSeawardNewDirection = ORIENTATION_SOUTH;

                     // As a last resort, go back (to the W)
                     nXGoBack = nX-1;
                     nYGoBack = nY;
                     nGoBackNewDirection = ORIENTATION_WEST;

                     break;

                  case ORIENTATION_SOUTH:
                     // The sea is towards the LHS (E) of the coast, so first try to go left (to the E)
                     nXSeaward = nX+1;
                     nYSeaward = nY;
                     nSeawardNewDirection = ORIENTATION_EAST;

                     // If can't do this, try to go straight on (to the S)
                     nXStraightOn = nX;
                     nYStraightOn = nY+1;

                     // If can't do either of these, try to go anti-seaward i.e. towards the RHS (W)
                     nXAntiSeaward = nX-1;
                     nYAntiSeaward = nY;
                     nAntiSeawardNewDirection = ORIENTATION_WEST;

                     // As a last resort, go back (to the N)
                     nXGoBack = nX;
                     nYGoBack = nY-1;
                     nGoBackNewDirection = ORIENTATION_NORTH;

                     break;

                  case ORIENTATION_WEST :
                     // The sea is towards the LHS (S) of the coast, so first try to go left (to the S)
                     nXSeaward = nX;
                     nYSeaward = nY+1;
                     nSeawardNewDirection = ORIENTATION_SOUTH;

                     // If can't do this, try to go straight on (to the W)
                     nXStraightOn = nX-1;
                     nYStraightOn = nY;

                     // If can't do either of these, try to go anti-seaward i.e. towards the RHS (N)
                     nXAntiSeaward = nX;
                     nYAntiSeaward = nY-1;
                     nAntiSeawardNewDirection = ORIENTATION_NORTH;

                     // As a last resort, go back (to the E)
                     nXGoBack = nX+1;
                     nYGoBack = nY;
                     nGoBackNewDirection = ORIENTATION_EAST;

                     break;
               }
               break;
         }

//         cout << "Iteration " << m_ulIter << ", before search nX = " << nX << ", nY = " << nY << endl;

         // Now do the actual search for this iteration: first try going in the direction of the sea. Is this seaward cell still within the grid?
         if (bIsWithinGrid(nXSeaward, nYSeaward))
         {
            // It is, so check if the cell in the seaward direction is a sea cell
            if (! m_pRasterGrid->Cell[nXSeaward][nYSeaward].bIsDryLand())
            {
               // There is sea in this seaward direction, so we are on the coast. Mark the current cell as a coast cell, if not already done so
               bAtCoast = true;
               if (! m_pRasterGrid->Cell[nX][nY].bIsCoastline())
               {
                  // Not already a coast cell, so mark the current cell as coast and add it to the vector object
                  m_pRasterGrid->Cell[nX][nY].SetAsCoastline(true);
                  LTempGridCRS.Append(&Pti);
               }
            }
            else
            {
               // The seaward cell is not a sea cell, so we will move to it next time
               nX = nXSeaward;
               nY = nYSeaward;

               // And set a new search direction, to keep turning seaward
               nSearchDirection = nSeawardNewDirection;
               continue;
            }
         }

         // OK, we couldn't move seaward (but we may have marked the current cell as coast) so next try to move straight on. Is this straight-ahead cell still within the grid?
         if (bIsWithinGrid(nXStraightOn, nYStraightOn))
         {
            // It is, so check if there is sea immediately in front
            if (! m_pRasterGrid->Cell[nXStraightOn][nYStraightOn].bIsDryLand())
            {
               // Sea is in front, so we are on the coast. Mark the current cell as a coast cell, if not already done so
               bAtCoast = true;
               if (! m_pRasterGrid->Cell[nX][nY].bIsCoastline())
               {
                  // Not already checked, so mark the current cell as coast and add it to the vector object
                  m_pRasterGrid->Cell[nX][nY].SetAsCoastline(true);
                  LTempGridCRS.Append(&Pti);
               }
            }
            else
            {
               // The straight-ahead cell is not a sea cell, so we will move to it next time
               nX = nXStraightOn;
               nY = nYStraightOn;

               // The search direction remains unchanged
               continue;
            }
         }

         // Couldn't move either seaward or straight on (but we may have marked the current cell as coast) so next try to move in the anti-seaward direction. Is this anti-seaward cell still within the grid?
         if (bIsWithinGrid(nXAntiSeaward, nYAntiSeaward))
         {
            // It is, so check if there is sea in this anti-seaward cell
            if (! m_pRasterGrid->Cell[nXAntiSeaward][nYAntiSeaward].bIsDryLand())
            {
               // There is sea on the anti-seaward side, so we are on the coast. Mark the current cell as a coast cell, if not already done so
               bAtCoast = true;
               if (! m_pRasterGrid->Cell[nX][nY].bIsCoastline())
               {
                  // Not already checked, so mark this point as coast and add it to the vector object
                  m_pRasterGrid->Cell[nX][nY].SetAsCoastline(true);
                  LTempGridCRS.Append(&Pti);
               }
            }
            else
            {
               // The anti-seaward cell is not a sea cell, so we will move to it next time
               nX = nXAntiSeaward;
               nY = nYAntiSeaward;

               // And set a new search direction, to keep turning seaward
               nSearchDirection = nAntiSeawardNewDirection;
               continue;
            }
         }

         // Could not move to the seaward side, straight ahead, or to the anti-seaward side, so we must be in a single-cell dead end! As a last resort, turn round and move back to where we just came from
         nX = nXGoBack;
         nY = nYGoBack;

         // And change the search direction
         nSearchDirection = nGoBackNewDirection;
      }
      while (true);

      if (bSearchedTooLong)
      {
         LogStream << WARN << m_ulIter << ": abandoned tracing coastline after " << nRoundTheLoop << " iterations" << endl;
         nThisCoast--;
         nThisEdgePoint++;
         continue;
      }

//      cout << m_ulIter << ": coastline = " << nThisCoast << " nRoundTheLoop = " << nRoundTheLoop << endl;

      // ========================================================================================================================
      // OK, we have finished tracing this coastline on the grid
      int nXEnd = nX;
      int nYEnd = nY;
      int nSize = LTempGridCRS.nGetSize();
      if (! ((LTempGridCRS[nSize-1].nGetX() == nXEnd) && (LTempGridCRS[nSize-1].nGetY()) == nYEnd))
      {
         // Not already at end of LTempGridCRS, so add it
         LTempGridCRS.Append(nXEnd, nYEnd);
         nSize++;
      }

      // Now do some consistency checking of the temporary vector coastline. The end point of this temporary coastline vector should be one of the previously-found endpoints. So compare it with each of the coastline endpoints which we found earlier
      bool bFound = false;
      int nEndEdge = ORIENTATION_NONE;
      const int nTolerance = 25;     // Seems to be necessary, sometimes the endpoint is a few cells away from a previously found endpoint
      for (int j = 0; j < static_cast<int>(VPtiEdgePoint->size()); j++)
      {
         // Must not match with itself
         if (j == nThisEdgePoint)
            continue;

         if (bFound)
            break;

         int nEndPointX = VPtiEdgePoint->at(j).nGetX();
         int nEndPointY = VPtiEdgePoint->at(j).nGetY();

         if ((0 == nEndPointX) || (m_nXGridMax-1 == nEndPointX))
         {
            if (bFound)
               break;

            // The endpoint is at the left (W) or right (E) edge, so search a few cells on either side in the Y direction
            vector<int> VnYCellsToTry;
            for (int i = -nTolerance; i < nTolerance+1; i++)
               VnYCellsToTry.push_back(nEndPointY + i);

            for (unsigned int i = 0; i < VnYCellsToTry.size(); i++)
            {
               if ((nXEnd == nEndPointX) && (nYEnd == VnYCellsToTry[i]) && (! VbMatched->at(j)))
               {
                  // OK, this end point is within +/- nTolerance of one of the edge points which we found earlier, and it hasn't already been matched with a previous coastline segment. So flag both points as matched
                  VbMatched->at(nThisEdgePoint) = true;
                  VbMatched->at(j) = true;
                  VnMatchedWith->at(nThisEdgePoint) = j;
                  VnMatchedWith->at(j) = nThisEdgePoint;
                  nEndEdge = VnStartEdge->at(j);
                  bFound = true;

                  break;
               }
            }
         }
         else
         {
            // The endpoint is at the top (N) or bottom (S) edge, so search a few cells on either side in the X direction
            if (bFound)
               break;

            vector<int> VnXCellsToTry;
            for (int i = -nTolerance; i < nTolerance+1; i++)
               VnXCellsToTry.push_back(nEndPointX + i);

            for (unsigned int i = 0; i < VnXCellsToTry.size(); i++)
            {
               if ((nYEnd == nEndPointY) && (nXEnd == VnXCellsToTry[i]) && (! VbMatched->at(j)))
               {
                  // OK, this end point is within +/- nTolerance of one of the edge points which we found earlier, and it hasn't already been matched with a previous coastline segment. So flag both points as matched
                  VbMatched->at(nThisEdgePoint) = true;
                  VbMatched->at(j) = true;
                  VnMatchedWith->at(nThisEdgePoint) = j;
                  VnMatchedWith->at(j) = nThisEdgePoint;
                  nEndEdge = VnStartEdge->at(j);
                  bFound = true;

                  break;
               }
            }
         }
      }

      // So how did we do? First check to see if the coastline is too short
      unsigned int nCoastSize = LTempGridCRS.nGetSize();
      if (nCoastSize < COASTMIN)
      {
         // The vector coastline is unreasonably small, so abandon it
         LogStream << WARN << "iteration " << m_ulIter << ": size of temporary coastline " << nThisCoast << " (" << nCoastSize << ") is less than minimum (" << COASTMIN << ")" << endl;

         nThisCoast--;

         // And now move on the the next edge point and try to trace a valiod coastline from that, since tracing from this edge point led to a too-short coastline
         nThisEdgePoint++;
         continue;
      }

      if (! bFound)
      {
         // We have a problem, the vector coastline endpoint was not amongst the previously-found endpoints
         int bIgnore = false;
         int nXStartPoint = VPtiEdgePoint->at(nThisEdgePoint).nGetX();
         int nYStartPoint = VPtiEdgePoint->at(nThisEdgePoint).nGetY();

         LogStream << setiosflags(ios::fixed);
         LogStream << WARN << "iteration " << m_ulIter << ": when temporary coastline " << nThisCoast << " was traced from endpoint " << nThisEdgePoint << " [" << nXStartPoint << "][" << nYStartPoint << "], it ended at [" << nXEnd << "][" << nYEnd << "], with " << nSize << " points. ";
         if ((nXStartPoint == nXEnd) && (nYStartPoint == nYEnd))
         {
            LogStream << "Coastline segments must not start from and end at the same cell, so not using this coastline segment.";
            bIgnore = true;
         }
         else
            LogStream << "This is not an unmatched previously-found endpoint. However, still using this coastline segment.";
         LogStream << endl << "Currently unmatched endpoints are:" << endl;
         for (int j = 0; j < static_cast<int>(VPtiEdgePoint->size()); j++)
         {
            if ((j != nThisEdgePoint) && (! VbMatched->at(j)))
               LogStream << j << " [" << static_cast<int>(VPtiEdgePoint->at(j).nGetX()) << "][" << static_cast<int>(VPtiEdgePoint->at(j).nGetY()) << "]" << endl;
         }
         LogStream << "Grid size is " << m_nXGridMax << " x " << m_nYGridMax << endl;
         LogStream << "---------------------" << endl;

         if (bIgnore)
         {
            nThisEdgePoint++;
            continue;
         }

         // Assume this is not serious, carry on. But need to specify nEndEdge or get errors in smoothing routines
         if (nXEnd == 0)
            nEndEdge = ORIENTATION_WEST;
         else if (nXEnd == m_nXGridMax-1)
            nEndEdge = ORIENTATION_EAST;
         else if (nYEnd == 0)
            nEndEdge = ORIENTATION_NORTH;
         else if (nYEnd == m_nYGridMax-1)
            nEndEdge = ORIENTATION_SOUTH;
      }

      // All OK, this is a valid coastline
      nValidCoast++;

      // Next, convert the grid coordinates in LTempGridCRS (integer values stored as doubles) to external CRS coordinates (which will probably be non-integer, again stored as doubles). This is done now, so that smoothing is more effective
      CLine LTempExtCRS;
      for (unsigned int j = 0; j < nCoastSize; j++)
         LTempExtCRS.Append(dGridXToExtCRSX(LTempGridCRS[j].nGetX()), dGridYToExtCRSY(LTempGridCRS[j].nGetY()));

      // Now do some smoothing of the vector output, if desired
      if (m_nCoastSmooth == SMOOTH_RUNNING_MEAN)
         LTempExtCRS = LSmoothCoastRunningMean(&LTempExtCRS, VnStartEdge->at(nThisEdgePoint), nEndEdge);
      else if (m_nCoastSmooth == SMOOTH_SAVITZKY_GOLAY)
         LTempExtCRS = LSmoothCoastSavitzkyGolay(&LTempExtCRS, VnStartEdge->at(nThisEdgePoint), nEndEdge);

      // Create a new coastline object and append to it the vector of coastline objects
      CCoast CoastTmp;
      m_VCoast.push_back(CoastTmp);
      for (unsigned int j = 0; j < nCoastSize; j++)
      {
         // Store the smoothed points (in external CRS) in the coast's m_LCoastline object, also append dummy values to the other attribute vectors
         m_VCoast[nValidCoast].AppendToCoast(LTempExtCRS[j].dGetX(), LTempExtCRS[j].dGetY());

         // Also store the locations of the corresponding unsmoothed points (in raster-grid CRS) in the coast's m_VCellsMarkedAsCoastline vector
         m_VCoast[nValidCoast].AppendCellMarkedAsCoastline(&LTempGridCRS[j]);
      }

      // Next, set values for the coast's other attributes. First set the coast's handedness
      m_VCoast[nValidCoast].SetSeaHandedness(nHandedness);

//       LogStream << "=====================================================================================================" << endl;
//       LogStream << "At iteration " << m_ulIter << ": coastline " << nValidCoast << " created, from [" << static_cast<int>(LTempGridCRS[0].nGetX()) << "][" << static_cast<int>(LTempGridCRS[0].nGetY()) << "] to [" << static_cast<int>(LTempGridCRS[nCoastSize-1].nGetX()) << "][" << static_cast<int>(LTempGridCRS[nCoastSize-1].nGetY()) << "] with " << nCoastSize << " points, handednness " << (nHandedness == LEFT_HANDED ? "left" : "right") << endl;
//       LogStream << "In external co-ordinates, the coastline is from " << LTempExtCRS[0].dGetX() << ", " << LTempExtCRS[0].dGetY() << " to " << LTempExtCRS[nCoastSize-1].dGetX() << ", " << LTempExtCRS[nCoastSize-1].dGetY() << endl;
// //       LogStream << "-----------------" << endl;
// //       for (unsigned int kk = 0; kk < nCoastSize; kk++)
// //          LogStream << "[" << LTempGridCRS[kk].nGetX() << "][" << LTempGridCRS[kk].nGetY() << "]" << endl;
// //       LogStream << "-----------------" << endl;
//       LogStream.flush();

      // Next calculate the curvature of the vector coastline
      DoCoastCurvature(nValidCoast, nHandedness);

      // Now calculate values for the coast's flux orientation vector
      DoFluxOrientation(nValidCoast);

      // Finally, move on to the next endpoint, as found earlier
      nThisEdgePoint++;
   }
   while (nThisEdgePoint < static_cast<int>(VPtiEdgePoint->size()));

   return true;
}

