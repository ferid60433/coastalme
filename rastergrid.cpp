/*!
 *
 * \file rastergrid.cpp
 * \brief CRasterGrid routines
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
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
#include "rastergrid.h"

CRasterGrid* CCell::pGrid = NULL;         // Initialise pGrid, the static member of CCell

CRasterGrid::CRasterGrid(CSimulation* pSimIn)
{
   pSim = pSimIn;
}

CRasterGrid::~CRasterGrid(void)
{
}

int CRasterGrid::nCreateGrid(void)
{
   // Create the 2D vector CCell array
   int nXMax = pSim->nGetGridXMax();
   int nYMax = pSim->nGetGridYMax();

   // TODO Check if we don't have enough memory, if so return RTN_ERR_MEMALLOC
   Cell.resize(nXMax);
   for (int x = 0; x < nXMax; x++)
      Cell[x].resize(nYMax);

   // Initialize the CCell shared pointer to the CRasterGrid object
   CCell::pGrid = this;

   return RTN_OK;
}

