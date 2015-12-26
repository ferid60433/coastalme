/*!
 *
 * \file profile.cpp
 * \brief CProfile routines
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
#include "profile.h"

CProfile::CProfile(int const nCoastPoint)
{
   m_nNumCoastPoint = nCoastPoint;
}

CProfile::~CProfile(void)
{
}

int CProfile::nGetNumCoastPoint(void) const
{
   return m_nNumCoastPoint;
}

void CProfile::SetProfile(vector<C2DPoint>* VNewPoints)
{
   m_VPoints = *VNewPoints;
}

void CProfile::ShowProfile(void) const
{
   for (unsigned int n = 0; n < m_VPoints.size(); n++)
   {
      cout << n << " [" << m_VPoints[n].dGetX() << "][" << m_VPoints[n].dGetY() << "]" << endl;
   }
}

int CProfile::nGetNumVecPointsInProfile(void) const
{
   return m_VPoints.size();
}

C2DPoint* CProfile::PtGetVecPointOnProfile(int const n)
{
   return &m_VPoints[n];
}

void CProfile::SetCellInProfile(C2DIPoint* Pti)
{
   // In grid CRS
   m_VCellsInProfile.push_back(*Pti);
}

void CProfile::SetCellInProfile(int const nX, int const nY)
{
   // In grid CRS
   m_VCellsInProfile.push_back(C2DIPoint(nX, nY));
}

void CProfile::SetCellsInProfile(vector<C2DIPoint>* VNewPoints)
{
   // In grid CRS
   m_VCellsInProfile = *VNewPoints;
}

vector<C2DIPoint>* CProfile::VPtiGetCellsInProfile(void)
{
   // In grid CRS
   return &m_VCellsInProfile;
}

C2DIPoint* CProfile::PtiGetCellInProfile(int const n)
{
   // In grid CRS
   // NOTE No check to see if n < size()
   return &m_VCellsInProfile[n];
}

int CProfile::nGetNCellsInProfile(void) const
{
   // In grid CRS
   return m_VCellsInProfile.size();
}

vector<C2DPoint>* CProfile::VPtGetCellsInProfileExtCRS(void)
{
   // In external CRS
   return &m_VCellsInProfileExtCRS;
}

void CProfile::SetCellInProfileExtCRS(double const dX, double const dY)
{
   // In external CRS
   m_VCellsInProfileExtCRS.push_back(C2DPoint(dX, dY));
}



