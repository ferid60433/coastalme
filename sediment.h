/*!
 *
 * \class CSedimentLayer
 * \brief Class used to represent a sediment layer object
 * \details TODO This is a more detailed description of the CSedimentLayer class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file sediment.h
 * \brief Contains CSedimentLayer definitions
 *
 */

#ifndef SEDIMENT_H
#define SEDIMENT_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
class CSedimentLayer
{
private:
   double
      m_dFine,                // depth equivalent in m
      m_dNotchFineLost,       // depth equivalent (m) of sediment lost via notch incision
      m_dSand,                // depth equivalent in m
      m_dNotchSandLost,       // depth equivalent (m) of sediment lost via notch incision
      m_dCoarse,              // depth equivalent in m
      m_dNotchCoarseLost;     // depth equivalent (m) of sediment lost via notch incision

public:
   CSedimentLayer(void);
   CSedimentLayer& operator= (const CSedimentLayer&);
   void SetFine(double const);
   double dGetFine(void) const;
   void SetNotchFineLost(double const);
   void IncrNotchFineLost(double const);
   double dGetNotchFineLost(void) const;
   void SetSand(double const);
   double dGetSand(void) const;
   void SetNotchSandLost(double const);
   void IncrNotchSandLost(double const);
   double dGetNotchSandLost(void) const;
   void SetCoarse(double const);
   double dGetCoarse(void) const;
   void SetNotchCoarseLost(double const);
   void IncrNotchCoarseLost(double const);
   double dGetNotchCoarseLost(void) const;
};
#endif // SEDIMENT_H
