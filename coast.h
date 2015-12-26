/*!
 * \class CCoast
 * \brief Class which is used for the coast object
 * \details TODO This is a more detailed description of the CCoast class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file coast.h
 * \brief Contains CCoast definitions
 *
 */

#ifndef COASTLINE_H
#define COASTLINE_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "2dipoint.h"
#include "cme.h"
#include "coastlandform.h"
#include "profile.h"

class CCoastLandform;
class CProfile;

class CCoast
{
private:
   int
      m_nSeaHandedness;          // Handedness of the direction of the sea from the coastline, traversing coastline in direction of increasing indices
   vector<int>
      m_nVBreakingDistance;      // Distance of breaking (in cells)
   vector<double>
      m_dVCurvature,
      m_dVBreakingWaveHeight,
      m_dVBreakingWaveAngle,
      m_dVDepthOfBreaking,
      m_dVFluxOrientation,       // As in Martin Hurst's COVE model, is the angle (measured from azimuth) of alongshore energy/sediment movement; a +ve flux is in direction of increasing indices along coast
      m_dVWaveEnergy;
   CLine
      m_LCoastline;              // Smoothed line of points (external CRS) giving the plan view of the vector coast
   vector<C2DIPoint>
      m_VCellsMarkedAsCoastline; // Unsmoothed integer x-y co-ords (grid CRS) of the cells that are marked as coastline NOTE same as point zero in profile coords
   vector<CProfile>
      m_VProfile;                // Coast profile objects
   vector<CCoastLandform*>
      m_pVLandforms;             // Pointers to coastal landform objects

public:
   CCoast(void);
   ~CCoast(void);

   void SetSeaHandedness(int const);
   int nGetSeaHandedness(void) const;

   double dGetCurvature(int const) const;
   void SetCurvature(int const, double const);

   void SetBreakingWaveHeight(int const, double const);
   double dGetBreakingWaveHeight(int const) const;
   void SetBreakingWaveAngle(int const, double const);
   double dGetBreakingWaveAngle(int const) const;
   void SetDepthOfBreaking(int const, double const);
   double dGetDepthOfBreaking(int const) const;
   void SetBreakingDistance(int const, int const);
   int nGetBreakingDistance(int const) const;
   void SetFluxOrientation(int const, double const);
   double dGetFluxOrientation(int const) const;
   void SetWaveEnergy(int const, double const);
   double dGetWaveEnergy(int const) const;

   void AppendToCoast(double const, double const);
   CLine* GetCoastline(void);
   C2DPoint* pPtGetVectorCoastlinePoint(int const);     // Can be const according to cppcheck
   int nGetCoastlineSize(void) const;
   double dGetCoastlineSegmentLength(int const, int const);
   double dGetCoastlineLengthSoFar(int const);
   void DisplayCoastline(void);
   void AppendCellMarkedAsCoastline(C2DIPoint*);
   void AppendCellMarkedAsCoastline(int const, int const);
   void SetCellsMarkedAsCoastline(vector<C2DIPoint>*);
   C2DIPoint* pPtiGetCellMarkedAsCoastline(int const);
   int nGetNCellsMarkedAsCoastline(void) const;

   void AppendProfile(int const);
   int nGetNumProfiles(void) const;
   CProfile* pGetProfile(int const);
   void RemoveProfile(int const);

   void AppendCoastLandform(CCoastLandform*);
   CCoastLandform* pGetCoastLandform(int const);
};
#endif //COASTLINE_H

