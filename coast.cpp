/*!
 *
 * \file coast.cpp
 * \brief CCoast routines
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
#include "coast.h"

CCoast::CCoast(void)
{
   m_nSeaHandedness = NULL_HANDED;
}

CCoast::~CCoast(void)
{
   for (unsigned int i = 0; i < m_pVLandforms.size(); i++)
      delete m_pVLandforms[i];
}

void CCoast::SetSeaHandedness(int const nNewHandedness)
{
   m_nSeaHandedness = nNewHandedness;
}

int CCoast::nGetSeaHandedness(void) const
{
   return m_nSeaHandedness;
}

double CCoast::dGetCurvature(int const nPoint) const
{
   // NOTE no sanity check for nPoint < m_dVCurvature.Size()
   return m_dVCurvature[nPoint];
}

void CCoast::SetCurvature(int const nPoint, double const dCurvature)
{
   // NOTE no check to see if nPoint < m_dVCurvature.size()
   m_dVCurvature[nPoint] = dCurvature;
}

void CCoast::SetBreakingWaveHeight(int const nPoint, double const dHeight)
{
   // NOTE no check to see if nPoint < m_dVBreakingWaveHeight.size()
   m_dVBreakingWaveHeight[nPoint] = dHeight;
}

double CCoast::dGetBreakingWaveHeight(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_dVBreakingWaveHeight.size()
   return m_dVBreakingWaveHeight[nPoint];
}

void CCoast::SetBreakingWaveAngle(int const nPoint, double const dAngle)
{
   // NOTE no check to see if nPoint < m_dVBreakingWaveAngle.size()
   m_dVBreakingWaveAngle[nPoint] = dAngle;
}

double CCoast::dGetBreakingWaveAngle(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_dVBreakingWaveAngle.size()
   return m_dVBreakingWaveAngle[nPoint];
}

void CCoast::SetDepthOfBreaking(int const nPoint, double const dDepth)
{
   // NOTE no check to see if nPoint < m_dVDepthOfBreaking.size()
   m_dVDepthOfBreaking[nPoint] = dDepth;
}

double CCoast::dGetDepthOfBreaking(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_dVDepthOfBreaking.size()
   return m_dVDepthOfBreaking[nPoint];
}

void CCoast::SetBreakingDistance(int const nPoint, int const nDist)
{
   // NOTE no check to see if nPoint < m_nVBreakingDistance.size()
   m_nVBreakingDistance[nPoint] = nDist;
}

int CCoast::nGetBreakingDistance(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_nVBreakingDistance.size()
   return m_nVBreakingDistance[nPoint];
}

void CCoast::SetFluxOrientation(int const nPoint, double const dAngle)
{
   // NOTE no check to see if nPoint < m_dVFluxOrientation.size()
   m_dVFluxOrientation[nPoint] = dAngle;
}

double CCoast::dGetFluxOrientation(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_dVFluxOrientation.size()
   return m_dVFluxOrientation[nPoint];
}

void CCoast::SetWaveEnergy(int const nPoint, double const dAngle)
{
   // NOTE no check to see if nPoint < m_dVWaveEnergy.size()
   m_dVWaveEnergy[nPoint] = dAngle;
}

double CCoast::dGetWaveEnergy(int const nPoint) const
{
   // NOTE no check to see if nPoint < m_dVWaveEnergy.size()
   return m_dVWaveEnergy[nPoint];
}


void CCoast::AppendToCoast(double const dX, double const dY)
{
   // Appends a coastline point (in external CRS), also appends dummy values for curvature, breaking wave height, wave angle, and flux orientation
   m_LCoastline.Append(dX, dY);
   m_dVCurvature.push_back(DBL_NODATA);
   m_dVBreakingWaveHeight.push_back(DBL_NODATA);
   m_dVBreakingWaveAngle.push_back(DBL_NODATA);
   m_dVDepthOfBreaking.push_back(DBL_NODATA);
   m_dVFluxOrientation.push_back(DBL_NODATA);
   m_dVWaveEnergy.push_back(DBL_NODATA);
   m_nVBreakingDistance.push_back(INT_NODATA);
}

CLine* CCoast::GetCoastline(void)
{
   return &m_LCoastline;
}

C2DPoint* CCoast::pPtGetVectorCoastlinePoint(int const n)      // Can be const according to cppcheck
{
   // Point is in external CRS NOTE no check to see that n is < m_LCoastline.Size()
   return &m_LCoastline[n];
}

int CCoast::nGetCoastlineSize(void) const
{
   return m_LCoastline.nGetSize();
}

void CCoast::DisplayCoastline(void)
{
   m_LCoastline.Display();
}

void CCoast::AppendCellMarkedAsCoastline(C2DIPoint* Pti)
{
   m_VCellsMarkedAsCoastline.push_back(*Pti);
}

void CCoast::AppendCellMarkedAsCoastline(int const nX, int const nY)
{
   m_VCellsMarkedAsCoastline.push_back(C2DIPoint(nX, nY));
}

void CCoast::SetCellsMarkedAsCoastline(vector<C2DIPoint>* VNewPoints)
{
   m_VCellsMarkedAsCoastline = *VNewPoints;
}

C2DIPoint* CCoast::pPtiGetCellMarkedAsCoastline(int const n)
{
   // NOTE No check to see if n < size()
   return &m_VCellsMarkedAsCoastline[n];
}

int CCoast::nGetNCellsMarkedAsCoastline(void) const
{
   return m_VCellsMarkedAsCoastline.size();
}

double CCoast::dGetCoastlineSegmentLength(int const m, int const n)     // can be const according to cppcheck
{
   // NOTE no check to see that m is < m_LCoastline.Size(), same for n
   return hypot(m_LCoastline[n].dGetX() - m_LCoastline[m].dGetX(), m_LCoastline[n].dGetY() - m_LCoastline[m].dGetY());
}

double CCoast::dGetCoastlineLengthSoFar(int const n)
{
   // NOTE no check to see that n is < m_LCoastline.Size()
   double dLen = 0;
   for (int m = 0; m < n; m++)
      dLen += dGetCoastlineSegmentLength(m, m+1);
   return dLen;
}

void CCoast::AppendProfile(int const nCoastPoint)
{
   CProfile Profile(nCoastPoint);
   m_VProfile.push_back(Profile);
}

int CCoast::nGetNumProfiles(void) const
{
   return m_VProfile.size();
}

CProfile* CCoast::pGetProfile(int const n)
{
   // NOTE No safety check that n < size()
   return &m_VProfile[n];
}

void CCoast::RemoveProfile(int const nCoastPoint)
{
   m_VProfile.erase(m_VProfile.begin() + nCoastPoint);
}


void CCoast::AppendCoastLandform(CCoastLandform* pCoastLandform)
{
   m_pVLandforms.push_back(pCoastLandform);
}

CCoastLandform* CCoast::pGetCoastLandform(int const nPoint)
{
   // Note no check to see if nPoint < m_VCellsMarkedAsCoastline.size()
   return m_pVLandforms[nPoint];
}

