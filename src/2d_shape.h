/*!
 *
 * \class CA2DShape
 * \brief Abstract class, used as a base class for 2D objects (line, area, etc.)
 * \details TODO This is a more detailed description of the CA2DShape abstract class.
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \date 2017
 * \copyright GNU General Public License
 *
 * \file 2d_shape.h
 * \brief Contains CA2DShape definitions
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
#include <vector>
using std::vector;

#include "2d_point.h"


class CA2DShape
{
protected:
    vector<CGeom2DPoint> m_VPoints;

public:
   CA2DShape(void);
   virtual ~CA2DShape(void);
   CGeom2DPoint& operator[] (int const);
   void Clear(void);
   void Resize(int const);
//    void InsertAtFront(double const, double const);
   void Append(CGeom2DPoint const*);
   void Append(double const, double const);
   int nGetSize(void) const;
//    void SetPoints(const vector<CGeom2DPoint>*);
//    int nLookUp(CGeom2DPoint*);
//    double dGetLength(void) const;
   vector<CGeom2DPoint>* pPtVGetPoints(void);

   CGeom2DPoint PtGetCentroid(void);

   virtual void Display() = 0;
};
#endif // C2DSHAPE_H

