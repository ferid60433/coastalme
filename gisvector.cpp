/*!
 *
 * \file gisvector.cpp
 * \brief These functions use OGR/GDAL to read and write vector GIS files in several formats
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
#include "sediment.h"
#include "simulation.h"

/*==============================================================================================================================

 Reads vector GIS datafiles NOT NOW USED BUT MAY BE SOMEDAY

===============================================================================================================================*/
int CSimulation::nReadVectorGISData(int const nDataItem)
{
   int
      nMaxLayer = 0,
      nNeedFieldType = 0,
      nNeedGeometry = 0;
   string
      strGISFile,
      strDriverCode,
      strGeometry,
      strDataType,
      strDataValue;

   // Set up file name and constraints
   switch (nDataItem)
   {
      case (COAST_VEC):
         strGISFile = m_strInitialCoastlineFile;
         nMaxLayer = COAST_VEC_MAX_LAYER;
         nNeedFieldType = COAST_VEC_FIELD_DATA_TYPE,
         nNeedGeometry = COAST_VEC_GEOMETRY;
         break;

      // TODO Others
   }

   // Open the OGR datasource
   OGRDataSource* pOGRDataSource = NULL;
   pOGRDataSource = OGRSFDriverRegistrar::Open(strGISFile.c_str(), FALSE);
   if (pOGRDataSource == NULL)
   {
      // Can't open file (note will already have sent GDAL error message to stdout)
      cerr << ERR << "cannot open " << strGISFile << " for input: " << CPLGetLastErrorMsg() << endl;
      return RTN_ERR_VECTOR_FILE_READ;
   }

   // Opened OK, so get dataset information
   strDriverCode = pOGRDataSource->GetDriver()->GetName();

   // Find out number of layers, and compare with the required number
   int nLayer = pOGRDataSource->GetLayerCount();
   if (nLayer > nMaxLayer)
      LogStream << WARN << "need " << nMaxLayer << (nMaxLayer > 1 ? "layers" : "layer") << " in " << strGISFile << ", " << nLayer << " found. Only the first " << nMaxLayer << (nMaxLayer > 1 ? "layers" : "layer") << " will be read." << endl;

   for (int n = 0; n < nMaxLayer; n++)
   {
      // Open this layer
      OGRLayer* pOGRLayer;
      pOGRLayer = pOGRDataSource->GetLayer(n);

      // Get features from the layer
      OGRFeature* pOGRFeature;

      // Make sure we are at the beginning of the layer, then iterate through all features in the layer
      pOGRLayer->ResetReading();
      while ((pOGRFeature = pOGRLayer->GetNextFeature()) != NULL)
      {
         OGRFeatureDefn* pOGRFeatureDefn = pOGRLayer->GetLayerDefn();
         for (int nField = 0; nField < pOGRFeatureDefn->GetFieldCount(); nField++)
         {
            OGRFieldDefn* pOGRFieldDefn = pOGRFeatureDefn->GetFieldDefn(nField);

            int nFieldType = pOGRFieldDefn->GetType();
            int nThisFieldType = 0;
            switch (nFieldType)
            {
               case OFTInteger:
                  nThisFieldType = VEC_FIELD_DATA_INT;
                  strDataType = "integer";
                  break;

               case OFTReal:
                  nThisFieldType = VEC_FIELD_DATA_REAL;
                  strDataType = "real";
                  break;

               case OFTString:
                  nThisFieldType = VEC_FIELD_DATA_STRING;
                  strDataType = "string";
                  break;

               default:
                  nThisFieldType = VEC_FIELD_DATA_OTHER;
                  strDataType = "other";
                  break;
            }

            // Check whether we have the expected field data type
            if (nNeedFieldType != VEC_FIELD_DATA_ANY)
            {
               if (nThisFieldType != nNeedFieldType)
               {
                  // Error: we have not got the expected field type
                  string strNeedType;
                  switch (nNeedFieldType)
                  {
                     case VEC_FIELD_DATA_INT:
                        strNeedType = "integer";
                        break;

                     case VEC_FIELD_DATA_REAL:
                        strNeedType = "real";
                        break;

                     case VEC_FIELD_DATA_STRING:
                        strNeedType = "string";
                        break;

                     case VEC_FIELD_DATA_OTHER:
                        strNeedType = "other";
                        break;
                  }

                  cerr << ERR << strDataType << " field data found in " << strGISFile << ", but " << strNeedType << " field data needed." << endl;
                  return RTN_ERR_VECTOR_FILE_READ;
               }
            }

            // OK we have the desired field data type, so get the value
            // TODO WILL WE EVER ACTUALLY USE THIS VALUE? May as well just get it as a string for later display
            strDataValue = pOGRFeature->GetFieldAsString(nField);
/*          switch (nFieldType)
            {
               case OFTInteger:
                  pOGRFeature->GetFieldAsInteger(nField);
                  break;

               case OFTReal:
                  pOGRFeature->GetFieldAsDouble(nField);
                  break;

               case OFTString:
                  pOGRFeature->GetFieldAsString(nField);
                  break;

               default:
                  pOGRFeature->GetFieldAsString(nField);
                  break;
            } */
         }

         // Now get the geometry
         OGRGeometry* pOGRGeometry;
         pOGRGeometry = pOGRFeature->GetGeometryRef();
         if (pOGRGeometry == NULL)
         {
            cerr << ERR << " null geometry in " << strGISFile << "." << endl;
            return RTN_ERR_VECTOR_FILE_READ;
         }

         // And then get the geometry type
         int nGeometry = wkbFlatten(pOGRGeometry->getGeometryType());
         int nThisGeometry = 0;
         switch (nGeometry)
         {
            case wkbPoint:
               nThisGeometry = VEC_GEOMETRY_POINT;
               strGeometry = "point";
               break;

            case wkbLineString:
               nThisGeometry = VEC_GEOMETRY_LINE;
               strGeometry = "line";
               break;

            case wkbPolygon:
               nThisGeometry = VEC_GEOMETRY_POLYGON;
               strGeometry = "polygon";
               break;

            default:
               // NOTE may need wkbMultiLineString or similar for channel network
               nThisGeometry = VEC_GEOMETRY_OTHER;
               strGeometry = "other";
               break;
         }

         // Have we got the expected geometry?
         if (nThisGeometry != nNeedGeometry)
         {
            // Error, we do not have the desired geometry
            string strNeedGeometry;
            switch (nNeedGeometry)
            {
               case VEC_FIELD_DATA_INT:
                  strNeedGeometry = "integer";
                  break;

               case VEC_FIELD_DATA_REAL:
                  strNeedGeometry = "real";
                  break;

               case VEC_FIELD_DATA_STRING:
                  strNeedGeometry = "string";
                  break;

               case VEC_FIELD_DATA_OTHER:
                  strNeedGeometry = "other";
                  break;
            }

            cerr << strGeometry << " data found in " << strGISFile << ", but " << strNeedGeometry << " data needed." << endl;
            return RTN_ERR_VECTOR_FILE_READ;
         }

         // The geometry is OK, so (at last) process the data
         int nPoints = 0;
         OGRPoint* pOGRPoint;
         OGRLineString* pOGRLineString;
         switch (nGeometry)
         {
            case wkbPoint:
               // Point data
               pOGRPoint = (OGRPoint *) pOGRGeometry;
               // TODO write the rest of this
               cout << "Point: x = " << pOGRPoint->getX() << ", y = " << pOGRPoint->getY() << endl;
               break;

            case wkbLineString:
               // Line data
               pOGRLineString = (OGRLineString *) pOGRGeometry;
               nPoints = pOGRLineString->getNumPoints();
               for (int i = 0; i < nPoints; i++)
               {
                  switch (nDataItem)
                  {
                     case (COAST_VEC):
                        // Add this point to the coastline. Note that we are assuming only one coastline object here
                        m_VCoast[0].AppendToCoast(pOGRLineString->getX(i), pOGRLineString->getY(i));
                        break;

                     // TODO others
                  }
               }
               break;

            case wkbPolygon:
               // Polygon data
               // TODO write this
               break;

            default:
               // NOTE will need wkbMultiLineString or similar for channel network
               // TODO write this
               break;
         }

         // Get rid of the Feature object
         OGRFeature::DestroyFeature(pOGRFeature);

         // Pass on some info to show in the text output
         switch (nDataItem)
         {
            case (COAST_VEC):
               m_strOGRICDriverCode = strDriverCode;
               m_strOGRICDataType   = strDataType;
               m_strOGRICDataValue  = strDataValue;
               m_strOGRICGeometry   = strGeometry;
               break;

            // TODO Others

         }
      }
   }

   // Clean up: get rid of the data source object
   OGRDataSource::DestroyDataSource(pOGRDataSource);

   return RTN_OK;
}


/*==============================================================================================================================

 Writes vector GIS files using OGR

===============================================================================================================================*/
bool CSimulation::bWriteVectorGIS(int const nDataItem, string const* strPlotTitle)
{
   // Begin constructing the file name for this save
   string strFilePathName(m_strOutPath);

   switch (nDataItem)
   {
      case (PLOT_COAST):
         strFilePathName.append(COASTNAME);
         break;

      case (PLOT_NORMALS):
         strFilePathName.append(NORMALSNAME);
         break;

      case (PLOT_COAST_CURVATURE):
         strFilePathName.append(COASTCURVATURENAME);
         break;

      case (PLOT_WAVE_ANGLE):
         strFilePathName.append(WAVEANGLENAME);
         break;
   }

   // Append the 'save number' to the filename
   if (m_nGISSave > 99)
   {
      // For save numbers of three or more digits, don't prepend zeros (note 10 digits is max)
      char szNumTmp[10] = "";
      strFilePathName.append(pszTrimLeft(pszLongToSz(m_nGISSave, szNumTmp, 10)));
   }
   else
   {
      // Prepend zeros to the save number
      char szNumTmp[3] = "";
      pszLongToSz(m_nGISSave, szNumTmp, 3);
      strFilePathName.append(pszTrimLeft(szNumTmp));
   }

   // Make a copy of the filename without any extension
   string strFilePathNameNoExt = strFilePathName;

   // If desired, append an extension
   if (! m_strOGRVectorOutputExtension.empty())
      strFilePathName.append(m_strOGRVectorOutputExtension);

   // Set up the driver
   OGRSFDriver* pOGRDriver = NULL;
   pOGRDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(m_strVectorGISOutFormat.c_str());
   if (pOGRDriver == NULL)
   {
      cerr << ERR << "vector GIS output driver " << m_strVectorGISOutFormat << CPLGetLastErrorMsg() << endl;
      return false;
   }

   // If the datasource already exists, delete it (otherwise subsequent creation will fail)
   if (0 == access(strFilePathName.c_str(), F_OK))
      // It exists, so delete it
      pOGRDriver->DeleteDataSource(strFilePathName.c_str());

   // Now create the data source output files
   OGRDataSource* pOGRDataSource = NULL;
   char** papszOptions = NULL;                     // TODO add driver-specific options if needed
   pOGRDataSource = pOGRDriver->CreateDataSource(strFilePathName.c_str(), papszOptions);
   if (pOGRDataSource == NULL)
   {
      cerr << ERR << "cannot create " << m_strVectorGISOutFormat << " named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      return false;
   }

   // Create the output layer
   OGRLayer* pOGRLayer = NULL;
   OGRSpatialReference* pOGRSpatialRef = NULL;     // TODO add spatial reference
   OGRwkbGeometryType eGType = wkbUnknown;
   string strType = "unknown";
   papszOptions = NULL;                            // TODO add driver-specific options if needed

   pOGRLayer = pOGRDataSource->CreateLayer(strFilePathNameNoExt.c_str(), pOGRSpatialRef, eGType, papszOptions);
   if (pOGRLayer == NULL)
   {
      cerr << ERR << "cannot create '" << strType << "' layer in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      return false;
   }

   switch (nDataItem)
   {
      case (PLOT_COAST):
      {
//          eGType = wkbMultiLineString;
//          strType = "multi-line";
         eGType = wkbLineString;
         strType = "line";

         // The layer has been created, so create an integer-numbered value (the number of the coast object) for the multi-line
         string strFieldValue1 = "Coast";
         OGRFieldDefn OGRField1(strFieldValue1.c_str(), OFTInteger);
         if (pOGRLayer->CreateField(&OGRField1) != OGRERR_NONE)
         {
            cerr << ERR << "cannot create " << strType << " attribute field 1 '" << strFieldValue1 << "' in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
            return false;
         }

         // OK, now do features
         OGRLineString OGRls;
//         OGRMultiLineString OGRmls;

         for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
         {
            // Create a feature object, one per coast
            OGRFeature *pOGRFeature = NULL;
            pOGRFeature = OGRFeature::CreateFeature(pOGRLayer->GetLayerDefn());

            // Set the feature's attribute (the coast number)
            pOGRFeature->SetField(strFieldValue1.c_str(), i);

            // Now attach a geometry to the feature object
            for (int j = 0; j < m_VCoast[i].GetCoastline()->nGetSize(); j++)
               //  In external CRS
               OGRls.addPoint(m_VCoast[i].pPtGetVectorCoastlinePoint(j)->dGetX(), m_VCoast[i].pPtGetVectorCoastlinePoint(j)->dGetY());

//            OGRmls.addGeometry(&OGRls);
//            pOGRFeature->SetGeometry(&OGRmls);
            pOGRFeature->SetGeometry(&OGRls);

            // Create the feature in the output layer
            if (pOGRLayer->CreateFeature(pOGRFeature) != OGRERR_NONE)
            {
               cerr << ERR << "cannot create  " << strType << " feature " << strPlotTitle << " for coast " << i << " in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
               return false;
            }

            // Tidy up: empty the line string and get rid of the feature object
            OGRls.empty();
            OGRFeature::DestroyFeature(pOGRFeature);
         }

         break;
      }

      case (PLOT_NORMALS):
      {
//          eGType = wkbMultiLineString;
//          strType = "multi-line";
         eGType = wkbLineString;
         strType = "line";

         // The layer has been created, so create an integer-numbered value (the number of the normal) associated with the line
         string strFieldValue1 = "Normal";
         OGRFieldDefn OGRField1(strFieldValue1.c_str(), OFTInteger);
         if (pOGRLayer->CreateField(&OGRField1) != OGRERR_NONE)
         {
            cerr << ERR << "cannot create " << strType << " attribute field 1 '" << strFieldValue1 << "' in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
            return false;
         }


//          // OK, now create features
//          OGRLineString OGRls;
//          OGRMultiLineString OGRmls;
//
//          for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
//          {
//             // Create a feature object, one per coast
//             OGRFeature *pOGRFeature = NULL;
//             pOGRFeature = OGRFeature::CreateFeature(pOGRLayer->GetLayerDefn());
//
//             // Set the feature's attribute
//             pOGRFeature->SetField(strFieldValue1.c_str(), i);
//
//             // Now attach a geometry to the feature object
//             for (int j = 0; j < m_VCoast[i].nGetNumProfiles(); j++)
//             {
//                for (int k = 0; k < m_VCoast[i].pGetProfile(j)->nGetNumVecPointsInProfile(); k++)
//                   OGRls.addPoint(m_VCoast[i].pGetProfile(j)->PtGetVecPointOnProfile(k)->dGetX(), m_VCoast[i].pGetProfile(j)->PtGetVecPointOnProfile(k)->dGetY());
//
//                OGRmls.addGeometry(&OGRls);
//                pOGRFeature->SetGeometry(&OGRmls);
//                OGRls.empty();
//             }
//
//             // Create the feature in the output file
//             if (pOGRLayer->CreateFeature(pOGRFeature) != OGRERR_NONE)
//             {
//                cerr << ERR << "cannot create " << strPlotTitle << " feature for coast " << i << " in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
//                return false;
//             }
//
//             // Tidy up: get rid of the feature object
//             OGRFeature::DestroyFeature(pOGRFeature);
//          }


         // OK, now create features
         OGRLineString OGRls;
//         OGRMultiLineString OGRmls;

         for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
         {
            for (int j = 0; j < m_VCoast[i].nGetNumProfiles(); j++)
            {
               // Create a feature object, one per profile
               OGRFeature *pOGRFeature = NULL;
               pOGRFeature = OGRFeature::CreateFeature(pOGRLayer->GetLayerDefn());

               // Set the feature's attributes
               pOGRFeature->SetField(strFieldValue1.c_str(), j);

               // Now attach a geometry to the feature object
               for (int k = 0; k < m_VCoast[i].pGetProfile(j)->nGetNumVecPointsInProfile(); k++)
                  OGRls.addPoint(m_VCoast[i].pGetProfile(j)->PtGetVecPointOnProfile(k)->dGetX(), m_VCoast[i].pGetProfile(j)->PtGetVecPointOnProfile(k)->dGetY());

//                OGRmls.addGeometry(&OGRls);
//                pOGRFeature->SetGeometry(&OGRmls);
//                OGRmls.empty();
               pOGRFeature->SetGeometry(&OGRls);
               OGRls.empty();

               // Create the feature in the output layer
               if (pOGRLayer->CreateFeature(pOGRFeature) != OGRERR_NONE)
               {
                  cerr << ERR << "cannot create  " << strType << " feature " << strPlotTitle << " for coast " << i << " and profile " << j << " in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
                  return false;
               }

               // Tidy up: get rid of the feature object
               OGRFeature::DestroyFeature(pOGRFeature);
            }
         }

         break;
      }

      case (PLOT_COAST_CURVATURE):
      {
         eGType = wkbPoint;
         strType = "point";

         // The layer has been created, so create a real-numbered value associated with each point
         string strFieldValue1 = "Curve";
         OGRFieldDefn OGRField1(strFieldValue1.c_str(), OFTReal);
         if (pOGRLayer->CreateField(&OGRField1) != OGRERR_NONE)
         {
            cerr << ERR << "cannot create " << strType << " attribute field 1 '" << strFieldValue1 << "' in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
            return false;
         }

         // OK, now create features
         OGRLineString OGRls;
         OGRMultiLineString OGRmls;
         OGRPoint OGRPt;

         for (int i = 0; i < static_cast<int>(m_VCoast.size()); i++)
         {
            for (int j = 0; j < m_VCoast[i].GetCoastline()->nGetSize(); j++)
            {
               // Create a feature object, one per coastline point
               OGRFeature *pOGRFeature = NULL;
               pOGRFeature = OGRFeature::CreateFeature(pOGRLayer->GetLayerDefn());

               // Set the feature's geometry (in external CRS)
               OGRPt.setX(m_VCoast[i].pPtGetVectorCoastlinePoint(j)->dGetX());
               OGRPt.setY(m_VCoast[i].pPtGetVectorCoastlinePoint(j)->dGetY());
               pOGRFeature->SetGeometry(&OGRPt);

               // Set the feature's attribute
               pOGRFeature->SetField(strFieldValue1.c_str(), m_VCoast[i].dGetCurvature(j));

               // Create the feature in the output layer
               if (pOGRLayer->CreateFeature(pOGRFeature) != OGRERR_NONE)
               {
                  cerr << ERR << "cannot create " << strType << " feature " << strPlotTitle << " for coast " << i << " point " << j << " in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
                  return false;
               }

               // Get rid of the feature object
               OGRFeature::DestroyFeature(pOGRFeature);
            }
         }

         break;
      }

      case (PLOT_WAVE_ANGLE):
      {
         eGType = wkbPoint;
         strType = "point";

         // The layer has been created, so create real-numbered values associated with each point
         string
            strFieldValue1 = "Angle",
            strFieldValue2 = "Height";

         // Create the first field
         OGRFieldDefn OGRField1(strFieldValue1.c_str(), OFTReal);
         if (pOGRLayer->CreateField(&OGRField1) != OGRERR_NONE)
         {
            cerr << ERR << "cannot create " << strType << " attribute field 1 '" << strFieldValue1 << "' in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
            return false;
         }

         // Create the second field
         OGRFieldDefn OGRField2(strFieldValue2.c_str(), OFTReal);
         if (pOGRLayer->CreateField(&OGRField2) != OGRERR_NONE)
         {
            cerr << ERR << "cannot create " << strType << " attribute field 2 '" << strFieldValue2 << "' in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
            return false;
         }

         // OK, now create features
         OGRLineString OGRls;
         OGRMultiLineString OGRmls;
         OGRPoint OGRPt;

         for (int nX = 0; nX < m_nXGridMax; nX++)
         {
            for (int nY = 0; nY < m_nYGridMax; nY++)
            {
               if (! m_pRasterGrid->Cell[nX][nY].bIsDryLand())
               {
                  // Create a feature object, one per sea cell
                  OGRFeature *pOGRFeature = NULL;
                  pOGRFeature = OGRFeature::CreateFeature(pOGRLayer->GetLayerDefn());

                  // Set the feature's geometry (in external CRS)
                  OGRPt.setX(dGridXToExtCRSX(nX));
                  OGRPt.setY(dGridYToExtCRSY(nY));
                  pOGRFeature->SetGeometry(&OGRPt);

                  // Set the feature's attributes
                  pOGRFeature->SetField(strFieldValue1.c_str(), m_pRasterGrid->Cell[nX][nY].dGetWaveOrientation());
                  pOGRFeature->SetField(strFieldValue2.c_str(), m_pRasterGrid->Cell[nX][nY].dGetWaveHeight());

                  if (m_pRasterGrid->Cell[nX][nY].dGetWaveHeight() > 3.0001)
                     LogStream << m_ulIter << ": [" << nX << "][" << nY << "] has wave height = " << m_pRasterGrid->Cell[nX][nY].dGetWaveHeight() << endl;

                  // Create the feature in the output layer
                  if (pOGRLayer->CreateFeature(pOGRFeature) != OGRERR_NONE)
                  {
                     cerr << ERR << "cannot create " << strType << " feature " << strPlotTitle << " for cell [" << nX << "][" << nY << "] in " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
                     return false;
                  }

                  // Get rid of the feature object
                  OGRFeature::DestroyFeature(pOGRFeature);
               }
            }
         }

         break;
      }
   }

   // Get rid of the datasource object
   OGRDataSource::DestroyDataSource(pOGRDataSource);

   return true;
}
