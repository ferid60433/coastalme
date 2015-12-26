/*!
 *
 * \file writeoutput.cpp
 * \brief Writes non-GIS output files for CoastalME
 * \details TODO A more detailed description of this routine.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public  License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include "cme.h"
#include "simulation.h"

/*==============================================================================================================================

 Writes run details to Out and Log files

==============================================================================================================================*/
void CSimulation::WriteRunDetails(void)
{
   // Set default output format to fixed point
   OutStream << setiosflags(ios::fixed);

   OutStream << PROGNAME << " for " << PLATFORM << " " << strGetBuild() << " on " << strGetComputerName() << endl << endl;

   LogStream << PROGNAME << " for " << PLATFORM << " " << strGetBuild() << " on " << strGetComputerName() << endl << endl;

   // ----------------------------------------------- Run Information ----------------------------------------------------------
   OutStream << "RUN DETAILS" << endl;
   OutStream << " Name                                                      \t: " << m_strRunName << endl;
   OutStream << " Started on                                                \t: " << ctime(&m_tSysStartTime);   //  << endl;

   // Same info. for Log file
   LogStream << m_strRunName << " run started on " << ctime(&m_tSysStartTime) << endl;

   // Contine with Out file
   OutStream << " Initialization file                                       \t: "
#ifdef _WIN32
      << pstrChangeToForwardSlash(&m_strCMEIni) << endl;
#else
      << m_strCMEIni << endl;
#endif

   OutStream << " Input data read from                                      \t: "
#ifdef _WIN32
      << pstrChangeToForwardSlash(&m_strDataPathName) << endl;
#else
      << m_strDataPathName << endl;
#endif

   OutStream << " Duration of simulation                                    \t: ";
   OutStream << strDispSimTime(m_dSimDuration) << endl;
   if (m_bSaveRegular)
   {
      // Saves at regular intervals
      OutStream << " Time between saves                                        \t: ";
      OutStream << strDispSimTime(m_dRSaveInterval) << endl;
   }
   else
   {
      // Saves at user-defined intervals
      OutStream << " Saves at                                                  \t: ";
      string strTmp;
      for (int i = 0; i < m_nUSave; i++)
      {
         strTmp.append(strDispSimTime(m_dUSaveTime[i]));
         strTmp.append(", ");
      }

      // Also at end of run
      strTmp.append(strDispSimTime(m_dSimDuration));
      OutStream << strTmp << endl;
   }

   OutStream << " Random number seeds                                       \t: ";
   {
      for (int i = 0; i < NRNG; i++)
         OutStream << m_ulRandSeed[i] << '\t';
   }
   OutStream << endl;

   OutStream << "*First random numbers generated                            \t: " << ulGetRand0() << '\t' << ulGetRand1() << endl;
   OutStream << " Raster GIS output format                                  \t: " << m_strGDALRasterOutputDriverLongname << endl;
   OutStream << " Raster GIS files saved                                    \t: " << strListRasterFiles() << endl;
   if (m_bSliceSave)
   {
      OutStream << setiosflags(ios::fixed) << setprecision(2);
      OutStream << " Elevations for 'slice' raster output files                \t: ";
      for (int i = 0; i < static_cast<int>(m_VdSliceElev.size()); i++)
         OutStream << m_VdSliceElev[i] << " ";
      OutStream << endl;
   }

   OutStream << " Vector GIS output format                                  \t: " << m_strVectorGISOutFormat << endl;
   OutStream << " Vector GIS files saved                                    \t: " << strListVectorFiles() << endl;
   OutStream << " Output file (this file)                                   \t: "
#ifdef _WIN32
      << pstrChangeToForwardSlash(&m_strOutFile) << endl;
#else
      << m_strOutFile << endl;
#endif
   OutStream << " Log file                                                  \t: "
#ifdef _WIN32
      << pstrChangeToForwardSlash(&m_strLogFile) << endl;
#else
      << m_strLogFile << endl;
#endif

   OutStream << " Optional time series files saved                          \t: " << strListTSFiles() << endl;

   OutStream << " Coastline vector smoothing algorithm                      \t: ";
   switch (m_nCoastSmooth)
   {
      case SMOOTH_NONE :
         OutStream << "none";
         break;

      case SMOOTH_RUNNING_MEAN :
         OutStream << "running mean";
         break;

      case SMOOTH_SAVITZKY_GOLAY :
         OutStream << "Savitzky-Golay";
         break;
   }
   OutStream << " Random edge for coastline search?                         \t: " << (m_bRandomCoastEdgeSearch ? "Y": "N") << endl;
   OutStream << endl;

   if (m_nCoastSmooth != SMOOTH_NONE)
   {
      OutStream << " Size of coastline vector smoothing window                 \t: " << m_nCoastSmoothWindow << endl;

      if (m_nCoastSmooth == SMOOTH_SAVITZKY_GOLAY)
         OutStream << " Savitzky-Golay coastline smoothing polynomial order       \t: " << m_nSavGolCoastPoly << endl;
   }
   OutStream << " Size of profile slope smoothing window                    \t: " << m_nProfileSmoothWindow << endl;
   OutStream << " Max local slope on profile (m/m)window                    \t: " << m_dProfileMaxSlope << endl;
   OutStream << endl;


   // --------------------------------------------------- Raster GIS stuff -------------------------------------------------------
   OutStream << "Raster GIS Input Files" << endl;
   OutStream << " Basement DEM file                                         \t: "
#ifdef _WIN32
      << pstrChangeToForwardSlash(&m_strInitialBasementDEMFile) << endl;
#else
      << m_strInitialBasementDEMFile << endl;
#endif
   OutStream << " Basement DEM driver code                                  \t: " << m_strGDALBasementDEMDriverCode << endl;
   OutStream << " GDAL basement DEM driver description                      \t: " << m_strGDALBasementDEMDriverDesc << endl;
   OutStream << " GDAL basement DEM projection                              \t: " << m_strGDALBasementDEMProjection << endl;
   OutStream << " GDAL basement DEM data type                               \t: " << m_strGDALBasementDEMDataType << endl;
   OutStream << " Grid size (X by Y)                                        \t: " << m_nXGridMax << " by " << m_nYGridMax << endl;
   OutStream << resetiosflags(ios::floatfield);
   OutStream << setiosflags(ios::fixed) << setprecision(1);
   OutStream << "*Coordinates of NW corner of grid (external CRS)           \t: " << m_dExtCRSNorthWestX << ", " << m_dExtCRSNorthWestY << endl;
   OutStream << "*Coordinates of SE corner of grid (external CRS)           \t: " << m_dExtCRSSouthEastX << ", " << m_dExtCRSSouthEastY << endl;
   OutStream << "*Cell size                                                 \t: " << m_dCellSide << " m" << endl;
   OutStream << "*Grid area                                                 \t: " << m_dExtCRSGridArea << " m2" << endl;
   OutStream << setiosflags(ios::fixed) << setprecision(2);
   OutStream << "*Grid area                                                 \t: " << m_dExtCRSGridArea * 1e-6 << " km2" << endl;
   OutStream << endl;

   if (! m_strInitialLandformFile.empty())
   {
      OutStream << " Initial Landform Class file                               \t: " << m_strInitialLandformFile << endl;
      OutStream << " GDAL Initial Landform Class file driver code              \t: " << m_strGDALLDriverCode << endl;
      OutStream << " GDAL Initial Landform Class file driver description       \t: " << m_strGDALLDriverDesc << endl;
      OutStream << " GDAL Initial Landform Class file projection               \t: " << m_strGDALLProjection << endl;
      OutStream << " GDAL Initial Landform Class file data type                \t: " << m_strGDALLDataType << endl;
      OutStream << endl;
   }

   if (! m_strInitialInterventionFile.empty())
   {
      OutStream << " Initial Intervention Class file                           \t: " << m_strInitialInterventionFile << endl;
      OutStream << " GDAL Initial Intervention Class file driver code          \t: " << m_strGDALIDriverCode << endl;
      OutStream << " GDAL Initial Intervention Class file driver description   \t: " << m_strGDALIDriverDesc << endl;
      OutStream << " GDAL Initial Intervention Class file projection           \t: " << m_strGDALIProjection << endl;
      OutStream << " GDAL Initial Intervention Class file data type            \t: " << m_strGDALIDataType << endl;
      OutStream << endl;
   }

   if (! m_strInitialSuspSedimentFile.empty())
   {
      OutStream << " Initial Susp Sediment file                                \t: " << m_strInitialSuspSedimentFile << endl;
      OutStream << " GDAL Initial Susp Sediment file driver code               \t: " << m_strGDALISSDriverCode << endl;
      OutStream << " GDAL Initial Susp Sediment file driver description        \t: " << m_strGDALISSDriverDesc << endl;
      OutStream << " GDAL Initial Susp Sediment file projection                \t: " << m_strGDALISSProjection << endl;
      OutStream << " GDAL Initial Susp Sediment file data type                 \t: " << m_strGDALISSDataType << endl;
      OutStream << endl;
   }

   for (int i = 0; i < m_nLayers; i++)
   {
      OutStream << " Layer " << i << (i == 0 ? "(Top)" : "") << (i == m_nLayers-1 ? "(Bottom)" : "") << endl;

      if (! m_VstrInitialFineUnconsSedimentFile[i].empty())
      {
         OutStream << "    Initial Fine Uncons Sediment file                      \t: " << m_VstrInitialFineUnconsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Fine Uncons Sediment file driver code     \t: " << m_VstrGDALIUFDriverCode[i] << endl;
         OutStream << "    GDAL Initial Fine Uncons Sediment file driver desc     \t: " << m_VstrGDALIUFDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Fine Uncons Sediment file projection      \t: " << m_VstrGDALIUFProjection[i] << endl;
         OutStream << "    GDAL Initial Fine Uncons Sediment file data type       \t: " << m_VstrGDALIUFDataType[i] << endl;
         OutStream << endl;
      }

      if (! m_VstrInitialSandUnconsSedimentFile[i].empty())
      {
         OutStream << "    Initial Sand Uncons Sediment file                      \t: " << m_VstrInitialSandUnconsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Sand Uncons Sediment file driver code     \t: " << m_VstrGDALIUSDriverCode[i] << endl;
         OutStream << "    GDAL Initial Sand Uncons Sediment file driver desc     \t: " << m_VstrGDALIUSDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Sand Uncons Sediment file projection      \t: " << m_VstrGDALIUSProjection[i] << endl;
         OutStream << "    GDAL Initial Sand Uncons Sediment file data type       \t: " << m_VstrGDALIUSDataType[i] << endl;
         OutStream << endl;
      }

      if (! m_VstrInitialCoarseUnconsSedimentFile[i].empty())
      {
         OutStream << "    Initial Coarse Uncons Sediment file                    \t: " << m_VstrInitialCoarseUnconsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Coarse Uncons Sediment file driver code   \t: " << m_VstrGDALIUCDriverCode[i] << endl;
         OutStream << "    GDAL Initial Coarse Uncons Sediment file driver desc   \t: " << m_VstrGDALIUCDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Coarse Uncons Sediment file projection    \t: " << m_VstrGDALIUCProjection[i] << endl;
         OutStream << "    GDAL Initial Coarse Uncons Sediment file data type     \t: " << m_VstrGDALIUCDataType[i] << endl;
         OutStream << endl;
      }

      if (! m_VstrInitialFineConsSedimentFile[i].empty())
      {
         OutStream << "    Initial Fine Cons Sediment file                        \t: " << m_VstrInitialFineConsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Fine Cons Sediment file driver code       \t: " << m_VstrGDALICFDriverCode[i] << endl;
         OutStream << "    GDAL Initial Fine Cons Sediment file driver desc       \t: " << m_VstrGDALICFDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Fine Cons Sediment file projection        \t: " << m_VstrGDALICFProjection[i] << endl;
         OutStream << "    GDAL Initial Fine Cons Sediment file data type         \t: " << m_VstrGDALICFDataType[i] << endl;
         OutStream << endl;
      }

      if (! m_VstrInitialSandConsSedimentFile[i].empty())
      {
         OutStream << "    Initial Sand Cons Sediment file                        \t: " << m_VstrInitialSandConsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Sand Cons Sediment file driver code       \t: " << m_VstrGDALICSDriverCode[i] << endl;
         OutStream << "    GDAL Initial Sand Cons Sediment file driver desc       \t: " << m_VstrGDALICSDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Sand Cons Sediment file projection        \t: " << m_VstrGDALICSProjection[i] << endl;
         OutStream << "    GDAL Initial Sand Cons Sediment file data type         \t: " << m_VstrGDALICSDataType[i] << endl;
         OutStream << endl;
      }

      if (! m_VstrInitialCoarseConsSedimentFile[i].empty())
      {
         OutStream << "    Initial Coarse Cons Sediment file                      \t: " << m_VstrInitialCoarseConsSedimentFile[i] << endl;
         OutStream << "    GDAL Initial Coarse Cons Sediment file driver code     \t: " << m_VstrGDALICCDriverCode[i] << endl;
         OutStream << "    GDAL Initial Coarse Cons Sediment file driver desc     \t: " << m_VstrGDALICCDriverDesc[i] << endl;
         OutStream << "    GDAL Initial Coarse Cons Sediment file projection      \t: " << m_VstrGDALICCProjection[i] << endl;
         OutStream << "    GDAL Initial Coarse Cons Sediment file data type       \t: " << m_VstrGDALICCDataType[i] << endl;
         OutStream << endl;
      }
   }
//   OutStream << endl;

   // ---------------------------------------------------- Vector GIS stuff ------------------------------------------------------
   OutStream << "Vector GIS Input Files" << endl;
   if (! m_strInitialCoastlineFile.empty())
   {
      OutStream << " Initial Coastline file                                    \t: " << m_strInitialCoastlineFile << endl;
      OutStream << " OGR Initial Coastline file driver code                    \t: " << m_strOGRICDriverCode << endl;
      OutStream << " OGR Initial Coastline file data type                      \t: " << m_strOGRICDataType << endl;
      OutStream << " OGR Initial Coastline file data value                     \t: " << m_strOGRICDataValue << endl;
      OutStream << " OGR Initial Coastline file geometry                       \t: " << m_strOGRICGeometry << endl;
      OutStream << endl;
   }
   OutStream << endl;

   // -------------------------------------------------------- Other data --------------------------------------------------------
   OutStream << "Other Input Data" << endl;

   OutStream << " Initial still water level                                 \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::fixed) << setprecision(1) << m_dOrigStillWaterLevel << " m" << endl;
   OutStream << " Wave period                                               \t: " << m_dWavePeriod << " s" << endl;
   OutStream << " Offshore wave height                                      \t: " << m_dOffshoreWaveHeight << " m" << endl;
   OutStream << " Offshore wave orientation                                 \t: " << m_dOffshoreWaveOrientationIn << " degrees" << endl;
   OutStream << " Tide data file                                            \t: " << m_strTideDataFile << endl;
   OutStream << " R value                                                   \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::scientific) << m_dR << endl;
   OutStream << " Do along-shore sediment transport?                        \t: " << (m_bDoAlongshoreTransport ? "Y": "N") << endl;
   OutStream << " Beach protection factor                                   \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::fixed) << m_dBeachProtectionFactor << endl;
   OutStream << " Fine-sized sediment erodibility                           \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::fixed) << setprecision(1) << m_dFineErodibility << endl;
   OutStream << " Sand-sized sediment erodibility                           \t: " << resetiosflags(ios::floatfield) << m_dSandErodibility << endl;
   OutStream << " Coarse-sized sediment erodibility                         \t: " << m_dCoarseErodibility << endl;
   OutStream << " Do cliff collapse?                                        \t: " << (m_bDoCliffCollapse ? "Y": "N") << endl;
   OutStream << " Cliff erodibility                                         \t: " << m_dCliffErodibility << endl;
   OutStream << " Notch overhang to initiate collapse                       \t: " << m_dNotchOverhangAtCollapse << " m" << endl;
   OutStream << " Notch base below still water level                        \t: " << m_dNotchBaseBelowStillWaterLevel << " m" << endl;
   OutStream << " Scale parameter A for cliff deposition                    \t: ";
   if (m_dCliffDepositionA == 0)
      OutStream << "auto";
   else
      OutStream << m_dCliffDepositionA << "  m^(1/3)";
   OutStream << endl;
   OutStream << " Planview width of cliff deposition talus                  \t: " << m_nCliffDepositionPlanviewWidth << " cells" << endl;
   OutStream << " Planview length of cliff deposition talus                 \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::fixed) << m_dCliffDepositionPlanviewLength << " m" << endl;
   OutStream << " Height of talus at land end (fraction of cliff elevation) \t: " << m_dCliffDepositionHeightFrac << endl;
   OutStream << endl;

   OutStream << " Spacing of coastline normals                              \t: " << resetiosflags(ios::floatfield) << setiosflags(ios::fixed) << m_dCoastNormalAvgSpacing << " m" << endl;
   OutStream << " Length of coastline normals                               \t: " << m_dCoastNormalLength << " m" << endl;
   if (m_dCoastNormalRandSpaceFact > 0)
      OutStream << " Random factor for spacing of coastline normals            \t: " << m_dCoastNormalRandSpaceFact << " m" << endl;
   else
      OutStream << " Spacing of coastline normals is deterministic" << endl;
   OutStream << " Interval for coastline curvature calculations             \t: " << m_dCoastCurvatureInterval << endl;
   OutStream << endl;
/*
   OutStream << setiosflags(ios::fixed) << setprecision(8);
   OutStream << " Erosion potential shape function:" << endl;
   OutStream << "\tDepth over DB\tErosion potential\tFirst derivative of erosion potential" << endl;
   for (unsigned int i = 0; i < m_VdDepthOverDB.size(); i++)
      OutStream << "\t" << m_VdDepthOverDB[i] << "\t\t" << m_VdErosionPotential[i] << "\t\t" << m_VdErosionPotentialFirstDeriv[i] << endl;
   OutStream << endl;
*/
   // ------------------------------------------------------ Testing only --------------------------------------------------------
   OutStream << "Testing only" << endl;

   OutStream << " Output profile data?                                      \t: " << (m_bOutputProfileData ? "Y": "N") << endl;
   OutStream << " Profile numbers to be saved                               \t: ";
   for (unsigned int i = 0; i < m_VnProfileToSave.size(); i++)
      OutStream << m_VnProfileToSave[i] << SPACE;
   OutStream << endl;
   OutStream << " Timesteps when profiles are saved                         \t: ";
   for (unsigned int i = 0; i < m_VulProfileTimestep.size(); i++)
      OutStream << m_VulProfileTimestep[i] << SPACE;
   OutStream << endl;
   OutStream << " Output parallel profile data?                             \t: " << (m_bOutputParallelProfileData ? "Y": "N") << endl;
   OutStream << " Output erosion potential look-up data?                    \t: " << (m_bOutputLookUpData ? "Y": "N");
   if (m_bOutputLookUpData)
      OutStream << " (see " << m_strOutPath << EROSIONPOTENTIALLOOKUPFILE << ")";
   OutStream << endl;
   OutStream << " Erode coast in alternate directions?                      \t: " << (m_bErodeCoastAlternateDir ? "Y": "N") << endl;

   OutStream << endl << endl;

   // -------------------------------------------------- Per-iteration output ----------------------------------------------------
   OutStream << setiosflags(ios::fixed) << setprecision(2);

   // Write per-iteration headers to .out file
   OutStream << PERITERHEAD << endl;
   OutStream << "Depths in metres, erosion and deposition in millimetres" << endl;
   OutStream << "GISn = GIS files saved as <filename>n." << endl;
   OutStream << endl;

   OutStream << PERITERHEAD1 << endl;
   OutStream << PERITERHEAD2 << endl;
   OutStream << PERITERHEAD3 << endl;
   OutStream << PERITERHEAD4 << endl;
}


/*==============================================================================================================================

 Write the results for this iteration to the .out file

==============================================================================================================================*/
bool CSimulation::bWritePerIterationResults(void)
{
   OutStream << resetiosflags(ios::floatfield);
   OutStream << setiosflags(ios::fixed) << setprecision(0);

   // Output iteration and simulated time info
   OutStream << setw(7) << m_ulIter;
   OutStream << setw(7) << m_dSimElapsed;                            // In hours
   OutStream << setiosflags(ios::fixed) << setprecision(4);
   OutStream << setw(8) << m_dSimElapsed / (24 * 365.25);            // In years

   // Output per-iteration hydrology, as an average depth (m) per sea cell
   OutStream << setiosflags(ios::fixed) << setprecision(4);
   double dAvgSeaDepth = m_dThisIterTotSeaDepth / m_ulThisIterNSeaCells;
   OutStream << setw(10) << dAvgSeaDepth;
   OutStream << " ";

   // Calculate change in sea depth for this iteration (m)
   static double dLastAvgSeaDepth = 0;
   double dOffEdge = dAvgSeaDepth - dLastAvgSeaDepth;
   OutStream << setw(8) << (dLastAvgSeaDepth == 0 ? 0 : dOffEdge);
   dLastAvgSeaDepth = dAvgSeaDepth;
   OutStream << " ";

   // Output the this-iteration % of sea cells with potential erosion
   OutStream << setiosflags(ios::fixed) << setprecision(2);
   OutStream << setw(7) << 100 * static_cast<double>(m_ulThisIterNPotentialErosionCells) / m_ulThisIterNSeaCells;

   // Output per-iteration potential erosion in mm (average for all sea cells)
   OutStream << setw(7) << 1000 * m_dThisIterPotentialErosion / m_ulThisIterNSeaCells;

   // Output per-iteration potential erosion in mm (average for all cells with potential erosion)
   if (m_ulThisIterNPotentialErosionCells > 0)
      OutStream << setw(8) << 1000 * m_dThisIterPotentialErosion / m_ulThisIterNPotentialErosionCells;
   else
      OutStream << setw(8) << SPACE;

   // Output the this-iteration % of sea cells with actual erosion
   OutStream << setw(8) << 100 * static_cast<double>(m_ulThisIterNActualErosionCells) / m_ulThisIterNSeaCells;

   // Output per-iteration actual erosion in mm (average for all sea cells)
   OutStream << setw(8) << 1000 * m_dThisIterActualErosion / m_ulThisIterNSeaCells;

   // Output per-iteration actual erosion in mm (average for all cells with actual erosion)
   if (m_ulThisIterNActualErosionCells > 0)
      OutStream << setw(8) << 1000 * m_dThisIterActualErosion / m_ulThisIterNActualErosionCells;
   else
      OutStream << setw(8) << SPACE;

   OutStream << setiosflags(ios::fixed) << setprecision(2);

   // Output per-iteration actual erosion in mm (average for all sea cells)
   OutStream << setw(7) << 1000 * m_dThisIterActualFineErosion / m_ulThisIterNSeaCells;
   OutStream << setw(7) << 1000 * m_dThisIterActualSandErosion / m_ulThisIterNSeaCells;
   OutStream << setw(7) << 1000 * m_dThisIterActualCoarseErosion / m_ulThisIterNSeaCells;

   // Output per-iteration cliff collapse in mm (average for all coast cells)
   OutStream << setw(7) << 1000 * m_dThisIterCliffCollapseFine / m_ulThisIterNCoastCells;
   OutStream << setw(7) << 1000 * m_dThisIterCliffCollapseSand / m_ulThisIterNCoastCells;
   OutStream << setw(7) << 1000 * m_dThisIterCliffCollapseCoarse / m_ulThisIterNCoastCells;

   // Output per-iteration deposition in mm (average for all sea cells)
   OutStream << setw(7) << 1000 * m_dThisIterFineDeposition / m_ulThisIterNSeaCells;
   OutStream << setw(7) << 1000 * m_dThisIterSandDeposition / m_ulThisIterNSeaCells;
   OutStream << setw(7) << 1000 * m_dThisIterCoarseDeposition / m_ulThisIterNSeaCells;

   // Output per-iteration suspended sediment in mm (average for all sea cells)
   OutStream << setw(8) << 1000 * m_dThisIterSuspendedSediment / m_ulThisIterNSeaCells;

   OutStream << " ";

   // Finally, set 'markers' for events that have occurred this iteration
   if (m_bSaveGISThisIter)
      OutStream << " GIS" << m_nGISSave;

   OutStream << endl;

   // Did a text file write error occur?
   if (OutStream.fail())
      return false;

   return true;
}

/*==============================================================================================================================

 Write the results for this iteration to the time series CSV files

==============================================================================================================================*/
bool CSimulation::bWriteTSFiles(void)
{
   if (m_bSeaAreaTS)
   {
      // Output in external CRS units
      SeaAreaTSStream << m_dSimElapsed << "\t,\t" << m_dExtCRSGridArea * m_ulThisIterNSeaCells / static_cast<double>(m_ulNCells) << endl;

      // Did a time series file write error occur?
      if (SeaAreaTSStream.fail())
         return false;
   }


   if (m_bStillWaterLevelTS)
   {
      // Output as is (m)
      StillWaterLevelTSStream << m_dSimElapsed << "\t,\t" << m_dThisIterStillWaterLevel << endl;

      // Did a time series file write error occur?
      if (StillWaterLevelTSStream.fail())
         return false;
   }

   if (m_bErosionTS)
   {
      // Output as is (m depth equivalent)
      ErosionTSStream << m_dSimElapsed  << "\t,\t" << m_dThisIterActualFineErosion << ",\t" << m_dThisIterActualSandErosion << ",\t" << m_dThisIterActualCoarseErosion << endl;

      // Did a time series file write error occur?
      if (ErosionTSStream.fail())
         return false;
   }

   if (m_bDepositionTS)
   {
      // Output as is (m depth equivalent)
      DepositionTSStream << m_dSimElapsed << "\t,\t" << m_dThisIterFineDeposition << ",\t" << m_dThisIterSandDeposition << ",\t" << m_dThisIterCoarseDeposition << endl;

      // Did a time series file write error occur?
      if (DepositionTSStream.fail())
         return false;
   }

   if (m_bSedLostFromGridTS)
   {
      // Output as is (m depth equivalent)
      SedLostTSStream << m_dSimElapsed << "\t,\t" << m_dThisIterSedLost << endl;

      // Did a time series file write error occur?
      if (SedLostTSStream.fail())
         return false;
   }

   if (m_bSuspSedTS)
   {
      // Output as is (m depth equivalent)
      SedLoadTSStream << m_dSimElapsed << "\t,\t" << m_dThisIterSuspendedSediment << endl;

      // Did a time series file write error occur?
      if (SedLoadTSStream.fail())
         return false;
   }

   return true;
}

/*==============================================================================================================================

 Output the erosion potential look-up values, for checking purposes

==============================================================================================================================*/
void CSimulation::WriteLookUpData(void)
{
   // Open the output file
   string strLookUpFile = m_strOutPath;
   strLookUpFile.append(EROSIONPOTENTIALLOOKUPFILE);
   ofstream LookUpOutStream;
   LookUpOutStream.open(strLookUpFile.c_str(), ios::out | ios::trunc);

   if (LookUpOutStream)
   {
      // File opened OK, so output the values
      LookUpOutStream << "DepthOverDB, \tErosionPotential" << endl;
      double dDepthOverBD = 0;
      for (unsigned int i = 0; i < m_VdErosionPotential.size(); i++)
      {
         LookUpOutStream << dDepthOverBD << ",\t" << dLookUpErosionPotential(dDepthOverBD) << endl;
         dDepthOverBD += DODBINCREMENT;
      }
      LookUpOutStream << endl;

      // And close the file
      LookUpOutStream.close();
   }
}


/*==============================================================================================================================

 Save a coastline-normal profile

==============================================================================================================================*/
int CSimulation::nSaveProfile(int const nProfile, int const nCoast, int const nProfSize, vector<double>* const pdVDistXY, vector<double>* const pdVZ, vector<double>* const pdVDepthOverDB, vector<double>* const pdVErosionPotentialFunc, vector<double>* const pdVSlope, vector<double>* const pdVRecessionXY, vector<double>* const pdVChangeElevZ, vector<C2DIPoint>* const pPtVGridProfile)
{
   // TODO make this more efficient, also give warnings if no profiles will be output
   for (unsigned int i = 0; i < m_VulProfileTimestep.size(); i++)
   {
      for (unsigned int j = 0; j < m_VnProfileToSave.size(); j++)
      {
         if ((m_ulIter == m_VulProfileTimestep[i]) && (nProfile == m_VnProfileToSave[j]))
         {
            if (! bWriteProfileData(nCoast, nProfile, nProfSize, pdVDistXY, pdVZ, pdVDepthOverDB, pdVErosionPotentialFunc, pdVSlope, pdVRecessionXY, pdVChangeElevZ, pPtVGridProfile))
               return RTN_ERR_PROFILEWRITE;
         }
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Writes values for a single profile, for checking purposes

==============================================================================================================================*/
bool CSimulation::bWriteProfileData(int const nCoast, int const nProfile, int const nProfSize, vector<double>* const pdVDistXY, vector<double>* const pdVZ, vector<double>* const pdVDepthOverDB, vector<double>* const pdVErosionPotentialFunc, vector<double>* const pdVSlope, vector<double>* const pdVRecessionXY, vector<double>* const pdVChangeElevZ, vector<C2DIPoint>* const pPtVGridProfile)
{
   string strFName = m_strOutPath;
   strFName.append("profile_");
   char szNumTmp[4] = "";
   pszLongToSz(nProfile, szNumTmp, 4);          // Pad with zeros
   strFName.append(pszTrimLeft(szNumTmp));
   strFName.append("_timestep_");
   pszLongToSz(m_ulIter, szNumTmp, 4);           // Pad with zeros
   strFName.append(pszTrimLeft(szNumTmp));
   strFName.append(".csv");

   ofstream OutProfStream;
   OutProfStream.open(strFName.c_str(), ios::out | ios::trunc);
   if (! OutProfStream)
   {
      // Error, cannot open file
      cerr << ERR << "cannot open " << strFName << " for output" << endl;
      return false;
   }

   OutProfStream << "\"Dist\", \"X\", \"Y\", \"Z (before erosion)\", \"Depth/DB\", \"Erosion Potential\", \"Slope\", \"Recession XY\", \"Change Elev Z\", \"Grid X\",  \"Grid Y\",  \"Weight\",  \"For profile " << nProfile << " from coastline " << nCoast << " at timestep " << m_ulIter << "\"" << endl;
   for (int i = 0; i < nProfSize; i++)
   {
      double dX = dGridXToExtCRSX(pPtVGridProfile->at(i).nGetX());
      double dY = dGridYToExtCRSY(pPtVGridProfile->at(i).nGetY());

      OutProfStream << pdVDistXY->at(i) << ",\t" << dX << ",\t" << dY << ",\t" << pdVZ->at(i) << ",\t" << pdVDepthOverDB->at(i) << ",\t" << pdVErosionPotentialFunc->at(i) << ",\t" << pdVSlope->at(i) << ",\t" << pdVRecessionXY->at(i) << ",\t" << pdVChangeElevZ->at(i) << ",\t" <<  pPtVGridProfile->at(i).nGetX() <<  ",\t" << pPtVGridProfile->at(i).nGetY() <<  ", \t" <<  endl;
   }

   OutProfStream.close();

   return true;
}


/*==============================================================================================================================

 Save a coastline-normal parallel profile

==============================================================================================================================*/
int CSimulation::nSaveParProfile(int const nProfile, int const nCoast, int const nParProfSize, int const nDirection, int const nDistFromProfile, vector<double>* const pdVDistXY, vector<double>* const pdVZ, vector<double>* const pdVDepthOverDB, vector<double>* const pdVErosionPotentialFunc, vector<double>* const pdVSlope, vector<double>* const pdVRecessionXY, vector<double>* const pdVChangeElevZ, vector<C2DIPoint>* const pPtVGridProfile)
{
   // TODO make this more efficient, also give warnings if no profiles will be output
   for (unsigned int i = 0; i < m_VulProfileTimestep.size(); i++)
   {
      for (unsigned int j = 0; j < m_VnProfileToSave.size(); j++)
      {
         if ((m_ulIter == m_VulProfileTimestep[i]) && (nProfile == m_VnProfileToSave[j]))
         {
            if (! bWriteParProfileData(nCoast, nProfile, nParProfSize, nDirection, nDistFromProfile, pdVDistXY, pdVZ, pdVDepthOverDB, pdVErosionPotentialFunc, pdVSlope, pdVRecessionXY, pdVChangeElevZ, pPtVGridProfile))
               return RTN_ERR_PROFILEWRITE;
         }
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Writes values for a single parallel profile, for checking purposes

==============================================================================================================================*/
bool CSimulation::bWriteParProfileData(int const nCoast, int const nProfile, int const nProfSize, int const nDirection, int const nDistFromProfile, vector<double>* const pdVDistXY, vector<double>* const pdVZ, vector<double>* const pdVDepthOverDB, vector<double>* const pdVErosionPotentialFunc, vector<double>* const pdVSlope, vector<double>* const pdVRecessionXY, vector<double>* const pdVChangeElevZ, vector<C2DIPoint>* const pPtVGridProfile)
{
   string strFName = m_strOutPath;
   strFName.append("profile_");
   char szNumTmp[4] = "";
   pszLongToSz(nProfile, szNumTmp, 4);          // Pad with zeros
   strFName.append(pszTrimLeft(szNumTmp));
   strFName.append("_parallel_");
   pszLongToSz(nDistFromProfile, szNumTmp, 4);  // Pad with zeros
   strFName.append(pszTrimLeft(szNumTmp));
   strFName.append((nDirection == 0 ? "_F" : "_B"));
   strFName.append("_timestep_");
   pszLongToSz(m_ulIter, szNumTmp, 4);          // Pad with zeros
   strFName.append(pszTrimLeft(szNumTmp));
   strFName.append(".csv");

   ofstream OutProfStream;
   OutProfStream.open(strFName.c_str(), ios::out | ios::trunc);
   if (! OutProfStream)
   {
      // Error, cannot open file
      cerr << ERR << "cannot open " << strFName << " for output" << endl;
      return false;
   }

   OutProfStream << "\"Dist\", \"X\", \"Y\", \"Z (before erosion)\", \"Depth/DB\", \"Erosion Potential\", \"Slope\", \"Recession XY\", \"Change Elev Z\", \"Grid X\",  \"Grid Y\",  \"Weight\",  \"For profile " << nProfile << " from coastline " << nCoast << " at timestep " << m_ulIter << "\"" << endl;
   for (int i = 0; i < nProfSize; i++)
   {
      double dX = dGridXToExtCRSX(pPtVGridProfile->at(i).nGetX());
      double dY = dGridYToExtCRSY(pPtVGridProfile->at(i).nGetY());

      OutProfStream << pdVDistXY->at(i) << ",\t" << dX << ",\t" << dY << ",\t" << pdVZ->at(i) << ",\t" << pdVDepthOverDB->at(i) << ",\t" << pdVErosionPotentialFunc->at(i) << ",\t" << pdVSlope->at(i) << ",\t" << pdVRecessionXY->at(i) << ",\t" << pdVChangeElevZ->at(i) << ",\t" <<  pPtVGridProfile->at(i).nGetX() <<  ",\t" << pPtVGridProfile->at(i).nGetY() <<  ", \t" << endl;
   }

   OutProfStream.close();

   return true;
}


