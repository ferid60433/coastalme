/*!
 *
 * \mainpage
   <b>CoastalME</b> (Coastal Modelling Environment) simulates the long-term behaviour of a coast. This initial version is a prototype which considers simple soft cliff cross-shore effects only\n

 * \section intro_sec Introduction
 * <b>TODO</b> Say more abut CoastalME here\n
   \n
   From Shingle Street\n
   To Orford Ness\n
   The waves maraud,\n
   The winds oppress,\n
   The earth can’t help\n
   But acquiesce\n
   For this is east\n
   And east means loss,\n
   A lessening shore, receding ground,\n
   Three feet gone last year, four feet this\n
   Where land runs out and nothing’s sound.\n
   Nothing lasts long on Shingle Street.\n
   \n
   By Blake Morrison (2016). See http://www.randomhouse.co.uk/editions/shingle-street/9780701188771\n

 * \section install_sec Installation

 * \subsection step1 Step 1: Opening the box

 * \subsection step2 Step 2: Running CoastalME

 * \subsection step3 Step 3: Building datasets
 *
 * \file cme.h
 * \brief Contains global definitions for CoastalME
 *
 */

#ifndef CME_H
#define CME_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
using namespace std;

#ifdef _MSC_VER
   #pragma warning(push, 0)
#endif

#include <assert.h>
#include <ctype.h>                  // for tolower()
#include <stdlib.h>                 // for atof()
#include <string.h>                 // for strcpy(), strlen() etc.
#include <ctime>
#include <cmath>
#include <cfloat>
#include <climits>
#ifdef _WIN32
   #include <windows.h>             // needed for CalcProcessStats()
   #include <psapi.h>               // not available if compiling under Win?
   #include <direct.h>              // for chdir()
   #include <io.h>                  // for isatty()
#else
   #include <sys/resource.h>        // needed for CalcProcessStats()
#endif
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <gdal_priv.h>
#include <cpl_string.h>
#include <ogrsf_frmts.h>
//#include <ogr_feature.h>

#ifdef _MSC_VER
   #pragma warning(pop)
#endif

using std::string;
using std::vector;
using std::stringstream;
using std::ofstream;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::setiosflags;
using std::resetiosflags;
using std::setprecision;
using std::setw;
using std::getline;
using std::find;
using std::transform;

//===================================================== platform-specific stuff =================================================
#ifdef _WIN32
   #define           access   _access
   #define           F_OK     0                                   // Test for file existence
#endif

#ifdef _MSC_VER
   // MS Visual C++, byte order is IEEE little-endian, 32-bit
   #ifdef _DEBUG
      #include <crtdbg.h>                          // useful
   #endif

   // clock_t is a signed long: see <time.h>
   long const     CLOCK_T_MIN                      = LONG_MIN;
   double const   CLOCK_T_RANGE                    = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
   #ifdef _M_ALPHA
      string const PLATFORM                   = "Alpha/MS Visual C++";
   #elif defined _M_IX86
      string const PLATFORM                   = "Intel x86/MS Visual C++";
   #elif defined _M_MPPC
      string const PLATFORM                   = "Power PC/MS Visual C++";
   #elif defined _M_MRX000
      string const PLATFORM                   = "MIPS/MS Visual C++";
   #else
      string const PLATFORM                   = "Other/MS Visual C++";
   #endif
#endif

#ifdef __GNUG__
   // GNU C++
   #ifndef CPU
      #error GNU CPU not defined!
   #else
      #ifdef x86
         // Intel x86, byte order is little-endian, 32-bit
         string const PLATFORM                = "Intel x86/GNU C++";
         // clock_t is an unsigned long: see <time.h>
         unsigned long const CLOCK_T_MIN           = 0;
         double const CLOCK_T_RANGE                = static_cast<double>(ULONG_MAX);

      #elif defined rs6000
         // IBM RS-6000, byte order is big-endian, 32-bit
         string const PLATFORM                = "IBM RS-6000/GNU C++";
         // clock_t is a signed long: see <time.h> NEED TO CHECK
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #elif defined ultrasparc
         // Sun UltraSparc, byte order is big-endian, 32-bit
         string const   PLATFORM              = "Sun UltraSPARC/GNU C++";
         // clock_t is a signed long: see <time.h>
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #else
         // Something else, assume 32-bit
         string const PLATFORM                = "Other/GNU C++";
         // clock_t is a signed long: NEED TO CHECK <time.h>
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #endif
   #endif
#endif

#ifdef __MINGW32__
   // Minimalist GNU for Windows
//   #define __USE_MINGW_ANSI_STDIO 1        // Fix long doubles output problem, see http://stackoverflow.com/questions/7134547/gcc-printf-and-long-double-leads-to-wrong-output-c-type-conversion-messes-u

   #define WEXITSTATUS(x) ((x) & 0xff)
#endif

#ifdef __HP_aCC
   // HP-UX aCC, byte order is big-endian, can be either 32-bit or 64-bit
   string const PLATFORM                      = "HP-UX aC++";
   // clock_t is an unsigned long: see <time.h>
   unsigned long const CLOCK_T_MIN                 = 0;
   #ifdef __ia64
      // However, clock_t is a 32-bit unsigned long and we are using 64-bit unsigned longs here
      double const CLOCK_T_RANGE                      = 4294967295UL;   // crude, improve
   #else
      double const CLOCK_T_RANGE                      = static_cast<double>(ULONG_MAX);
   #endif
#endif


//===================================================== hard-wired constants ====================================================
string const   PROGNAME                      = "CoastalME 0.3";
string const   SHORTNAME                     = "CME";
string const   CME_INI                       = "cme.ini";

string const   COPYRIGHT                     = "(C) 2016 David Favis-Mortlock, Andres Payo, and Jim Hall";
string const   LINE                          = "-------------------------------------------------------------------------------";
string const   DISCLAIMER1                   = "This program is distributed in the hope that it will be useful, but WITHOUT ANY";
string const   DISCLAIMER2                   = "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A";
string const   DISCLAIMER3                   = "PARTICULAR PURPOSE. See the GNU General Public License for more details. You";
string const   DISCLAIMER4                   = "should have received a copy of the GNU General Public License along with this";
string const   DISCLAIMER5                   = "program; if not, contact the Free Software Foundation, Inc., 675 Mass Ave,";
string const   DISCLAIMER6                   = "Cambridge, MA 02139, USA.";

string const   ABOUT                         = "CoastalME (Coastal Modelling Environment) simulates the long-term behaviour of a coast.\nThis initial version is a prototype which considers simple soft cliff cross-shore effects only\n";
string const   THANKS                        = "Many thanks to:\n\tMartin Hurst\n\tIan Townend\n\n";
string const   GDALDRIVERS                   = "GDAL drivers";

string const   USAGE                         = "Usage: cme [OPTION]...";
string const   USAGE1                        = "  --gdal             List GDAL drivers";
string const   USAGE2                        = "  --about            Information about this program";
string const   USAGE3                        = "  --help             Display this text";
string const   USAGE4                        = "  --home=DIRECTORY   Specify the location of the .ini file etc.";
string const   USAGE5                        = "  --datafile=FILE    Specify the location and name of the main datafile";

string const   STARTNOTICE                   = "- Started on ";
string const   INITNOTICE                    = "- Initializing";
string const   READFILELOC                   = "  - Reading file locations: ";
string const   READRUNDATA                   = "  - Reading run data file: ";
string const   READBASEMENT                  = "  - Reading basement DEM: ";
string const   READRASTERFILES               = "  - Reading raster GIS files";
string const   READLCFILE                    = "    - Landform complex Class: ";
string const   READLFILE                     = "    - Landform class: ";
string const   READIFILE                     = "    - Intervention class: ";
string const   READISSFILE                   = "    - Suspended sed: ";
string const   READIFUCSFILE                 = "    - Uncons fine sed (layer ";
string const   READISUCSFILE                 = "    - Uncons sand sed (layer ";
string const   READICUCSFILE                 = "    - Uncons coarse sed (layer ";
string const   READIFCSFILE                  = "    - Cons fine sed (layer ";
string const   READISCSFILE                  = "    - Cons sand sed (layer ";
string const   READICCSFILE                  = "    - Cons coarse sed (layer ";
string const   READVECTORFILES               = "  - Reading vector GIS files";
string const   READICVFILE                   = "    - Coastline: ";
string const   READSHAPEFUNCTIONFILE         = "  - Reading shape function file";
string const   READTIDEDATAFILE              = "  - Reading tide data file: ";
string const   ALLOCATEMEMORY                = "  - Allocating memory for raster grid";
string const   ADDLAYERS                     = "  - Adding sed layers to raster grid";
string const   INITIALIZING                  = "  - Initializing";
string const   RUNNOTICE                     = "- Running simulation";
string const   SIMULATING                    = "\r  - Simulating ";
string const   FINALOUTPUT                   = "- Writing final output";
string const   SENDEMAIL                     = "  - Sending email to ";
string const   RUNENDNOTICE                  = "- Run ended on ";
string const   PRESSKEY                      = "Press any key to continue...";

string const   ERRORNOTICE                   = "- Run ended with error code ";
string const   EMAILERROR                    = "Could not send email";

string const   SHAPEFUNCTIONFILE             = "in/ShapeFunction.dat";
string const   EROSIONPOTENTIALLOOKUPFILE    = "ErosionPotential.csv";

char const     PATH_SEPARATOR                = '/';               // Works for Windows too!
char const     SPACE                         = ' ';
char const     QUOTE1                        = ';';
char const     QUOTE2                        = '#';
string const   SPACESTR                      = " ";

int const      BUFSIZE                       = 1024;              // Max length (inc. terminating NULL) of any C-type string
int const      SAVEMAX                       = 100;
int const      CLOCKCHKITER                  = 5000;
int const      NRNG                          = 2;                 // Might add more RNGs in later version
int const      OUTWIDTH                      = 103;               // Width of rh bit of .out file, wrap after this
int const      SAVGOLPOLYMAX                 = 6;                 // Maximum order of Savitsky-Golay smoothing polynomial
int const      COASTMAX                      = 1000;              // For safety check when tracing coast

// TODO does this still work on 64-bit platforms?
unsigned long const  MASK                    = 0xfffffffful;

double const   PI                            = 3.141592653589793238462643;
double const   G                             = 9.81;              // m/s2
double const   ACTIVE_ZONE_RATIO             = 0.78;              // In active zone if wave height / water depth exceeds this
double const   TOLERANCE                     = 1e-4;              // For bFPIsEqual, if too small (e.g. 1e-10), get
                                                                  // spurious "rounding" errors
double const   DODBINCREMENT                 = 0.001;             // Depth Over DB increment for erosion potential look-up function
double const   INVDODBINCREMENT              = 1000;              // Inverse of the above

double const   EROSIONPOTENTIALTOLERANCE     = -1e-12;            // Erosion potential above this (is a -ve number) is ignored

int const      ROUNDLOOPMAX                  = 50000;             // In coastline tracing, give up if round loop more than this
unsigned int const COASTMIN                  = 9;                 // Ditto, minimum number of points in a coastline

string const   ERR                           = "ERROR ";
string const   WARN                          = "WARNING ";

int const      INT_NODATA                     = -999;
double const   DBL_NODATA                     = -999;

string const   PERITERHEAD1 =
   "<------ELAPSED-------><----SEA DEPTH----><-POTENTIAL EROSION---><-------------ACTUAL EROSION----------------><--CLIFF COLLAPSE---><-----DEPOSITION----><-SUSP->";
string const   PERITERHEAD2 =
   "Time     Hours   Years       Avg    Delta   % Sea    All Eroding   % Sea     All Eroding <-sea area average-><---coast average---><------sea area average----->";
string const   PERITERHEAD3 =
   "Step                       Depth    Depth    Area    Sea    Area    Area     Sea    Area   Fine   Sand Coarse   Fine   Sand Coarse   Fine   Sand Coarse";
string const   PERITERHEAD4 =
   "                                                     Avg     Avg             Avg     Avg";

string const   PERITERHEAD   =
   "PER-ITERATION RESULTS ===================================================================================================================================================";
string const   ENDHYDROLOGYHEAD    =
   "HYDROLOGY ===============================================================================================================================================================";
string const   ENDSEDIMENTHEAD =
   "SEDIMENT MOVEMENT =======================================================================================================================================================";
string const   PERFORMHEAD   =
   "PERFORMANCE =============================================================================================================================================================";

string const   OUTEXT                              = ".out";
string const   LOGEXT                              = ".log";
string const   CSVEXT                              = ".csv";

int const      ORIENTATION_NONE                    = 0;
int const      ORIENTATION_NORTH                   = 1;
int const      ORIENTATION_EAST                    = 2;
int const      ORIENTATION_SOUTH                   = 3;
int const      ORIENTATION_WEST                    = 4;

int const      DIRECTION_FORWARD                   = 0;
int const      DIRECTION_BACKWARD                  = 1;

// Handedness codes, these show which side the sea is on when travelling along the coast (i.e. from beginning to end)
int const      NULL_HANDED                         = -1;
int const      RIGHT_HANDED                        = 0;
int const      LEFT_HANDED                         = 1;

// Time unit codes
int const      TIME_UNKNOWN                        = -1;
int const      TIME_HOURS                          = 0;
int const      TIME_DAYS                           = 1;
int const      TIME_MONTHS                         = 2;
int const      TIME_YEARS                          = 3;

// Landform category codes (see separate doc for full list, to be used eventually)
int const      LF_NONE                             = 0;
int const      LF_HINTERLAND                       = 1;
int const      LF_SEA                              = 2;
int const      LF_CLIFF                            = 3;

// GIS raster input codes
int const      FINE_CONS_RASTER                    = 1;              // Initial Consolidated Fine Sediment GIS raster data
int const      SAND_CONS_RASTER                    = 2;              // Initial Consolidated Sand Sediment GIS raster data
int const      COARSE_CONS_RASTER                  = 3;              // Initial Consolidated Coarse Sediment GIS raster data
int const      FINE_UNCONS_RASTER                  = 4;              // Initial Unconsolidated Fine Sediment raster GIS data
int const      SAND_UNCONS_RASTER                  = 5;              // Initial Unconsolidated Sand Sediment GIS raster data
int const      COARSE_UNCONS_RASTER                = 6;              // Initial Unconsolidated Coarse Sediment GIS raster data
int const      SUSP_SED_RASTER                     = 7;              // Initial Suspended Sediment GIS raster data
int const      LANDFORM_RASTER                     = 8;              // Initial Landform Class GIS raster data
int const      INTERVENTION_RASTER                 = 9;              // Initial Intervention Class GIS raster data

// GIS vector data type codes
int const      VEC_FIELD_DATA_ANY                  = 0;
int const      VEC_FIELD_DATA_INT                  = 1;
int const      VEC_FIELD_DATA_REAL                 = 2;
int const      VEC_FIELD_DATA_STRING               = 3;
int const      VEC_FIELD_DATA_OTHER                = 4;

// GIS vector geometry codes
int const      VEC_GEOMETRY_POINT                  = 1;
int const      VEC_GEOMETRY_LINE                   = 2;
int const      VEC_GEOMETRY_POLYGON                = 3;
int const      VEC_GEOMETRY_OTHER                  = 4;

// GIS vector input codes and constraints
int const      COAST_VEC                           = 1;              // Initial coastline GIS vector data
int const      COAST_VEC_MAX_LAYER                 = 1;
int const      COAST_VEC_FIELD_DATA_TYPE           = VEC_FIELD_DATA_ANY;
int const      COAST_VEC_GEOMETRY                  = VEC_GEOMETRY_LINE;

// GIS raster output user codes
string const   ALL_RASTER_CODE                        = "all";

string const   BASEMENT_ELEV_RASTER_CODE              = "baseelev";
string const   BASEMENTELEVNAME                       = "baseelev";

string const   SEDIMENT_TOP_RASTER_CODE               = "sed_top_elev";
string const   SEDIMENTTOPNAME                        = "sedtopelev";

string const   LOCAL_SLOPE_CODE                       = "local_slope";
string const   LOCALSLOPENAME                         = "localslope";

string const   WATER_DEPTH_RASTER_CODE                = "water_depth";
string const   WATERDEPTHNAME                         = "waterdepth";

string const   WAVE_HEIGHT_RASTER_CODE                = "wave_height";
string const   WAVEHEIGHTNAME                         = "waveheight";

string const   BINARY_POTENTIAL_EROSION_RASTER_CODE   = "binary_erosion";
string const   BINARYPOTENTIALEROSIONNAME             = "binaryerosion";

string const   POTENTIAL_EROSION_RASTER_CODE          = "potential_erosion";
string const   POTENTIALEROSIONNAME                   = "erosion_potential";

string const   ACTUAL_EROSION_RASTER_CODE             = "actual_erosion";
string const   ACTUALEROSIONNAME                      = "erosion_actual";

string const   TOTAL_POTENTIAL_EROSION_RASTER_CODE    = "total_potential_erosion";
string const   TOTALPOTENTIALEROSIONNAME              = "erosion_potential_total";

string const   TOTAL_ACTUAL_EROSION_RASTER_CODE       = "total_actual_erosion";
string const   TOTALACTUALEROSIONNAME                 = "erosion_actual_total";

string const   LANDFORM_RASTER_CODE                   = "landform_class";
string const   LANDFORMNAME                           = "lformclass";

string const   INTERVENTION_RASTER_CODE               = "intervention_class";
string const   INTERVENTIONNAME                       = "intervclass";

string const   SUSP_SED_RASTER_CODE                   = "susp_sed";
string const   SUSPSEDNAME                            = "suspsed";

string const   FINE_UNCONS_RASTER_CODE                = "uncons_sed_fine";
string const   FINEUNCONSSEDNAME                      = "unconssedfine";

string const   SAND_UNCONS_RASTER_CODE                = "uncons_sed_sand";
string const   SANDUNCONSSEDNAME                      = "unconssedsand";

string const   COARSE_UNCONS_RASTER_CODE              = "uncons_sed_coarse";
string const   COARSEUNCONSSEDNAME                    = "unconssedcoarse";

string const   FINE_CONS_RASTER_CODE                  = "cons_sed_fine";
string const   FINECONSSEDNAME                        = "conssedfine";

string const   SAND_CONS_RASTER_CODE                  = "cons_sed_sand";
string const   SANDCONSSEDNAME                        = "conssedsand";

string const   COARSE_CONS_RASTER_CODE                = "cons_sed_coarse";
string const   COARSECONSSEDNAME                      = "conssedcoarse";

string const   RASTER_COAST_CODE                      = "rcoast";
string const   RASTERCOASTNAME                        = "rcoast";

string const   RASTER_COAST_NORMAL_CODE               = "rcoast_normal";
string const   RASTERCOASTNORMALNAME                  = "rcoastnormal";

string const   DISTWEIGHT_RASTER_CODE                 = "dist_weight";
string const   DISTWEIGHTNAME                         = "distweight";

string const   ACTIVEZONE_CODE                        = "active_zone";
string const   ACTIVEZONENAME                         = "activezone";

string const   COLLAPSE_RASTER_CODE                   = "collapse";
string const   COLLAPSENAME                           = "collapse";

string const   TOTAL_COLLAPSE_RASTER_CODE             = "total_collapse";
string const   TOTALCOLLAPSENAME                      = "totalcollapse";

string const   COLLAPSE_DEPOSIT_RASTER_CODE           = "collapse_deposit";
string const   COLLAPSEDEPOSITNAME                    = "collapsedeposit";

string const   TOTAL_COLLAPSE_DEPOSIT_RASTER_CODE     = "total_collapse_deposit";
string const   TOTALCOLLAPSEDEPOSITNAME               = "totalcollapsedeposit";

string const   SLICENAME                              = "slice";

// GIS raster output codes and titles
int const      PLOT_BASEMENT_ELEV                  = 1;
string const   PLOT_BASEMENT_ELEV_TITLE            = "Basement elevation";
int const      PLOT_SEDIMENT_TOP_ELEV              = 2;
string const   PLOT_SEDIMENT_TOP_ELEV_TITLE        = "Elevation of sediment top";
int const      PLOT_LOCAL_SLOPE                    = 3;
string const   PLOT_LOCAL_SLOPE_TITLE              = "Local slope";
int const      PLOT_WATER_DEPTH                    = 4;
string const   PLOT_WATER_DEPTH_TITLE              = "Water depth";
int const      PLOT_WAVE_HEIGHT                    = 5;
string const   PLOT_WAVE_HEIGHT_TITLE              = "Wave height";
int const      PLOT_ACTIVEZONE                     = 6;
string const   PLOT_ACTIVEZONE_TITLE               = "Active zone";
int const      PLOT_BINARY_POTENTIAL_EROSION       = 7;
string const   PLOT_BINARY_POTENTIAL_EROSION_TITLE = "Potential (unconstrained) erosion: yes/no";
int const      PLOT_POTENTIAL_EROSION              = 8;
string const   PLOT_POTENTIAL_EROSION_TITLE        = "Potential (unconstrained) erosion depth";
int const      PLOT_ACTUAL_EROSION                 = 9;
string const   PLOT_ACTUAL_EROSION_TITLE           = "Actual (constrained) erosion depth";
int const      PLOT_TOTAL_POTENTIAL_EROSION        = 10;
string const   PLOT_TOTAL_POTENTIAL_EROSION_TITLE  = "Total potential (unconstrained) erosion depth";
int const      PLOT_TOTAL_ACTUAL_EROSION           = 11;
string const   PLOT_TOTAL_ACTUAL_EROSION_TITLE     = "Total actual (constrained) erosion depth";
int const      PLOT_LANDFORM                       = 12;
string const   PLOT_LANDFORM_TITLE                 = "Landform class";
int const      PLOT_INTERVENTION                   = 13;
string const   PLOT_INTERVENTION_TITLE             = "Intervention class";
int const      PLOT_SUSPSED                        = 14;
string const   PLOT_SUSPSED_TITLE                  = "Suspended sediment depth";
int const      PLOT_FINECONSSED                    = 15;
string const   PLOT_FINECONSSED_TITLE              = "Consolidated fine sediment depth";
int const      PLOT_SANDCONSSED                    = 16;
string const   PLOT_SANDCONSSED_TITLE              = "Consolidated sand sediment depth";
int const      PLOT_COARSECONSSED                  = 17;
string const   PLOT_COARSECONSSED_TITLE            = "Consolidated coarse sediment depth";
int const      PLOT_FINEUNCONSSED                  = 18;
string const   PLOT_FINEUNCONSSED_TITLE            = "Unconsolidated fine sediment depth";
int const      PLOT_SANDUNCONSSED                  = 19;
string const   PLOT_SANDUNCONSSED_TITLE            = "Unconsolidated sand sediment depth";
int const      PLOT_COARSEUNCONSSED                = 20;
string const   PLOT_COARSEUNCONSSED_TITLE          = "Unconsolidated coarse sediment depth";
int const      PLOT_SLICE                          = 21;
string const   PLOT_SLICE_TITLE                    = "Slice though layers at elevation = ";
int const      PLOT_RASTER_COAST                   = 22;
string const   PLOT_RASTER_COAST_TITLE             = "Rasterized coastline";
int const      PLOT_RASTER_NORMAL                  = 23;
string const   PLOT_RASTER_NORMAL_TITLE            = "Rasterized normals to coastline";
int const      PLOT_DISTWEIGHT                     = 24;
string const   PLOT_DISTWEIGHT_TITLE               = "Inverse distance weighting value";
int const      PLOT_COLLAPSE                       = 25;
string const   PLOT_COLLAPSE_TITLE                 = "Collapse depth";
int const      PLOT_TOTAL_COLLAPSE                 = 26;
string const   PLOT_TOTAL_COLLAPSE_TITLE           = "Total of collapse depth";
int const      PLOT_COLLAPSE_DEPOSIT               = 27;
string const   PLOT_COLLAPSE_DEPOSIT_TITLE         = "Collapse deposition depth";
int const      PLOT_TOTAL_COLLAPSE_DEPOSIT         = 28;
string const   PLOT_TOTAL_COLLAPSE_DEPOSIT_TITLE   = "Total of collapse deposition depth";

// GIS vector output user codes
string const   ALL_VECTOR_CODE                     = "all";
string const   VECTOR_COAST_CODE                   = "coast";
string const   COASTNAME                           = "coast";
string const   VECTOR_NORMALS_CODE                 = "normals";
string const   NORMALSNAME                         = "normals";
string const   COLLAPSENORMALSNAME                 = "collapse_normals";
string const   VECTOR_COAST_CURVATURE_CODE         = "coast_curvature";
string const   COASTCURVATURENAME                  = "coast_curvature";
string const   WAVE_ANGLE_CODE                     = "wave_angle";
string const   WAVEANGLENAME                       = "waveangle";

// GIS vector output codes and titles
int const      PLOT_COAST                          = 1;
string const   PLOT_COAST_TITLE                    = "Coastline";
int const      PLOT_NORMALS                        = 2;
string const   PLOT_NORMALS_TITLE                  = "Coastline-normal profiles";
int const      PLOT_COAST_CURVATURE                = 3;
string const   PLOT_COAST_CURVATURE_TITLE          = "Coastline curvature";
int const      PLOT_WAVE_ANGLE                     = 4;
string const   PLOT_WAVE_ANGLE_TITLE               = "Wave angle";

// Time series codes
string const   SEAAREATSNAME                       = "sea_area";
string const   SEAAREATSCODE                       = "seaarea";

string const   STILLWATERLEVELTSNAME               = "still_water_level";
string const   STILLWATERLEVELCODE                 = "waterlevel";

string const   EROSIONTSNAME                       = "erosion";
string const   EROSIONTSCODE                       = "erosion";

string const   DEPOSITIONTSNAME                    = "deposition";
string const   DEPOSITIONTSCODE                    = "deposition";

string const   SEDLOSSFROMGRIDTSNAME               = "sediment_loss";
string const   SEDLOSTFROMGRIDTSCODE               = "sedlost";

string const   SUSPSEDTSNAME                       = "suspended_sediment";
string const   SUSPSEDTSCODE                       = "suspended";

// Return codes
int const      RTN_OK                              = 0;
int const      RTN_HELPONLY                        = 1;
int const      RTN_CHECKONLY                       = 2;
int const      RTN_USERABORT                       = 3;
int const      RTN_ERR_BADPARAM                    = 4;
int const      RTN_ERR_INI                         = 5;
int const      RTN_ERR_CMEDIR                      = 6;
int const      RTN_ERR_RUNDATA                     = 7;
int const      RTN_ERR_SHAPEFUNCTIONFILE           = 8;
int const      RTN_ERR_TIDEDATAFILE                = 9;
int const      RTN_ERR_LOGFILE                     = 10;
int const      RTN_ERR_OUTFILE                     = 11;
int const      RTN_ERR_TSFILE                      = 12;
int const      RTN_ERR_DEMFILE                     = 13;
int const      RTN_ERR_RASTER_FILE_READ            = 14;
int const      RTN_ERR_VECTOR_FILE_READ            = 15;
int const      RTN_ERR_MEMALLOC                    = 16;
int const      RTN_ERR_RASTER_GIS_OUT_FORMAT       = 17;
int const      RTN_ERR_VECTOR_GIS_OUT_FORMAT       = 18;
int const      RTN_ERR_TEXTFILEWRITE               = 19;
int const      RTN_ERR_RASTER_FILE_WRITE           = 20;
int const      RTN_ERR_VECTOR_FILE_WRITE           = 21;
int const      RTN_ERR_TSFILEWRITE                 = 22;
int const      RTN_ERR_LINETOGRID                  = 23;
int const      RTN_ERR_NOSEACELLS                  = 24;
int const      RTN_ERR_GRIDTOLINE                  = 25;
int const      RTN_ERR_FINDCOAST                   = 26;
int const      RTN_ERR_MASSBALANCE                 = 27;
int const      RTN_ERR_PROFILEWRITE                = 28;
int const      RTN_ERR_TIMEUNITS                   = 29;
int const      RTN_ERR_BADENDPOINT                 = 30;
int const      RTN_ERR_OFFGRIDENDPOINT             = 31;
int const      RTN_ERR_CLIFFNOTCH                  = 32;
int const      RTN_ERR_CLIFFDEPOSIT                = 33;

// 'SLICE' CODES
int const      ELEV_IN_BASEMENT                   = -1;
int const      ELEV_ABOVE_SEDIMENT_TOP            = 999;

// CODES FOR LINEAR FEATURES ONTO/FROM GRID
int const      COAST_LINE                          = 1;
int const      COAST_NORMAL_PROFILE_LINE           = 2;

// VECTOR SMOOTHING CODES
int const      SMOOTH_NONE                         = 0;
int const      SMOOTH_RUNNING_MEAN                 = 1;
int const      SMOOTH_SAVITZKY_GOLAY               = 2;


//================================================ Globally-available functions =================================================
template <class T> T tMax(T a, T b)
{
   return ((a > b) ? a : b);
}

template <class T> T tMax(T a, T b, T c)
{
   T max = (a < b ) ? b : a;
   return (( max < c ) ? c : max);
}

template <class T> T tMin(T a, T b)
{
   return ((a < b) ? a : b);
}

template <class T> T tAbs(T a)
{
   // From a posting dated 18 Nov 93 by rmartin@rcmcon.com (Robert Martin), archived in cpp_tips
   return ((a < 0) ? -a : a);
}

// Definitions are in utilsglobal.cpp
bool bIsNumber(double const);
bool bIsFinite(double const);
extern double dRound(double const);
extern double dGetMean(vector<double>* const);
extern double dGetStdDev(vector<double>* const);
extern string strTrim(string*);
extern string strTrimLeft(string const*);
extern string strTrimRight(string const*);
extern string strToLower(string*);
extern string strToUpper(string*);
extern string strRemoveSubstr(string*, string const*);
extern vector<string>* strSplit(string const*, char const, vector<string>*);
extern vector<string> strSplit(string const*, char const);
extern string pstrChangeToBackslash(string const*);
extern string pstrChangeToForwardSlash(string const*);

// Some public domain utility routines, definitions are in utilsglobal.cpp
extern "C"
{
   void* MoveStr(char* dest, char* const source);
   char* pszToLower(char* string);
   char* pszToUpper(char* string);
   char* pszTrimLeft(char* string);
   char* pszTrimRight(char* string);
   char* pszLongToSz(long num, char* string, int max_chars, int base = 10);

   // And some of my own
   char* pszRemoveSubstr(char* string, char* substr, char* subpos);
}

//================================================= debugging stuff =============================================================
//#define CLOCKCHECK          // Uncomment to check CPU clock rollover settings
//#define RANDCHECK           // Uncomment to check randomness of random number generator

#endif // CME_H
