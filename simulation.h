/*!
 *
 * \class CSimulation
 * \brief This class runs CoastalME simulations
 * \details TODO This is a more detailed description of the CSimulation class
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file simulation.h
 * \brief Contains CSimulation definitions
 *
 */

#ifndef SIMULATION_H
#define SIMULATION_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "cell.h"
#include "coast.h"

class CRasterGrid;               // Forward declaration
class CCoast;
class CCliff;

class CSimulation
{
private:
   bool
      m_bBasementElevSave,
      m_bTopSurfSave,
      m_bSliceSave,
      m_bWaterDepthSave,
      m_bWaveHeightSave,
      m_bWaveAngleSave,
      m_bPotentialErosionSave,
      m_bActualErosionSave,
      m_bTotalPotentialErosionSave,
      m_bTotalActualErosionSave,
      m_bLandformSave,
      m_bInterventionSave,
      m_bSuspSedSave,
      m_bFineUnconsSedSave,
      m_bSandUnconsSedSave,
      m_bCoarseUnconsSedSave,
      m_bFineConsSedSave,
      m_bSandConsSedSave,
      m_bCoarseConsSedSave,
      m_bRasterCoastlineSave,
      m_bRasterNormalSave,
      m_bDistWeightSave,
      m_bActiveZoneSave,
      m_bCollapseSave,
      m_bTotCollapseSave,
      m_bCollapseDepositSave,
      m_bTotCollapseDepositSave,
      m_bSaveRegular,
      m_bCoastSave,
      m_bNormalsSave,
      m_bCoastCurvatureSave,
      m_bSeaAreaTS,
      m_bStillWaterLevelTS,
      m_bErosionTS,
      m_bDepositionTS,
      m_bSedLostFromGridTS,
      m_bSuspSedTS,
      m_bSaveGISThisIter,
      m_bOutputProfileData,
      m_bOutputParallelProfileData,
      m_bOutputLookUpData,
      m_bRandomCoastEdgeSearch,
      m_bErodeCoastAlternateDir,
      m_bDoAlongshoreTransport,
      m_bDoCliffCollapse;

   int
      m_nXGridMax,
      m_nYGridMax,
      m_nLayers,
      m_nCoastSmooth,
      m_nCoastSmoothWindow,
      m_nSavGolCoastPoly,
      m_nProfileSmoothWindow,
      m_nGISSave,
      m_nUSave,
      m_nThisSave,
      m_nDurationUnitsMult,
      m_nCoastMax,
      m_nNThisIterCliffCollapse,
      m_nNTotCliffCollapse,
      m_nCliffDepositionPlanviewWidth;

   unsigned long
      m_ulIter,
      m_ulTotIter,
      m_ulRandSeed[NRNG],
      m_ulNCells,
      m_ulThisIterNSeaCells,
      m_ulThisIterNCoastCells,
      m_ulThisIterNPotentialErosionCells,
      m_ulThisIterNActualErosionCells,
      m_ulTotPotErosionOnProfiles,
      m_ulTotPotErosionBetweenProfiles;

   double
      m_dExtCRSNorthWestX,
      m_dExtCRSSouthEastX,
      m_dExtCRSNorthWestY,
      m_dExtCRSSouthEastY,
      m_dExtCRSGridArea,
      m_dCellSide,
      m_dCellArea,
      m_dCellDiagonal,
      m_dSimDuration,                  // Duration of simulation, in hours
      m_dTimeStep,
      m_dSimElapsed,                   // Time simulated so far, in hours
      m_dRSaveTime,
      m_dRSaveInterval,
      m_dUSaveTime[SAVEMAX],
      m_dClkLast,                      // Last value returned by clock()
      m_dCPUClock,                     // Total elapsed CPU time
      m_dGeoTransform[6],
      m_dOrigStillWaterLevel,
      m_dThisIterStillWaterLevel,
      m_dMinStillWaterLevel,
      m_dMaxStillWaterLevel,
      m_dBreakingWaveHeight,
      m_dWavePeriod,
      m_dC_0,                          // Deep water wave speed (m/s)
      m_dL_0,                          // Deep water wave length (m)
      m_dOffshoreWaveHeight,
      m_dOffshoreWaveOrientationIn,
      m_dOffshoreWaveOrientation,
      m_dR,
      m_dBeachProtectionFactor,
      m_dFineErodibility,
      m_dSandErodibility,
      m_dCoarseErodibility,
      m_dCoastNormalAvgSpacing,
      m_dCoastNormalLength,
      m_dCoastNormalRandSpaceFact,
      m_dCoastCurvatureInterval,
      m_dThisIterTotSeaDepth,
      m_dThisIterPotentialErosion,
      m_dThisIterActualErosion,
      m_dThisIterActualFineErosion,
      m_dThisIterActualSandErosion,
      m_dThisIterActualCoarseErosion,
      m_dThisIterFineDeposition,
      m_dThisIterSandDeposition,
      m_dThisIterCoarseDeposition,
      m_dThisIterSuspendedSediment,
      m_dThisIterSedLost,
      m_dDepthOverDBMax,                                    // Used in erosion potential look-up function
      m_dTotPotErosionOnProfiles,
      m_dTotPotErosionBetweenProfiles,
      m_dProfileMaxSlope,
      m_dCliffErodibility,
      m_dNotchOverhangAtCollapse,
      m_dNotchBaseBelowStillWaterLevel,
      m_dCliffDepositionA,
      m_dCliffDepositionPlanviewLength,
      m_dCliffDepositionHeightFrac,
      m_dThisIterCliffCollapseFine,
      m_dThisIterCliffCollapseSand,
      m_dThisIterCliffCollapseCoarse,
      m_dThisIterCliffCollapseFineDeposition,
      m_dThisIterCliffCollapseSandDeposition,
      m_dThisIterCliffCollapseCoarseDeposition;

   // These grand totals are all long doubles, the aim is to minimize rounding errors when many very small numbers are added to a single much larger number, see e.g. http://www.ddj.com/cpp/184403224
   long double
      m_ldGTotPotentialErosion,
      m_ldGTotActualErosion,
      m_ldGTotFineActualErosion,
      m_ldGTotSandActualErosion,
      m_ldGTotCoarseActualErosion,
      m_ldGTotFineDeposition,
      m_ldGTotSandDeposition,
      m_ldGTotCoarseDeposition,
      m_ldGTotSedLost,
      m_ldGTotCliffCollapseFine,
      m_ldGTotCliffCollapseSand,
      m_ldGTotCliffCollapseCoarse,
      m_ldGTotCliffCollapseFineDeposition,
      m_ldGTotCliffCollapseSandDeposition,
      m_ldGTotCliffCollapseCoarseDeposition;

   string
      m_strCMEDir,
      m_strCMEIni,
      m_strMailAddress,
      m_strDataPathName,
      m_strRasterGISOutFormat,
      m_strVectorGISOutFormat,
      m_strInitialBasementDEMFile,
      m_strInitialLandformFile,
      m_strInitialInterventionFile,
      m_strInitialSuspSedimentFile,
      m_strInitialCoastlineFile,
      m_strShapeFunctionFile,
      m_strTideDataFile,
      m_strLogFile,
      m_strOutPath,
      m_strOutFile,
      m_strPalFile,
      m_strGDALBasementDEMDriverCode,           // Basement DEM (raster)
      m_strGDALBasementDEMDriverDesc,
      m_strGDALBasementDEMProjection,
      m_strGDALBasementDEMDataType,
      m_strGDALLDriverCode,                     // Initial Landform Class (raster)
      m_strGDALLDriverDesc,
      m_strGDALLProjection,
      m_strGDALLDataType,
      m_strGDALIDriverCode,                     // Initial Intervention Class (raster)
      m_strGDALIDriverDesc,
      m_strGDALIProjection,
      m_strGDALIDataType,
      m_strGDALIWDriverCode,                    // Initial Water Depth (raster)
      m_strGDALIWDriverDesc,
      m_strGDALIWProjection,
      m_strGDALIWDataType,
      m_strGDALISSDriverCode,                   // Initial Suspended Sediment (raster)
      m_strGDALISSDriverDesc,
      m_strGDALISSProjection,
      m_strGDALISSDataType,
      m_strOGRICDriverCode,                     // Initial Coastline (vector)
      m_strOGRICGeometry,
      m_strOGRICDataType,
      m_strOGRICDataValue,
      m_strGDALRasterOutputDriverLongname,
      m_strGDALRasterOutputDriverExtension,
      m_strOGRVectorOutputExtension,
      m_strRunName,
      m_strDurationUnits;

   struct RandState
   {
      unsigned long s1, s2, s3;
   } m_ulRState[NRNG];

   time_t
      m_tSysStartTime,
      m_tSysEndTime;

   ofstream
      OutStream,
      SeaAreaTSStream,
      StillWaterLevelTSStream,
      ErosionTSStream,
      DepositionTSStream,
      SedLostTSStream,
      SedLoadTSStream;

   vector<int>
      m_VnProfileToSave,
      m_VnSavGolIndexCoast;            // Savitzky-Golay shift index for the coastline vector(s)

   vector<unsigned long>
      m_VulProfileTimestep;

   vector<double>
      m_VdSliceElev,
      m_VdErosionPotential,            // For erosion potential lookup
      m_VdSavGolFCCoast,               // Savitzky-Golay filter coefficients for the coastline vector(s)
      m_VdSavGolFCProfile,             // Savitzky-Golay filter coefficients for the profile vectors
      m_VdTideData;                    // Tide data: one record per timestep, is the change (m) from still water level for that timestep

   vector<string>
      m_VstrInitialFineUnconsSedimentFile,
      m_VstrInitialSandUnconsSedimentFile,
      m_VstrInitialCoarseUnconsSedimentFile,
      m_VstrInitialFineConsSedimentFile,
      m_VstrInitialSandConsSedimentFile,
      m_VstrInitialCoarseConsSedimentFile,
      m_VstrGDALIUFDriverCode,          // Initial Fine Unconsolidated Sediment (raster)
      m_VstrGDALIUFDriverDesc,
      m_VstrGDALIUFProjection,
      m_VstrGDALIUFDataType,
      m_VstrGDALIUSDriverCode,          // Initial Sand Unconsolidated Sediment (raster)
      m_VstrGDALIUSDriverDesc,
      m_VstrGDALIUSProjection,
      m_VstrGDALIUSDataType,
      m_VstrGDALIUCDriverCode,          // Initial Coarse Unconsolidated Sediment (raster)
      m_VstrGDALIUCDriverDesc,
      m_VstrGDALIUCProjection,
      m_VstrGDALIUCDataType,
      m_VstrGDALICFDriverCode,          // Initial Fine Consolidated Sediment (raster)
      m_VstrGDALICFDriverDesc,
      m_VstrGDALICFProjection,
      m_VstrGDALICFDataType,
      m_VstrGDALICSDriverCode,          // Initial Sand Consolidated Sediment (raster)
      m_VstrGDALICSDriverDesc,
      m_VstrGDALICSProjection,
      m_VstrGDALICSDataType,
      m_VstrGDALICCDriverCode,          // Initial Coarse Consolidated Sediment (raster)
      m_VstrGDALICCDriverDesc,
      m_VstrGDALICCProjection,
      m_VstrGDALICCDataType;

   // The raster grid object
   CRasterGrid* m_pRasterGrid;

   // The coastline objects
   vector<CCoast> m_VCoast;

private:
   // Input and output routines
   int nHandleCommandLineParams(int, char* []);
   bool bReadIni(void);
   bool bReadRunData(void);
   bool bOpenLogFile(void);
   bool bSetUpTSFiles(void);
   void WriteRunDetails(void);
   bool bWritePerIterationResults(void);
   bool bWriteTSFiles(void);
   int nReadShapeFunction(void);
   int nReadTideData(void);
   int nSaveProfile(int const, int const, int const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<C2DIPoint>* const);
   bool bWriteProfileData(int const, int const, int const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<C2DIPoint>* const);
   int nSaveParProfile(int const, int const, int const, int const, int const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<C2DIPoint>* const);
   bool bWriteParProfileData(int const, int const, int const, int const, int const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<double>* const, vector<C2DIPoint>* const);
   void WriteLookUpData(void);

   // GIS input and output stuff
   int nReadBasementDEMData(void);
   int nReadRasterGISData(int const, int const);
   int nReadVectorGISData(int const);        // NO LONGER USED BUT MAY BE USEFUL SOMEDAY
   bool bWriteRasterGISFloat(int const, string const*, int const = 0);
   bool bWriteRasterGISInt(int const, string const*, double const = 0);
   bool bWriteVectorGIS(int const, string const*);

   // Initialization
   bool bCreateErosionPotentialLookUp(vector<double>*, vector<double>*, vector<double>*);

   // Top-level simulation routines
   int nUpdateIntervention(void);
   int nCalcExternalForcing(void);
   int nInitGridAndCalcStillWaterLevel(void);
   int nLocateCoastlineAndProfiles(void);
   int nDoAllPropagateWaves(void);
   int nDoAllCliffCollapse(void);
   int nDoCliffCollapse(CCliff*, double const, double&, double&, double&);
   int nDoCliffCollapseDeposition(CCliff*, double const, double const);
   int nLocateEstuaries(void);
   int nClassifyCoastlineProfiles(void);
   int nErodeAllCoasts(void);
   int nDoAllAlongshoreSedimentTransport(void);
   int nUpdateGrid(void);

   // Lower-level simulation routines
   void ClearRasterAndVectorCoastlines(void);
   int nTraceCoastline(void);
   void FindEdgePoints(int const, vector<bool>*, vector <int>*, vector<int>*, vector<int>*, vector<int>*, vector<C2DIPoint>*);
   bool bCheckEdgePoints(vector<C2DIPoint>*);
   bool bDoTracing(vector<bool>*, vector <int>*, vector<int>*, vector<int>*, vector<int>*, vector<C2DIPoint>*);
   void DoCoastCurvature(int const, int const);
   int nAssignCoastalLandforms(void);
   int nCreateCoastlineProfiles(void);
   int nAllCoastlineNormalProfilesToGrid(void);
   int nRasterizeCoastlineNormalProfile(vector<C2DPoint>* const, vector<C2DIPoint>*);
   int nRasterizeCliffCollapseProfile(vector<C2DPoint>* const, vector<C2DIPoint>*);
   int nCalcPotentialErosionOnProfile(int const, int const, int const, vector<C2DIPoint>*, vector<C2DPoint>*);
   int nCalcPotentialErosionOneSideOfProfile(int const, int const, int const, int const, vector<C2DIPoint>* const);
   void ConstructParallelProfile(int const, int const, int const, int const, int const, int const, vector<C2DIPoint>* const, vector<C2DIPoint>*, vector<C2DPoint>*);

   int nCalcActualErosionOnAllSeaCells(void);
   void DoActualErosionOnCell(int const, int const);
   double dLookUpErosionPotential(double const) const;
   C2DPoint PtChooseEndPoint(bool const, C2DPoint* const, C2DPoint* const, double const, double const, double const, double const);
   int nGetCoastNormalEndPoint(int const, int const, C2DPoint* const, double const, C2DPoint*);
   void LandformToGrid(int const, int const);
   void CalcWaveProperties(int const, int const, int const);
   void DoFluxOrientation(int const);
   void InterpolateWavePropertiesToCoastline(int const, int const, int const, int const);
   void InterpolateWavePropertiesToCells(int const, int const, int const, int const);
   double dCalcCurvature(int const, C2DPoint* const, C2DPoint* const, C2DPoint* const);

   // GIS utility routines
   bool bCheckRasterGISOutputFormat(void);
   bool bCheckVectorGISOutputFormat(void);
   bool bSaveAllRasterGISFiles(void);
   bool bSaveAllVectorGISFiles(void);
   bool bIsWithinGrid(int const, int const) const;
   bool bIsWithinGrid(C2DIPoint* const) const;
   double dGridXToExtCRSX(double const) const;
   double dGridYToExtCRSY(double const) const;
   double dExtCRSXToGridX(double const) const;
   double dExtCRSYToGridY(double const) const;
   int nExtCRSXToGridX(double const) const;
   int nExtCRSYToGridY(double const) const;
   double dGetLengthBetween(C2DPoint* const, C2DPoint* const);
   void KeepWithinGrid(int&, int&);
   C2DIPoint* PtiKeepWithinGrid(C2DIPoint*);
   C2DPoint* pPtExtCRSKeepWithinGrid(C2DPoint*);
   double dKeepWithin360(double dAngle);

   // Utility routines
   void AnnounceStart(void);
   void AnnounceLicence(void);
   void AnnounceReadBasementDEM(void) const;
   void AnnounceAddLayers(void) const;
   void AnnounceReadRasterFiles(void) const;
   void AnnounceReadVectorFiles(void) const;
   void AnnounceReadLGIS(void) const;
   void AnnounceReadIGIS(void) const;
   void AnnounceInitializing(void) const;
   void AnnounceReadInitialSuspSedGIS(void) const;
   void AnnounceReadInitialFineUnconsSedGIS(int const) const;
   void AnnounceReadInitialSandUnconsSedGIS(int const) const;
   void AnnounceReadInitialCoarseUnconsSedGIS(int const) const;
   void AnnounceReadInitialFineConsSedGIS(int const) const;
   void AnnounceReadInitialSandConsSedGIS(int const) const;
   void AnnounceReadInitialCoarseConsSedGIS(int const) const;
   void AnnounceReadTideData(void) const;
   void AnnounceReadShapeFunctionFile(void) const;
   void AnnounceAllocateMemory(void) const;
   void AnnounceIsRunning(void) const;
   void AnnounceSimEnd(void) const;
   void StartClock(void);
   bool bFindExeDir(char* pcArg);
   bool bTimeToQuit(void);
   int nDoTimeUnits(string const*);
   int nDoSimulationTimeMultiplier(string const*);
   double dGetTimeMultiplier(string const*);
   int nCheckForInstability(void);
   void UpdateGrandTotals(void);
   string strGetBuild(void);
   string strGetComputerName(void);
   void DoCPUClockReset(void);
   void CalcTime(double const);
   string strDispTime(double const, bool const, bool const) const;
   string strDispSimTime(double const) const;
   void AnnounceProgress(void);
   string strGetErrorText(int const);
   string strListRasterFiles(void) const;
   string strListVectorFiles(void) const;
   string strListTSFiles(void) const;
   void CalcProcessStats(void);
   bool bFPIsEqual(double const, double const, double const) const;
   bool bIsWhole(double const) const;
   void CalcSavitzkyGolayCoeffs(void);
   CLine LSmoothCoastSavitzkyGolay(CLine*, int const, int const) const;
   CLine LSmoothCoastRunningMean(CLine*, int const, int const) const;
   vector<double> dVSmoothProfileSlope(vector<double>*);
   vector<double> dVCalcProfileSlope(vector<C2DPoint>*, vector<double>*);
   vector<double> dVSmoothProfileSavitzkyGolay(vector<double>*, vector<double>*);
   vector<double> dVSmoothProfileRunningMean(vector<double>*);
   void CalcSavitzkyGolay(double [], int const, int const, int const, int const, int const);

   // Random number stuff
   unsigned long ulGetTausworthe(unsigned long const, unsigned long const, unsigned long const, unsigned long const, unsigned long const) const;
   void InitRand0(unsigned long const);
   void InitRand1(unsigned long const);
   unsigned long ulGetRand0(void);
   unsigned long ulGetRand1(void);
   unsigned long ulGetLCG(unsigned long const) const;            // Used by all generators
   double dGetRand0d1(void);
   int nGetRand0To(int const);
   int nGetRand1To(int const);
   double dGetRand0GaussPos(double const, double const);
   double dGetRand0Gaussian(void);
   double dGetCGaussianPDF(double const);
   void Rand1Shuffle(int*, int);
#ifdef RANDCHECK
   void CheckRand(void) const;
#endif

public:
   ofstream LogStream;
      int nNextCoastPoint;

   CSimulation(void);
   ~CSimulation(void);
   double dGetStillWaterLevel(void) const;
   double dGetCellSide(void) const;
   int nGetGridXMax(void) const;
   int nGetGridYMax(void) const;
   int nDoSimulation(int, char* []);            // can be const according to cppcheck
   void DoSimulationEnd(int const);

};
#endif // SIMULATION_H
