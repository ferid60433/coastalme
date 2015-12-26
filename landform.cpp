/*!
 *
 * \file landform.cpp
 * \brief Landform routines
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 */

/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "simulation.h"
#include "cliff.h"

/*==============================================================================================================================

 When a coastline is created at the start of each timestep, this routine puts coastal landforms on the coastline. The coastal landform objects are given attributes values from the grid cell 'under' the coastline landform object

===============================================================================================================================*/
int CSimulation::nAssignCoastalLandforms(void)
{
   // For each coastline, put a coastal landform at every point along the coastline
   for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
   {
      for (int j = 0; j < m_VCoast[i].nGetCoastlineSize(); j++)
      {
         // TODO Only cliffs at present
         // Set some initial values for the cliff object's attributes
         double
            dAccumWaveEnergy  = 0,
            dNotchBaseElev    = m_dMinStillWaterLevel,
            dNotchOverhang    = 0,
            dRemaining        = m_dCellSide;

         // Get the coords of the grid cell marked as coastline for the coastal landform object
         int nX = m_VCoast[i].pPtiGetCellMarkedAsCoastline(j)->nGetX();
         int nY = m_VCoast[i].pPtiGetCellMarkedAsCoastline(j)->nGetY();

         // On the raster grid, also store coastline number and and number of point on coastline so can get these quickly later
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCoast(i);
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetPointOnCoast(j);


         if (m_ulIter == 1)
         {
            // No coastal landforms already exist at the first timestep, so no grid cells are flagged with coastal landform attributes. So we must update the grid now using the initial values for the cliff object's attributes, ready for LandformToGrid() at the end of the first timestep
            m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_CLIFF);
            m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchBaseElev(dNotchBaseElev);
            m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchOverhang(dNotchOverhang);
            m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffRemaining(dRemaining);
         }
         else
         {
            // Not the first iteration, so get the landform category of this cell from the previous timestep
            int nLastCategory = m_pRasterGrid->Cell[nX][nY].pGetLandform()->nGetCategory();
            if (nLastCategory == LF_CLIFF)
            {
               // This cell was a cliff in the previous timestep, so set the new cliff object's attributes from data stored in the cell
               dAccumWaveEnergy = m_pRasterGrid->Cell[nX][nY].pGetLandform()->dGetAccumWaveEnergy();
               dNotchOverhang   = m_pRasterGrid->Cell[nX][nY].pGetLandform()->dGetCliffNotchOverhang();
               dNotchBaseElev   = m_pRasterGrid->Cell[nX][nY].pGetLandform()->dGetCliffNotchBaseElev();
               dRemaining       = m_pRasterGrid->Cell[nX][nY].pGetLandform()->dGetCliffRemaining();
            }
            else
            {
               // This cell was not a cliff object in the previous timestep, so mark it as one now and set the cell with the initial values for the cliff object's attributes
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_CLIFF);
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchBaseElev(dNotchBaseElev);
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchOverhang(dNotchOverhang);
               m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffRemaining(dRemaining);
            }
         }

         // Finally create a cliff object on the vector coastline with these attributes
         CCoastLandform* pCliff = new CCliff(&m_VCoast[i], i, j, dRemaining, dNotchBaseElev, dNotchOverhang, dAccumWaveEnergy);
         m_VCoast[i].AppendCoastLandform(pCliff);
      }
   }

   return RTN_OK;
}

/*===============================================================================================================================

 At the end of each timestep, this routine stores the attributes from a single coastal landform object in the grid cell 'under' the object, ready for the next timestep

===============================================================================================================================*/
void CSimulation::LandformToGrid(int const nCoast, int const nPoint)
{
   // What is the coastal landform here?
   CCoastLandform* pCoastLandform = m_VCoast[nCoast].pGetCoastLandform(nPoint);
   int nCategory = pCoastLandform->nGetLandFormCategory();

   // TODO For cliffs only at present
   if (nCategory == LF_CLIFF)
   {
      CCliff* pCliff = reinterpret_cast<CCliff*>(pCoastLandform);

      // Get attribute values from the cliff object
      double dNotchBaseElev = pCliff->dGetNotchBaseElev();
      double dNotchOverhang = pCliff->dGetNotchOverhang();
      double dRemaining = pCliff->dGetRemaining();

      int nX = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nPoint)->nGetX();
      int nY = m_VCoast[nCoast].pPtiGetCellMarkedAsCoastline(nPoint)->nGetY();

      if (pCliff->bAllSedimentGone())
      {
//         cout << m_ulIter << ": cell [" << nX << "][" << nY << "] before removing cliff, dGetVolEquivSedTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetVolEquivSedTopElev() << ", dGetSedimentTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl;

         // All the sediment is gone from this cliff object via cliff collapse, so this cell is no longer a cliff
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_SEA);

         int nLayers = m_pRasterGrid->Cell[nX][nY].nGetNLayers();
         for (int n = 0; n < nLayers; n++)
            m_pRasterGrid->Cell[nX][nY].pGetLayer(n)->RemoveCliff();

         // Update the cell's layer elevations
         m_pRasterGrid->Cell[nX][nY].CalcAllLayerElevs();

//         cout << m_ulIter << ": cell [" << nX << "][" << nY << "] after removing cliff, dGetVolEquivSedTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetVolEquivSedTopElev() << ", dGetSedimentTopElev() = " << m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev() << endl;
//         cout << m_ulIter << ": cell [" << nX << "][" << nY << "] is no longer a cliff" << endl << endl;
      }
      else
      {
         // Still some sediment available in this cliff object, so store the attribute values in the cell
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(LF_CLIFF);
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchBaseElev(dNotchBaseElev);
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffNotchOverhang(dNotchOverhang);
         m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCliffRemaining(dRemaining);
//         cout << m_ulIter << ": cell [" << nX << "][" << nY << "] has dRemaining = " << dRemaining << endl;
      }

      // Always accumulate wave energy
      m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetAccumWaveEnergy(pCliff->dGetTotWaveEnergy());

   }
}
