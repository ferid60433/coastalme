/*!
 *
 * \file cme.cpp
 * \brief The start-up routine for CoastalME
 * \details TODO A more detailed description of this routine
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
// TODO Get the rest of CoastalME working :-)

// Not-urgent stuff
// TODO Rewrite the old parts of readinput.cpp to use strSplit()
// TODO Rationalize GIS layer names
// TODO check if C-strings are NULL before using string.assign()
// TODO check all uses of strcpy for buffer overflow, see http://cwe.mitre.org/data/definitions/676.html#Demonstrative%20Examples
// TODO use integers where poss; check for integer overflow, http://cwe.mitre.org/data/definitions/190.html#Demonstrative%20Examples
// TODO check call code for 64-bit porting issues, see "20 issues of porting c++ code on the 64-bit platform' in sliding drawer
// TODO Try ICC, see http://www.gentoo-wiki.info/HOWTO_ICC_and_Portage. See also http://software.intel.com/en-us/articles/non-commercial-software-download/
// TODO Read in a saved CSimulation object (use Boost for this: http://www.boost.org/doc/libs/1_39_0/libs/serialization/doc/index.html), and save one at the end; so can restart

#include "cme.h"
#include "simulation.h"

/*===============================================================================================================================

 CoastalME's main function

===============================================================================================================================*/
int main (int argc, char* argv[])
{
   // TODO This is supposed to enable the use of UTF-8 symbols in CoastalME output, such as the \u0394 character. But does it work? If not, remove it?
   setlocale(LC_ALL, "en_GB.UTF-8");

   // OK, create a CSimulation objeect (duh!)
   CSimulation* pSimulation = new CSimulation;

   // Run the simulation and then check how it ends
   int nRtn = pSimulation->nDoSimulation(argc, argv);
   pSimulation->DoSimulationEnd(nRtn);

   // Then go back to the OS
   return (nRtn);
}
