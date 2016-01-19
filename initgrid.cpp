/*!
 *
 * \file initgrid.cpp
 * \brief Initialises the raster grid and calculates sea depth on each cell
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

/*===============================================================================================================================

 Initialize the raster grid and do some per-iteration accounting variables at the beginning of each timestep

===============================================================================================================================*/
int CSimulation::nInitGridAndCalcStillWaterLevel(void)
{
   // Do some per-iteration initialization
   m_ulThisIterNSeaCells              =
   m_ulThisIterNCoastCells            =
   m_ulThisIterNPotentialErosionCells =
   m_ulThisIterNActualErosionCells    = 0;

   m_dThisIterTotSeaDepth                   =
   m_dThisIterPotentialErosion              =
   m_dThisIterActualErosion                 =
   m_dThisIterActualFineErosion             =
   m_dThisIterFineDeposition                =
   m_dThisIterActualSandErosion             =
   m_dThisIterSandDeposition                =
   m_dThisIterActualCoarseErosion           =
   m_dThisIterCoarseDeposition              =
   m_dThisIterSedLost                       =
   m_dThisIterCliffCollapseFine             =
   m_dThisIterCliffCollapseSand             =
   m_dThisIterCliffCollapseCoarse           =
   m_dThisIterCliffCollapseFineDeposition   =
   m_dThisIterCliffCollapseSandDeposition   =
   m_dThisIterCliffCollapseCoarseDeposition = 0;

   // And go through all cells in the RasterGrid array
   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         // Calculate the seawater depth (if any) on this cell and do initialization of erosion etc.
         m_pRasterGrid->Cell[nX][nY].InitAndCalcSeaDepth();

         if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
         {
            // Set wave properties for non-sea cells to missing value number
            m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(DBL_NODATA);
            m_pRasterGrid->Cell[nX][nY].SetWaveHeight(DBL_NODATA);
         }
         else
         {
            // Set all sea cells to have deep water (off-shore) wave orientation and height, will change this later for cells closer to the shoreline
            m_pRasterGrid->Cell[nX][nY].SetWaveOrientation(m_dOffshoreWaveOrientationIn);
            m_pRasterGrid->Cell[nX][nY].SetWaveHeight(m_dOffshoreWaveHeight);
         }

         if (1 == m_ulIter)
         {
            // For the first iteration only, calculate the elevation of all this cell's layers. During the rest of the simulation, this is calculated just after any change occurs
            m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

            if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
            {
               // For the first iteration only: this cell is not a sea cell, so mark it as hinterland TODO this is very crude, improve
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_HINTERLAND);
            }
            else
            {
               // For the first iteration only: this is a sea cell, so mark it as sea TODO this is very crude, improve
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_SEA);
            }
         }
      }
   }

   return RTN_OK;
}
