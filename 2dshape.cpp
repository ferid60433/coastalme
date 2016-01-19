/*!
 *
 * \file 2dshape.cpp
 * \brief C2DShape routines
 * \details TODO A more detailed description of these routines.
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
#include "2dshape.h"

C2DShape::C2DShape(void)
{
}

C2DShape::~C2DShape(void)
{
}

C2DPoint& C2DShape::operator[] (int n)
{
   // NOTE No safety check
   return m_VPoints[n];
}

void C2DShape::Clear(void)
{
   m_VPoints.clear();
}

void C2DShape::Resize(const int nSize)
{
   m_VPoints.resize(nSize);
}

void C2DShape::InsertAtFront(double const dX, double const dY)
{
   m_VPoints.insert(m_VPoints.begin(), C2DPoint(dX, dY));
}

void C2DShape::Append(const C2DPoint* PtNew)
{
   m_VPoints.push_back(*PtNew);
}

void C2DShape::Append(double const dX, double const dY)
{
   m_VPoints.push_back(C2DPoint(dX, dY));
}

int C2DShape::nGetSize(void) const
{
   return m_VPoints.size();
}

void C2DShape::SetPoints(const vector<C2DPoint>* VNewPoints)
{
   m_VPoints = *VNewPoints;
}

int C2DShape::nLookUp(C2DPoint* Pt)
{
   vector<C2DPoint>::iterator it = find(m_VPoints.begin(), m_VPoints.end(), *Pt);
   if (it != m_VPoints.end())
      return it - m_VPoints.begin();
   else
      return -1;
}

double C2DShape::dGetLength(void) const
{
   int nSize = m_VPoints.size();

   if (nSize < 2)
      return -1;

   double dLength = 0;
   for (int n = 1; n < nSize; n++)
   {
      double dXlen = m_VPoints[n].dGetX() - m_VPoints[n-1].dGetX();
      double dYlen = m_VPoints[n].dGetY() - m_VPoints[n-1].dGetY();

      dLength += hypot(dXlen, dYlen);
   }

   return dLength;
}

vector<C2DPoint>* C2DShape::pVGetPoints(void)
{
   return &m_VPoints;
}

vector<C2DPoint> C2DShape::VGetPerpendicular(const C2DPoint* StartPt, const C2DPoint* OtherPt, double const dDesiredLength, int const nHandedness)
{
   // Returns a two-point vector which passes through StartPt with a scaled length
   double dXLen = OtherPt->dGetX() - StartPt->dGetX();
   double dYLen = OtherPt->dGetY() - StartPt->dGetY();

   double dLength = hypot(dXLen, dYLen);
   double dScaleFactor = dDesiredLength / dLength;

   // The difference vector is (dXLen, dYLen), so the perpendicular difference vector is (-dYLen, dXLen) or (dYLen, -dXLen)
   C2DPoint EndPt;
   if (nHandedness == RIGHT_HANDED)
   {
      EndPt.SetX(StartPt->dGetX() + (dScaleFactor * dYLen));
      EndPt.SetY(StartPt->dGetY() - (dScaleFactor * dXLen));
   }
   else
   {
      EndPt.SetX(StartPt->dGetX() - (dScaleFactor * dYLen));
      EndPt.SetY(StartPt->dGetY() + (dScaleFactor * dXLen));
   }

   vector<C2DPoint> VNew;
   VNew.push_back(*StartPt);
   VNew.push_back(EndPt);
   return VNew;
}

C2DPoint C2DShape::PtGetPerpendicular(const C2DPoint* StartPt, const C2DPoint* OtherPt, double const dDesiredLength, int const nHandedness)
{
   // Returns a C2DPoint which is the 'other' point of a two-point vector passing through StartPt
   double dXLen = OtherPt->dGetX() - StartPt->dGetX();
   double dYLen = OtherPt->dGetY() - StartPt->dGetY();

   double dLength = hypot(dXLen, dYLen);
   double dScaleFactor = dDesiredLength / dLength;

   // The difference vector is (dXLen, dYLen), so the perpendicular difference vector is (-dYLen, dXLen) or (dYLen, -dXLen)
   C2DPoint EndPt;
   if (nHandedness == RIGHT_HANDED)
   {
      EndPt.SetX(StartPt->dGetX() + (dScaleFactor * dYLen));
      EndPt.SetY(StartPt->dGetY() - (dScaleFactor * dXLen));
   }
   else
   {
      EndPt.SetX(StartPt->dGetX() - (dScaleFactor * dYLen));
      EndPt.SetY(StartPt->dGetY() + (dScaleFactor * dXLen));
   }

   return EndPt;
}
