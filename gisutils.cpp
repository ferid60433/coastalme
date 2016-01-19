/*!
 *
 * \file gisutils.cpp
 * \brief Various GIS-related functions
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
#include "simulation.h"

/*==============================================================================================================================

 Transforms an X-axis ordinate in the external CRS to the equivalent raster-grid CRS X-axis ordinate

 Note that cell[0][0] is at the top left (NW) corner of the grid. If the output is an integer between 0 and m_nXGridMax then this refers to the centroid of a cell. However, the output raster-grid value may be non-integer, i.e. it may refer to a point which is not the centroid of a cell. Also the output raster-grid value may be -ve or greater than m_nXGridMax-1 i.e. it may refer to a point which lies outside the raster grid

===============================================================================================================================*/
double CSimulation::dExtCRSXToGridX(double const dX) const
{
   return ((dX - (m_dCellSide / 2) - m_dExtCRSNorthWestX) / m_dCellSide);
}

/*==============================================================================================================================

 Transforms a Y-axis ordinate in the external CRS to the equivalent raster-grid CRS Y-axis co-ordinate

 Note that cell[0][0] is at the top left (NW) corner of the grid. If the output is an integer between 0 and m_nYGridMax then this refers to the centroid of a cell. However, the output raster-grid value may be non-integer, i.e. it may refer to a point which is not the centroid of a cell. Also the output raster-grid value may be -ve or greater than m_nYGridMax-1 than i.e. it may refer to a point which lies outside the raster grid

===============================================================================================================================*/
double CSimulation::dExtCRSYToGridY(double const dY) const
{
   return ((m_dExtCRSNorthWestY - (dY  + (m_dCellSide / 2))) / m_dCellSide);
}

/*==============================================================================================================================

 Transforms an X-axis ordinate in the raster-grid CRS to the equivalent external CRS X-axis ordinate

 Note that cell[0][0] is at the top left (NW) corner of the grid. If the input is an integer between 0 and m_nXGridMax then this refers to the centroid of a cell. However, the input raster-grid value may be non-integer, i.e. it may refer to a point which is not the centroid of a cell. Also the input raster-grid value may be -ve or greater than m_nXGridMax-1 i.e. it may refer to a point which lies outside the raster grid

===============================================================================================================================*/
double CSimulation::dGridXToExtCRSX(double const dX) const
{
   return (m_dExtCRSNorthWestX + (dX * m_dCellSide) + (m_dCellSide / 2));
}

/*==============================================================================================================================

 Transforms a Y-axis ordinate in the raster-grid CRS to the equivalent external CRS Y-axis ordinate

 Note that cell[0][0] is at the top left (NW) corner of the grid. If the input is an integer between 0 and m_nYGridMax then this refers to the centroid of a cell. However, the input raster-grid value may be non-integer, i.e. it may refer to a point which is not the centroid of a cell. Also the input raster-grid value may be -ve or greater than m_nYGridMax-1 i.e. it may refer to a point which lies outside the raster grid

===============================================================================================================================*/
double CSimulation::dGridYToExtCRSY(double const dY) const
{
   return (m_dExtCRSNorthWestY - (dY * m_dCellSide) - (m_dCellSide / 2));
}

/*==============================================================================================================================

 Returns the nearest integer column number (i.e. the nearest integer raster-grid X-axis ordinate) given an X-axis ordinate in the external CRS

 Note that cell[0][0] is at the top left (NW) corner of the grid. Note that the output raster-grid value may be -ve or greater than m_nXGridMax-1 i.e. this routine does not check to see whether the point lies inside the raster grid

===============================================================================================================================*/
int CSimulation::nExtCRSXToGridX(double const dX) const
{
   return (static_cast<int>(dRound(this->dExtCRSXToGridX(dX))));
}

/*==============================================================================================================================

 Returns the nearest integer row number (i.e. the nearest integer raster-grid Y-axis ordinate) given a Y-axis ordinate in the external CRS

 Note that cell[0][0] is at the top left (NW) corner of the grid. Note that the output raster-grid value may be -ve or greater than m_nYGridMax-1 i.e. this routine does not check to see whether the point lies inside the raster grid

===============================================================================================================================*/
int CSimulation::nExtCRSYToGridY(double const dY) const
{
   return (static_cast<int>(dRound(this->dExtCRSYToGridY(dY))));
}

/*==============================================================================================================================

 Returns the length between two points

===============================================================================================================================*/
double CSimulation::dGetLengthBetween(C2DPoint* const pPt1, C2DPoint* const pPt2)
{
   double dXDist = pPt1->dGetX() - pPt2->dGetX();
   double dYDist = pPt1->dGetY() - pPt2->dGetY();
   return hypot(dXDist, dYDist);
}

/*==============================================================================================================================

 Checks whether the supplied point (an x-y pair, in the grid CRS) is within the raster grid

===============================================================================================================================*/
bool CSimulation::bIsWithinGrid(int const nX, int const nY) const
{
   if (nX < 0)
      return false;

   if (nX >= m_nXGridMax)
      return false;

   if (nY < 0)
      return false;

   if (nY >= m_nYGridMax)
      return false;

   return true;
}

/*==============================================================================================================================

 Checks whether the supplied point (a reference to a C2DIPoint, in the grid CRS) is within the raster grid

===============================================================================================================================*/
bool CSimulation::bIsWithinGrid(C2DIPoint* const Pti) const
{
   int nX = Pti->nGetX();

   if (nX < 0)
      return false;

   if (nX >= m_nXGridMax)
      return false;

   int nY = Pti->nGetY();

   if (nY < 0)
      return false;

   if (nY >= m_nYGridMax)
      return false;

   return true;
}

/*==============================================================================================================================

 Constrains the supplied point (a reference to a C2DIPoint, in the grid CRS) to be within the raster grid

===============================================================================================================================*/
C2DIPoint* CSimulation::PtiKeepWithinGrid(C2DIPoint* Pti)
{
   int nX = Pti->nGetX();
   nX = tMax(nX, 0);
   nX = tMin(nX, m_nXGridMax-1);
   Pti->SetX(nX);

   int nY = Pti->nGetY();
   nY = tMax(nY, 0);
   nY = tMin(nY, m_nYGridMax-1);
   Pti->SetY(nY);

   return Pti;
}

/*==============================================================================================================================

 Constrains the supplied point (an x-y pair, in the grid CRS) to be within the raster grid

===============================================================================================================================*/
void CSimulation::KeepWithinGrid(int& nX, int& nY)
{
   nX = tMax(nX, 0);
   nX = tMin(nX, m_nXGridMax-1);

   nY = tMax(nY, 0);
   nY = tMin(nY, m_nYGridMax-1);
}

/*==============================================================================================================================

 Constrains the supplied point (a reference to a C2DPoint, in the external CRS) to be within the raster grid

===============================================================================================================================*/
C2DPoint* CSimulation::pPtExtCRSKeepWithinGrid(C2DPoint* pPt)
{
   int nGridX = nExtCRSXToGridX(pPt->dGetX());
   nGridX = tMax(nGridX, 0);
   nGridX = tMin(nGridX, m_nXGridMax-1);
   pPt->SetX(dGridXToExtCRSX(nGridX));

   int nGridY = nExtCRSYToGridY(pPt->dGetY());
   nGridY = tMax(nGridY, 0);
   nGridY = tMin(nGridY, m_nYGridMax-1);
   pPt->SetY(dGridYToExtCRSY(nGridY));

   return pPt;
}

/*==============================================================================================================================

 Constrains the supplied angle to be within 0 and 360 degrees

===============================================================================================================================*/
double CSimulation::dKeepWithin360(double dAngle)
{
   if (dAngle < 0)
      dAngle += 360;
   else if (dAngle >= 360)
      dAngle -= 360;

   return dAngle;
}

/*==============================================================================================================================

 Checks whether the selected raster GDAL driver supports file creation, 32-bit doubles, etc.

===============================================================================================================================*/
bool CSimulation::bCheckRasterGISOutputFormat(void)
{
   // If user hasn't specified a GIS output format, assume that we will use the same GIS format as the input basement DEM
   if (m_strRasterGISOutFormat.empty())
      m_strRasterGISOutFormat = m_strGDALBasementDEMDriverCode;

   GDALDriver* pDriver;
   char** papszMetadata;
   pDriver = GetGDALDriverManager()->GetDriverByName (m_strRasterGISOutFormat.c_str());
   if (NULL == pDriver)
   {
      // Can't load GDAL driver. Incorrectly specified?
      cerr << ERR << "Unknown raster GIS output format '" << m_strRasterGISOutFormat << "'." << endl;
      return false;
   }

   // Get the metadata for this driver
   papszMetadata = pDriver->GetMetadata();

   if (! CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATE, FALSE))
   {
      // Driver does not supports create() method
      cerr << ERR << "Cannot write raster GIS files using GDAL driver '" << m_strRasterGISOutFormat << "'. Choose another format." << endl;
      return false;
   }

   if (! strstr(CSLFetchNameValue (papszMetadata, "DMD_CREATIONDATATYPES"), "Float32"))
   {
      // Driver does not supports 32-bit doubles
      cerr << ERR << "Cannot write floating-point values using raster GDAL driver '" << m_strRasterGISOutFormat << "'. Choose another format." << endl;
      return false;
   }

   // This driver is OK, so store its longname and the default file extension
   m_strGDALRasterOutputDriverLongname  = CSLFetchNameValue(papszMetadata, "DMD_LONGNAME");
   m_strGDALRasterOutputDriverExtension = CSLFetchNameValue(papszMetadata, "DMD_EXTENSION");

   return true;
}

/*==============================================================================================================================

 Checks whether the selected vector OGR driver supports file creation etc.

===============================================================================================================================*/
bool CSimulation::bCheckVectorGISOutputFormat(void)
{

   OGRSFDriver* pOGRDriver;
   pOGRDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(m_strVectorGISOutFormat.c_str());
   if (pOGRDriver == NULL)
   {
      // Can't load OGR driver. Incorrectly specified?
      cerr << ERR << "Unknown vector GIS output format '" << m_strVectorGISOutFormat << "'." << endl;
      return false;
   }

   // Can this driver create files?
   if (! pOGRDriver->TestCapability(ODrCCreateDataSource))
   {
      // Driver does not support creating
      cerr << ERR << "Cannot write vector GIS files using OGR driver '" << m_strVectorGISOutFormat << "'. Choose another format." << endl;
      return false;
   }

   // Can this driver delete files? (Need this to get rid of pre-existing files with same name)
   if (! pOGRDriver->TestCapability(ODrCDeleteDataSource))
   {
      // Driver does not support deleting
      cerr << ERR << "Cannot delete vector GIS files using OGR driver '" << m_strVectorGISOutFormat << "'. Choose another format." << endl;
      return false;
   }

   // Driver is OK, now set some options for individual drivers
   if (m_strVectorGISOutFormat == "ESRI Shapefile")
   {
      // Set this, so that just a single dataset-with-one-layer shapefile is created, rather than a directory
      // (see http://www.gdal.org/ogr/drv_shapefile.html)
      m_strOGRVectorOutputExtension = ".shp";

   }
   // TODO Others

   return true;
}

/*==============================================================================================================================

 The bSaveAllRasterGISFiles member function saves the raster GIS files using values from the RasterGrid array

==============================================================================================================================*/
bool CSimulation::bSaveAllRasterGISFiles(void)
{
   // Increment file number
   m_nGISSave++;

   // Set for next save
   if (m_bSaveRegular)
      m_dRSaveTime += m_dRSaveInterval;
   else
      m_nThisSave = tMin(++m_nThisSave, m_nUSave);

   // These are always written
   if (! bWriteRasterGISFloat(PLOT_SEDIMENT_TOP_ELEV, &PLOT_SEDIMENT_TOP_ELEV_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_LOCAL_SLOPE, &PLOT_LOCAL_SLOPE_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_WATER_DEPTH, &PLOT_WATER_DEPTH_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_WAVE_HEIGHT, &PLOT_WAVE_HEIGHT_TITLE))
      return false;

   if (! bWriteRasterGISInt(PLOT_BINARY_POTENTIAL_EROSION, &PLOT_BINARY_POTENTIAL_EROSION_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_POTENTIAL_EROSION, &PLOT_POTENTIAL_EROSION_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_ACTUAL_EROSION, &PLOT_ACTUAL_EROSION_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_TOTAL_POTENTIAL_EROSION, &PLOT_TOTAL_POTENTIAL_EROSION_TITLE))
      return false;

   if (! bWriteRasterGISFloat(PLOT_TOTAL_ACTUAL_EROSION, &PLOT_TOTAL_ACTUAL_EROSION_TITLE))
      return false;

   if (! bWriteRasterGISInt(PLOT_LANDFORM, &PLOT_LANDFORM_TITLE))
      return false;

   if (! bWriteRasterGISInt(PLOT_INTERVENTION, &PLOT_INTERVENTION_TITLE))
      return false;

   // These are optional
   if (m_bDistWeightSave)
   {
      if (! bWriteRasterGISFloat(PLOT_DISTWEIGHT, &PLOT_DISTWEIGHT_TITLE))
         return false;
   }

   if (m_bSuspSedSave)
   {
      if (! bWriteRasterGISFloat(PLOT_SUSPSED, &PLOT_SUSPSED_TITLE))
         return false;
   }

   if (m_bBasementElevSave)
   {
      if (! bWriteRasterGISFloat(PLOT_BASEMENT_ELEV, &PLOT_BASEMENT_ELEV_TITLE))
         return false;
   }

   for (int nLayer = 0; nLayer < m_nLayers; nLayer++)
   {
      if (m_bFineUnconsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_FINEUNCONSSED, &PLOT_FINEUNCONSSED_TITLE, nLayer))
            return false;
      }

      if (m_bSandUnconsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_SANDUNCONSSED, &PLOT_SANDUNCONSSED_TITLE, nLayer))
            return false;
      }

      if (m_bCoarseUnconsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_COARSEUNCONSSED, &PLOT_COARSEUNCONSSED_TITLE, nLayer))
            return false;
      }

      if (m_bFineConsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_FINECONSSED, &PLOT_FINECONSSED_TITLE, nLayer))
            return false;
      }

      if (m_bSandConsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_SANDCONSSED, &PLOT_SANDCONSSED_TITLE, nLayer))
            return false;
      }

      if (m_bCoarseConsSedSave)
      {
         if (! bWriteRasterGISFloat(PLOT_COARSECONSSED, &PLOT_COARSECONSSED_TITLE, nLayer))
            return false;
      }
   }

   if (m_bSliceSave)
   {
      for (int i = 0; i < static_cast<int>(m_VdSliceElev.size()); i++)
      {
         if (! bWriteRasterGISInt(PLOT_SLICE, &PLOT_SLICE_TITLE, m_VdSliceElev[i]))
            return false;
      }
   }

   if (m_bRasterCoastlineSave)
   {
      if (! bWriteRasterGISInt(PLOT_RASTER_COAST, &PLOT_RASTER_COAST_TITLE))
         return false;
   }

   if (m_bRasterNormalSave)
   {
      if (! bWriteRasterGISInt(PLOT_RASTER_NORMAL, &PLOT_RASTER_NORMAL_TITLE))
         return false;
   }

   if (m_bActiveZoneSave)
   {
      if (! bWriteRasterGISInt(PLOT_ACTIVEZONE, &PLOT_ACTIVEZONE_TITLE))
         return false;
   }

   if (m_bCollapseSave)
   {
      if (! bWriteRasterGISFloat(PLOT_COLLAPSE, &PLOT_COLLAPSE_TITLE))
         return false;
   }

   if (m_bTotCollapseSave)
   {
      if (! bWriteRasterGISFloat(PLOT_TOTAL_COLLAPSE, &PLOT_TOTAL_COLLAPSE_TITLE))
         return false;
   }

   if (m_bCollapseDepositSave)
   {
      if (! bWriteRasterGISFloat(PLOT_COLLAPSE_DEPOSIT, &PLOT_COLLAPSE_DEPOSIT_TITLE))
         return false;
   }

   if (m_bTotCollapseDepositSave)
   {
      if (! bWriteRasterGISFloat(PLOT_TOTAL_COLLAPSE_DEPOSIT, &PLOT_TOTAL_COLLAPSE_DEPOSIT_TITLE))
         return false;
   }

   return true;
}

/*==============================================================================================================================

 The bSaveAllvectorGISFiles member function saves the vector GIS files

==============================================================================================================================*/
bool CSimulation::bSaveAllVectorGISFiles(void)
{
   // Always written
   if (! bWriteVectorGIS(PLOT_COAST, &PLOT_COAST_TITLE))
      return false;

   if (! bWriteVectorGIS(PLOT_NORMALS, &PLOT_NORMALS_TITLE))
      return false;

   if (m_bCoastCurvatureSave)
   {
      if (! bWriteVectorGIS(PLOT_COAST_CURVATURE, &PLOT_COAST_CURVATURE_TITLE))
         return false;
   }

   if (m_bWaveAngleSave)
   {
      if (! bWriteVectorGIS(PLOT_WAVE_ANGLE, &PLOT_WAVE_ANGLE_TITLE))
         return false;
   }

   return true;
}
