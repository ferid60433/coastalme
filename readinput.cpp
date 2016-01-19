/*!
 *
 * \file readinput.cpp
 * \brief Reads non-GIS input files
 * \details TODO A more detailed description of these routines.
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

 The bReadIni member function reads the initialization file

==============================================================================================================================*/
bool CSimulation::bReadIni(void)
{
   m_strCMEIni = m_strCMEDir;
   m_strCMEIni.append(CME_INI);

   // The .ini file is assumed to be in the CoastalME executable's directory
   string strFilePathName(m_strCMEIni);

   // Tell the user what is happening
   cout << READFILELOC << strFilePathName << endl;

   // Create an ifstream object
   ifstream InStream;

   // Try to open .ini file for input
   InStream.open(strFilePathName.c_str(), ios::in);

   // Did it open OK?
   if (! InStream.is_open())
   {
      // Error: cannot open .ini file for input
      cerr << ERR << "cannot open " << strFilePathName << " for input" << endl;
      return false;
   }

   char szRec[BUFSIZE] = "";
   int i = 0;
   string strRec, strErr;

   while (InStream.getline(szRec, BUFSIZE, '\n'))
   {
      strRec = szRec;

      // Trim off leading and trailing whitespace
      strRec = strTrimLeft(&strRec);
      strRec = strTrimRight(&strRec);

      // If it is a blank line or a comment then ignore it
      if ((! strRec.empty()) && (strRec[0] != QUOTE1) && (strRec[0] != QUOTE2))
      {
         // It isn't so increment counter
         i++;

         // Find the colon: note that lines MUST have a colon separating data from leading description portion
         size_t nPos = strRec.find(':');
         if (nPos == string::npos)
         {
            // Error: badly formatted line (no colon)
            cerr << ERR << "badly formatted line (no ':') in " << strFilePathName << endl << szRec << endl;
            return false;
         }

         if (nPos == strRec.size()-1)
         {
            // Error: badly formatted line (colon with nothing following)
            cerr << ERR << "badly formatted line (nothing following ':') in " << strFilePathName << endl << szRec << endl;
            return false;
         }

         // Strip off leading portion (the bit up to and including the colon)
         string strRH = strRec.substr(nPos+1);

         // Remove leading whitespace
         strRH = strTrimLeft(&strRH);

         // Look for a trailing comment, if found then terminate string at that point and trim off any trailing whitespace
         nPos = strRH.rfind(QUOTE1);
         if (nPos != string::npos)
            strRH = strRH.substr(0, nPos+1);

         nPos = strRH.rfind(QUOTE2);
         if (nPos != string::npos)
            strRH = strRH.substr(0, nPos+1);

         // Remove trailing whitespace
         strRH = strTrimRight(&strRH);

         switch (i)
         {
         case 1:
            // The main input run-data filename
            if (strRH.empty())
               strErr = "path and name of main datafile";
            else
            {
               // First check that we don't already have an input run-data filename, e.g. one entered on the command-line
               if (m_strDataPathName.empty())
               {
                  // We don't: so first check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                  if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                     // It has an absolute path, so use it 'as is'
                     m_strDataPathName = strRH;
                  else
                  {
                     // It has a relative path, so prepend the CoastalME dir
                     m_strDataPathName = m_strCMEDir;
                     m_strDataPathName.append(strRH);
                  }
               }
            }
            break;

         case 2:
            // Path for CoastalME output
            if (strRH.empty())
               strErr = "path for CoastalME output";
            else
            {
               // Check for trailing slash on CoastalME output directory name (is vital)
               if (strRH[strRH.size()-1] != PATH_SEPARATOR)
                  strRH.append(&PATH_SEPARATOR);

               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                  // It is an absolute path, so use it 'as is'
                  m_strOutPath = strRH;
               else
               {
                  // It is a relative path, so prepend the CoastalME dir
                  m_strOutPath = m_strCMEDir;
                  m_strOutPath.append(strRH);
               }
            }
            break;

         case 3:
            // Email address, only useful if running under Linux/Unix
            if (! strRH.empty())
            {
               // Something was entered, do rudimentary check for valid email address
               if (strRH.find('@') == string::npos)
                  strErr = "email address for messages";
               else
                  m_strMailAddress = strRH;
            }
            break;
         }

         // Did an error occur?
         if (! strErr.empty())
         {
            // Error in input to initialisation file
            cerr << ERR << "reading " << strErr << " in " << strFilePathName << endl << "'" << strRec << "'" << endl;
            InStream.close();

            return false;
         }
      }
   }

   InStream.close();
   return true;
}


/*==============================================================================================================================

 Reads the run details input file and does some initialization

==============================================================================================================================*/
bool CSimulation::bReadRunData(void)
{
   // Create an ifstream object
   ifstream InStream;

   // Try to open run details file for input
   InStream.open(m_strDataPathName.c_str(), ios::in);

   // Did it open OK?
   if (! InStream.is_open())
   {
      // Error: cannot open run details file for input
      cerr << ERR << "cannot open " << m_strDataPathName << " for input" << endl;
      return false;
   }

   char szRec[BUFSIZE] = "";
   int
      i = 0,
      nMult = 0;
      size_t nPos = 0;
   string strRec, strErr;

   while (InStream.getline(szRec, BUFSIZE, '\n'))
   {
      strRec = szRec;

      // Trim off leading and trailing whitespace
      strRec = strTrimLeft(&strRec);
      strRec = strTrimRight(&strRec);

      // If it is a blank line or a comment then ignore it
      if ((! strRec.empty()) && (strRec[0] != QUOTE1) && (strRec[0] != QUOTE2))
      {
         // It isn't so increment counter
         i++;

         // Find the colon: note that lines MUST have a colon separating data from leading description portion
         nPos = strRec.find(':');
         if (nPos == string::npos)
         {
            // Error: badly formatted line (no colon)
            cerr << ERR << "badly formatted line (no ':') in " << m_strDataPathName << endl << szRec << endl;
            return false;
         }

         // Strip off leading portion (the bit up to and including the colon)
         string strRH = strRec.substr(nPos+1);

         // Remove leading whitespace after the colon
         strRH = strTrimLeft(&strRH);

         // Look for a trailing comment, if found then terminate string at that point and trim off any trailing whitespace
         nPos = strRH.rfind(QUOTE1);
         if (nPos != string::npos)
            strRH = strRH.substr(0, nPos);

         nPos = strRH.rfind(QUOTE2);
         if (nPos != string::npos)
            strRH = strRH.substr(0, nPos);

         // Trim trailing spaces
         strRH = strTrimRight(&strRH);

#ifdef _WIN32
            // For Windows, make sure has backslashes, not Unix-style slashes
            strRH = pstrChangeToBackslash(&strRH);
#endif
         bool bFirst = true;
         int nRet = 0;
         double dMult = 0;
         string strTmp;

         switch (i)
         {
         // ---------------------------------------------- Run Information -----------------------------------------------------
         case 1:
            // Text output file names, don't change case
            if (strRH.empty())
               strErr = "output file names";
            else
            {
               m_strRunName = strRH;

               m_strOutFile = m_strOutPath;
               m_strOutFile.append(strRH);
               m_strOutFile.append(OUTEXT);

               m_strLogFile = m_strOutPath;
               m_strLogFile.append(strRH);
               m_strLogFile.append(LOGEXT);
            }
            break;

         case 2:
            // Duration of simulation (in hours, days, months, or years): sort out multiplier and user units, as used in the per-iteration output
            nRet = nDoSimulationTimeMultiplier(&strRH);
            if (nRet != RTN_OK)
            {
               strErr = "units for duration of simulation";
               break;
            }

            // And now calculate the duration of the simulation in hours: first find whitespace between the number and the unit
            nPos = strRH.rfind(SPACE);
            if (nPos == string::npos)
            {
               strErr = "format of duration simulation line";
               break;
            }

            // cut off rh bit of string
            strRH = strRH.substr(0, nPos+1);

            // remove trailing spaces
            strRH = strTrimRight(&strRH);

            // Calculate the duration of the simulation in hours
            m_dSimDuration = atof(strRH.c_str()) * m_dDurationUnitsMult;

            if (m_dSimDuration <= 0)
               strErr = "duration of simulation must be greater than zero";
            break;

         case 3:
            // Timestep of simulation (in hours or days)
            dMult = dGetTimeMultiplier(&strRH);
            if (static_cast<int>(dMult) == TIME_UNKNOWN)
            {
               strErr = "units for simulation timestep";
               break;
            }

            // we have the multiplier, now calculate the timestep in hours: look for the whitespace between the number and unit
            nPos = strRH.rfind(SPACE);
            if (nPos == string::npos)
            {
               strErr = "format of simulation timestep line";
               break;
            }

            // cut off rh bit of string
            strRH = strRH.substr(0, nPos+1);

            // remove trailing spaces
            strRH = strTrimRight(&strRH);

            m_dTimeStep = atof(strRH.c_str()) * dMult;         // in hours

            if (m_dTimeStep <= 0)
               strErr = "timestep of simulation must be greater than zero";

            if (m_dTimeStep >= m_dSimDuration)
               strErr = "timestep of simulation must be less than the duration of the simulation";

            break;

         case 4:
            // Save interval(s): first get the multiplier
            dMult = dGetTimeMultiplier(&strRH);
            if (static_cast<int>(dMult) == TIME_UNKNOWN)
            {
               strErr = "units for save intervals";
               break;
            }

            // search for a space from the right-hand end of the string
            nPos = strRH.rfind(SPACE);
            if (nPos == string::npos)
            {
               strErr = "format of save times/intervals line";
               break;
            }

            // cut off rh bit of string
            strRH = strRH.substr(0, nPos+1);

            // remove trailing spaces
            strRH = strTrimRight(&strRH);

            // Now find out whether we're dealing with a single regular save interval or a vector of save times: again check for a space
            nPos = strRH.find(SPACE);
            if (nPos != string::npos)
            {
               // There's another space, so we must have more than one number
               m_bSaveRegular = false;
               do
               {
                  // Put the number into the array
                  if (m_nUSave > static_cast<int>(SAVEMAX)-1)
                  {
                     strErr = "too many save intervals";
                     break;
                  }
                  m_dUSaveTime[m_nUSave++] = atof(strRH.substr(nPos).c_str()) * dMult;        // convert to hours

                  // Trim off the number and remove leading whitespace
                  strRH = strRH.substr(nPos, strRH.size()-nPos);
                  strRH = strTrimLeft(&strRH);

                  // Now look for another space
                  nPos = strRH.find(SPACE);
               }
               while (nPos != string::npos);

               // Check that we have at least 2 numbers
               if (m_nUSave < 1)
               {
                  strErr = "must have at least two save times";
                  break;
               }

               if (m_dUSaveTime[0] < m_dTimeStep)
               {
                  strErr = "first save time cannot be less than timestep";
                  break;
               }

               // Put a dummy save interval as the last entry in the array: this is needed to stop problems at end of run
               m_dUSaveTime[m_nUSave] = m_dSimDuration + 1;
            }
            else
            {
               // There isn't a space, so we have only one number
               m_bSaveRegular = true;
               m_dRSaveInterval = atof(strRH.c_str()) * nMult;                   // convert to hours
               if (m_dRSaveTime <= m_dTimeStep)
                  strErr = "save interval cannot be less than timestep";
               else
                  // Set up for first save
                  m_dRSaveTime = m_dRSaveInterval;
            }
            break;

         case 5:
            // Random number seed(s)
            m_ulRandSeed[0] = atol(strRH.c_str());
            if (0 == m_ulRandSeed[0])
            {
               strErr = "random number seed";
               break;
            }

            // TODO rewrite this, similar to reading raster slice elevations
            // OK, so find out whether we're dealing with a single seed or more than one: check for a space
            nPos = strRH.find(SPACE);
            if (nPos != string::npos)
            {
               // There's a space, so we must have more than one number
               int n = 0;
               do
               {
                  // Trim off the part before the first space then remove leading whitespace
                  strRH = strRH.substr(nPos, strRH.size()-nPos);
                  strRH = strTrimLeft(&strRH);

                  // Put the number into the array
                  m_ulRandSeed[++n] = atol(strRH.c_str());

                  // Now look for another space
                  nPos = strRH.find(SPACE);
               }
               while ((n < NRNG) && (nPos != string::npos));
            }
            else
            {
               // Only one seed specified, so make all seeds the same
               for (int n = 1; n < NRNG; n++)
                  m_ulRandSeed[n] = m_ulRandSeed[n-1];
            }
            break;

         case 6:
            // Raster GIS files to output, these are always saved
            m_bTopSurfSave                =
            m_bWaterDepthSave             =
            m_bWaveHeightSave             =
            m_bWaveAngleSave              =
            m_bPotentialErosionSave       =
            m_bActualErosionSave          =
            m_bTotalPotentialErosionSave  =
            m_bTotalActualErosionSave     =
            m_bLandformSave               =
            m_bInterventionSave           = true;

            // Now look for "all"
            if (strRH.find(ALL_RASTER_CODE) != string::npos)
            {
               m_bBasementElevSave        =
               m_bSuspSedSave             =
               m_bFineUnconsSedSave       =
               m_bSandUnconsSedSave       =
               m_bCoarseUnconsSedSave     =
               m_bFineConsSedSave         =
               m_bSandConsSedSave         =
               m_bCoarseConsSedSave       =
               m_bRasterCoastlineSave     =
               m_bRasterNormalSave        =
               m_bDistWeightSave          =
               m_bActiveZoneSave          =
               m_bCollapseSave            =
               m_bTotCollapseSave         =
               m_bCollapseDepositSave     =
               m_bTotCollapseDepositSave  = true;
            }
            else
            {
               // These are only saved if the user specified the code
               if (strRH.find(BASEMENT_ELEV_RASTER_CODE) != string::npos)
               {
                  m_bBasementElevSave = true;
                  strRH = strRemoveSubstr(&strRH, &BASEMENT_ELEV_RASTER_CODE);
               }

               if (strRH.find(SUSP_SED_RASTER_CODE) != string::npos)
               {
                  m_bSuspSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &SUSP_SED_RASTER_CODE);
               }

               if (strRH.find(FINE_UNCONS_RASTER_CODE) != string::npos)
               {
                  m_bFineUnconsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &FINE_UNCONS_RASTER_CODE);
               }

               if (strRH.find(SAND_UNCONS_RASTER_CODE) != string::npos)
               {
                  m_bSandUnconsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &SAND_UNCONS_RASTER_CODE);
               }

               if (strRH.find(COARSE_UNCONS_RASTER_CODE) != string::npos)
               {
                  m_bCoarseUnconsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &COARSE_UNCONS_RASTER_CODE);
               }

               if (strRH.find(FINE_CONS_RASTER_CODE) != string::npos)
               {
                  m_bFineConsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &FINE_CONS_RASTER_CODE);
               }

               if (strRH.find(SAND_CONS_RASTER_CODE) != string::npos)
               {
                  m_bSandConsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &SAND_CONS_RASTER_CODE);
               }

               if (strRH.find(COARSE_CONS_RASTER_CODE) != string::npos)
               {
                  m_bCoarseConsSedSave = true;
                  strRH = strRemoveSubstr(&strRH, &COARSE_CONS_RASTER_CODE);
               }

               if (strRH.find(RASTER_COAST_CODE) != string::npos)
               {
                  m_bRasterCoastlineSave = true;
                  strRH = strRemoveSubstr(&strRH, &RASTER_COAST_CODE);
               }

               if (strRH.find(RASTER_COAST_NORMAL_CODE) != string::npos)
               {
                  m_bRasterNormalSave = true;
                  strRH = strRemoveSubstr(&strRH, &RASTER_COAST_NORMAL_CODE);
               }

               if (strRH.find(DISTWEIGHT_RASTER_CODE) != string::npos)
               {
                  m_bDistWeightSave = true;
                  strRH = strRemoveSubstr(&strRH, &DISTWEIGHT_RASTER_CODE);
               }

               if (strRH.find(ACTIVEZONE_CODE) != string::npos)
               {
                  m_bActiveZoneSave = true;
                  strRH = strRemoveSubstr(&strRH, &ACTIVEZONE_CODE);
               }

               if (strRH.find(COLLAPSE_RASTER_CODE) != string::npos)
               {
                  m_bCollapseSave = true;
                  strRH = strRemoveSubstr(&strRH, &COLLAPSE_RASTER_CODE);
               }

               if (strRH.find(TOTAL_COLLAPSE_RASTER_CODE) != string::npos)
               {
                  m_bTotCollapseSave = true;
                  strRH = strRemoveSubstr(&strRH, &TOTAL_COLLAPSE_RASTER_CODE);
               }

               if (strRH.find(COLLAPSE_DEPOSIT_RASTER_CODE) != string::npos)
               {
                  m_bCollapseDepositSave = true;
                  strRH = strRemoveSubstr(&strRH, &COLLAPSE_DEPOSIT_RASTER_CODE);
               }

               if (strRH.find(TOTAL_COLLAPSE_DEPOSIT_RASTER_CODE) != string::npos)
               {
                  m_bTotCollapseDepositSave = true;
                  strRH = strRemoveSubstr(&strRH, &TOTAL_COLLAPSE_DEPOSIT_RASTER_CODE);
               }

               // Check to see if all codes have been removed
               strRH = strTrimLeft(&strRH);
               if (! strRH.empty())
                  strErr = "raster GIS output file list";
            }
            break;

         case 7:
            // Raster GIS output format: blank means use same format as input DEM file (if possible)
            if (m_strRasterGISOutFormat.empty())
               m_strRasterGISOutFormat = strRH;

            if (m_strRasterGISOutFormat.empty())
               strErr = "raster GIS output format";
            break;

         case 8:
            // Elevations for raster slice output, if desired
            if (! strRH.empty())
            {
               m_bSliceSave = true;

               // OK, so find out whether we're dealing with a single seed or more than one: check for a space
               nPos = strRH.find(SPACE);
               if (nPos != string::npos)
               {
                  // There's a space, so we must have more than one number
                  do
                  {
                     // Get LH bit
                     strTmp = strRH.substr(0, nPos);
                     m_VdSliceElev.push_back(atof(strTmp.c_str()));

                     // Get the RH bit
                     strRH = strRH.substr(nPos, strRH.size()-nPos);
                     strRH = strTrimLeft(&strRH);

                     // Now look for another space
                     nPos = strRH.find(SPACE);
                  }
                  while (nPos != string::npos);
               }
               // Read either the single number, or the left-over number
               m_VdSliceElev.push_back(atof(strTmp.c_str()));
            }
            break;

         case 9:
            // Vector GIS files to output, first look for "all"
            if (strRH.find(ALL_VECTOR_CODE) != string::npos)
            {
               m_bCoastSave            =
               m_bNormalsSave          =
               m_bCoastCurvatureSave   = true;
            }
            else
            {
               // These are always saved
               m_bCoastSave            = true;

               // These are only saved if the user specified the code
               if (strRH.find(VECTOR_NORMALS_CODE) != string::npos)
               {
                  m_bNormalsSave = true;
                  strRH = strRemoveSubstr(&strRH, &VECTOR_NORMALS_CODE);
               }

               if (strRH.find(VECTOR_COAST_CURVATURE_CODE) != string::npos)
               {
                  m_bCoastCurvatureSave = true;
                  strRH = strRemoveSubstr(&strRH, &VECTOR_COAST_CURVATURE_CODE);
               }

               // Check to see if all codes have been removed
               strRH = strTrimLeft(&strRH);
               if (! strRH.empty())
                  strErr = "vector GIS output file list";
            }
            break;

         case 10:
            // Vector GIS output format
            m_strVectorGISOutFormat = strRH;

            if (strRH.empty())
               strErr = "vector GIS output format";
            break;

         case 11:
            // Time series files to output, first check for "all"
            if (strRH.find(ALL_RASTER_CODE) != string::npos)
            {
               m_bSeaAreaTS            =
               m_bStillWaterLevelTS    =
               m_bErosionTS            =
               m_bDepositionTS         =
               m_bSedLostFromGridTS    =
               m_bSuspSedTS            = true;
            }
            else
            {
               if (strRH.find(SEAAREATSCODE) != string::npos)
               {
                  m_bSeaAreaTS = true;
                  strRH = strRemoveSubstr(&strRH, &SEAAREATSCODE);
               }

               if (strRH.find(STILLWATERLEVELCODE) != string::npos)
               {
                  m_bStillWaterLevelTS = true;
                  strRH = strRemoveSubstr(&strRH, &STILLWATERLEVELCODE);
               }

               if (strRH.find(EROSIONTSCODE) != string::npos)
               {
                  m_bErosionTS = true;
                  strRH = strRemoveSubstr(&strRH, &EROSIONTSCODE);
               }

               if (strRH.find(DEPOSITIONTSCODE) != string::npos)
               {
                  m_bDepositionTS = true;
                  strRH = strRemoveSubstr(&strRH, &DEPOSITIONTSCODE);
               }

               if (strRH.find(SEDLOSTFROMGRIDTSCODE) != string::npos)
               {
                  m_bSedLostFromGridTS = true;
                  strRH = strRemoveSubstr(&strRH, &SEDLOSTFROMGRIDTSCODE);
               }

               if (strRH.find(SUSPSEDTSCODE) != string::npos)
               {
                  m_bSuspSedTS = true;
                  strRH = strRemoveSubstr(&strRH, &SUSPSEDTSCODE);
               }

               // Check to see if all codes have been removed
               strRH = strTrimLeft(&strRH);
               if (! strRH.empty())
                  strErr = "time-series output file list";
            }
            break;

         case 12:
            // Vector coastline smoothing algorithm: 0 = none, 1 = running mean, 2 = Savitsky-Golay
            m_nCoastSmooth = atoi(strRH.c_str());
            if ((m_nCoastSmooth < SMOOTH_NONE) || (m_nCoastSmooth > SMOOTH_SAVITZKY_GOLAY))
               strErr = "coastline vector smoothing algorithm";
            break;

         case 13:
            // Size of coastline smoothing window: must be odd
            m_nCoastSmoothWindow = atoi(strRH.c_str());
            if ((m_nCoastSmoothWindow <= 0) || !(m_nCoastSmoothWindow % 2))
               strErr = "size of coastline vector smoothing window (must be > 0 and odd)";
            break;

         case 14:
            // Order of coastline profile smoothing polynomial for Savitsky-Golay: usually 2 or 4, max is 6
            m_nSavGolCoastPoly = atoi(strRH.c_str());
            if ((m_nSavGolCoastPoly <= 0) || (m_nSavGolCoastPoly > 6))
               strErr = "value of Savitsky-Golay polynomial for coastline smoothing (must be <= 6)";
            break;

         case 15:
            // Random edge for coastline search?
            m_bRandomCoastEdgeSearch = false;
            if (strRH.find("y") != string::npos)
            m_bRandomCoastEdgeSearch = true;
               break;

         case 16:
            // Profile slope running-mean smoothing window size: must be odd
            m_nProfileSmoothWindow = atoi(strRH.c_str());
            if ((m_nProfileSmoothWindow <= 0) || !(m_nProfileSmoothWindow % 2))
               strErr = "size of profile vector smoothing window (must be > 0 and odd)";
            break;

         case 17:
            // Max local slope (m/m)
            m_dProfileMaxSlope = atoi(strRH.c_str());
            if (m_dProfileMaxSlope <= 0)
               strErr = "max local slope must be greater than zero";
            break;

         // ------------------------------------------------- Raster GIS layers ------------------------------------------------
         case 18:
            // Initial number of layers
            m_nLayers = atoi(strRH.c_str());
            if (m_nLayers < 1)
            {
               strErr = "must be at least one initial layer";
               break;
            }

            // OK we know the number of layers, so add elements to these vectors
            for (int i = 0; i < m_nLayers; i++)
            {
               m_VstrInitialFineUnconsSedimentFile.push_back("");
               m_VstrInitialSandUnconsSedimentFile.push_back("");
               m_VstrInitialCoarseUnconsSedimentFile.push_back("");
               m_VstrInitialFineConsSedimentFile.push_back("");
               m_VstrInitialSandConsSedimentFile.push_back("");
               m_VstrInitialCoarseConsSedimentFile.push_back("");
               m_VstrGDALIUFDriverCode.push_back("");
               m_VstrGDALIUFDriverDesc.push_back("");
               m_VstrGDALIUFProjection.push_back("");
               m_VstrGDALIUFDataType.push_back("");
               m_VstrGDALIUSDriverCode.push_back("");
               m_VstrGDALIUSDriverDesc.push_back("");
               m_VstrGDALIUSProjection.push_back("");
               m_VstrGDALIUSDataType.push_back("");
               m_VstrGDALIUCDriverCode.push_back("");
               m_VstrGDALIUCDriverDesc.push_back("");
               m_VstrGDALIUCProjection.push_back("");
               m_VstrGDALIUCDataType.push_back("");
               m_VstrGDALICFDriverCode.push_back("");
               m_VstrGDALICFDriverDesc.push_back("");
               m_VstrGDALICFProjection.push_back("");
               m_VstrGDALICFDataType.push_back("");
               m_VstrGDALICSDriverCode.push_back("");
               m_VstrGDALICSDriverDesc.push_back("");
               m_VstrGDALICSProjection.push_back("");
               m_VstrGDALICSDataType.push_back("");
               m_VstrGDALICCDriverCode.push_back("");
               m_VstrGDALICCDriverDesc.push_back("");
               m_VstrGDALICCProjection.push_back("");
               m_VstrGDALICCDataType.push_back("");
            }
            break;

         case 19:
            // Basement DEM file (can be blank)
            if (! strRH.empty())
            {
#ifdef _WIN32
               // For Windows, make sure has backslashes, not Unix-style slashes
               strRH = pstrChangeToBackslash(&strRH);
#endif
               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                  // It has an absolute path, so use it 'as is'
                  m_strInitialBasementDEMFile = strRH;
               else
               {
                  // It has a relative path, so prepend the CoastalME dir
                  m_strInitialBasementDEMFile = m_strCMEDir;
                  m_strInitialBasementDEMFile.append(strRH);
               }
            }
            break;

         case 20:
            // Read 6 x sediment files for each layer
            for (int nLayer = 0; nLayer < m_nLayers; nLayer++)
            {
//               char szNumTmp[4] = "";
//               string strLayer = pszTrimLeft(pszLongToSz(nLayer+1, szNumTmp, 4));
               for (int j = 1; j <= 6; j++)
               {
                  if (! bFirst)
                  {
                     do
                     {
                        if (! InStream.getline(szRec, BUFSIZE, '\n'))
                        {
                           cerr << ERR << "premature end of file in " << m_strDataPathName << endl;
                           return false;
                        }

                        strRec = szRec;

                        // Trim off leading and trailing whitespace
                        strRec = strTrimLeft(&strRec);
                        strRec = strTrimRight(&strRec);
                     }
                     // If it is a blank line or a comment then ignore it
                     while (strRec.empty() || (strRec[0] == QUOTE1) || (strRec[0] == QUOTE2));

                     // Not blank or a comment, so find the colon: lines MUST have a colon separating data from leading description portion
                     nPos = strRec.find(':');
                     if (nPos == string::npos)
                     {
                        // Error: badly formatted line (no colon)
                        cerr << ERR << "badly formatted line (no ':') in " << m_strDataPathName << endl << szRec << endl;
                        return false;
                     }

                     // Strip off leading portion (the bit up to and including the colon)
                     strRH = strRec.substr(nPos+1);

                     // Remove leading whitespace after the colon
                     strRH = strTrimLeft(&strRH);

                     // Look for a trailing comment, if found then terminate string at that point and trim off any trailing whitespace
                     nPos = strRH.rfind(QUOTE1);
                     if (nPos != string::npos)
                        strRH = strRH.substr(0, nPos);

                     nPos = strRH.rfind(QUOTE2);
                     if (nPos != string::npos)
                        strRH = strRH.substr(0, nPos);

                     // Trim trailing spaces
                     strRH = strTrimRight(&strRH);

#ifdef _WIN32
                     // For Windows, make sure has backslashes, not Unix-style slashes
                     strRH = pstrChangeToBackslash(&strRH);
#endif
                  }

                  bFirst = false;

                  switch (j)
                  {
                  case 1:
                     // Initial fine unconsolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif

                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialFineUnconsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialFineUnconsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialFineUnconsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;

                  case 2:
                     // Initial sand unconsolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif
                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialSandUnconsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialSandUnconsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialSandUnconsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;

                  case 3:
                     // Initial coarse unconsolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif
                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialCoarseUnconsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialCoarseUnconsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialCoarseUnconsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;

                  case 4:
                     // Initial fine consolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif
                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialFineConsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialFineConsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialFineConsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;

                  case 5:
                     // Initial sand consolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif
                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialSandConsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialSandConsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialSandConsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;

                  case 6:
                     // Initial coarse consolidated sediment depth GIS file (can be blank)
                     if (! strRH.empty())
                     {
#ifdef _WIN32
                        // For Windows, make sure has backslashes, not Unix-style slashes
                        strRH = pstrChangeToBackslash(&strRH);
#endif
                        // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
                        if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                        {
                           // It has an absolute path, so use it 'as is'
                           m_VstrInitialCoarseConsSedimentFile[nLayer] = strRH;
                        }
                        else
                        {
                           // It has a relative path, so prepend the CoastalME dir
                           m_VstrInitialCoarseConsSedimentFile[nLayer] = m_strCMEDir;
                           m_VstrInitialCoarseConsSedimentFile[nLayer].append(strRH);
                        }
                     }
                     break;
                  }

                  // Did an error occur?
                  if (! strErr.empty())
                  {
                     // Error in input to run details file
                     cerr << ERR << "reading " << strErr << " in " << m_strDataPathName << endl << "'" << szRec << "'" << endl;
                     InStream.close();
                     return false;
                  }
               }
            }
            break;

         case 21:
            // Initial suspended sediment depth GIS file (can be blank)
            if (! strRH.empty())
            {
#ifdef _WIN32
               // For Windows, make sure has backslashes, not Unix-style slashes
               strRH = pstrChangeToBackslash(&strRH);
#endif
               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
               {
                  // It has an absolute path, so use it 'as is'
                  m_strInitialSuspSedimentFile = strRH;
               }
               else
               {
                  // It has a relative path, so prepend the CoastalME dir
                  m_strInitialSuspSedimentFile = m_strCMEDir;
                  m_strInitialSuspSedimentFile.append(strRH);
               }
            }
            break;

         case 22:
            // Initial Landform class GIS file (can be blank)
            if (! strRH.empty())
            {
#ifdef _WIN32
               // For Windows, make sure has backslashes, not Unix-style slashes
               strRH = pstrChangeToBackslash(&strRH);
#endif
               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
               {
                  // It has an absolute path, so use it 'as is'
                  m_strInitialLandformFile = strRH;
               }
               else
               {
                  // It has a relative path, so prepend the CoastalME dir
                  m_strInitialLandformFile = m_strCMEDir;
                  m_strInitialLandformFile.append(strRH);
               }
            }
            break;

         case 23:
            // Initial Intervention class GIS file (can be blank)
            if (! strRH.empty())
            {
#ifdef _WIN32
               // For Windows, make sure has backslashes, not Unix-style slashes
               strRH = pstrChangeToBackslash(&strRH);
#endif
               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
               {
                  // It has an absolute path, so use it 'as is'
                  m_strInitialInterventionFile = strRH;
               }
               else
               {
                  // It has a relative path, so prepend the CoastalME dir
                  m_strInitialInterventionFile = m_strCMEDir;
                  m_strInitialInterventionFile.append(strRH);
               }
            }
            break;

         // ---------------------------------------------------- Hydrology data ------------------------------------------------
         case 24:
            // Initial still water level (m)
            m_dOrigStillWaterLevel = atof(strRH.c_str());
            break;

         case 25:
            // Wave period (sec)
            m_dWavePeriod = atof(strRH.c_str());
            if (m_dWavePeriod <= 0)
               strErr = "wave period must be greater than zero";
            break;

         case 26:
            // Offshore wave height (m)
            m_dOffshoreWaveHeight = atof(strRH.c_str());
            if (m_dOffshoreWaveHeight <= 0)
               strErr = "offshore wave height must be greater than zero";
            break;

         case 27:
            // Offshore wave orientation in input CRS: this is the direction TOWARDS which the waves move (in degrees from azimuth)
            m_dOffshoreWaveOrientationIn = atof(strRH.c_str());
            if (m_dOffshoreWaveOrientationIn < 0)
               strErr = "offshore wave orientation must be zero degrees or more";
            else if (m_dOffshoreWaveOrientationIn >= 360)
               strErr = "offshore wave orientation must be less than 360 degrees";

            // Wave orientation used in wave propagation calcs is 180 degrees from this
            m_dOffshoreWaveOrientation = dKeepWithin360(m_dOffshoreWaveOrientationIn + 180);
            break;

         case 28:
            // Tide data file (can be blank)
            if (! strRH.empty())
            {
#ifdef _WIN32
               // For Windows, make sure has backslashes, not Unix-style slashes
               strRH = pstrChangeToBackslash(&strRH);
#endif
               // Now check for leading slash, or leading Unix home dir symbol, or occurrence of a drive letter
               if ((strRH[0] == PATH_SEPARATOR) || (strRH[0] == '~') || (strRH[1] == ':'))
                  // It has an absolute path, so use it 'as is'
                  m_strTideDataFile = strRH;
               else
               {
                  // It has a relative path, so prepend the CoastalME dir
                  m_strTideDataFile = m_strCMEDir;
                  m_strTideDataFile.append(strRH);
               }
            }
            break;

         // ----------------------------------------------------- Sediment data ------------------------------------------------
         case 29:
            // R values along profile, see Walkden & Hall, 2011
            m_dR = atof(strRH.c_str());
            if (m_dR <= 0)
               strErr = "R values must be greater than zero";
            break;

         case 30:
            // Do along-shore sediment transport?
            m_bDoAlongshoreTransport = false;
            if (strRH.find("y") != string::npos)
               m_bDoAlongshoreTransport = true;
            break;

         case 31:
            // Erodibility of fine-sized sediment
            m_dFineErodibility = atof(strRH.c_str());
            break;

         case 32:
            // Erodibility of sand-sized sediment
            m_dSandErodibility = atof(strRH.c_str());
            break;

         case 33:
            // Erodibility of coarse-sized sediment
            m_dCoarseErodibility = atof(strRH.c_str());
            break;

/*
     ; LAYER 1, LOWEST
   dErodFine                                                      : 0.8
   dErodSand                                                      : 0.7
   dErodCoarse                                                    : 0.9
   ; LAYER 2
   dErodFine                                                      : 1.0
   dErodSand                                                      : 0.9
   dErodCoarse                                                    : 0.6
   ; LAYER 3, HIGHEST
   dErodFine                                                      : 0.95
   dErodSand                                                      : 0.4
   dErodCoarse                                                    : 0.3
*/

         // ------------------------------------------------ Cliff collapse data -----------------------------------------------
         case 34:
            // Do cliff collapse?
            m_bDoCliffCollapse = false;
            if (strRH.find("y") != string::npos)
               m_bDoCliffCollapse = true;
            break;

         case 35:
            // Cliff erodibility
            m_dCliffErodibility = atof(strRH.c_str());
            if (m_dCliffErodibility <= 0)
               strErr = "cliff erodibility must be greater than 0";
            break;

         case 36:
            // Notch overhang at collapse (m)
            m_dNotchOverhangAtCollapse = atof(strRH.c_str());
            if (m_dNotchOverhangAtCollapse <= 0)
               strErr = "cliff notch overhang at collapse must be greater than 0";
            break;

         case 37:
            // Notch base below still water level (m)
            m_dNotchBaseBelowStillWaterLevel = atof(strRH.c_str());
            if (m_dNotchBaseBelowStillWaterLevel < 0)
               strErr = "cliff notch base below still water level must be greater than 0";
            break;

         case 38:
            // Scale parameter A for cliff deposition (m^(1/3)) [0 = auto]
            m_dCliffDepositionA = atof(strRH.c_str());
            if (m_dCliffDepositionA < 0)
               strErr = "scale parameter A for cliff deposition must be 0 [= auto] or greater";
            break;

         case 39:
            // Planview width of cliff deposition talus (in cells) [must be odd]
            m_nCliffDepositionPlanviewWidth = atoi(strRH.c_str());
            if ((m_nCliffDepositionPlanviewWidth % 2) == 0)
               strErr = "planview width of cliff deposition must be odd";
            if (m_nCliffDepositionPlanviewWidth <= 0)
               strErr = "planview width of cliff deposition must be greater than 0";
            break;

         case 40:
            // Planview length of cliff deposition talus (m)
            m_dCliffDepositionPlanviewLength = atof(strRH.c_str());
            if (m_dCliffDepositionPlanviewLength <= 0)
               strErr = "planview length of cliff deposition must be greater than 0";
            break;

         case 41:
            // Height of landward end of talus, as a fraction of cliff elevation
            m_dCliffDepositionHeightFrac = atof(strRH.c_str());
            if (m_dCliffDepositionHeightFrac < 0)
               strErr = "height of cliff collapse (as a fraction of cliff elevation) must be 0 or greater";
            break;

         // -------------------------------------------------- Intervention data -----------------------------------------------
         case 42:
            // Beach protection factor [0-1]
            m_dBeachProtectionFactor = atof(strRH.c_str());
            if ((m_dBeachProtectionFactor < 0) || (m_dBeachProtectionFactor > 1))
               strErr = "beach protection factor must be between 0 and 1";
            break;

         // ------------------------------------------------------ Other data --------------------------------------------------
         case 43:
            // Spacing of coastline normals (m)
            m_dCoastNormalAvgSpacing = atof(strRH.c_str());
            if (m_dCoastNormalAvgSpacing < 0)
               strErr = "spacing of coastline normals must be greater than zero";
            break;

         case 44:
            // Length of coastline normals (m)
            m_dCoastNormalLength = atof(strRH.c_str());
            if (m_dCoastNormalLength <= 0)
               strErr = "length of coastline normals must be greater than zero";
            break;

         case 45:
            // Random factor for spacing of coastline normals
            m_dCoastNormalRandSpaceFact = atof(strRH.c_str());
            if (m_dCoastNormalRandSpaceFact < 0)
               strErr = "random factor for spacing of coastline normals must be zero (deterministic) or greater than zero";
            break;

         case 46:
            // Interval for coast curvature calcs (integer number of points)
            m_nCoastCurvatureInterval = atoi(strRH.c_str());
            if (m_nCoastCurvatureInterval <= 0)
               strErr = "interval for coast curvature calculations must be greater than zero";
            break;

         // ----------------------------------------------------- Testing only -------------------------------------------------
         case 47:
            // Output profile data?
            m_bOutputProfileData = false;
            if (strRH.find("y") != string::npos)
               m_bOutputProfileData = true;

            // Check whether profile spacing has a random component, if it does then saving profiles is meaningless since the 'same' profile will not be saved each time
            if (m_bOutputProfileData && (m_dCoastNormalRandSpaceFact != 0))
            {
               stringstream s;
               s << m_dCoastNormalRandSpaceFact;
               // There is a random component, so warn the user and turn off the random component
               strErr = "You have specified a random factor of " + s.str() + " for the spacing of coastline normals.\nThis is incompatible with the option to output profile data";
            }

            break;

         case 48:
            // Numbers of profiles to be saved
            if (m_bOutputProfileData)
            {
               vector<string> strTmp = strSplit(&strRH, SPACE);
               for (unsigned int j = 0; j < strTmp.size(); j++)
               {
                  strTmp[j] = strTrim(&strTmp[j]);
                  int nTmp = atoi(strTmp[j].c_str());
                  if (nTmp < 0)
                  {
                     strErr = "Profile number for saving must be zero or greater";
                     break;
                  }

                  m_VnProfileToSave.push_back(nTmp);
               }
            }
            break;

         case 49:
           // Timesteps to save profile for output
            if (m_bOutputProfileData)
            {
               vector<string> strTmp = strSplit(&strRH, SPACE);
               for (unsigned int j = 0; j < strTmp.size(); j++)
               {
                  strTmp[j] = strTrim(&strTmp[j]);
                  unsigned long ulTmp = atol(strTmp[j].c_str());
                  if (ulTmp < 1)
                  {
                     strErr = "Timestep for profile saves must be one or greater";
                     break;
                  }

                  m_VulProfileTimestep.push_back(ulTmp);
               }
            }
            break;

         case 50:
            // Output parallel profile data?
            m_bOutputParallelProfileData = false;
            if (strRH.find("y") != string::npos)
               m_bOutputParallelProfileData = true;
            break;

         case 51:
            // Output erosion potential look-up data?
            m_bOutputLookUpData = false;
            if (strRH.find("y") != string::npos)
               m_bOutputLookUpData = true;
            break;

         case 52:
            // Erode coast in alternate direction each timestep?
            m_bErodeCoastAlternateDir = false;
            if (strRH.find("y") != string::npos)
            m_bErodeCoastAlternateDir = true;
               break;
         }

         // Did an error occur?
         if (! strErr.empty())
         {
            // Error in input to run details file
            cerr << endl << ERR << strErr << ".\nPlease edit " << m_strDataPathName << " and change this line:" << endl << "'" << szRec << "'" << endl << endl;
            InStream.close();
            return false;
         }
      }
   }

   // Close file
   InStream.close();

   // Finally, need to check that we have at least one raster file, so that we know the grid size and units (and preferably also the projection)
   bool bNoRasterFiles = true;
   if ((! m_strInitialBasementDEMFile.empty()) || (! m_strInitialSuspSedimentFile.empty()) || (! m_strInitialLandformFile.empty()) || (! m_strInitialInterventionFile.empty()))
      bNoRasterFiles = false;

   for (int i = 0; i < m_nLayers; i++)
   {
      if ((! m_VstrInitialFineUnconsSedimentFile[i].empty()) || (! m_VstrInitialSandUnconsSedimentFile[i].empty()) || (! m_VstrInitialCoarseUnconsSedimentFile[i].empty()) || (! m_VstrInitialFineConsSedimentFile[i].empty()) || (! m_VstrInitialSandConsSedimentFile[i].empty()) || (! m_VstrInitialCoarseConsSedimentFile[i].empty()))
         bNoRasterFiles = false;
   }

   if (bNoRasterFiles)
   {
      // No raster files
      cerr << ERR << "at least one raster GIS file is needed" << endl;
      return false;
   }

   return true;
}

/*==============================================================================================================================

 Reads the tide data

==============================================================================================================================*/
int CSimulation::nReadTideData()
{
   // Create an ifstream object
   ifstream InStream;

   // Try to open the file for input
   InStream.open(m_strTideDataFile.c_str(), ios::in);

   // Did it open OK?
   if (! InStream.is_open())
   {
      // Error: cannot open tide datan file for input
      cerr << ERR << "cannot open " << m_strTideDataFile << " for input" << endl;
      return RTN_ERR_TIDEDATAFILE;
   }

   // Opened OK
   char szRec[BUFSIZE] = "";
   string strRec;

   // Now read the data from the file
   while (InStream.getline(szRec, BUFSIZE, '\n'))
   {
      strRec = szRec;

      // Trim off leading and trailing whitespace
      strRec = strTrimLeft(&strRec);
      strRec = strTrimRight(&strRec);

      // If it is a blank line or a comment then ignore it
      if ((strRec.empty()) || (strRec[0] == QUOTE1) || (strRec[0] == QUOTE2))
         continue;

      // Convert to double then append the value to the vector TODO check for floating point validity
      m_VdTideData.push_back(strtod(strRec.c_str(), NULL));
   }

   // Close file
   InStream.close();

   return RTN_OK;
}

/*==============================================================================================================================

 Reads the shape of the erosion potential distribution (see shape function in Walkden & Hall, 2005)

==============================================================================================================================*/
int CSimulation::nReadShapeFunction()
{
   // Sort out the path and filename
   m_strShapeFunctionFile = m_strCMEDir;
   m_strShapeFunctionFile.append(SHAPEFUNCTIONFILE);

   // Create an ifstream object
   ifstream InStream;

   // Try to open the file for input
   InStream.open(m_strShapeFunctionFile.c_str(), ios::in);

   // Did it open OK?
   if (! InStream.is_open())
   {
      // Error: cannot open shape function file for input
      cerr << ERR << "cannot open " << m_strShapeFunctionFile << " for input" << endl;
      return RTN_ERR_SHAPEFUNCTIONFILE;
   }

   // Opened OK
   int nExpected = 0, nRead = 0;
   char szRec[BUFSIZE] = "";
   string strRec;

   // Read in the number of data lines expected
   InStream >> nExpected;

   // Set up the vectors to hold the input data
   vector<double>
      VdDepthOverDB,
      VdErosionPotential,
      VdErosionPotentialFirstDeriv;

   // Now read the rest of the data from the file to get the Erosion Potential Shape function
   while (InStream.getline(szRec, BUFSIZE, '\n'))
   {
      strRec = szRec;

      // Trim off leading and trailing whitespace
      strRec = strTrimLeft(&strRec);
      strRec = strTrimRight(&strRec);

      // If it is a blank line or a comment then ignore it
      if ((strRec.empty()) || (strRec[0] == QUOTE1) || (strRec[0] == QUOTE2))
         continue;

      // It isn't so increment counter
      nRead++;

      // Split the string, and remove whitespace
      vector<string> strTmp = strSplit(&strRec, SPACE);
      for (unsigned int i = 0; i < strTmp.size(); i++)
         strTmp[i] = strTrim(&strTmp[i]);

      // Convert to doubles then append the values to the vectors TODO check for floating point validity
      VdDepthOverDB.push_back(strtod(strTmp[0].c_str(), NULL));
      VdErosionPotential.push_back(strtod(strTmp[1].c_str(), NULL));
      VdErosionPotentialFirstDeriv.push_back(strtod(strTmp[2].c_str(), NULL));
   }

   // Close file
   InStream.close();

   // Did we read in what we expected?
   if (nExpected != nRead)
   {
      cout << ERR << "read in " << nRead << " lines from " << m_strShapeFunctionFile << " but " << nExpected << " lines expected" << endl;
      return RTN_ERR_SHAPEFUNCTIONFILE;
   }

   // OK, now use this data to create a look-up table to be used for the rest of the simulation
   if (! bCreateErosionPotentialLookUp(&VdDepthOverDB, &VdErosionPotential, &VdErosionPotentialFirstDeriv))
   {
      cout << ERR << " in " << m_strShapeFunctionFile << ", erosion potential function is unbounded for high values of depth over DB" << endl;
      return RTN_ERR_SHAPEFUNCTIONFILE;
   }

   return RTN_OK;
}
