/*!
 *
 * \class CCoastLandform
 * \brief Abstract class, used as a base class for landforms on the coastline
 * \details TODO This is a more detailed description of the CCoastLandform class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file coastlandform.h
 * \brief Contains CCoastLandform definitions
 *
 */

#ifndef COASTLANDFORM_H
#define COASTLANDFORM_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "2dipoint.h"
#include "coast.h"

class CCoast;                    // Forward declaration

class CCoastLandform
{
protected:
   int
      m_nCoast,
      m_nPointOnCoast,
      m_nCategory;               // Landform category code
   double
      m_dTotWaveEnergy;          // Total accumulated wave energy since beginning of simulation
   CCoast* pCoast;

public:
   CCoastLandform(void);
   virtual ~CCoastLandform(void);

   int nGetCoast(void) const;
   int nGetPointOnCoast(void) const;
   void SetLandFormCategory(int const);
   int nGetLandFormCategory(void) const;
   C2DIPoint PtiGetLocation(void) const;
   void SetTotWaveEnergy(double const);
   void IncTotWaveEnergy(double const);
   double dGetTotWaveEnergy(void) const;

   // Pure virtual function
   virtual void Display() = 0;
};
#endif // COASTLANDFORM_H

