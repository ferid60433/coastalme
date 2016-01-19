/*!
 *
 * \class CCliff
 * \brief Class used to represent cliff objects
 * \details TODO This is a more detailed description of the CCliff class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
 * \copyright GNU General Public License
 *
 * \file cliff.h
 * \brief Contains CCliff definitions
 *
 */

#ifndef CLIFF_H
#define CLIFF_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "2dpoint.h"
#include "coastlandform.h"

class CCliff : public CCoastLandform
{
private:
   bool
      m_bCliffCollapse,
      m_bAllSedimentGone;
   double
      m_dNotchBaseElev,
      m_dNotchOverhang,
      m_dRemaining;

public:
   CCliff(CCoast*, int const, int const, double const, double const, double const, double const);
   ~CCliff(void);

   void SetCliffCollapse(bool const);
   bool bHasCollapsed(void) const;
   bool bAllSedimentGone(void) const;
   void SetAllSedimentGone(void);
   void SetNotchBaseElev(double const);
   double dGetNotchBaseElev(void) const;
   void SetRemaining(double const);
   double dGetRemaining(void);
   void SetNotchOverhang(double const);
   double dGetNotchOverhang(void);
   bool bReadyToCollapse(double const);
   double dErodeNotch(double const);
   void Display(void);
};
#endif // CLIFF_H

