/*!
 *
 * \class C2DIPoint
 * \brief Class used to represent 2D integer point objects
 * \details TODO This is a more detailed description of the C2DIPoint class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file 2dipoint.h
 * \brief Contains C2DPoint definitions
 *
 */

#ifndef C2DIPOINT_H
#define C2DIPOINT_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
class C2DIPoint
{
private:
   int nX, nY;

public:
   C2DIPoint(void);
   C2DIPoint(int const, int const);
   int nGetX(void) const;
   int nGetY(void) const;
   void SetX(int const);
   void SetY(int const);
   void SetXY(int const, int const);
   void SetXY(C2DIPoint const*);
   void operator= (C2DIPoint*);
   bool operator== (C2DIPoint*) const;
   bool operator== (C2DIPoint) const;
};
#endif // C2DIPOINT_H
