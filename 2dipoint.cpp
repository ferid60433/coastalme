/*!
 *
 * \file 2dipoint.cpp
 * \brief C2DIPoint routines
 * \details TODO A more detailed description of these routines.
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
#include "cme.h"
#include "2dipoint.h"

C2DIPoint::C2DIPoint(void)
{
   nX = nY = 0;
}

C2DIPoint::C2DIPoint(int const nNewX, int const nNewY)
{
   nX = nNewX;
   nY = nNewY;
}

int C2DIPoint::nGetX(void) const
{
   return nX;
}

int C2DIPoint::nGetY(void) const
{
   return nY;
}

void C2DIPoint::SetX(int const nNewX)
{
   nX = nNewX;
}

void C2DIPoint::SetY(int const nNewY)
{
   nY = nNewY;
}

void C2DIPoint::SetXY(int const nNewX, int const nNewY)
{
   nX = nNewX;
   nY = nNewY;
}

void C2DIPoint::SetXY(C2DIPoint const* Pti)
{
   nX = Pti->nGetX();
   nY = Pti->nGetY();
}

void C2DIPoint::operator= (C2DIPoint* Pti)
{
   nX = Pti->nGetX();
   nY = Pti->nGetY();
}

bool C2DIPoint::operator== (C2DIPoint* Pti) const
{
   if ((Pti->nGetX() == nX) && (Pti->nGetY() == nY))
      return true;

   return false;
}

bool C2DIPoint::operator== (C2DIPoint Pti) const
{
   if ((Pti.nGetX() == nX) && (Pti.nGetY() == nY))
      return true;

   return false;
}
