/*!
 *
 * \file gisraster.cpp
 * \brief These functions use GDAL to read and write raster GIS files in several formats
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
#include "sediment.h"
#include "simulation.h"

/*==============================================================================================================================

 Reads a raster DEM of basement elevation data to the Cell array

===============================================================================================================================*/
int CSimulation::nReadBasementDEMData(void)
{
   // Use GDAL to create a dataset object, which then opens the DEM file
   GDALDataset* pGDALDataset = NULL;
   pGDALDataset = (GDALDataset *) GDALOpen(m_strInitialBasementDEMFile.c_str(), GA_ReadOnly);
   if (NULL == pGDALDataset)
   {
      // Can't open file (note will already have sent GDAL error message to stdout)
      cerr << ERR << "cannot open " << m_strInitialBasementDEMFile << " for input: " << CPLGetLastErrorMsg() << endl;
      return RTN_ERR_DEMFILE;
   }

   // Opened OK, so get GDAL basement DEM dataset information
   m_strGDALBasementDEMDriverCode = pGDALDataset->GetDriver()->GetDescription();
   m_strGDALBasementDEMDriverDesc = pGDALDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME);
   m_strGDALBasementDEMProjection = pGDALDataset->GetProjectionRef();

   // If we have reference units, then check that they are in meters (note US spelling)
   if (! m_strGDALBasementDEMProjection.empty())
   {
      string strTmp = strToLower(&m_strGDALBasementDEMProjection);
      if ((strTmp.find("kilometer") != string::npos) || (strTmp.find("meter") == string::npos))
      {
         // error: x-y values must be in metres
         cerr << ERR << "GIS file x-y values (" << m_strGDALBasementDEMProjection << ") in " << m_strInitialBasementDEMFile << " must be in metres" << endl;
         return RTN_ERR_DEMFILE;
      }
   }

   // Now get dataset size, and do some rudimentary checks
   m_nXGridMax = pGDALDataset->GetRasterXSize();
   if (m_nXGridMax == 0)
   {
      // Error: silly number of columns specified
      cerr << ERR << "invalid number of columns (" << m_nXGridMax << ") in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   m_nYGridMax = pGDALDataset->GetRasterYSize();
   if (m_nYGridMax == 0)
   {
      // Error: silly number of rows specified
      cerr << ERR << "invalid number of rows (" << m_nYGridMax << ") in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   // Get geotransformation info
   if (CE_Failure == pGDALDataset->GetGeoTransform(m_dGeoTransform))
   {
      // Can't get geotransformation (note will already have sent GDAL error message to stdout)
      cerr << ERR << CPLGetLastErrorMsg() << " in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   // Get X and Y coordinates for the top left (NW) corner, in the external CRS
   m_dExtCRSNorthWestX = m_dGeoTransform[0];
   m_dExtCRSNorthWestY = m_dGeoTransform[3];

   // Get the X and Y cell sizes, in the external CRS. Note that while the cell is supposed to be square, it may not be exactly so
   // due to oddities with some GIS calculations
   double dCellSideX = tAbs(m_dGeoTransform[1]);
   double dCellSideY = tAbs(m_dGeoTransform[5]);

   // Check that the cell is more or less square
   if (! bFPIsEqual(dCellSideX, dCellSideY, TOLERANCE))
   {
      // Error: cell is not square enough
      cerr << ERR << "cell is not square in " << m_strInitialBasementDEMFile << ", is " << dCellSideX << " x " << dCellSideY << endl;
      return (RTN_ERR_RASTER_FILE_READ);
   }

   // Calculate the average length of cell side, the cell's diagonal, and the area of a cell
   m_dCellSide = (dCellSideX + dCellSideY) / 2;
   m_dCellArea = m_dCellSide * m_dCellSide;
   m_dCellDiagonal = hypot(m_dCellSide, m_dCellSide);

   // Calculate X and Y coordinates for the bottom right (SE) corner, in the external CRS
   m_dExtCRSSouthEastX = m_dExtCRSNorthWestX + (m_dCellSide * m_nXGridMax);
   m_dExtCRSSouthEastY = m_dExtCRSNorthWestY - (m_dCellSide * m_nYGridMax);

   // And calc the grid area in external CRS units
   m_dExtCRSGridArea = tAbs(m_dExtCRSNorthWestX - m_dExtCRSSouthEastX) * tAbs(m_dExtCRSNorthWestY * m_dExtCRSSouthEastY);

   // Now get GDAL raster band information
   GDALRasterBand* pGDALBand = NULL;
   int nBlockXSize = 0, nBlockYSize = 0;
   pGDALBand = pGDALDataset->GetRasterBand(1);
   pGDALBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
   m_strGDALBasementDEMDataType = GDALGetDataTypeName(pGDALBand->GetRasterDataType());

   // If we have value units, then check them
   char szUnits[10] = "";

   strcpy(szUnits, pGDALBand->GetUnitType());

   if ((*szUnits != '\0') && strcmp(szUnits, "m"))
   {
      // Error: value units must be m
      cerr << ERR << "DEM vertical units are (" << szUnits << " ) in " << m_strInitialBasementDEMFile << ", should be 'm'" << endl;
      return RTN_ERR_DEMFILE;
   }

   // Next allocate memory for two 2D arrays of raster cell objects: tell the user what is happening
   AnnounceAllocateMemory();
   int nRet = m_pRasterGrid->nCreateGrid();
   if (nRet != RTN_OK)
      return nRet;

   // Allocate memory for a 1D floating-point array, to hold the scan line for GDAL
   float* pfScanline;
   pfScanline = new float[m_nXGridMax];

   if (NULL == pfScanline)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_nXGridMax << " x 1D array" << endl;
      return (RTN_ERR_MEMALLOC);
   }

   // Now read in the data
   for (int j = 0; j < m_nYGridMax; j++)
   {
      // Read scanline
      if (CE_Failure == pGDALBand->RasterIO(GF_Read, 0, j, m_nXGridMax, 1, pfScanline, m_nXGridMax, 1, GDT_Float32, 0, 0))
      {
         // Error while reading scanline
         cerr << ERR << CPLGetLastErrorMsg() << " in " << m_strInitialBasementDEMFile << endl;
         return RTN_ERR_DEMFILE;
      }

      // All OK, so read scanline into cell elevations
      for (int i = 0; i < m_nXGridMax; i++)
         m_pRasterGrid->Cell[i][j].SetBasementElev(pfScanline[i]);
   }

   // Finished, so get rid of dataset object
   GDALClose((GDALDatasetH) pGDALDataset);

   // Get rid of memory allocated to this array
   delete[] pfScanline;

   return RTN_OK;
}


/*==============================================================================================================================

 Reads all other raster GIS datafiles into the RasterGrid array

===============================================================================================================================*/
int CSimulation::nReadRasterGISData(int const nDataItem, int const nLayer)
{
   string
      strGISFile,
      strDriverCode,
      strDriverDesc,
      strProjection,
      strDataType;

   switch (nDataItem)
   {
      case (LANDFORM_RASTER):
         // Initial Landform Class GIS data
         strGISFile = m_strInitialLandformFile;
         break;

      case (INTERVENTION_RASTER):
         // Initial Intervention Class GIS data
         strGISFile = m_strInitialInterventionFile;
         break;

      case (SUSP_SED_RASTER):
         // Initial Suspended Sediment GIS data
         strGISFile = m_strInitialSuspSedimentFile;
         break;

      case (FINE_UNCONS_RASTER):
         // Initial Unconsolidated Fine Sediment GIS data
         strGISFile = m_VstrInitialFineUnconsSedimentFile[nLayer];
         break;

      case (SAND_UNCONS_RASTER):
         // Initial Unconsolidated Sand Sediment GIS data
         strGISFile = m_VstrInitialSandUnconsSedimentFile[nLayer];
         break;

      case (COARSE_UNCONS_RASTER):
         // Initial Unconsolidated Coarse Sediment GIS data
         strGISFile = m_VstrInitialCoarseUnconsSedimentFile[nLayer];
         break;

      case (FINE_CONS_RASTER):
         // Initial Consolidated Fine Sediment GIS data
         strGISFile = m_VstrInitialFineConsSedimentFile[nLayer];
         break;

      case (SAND_CONS_RASTER):
         // Initial Consolidated Sand Sediment GIS data
         strGISFile = m_VstrInitialSandConsSedimentFile[nLayer];
         break;

      case (COARSE_CONS_RASTER):
         // Initial Consolidated Coarse Sediment GIS data
         strGISFile = m_VstrInitialCoarseConsSedimentFile[nLayer];
         break;
   }

   // Do we have a filename for this data item? If we don't then the data item remains as whatever it was when initialised (zeros)
   if (! strGISFile.empty())
   {
      // We do have a filename, so use GDAL to create a dataset object, which then opens the GIS file
      GDALDataset* pGDALDataset = NULL;
      pGDALDataset = (GDALDataset *) GDALOpen(strGISFile.c_str(), GA_ReadOnly);
      if (NULL == pGDALDataset)
      {
         // Can't open file (note will already have sent GDAL error message to stdout)
         cerr << ERR << "cannot open " << strGISFile << " for input: " << CPLGetLastErrorMsg() << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Opened OK, so get dataset information
      strDriverCode = pGDALDataset->GetDriver()->GetDescription();
      strDriverDesc = pGDALDataset->GetDriver()->GetMetadataItem (GDAL_DMD_LONGNAME);
      strProjection = pGDALDataset->GetProjectionRef();

      // If we have reference units, then check that they are in meters (note US spelling)
   //   if (! strProjection.empty())
   //   {
   //      string strTmp = strToLower(strProjection);
         // TODO this is causing problems with the test data
   //      if ((strTmp.find("kilometer") != string::npos) || (strTmp.find("meter") == string::npos))
   //      {
            // error: x-y values must be in metres
   //         cerr << ERR << "GIS file x-y values (" << strProjection << ") in " << strGISFile << " must be 'meter'" << endl;
   //         return (RTN_ERR_RASTER_FILE_READ);
   //      }
   //   }

      // Get geotransformation info
      double dGeoTransform[6];
      if (CE_Failure == pGDALDataset->GetGeoTransform(dGeoTransform))
      {
         // Can't get geotransformation (note will already have sent GDAL error message to stdout)
         cerr << ERR << CPLGetLastErrorMsg() << " in " << strGISFile << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Now get dataset size, and do some checks
      int nTmp = pGDALDataset->GetRasterXSize();
      if (nTmp != m_nXGridMax)
      {
         // Error: incorrect number of columns specified
         cerr << ERR << "different number of columns in " << strGISFile << " (" << nTmp << ") and " << m_strInitialBasementDEMFile <<  "(" << m_nXGridMax << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      nTmp = pGDALDataset->GetRasterYSize();
      if (nTmp != m_nYGridMax)
      {
         // Error: incorrect number of rows specified
         cerr << ERR << "different number of rows in " << strGISFile << " (" <<  nTmp << ") and " << m_strInitialBasementDEMFile << " (" << m_nYGridMax << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmp = dGeoTransform[0];
      if (! bFPIsEqual(dTmp, m_dExtCRSNorthWestX, TOLERANCE))
      {
         // Error: different min x from DEM file
         cerr << ERR << "different min x values in " << strGISFile << " (" << dTmp << ") and " << m_strInitialBasementDEMFile << " (" << m_dExtCRSNorthWestX << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      dTmp = dGeoTransform[3];
      if (! bFPIsEqual(dTmp, m_dExtCRSNorthWestY, TOLERANCE))
      {
         // Error: different min x from DEM file
         cerr << ERR << "different min y values in " << strGISFile << " (" << dTmp << ") and " << m_strInitialBasementDEMFile << " (" << m_dExtCRSNorthWestY << ")" << endl;
//         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmpResX = tAbs(dGeoTransform[1]);
      if (! bFPIsEqual(dTmpResX, m_dCellSide, 1e-2))
      {
         // Error: different cell size in X direction: note that due to rounding errors in some GIS packages, must expect some discrepancies
         cerr << ERR << "cell size in X direction (" << dTmpResX << ") in " << strGISFile << " differs from cell size in of basement DEM (" << m_dCellSide << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmpResY = tAbs(dGeoTransform[5]);
      if (! bFPIsEqual(dTmpResY, m_dCellSide, 1e-2))
      {
         // Error: different cell size in Y direction: note that due to rounding errors in some GIS packages, must expect some discrepancies
         cerr << ERR << "cell size in Y direction (" << dTmpResY << ") in " << strGISFile << " differs from cell size of basement DEM (" << m_dCellSide << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Now get GDAL raster band information
      GDALRasterBand* pGDALBand = NULL;
      int nBlockXSize = 0, nBlockYSize = 0;
      pGDALBand = pGDALDataset->GetRasterBand(1);              // TODO give a message if there are several bands
      pGDALBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
      strDataType = GDALGetDataTypeName(pGDALBand->GetRasterDataType());

      switch (nDataItem)
      {
         case (LANDFORM_RASTER):
            // Initial Landform Class GIS data
            m_strGDALLDriverCode = strDriverCode;
            m_strGDALLDriverDesc = strDriverDesc;
            m_strGDALLProjection = strProjection;
            m_strGDALLDataType = strDataType;
            break;

         case (INTERVENTION_RASTER):
            // Initial Intervention Class GIS data
            m_strGDALIDriverCode = strDriverCode;
            m_strGDALIDriverDesc = strDriverDesc;
            m_strGDALIProjection = strProjection;
            m_strGDALIDataType = strDataType;
            break;

         case (SUSP_SED_RASTER):
            // Initial Suspended Sediment GIS data
            m_strGDALISSDriverCode = strDriverCode;
            m_strGDALISSDriverDesc = strDriverDesc;
            m_strGDALISSProjection = strProjection;
            m_strGDALISSDataType = strDataType;
            break;

         case (FINE_UNCONS_RASTER):
            // Initial Unconsolidated Fine Sediment GIS data
            m_VstrGDALIUFDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUFDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUFProjection[nLayer] = strProjection;
            m_VstrGDALIUFDataType[nLayer] = strDataType;
            break;

         case (SAND_UNCONS_RASTER):
            // Initial Unconsolidated Sand Sediment GIS data
            m_VstrGDALIUSDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUSDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUSProjection[nLayer] = strProjection;
            m_VstrGDALIUSDataType[nLayer] = strDataType;
            break;

         case (COARSE_UNCONS_RASTER):
            // Initial Unconsolidated Coarse Sediment GIS data
            m_VstrGDALIUCDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUCDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUCProjection[nLayer] = strProjection;
            m_VstrGDALIUCDataType[nLayer] = strDataType;
            break;

         case (FINE_CONS_RASTER):
            // Initial Consolidated Fine Sediment GIS data
            m_VstrGDALICFDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICFDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICFProjection[nLayer] = strProjection;
            m_VstrGDALICFDataType[nLayer] = strDataType;
            break;

         case (SAND_CONS_RASTER):
            // Initial Consolidated Sand Sediment GIS data
            m_VstrGDALICSDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICSDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICSProjection[nLayer] = strProjection;
            m_VstrGDALICSDataType[nLayer] = strDataType;
            break;

         case (COARSE_CONS_RASTER):
            // Initial Consolidated Coarse Sediment GIS data
            m_VstrGDALICCDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICCDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICCProjection[nLayer] = strProjection;
            m_VstrGDALICCDataType[nLayer] = strDataType;
            break;
      }

      // Allocate memory for a 1D array, to hold the scan line for GDAL
      float* pfScanline;
      pfScanline = new float[m_nXGridMax];
      if (NULL == pfScanline)
      {
         // Error, can't allocate memory
         cerr << ERR << "cannot allocate memory for " << m_nXGridMax << " x 1D array" << endl;
         return (RTN_ERR_MEMALLOC);
      }

      // Now read in the data
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         // Read scanline
         if (CE_Failure == pGDALBand->RasterIO(GF_Read, 0, nY, m_nXGridMax, 1, pfScanline, m_nXGridMax, 1, GDT_Float32, 0, 0))
         {
            // Error while reading scanline
            cerr << ERR << CPLGetLastErrorMsg() << " in " << strGISFile << endl;
            return (RTN_ERR_RASTER_FILE_READ);
         }

         // All OK, so read scanline into cells
         for (int nX = 0; nX < m_nXGridMax; nX++)
         {
            switch (nDataItem)
            {
               case (LANDFORM_RASTER):
                  // Initial Landform Class GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLandform()->SetCategory(int(pfScanline[nX]));
                  break;

               case (INTERVENTION_RASTER):
                  // Initial Intervention Class GIS data
                  m_pRasterGrid->Cell[nX][nY].SetIntervention(int(pfScanline[nX]));
                  break;

               case (SUSP_SED_RASTER):
                  // Initial Suspended Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].SetSuspendedSediment(pfScanline[nX]);
                  break;

               case (FINE_UNCONS_RASTER):
                  // Initial Unconsolidated Fine Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->SetFine(pfScanline[nX]);
                  break;

               case (SAND_UNCONS_RASTER):
                  // Initial Unconsolidated Sand Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->SetSand(pfScanline[nX]);
                  break;

               case (COARSE_UNCONS_RASTER):
                  // Initial Unconsolidated Coarse Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->SetCoarse(pfScanline[nX]);
                  break;

               case (FINE_CONS_RASTER):
                  // Initial Consolidated Fine Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->SetFine(pfScanline[nX]);
                  break;

               case (SAND_CONS_RASTER):
                  // Initial Consolidated Sand Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->SetSand(pfScanline[nX]);
                  break;

               case (COARSE_CONS_RASTER):
                  // Initial Consolidated Coarse Sediment GIS data
                  m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->SetCoarse(pfScanline[nX]);
                  break;
            }
         }
      }

      // Finished, so get rid of dataset object
      GDALClose( (GDALDatasetH) pGDALDataset);

      // Get rid of memory allocated to this array
      delete[] pfScanline;
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Writes floating point GIS raster files using GDAL, using data from the RasterGrid array

===============================================================================================================================*/
bool CSimulation::bWriteRasterGISFloat(int const nDataItem, string const* strPlotTitle, int const nLayer)
{
   // Begin constructing the file name for this save
   string strFilePathName(m_strOutPath);

   char szNumTmp[4] = "";
   string strLayer = "_layer_";
   strLayer.append(pszTrimLeft(pszLongToSz(nLayer+1, szNumTmp, 4)));
   strLayer.append("_");

   switch (nDataItem)
   {
      case (PLOT_BASEMENT_ELEV):
         strFilePathName.append(BASEMENTELEVNAME);
         break;

      case (PLOT_SEDIMENT_TOP_ELEV):
         strFilePathName.append(SEDIMENTTOPNAME);
         break;

      case (PLOT_LOCAL_SLOPE):
      strFilePathName.append(LOCALSLOPENAME);
         break;

      case (PLOT_WATER_DEPTH):
         strFilePathName.append(WATERDEPTHNAME);
         break;

      case (PLOT_WAVE_HEIGHT):
         strFilePathName.append(WAVEHEIGHTNAME);
         break;

      case (PLOT_DISTWEIGHT):
         strFilePathName.append(DISTWEIGHTNAME);
         break;

      case (PLOT_POTENTIAL_EROSION):
         strFilePathName.append(POTENTIALEROSIONNAME);
         break;

      case (PLOT_ACTUAL_EROSION):
         strFilePathName.append(ACTUALEROSIONNAME);
         break;

      case (PLOT_TOTAL_POTENTIAL_EROSION):
         strFilePathName.append(TOTALPOTENTIALEROSIONNAME);
         break;

      case (PLOT_TOTAL_ACTUAL_EROSION):
         strFilePathName.append(TOTALACTUALEROSIONNAME);
         break;

      case (PLOT_SUSPSED):
         strFilePathName.append(SUSPSEDNAME);
         break;

      case (PLOT_FINEUNCONSSED):
         strFilePathName.append(FINEUNCONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_SANDUNCONSSED):
         strFilePathName.append(SANDUNCONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_COARSEUNCONSSED):
         strFilePathName.append(COARSEUNCONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_FINECONSSED):
         strFilePathName.append(FINECONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_SANDCONSSED):
         strFilePathName.append(SANDCONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_COARSECONSSED):
         strFilePathName.append(COARSECONSSEDNAME);
         strFilePathName.append(strLayer);
         break;

      case (PLOT_COLLAPSE):
         strFilePathName.append(COLLAPSENAME);
         break;

      case (PLOT_TOTAL_COLLAPSE):
         strFilePathName.append(TOTALCOLLAPSENAME);
         break;

      case (PLOT_COLLAPSE_DEPOSIT):
         strFilePathName.append(COLLAPSEDEPOSITNAME);
         break;

      case (PLOT_TOTAL_COLLAPSE_DEPOSIT):
         strFilePathName.append(TOTALCOLLAPSEDEPOSITNAME);
         break;
   }

   // Append the 'save number' to the filename
   if (m_nGISSave > 99)
   {
      // For save numbers of three or more digits, don't prepend zeros (note 10 digits is max)
      char szNumTmp[10] = "";
      strFilePathName.append(pszTrimLeft(pszLongToSz(m_nGISSave, szNumTmp, 10)));
   }
   else
   {
      // Prepend zeros to the save number
      char szNumTmp[3] = "";
      pszLongToSz(m_nGISSave, szNumTmp, 3, 10);
      strFilePathName.append(pszTrimLeft(szNumTmp));
   }

   // Finally, maybe append the extension
   if (! m_strGDALRasterOutputDriverExtension.empty())
   {
      strFilePathName.append(".");
      strFilePathName.append(m_strGDALRasterOutputDriverExtension);
   }

   GDALDriver* pDriver;
   pDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
   GDALDataset* pOutDataSet;
   char** papszOptions = NULL;      // driver-specific options
   pOutDataSet = pDriver->Create(strFilePathName.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float32, papszOptions);
   if (NULL == pOutDataSet)
   {
      // Couldn't create file
      cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      return false;
   }

   // Set projection info for output dataset (will be same as was read in from DEM)
   CPLPushErrorHandler(CPLQuietErrorHandler);                        // needed to get next line to fail silently, if it fails
   pOutDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());       // will fail for some formats
   CPLPopErrorHandler();

   // Set geotransformation info for output dataset (will be same as was read in from DEM)
   if (CE_Failure == pOutDataSet->SetGeoTransform(m_dGeoTransform))
      LogStream << WARN << "cannot write geotransformation information to " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;

   // Allocate memory for a 1D array, to hold the floating point raster band data for GDAL
   float* pfRaster;
   pfRaster = new float[m_nXGridMax * m_nYGridMax];
   if (NULL == pfRaster)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_nXGridMax * m_nYGridMax << " x 1D floating-point array for " << m_strRasterGISOutFormat << " file named " << strFilePathName << endl;
      return (RTN_ERR_MEMALLOC);
   }

   // Fill the array
   int n = 0;
   double dTmp = 0;
   for (int nY = 0; nY < m_nYGridMax; nY++)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         switch (nDataItem)
         {
            case (PLOT_BASEMENT_ELEV):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetBasementElev();
               break;

            case (PLOT_SEDIMENT_TOP_ELEV):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetSedimentTopElev();
               break;

            case (PLOT_LOCAL_SLOPE):
            dTmp = m_pRasterGrid->Cell[nX][nY].dGetLocalSlope();
               break;

            case (PLOT_WATER_DEPTH):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetWaterDepth();
               break;

            case (PLOT_WAVE_HEIGHT):
               if (m_pRasterGrid->Cell[nX][nY].bIsDryLand())
                  dTmp = DBL_NODATA;
               else
                  dTmp = m_pRasterGrid->Cell[nX][nY].dGetWaveHeight();
               break;

            case (PLOT_DISTWEIGHT):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetWeight();
               break;

            case (PLOT_POTENTIAL_EROSION):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetPotentialErosion();
               break;

            case (PLOT_ACTUAL_EROSION):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetActualErosion();
               break;

            case (PLOT_TOTAL_POTENTIAL_EROSION):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetTotPotentialErosion();
               break;

            case (PLOT_TOTAL_ACTUAL_EROSION):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetTotActualErosion();
               break;

            case (PLOT_SUSPSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetSuspendedSediment();
               break;

            case (PLOT_FINEUNCONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->dGetFine();
               break;

            case (PLOT_SANDUNCONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->dGetSand();
               break;

            case (PLOT_COARSEUNCONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetUnconsolidatedSediment()->dGetCoarse();
               break;

            case (PLOT_FINECONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->dGetFine();
               break;

            case (PLOT_SANDCONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->dGetSand();
               break;

            case (PLOT_COARSECONSSED):
               dTmp = m_pRasterGrid->Cell[nX][nY].pGetLayer(nLayer)->pGetConsolidatedSediment()->dGetCoarse();
               break;

            case (PLOT_COLLAPSE):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetCollapsedDepth();
               break;

            case (PLOT_TOTAL_COLLAPSE):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetTotCollapsedDepth();
               break;

            case (PLOT_COLLAPSE_DEPOSIT):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetCollapsedDepositDepth();
               break;

            case (PLOT_TOTAL_COLLAPSE_DEPOSIT):
               dTmp = m_pRasterGrid->Cell[nX][nY].dGetTotCollapsedDepositDepth();
               break;

         }

         // Write this value to the array
         pfRaster[n++] = dTmp;
      }
   }

   // Now write the data. Create a single raster band
   GDALRasterBand* pBand;
   pBand = pOutDataSet->GetRasterBand(1);
   if (CE_Failure == pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pfRaster, m_nXGridMax, m_nYGridMax, GDT_Float32, 0, 0))
   {
      // Write error, better error message
      cerr << ERR << "cannot write data for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      delete[] pfRaster;
      return false;
   }

   // Calculate statistics for this band
   double dMin, dMax, dMean, dStdDev;
   if (CE_Failure == pBand->ComputeStatistics(false, &dMin, &dMax, &dMean, &dStdDev, NULL, NULL))
   {
      cerr << ERR << CPLGetLastErrorMsg() << endl;
      delete[] pfRaster;
      return false;
   }

   // And then write the statistics, fail silently if not supported by this format
   pBand->SetStatistics(dMin, dMax, dMean, dStdDev);

   // Set value units for this band
   char szUnits[10] = "";

   switch (nDataItem)
   {
      case (PLOT_BASEMENT_ELEV):
      case (PLOT_SEDIMENT_TOP_ELEV):
      case (PLOT_WATER_DEPTH):
      case (PLOT_WAVE_HEIGHT):
      case (PLOT_POTENTIAL_EROSION):
      case (PLOT_ACTUAL_EROSION):
      case (PLOT_TOTAL_POTENTIAL_EROSION):
      case (PLOT_TOTAL_ACTUAL_EROSION):
      case (PLOT_SUSPSED):
      case (PLOT_FINEUNCONSSED):
      case (PLOT_SANDUNCONSSED):
      case (PLOT_COARSEUNCONSSED):
      case (PLOT_FINECONSSED):
      case (PLOT_SANDCONSSED):
      case (PLOT_COARSECONSSED):
      case (PLOT_COLLAPSE):
      case (PLOT_TOTAL_COLLAPSE):
      case (PLOT_COLLAPSE_DEPOSIT):
      case (PLOT_TOTAL_COLLAPSE_DEPOSIT):
         strcpy(szUnits, "m");
         break;

      case (PLOT_LOCAL_SLOPE):
         strcpy(szUnits, "m/m");
         break;

      case (PLOT_DISTWEIGHT):
         strcpy(szUnits, "none");
         break;
   }

   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetUnitType(szUnits);                                // Not supported for some GIS formats
   CPLPopErrorHandler();

   // Tell the output deataset about NODATA (missing values)
   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetNoDataValue(DBL_NODATA);                          // Will fail for some formats
   CPLPopErrorHandler();

   // Construct the description
   string strDesc(*strPlotTitle);
   strDesc.append(" at ");
   strDesc.append(strDispTime (m_dSimElapsed, false, false));

   // Set the GDAL description
   pBand->SetDescription(strDesc.c_str());

   // Finished, so get rid of dataset object
   GDALClose( (GDALDatasetH) pOutDataSet);

   // Also get rid of memory allocated to this array
   delete[] pfRaster;

   return true;
}


/*==============================================================================================================================

 Writes integer GIS raster files using GDAL, using data from the RasterGrid array

===============================================================================================================================*/
bool CSimulation::bWriteRasterGISInt(int const nDataItem, string const* strPlotTitle, double const dElev)
{
   // Begin constructing the file name for this save
   string strFilePathName(m_strOutPath);
   stringstream ststrTmp;

   switch (nDataItem)
   {
      case (PLOT_BINARY_POTENTIAL_EROSION) :
         strFilePathName.append(BINARYPOTENTIALEROSIONNAME);
         break;

      case (PLOT_SLICE) :
         // TODO get working for multiple slices
         strFilePathName.append(SLICENAME);
         ststrTmp << "_" << dElev << "_";
         strFilePathName.append(ststrTmp.str());
         break;

      case (PLOT_LANDFORM) :
         strFilePathName.append(LANDFORMNAME);
         break;

      case (PLOT_INTERVENTION) :
         strFilePathName.append(INTERVENTIONNAME);
         break;

      case (PLOT_RASTER_COAST) :
         strFilePathName.append(RASTERCOASTNAME);
         break;

      case (PLOT_RASTER_NORMAL) :
         strFilePathName.append(RASTERCOASTNORMALNAME);
         break;

      case (PLOT_ACTIVEZONE) :
         strFilePathName.append(ACTIVEZONENAME);
         break;
   }

   // Append the 'save number' to the filename
   if (m_nGISSave > 99)
   {
      // For save numbers of three or more digits, don't prepend zeros (note 10 digits is max)
      char szNumTmp[10] = "";
      strFilePathName.append(pszTrimLeft(pszLongToSz(m_nGISSave, szNumTmp, 10)));
   }
   else
   {
      // Prepend zeros to the save number
      char szNumTmp[3] = "";
      pszLongToSz(m_nGISSave, szNumTmp, 3);
      strFilePathName.append(pszTrimLeft(szNumTmp));
   }

   // Finally, maybe append the extension
   if (! m_strGDALRasterOutputDriverExtension.empty())
   {
      strFilePathName.append(".");
      strFilePathName.append(m_strGDALRasterOutputDriverExtension);
   }

   GDALDriver* pDriver;
   pDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
   GDALDataset* pOutDataSet;
   char** papszOptions = NULL;      // driver-specific options
   pOutDataSet = pDriver->Create(strFilePathName.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Int32, papszOptions);
   if (NULL == pOutDataSet)
   {
      // Couldn't create file
      cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      return false;
   }

   // Set projection info for output dataset (will be same as was read in from DEM)
   CPLPushErrorHandler(CPLQuietErrorHandler);                              // Needed to get next line to fail silently, if it fails
   pOutDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());     // Will fail for some formats
   CPLPopErrorHandler();

   // Set geotransformation info for output dataset (will be same as was read in from DEM)
   if (CE_Failure == pOutDataSet->SetGeoTransform(m_dGeoTransform))
      LogStream << WARN << "cannot write geotransformation information to " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;

   // Allocate memory for a 1D array, to hold the integer raster band data for GDAL
   int* pnRaster;
   pnRaster = new GInt32[m_nXGridMax * m_nYGridMax];
   if (NULL == pnRaster)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_nXGridMax * m_nYGridMax << " x 1D floating-point array for " << m_strRasterGISOutFormat << " file named " << strFilePathName << endl;
      return (RTN_ERR_MEMALLOC);
   }

   // Fill the array
   int nTmp  = 0, n = 0;
   for (int nY = 0; nY < m_nYGridMax; nY++)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         switch (nDataItem)
         {
            case (PLOT_BINARY_POTENTIAL_EROSION):
               nTmp = m_pRasterGrid->Cell[nX][nY].bPotentialErosion();
               break;

            case (PLOT_SLICE):
               nTmp = m_pRasterGrid->Cell[nX][nY].nGetLayerAtElev(dElev);
               break;

            case (PLOT_LANDFORM):
               nTmp = m_pRasterGrid->Cell[nX][nY].pGetLandform()->nGetCategory();
               break;

            case (PLOT_INTERVENTION):
               nTmp = m_pRasterGrid->Cell[nX][nY].nGetIntervention();
               break;

            case (PLOT_RASTER_COAST):
               nTmp = (m_pRasterGrid->Cell[nX][nY].bIsCoastline() ? 1 : 0);
               break;

            case (PLOT_RASTER_NORMAL):
               nTmp = (m_pRasterGrid->Cell[nX][nY].bGetProfile() ? 1 : 0);
               break;

            case (PLOT_ACTIVEZONE):
               nTmp = (m_pRasterGrid->Cell[nX][nY].bInActiveZone() ? 1 : 0);
               break;
         }

         // Write this value to the array
         pnRaster[n++] = nTmp;
      }
   }

   // Now write the data. Create a single raster band
   GDALRasterBand* pBand;
   pBand = pOutDataSet->GetRasterBand(1);
   if (CE_Failure == pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pnRaster, m_nXGridMax, m_nYGridMax, GDT_Int32, 0, 0))
   {
      // Write error, better error message
      cerr << ERR << "cannot write data for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      delete[] pnRaster;
      return false;
   }

   // Calculate statistics for this band
   double dMin, dMax, dMean, dStdDev;
   if (CE_Failure == pBand->ComputeStatistics(false, &dMin, &dMax, &dMean, &dStdDev, NULL, NULL))
   {
      cerr << ERR << CPLGetLastErrorMsg() << endl;
      delete[] pnRaster;
      return false;
   }

   // And then write the statistics, fail silently if not supported by this format
   pBand->SetStatistics(dMin, dMax, dMean, dStdDev);

   // Set value units for this band
   string strUnits;

   switch (nDataItem)
   {
      case (PLOT_BINARY_POTENTIAL_EROSION):
      case (PLOT_SLICE):
      case (PLOT_LANDFORM):
      case (PLOT_INTERVENTION):
      case (PLOT_RASTER_COAST):
      case (PLOT_RASTER_NORMAL):
      case (PLOT_ACTIVEZONE):
         strUnits = "none";
   }

   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetUnitType(strUnits.c_str());                       // Not supported for some GIS formats
   CPLPopErrorHandler();

   // Tell the output deataset about NODATA (missing values)
   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetNoDataValue(INT_NODATA);                          // Will fail for some formats
   CPLPopErrorHandler();

   // Construct the description
   string strDesc(*strPlotTitle);
   if (nDataItem == PLOT_SLICE)
   {
      ststrTmp.clear();
      ststrTmp << dElev << "m, ";
      strDesc.append(ststrTmp.str());
   }
   strDesc.append(" at ");
   strDesc.append(strDispTime(m_dSimElapsed, false, false));

   // Set the GDAL description
   pBand->SetDescription(strDesc.c_str());

   // Set raster category names
   char** papszCategoryNames = NULL;

   switch (nDataItem)
   {
      case (PLOT_SLICE):
         papszCategoryNames = CSLAddString(papszCategoryNames, "Basement");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 1");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 2");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 3");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 4");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 5");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 6");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 7");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 8");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 9");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 10");
         break;

      case (PLOT_LANDFORM):
         // TODO
         break;

      case (PLOT_INTERVENTION):
         // TODO
         break;

      case (PLOT_RASTER_COAST):
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not coastline");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Coastline");
         break;

      case (PLOT_RASTER_NORMAL):
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not coastline consolidated normal");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Coastline consolidated normal");
         break;

      case (PLOT_ACTIVEZONE):
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not in active zone");
         papszCategoryNames = CSLAddString(papszCategoryNames, "In active zone");
         break;

   }

   CPLPushErrorHandler(CPLQuietErrorHandler);        // needed to get next line to fail silently, if it fails
   pBand->SetCategoryNames(papszCategoryNames);      // not supported for some GIS formats
   CPLPopErrorHandler();

   // Finished, so get rid of dataset object
   GDALClose( (GDALDatasetH) pOutDataSet);
//   delete pOutDataSet;

   // Get rid of memory allocated to this array
   delete[] pnRaster;

   return true;
}
