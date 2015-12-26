/*!
 *
 * \file cell.cpp
 * \brief CCell routines
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
#include "cell.h"

CCell::CCell()
{
   m_bInActiveZone               =
   m_bCoastline                  =
   m_bCoastlineNormal            = false;

   m_nIntervention               = LF_NONE;

   m_dLocalSlope                 =
   m_dBasementElevation          =
   m_dWaterDepth                 =
   m_dTotWaterDepth              =
   m_dWaveHeight                 =
   m_dWaveOrientation                  =
   m_dSuspendedSediment          =
   m_dTotSuspendedSediment       =
   m_dPotentialErosion           =
   m_dTotPotentialErosion        =
   m_dActualErosion              =
   m_dTotActualErosion           =
   m_dCollapseDepth              =
   m_dTotCollapsedDepth          =
   m_dCollapseDepositDepth       =
   m_dTotCollapsedDepositDepth   = 0;

   m_dInvDistFromProfile         = DBL_NODATA;
}

CCell::~CCell(void)
{
}

//! Sets the flag to show whether this cell is in the active zone
void CCell::SetInActiveZone(bool const bFlag)
{
   m_bInActiveZone = bFlag;
}

//! Returns the flag which shows whether this cell is in the active zone
bool CCell::bInActiveZone(void) const
{
   return m_bInActiveZone;
}

//! Returns true if this cell has had potential erosion
bool CCell::bPotentialErosion(void) const
{
   return (m_dPotentialErosion > 0);
}

//! Returns true if this cell has had actual erosion
bool CCell::bActualErosion(void) const
{
   return (m_dActualErosion > 0);
}

//! Marks this cell as 'under' a coastline
void CCell::SetAsCoastline(bool const bNewFlag)
{
   m_bCoastline = bNewFlag;
}

//! Returns the Coastline flag
bool CCell::bIsCoastline(void) const
{
   return m_bCoastline;
}

//! Marks this cell as 'under' a costline-normal profile
void CCell::SetAsNormalProfile(bool const bNewFlag)
{
   m_bCoastlineNormal = bNewFlag;
}

//! Returns the CoastlineNormal flag
bool CCell::bGetProfile(void) const
{
   return m_bCoastlineNormal;
}

//! Sets the Intervention class
void CCell::SetIntervention(int const nNewCode)
{
   m_nIntervention = nNewCode;
}

//! Returns the Intervention class
int CCell::nGetIntervention(void) const
{
   return (m_nIntervention);
}

//! Sets a value for the inverse distance (in cells) from a coastline-normal profile, is used to weight between-profile erosion calcs
void CCell::SetWeight(double const dNewInvDist)
{
   m_dInvDistFromProfile = dNewInvDist;
}

//! Returns the inverse distance (in cells) from a coastline-normal profile, is used to weight between-profile erosion calcs
double CCell::dGetWeight(void) const
{
   return (m_dInvDistFromProfile);
}

//! Sets the local slope
void CCell::SetLocalSlope(double const dNewSlope)
{
   m_dLocalSlope = dNewSlope;
}

//! Gets the local slope
double CCell::dGetLocalSlope(void) const
{
   return m_dLocalSlope;
}

//! Sets this cell's basement elevation
void CCell::SetBasementElev(double const dNewElev)
{
   m_dBasementElevation = dNewElev;
}

//! Returns this cell's basement elevation
double CCell::dGetBasementElev(void) const
{
   return (m_dBasementElevation);
}

//! Returns the seawater depth on this cell
double CCell::dGetWaterDepth(void) const
{
   return (m_dWaterDepth);
}

//! Increases this cell's seawater depth. No checks here to see that new depth is sensible
void CCell::IncWaterDepth(double const dChngWater)
{
   m_dWaterDepth += dChngWater;
}

//! Decreases this cell's seawater depth. No checks here to see that new depth is sensible
void CCell::DecWaterDepth(double const dChngWater)
{
   m_dWaterDepth -= dChngWater;
}

//! Gets the total water depth for this cell so far during the simulation
double CCell::dGetTotWaterDepth(void) const
{
   return (m_dTotWaterDepth);
}

//! Sets this cell's suspended sediment depth equivalent, it also sets the total-so-far suspended sediment depth equivalent. Note that the no checks here to see if new equiv depth is sensible (e.g. non-negative)
void CCell::SetSuspendedSediment(double const dNewSedDepth)
{
   m_dSuspendedSediment = dNewSedDepth;
   m_dTotSuspendedSediment = dNewSedDepth;
}

//! Increments this cell's suspended sediment depth equivalent, it also increments the total-so-far suspended sediment depth equivalent. Note no checks here to see if new equiv depth is sensible (e.g. non-negative)
void CCell::AddSuspendedSediment(double const dSedDepth)
{
   m_dSuspendedSediment += dSedDepth;
   m_dTotSuspendedSediment += dSedDepth;
}

//! Returns the suspended sediment depth equivalent on this cell
double CCell::dGetSuspendedSediment(void) const
{
   return (m_dSuspendedSediment);
}

//! Gets the total depth equivalent of suspended sediment so far during the simulation
double CCell::dGetTotSuspendedSediment(void) const
{
   return (m_dTotSuspendedSediment);
}

//! Returns the number of layers
int CCell::nGetNLayers(void) const
{
   return m_VLayer.size();
}

//! Return a reference to the Nth layer
CCellLayer* CCell::pGetLayer(int const nLayer)
{
   // NOTE no check that nLayer < size()
   return &m_VLayer[nLayer];
}

//! Returns the volume-equivalent elevation of the sediment's top surface for this cell (if there is a notch, then lower the surface by the missing volume)
double CCell::dGetVolEquivSedTopElev(void) const
{
   double dTopElev = m_dBasementElevation;
   for (unsigned int n = 0; n < m_VLayer.size(); n++)
   {
      dTopElev += (m_VLayer[n].dGetUnconsolidatedThickness() - m_VLayer[n].dGetNotchUnconsolidatedLost());
      dTopElev += (m_VLayer[n].dGetConsolidatedThickness() - m_VLayer[n].dGetNotchConsolidatedLost());
   }

   return dTopElev;
}

//! Returns the true elevation of the sediment's top surface for this cell (if there is a notch, ignore the missing volume)
double CCell::dGetSedimentTopElev(void) const
{
   double dTopElev = m_dBasementElevation;
   for (unsigned int n = 0; n < m_VLayer.size(); n++)
   {
      dTopElev += m_VLayer[n].dGetUnconsolidatedThickness();
      dTopElev += m_VLayer[n].dGetConsolidatedThickness();
   }

   return dTopElev;
}

//! Returns the elevation of this cell's top surface, which could be the seawater surface if wet, or the basement surface if dry
double CCell::dGetTop(void) const
{
   return (this->dGetSedimentTopElev() + m_dWaterDepth);
}

//! Returns true if the elevation of the sediment top surface for this cell is greater than or equal to the grid's still water elevation
bool CCell::bIsDryLand(void) const
{
   return ((this->dGetSedimentTopElev()) >= pGrid->pSim->CSimulation::dGetStillWaterLevel());
}

//! Returns the total thickness of consolidated sediment for this cell
double CCell::dGetConsThickness(void) const
{
   double dThick = 0;
   for (unsigned int n = 0; n < m_VLayer.size(); n++)
      dThick += m_VLayer[n].dGetConsolidatedThickness();

   return dThick;
}

//! Returns the total thickness of unconsolidated sediment for this cell
double CCell::dGetUnconsThickness(void) const
{
   double dThick = 0;
   for (unsigned int n = 0; n < m_VLayer.size(); n++)
      dThick += m_VLayer[n].dGetUnconsolidatedThickness();

   return dThick;
}

//! Appends N layers
void CCell::AddLayers(int const nLayer)
{
   CCellLayer tmpLayer;
   for (int i = 0; i < nLayer; i++)
      m_VLayer.push_back(tmpLayer);
}

//! For this cell, calculates the elevation of the top of every layer
void CCell::CalcAllLayerElevs(void)
{
   m_VdHorizonElev.clear();
   m_VdHorizonElev.push_back(m_dBasementElevation);                                          // Elevation of base of first layer
   int m = 0;
   for (unsigned int n = 0; n < m_VLayer.size(); n++)
      m_VdHorizonElev.push_back(m_VLayer[n].dGetTotalThickness() + m_VdHorizonElev[m++]);    // Elevation of top of layer n
}

//! Given an elevation, this finds the index of the layer that contains that elevation. However the elevation cannot exactly equal the elevation of the layer's top surface (this causes problems with e.g. cliff notches, which extend above this elevation)
int CCell::nGetLayerAtElev(double const dElev) const
{
   /*! Returns ELEV_IN_BASEMENT if in basement, layer number (0 to n), ELEV_ABOVE_SEDIMENT_TOP if higher than top of sediment */
   if (dElev < m_dBasementElevation)
      return ELEV_IN_BASEMENT;

   int nLayers = m_VdHorizonElev.size();        // This includes the basement, so the number of layers is 1 less
   int nRet = ELEV_ABOVE_SEDIMENT_TOP;

   // Ignore m_VdHorizonElev[0] since it is the basement elev
   for (int n = 1; n < nLayers; n++)
   {
      if (dElev < m_VdHorizonElev[n])
         nRet = n-1;
   }

   return nRet;
}

//! For this cell, calculates the elevation of the top of a given layer
double CCell::dCalcLayerElev(const int nLayer)
{
   // Note no check to see if nLayer < m_VLayer.size()
   double dTopElev = m_dBasementElevation;

   for (int n = 0; n <= nLayer; n++)
      dTopElev += m_VLayer[n].dGetTotalThickness();

   return dTopElev;
}

//! Set potential (unconstrained) erosion and increment total potential erosion
void CCell::SetPotentialErosion(double const dPotentialIn)
{
   m_dPotentialErosion = dPotentialIn;
   m_dTotPotentialErosion += dPotentialIn;
}

//! Get potential (unconstrained) erosion
double CCell::dGetPotentialErosion(void) const
{
   return m_dPotentialErosion;
}

//! Get total potential (unconstrained) erosion
double CCell::dGetTotPotentialErosion(void) const
{
   return m_dTotPotentialErosion;
}

//! Set this-iteration actual (constrained) erosion and increment total actual erosion
void CCell::SetActualErosion(double const dThisActualErosion)
{
   m_dActualErosion = dThisActualErosion;
   m_dTotActualErosion += dThisActualErosion;
}

//! Get actual (constrained) erosion
double CCell::dGetActualErosion(void) const
{
   return m_dActualErosion;
}

//! Get total actual (constrained) erosion
double CCell::dGetTotActualErosion(void) const
{
   return m_dTotActualErosion;
}

//! Initialise values for this cell, also calculate the seawater depth (could be zero)
void CCell::InitAndCalcSeaDepth(void)
{
   m_bCoastline               =
   m_bCoastlineNormal         =
   m_bInActiveZone            = false;

   m_dLocalSlope              =
   m_dPotentialErosion        =
   m_dActualErosion           =
   m_dCollapseDepth           =
   m_dCollapseDepositDepth    = 0;

   m_dInvDistFromProfile      = DBL_NODATA;

   m_dWaterDepth = tMax(pGrid->pSim->CSimulation::dGetStillWaterLevel() - this->dGetSedimentTopElev(), 0.0);
}

void CCell::SetWaveHeight(double const dWaveHeight)
{
   m_dWaveHeight = dWaveHeight;
   if (m_dWaveHeight != DBL_NODATA)
      assert(m_dWaveHeight >= 0);
}

double CCell::dGetWaveHeight(void) const
{
   return m_dWaveHeight;
}

void CCell::SetWaveOrientation(double const dWaveOrientation)
{
   m_dWaveOrientation = dWaveOrientation;
}

double CCell::dGetWaveOrientation(void) const
{
   return m_dWaveOrientation;
}

//! Increment the depth of this-iteration cliff collapse on this cell, also increment the total and set the collapse switch to true
void CCell::IncrCollapsedDepth(double const dDepth)
{
   m_dCollapseDepth += dDepth;
   m_dTotCollapsedDepth += dDepth;
}

//! Get the depth of this-iteration cliff collapse on this cell
double CCell::dGetCollapsedDepth(void) const
{
   return m_dCollapseDepth;
}

//! Get the total depth of cliff collapse on this cell
double CCell::dGetTotCollapsedDepth(void) const
{
   return m_dTotCollapsedDepth;
}

//! Increment the depth of this-iteration cliff deposition collapse on this cell, also increment the total
void CCell::IncrCollapsedDepositionDepth(double const dDepth)
{
   m_dCollapseDepositDepth += dDepth;
   m_dTotCollapsedDepositDepth += dDepth;
}

//! Get the depth of this-iteration cliff deposition collapse on this cell
double CCell::dGetCollapsedDepositDepth(void) const
{
   return m_dCollapseDepositDepth;
}

//! Get the total depth of cliff deposition collapse on this cell
double CCell::dGetTotCollapsedDepositDepth(void) const
{
   return m_dTotCollapsedDepositDepth;
}

//! Return a pointer to the cell's CCellLandform object
CCellLandform* CCell::pGetLandform(void)
{
   return &m_Landform;
}
