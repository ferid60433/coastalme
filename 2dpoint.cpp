/*!
 *
 * \file 2dpoint.cpp
 * \brief C2DPoint routines
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
#include "2dpoint.h"

C2DPoint::C2DPoint(void)
{
   dX = dY = 0;
}

C2DPoint::C2DPoint(double const dNewX, double const dNewY)
{
   dX = dNewX;
   dY = dNewY;
}

double C2DPoint::dGetX(void) const
{
   return dX;
}

double C2DPoint::dGetY(void) const
{
   return dY;
}

void C2DPoint::SetX(double const dNewX)
{
   dX = dNewX;
}

void C2DPoint::SetY(double const dNewY)
{
   dY = dNewY;
}

void C2DPoint::SetXY(double const dNewX, double const dNewY)
{
   dX = dNewX;
   dY = dNewY;
}

void C2DPoint::SetXY(C2DPoint const* Pt)
{
   dX = Pt->dGetX();
   dY = Pt->dGetY();
}

void C2DPoint::operator= (C2DPoint* Pt)
{
   dX = Pt->dGetX();
   dY = Pt->dGetY();
}

bool C2DPoint::operator== (C2DPoint* Pt) const
{
   if ((Pt->dGetX() == dX) && (Pt->dGetY() == dY))
      return true;

   return false;
}

bool C2DPoint::operator== (C2DPoint Pt) const
{
   if ((Pt.dGetX() == dX) && (Pt.dGetY() == dY))
      return true;

   return false;
}
