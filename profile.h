/*!
 *
 * \class CProfile
 * \brief Class used to represent coast profile objects
 * \details TODO This is a more detailed description of the CProfile class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file profile.h
 * \brief Contains CProfile definitions
 *
 */

#ifndef PROFILE_H
#define PROFILE_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "line.h"
#include "iline.h"

class CProfile : public CLine
{
private:
   int m_nNumCoastPoint;                        // The coastline index number of the point at which this profile hits the coast
   vector<C2DIPoint> m_VCellsInProfile;         // In grid CRS. the integer coords of the cells 'under' this profile. NOTE point zero is the same as 'cell marked as coastline' in coast object
   vector<C2DPoint> m_VCellsInProfileExtCRS;    // In external CRS, the coords of cells 'under' this profile

public:
   CProfile(int const);
   ~CProfile(void);

   int nGetNumCoastPoint(void) const;

   void SetProfile(vector<C2DPoint>*);
   void ShowProfile(void) const;
   int nGetNumVecPointsInProfile(void) const;
   C2DPoint* PtGetVecPointOnProfile(int const);

   void SetCellInProfile(C2DIPoint*);
   void SetCellInProfile(int const, int const);
   void SetCellsInProfile(vector<C2DIPoint>*);
   vector<C2DIPoint>* VPtiGetCellsInProfile(void);
   C2DIPoint* PtiGetCellInProfile(int const);
   int nGetNCellsInProfile(void) const;

   vector<C2DPoint>* VPtGetCellsInProfileExtCRS(void);
   void SetCellInProfileExtCRS(double const, double const);
};
#endif //PROFILE_H

