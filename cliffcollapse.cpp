/*!
 *
 * \file cliffcollapse.cpp
 * \brief Collapses cliffs
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
#include "cliff.h"

/*===============================================================================================================================

 Collapses all cliffs

===============================================================================================================================*/
int CSimulation::nDoAllCliffCollapse(void)
{
   int nRet = RTN_OK;

   // First go along each coastline and update the total wave energy which it has experienced
   for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
   {
      for (int j = 0; j < m_VCoast[i].nGetCoastlineSize(); j++)
      {
         CCoastLandform* pCoastLandform = m_VCoast[i].pGetCoastLandform(j);

         // Update accumulated wave energy for the coastal landform object
         pCoastLandform->IncTotWaveEnergy(m_VCoast[i].dGetWaveEnergy(j));

         // Now update the coastal landform with the effects of the wave energy
         int nCategory = pCoastLandform->nGetLandFormCategory();
         if (nCategory == LF_CLIFF)
         {
            // TODO Only cliffs at present
            CCliff* pCliff = reinterpret_cast<CCliff*>(pCoastLandform);

            // Calculate this-iteration notch deepening (is a length in grid units TODO Discuss with Andres)
            double dNotchDeepening = m_dCliffErodibility * m_VCoast[i].dGetWaveEnergy(j);

            // Constrain notch deepening if it is more than one cellside (the most we can remove in a single timestep is one cell)
            dNotchDeepening = tMin(m_dCellSide, dNotchDeepening);

            // Deepen the cliff object's erosional notch as a result of wave energy during this timestep. It may be constrained, since deepening cannot exceed the depth of sediment remaining
            dNotchDeepening = pCliff->dErodeNotch(dNotchDeepening);        // Constrain

            // OK, is the notch now deep enough to cause collapse (either because the overhang is greater than the threshold, or because there is no sediment remaining)?
            if (pCliff->bReadyToCollapse(m_dNotchOverhangAtCollapse))
            {
               // It is
               double
                  dFineCollapse = 0,
                  dSandCollapse = 0,
                  dCoarseCollapse = 0;

               // Do the cliff collapse
               nRet = nDoCliffCollapse(pCliff, dNotchDeepening, dFineCollapse, dSandCollapse, dCoarseCollapse);
               if (nRet == RTN_OK)
               {
                  // No problems with the collapse, but what if only fine sediment was produced by the collapse?
                  if ((dSandCollapse + dCoarseCollapse) > 0)
                  {
                     // OK, there is some sand and/or coarse sediment to deposit
                     nRet = nDoCliffCollapseDeposition(pCliff, dSandCollapse, dCoarseCollapse);
                     if (nRet != RTN_OK)
                        return nRet;
                  }
               }
               else
               {
                  LogStream << m_ulIter << ": ERROR in cliff collapse, continuing however" << endl;
                  nRet = RTN_OK;
               }
            }
         }
      }
   }

   return RTN_OK;
}

/*===============================================================================================================================

 Simulates cliff collapse on a single cliff object: it updates both the cliff object and the cell 'under' the cliff object

===============================================================================================================================*/
int CSimulation::nDoCliffCollapse(CCliff* pCliff, double const dNotchDeepen, double& dFineCollapse, double& dSandCollapse, double& dCoarseCollapse)
{
   // Get the cliff cell's grid coords
   int nX = pCliff->PtiGetLocation().nGetX();
   int nY = pCliff->PtiGetLocation().nGetY();

   // Then get the cliff object's notch elevation
   double dNotchElev = pCliff->dGetNotchBaseElev() - m_dNotchBaseBelowStillWaterLevel;

   int nNotchLayer = m_pRasterGrid->Cell[nX][nY].nGetLayerAtElev(dNotchElev);
   if (nNotchLayer == ELEV_ABOVE_SEDIMENT_TOP)
   {
      LogStream << endl << m_ulIter << ": " << ERR << " for cell [" << nX << "][" << nY << "] dNotchElev = " << dNotchElev << " sediment top elevation = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl;

      return RTN_ERR_CLIFFNOTCH;
   }
//    else
//       LogStream << endl << m_ulIter << ": for cell [" << nX << "][" << nY << "] dNotchElev = " << dNotchElev << " sediment top elevation = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl;

   // Flag the coastline cliff object
   pCliff->SetCliffCollapse(true);

   int nLayers = m_pRasterGrid->Cell[nX][nY].nGetNLayers();

   double dRemaining = pCliff->dGetRemaining();
   if (dRemaining <= 0)
   {
      // No cliff sediment left on this cliff object, so the cell which it occupies will no longer be a cliff in the next timestep
      pCliff->SetAllSedimentGone();

      // Set the base of the collapse (see above)
      pCliff->SetNotchBaseElev(dNotchElev);

//      int nX = pCliff->PtiGetLocation().nGetX();
//      int nY = pCliff->PtiGetLocation().nGetY();
//      LogStream << m_ulIter << ": all sediment removed from cliff object after cliff collapse on [" << nX << "][" << nY << "], dNotchElev = " << dNotchElev << endl;
   }

   // Now calculate the vertical depth of sediment lost in this cliff collapse
   double dAboveNotch = m_pRasterGrid->Cell[nX][nY].dGetVolEquivSedTopElev() - dNotchElev;

   // In CoastalME, all depth equivalents are assumed to be a depth on the whole of a cell i.e. on the area of a whole cell. The vertical depth of sediment lost in each cliff collapse is a depth upon only part of a cell, i.e. on a fraction of a cell's area. To keep the depth of cliff collapse consisent with all other depth equivalents, weight it by the fraction of the cell's area which is being removed
   double dNotchAreaFrac = dNotchDeepen / m_dCellSide;
   double dCollapseDepth = dAboveNotch * dNotchAreaFrac;

   // Update the cell's totals for cliff collapse
   m_pRasterGrid->Cell[nX][nY].IncrCollapsedDepth(dCollapseDepth);

   double
      dAvailable = 0,
      dLost = 0;

//   LogStream << m_ulIter << ": cell [" << nX << "][" << nY << "] before removing sediment, dGetVolEquivSedTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetVolEquivSedTopElev() << ", dGetSedimentTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl;

   // Now update the cell's sediment. If there are sediment layers above the notched layer, we must remove sediment from the whole depth of each layer. Again, weight the depth lost by the fraction of the cell's area which is being removed
   for (int n = nLayers-1; n > nNotchLayer; n--)
   {
      // Add in this layer's sediment, both consolidated and unconsolidated, and adjust what is left
      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetFine() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetNotchFineLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dFineCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->IncrNotchFineLost(dLost);
      }

      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetSand() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetNotchSandLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dSandCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->IncrNotchSandLost(dLost);
      }

      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetCoarse() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->dGetNotchCoarseLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dCoarseCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetUnconsolidatedSediment()->IncrNotchCoarseLost(dLost);
      }

      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetFine() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetNotchFineLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dFineCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->IncrNotchFineLost(dLost);
      }

      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetSand() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetNotchSandLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dSandCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->IncrNotchSandLost(dLost);
      }

      dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetCoarse() - m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->dGetNotchCoarseLost();
      if (dAvailable > 0)
      {
         dLost = dAvailable * dNotchAreaFrac;
         dCoarseCollapse += dLost;
         m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->pGetConsolidatedSediment()->IncrNotchCoarseLost(dLost);
      }
   }

   // For the layer which contains the notch, remove only part of the sediment depth
   double dNotchLayerTop = m_pRasterGrid->Cell[nX][nY].dCalcLayerElev(nNotchLayer);
   double dNotchLayerThickness = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->dGetTotalThickness();
//    assert(dNotchLayerThickness > 0);
   double dNotchLayerVertFracRemoved = (dNotchLayerTop - dNotchElev ) / dNotchLayerThickness;

   // Now calculate the fraction of the volume which is removed
   double dNotchLayerFracRemoved = dNotchLayerVertFracRemoved * dNotchAreaFrac;

   // Sort out the notched layer's sediment, both consolidated and unconsolidated, for this cell
   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetFine() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetNotchFineLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dFineCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->IncrNotchFineLost(dLost);
   }

   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetSand() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetNotchSandLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dSandCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->IncrNotchSandLost(dLost);
   }

   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetCoarse() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->dGetNotchCoarseLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dCoarseCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetUnconsolidatedSediment()->IncrNotchCoarseLost(dLost);
   }

   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetFine() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetNotchFineLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dFineCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->IncrNotchFineLost(dLost);
   }

   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetSand() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetNotchSandLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dSandCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->IncrNotchSandLost(dLost);
   }

   dAvailable = m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetCoarse() - m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->dGetNotchCoarseLost();
   if (dAvailable > 0)
   {
      dLost = dAvailable * dNotchLayerFracRemoved;
      dCoarseCollapse += dLost;
      m_pRasterGrid->Cell[nX][nY].pGetLayer(nNotchLayer)->pGetConsolidatedSediment()->IncrNotchCoarseLost(dLost);
   }

   // Update the cell's layer elevations
   m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

//   LogStream << m_ulIter << ": cell [" << nX << "][" << nY << "] after removing sediment, dGetVolEquivSedTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetVolEquivSedTopElev() << ", dGetSedimentTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl << endl;

   // And update the this-iteration totals and the grand totals for collapse
   m_nNThisIterCliffCollapse++;
   m_nNTotCliffCollapse++;

   m_dThisIterCliffCollapseFine += dFineCollapse;
   m_dThisIterCliffCollapseSand += dSandCollapse;
   m_dThisIterCliffCollapseCoarse += dCoarseCollapse;

   m_ldGTotCliffCollapseFine += dFineCollapse;
   m_ldGTotCliffCollapseSand += dSandCollapse;
   m_ldGTotCliffCollapseCoarse += dCoarseCollapse;

   return RTN_OK;
}

/*===============================================================================================================================

 Redistributes the sand-sized and coarse-sized sediment from all this iteration's cliff collapses onto the foreshore, as talus. The talus is added to the existing beach volume (i.e. the unconsolidated sediment). The shoreline is then iteratively advanced seaward until all this volume is accommodated under a Dean equilibrium profile. This equilibrium beach profile is h(y) = A * y^(2/3) where h(y) is the water depth at a distance y from the shoreline and A is a sediment-dependent scale parameter (for a 0.2mm D50, it will be 0.1 m^(1/3))

===============================================================================================================================*/
int CSimulation::nDoCliffCollapseDeposition(CCliff* pCliff, double const dSandCollapse, double const dCoarseCollapse)
{
   int
      nCoast = pCliff->nGetCoast(),
      nStartPoint = pCliff->nGetPointOnCoast(),
      nCoastSize = m_VCoast[nCoast].nGetCoastlineSize();

   double
      dTotSandToDeposit = dSandCollapse,
      dTotCoarseToDeposit = dCoarseCollapse,
      dSandProp = dSandCollapse / (dSandCollapse + dCoarseCollapse),
      dCoarseProp = 1 - dSandProp;

//    LogStream << "=====================================================================================================" << endl;
//    LogStream << m_ulIter << ": coast = " << nCoast << ", point = " << nStartPoint << endl;

   // Calculate the proportion per planview collapse profile
   vector<int>
      nVWidthDistSigned(m_nCliffDepositionPlanviewWidth),
      nVProfileLength(m_nCliffDepositionPlanviewWidth);
   vector<double>
      dVToDepositPerProfile(m_nCliffDepositionPlanviewWidth);

   // TODO IMPROVE LATER, at present all planview profiles are the same length
   int nSigned = - (m_nCliffDepositionPlanviewWidth - 1) / 2;
   for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
   {
      nVWidthDistSigned[n] = nSigned++;
      nVProfileLength[n] = m_dCliffDepositionPlanviewLength;
      dVToDepositPerProfile[n] = (dTotSandToDeposit + dTotCoarseToDeposit) / m_nCliffDepositionPlanviewWidth;
   }

//    LogStream << "Width offsets = ";
//    for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
//    {
//       LogStream << nVWidthDistSigned[n] << " ";
//    }
//    LogStream << endl << "Profile lengths = ";
//    for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
//    {
//       LogStream << nVProfileLength[n] << " ";
//    }
//    LogStream << endl << "Deposition per profile = ";
//    for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
//    {
//       LogStream << dVToDepositPerProfile[n] << " ";
//    }
//    LogStream << endl;

//   LogStream << "dSandCollapse = " << dSandCollapse << " dCoarseCollapse = " << dCoarseCollapse << " m_nCliffDepositionPlanviewWidth = " << m_nCliffDepositionPlanviewWidth << endl;

//    int nX = pCliff->PtiGetLocation().nGetX();
//    int nY = pCliff->PtiGetLocation().nGetY();
//   LogStream << "Cliff object is at cell[" << nX << "][" << nY << "] which is " << dGridXToExtCRSX(nX) << ", " << dGridYToExtCRSY(nY) << endl;

   for (int nAcross = 0; nAcross < m_nCliffDepositionPlanviewWidth; nAcross++)
   {
      int nWidthDistSigned = nVWidthDistSigned[nAcross];

      // Get the start point of the deposition collapse profile
      int nThisPoint = nStartPoint + nWidthDistSigned;

      // Is this start point valid?
      if ((nThisPoint < 0) || (nThisPoint > (nCoastSize-1)))
      {
//          LogStream << endl << m_ulIter << ": ABANDONING PROFILE with nWidthDistSigned = " << nWidthDistSigned << endl;
//          LogStream << "START point " << nThisPoint << " of profile would have been outside the grid, so " << dVToDepositPerProfile[nAcross] << " exported from grid" << endl;
//          LogStream << "dTotSandToDeposit WAS = " << dTotSandToDeposit << " dTotCoarseToDeposit WAS = " << dTotCoarseToDeposit << endl;

         // The start point of the profile would have been outside the grid, so just add this profile's sediment to the volume exported from the grid this timestep
         m_dThisIterSedLost += dVToDepositPerProfile[nAcross];

         // Remove this volume from the total still to be deposited
         dTotSandToDeposit -= (dVToDepositPerProfile[nAcross] * dSandProp);
         dTotCoarseToDeposit -= (dVToDepositPerProfile[nAcross] * dCoarseProp);
//          assert(bIsNumber(dTotSandToDeposit));
//          LogStream << "dTotSandToDeposit NOW = " << dTotSandToDeposit << " dTotCoarseToDeposit NOW = " << dTotCoarseToDeposit << endl;

         continue;
      }

      C2DPoint
         PtStart,
         PtEnd;

      // Make the start of the deposition profile the cliff cell that is marked as coast (not the cell under the smoothed vector coast, they may well be different)
      PtStart.SetX(dGridXToExtCRSX(m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisPoint)->nGetX()));
      PtStart.SetY(dGridYToExtCRSY(m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nThisPoint)->nGetY()));

      int nSeawardOffset = -1;
      do
      {
//         LogStream << endl;

         nSeawardOffset++;

//          if (nSeawardOffset > 20)
//          {
//             // Arbitrary safety check, if we can't store sufficient sediment with an offset of this size then move to the next point along the coast
//             LogStream << "*** PROFILE TOO LONG WITH nSeawardOffset = " << nSeawardOffset << " MOVING TO NEXT POINT ALONG COAST" << endl;
//             break;
//          }

         // Now construct a deposition collapse profile from the start point. First get the end point of this coastline-normal line, it is one longer than the specified length because it includes the cliff point in the profile
         double dThisProfileLength = nVProfileLength[nAcross] + nSeawardOffset + 1;
         int nRtn = nGetCoastNormalEndPoint(nCoast, nThisPoint, &PtStart, dThisProfileLength, &PtEnd);
         if (nRtn != RTN_OK)
         {
            // Could not find an end point so forget this profile
//            LogStream << endl << m_ulIter << ": ABANDONING PROFILE with nWidthDistSigned = " << nWidthDistSigned << endl;

            if (nRtn == RTN_ERR_OFFGRIDENDPOINT)
            {
//                LogStream << "dTotSandToDeposit WAS = " << dTotSandToDeposit << " dTotCoarseToDeposit WAS = " << dTotCoarseToDeposit << endl;
//                LogStream << "END point of profile would have been outside the grid, so " << dVToDepositPerProfile[nAcross] << " exported from grid" << endl;

               // The end point of the profile would have been outside the grid, so just add this profile's sediment to the volume exported from the grid this timestep
               m_dThisIterSedLost += dVToDepositPerProfile[nAcross];

               // Remove this volume from the total still to be deposited
               dTotSandToDeposit -= (dVToDepositPerProfile[nAcross] * dSandProp);
               dTotCoarseToDeposit -= (dVToDepositPerProfile[nAcross] * dCoarseProp);
//                LogStream << "dTotSandToDeposit NOW = " << dTotSandToDeposit << " dTotCoarseToDeposit NOW = " << dTotCoarseToDeposit << endl;
            }

            if (nRtn == RTN_ERR_BADENDPOINT)
            {
               // The profile has a different problem, so (if possible) move one point along the coast and try again. Must add this profile's sediment to the amount remaining per profile however
//                for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
//                {
//                   LogStream << dVToDepositPerProfile[n] << " ";
//                }
//                LogStream << endl;
//                LogStream << "Deposition per profile WAS = ";

               int nWidthRemaining = m_nCliffDepositionPlanviewWidth - nAcross - 1;
               for (int n = nAcross+1; n < m_nCliffDepositionPlanviewWidth; n++)
                  dVToDepositPerProfile[n] = (dTotSandToDeposit + dTotCoarseToDeposit) / nWidthRemaining;

//                LogStream << "Deposition per profile NOW = ";
//                for (int n = 0; n < m_nCliffDepositionPlanviewWidth; n++)
//                {
//                   LogStream << dVToDepositPerProfile[n] << " ";
//                }
//                LogStream << endl;
            }

            break;
         }

//         LogStream << m_ulIter << ": nWidthDistSigned = " << nWidthDistSigned << " cliff collapse profile from " << PtStart.dGetX() << ", " << PtStart.dGetY() << " to " << PtEnd.dGetX() << ", " << PtEnd.dGetY() << " with length (inc. cliff point) = " << dThisProfileLength << endl;

         vector<C2DPoint> VTmpProfile;
         VTmpProfile.push_back(PtStart);
         VTmpProfile.push_back(PtEnd);
         vector<C2DIPoint> VCellsUnderProfile;

         // Now get the raster cells under this profile
         if (nRasterizeCliffCollapseProfile(&VTmpProfile, &VCellsUnderProfile) != RTN_OK)
         {
            cout << m_ulIter << ": error when rasterizing cells during cliff collapse" << endl;
            return RTN_ERR_LINETOGRID;
         }

         int nRasterProfileLength = VCellsUnderProfile.size();
         vector<double> dVProfileNow(nRasterProfileLength);
//         LogStream << "RASTER PROFILE LENGTH = " << nRasterProfileLength << endl;
         if (nRasterProfileLength != dThisProfileLength)
//            LogStream << "*************************" << endl;

//         LogStream << "Profile now (inc. cliff cell) = ";
         for (int n = 0; n < nRasterProfileLength; n++)
         {
            int nX = VCellsUnderProfile[n].nGetX();
            int nY = VCellsUnderProfile[n].nGetY();
            dVProfileNow[n] = m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev();
   //         LogStream << "[" << nX << "][" << nY << "] " << dGridXToExtCRSX(nX) << ", " << dGridYToExtCRSY(nY) << " elev = " << dVProfileNow[n] << " ";
//            LogStream << dVProfileNow[n] << " ";
         }
//         LogStream << endl;

         // Calculate the elevation of the talus top
         double dCliffTopElev = dVProfileNow[0];
         double dCliffBaseElev = dVProfileNow[1];
         double dCliffHeight = dCliffTopElev - dCliffBaseElev;
         double dTalusTopElev = dCliffBaseElev + (dCliffHeight * m_dCliffDepositionHeightFrac);

//         LogStream << "Elevations: cliff top = " << dCliffTopElev << " cliff base = " << dCliffBaseElev << " talus top = " << dTalusTopElev << endl;

//          if (dCliffTopElev < dCliffBaseElev)
//             LogStream << "*** ERROR, cliff top is lower than cliff base" << endl;

         double const dPower = 2.0 / 3.0;
         double dTalusSlopeLength = dThisProfileLength - nSeawardOffset - 1;

         // If user has not supplied a value for m_dCliffDepositionA, then solve for dA so that elevations at end of profile, and at end of equilibrium profile, are the same
         double dA = 0;
         if (m_dCliffDepositionA != 0)
            dA = m_dCliffDepositionA;
         else
            dA = (dTalusTopElev - dVProfileNow[nRasterProfileLength-1]) /  pow(dTalusSlopeLength, dPower);

//         LogStream << "dTalusSlopeLength = " << dTalusSlopeLength << " dA = " << dA << endl;

         // Calculate the equilibrium profile of the talus h(y) = A * y^(2/3) where h(y) is the distance below the talus-top elevation at a distance y from the cliff
         vector<double> dVEquiProfile(nRasterProfileLength);
         double dDistFromTalusStart = 0;
         double dInc = dTalusSlopeLength / (nRasterProfileLength-nSeawardOffset-2);
         dVEquiProfile[0] = dCliffTopElev;
         for (int n = 1; n < nRasterProfileLength; n++)
         {
            if (n <= nSeawardOffset)
            {
               dVEquiProfile[n] = dTalusTopElev;
            }
            else
            {
               double dDistBelowTT = dA * pow(dDistFromTalusStart, dPower);
               dVEquiProfile[n] = dTalusTopElev - dDistBelowTT;
               dDistFromTalusStart += dInc;
            }
         }

//         LogStream << "dDistFromTalusStart - dInc = " << dDistFromTalusStart - dInc << " dThisProfileLength - nSeawardOffset - 2 = " << dThisProfileLength - nSeawardOffset - 1<< endl;

//          LogStream << "Equilibrium profile (inc. cliff cell) = ";
//          for (int n = 0; n < nRasterProfileLength; n++)
//          {
//             LogStream << dVEquiProfile[n] << " ";
//          }
//          LogStream << endl;

         // Subtract the two elevations
         double dTotElevDiff = 0;

//         LogStream << "Difference (inc. cliff cell) = ";
         for (int n = 0; n < nRasterProfileLength; n++)
         {
            double dProfileDiff = dVEquiProfile[n] - dVProfileNow[n];
//            LogStream << dProfileDiff << " ";

            dTotElevDiff += dProfileDiff;
         }
//         LogStream << endl;

         // Does the equilibrium profile allow us to deposit all the sediment which we need to get rid of for this planview profile?
         if (dTotElevDiff >= dVToDepositPerProfile[nAcross])
         {
//            LogStream << "Offset SUFFICIENT with nSeawardOffset = " << nSeawardOffset << endl;
//            LogStream << "dTotElevDiff = " << dTotElevDiff << " dVToDepositPerProfile[nAcross] = " << dVToDepositPerProfile[nAcross] << endl;

            double dPropToDeposit = dVToDepositPerProfile[nAcross] / dTotElevDiff;
//            LogStream << "dPropToDeposit = " << dPropToDeposit << endl;

            double
               dDepositedCheck = 0,
               dRemovedCheck = 0;

            // Yes it does, so adjust all cells in this profile
            for (int n = 0; n < nRasterProfileLength; n++)
            {
               int
                  nX = VCellsUnderProfile[n].nGetX(),
                  nY = VCellsUnderProfile[n].nGetY();

               // TODO get working with multiple layers i.e. discard any layers with zero thickness. At present, just considering the top layer only
               int nTopLayer = m_pRasterGrid->Cell[nX][nY].nGetNLayers() - 1;

               if (dVEquiProfile[n] > dVProfileNow[n])
               {
//                  LogStream << "DEPOSIT ";
                  // At this point along the profile, the equilibrium profile is higher than the present profile. So we can deposit some sediment here
                  double dPotentialSandToDeposit = 0;
                  if (dTotSandToDeposit > 0)
                  {
                     dPotentialSandToDeposit = (dVEquiProfile[n] - dVProfileNow[n]) * dSandProp * dPropToDeposit;
                     dPotentialSandToDeposit = tMin(dPotentialSandToDeposit, dTotSandToDeposit);

                     double dSandNow = m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->dGetSand();
                     m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->SetSand(dSandNow + dPotentialSandToDeposit);

                     dTotSandToDeposit -= dPotentialSandToDeposit;
                    dDepositedCheck += dPotentialSandToDeposit;
                  }

                  double dPotentialCoarseToDeposit = 0;
                  if (dTotCoarseToDeposit > 0)
                  {
                     dPotentialCoarseToDeposit = (dVEquiProfile[n] - dVProfileNow[n]) * dCoarseProp * dPropToDeposit;
                     dPotentialCoarseToDeposit = tMin(dPotentialCoarseToDeposit, dTotCoarseToDeposit);

                     double dCoarseNow = m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->dGetCoarse();
                     m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->SetCoarse(dCoarseNow + dPotentialCoarseToDeposit);

                     dTotCoarseToDeposit -= dPotentialCoarseToDeposit;
                    dDepositedCheck += dPotentialCoarseToDeposit;
                  }

                  // Now update the cell's layer elevations
                  m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

                  // And update the cell's collapse deposition, and total collapse deposition, values
                  m_pRasterGrid->Cell[nX][nY].IncrCollapsedDepositionDepth(dPotentialSandToDeposit + dPotentialCoarseToDeposit);
               }

               else if (dVEquiProfile[n] < dVProfileNow[n])
               {
                  // The equilibrium profile is lower than the present profile, so we must remove some some sediment from here
//                  LogStream << "REMOVE ";
                  double dThisLowering = dVProfileNow[n] - dVEquiProfile[n];

                  // Find out how much sediment we have available on this cell
                  double dExistingAvailableFine = m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->dGetFine();
                  double dExistingAvailableSand = m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->dGetSand();
                  double dExistingAvailableCoarse = m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->dGetCoarse();

                  // Now partition the total lowering for this cell between the three size fractions: do this by relative erodibility
                  int nFineWeight = (dExistingAvailableFine > 0 ? 1 : 0);
                  int nSandWeight = (dExistingAvailableSand > 0 ? 1 : 0);
                  int nCoarseWeight = (dExistingAvailableCoarse > 0 ? 1 : 0);

                  double dTotErodibility = (nFineWeight * m_dFineErodibility) + (nSandWeight * m_dSandErodibility) + (nCoarseWeight * m_dCoarseErodibility);
                 double dTotActualErosion = 0;

                  if (nFineWeight)
                  {
                     // Erode some fine-sized sediment
                     double dFineLowering = (nFineWeight * m_dFineErodibility * dThisLowering) / dTotErodibility;

                     // Make sure we don't get -ve amounts left on the cell
                     double dFine = tMin(dExistingAvailableFine, dFineLowering);
                     double dRemaining = dExistingAvailableFine - dFine;

                    dTotActualErosion += dFine;
                    dRemovedCheck += dFine;

                     // Set the value for this layer
                     m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->SetFine(dRemaining);

                     // And increment the per-iteration total
                    m_dThisIterActualFineErosion += dFine;
                  }

                  if (nSandWeight)
                  {
                     // Erode some sand-sized sediment
                     double dSandLowering = (nSandWeight * m_dSandErodibility * dThisLowering) / dTotErodibility;

                     // Make sure we don't get -ve amounts left on the source cell
                     double dSand = tMin(dExistingAvailableSand, dSandLowering);
                     double dRemaining = dExistingAvailableSand - dSand;

                    dTotActualErosion += dSand;
                    dRemovedCheck += dSand;

                     // Set the value for this layer
                     m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->SetSand(dRemaining);

                     // And increment the per-iteration total
                    m_dThisIterActualSandErosion += dSand;
                  }

                  if (nCoarseWeight)
                  {
                     // Erode some coarse-sized sediment
                     double dCoarseLowering = (nCoarseWeight * m_dCoarseErodibility * dThisLowering) / dTotErodibility;

                     // Make sure we don't get -ve amounts left on the source cell
                     double dCoarse = tMin(dExistingAvailableCoarse, dCoarseLowering);
                     double dRemaining = dExistingAvailableCoarse - dCoarse;

                    dTotActualErosion += dCoarse;
                    dRemovedCheck += dCoarse;

                     // Set the value for this layer
                     m_pRasterGrid->Cell[nX][nY].pGetLayer(nTopLayer)->pGetUnconsolidatedSediment()->SetCoarse(dRemaining);

                     // And increment the per-iteration total
                    m_dThisIterActualCoarseErosion += dCoarse;
                  }

                  // Set the actual erosion value for this cell
//                  m_pRasterGrid->Cell[nX][nY].SetActualErosion(dTotActualErosion);

                  // Recalculate the elevation of every layer
                  m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

//                   // Update per-iteration totals
//                   if (dTotActualErosion > 0)
//                   {
//                      m_ulThisIterNActualErosionCells++;
//                      m_dThisIterActualErosion += dTotActualErosion;
//                   }
               }
            }
 //           LogStream << endl;
 //          LogStream << "Profile done, dDepositedCheck = " << dDepositedCheck << " dRemovedCheck = " << dRemovedCheck << endl;

            break;
         }
//          else
//             LogStream << "Offset not sufficient with nSeawardOffset = " << nSeawardOffset << endl;

      }
      while (true);

//      LogStream << "REMAINING dTotSandToDeposit = " << dTotSandToDeposit << " dTotCoarseToDeposit = " << dTotCoarseToDeposit << endl;
   }

   // Safety check
   if (! bFPIsEqual(dTotSandToDeposit, 0, TOLERANCE))
   {
//      LogStream << ERR << m_ulIter << ": dTotSandToDeposit = " << dTotSandToDeposit << " SET TO ZERO" << endl;
      dTotSandToDeposit = 0;
   }

   // Ditto
   if (! bFPIsEqual(dTotCoarseToDeposit, 0, TOLERANCE))
   {
//      LogStream << ERR << m_ulIter << ": dTotCoarseToDeposit = " << dTotCoarseToDeposit << " SET TO ZERO" << endl;
      dTotCoarseToDeposit = 0;
   }

   // Increment this-iteration totals for cliff collapse deposition
   m_dThisIterCliffCollapseSandDeposition += dSandCollapse;
   m_dThisIterCliffCollapseCoarseDeposition += dCoarseCollapse;

   // TODO check with Andres, do we need (see above) a separate set of this-iteration totals for cliff collapse deposition? Temporarily, also add this into main deposition totals
   m_dThisIterSandDeposition += dSandCollapse;
   m_dThisIterCoarseDeposition += dCoarseCollapse;

   return RTN_OK;
}

/*==============================================================================================================================

 Rasterizes a cliff collapse profile i.e. returns an output vector of cells which are 'under' the vector line

===============================================================================================================================*/
int CSimulation::nRasterizeCliffCollapseProfile(vector<C2DPoint>* const pVPointsIn, vector<C2DIPoint>* pVIPointsOut)
{
   int
      nX,
      nY,
      nSize = pVPointsIn->size();

   pVIPointsOut->clear();

   for (int k = 0; k < (nSize - 1); k++)
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
         nX = dRound(dX);
         nY = dRound(dY);

         // Make sure the interpolated point is within the raster grid (can get this kind of problem due to rounding)
         KeepWithinGrid(nX, nY);

         // This point is fine, so add it to the output vector
         pVIPointsOut->push_back(C2DIPoint(nX, nY));         // In raster-grid co-ordinates

         // And increment for next time
         dX += dXInc;
         dY += dYInc;
      }
   }

   return RTN_OK;
}
