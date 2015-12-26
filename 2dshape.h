/*!
 *
 * \class C2DShape
 * \brief Abstract class, used as a base class for 2D objects (line, area, etc.)
 * \details TODO This is a more detailed description of the C2DShape abstract class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \author Jim Hall
 * \date 2015
 * \copyright GNU General Public License
 *
 * \file 2dshape.h
 * \brief Contains C2DShape definitions
 *
 */

#ifndef C2DSHAPE_H
#define C2DSHAPE_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "2dpoint.h"

class C2DShape
{
protected:
    vector<C2DPoint> m_VPoints;

public:
   C2DShape(void);
   virtual ~C2DShape(void);
   C2DPoint& operator[] (int);
   void Clear(void);
   void Resize(const int);
   void InsertAtFront(double const, double const);
   void Append(const C2DPoint*);
   void Append(double const, double const);
   int nGetSize(void) const;
   void SetPoints(const vector<C2DPoint>*);
   int nLookUp(C2DPoint*);
   double dGetLength(void) const;
   vector<C2DPoint>* pVGetPoints(void);
   vector<C2DPoint> VGetPerpendicular(const C2DPoint*, const C2DPoint*, double const, int const);
   C2DPoint PtGetPerpendicular(const C2DPoint*, const C2DPoint*, double const, int const);
   // Pure virtual function
   virtual void Display() = 0;

};
#endif // C2DSHAPE_H

