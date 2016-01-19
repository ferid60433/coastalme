/*!
 *
 * \file updategrid.cpp
 * \brief Updates the raster grid
 * \details TODO A more detailed description of this routine.
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
#include "cliff.h"

/*===============================================================================================================================

 Update all cells in the raster raster grid and do some per-iteration accounting

===============================================================================================================================*/
int CSimulation::nUpdateGrid(void)
{
   // Go through all cells in the raster grid and calculate some this-iteration totals
   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         if (m_pRasterGrid->Cell[nX][nY].bIsCoastline())
            m_ulThisIterNCoastCells++;

         if (! m_pRasterGrid->Cell[nX][nY].bIsDryLand())
         {
            // Is a sea cell
            m_ulThisIterNSeaCells++;

            m_dThisIterTotSeaDepth += m_pRasterGrid->Cell[nX][nY].dGetWaterDepth();
         }
      }
   }

   // No sea cells?
   if (m_ulThisIterNSeaCells == 0)
      // All land, assume this is an error
      return RTN_ERR_NOSEACELLS;

   // Now go through all cells again and sort out deposition, assuming for the moment that all fine sediment (both from erosion and from cliff collapse) goes into suspension TODO
   m_dThisIterSuspendedSediment = 0;
   double dSuspPerSeaCell = (m_dThisIterActualFineErosion + m_dThisIterCliffCollapseFine) / m_ulThisIterNSeaCells;
   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         if (! m_pRasterGrid->Cell[nX][nY].bIsDryLand())
         {
            m_pRasterGrid->Cell[nX][nY].AddSuspendedSediment(dSuspPerSeaCell);
            m_dThisIterSuspendedSediment += m_pRasterGrid->Cell[nX][nY].dGetSuspendedSediment();
         }
      }
   }

   // Go along each coastline and update the grid with landform attributes, ready for next iteration
   for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
      for (int j = 0; j < m_VCoast[i].nGetCoastlineSize(); j++)
         LandformToGrid(i, j);

   return RTN_OK;
}

