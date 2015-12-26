/*!
 * \class CCell
 * \brief Class for the cell objects which comprise the raster grid
 * \details TODO This is a more detailed description of the CCell class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file cell.h
 * \brief Contains CCell definitions
 *
 */

#ifndef CELL_H
#define CELL_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "cell_layer.h"
#include "cell_landform.h"
#include "rastergrid.h"

class CRasterGrid;                           // Forward declaration

class CCell
{
private:
   bool
      m_bInActiveZone,
      m_bCoastline,
      m_bCoastlineNormal;
   int
      m_nIntervention;                       // Intervention code, see cme.h for codes
   double
      m_dInvDistFromProfile,                 // Inverse of distance (in cells) from a coastline-normal profile, used to weight potential erosion
      m_dLocalSlope,                         // As used in erosion calcs (really just for display purposes)
      m_dBasementElevation,                  // Elevation of basement surface (m)
      m_dWaterDepth,                         // Depth of still water (m), is zero if not inundated
      m_dTotWaterDepth,                      // Depth of still water (m) since beginning of simulation
      m_dWaveHeight,
      m_dWaveOrientation,
      m_dSuspendedSediment,                  // Suspended sediment as depth equivalent (m)
      m_dTotSuspendedSediment,               // Depth of suspended sediment (m) since simulation start
      m_dPotentialErosion,                   // Depth of sediment that would be eroded this timestep, if no supply-limitation
      m_dTotPotentialErosion,                // Total depth of sediment eroded, if no supply-limitation
      m_dActualErosion,                      // Depth of sediment actually eroded this timestep
      m_dTotActualErosion,                   // Total depth of sediment actually eroded so far
      m_dCollapseDepth,                      // Depth of sediment collapsed this iteration
      m_dTotCollapsedDepth,                  // Total depth of sediment collapsed since simulation start
      m_dCollapseDepositDepth,               // Depth of sediment collapsed this iteration
      m_dTotCollapsedDepositDepth;           // Total depth of sediment collapsed since simulation start

   // This cell's landform data
   CCellLandform m_Landform;

   // Initialize these as empty vectors
   vector<CCellLayer> m_VLayer;              // The number of layers NOT including the basement
   vector<double> m_VdHorizonElev;           // The number of layer-top elevations (including that of the basement)
                                             // So this has size 1 greater than size of m_VLayer

public:
   static CRasterGrid* pGrid;                //!< Is static, so every CCell object sees the same value of pGrid

   CCell();
   ~CCell(void);

   void SetInActiveZone(bool const);
   bool bInActiveZone(void) const;
   bool bPotentialErosion(void) const;
   bool bActualErosion(void) const;
   void SetAsCoastline(bool const);
   bool bIsCoastline(void) const;
   void SetAsNormalProfile(bool const);
   bool bGetProfile(void) const;
   void SetLandformComplex(int const);
   int nGetLandformComplex(void) const;
   void SetLandform(int const);
   int nGetLandform(void) const;
   void SetIntervention(int const);
   int nGetIntervention(void) const;
   void SetWeight(double const);
   double dGetWeight(void) const;
   void SetLocalSlope(double const);
   double dGetLocalSlope(void) const;
   void SetBasementElev(double const);
   double dGetBasementElev(void) const;
   double dGetVolEquivSedTopElev(void) const;
   double dGetSedimentTopElev(void) const;
   double dGetTop(void) const;
   bool bIsDryLand(void) const;
   double dGetConsThickness(void) const;
   double dGetUnconsThickness(void) const;
   double dGetWaterDepth(void) const;
   void IncWaterDepth(double const);
   void DecWaterDepth(double const);
   void InitAndCalcSeaDepth(void);
   double dGetTotWaterDepth(void) const;
   void SetWaveHeight(double const);
   double dGetWaveHeight(void) const;
   void SetWaveOrientation(double const);
   double dGetWaveOrientation(void) const;
   void SetSuspendedSediment(double const);
   void AddSuspendedSediment(double const);
   double dGetSuspendedSediment(void) const;
   double dGetTotSuspendedSediment(void) const;
   int nGetNLayers(void) const;
   CCellLayer* pGetLayer(int const);
   void AddLayers(int const);
   void CalcAllLayerElevs(void);
   int nGetLayerAtElev(double const) const;
   double dCalcLayerElev(const int);
   void SetPotentialErosion(double const);
   double dGetPotentialErosion(void) const;
   double dGetTotPotentialErosion(void) const;
   void SetActualErosion(double const);
   double dGetActualErosion(void) const;
   double dGetTotActualErosion(void) const;
   void IncrCollapsedDepth(double const);
   double dGetCollapsedDepth(void) const;
   double dGetTotCollapsedDepth(void) const;
   void IncrCollapsedDepositionDepth(double const);
   double dGetCollapsedDepositDepth(void) const;
   double dGetTotCollapsedDepositDepth(void) const;
   CCellLandform* pGetLandform(void);
};
#endif // CELL_H
