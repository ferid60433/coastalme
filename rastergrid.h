/*!
 *
 * \class CRasterGrid
 * \brief Class used to represent the raster grid of cell objects
 * \details TODO This is a more detailed description of the CRasterGrid class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
 * \copyright GNU General Public License
 *
 * \file rastergrid.h
 * \brief Contains CRasterGrid definitions
 *
 */

#ifndef RASTERGRID_H
#define RASTERGRID_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "simulation.h"

class CCell;         // Forward declaration
class CSimulation;   // Ditto

class CRasterGrid
{
public:
   CSimulation* pSim;
   vector< vector<CCell> > Cell;

   explicit CRasterGrid(CSimulation*);
   ~CRasterGrid(void);
   int nCreateGrid(void);

};
#endif // RASTERGRID_H
