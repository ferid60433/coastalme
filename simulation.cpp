/*!
 *
 * \file simulation.cpp
 * \brief The start-of-simulation routine
 * \details TODO A more detailed description of this routine.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2016
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include "cme.h"
#include "simulation.h"

/*==============================================================================================================================

 The CSimulation constructor

==============================================================================================================================*/
CSimulation::CSimulation(void)
{
   // Initialization
   m_bBasementElevSave                 =
   m_bTopSurfSave                      =
   m_bSliceSave                        =
   m_bWaterDepthSave                   =
   m_bWaveHeightSave                   =
   m_bWaveAngleSave                    =
   m_bPotentialErosionSave             =
   m_bActualErosionSave                =
   m_bTotalPotentialErosionSave        =
   m_bTotalActualErosionSave           =
   m_bLandformSave                     =
   m_bInterventionSave                 =
   m_bSuspSedSave                      =
   m_bFineUnconsSedSave                =
   m_bSandUnconsSedSave                =
   m_bCoarseUnconsSedSave              =
   m_bFineConsSedSave                  =
   m_bSandConsSedSave                  =
   m_bCoarseConsSedSave                =
   m_bRasterCoastlineSave              =
   m_bRasterNormalSave                 =
   m_bDistWeightSave                   =
   m_bActiveZoneSave                   =
   m_bCollapseSave                     =
   m_bTotCollapseSave                  =
   m_bCollapseDepositSave              =
   m_bTotCollapseDepositSave           =
   m_bSaveRegular                      =
   m_bCoastSave                        =
   m_bNormalsSave                      =
   m_bCoastCurvatureSave               =
   m_bSeaAreaTS                        =
   m_bStillWaterLevelTS                =
   m_bErosionTS                        =
   m_bSuspSedTS                        =
   m_bSedLostFromGridTS                =
   m_bDepositionTS                     =
   m_bSaveGISThisIter                  =
   m_bOutputProfileData                =
   m_bOutputParallelProfileData        =
   m_bOutputLookUpData                 =
   m_bRandomCoastEdgeSearch            =
   m_bErodeCoastAlternateDir           =
   m_bDoAlongshoreTransport            =
   m_bDoCliffCollapse                  = false;

   m_nLayers                           =
   m_nCoastSmooth                      =
   m_nCoastSmoothWindow                =
   m_nSavGolCoastPoly                  =
   m_nProfileSmoothWindow              =
   m_nGISSave                          =
   m_nUSave                            =
   m_nThisSave                         =
   m_nXGridMax                         =
   m_nYGridMax                         =
   m_nCoastMax                         =
   m_nCoastCurvatureInterval           =
   m_nNThisIterCliffCollapse           =
   m_nNTotCliffCollapse                =
   m_nCliffDepositionPlanviewWidth     = 0;

   m_ulIter                            =
   m_ulTotIter                         =
   m_ulNCells                          =
   m_ulThisIterNSeaCells               =
   m_ulThisIterNCoastCells             =
   m_ulThisIterNPotentialErosionCells  =
   m_ulThisIterNActualErosionCells     =
   m_ulTotPotErosionOnProfiles         =
   m_ulTotPotErosionBetweenProfiles    = 0;

   for (int i = 0; i < NRNG; i++)
      m_ulRandSeed[i]  = 0;

   for (int i = 0; i < SAVEMAX; i++)
      m_dUSaveTime[i] = 0;

   m_dDurationUnitsMult                     =
   m_dExtCRSNorthWestX                      =
   m_dExtCRSSouthEastX                      =
   m_dExtCRSNorthWestY                      =
   m_dExtCRSSouthEastY                      =
   m_dExtCRSGridArea                        =
   m_dCellSide                              =
   m_dCellDiagonal                          =
   m_dCellArea                              =
   m_dSimDuration                           =
   m_dTimeStep                              =
   m_dSimElapsed                            =
   m_dRSaveTime                             =
   m_dRSaveInterval                         =
   m_dClkLast                               =
   m_dCPUClock                              =
   m_dThisIterStillWaterLevel               =
   m_dOrigStillWaterLevel                   =
   m_dBreakingWaveHeight                    =
   m_dWavePeriod                            =
   m_dC_0                                   =
   m_dL_0                                   =
   m_dOffshoreWaveHeight                    =
   m_dOffshoreWaveOrientationIn             =
   m_dOffshoreWaveOrientation               =
   m_dR                                     =
   m_dBeachProtectionFactor                 =
   m_dFineErodibility                       =
   m_dSandErodibility                       =
   m_dCoarseErodibility                     =
   m_dCoastNormalAvgSpacing                 =
   m_dCoastNormalLength                     =
   m_dCoastNormalRandSpaceFact              =
   m_dThisIterTotSeaDepth                   =
   m_dThisIterSedLost                       =
   m_dThisIterPotentialErosion              =
   m_dThisIterActualErosion                 =
   m_dThisIterActualFineErosion             =
   m_dThisIterActualSandErosion             =
   m_dThisIterActualCoarseErosion           =
   m_dThisIterFineDeposition                =
   m_dThisIterSandDeposition                =
   m_dThisIterCoarseDeposition              =
   m_dThisIterSuspendedSediment             =
   m_dDepthOverDBMax                        =
   m_dTotPotErosionOnProfiles               =
   m_dTotPotErosionBetweenProfiles          =
   m_dProfileMaxSlope                       =
   m_dCliffErodibility                      =
   m_dNotchOverhangAtCollapse               =
   m_dNotchBaseBelowStillWaterLevel         =
   m_dCliffDepositionA                      =
   m_dCliffDepositionPlanviewLength         =
   m_dCliffDepositionHeightFrac             =
   m_dThisIterCliffCollapseFine             =
   m_dThisIterCliffCollapseSand             =
   m_dThisIterCliffCollapseCoarse           =
   m_dThisIterCliffCollapseFineDeposition   =
   m_dThisIterCliffCollapseSandDeposition   =
   m_dThisIterCliffCollapseCoarseDeposition = 0;

   m_dMinStillWaterLevel                    = 9999;
   m_dMaxStillWaterLevel                    = -9999;

   for (int i = 0; i < 6; i++)
      m_dGeoTransform[i] = 0;

   m_ldGTotPotentialErosion              =
   m_ldGTotActualErosion                 =
   m_ldGTotFineActualErosion             =
   m_ldGTotSandActualErosion             =
   m_ldGTotCoarseActualErosion           =
   m_ldGTotFineDeposition                =
   m_ldGTotSandDeposition                =
   m_ldGTotCoarseDeposition              =
   m_ldGTotSedLost                       =
   m_ldGTotCliffCollapseFine             =
   m_ldGTotCliffCollapseSand             =
   m_ldGTotCliffCollapseCoarse           =
   m_ldGTotCliffCollapseFineDeposition   =
   m_ldGTotCliffCollapseSandDeposition   =
   m_ldGTotCliffCollapseCoarseDeposition = 0;

   for (int i = 0; i < 2; i++)
   {
      m_ulRState[i].s1                 =
      m_ulRState[i].s2                 =
      m_ulRState[i].s3                 = 0;
   }

   m_tSysStartTime                     =
   m_tSysEndTime                       = 0;

   m_pRasterGrid                       = NULL;
}

/*==============================================================================================================================

 The CSimulation destructor

==============================================================================================================================*/
CSimulation::~CSimulation(void)
{
   // Close output files if open
   if (LogStream && LogStream.is_open())
      LogStream.close();

   if (OutStream && OutStream.is_open())
      OutStream.close();

   if (SeaAreaTSStream && SeaAreaTSStream.is_open())
      SeaAreaTSStream.close();

   if (StillWaterLevelTSStream && StillWaterLevelTSStream.is_open())
      StillWaterLevelTSStream.close();

   if (ErosionTSStream && ErosionTSStream.is_open())
      ErosionTSStream.close();

   if (DepositionTSStream && DepositionTSStream.is_open())
      DepositionTSStream.close();

   if (SedLostTSStream && SedLostTSStream.is_open())
      SedLostTSStream.close();

   if (SedLoadTSStream && SedLoadTSStream.is_open())
      SedLoadTSStream.close();

   if (m_pRasterGrid)
      delete m_pRasterGrid;
}

double CSimulation::dGetStillWaterLevel(void) const
{
   return m_dThisIterStillWaterLevel;
}

double CSimulation::dGetCellSide(void) const
{
   return m_dCellSide;
}

int CSimulation::nGetGridXMax(void) const
{
   return m_nXGridMax;
}

int CSimulation::nGetGridYMax(void) const
{
   return m_nYGridMax;
}

/*==============================================================================================================================

 The nDoSimulation member function of CSimulation sets up and runs the simulation

==============================================================================================================================*/
int CSimulation::nDoSimulation(int nArg, char* pcArgv[])       // can be const according to cppcheck
{
#ifdef RANDCHECK
   CheckRand();
   return RTN_OK;
#endif

   // ================================================== initialization section ================================================
   // Hello, World!
   AnnounceStart();

   // Start the clock ticking
   StartClock();

   // Find out the folder in which the CoastalME executable sits, in order to open the .ini file (they are assumed to be in the same folder)
   if (! bFindExeDir(pcArgv[0]))
      return (RTN_ERR_CMEDIR);

   // Register all available GDAL raster drivers
   GDALAllRegister();

   // Register all available OGR vector drivers
   OGRRegisterAll();

   // Deal with command-line parameters
   int nRet = nHandleCommandLineParams(nArg, pcArgv);
   if (nRet != RTN_OK)
      return (nRet);

   // OK, we are off, tell the user about the licence
   AnnounceLicence();

   // Read the .ini file and get the name of the run-data file, and path for output etc.
   if (! bReadIni())
      return (RTN_ERR_INI);

   // We have the name of the run-data input file, so read it
   if (! bReadRunData())
      return (RTN_ERR_RUNDATA);

   // Check raster GIS output format
   if (! bCheckRasterGISOutputFormat())
      return (RTN_ERR_RASTER_GIS_OUT_FORMAT);

   // Check vector GIS output format
   if (! bCheckVectorGISOutputFormat())
      return (RTN_ERR_VECTOR_GIS_OUT_FORMAT);

   // Open log file
   if (! bOpenLogFile())
      return (RTN_ERR_LOGFILE);

   // Set up the time series output files
   if (! bSetUpTSFiles())
      return (RTN_ERR_TSFILE);

   // Initialize the random number generators
   InitRand0(m_ulRandSeed[0]);
   InitRand1(m_ulRandSeed[1]);

   // If we are doing Savitzky-Golay smoothing of the vector coastline(s), calculate the filter coefficients
   if (m_nCoastSmooth == SMOOTH_SAVITZKY_GOLAY)
      CalcSavitzkyGolayCoeffs();

   // Create the raster grid object
   m_pRasterGrid = new CRasterGrid(this);

   // Read in the basement DEM (NOTE MUST HAVE THIS FILE) and create the raster grid, then read in the basement DEM data to the array
   AnnounceReadBasementDEM();
   nRet = nReadBasementDEMData();
   if (nRet != RTN_OK)
      return nRet;

   // We have at least one filename for the first layer, so add the correct number of layers. Note the the number of layers does not change during the simulation: however layers can decrease in thickness until they have zero thickness
   AnnounceAddLayers();
   for (int nX = 0; nX < m_nXGridMax; nX++)
      for (int nY = 0; nY < m_nYGridMax; nY++)
         m_pRasterGrid->Cell[nX][nY].AddLayers(m_nLayers);

   // Tell the user what is happening then read in the layer files
   AnnounceReadRasterFiles();
   for (int nLayer = 0; nLayer < m_nLayers; nLayer++)
   {
      // Read in the initial fine unconsolidated sediment depth file(s)
      AnnounceReadInitialFineUnconsSedGIS(nLayer);
      nRet = nReadRasterGISData(FINE_UNCONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);

      // Read in the initial sand unconsolidated sediment depth file
      AnnounceReadInitialSandUnconsSedGIS(nLayer);
      nRet = nReadRasterGISData(SAND_UNCONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);

      // Read in the initial coarse unconsolidated sediment depth file
      AnnounceReadInitialCoarseUnconsSedGIS(nLayer);
      nRet = nReadRasterGISData(COARSE_UNCONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);

      // Read in the initial fine consolidated sediment depth file
      AnnounceReadInitialFineConsSedGIS(nLayer);
      nRet = nReadRasterGISData(FINE_CONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);

      // Read in the initial sand consolidated sediment depth file
      AnnounceReadInitialSandConsSedGIS(nLayer);
      nRet = nReadRasterGISData(SAND_CONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);

      // Read in the initial coarse consolidated sediment depth file
      AnnounceReadInitialCoarseConsSedGIS(nLayer);
      nRet = nReadRasterGISData(COARSE_CONS_RASTER, nLayer);
      if (nRet != RTN_OK)
         return (nRet);
   }

   // Read in the initial suspended sediment depth file
   AnnounceReadInitialSuspSedGIS();
   nRet = nReadRasterGISData(SUSP_SED_RASTER, 0);
   if (nRet != RTN_OK)
      return (nRet);

   // If required, read in the Landform class, and the Intervention class foreach cell. Otherwise calculate all/any of these during the first iteration using the identification rules
   if (! m_strInitialLandformFile.empty())
   {
      AnnounceReadLGIS();
      nRet = nReadRasterGISData(LANDFORM_RASTER, 0);
      if (nRet != RTN_OK)
         return (nRet);
   }

   if (! m_strInitialInterventionFile.empty())
   {
      AnnounceReadIGIS();
      nRet = nReadRasterGISData(INTERVENTION_RASTER, 0);
      if (nRet != RTN_OK)
         return (nRet);
   }

   // May wish to read in some vector files someday
/*   AnnounceReadVectorFiles();
   if (! m_strInitialCoastlineFile.empty())
   {
      AnnounceReadInitialCoastlineGIS();

      // Create a new coastline object
      CCoast CoastTmp;
      m_VCoast.push_back(CoastTmp);

      // Read in
      nRet = nReadVectorGISData(COAST_VEC);
      if (nRet != RTN_OK)
         return (nRet);
   } */

   // Read in the tide data
   if (! m_strTideDataFile.empty())
   {
      AnnounceReadTideData();
      nRet = nReadTideData();
      if (nRet != RTN_OK)
         return (nRet);
   }

   // Read in the erosion potential shape function data
   AnnounceReadShapeFunctionFile();
   nRet = nReadShapeFunction();
   if (nRet != RTN_OK)
      return (nRet);

   // Do we want to output the erosion potential look-up values, for checking purposes?
   if (m_bOutputLookUpData)
      WriteLookUpData();

   // Open OUT file
   OutStream.open(m_strOutFile.c_str(), ios::out | ios::trunc);
   if (! OutStream)
   {
      // Error, cannot open Out file
      cerr << ERR << "cannot open " << m_strOutFile << " for output" << endl;
      return (RTN_ERR_OUTFILE);
   }

   // Write beginning-of-run information to Out and Log files
   WriteStartRunDetails();

   // Start initializing
   AnnounceInitializing();

   // Misc initialization calcs
   m_ulNCells = m_nXGridMax * m_nYGridMax;
   m_nCoastMax = COASTMAX * tMax(m_nXGridMax, m_nYGridMax);    // Arbitrary but probably OK

    // ===================================================== The main loop ======================================================
   // Tell the user what is happening
   AnnounceIsRunning();
   while (true)
   {
      // Check that we haven't gone on too long: if not then update iteration number etc.
      if (bTimeToQuit())
         break;

      // Tell the user how the simulation is progressing
      AnnounceProgress();

      // Check to see if there is a new intervention in place: if so, update it on the RasterGrid array
      nRet = nUpdateIntervention();
      if (nRet != RTN_OK)
         return nRet;

      // Make changes to boundary cells of the RasterGrid array due to external forcing functions: still water level, tidal range,
      // tidal period, wave energy (height, period)
      nRet = nCalcExternalForcing();
      if (nRet != RTN_OK)
         return nRet;

      // PropagateTide();

      // Do per-iteration intialization: set up the grid cells ready for this timestep, also initialize per-iteration totals
      nRet = nInitGridAndCalcStillWaterLevel();
      if (nRet != RTN_OK)
         return nRet;

      // Now we know which cells are inundated we can locate the coastline, and set up the coastline-normal profiles
      nRet = nLocateAllCoastlinesAndProfiles();
      if (nRet != RTN_OK)
         return nRet;

      // Locate estuaries
      nRet = nLocateAllEstuaries();
      if (nRet != RTN_OK)
         return nRet;

      // For each estuary (if any):
      //   FindChannelNetwork();
      //   GetEstuaryEvents(Area, Volume);

      //   For each channel section:
      //      CreateEstuaryCrossSection();
      //      GetSectionsEvents(Geometry, Properties);

      // For each coastline-normal profile, get the region type (i.e. use classification rules to assign landform complex/landform/intervention categories)
      nRet = nClassifyCoastlineProfiles();
      if (nRet != RTN_OK)
         return nRet;

      // PropagateWind();

      // Propagate waves for this iteration
      nRet = nDoAllPropagateWaves();
      if (nRet != RTN_OK)
         return nRet;

      // Simulate erosional elevation change on every coastline-normal profile, and between profiles
      nRet = nDoAllShorePlatFormErosion();
      if (nRet != RTN_OK)
         return nRet;

      if (m_bDoCliffCollapse)
      {
         // Do cliff collapses for this iteration (if any)
         nRet = nDoAllCliffCollapse();
         if (nRet != RTN_OK)
            return nRet;
      }

      if (m_bDoAlongshoreTransport)
      {
         // Do along-shore sediment transport for this iteration
         nRet = nDoAllAlongshoreSedimentTransport();
         if (nRet != RTN_OK)
            return nRet;
      }

      // Update the raster grid due to the above changes, also update per-iteration and running totals
      nRet = nUpdateGrid();
      if (nRet != RTN_OK)
         return nRet;

      // Now save results, first the raster and vector GIS files if required
      m_bSaveGISThisIter = false;
      if ((m_bSaveRegular && (m_dSimElapsed >= m_dRSaveTime) && (m_dSimElapsed < m_dSimDuration)) || (! m_bSaveRegular && (m_dSimElapsed >= m_dUSaveTime[m_nThisSave])))
      {
         m_bSaveGISThisIter = true;

         // Save the values from the RasterGrid array into raster GIS files
         if (! bSaveAllRasterGISFiles())
            return (RTN_ERR_RASTER_FILE_WRITE);

         // Save the vector GIS files
         if (! bSaveAllVectorGISFiles())
            return (RTN_ERR_VECTOR_FILE_WRITE);
      }

      // Output per-iteration results to the .out file
      if (! bWritePerIterationResults())
         return (RTN_ERR_TEXTFILEWRITE);

      // Now output time series CSV stuff
      if (! bWriteTSFiles())
         return (RTN_ERR_TSFILEWRITE);

      // Next, check for consistency and instability
      nRet = nCheckForInstability();
      if (nRet != RTN_OK)
         return (nRet);

      // Update grand totals
      UpdateGrandTotals();

   }  // ================================================ End of main loop ======================================================

   // =================================================== post-loop tidying =====================================================
   // Tell the user what is happening
   AnnounceSimEnd();

   // Write end-of-run information to Out, Log and time-series files
   nRet = nWriteEndRunDetails();
   if (nRet != RTN_OK)
      return (nRet);

   return RTN_OK;
}

