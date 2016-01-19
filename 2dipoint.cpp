/*!
 *
 * \file 2dipoint.cpp
 * \brief Class used to represent 2D point objects with integer co-ordinates
 * \details The C2DIPoint class is used to represent 2D points where the x and y co-ordinates can only be integer values, e.g. points for which the x and y co-ordinates are in the raster-grid CRS (co-ordinate reference system)
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
#include "2dipoint.h"

//! Constructor with no parameters (the x and y co-ordinates of the C2DIPoint object are set to zero)
C2DIPoint::C2DIPoint(void)
{
   nX = nY = 0;
}

//! Constructor with two integer parameters, for the x and y co-ordinates of the C2DIPoint object
C2DIPoint::C2DIPoint(int const nNewX, int const nNewY)
{
   nX = nNewX;
   nY = nNewY;
}

//! Returns the C2DIPoint object's integer x co-ordinate
int C2DIPoint::nGetX(void) const
{
   return nX;
}

//! Returns the C2DIPoint object's integer y co-ordinate
int C2DIPoint::nGetY(void) const
{
   return nY;
}

//! The integer parameter sets a value for the C2DIPoint object's x co-ordinate
void C2DIPoint::SetX(int const nNewX)
{
   nX = nNewX;
}

//! The integer parameter sets a value for the C2DIPoint object's y co-ordinate
void C2DIPoint::SetY(int const nNewY)
{
   nY = nNewY;
}

//! The two integer parameters set values for the C2DIPoint object's x and y co-ordinates
void C2DIPoint::SetXY(int const nNewX, int const nNewY)
{
   nX = nNewX;
   nY = nNewY;
}

//! The parameter is a pointer to a C2DIPoint object, this is used to set values for the C2DIPoint object's x and y co-ordinates
void C2DIPoint::SetXY(C2DIPoint const* Pti)
{
   nX = Pti->nGetX();
   nY = Pti->nGetY();
}

//! Sets one C2DIPoint object to be the same as another C2DIPoint object
void C2DIPoint::operator= (C2DIPoint* Pti)
{
   nX = Pti->nGetX();
   nY = Pti->nGetY();
}

//! Returns true if a pointed-to C2DIPoint object is the same as this C2DIPoint object, returns false otherwise
bool C2DIPoint::operator== (C2DIPoint* Pti) const
{
   if ((Pti->nGetX() == nX) && (Pti->nGetY() == nY))
      return true;

   return false;
}

//! Returns true if a C2DIPoint object is the same as this C2DIPoint object, returns false otherwise
bool C2DIPoint::operator== (C2DIPoint Pti) const
{
   if ((Pti.nGetX() == nX) && (Pti.nGetY() == nY))
      return true;

   return false;
}
