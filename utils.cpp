/*!
 *
 * \file utils.cpp
 * \brief Utility routines
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

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public  License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include "cme.h"
#include "simulation.h"

/*==============================================================================================================================

 Handles command-line parameters

==============================================================================================================================*/
int CSimulation::nHandleCommandLineParams(int nArg, char* pcArgv[])
{
   for (int i = 1; i < nArg; i++)
   {
      string strArg = pcArgv[i];
#ifdef _WIN32
      // Swap any forward slashes to backslashes
      strArg = pstrChangeToBackslash(&strArg);
#endif

      // change to lower case
      strArg = strToLower(&strArg);

      if (strArg.find("--gdal") != string::npos)
      {
         // User wants to know what GDAL raster drivers are available
         cout << GDALDRIVERS << endl << endl;

         for (int i = 0; i < GDALGetDriverCount(); i++ )
         {
            GDALDriverH hDriver = GDALGetDriver(i);

            string strTmp(GDALGetDriverShortName(hDriver));
            strTmp.append("          ");
            strTmp.append(GDALGetDriverLongName(hDriver));

            cout << strTmp << endl;
         }
         return (RTN_HELPONLY);
      }

      else if (strArg.find("--about") != string::npos)
      {
         // User wants information about CoastalME
         cout << ABOUT << endl << endl;
         cout << THANKS << endl;

         return (RTN_HELPONLY);
      }

      // TODO handle other command line parameters e.g. path to .ini file, path to datafile

      else
      {
         // Display usage information
         cout << USAGE << endl;
         cout << USAGE1 << endl;
         cout << USAGE2 << endl;
         cout << USAGE3 << endl;
         cout << USAGE4 << endl;
         cout << USAGE5 << endl;

         return (RTN_HELPONLY);
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Tells the user that we have started the simulation

==============================================================================================================================*/
void CSimulation::AnnounceStart(void)
{
   cout << endl << PROGNAME << " for " << PLATFORM << " " << strGetBuild() << endl;
}


/*==============================================================================================================================

 Starts the clock ticking

==============================================================================================================================*/
void CSimulation::StartClock(void)
{
   // First start the 'CPU time' clock ticking
   if (static_cast<clock_t>(-1) == clock())
   {
      // There's a problem with the clock, but continue anyway
      LogStream << WARN << "CPU time not available" << endl;
      m_dCPUClock = -1;
   }
   else
   {
      // All OK, so get the time in m_dClkLast (this is needed to check for clock rollover on long runs)
      m_dClkLast = static_cast<double>(clock());
      m_dClkLast -= CLOCK_T_MIN;       // necessary if clock_t is signed to make m_dClkLast unsigned
   }

   // And now get the actual time we started
   time(&m_tSysStartTime);
}


/*==============================================================================================================================

 Finds the folder (directory) in which the CoastalME executable is located

==============================================================================================================================*/
bool CSimulation::bFindExeDir(char* pcArg)
{
   string strTmp;
   char szBuf[BUFSIZE];

#ifdef _WIN32
   if (0 != GetModuleFileName(NULL, szBuf, BUFSIZE))
      strTmp = szBuf;
   else
      // It failed, so try another approach
      strTmp = pcArg;
#else
//   char* pResult = getcwd(szBuf, BUFSIZE);          // Used to use this, but what if cwd is not the same as the CoastalME dir?

   if (-1 != readlink("/proc/self/exe", szBuf, BUFSIZE))
      strTmp = szBuf;
   else
      // It failed, so try another approach
      strTmp = pcArg;
#endif

   // Neither approach has worked, so give up
   if (strTmp.empty())
      return false;

   // It's OK, so trim off the executable's name
   int nPos = strTmp.find_last_of(PATH_SEPARATOR);
   m_strCMEDir = strTmp.substr(0, nPos+1);            // Note that this must be terminated with a backslash

   return true;
}


/*==============================================================================================================================

 Tells the user about the licence

==============================================================================================================================*/
void CSimulation::AnnounceLicence(void)
{
   cout << COPYRIGHT << endl << endl;
   cout << LINE << endl;
   cout << DISCLAIMER1 << endl;
   cout << DISCLAIMER2 << endl;
   cout << DISCLAIMER3 << endl;
   cout << DISCLAIMER4 << endl;
   cout << DISCLAIMER5 << endl;
   cout << DISCLAIMER6 << endl;
   cout << LINE << endl << endl;

   // Note endl not needed, ctime() seems to always output a trailing <cr>
   cout << STARTNOTICE << strGetComputerName() << " on " << ctime(&m_tSysStartTime);
   cout << INITNOTICE << endl;
}

/*==============================================================================================================================

 Given a string containing time units, this returns the appropriate multiplier

==============================================================================================================================*/
double CSimulation::dGetTimeMultiplier(string const* strIn)
{
   // First decide what the time units are
   int nTimeUnits = nDoTimeUnits(strIn);

   // Then return the correct multiplier, since m_dTimeStep is in hours
   switch (nTimeUnits)
   {
      case TIME_UNKNOWN:
         return TIME_UNKNOWN;
         break;

      case TIME_HOURS:
         return 1;                     // Multiplier for hours
         break;

      case TIME_DAYS:
         return 24;                    // Multiplier for days -> hours
         break;

      case TIME_MONTHS:
         return 24 * 30.416667;        // Multiplier for months -> hours (assume 30 + 5/12 day months, no leap years)
         break;

      case TIME_YEARS:
         return 24 * 365.25;           // Multiplier for years -> hours
         break;
   }

   return 0;
}

/*==============================================================================================================================

 Given a string containing time units, this sets up the appropriate multiplier and display units for the simulation

==============================================================================================================================*/
int CSimulation::nDoSimulationTimeMultiplier(string const* strIn)
{
   // First decide what the time units are
   int nTimeUnits = nDoTimeUnits(strIn);

   // Next set up the correct multiplier, since m_dTimeStep is in hours
   switch (nTimeUnits)
   {
      case TIME_UNKNOWN:
         return RTN_ERR_TIMEUNITS;
         break;

      case TIME_HOURS:
         m_dDurationUnitsMult = 1;                     // Multiplier for hours
         m_strDurationUnits = "hours";
         break;

      case TIME_DAYS:
         m_dDurationUnitsMult = 24;                    // Multiplier for days -> hours
         m_strDurationUnits = "days";
         break;

      case TIME_MONTHS:
         m_dDurationUnitsMult = 24 * 30.416667;        // Multiplier for months -> hours (assume 30 + 5/12 day months, no leap years)
         m_strDurationUnits = "months";
         break;

      case TIME_YEARS:
         m_dDurationUnitsMult = 24 * 365.25;           // Multiplier for years -> hours
         m_strDurationUnits = "years";
         break;
   }

   return RTN_OK;
}

/*==============================================================================================================================

 This finds time units in a string

==============================================================================================================================*/
int CSimulation::nDoTimeUnits(string const* strIn)
{
   if (strIn->find('h') != string::npos)            // Search for 'hours'
      return TIME_HOURS;
   else if (strIn->find('d') != string::npos)       // Search for 'days'
      return TIME_DAYS;
   else if (strIn->find('m') != string::npos)       // Search for 'months'
      return TIME_MONTHS;
   else if (strIn->find('y') != string::npos)       // Search for 'years'
      return TIME_YEARS;
   else
      return TIME_UNKNOWN;
}

/*==============================================================================================================================

 Opens the log file

==============================================================================================================================*/
bool CSimulation::bOpenLogFile(void)
{
   // Open in binary mode if just checking random numbers
#ifdef RANDCHECK
   LogStream.open(m_strLogFile.c_str(), ios::out | ios::binary | ios::trunc);
#else
   LogStream.open(m_strLogFile.c_str(), ios::out | ios::trunc);
#endif

   if (! LogStream)
   {
      // Error, cannot open log file
      cerr << ERR << "cannot open " << m_strLogFile << " for output" << endl;
      return false;
   }

   return true;
}

/*==============================================================================================================================

 Tells the user that we are now reading the DEM file

==============================================================================================================================*/
void CSimulation::AnnounceReadBasementDEM(void) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READBASEMENT << pstrChangeToForwardSlash(&m_strInitialBasementDEMFile) << endl;
#else
   cout << READBASEMENT << m_strInitialBasementDEMFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now allocating memory

==============================================================================================================================*/
void CSimulation::AnnounceAllocateMemory(void) const
{
   cout << ALLOCATEMEMORY << endl;
}


/*==============================================================================================================================

 Tells the user that we are now adding layers

==============================================================================================================================*/
void CSimulation::AnnounceAddLayers(void) const
{
   // Tell the user what is happening
   cout << ADDLAYERS << endl;
}


/*==============================================================================================================================

 Now reading raster GIS files

==============================================================================================================================*/
void CSimulation::AnnounceReadRasterFiles(void) const
{
   cout << READRASTERFILES << endl;
}


/*==============================================================================================================================

 Now reading vector GIS files

==============================================================================================================================*/
void CSimulation::AnnounceReadVectorFiles(void) const
{
   cout << READVECTORFILES << endl;
}


/*==============================================================================================================================

 Tells the user that we are now reading the Landscape category GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadLGIS(void) const
{
   // Tell the user what is happening
   if (! m_strInitialLandformFile.empty())
#ifdef _WIN32
      cout << READLFILE << pstrChangeToForwardSlash(&m_strInitialLandformFile) << endl;
#else
      cout << READLFILE << m_strInitialLandformFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the Intervention category GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadIGIS(void) const
{
   // Tell the user what is happening
   if (! m_strInitialInterventionFile.empty())
#ifdef _WIN32
      cout << READIFILE << pstrChangeToForwardSlash(&m_strInitialInterventionFile) << endl;
#else
      cout << READIFILE << m_strInitialInterventionFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial suspended sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSuspSedGIS(void) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READISSFILE << pstrChangeToForwardSlash(&m_strInitialSuspSedimentFile) << endl;
#else
   cout << READISSFILE << m_strInitialSuspSedimentFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial fine unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialFineUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READIFUCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialFineUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READIFUCSFILE << nLayer+1 << "): " << m_VstrInitialFineUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial sand unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSandUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READISUCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialSandUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READISUCSFILE << nLayer+1 << "): " << m_VstrInitialSandUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial coarse unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialCoarseUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READICUCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialCoarseUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READICUCSFILE << nLayer+1 << "): " << m_VstrInitialCoarseUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial fine consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialFineConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READIFCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialFineConsSedimentFile[nLayer]) << endl;
#else
   cout << READIFCSFILE << nLayer+1 << "): " << m_VstrInitialFineConsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial sand consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSandConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READISCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialSandConsSedimentFile[nLayer]) << endl;
#else
   cout << READISCSFILE << nLayer+1 << "): " << m_VstrInitialSandConsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial coarse consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialCoarseConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READICCSFILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialCoarseConsSedimentFile[nLayer]) << endl;
#else
   cout << READICCSFILE << nLayer+1 << "): " << m_VstrInitialCoarseConsSedimentFile[nLayer] << endl;
#endif
}

/*==============================================================================================================================

 Now reading tide data file

==============================================================================================================================*/
void CSimulation::AnnounceReadTideData(void) const
{
#ifdef _WIN32
      cout << READTIDEDATAFILE << pstrChangeToForwardSlash(&m_strTideDataFile) << endl;
#else
      cout << READTIDEDATAFILE << m_strTideDataFile << endl;
#endif
}

/*==============================================================================================================================

 Now reading shape function file

==============================================================================================================================*/
void CSimulation::AnnounceReadShapeFunctionFile(void) const
{
   cout << READSHAPEFUNCTIONFILE << endl;
}

/*==============================================================================================================================

 Tells the user that we are now initializing

==============================================================================================================================*/
void CSimulation::AnnounceInitializing(void) const
{
   // Tell the user what is happening
   cout << INITIALIZING << endl;
}


/*==============================================================================================================================

 Tell the user that the simulation is now running

==============================================================================================================================*/
void CSimulation::AnnounceIsRunning(void) const
{
   cout << RUNNOTICE << endl;
}

/*==============================================================================================================================

 Return a space-separated string containing the names of the raster GIS output files

==============================================================================================================================*/
string CSimulation::strListRasterFiles(void) const
{
   string strTmp;

   if (m_bBasementElevSave)
   {
      strTmp.append(BASEMENT_ELEV_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bTopSurfSave)
   {
      strTmp.append(SEDIMENT_TOP_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bWaterDepthSave)
   {
      strTmp.append(WATER_DEPTH_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bWaveHeightSave)
   {
      strTmp.append(WAVE_HEIGHT_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bWaveAngleSave)
   {
      strTmp.append(WAVE_ANGLE_CODE);
      strTmp.append(", ");
   }

   if (m_bPotentialErosionSave)
   {
      strTmp.append(POTENTIAL_EROSION_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bActualErosionSave)
   {
      strTmp.append(ACTUAL_EROSION_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bTotalPotentialErosionSave)
   {
      strTmp.append(TOTAL_POTENTIAL_EROSION_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bTotalActualErosionSave)
   {
      strTmp.append(TOTAL_ACTUAL_EROSION_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bLandformSave)
   {
      strTmp.append(LANDFORM_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bInterventionSave)
   {
      strTmp.append(INTERVENTION_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bSuspSedSave)
   {
      strTmp.append(SUSP_SED_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bFineUnconsSedSave)
   {
      strTmp.append(FINE_UNCONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bSandUnconsSedSave)
   {
      strTmp.append(SAND_UNCONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bCoarseUnconsSedSave)
   {
      strTmp.append(COARSE_UNCONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bFineConsSedSave)
   {
      strTmp.append(FINE_CONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bSandConsSedSave)
   {
      strTmp.append(SAND_CONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bCoarseConsSedSave)
   {
      strTmp.append(COARSE_CONS_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bRasterCoastlineSave)
   {
      strTmp.append(RASTER_COAST_CODE);
      strTmp.append(", ");
   }

   if (m_bRasterNormalSave)
   {
      strTmp.append(RASTER_COAST_NORMAL_CODE);
      strTmp.append(", ");
   }

   if (m_bDistWeightSave)
   {
      strTmp.append(DISTWEIGHT_RASTER_CODE);
      strTmp.append(", ");
   }

   if (m_bActiveZoneSave)
   {
      strTmp.append(ACTIVEZONE_CODE);
      strTmp.append(", ");
   }

   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}

/*==============================================================================================================================

 Return a space-separated string containing the names of the vector GIS output files

==============================================================================================================================*/
string CSimulation::strListVectorFiles(void) const
{
   string strTmp;

   if (m_bCoastSave)
   {
      strTmp.append(VECTOR_COAST_CODE);
      strTmp.append(", ");
   }

   if (m_bNormalsSave)
   {
      strTmp.append(VECTOR_NORMALS_CODE);
      strTmp.append(", ");
   }

   if (m_bCoastCurvatureSave)
   {
      strTmp.append(VECTOR_COAST_CURVATURE_CODE);
      strTmp.append(", ");
   }

   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}

/*==============================================================================================================================

 Return a space-separated string containing the names of the time series output files

==============================================================================================================================*/
string CSimulation::strListTSFiles(void) const
{
   string strTmp;

   if (m_bSeaAreaTS)
   {
      strTmp.append(SEAAREATSCODE);
      strTmp.append(", ");
   }

   if (m_bStillWaterLevelTS)
   {
      strTmp.append(STILLWATERLEVELCODE);
      strTmp.append(", ");
   }

   if (m_bErosionTS)
   {
      strTmp.append(EROSIONTSCODE);
      strTmp.append(", ");
   }

   if (m_bDepositionTS)
   {
      strTmp.append(DEPOSITIONTSCODE);
      strTmp.append(", ");
   }

   if (m_bSedLostFromGridTS)
   {
      strTmp.append(SEDLOSTFROMGRIDTSCODE);
      strTmp.append(", ");
   }

   if (m_bSuspSedTS)
   {
      strTmp.append(SUSPSEDTSCODE);
      strTmp.append(", ");
   }

   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}

/*==============================================================================================================================

 The bSetUpTSFiles member function sets up the time series files

==============================================================================================================================*/
bool CSimulation::bSetUpTSFiles(void)
{
   string strTSFile;

   if (m_bSeaAreaTS)
   {
      // Start with wetted area
      strTSFile = m_strOutPath;
      strTSFile.append(SEAAREATSNAME);
      strTSFile.append(CSVEXT);

      // Open wetted time-series CSV file
      SeaAreaTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! SeaAreaTSStream)
      {
         // Error, cannot open wetted area  time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bStillWaterLevelTS)
   {
      // Now still water level
      strTSFile = m_strOutPath;
      strTSFile.append(STILLWATERLEVELTSNAME);
      strTSFile.append(CSVEXT);

      // Open still water level time-series CSV file
      StillWaterLevelTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! StillWaterLevelTSStream)
      {
         // Error, cannot open still water level time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bErosionTS)
   {
      // Erosion (fine, sand, coarse)
      strTSFile = m_strOutPath;
      strTSFile.append(EROSIONTSNAME);
      strTSFile.append(CSVEXT);

      // Open erosion time-series CSV file
      ErosionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! ErosionTSStream)
      {
         // Error, cannot open erosion time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bDepositionTS)
   {
      // Flow deposition
      strTSFile = m_strOutPath;
      strTSFile.append(DEPOSITIONTSNAME);
      strTSFile.append(CSVEXT);

      // Open flow deposition time-series CSV file
      DepositionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! DepositionTSStream)
      {
         // Error, cannot open flow deposition time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bSedLostFromGridTS)
   {
      // Sediment loss
      strTSFile = m_strOutPath;
      strTSFile.append(SEDLOSSFROMGRIDTSNAME);
      strTSFile.append(CSVEXT);

      // Open sediment loss time-series CSV file
      SedLostTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! SedLostTSStream)
      {
         // Error, cannot open sediment loss time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bSuspSedTS)
   {
      // Sediment load
      strTSFile = m_strOutPath;
      strTSFile.append(SUSPSEDTSNAME);
      strTSFile.append(CSVEXT);

      // Open sediment load time-series CSV file
      SedLoadTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! SedLoadTSStream)
      {
         // Error, cannot open sediment load time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   return true;
}


/*==============================================================================================================================

 Checks to see if the simulation has gone on too long, amongst other things

==============================================================================================================================*/
bool CSimulation::bTimeToQuit(void)
{
   // Add timestep to the total time simulated so far
   m_dSimElapsed += m_dTimeStep;

   if (m_dSimElapsed >= m_dSimDuration)
   {
      // It is time to quit
      m_dSimElapsed = m_dSimDuration;
      AnnounceProgress();
      return true;
   }

   // Not quitting, so increment the iteration count, and recalc total iterations
   m_ulIter++;
   m_ulTotIter = static_cast<unsigned long>(dRound(m_dSimDuration / m_dTimeStep));

   // Check to see if we have done CLOCKCHKITER iterations: if so, it is time to reset the CPU time running total in case the clock()
   // function later rolls over
   if (0 == m_ulIter % CLOCKCHKITER)
      DoCPUClockReset();

   // Not yet time to quit
   return false;
}


/*==============================================================================================================================

 Update grand totals at the end of each iteration

==============================================================================================================================*/
void CSimulation::UpdateGrandTotals(void)
{
   m_ldGTotPotentialErosion    += m_dThisIterPotentialErosion;
   m_ldGTotActualErosion       += m_dThisIterActualErosion;

   m_ldGTotFineActualErosion   += m_dThisIterActualFineErosion;
   m_ldGTotSandActualErosion   += m_dThisIterActualSandErosion;
   m_ldGTotCoarseActualErosion += m_dThisIterActualCoarseErosion;

   m_ldGTotFineDeposition      += m_dThisIterFineDeposition;
   m_ldGTotSandDeposition      += m_dThisIterSandDeposition;
   m_ldGTotCoarseDeposition    += m_dThisIterCoarseDeposition;

   m_ldGTotSedLost             += m_dThisIterSedLost;

   m_ldGTotCliffCollapseFine   += m_dThisIterCliffCollapseFine;
   m_ldGTotCliffCollapseSand   += m_dThisIterCliffCollapseSand;
   m_ldGTotCliffCollapseCoarse += m_dThisIterCliffCollapseCoarse;

   // TODO check with Andres, do we need separate cliff collapse deposition per-iter totals?
   m_ldGTotCliffCollapseFine   += m_dThisIterCliffCollapseFineDeposition;
   m_ldGTotCliffCollapseSand   += m_dThisIterCliffCollapseSandDeposition;
   m_ldGTotCliffCollapseCoarse += m_dThisIterCliffCollapseCoarseDeposition;

   m_ldGTotCliffCollapseFineDeposition   += m_dThisIterCliffCollapseFineDeposition;
   m_ldGTotCliffCollapseSandDeposition   += m_dThisIterCliffCollapseSandDeposition;
   m_ldGTotCliffCollapseCoarseDeposition += m_dThisIterCliffCollapseCoarseDeposition;
}

/*==============================================================================================================================

 Returns a string, hopefully giving the name of the computer on which the simulation is running

==============================================================================================================================*/
string CSimulation::strGetComputerName(void)
{
   string strComputerName;

#ifdef _WIN32
   // Being compiled to run under Windows, either by MS VC++, Borland C++, or Cygwin
   strComputerName = getenv("COMPUTERNAME");
#else
   // Being compiled for another platform; assume for Linux-Unix
   char szHostName[BUFSIZE] = "";
   gethostname(szHostName, BUFSIZE);

   strComputerName = szHostName;
   if (strComputerName.empty())
      strComputerName = "Unknown Computer";
#endif

   return strComputerName;
}

/*==============================================================================================================================

 Resets the CPU clock timer to prevent it 'rolling over', as can happen during long runs. This is a particularly problem under Unix systems where the value returned by clock() is defined in microseconds (for compatibility with systems that have CPU clocks with much higher resolution) i.e. CLOCKS_PER_SEC is 1000000 rather than the more usual 1000. In this case, the value returned from clock() will wrap around after accumulating only 2147 seconds of CPU time (about 36 minutes).

==============================================================================================================================*/
void CSimulation::DoCPUClockReset(void)
{
   if (static_cast<clock_t>(-1) == clock())
   {
      // Error
      LogStream << "CPU time not available" << endl;
      m_dCPUClock = -1;
      return;
   }

   // OK, so carry on
   double dClkThis = static_cast<double>(clock());
   dClkThis -= CLOCK_T_MIN;   // necessary when clock_t is signed, to make dClkThis unsigned

   if (dClkThis < m_dClkLast)
   {
      // Clock has 'rolled over'
      m_dCPUClock += (CLOCK_T_RANGE + 1 - m_dClkLast);   // this elapsed before rollover
      m_dCPUClock += dClkThis;                           // this elapsed after rollover

#ifdef CLOCKCHECK
      // For debug purposes
      LogStream << "Rolled over: dClkThis=" << dClkThis << " m_dClkLast=" << m_dClkLast << endl << "\t" << " before rollover=" << (CLOCK_T_RANGE + 1 - m_dClkLast) << endl << "\t" << " after rollover=" << dClkThis << endl << "\t" << " ADDED=" << (CLOCK_T_RANGE + 1 - m_dClkLast + dClkThis) << endl;
#endif
   }
   else
   {
      // No rollover
      m_dCPUClock += (dClkThis - m_dClkLast);

#ifdef CLOCKCHECK
      // For debug purposes
      LogStream << "No rollover: dClkThis=" << dClkThis << " m_dClkLast=" << m_dClkLast << " ADDED=" << dClkThis - m_dClkLast << endl;
#endif
   }

   // Reset for next time
   m_dClkLast = dClkThis;
}


/*==============================================================================================================================

 Announce the end of the simulation

==============================================================================================================================*/
void CSimulation::AnnounceSimEnd(void) const
{
   cout << endl << FINALOUTPUT << endl;
}


/*==============================================================================================================================

 Calculates and displays time elapsed in terms of CPU time and real time, also calculates time per iteration in terms of both CPU time
 and real time

==============================================================================================================================*/
void CSimulation::CalcTime(double const dRunLength)
{
   // Reset CPU count for last time
   DoCPUClockReset();

   if (m_dCPUClock != -1)
   {
      // Calculate CPU time in secs
      double dDuration = m_dCPUClock/CLOCKS_PER_SEC;

      // And write CPU time out to OutStream and LogStream
      OutStream << "CPU time elapsed: " << strDispTime(dDuration, false, true);
      LogStream << "CPU time elapsed: " << strDispTime(dDuration, false, true);

      // Calculate CPU time per iteration
      double fPerIter = dDuration / m_ulTotIter;

      // And write CPU time per iteration to OutStream and LogStream
      OutStream << setiosflags(ios::fixed) << setprecision(4) << " (" << fPerIter << " per iteration)" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(4) << " (" << fPerIter << " per iteration)" << endl;

      // Calculate ratio of CPU time to time simulated
      OutStream << resetiosflags(ios::floatfield);
      OutStream << setiosflags(ios::fixed) << setprecision(0) << "In terms of CPU time, this is ";
      LogStream << resetiosflags(ios::floatfield);
      LogStream << setiosflags(ios::fixed) << setprecision(0) << "In terms of CPU time, this is ";
      if (dDuration > dRunLength)
      {
         OutStream << dDuration / dRunLength << " x slower than reality" << endl;
         LogStream << dDuration / dRunLength << " x slower than reality" << endl;
      }
      else
      {
         OutStream << dRunLength / dDuration << " x faster than reality" << endl;
         LogStream << dRunLength / dDuration << " x faster than reality" << endl;
      }
   }

   // Now calculate actual time of run (only really useful if run is a background batch job e.g. under Unix)
   time(&m_tSysEndTime);

   // Calculate run time
   double dDuration = difftime(m_tSysEndTime, m_tSysStartTime);

   // And write run time out to OutStream and LogStream
   OutStream << "Run time elapsed: " << strDispTime(dDuration, false, false);
   LogStream << "Run time elapsed: " << strDispTime(dDuration, false, false);

   // Calculate run time per iteration
   double fPerIter = dDuration / m_ulTotIter;

   // And write run time per iteration to OutStream and LogStream
   OutStream << resetiosflags(ios::floatfield);
   OutStream << " (" << setiosflags(ios::fixed) << setprecision(4) << fPerIter << " per iteration)" << endl;
   LogStream << resetiosflags(ios::floatfield);
   LogStream << " (" << setiosflags(ios::fixed) << setprecision(4) << fPerIter << " per iteration)" << endl;

   // Calculate ratio of run time to time simulated
   OutStream << "In terms of run time, this is ";
   LogStream << "In terms of run time, this is ";
   if (dDuration > dRunLength)
   {
      OutStream << setiosflags(ios::fixed) << setprecision(3) << dDuration / dRunLength << " x slower than reality" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(3) << dDuration / dRunLength << " x slower than reality" << endl;
   }
   else
   {
      OutStream << setiosflags(ios::fixed) << setprecision(3) << dRunLength / dDuration << " x faster than reality" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(3) << dRunLength / dDuration << " x faster than reality" << endl;
   }
}

/*==============================================================================================================================

 strDispSimTime returns a string formatted as year Julian_day hour, given a parameter in hours

==============================================================================================================================*/
string CSimulation::strDispSimTime(const double dTimeIn) const
{
   // Make sure no negative times
   double dTime = tMax(dTimeIn, 0.0);

   string strTime;

   unsigned long ulTimeIn = static_cast<unsigned long>(floor(dTime));

   // Constants
   double const dHoursInYear = 24 * 365.25;
   unsigned long const ulHoursInDay = 24;

   // Display years
   if (ulTimeIn >= dHoursInYear)
   {
      unsigned long ulYears = static_cast<unsigned long>(dRound(ulTimeIn / dHoursInYear));
      ulTimeIn -= static_cast<unsigned long>(dRound(ulYears * dHoursInYear));

      char szTmp[6] = "";
      strTime = pszTrimLeft(pszLongToSz(ulYears, szTmp, 6));
      strTime.append("y ");
   }
   else
      strTime = "0y ";

   // Display Julian days
   if (ulTimeIn >= ulHoursInDay)
   {
      unsigned long ulJDays = ulTimeIn / ulHoursInDay;
      ulTimeIn -= (ulJDays * ulHoursInDay);

      char szTmp[4] = "";
      pszLongToSz(ulJDays, szTmp, 4);              // Pad with zeros
      strTime.append(szTmp);
      strTime.append("d ");
   }
   else
      strTime.append("000 d ");

   // Display hours
   char szTmp[3] = "";
   pszLongToSz(ulTimeIn, szTmp, 3);                // Pad with zeros
   strTime.append(szTmp);
   strTime.append("h");

   return strTime;
}


/*==============================================================================================================================

 strDispTime returns a string formatted as h:mm:ss, given a parameter in seconds, with rounding and fractions of a second if desired

==============================================================================================================================*/
string CSimulation::strDispTime(const double dTimeIn, const bool bRound, const bool bFrac) const
{
   // Make sure no negative times
   double dTime = tMax(dTimeIn, 0.0);

   string strTime;

   if (bRound)
      dTime = dRound(dTime);

   unsigned long ulTimeIn = static_cast<unsigned long>(floor(dTime));
   dTime -= ulTimeIn;

   // Hours
   if (ulTimeIn >= 3600)
   {
      // Display some hours
      unsigned long ulHours = ulTimeIn / 3600ul;
      ulTimeIn -= (ulHours * 3600ul);

      char szTmp[6] = "";
      strTime = pszTrimLeft(pszLongToSz(ulHours, szTmp, 6, 10));
      strTime.append(":");
   }
   else
      strTime = "0:";

   // Minutes
   if (ulTimeIn >= 60)
   {
      // display some minutes
      unsigned long ulMins = ulTimeIn / 60ul;
      ulTimeIn -= (ulMins * 60ul);

      char szTmp[3] = "";
      pszLongToSz(ulMins, szTmp, 3);            // Pad with zeros
      strTime.append(szTmp);
      strTime.append(":");
   }
   else
      strTime.append("00:");

   // Seconds
   char szTmp[3] = "";
   pszLongToSz(ulTimeIn, szTmp, 3);             // Pad with zeros
   strTime.append(szTmp);

   if (bFrac)
   {
      // Fractions of a second
      pszLongToSz(static_cast<unsigned long>(dTime * 100), szTmp, 3);
      strTime.append(".");
      strTime.append(szTmp);
   }

   return strTime;
}


/*==============================================================================================================================

 Returns the date and time on which the program was compiled

==============================================================================================================================*/
string CSimulation::strGetBuild(void)
{
   string strBuild("(");
   strBuild.append(__TIME__);
   strBuild.append(" ");
   strBuild.append(__DATE__);
#ifdef _DEBUG
   strBuild.append(" DEBUG");
#endif
   strBuild.append(" build)");

   return strBuild;
}


/*==============================================================================================================================

 Displays information regarding the progress of the simulation

==============================================================================================================================*/
void CSimulation::AnnounceProgress(void)
{
   if (isatty(1))
   {
      // Stdout is connected to a tty, so not running as a background job
      static double sdElapsed = 0;
      static double sdToGo = 0;

      // Update time elapsed and time remaining every nInterval iterations
      time_t tNow;
      time(&tNow);

      // Calculate time elapsed and remaining
      sdElapsed = difftime(tNow, m_tSysStartTime);
      sdToGo = (sdElapsed * m_dSimDuration / m_dSimElapsed) - sdElapsed;

      // Tell the user about progress (note need to make several separate calls to cout here, or MS VC++ compiler appears to get confused)
      cout << SIMULATING << strDispSimTime(m_dSimElapsed);
      cout << setiosflags(ios::fixed) << setprecision(3) << setw(9) << 100 * m_dSimElapsed / m_dSimDuration;
      cout << "%   (elapsed " << strDispTime(sdElapsed, false, false) << " remaining ";

      cout << strDispTime(sdToGo, false, false) << ")  ";
      cout.flush();
   }
}

/*==============================================================================================================================

 This routine checks for consistency and instability during the simulation: if any of the per-iteration totals are infeasibly large, the routine return an error code

==============================================================================================================================*/
int CSimulation::nCheckForInstability(void)
{
   // TODO reinstate this
/*
   // First check for consistency
   if (! bFPIsEqual(m_dThisIterActualFineErosion, m_dThisIterFineDeposition, TOLERANCE))
   {
      cerr << ERR << "unequal totals of fine sediment eroded and deposited during iteration " << m_ulIter << endl;
      return RTN_ERR_MASSBALANCE;
   }

   if (! bFPIsEqual(m_dThisIterActualSandErosion, m_dThisIterSandDeposition, TOLERANCE))
   {
      cerr << ERR << "unequal totals of sand sediment eroded and deposited during iteration " << m_ulIter << endl;
      return RTN_ERR_MASSBALANCE;
   }

   if (! bFPIsEqual(m_dThisIterActualCoarseErosion, m_dThisIterCoarseDeposition, TOLERANCE))
   {
      cerr << ERR << "unequal totals of coarse sediment eroded and deposited during iteration " << m_ulIter << endl;
      return RTN_ERR_MASSBALANCE;
   }

   // If an absurd per-cell depth of sediment has been eroded during this iteration, then abort the run
//   if (tAbs(m_dThisIterErosion) / m_ulNCells > FLOW_DETACH_MAX)
//      return RTN_ERR_EROSIONMAX;

   // If an absurd per-cell average depth of sediment has been deposited during this iteration, then abort the run
//   if (tAbs(m_dThisIterDeposition) / m_ulNCells > FLOW_DEPOSIT_MAX)
//      return RTN_ERR_DEPOSITIONMAX;

   // If an absurd per-cell average depth of sediment is being transported during this iteration, then abort the run
//   if (tAbs(m_dThisIterSedLoad) / m_ulNCells > SED_TRANSPORT_MAX)
//      return RTN_ERR_SEDLOADMAX;
*/
   // All OK
   return RTN_OK;
}

bool CSimulation::bFPIsEqual(double const d1, double const d2, double const dEpsilon) const
{
   // For comparison of two floating-point numbers: since the accuracy of floating-point numbers varies with their magnitude, must compare them by using an accuracy threshold which is relative to the magnitude of the two numbers being compared. This is a blend of an example from Knuth's 'The Art of Computer Programming. Volume 1. Fundamental Algorithms' and a posting dated 18 Nov 93 by rmartin@rcmcon.com (Robert Martin), archived in cpp_tips
   if ((0 == d1) && (tAbs(d2) < dEpsilon))
      return true;
   else if ((0 == d2) && (tAbs(d1) < dEpsilon))
      return true;
   else
      return ((tAbs(d1 - d2) < (dEpsilon * tAbs(d1))) ? true : false);
}

bool CSimulation::bIsWhole(double d) const
{
   // From http://answers.yahoo.com/question/index?qid=20110320132617AAMdb7u
   return (static_cast<int>(d) == d);
}

unsigned long CSimulation::ulGetTausworthe(unsigned long const ulS, unsigned long const ulA, unsigned long const ulB, unsigned long const ulC, unsigned long const ulD) const
{
   return (((ulS & ulC) << ulD) & MASK) ^ ((((ulS << ulA) & MASK) ^ ulS) >> ulB);
}

double CSimulation::dGetRand0d1(void)
{
   // Uses ulGetRand0() to return a double precision floating point number uniformly distributed in the range [0, 1) i.e. includes 0.0 but excludes 1.0. Based on a routine in taus.c from gsl-1.2
   return (ulGetRand0() / 4294967296.0);
}

int CSimulation::nGetRand0To(int const nBound)
{
   // Uses ulGetRand0() to return a random integer uniformly distributed in the range [0, nBound) i.e. includes 0 but excludes nBound
   int nRtn;
   unsigned long ulScale = 4294967295ul / nBound;                 // nBound must be > 1
   do
   {
      nRtn = ulGetRand0() / ulScale;
   }
   while (nRtn >= nBound);
   return (nRtn);
}

int CSimulation::nGetRand1To(int const nBound)
{
   // As above, but uses ulGetRand1()
   int nRtn;
   unsigned long ulScale = 4294967295ul / nBound;                 // nBound must be > 1
   do
   {
      nRtn = ulGetRand1() / ulScale;
   }
   while (nRtn >= nBound);
   return (nRtn);
}

double CSimulation::dGetRand0GaussPos(double const dMean, double const dStd)
{
   // Uses ulGetRand0()
   return (tMax((dGetRand0Gaussian() * dStd) + dMean, 0.0));
}

/*==============================================================================================================================

 This calculates and displays process statistics

==============================================================================================================================*/
void CSimulation::CalcProcessStats(void)
{
   string const NA = "Not available";

   OutStream << endl;
   OutStream << "Process statistics" << endl;
   OutStream << "------------------" << endl;

#ifdef _WIN32
   // First, find out which version of Windows we are running under
   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));              // fill this much memory with zeros
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if (! (bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi)))
   {
      // OSVERSIONINFOEX didn't work so try OSVERSIONINFO instead
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

      if (! GetVersionEx((OSVERSIONINFO *) &osvi))
      {
         // That didn't work either, too risky to proceed so give up
         OutStream << NA << endl;
         return;
      }
   }

   // OK, we have Windows version so display it
   OutStream << "Running under                                \t: ";
   switch (osvi.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_NT:
         if (osvi.dwMajorVersion <= 4)
            OutStream << "Windows NT ";
         else if (5 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
            OutStream << "Windows 2000 ";
         else if (5 == osvi.dwMajorVersion && 1 == osvi.dwMinorVersion)
            OutStream << "Windows XP ";
         else if (6 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
            OutStream << "Windows Vista ";
         else if (6 == osvi.dwMajorVersion && 1 == osvi.dwMinorVersion)
            OutStream << "Windows 7 ";
         else if (6 == osvi.dwMajorVersion && 2 == osvi.dwMinorVersion)
            OutStream << "Windows 8 ";
         // TODO add info for other Windows version
         else
            OutStream << "unknown Windows version ";

         // Display version, service pack (if any), and build number
         if (osvi.dwMajorVersion <= 4)
            // TODO does this still work on 64-bit platforms?
            OutStream << "version " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " " << osvi.szCSDVersion << " (Build " << (osvi.dwBuildNumber & 0xFFFF) << ")" << endl;
         else
            // TODO does this still work on 64-bit platforms?
            OutStream << osvi.szCSDVersion << " (Build " << (osvi.dwBuildNumber & 0xFFFF) << ")" << endl;
         break;

      case VER_PLATFORM_WIN32_WINDOWS:
         if (4 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
         {
             OutStream << "Windows 95";
             if ('C' == osvi.szCSDVersion[1] || 'B' == osvi.szCSDVersion[1])
                OutStream << " OSR2";
             OutStream << endl;
         }
         else if (4 == osvi.dwMajorVersion && 10 == osvi.dwMinorVersion)
         {
             OutStream << "Windows 98";
             if ('A' == osvi.szCSDVersion[1])
                OutStream << "SE";
             OutStream << endl;
         }
         else if (4 == osvi.dwMajorVersion && 90 == osvi.dwMinorVersion)
             OutStream << "Windows Me" << endl;
         else
             OutStream << "unknown 16-bit Windows version " << endl;
         break;

      case VER_PLATFORM_WIN32s:
         OutStream << "Win32s" << endl;
         break;
   }

   // Now get process timimgs: this only works under 32-bit windows
   if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
   {
      FILETIME ftCreate, ftExit, ftKernel, ftUser;
      if (GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser))
      {
         ULARGE_INTEGER ul;
         ul.LowPart = ftUser.dwLowDateTime;
         ul.HighPart = ftUser.dwHighDateTime;
         OutStream << "Time spent executing user code               \t: " << strDispTime(static_cast<double>(ul.QuadPart) * 1e-7, false, true) << endl;
         ul.LowPart = ftKernel.dwLowDateTime;
         ul.HighPart = ftKernel.dwHighDateTime;
         OutStream << "Time spent executing kernel code             \t: " << strDispTime(static_cast<double>(ul.QuadPart) * 1e-7, false, true) << endl;
      }
   }
   else
      OutStream << "Process timings                              \t: " << NA << endl;

   // Finally get more process statistics: this needs psapi.dll, so only proceed if it is present on this system
   HINSTANCE hDLL = LoadLibrary("psapi.dll");
   if (hDLL != NULL)
   {
      // The dll has been found
      typedef BOOL (__stdcall* DLLPROC) (HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
      DLLPROC ProcAdd;

      // Try to get the address of the function we will call
      ProcAdd = (DLLPROC) GetProcAddress(hDLL, "GetProcessMemoryInfo");
      if (ProcAdd)
      {
         // Address was found
         PROCESS_MEMORY_COUNTERS pmc;

         // Now call the function
         if ((ProcAdd) (GetCurrentProcess(), &pmc, sizeof(pmc)))
         {
            OutStream << "Peak working set size                        \t: " << pmc.PeakWorkingSetSize / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current working set size                     \t: " << pmc.WorkingSetSize / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak paged pool usage                        \t: " << pmc.QuotaPeakPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current paged pool usage                     \t: " << pmc.QuotaPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak non-paged pool usage                    \t: " << pmc.QuotaPeakNonPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current non-paged pool usage                 \t: " << pmc.QuotaNonPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak pagefile usage                          \t: " << pmc.PeakPagefileUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current pagefile usage                       \t: " << pmc.PagefileUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "No. of page faults                           \t: " << pmc.PageFaultCount << endl;
         }
      }

      // Free the memory used by the dll
      FreeLibrary(hDLL);
   }

#elif defined __GNUG__
   rusage ru;
   if (getrusage(RUSAGE_SELF, &ru) >= 0)
   {
      OutStream << "Time spent executing user code               \t: "  << strDispTime(ru.ru_utime.tv_sec, false, true) << endl;
//      OutStream << "ru_utime.tv_usec                             \t: " << ru.ru_utime.tv_usec << endl;
      OutStream << "Time spent executing kernel code             \t: " << strDispTime(ru.ru_stime.tv_sec, false, true) << endl;
//      OutStream << "ru_stime.tv_usec                             \t: " << ru.ru_stime.tv_usec << endl;
//      OutStream << "Maximum resident set size                    \t: " << ru.ru_maxrss/1024.0 << " Mb" << endl;
//      OutStream << "ixrss (???)                                  \t: " << ru.ru_ixrss << endl;
//      OutStream << "Sum of rm_asrss (???)                        \t: " << ru.ru_idrss << endl;
//      OutStream << "isrss (???)                                  \t: " << ru.ru_isrss << endl;
      OutStream << "No. of page faults not requiring physical I/O\t: " << ru.ru_minflt << endl;
      OutStream << "No. of page faults requiring physical I/O    \t: " << ru.ru_majflt << endl;
//      OutStream << "No. of times swapped out of main memory      \t: " << ru.ru_nswap << endl;
//      OutStream << "No. of times performed input (read request)  \t: " << ru.ru_inblock << endl;
//      OutStream << "No. of times performed output (write request)\t: " << ru.ru_oublock << endl;
//      OutStream << "No. of signals received                      \t: " << ru.ru_nsignals << endl;
      OutStream << "No. of voluntary context switches            \t: " << ru.ru_nvcsw << endl;
      OutStream << "No. of involuntary context switches          \t: " << ru.ru_nivcsw << endl;
   }
   else
      OutStream << NA << endl;
#else
   OutStream << NA << endl;
#endif

#ifdef _OPENMP
#pragma omp parallel
{
   if (0 == omp_get_thread_num())
   {
      OutStream << "Number of OpenMP threads                     \t: " << omp_get_num_threads() << endl;
      OutStream << "Number of OpenMP processors                  \t: " << omp_get_num_procs() << endl;
   }
}
#endif
}

/*==============================================================================================================================

 Returns an error message given an error code

==============================================================================================================================*/
string CSimulation::strGetErrorText(int const nErr)
{
   string strErr;

   switch (nErr)
   {
   case RTN_USERABORT:
      strErr = "aborted by user";
      break;
   case RTN_ERR_BADPARAM:
      strErr = "error in command-line parameter";
      break;
   case RTN_ERR_INI:
      strErr = "error reading initialization file";
      break;
   case RTN_ERR_CMEDIR:
      strErr = "error in directory name";
      break;
   case RTN_ERR_RUNDATA:
      strErr = "error reading run details file";
      break;
   case RTN_ERR_SHAPEFUNCTIONFILE:
      strErr = "error reading shape function file";
      break;
   case RTN_ERR_TIDEDATAFILE:
      strErr = "error reading tide data file";
      break;
   case RTN_ERR_LOGFILE:
      strErr = "error creating log file";
      break;
   case RTN_ERR_OUTFILE:
      strErr = "error creating text output file";
      break;
   case RTN_ERR_TSFILE:
      strErr = "error creating time series file";
      break;
   case RTN_ERR_DEMFILE:
      strErr = "error reading initial DEM file";
      break;
   case RTN_ERR_RASTER_FILE_READ:
      strErr = "error reading raster GIS file";
      break;
   case RTN_ERR_VECTOR_FILE_READ:
      strErr = "error reading vector GIS file";
      break;
   case RTN_ERR_MEMALLOC:
      strErr = "error allocating memory";
      break;
   case RTN_ERR_RASTER_GIS_OUT_FORMAT:
      strErr = "problem with raster GIS output format";
      break;
   case RTN_ERR_VECTOR_GIS_OUT_FORMAT:
      strErr = "problem with vector GIS output format";
      break;
   case RTN_ERR_TEXTFILEWRITE:
      strErr = "error writing text output file";
      break;
   case RTN_ERR_RASTER_FILE_WRITE:
      strErr = "error writing raster GIS output file";
      break;
   case RTN_ERR_VECTOR_FILE_WRITE:
      strErr = "error writing vector GIS output file";
      break;
   case RTN_ERR_TSFILEWRITE:
      strErr = "error writing time series output file";
      break;
   case RTN_ERR_LINETOGRID:
      strErr = "error putting linear feature onto raster grid";
      break;
   case RTN_ERR_NOSEACELLS:
      strErr = "no sea cells found";
      break;
   case RTN_ERR_GRIDTOLINE:
      strErr = "error when searching grid for linear feature";
      break;
   case RTN_ERR_FINDCOAST:
      strErr = "error finding coastline on grid";
      break;
   case RTN_ERR_MASSBALANCE:
      strErr = "error in this-iteration mass balance";
      break;
   case RTN_ERR_PROFILEWRITE:
      strErr = "error writing coastline-normal profiles";
      break;
   case RTN_ERR_TIMEUNITS:
      strErr = "error in time units";
      break;
   case RTN_ERR_BADENDPOINT:
      strErr = "finding end point for coastline-normal line";
      break;
   case RTN_ERR_OFFGRIDENDPOINT:
      strErr = "end point for coastline-normal line is off the grid";
      break;
   case RTN_ERR_CLIFFNOTCH:
      strErr = "cliff notch is above sediment top elevation";
      break;
   case RTN_ERR_CLIFFDEPOSIT:
      strErr = "unable to deposit sediment from cliff collapse";
      break;
   default:
      // should never get here
      strErr = "unknown error";
   }

   return strErr;
}


/*==============================================================================================================================

 Notifies the user that the simulation has ended, asks for keypress if necessary, and if compiled under GNU can send an email

==============================================================================================================================*/
void CSimulation::DoSimulationEnd(int const nRtn)
{
   switch (nRtn)
   {
   case (RTN_OK):
      // normal ending
      cout << RUNENDNOTICE << ctime(&m_tSysEndTime);
      break;

   case (RTN_HELPONLY):
   case (RTN_CHECKONLY):
      return;

   default:
      // Aborting because of some error
      time(&m_tSysEndTime);
      cerr << ERRORNOTICE << nRtn << " (" << strGetErrorText(nRtn) << ") on " << ctime(&m_tSysEndTime);

      if (LogStream && LogStream.is_open())
      {
         LogStream << ERR << "run aborted (error code " << nRtn << "): " << strGetErrorText(nRtn) << " on " << ctime(&m_tSysEndTime);
         LogStream.flush();
      }

      if (OutStream && OutStream.is_open())
      {
         OutStream << ERR << "run aborted (error code " << nRtn << "): " << strGetErrorText(nRtn) << " on " << ctime(&m_tSysEndTime);
         OutStream.flush();
      }
   }

#ifdef __GNUG__
   if (isatty(1))
   {
      // Stdout is connected to a tty, so not running as a background job
      cout << endl << PRESSKEY;
      cout.flush();
      getchar();
   }
   else
   {
      // Stdout is not connected to a tty, so must be running in the background; if we have something entered for the email address, then send an email
      if (! m_strMailAddress.empty())
      {
         cout << SENDEMAIL << m_strMailAddress << endl;

         string strCmd("echo \"");
         time_t tNow;
         time(&tNow);

         // Send an email using Linux/Unix mail command
         if (RTN_OK == nRtn)
         {
            // Finished normally
            strCmd.append("Simulation ");
            strCmd.append(m_strRunName);
            strCmd.append(", running on ");
            strCmd.append(strGetComputerName());
            strCmd.append(", completed normally on ");
            strCmd.append(ctime(&tNow));
            strCmd.append("\" | mail -s \"");
            strCmd.append(PROGNAME);
            strCmd.append(": normal completion\" ");
            strCmd.append(m_strMailAddress);
         }
         else
         {
            // Error, so give some information to help debugging
            strCmd.append("Simulation ");
            strCmd.append(m_strRunName);
            strCmd.append(", running on ");
            strCmd.append(strGetComputerName());
            strCmd.append(", aborted with error code ");
            char szTmp[15] = "";
            pszLongToSz(nRtn, szTmp, 3);
            strCmd.append(szTmp);
            strCmd.append(": ");
            strCmd.append(strGetErrorText(nRtn));
            strCmd.append(" at timestep ");
            szTmp[0] = '\0';
            strcpy(szTmp, pszLongToSz(m_ulIter, szTmp, 14));
            strCmd.append(szTmp);
            strCmd.append(" (");
            strCmd.append(strDispSimTime(m_dSimElapsed));
            strCmd.append(").\n\n This message sent ");
            strCmd.append(ctime(&tNow));
            strCmd.append("\" | mail -s \"");
            strCmd.append(PROGNAME);
            strCmd.append(": ERROR\" ");
            strCmd.append(m_strMailAddress);
         }
         int nRet = system(strCmd.c_str());
         if (WEXITSTATUS(nRet) != 0)
            cerr << ERR << EMAILERROR << endl;
      }
   }
#endif
}
