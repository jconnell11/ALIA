// jhcFRecoOCV.cpp : face recognizer based on OpenCV LBP histogram method
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#include "opencv2/contrib/contrib.hpp"
#include <set>

#include "Face/jhcFRecoOCV.h"


//= Symmetric version of function in later versions of OpenCV.

#ifndef CV_COMP_CHISQR_ALT
  #define CV_COMP_CHISQR_ALT   CV_COMP_CHISQR
#endif


///////////////////////////////////////////////////////////////////////////
//               Partial Version of OpenCV facerec.cpp                   //
///////////////////////////////////////////////////////////////////////////

/*
 * Copyright (c) 2011,2012. Philipp Wagner <bytefish[at]gmx[dot]de>.
 * Released to public domain under terms of the BSD Simplified license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the organization nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *   See <http://www.opensource.org/licenses/bsd-license>
 */

namespace cv
{

using std::set;

//------------------------------------------------------------------------------
// cv::elbp
//------------------------------------------------------------------------------
template <typename _Tp> static
inline void elbp_(InputArray _src, OutputArray _dst, int radius, int neighbors) {
    //get matrices
    Mat src = _src.getMat();
    // allocate memory for result
    _dst.create(src.rows-2*radius, src.cols-2*radius, CV_32SC1);
    Mat dst = _dst.getMat();
    // zero
    dst.setTo(0);
    for(int n=0; n<neighbors; n++) {
        // sample points
        float x = static_cast<float>(-radius * sin(2.0*CV_PI*n/static_cast<float>(neighbors)));
        float y = static_cast<float>(radius * cos(2.0*CV_PI*n/static_cast<float>(neighbors)));
        // relative indices
        int fx = static_cast<int>(floor(x));
        int fy = static_cast<int>(floor(y));
        int cx = static_cast<int>(ceil(x));
        int cy = static_cast<int>(ceil(y));
        // fractional part
        float ty = y - fy;
        float tx = x - fx;
        // set interpolation weights
        float w1 = (1 - tx) * (1 - ty);
        float w2 =      tx  * (1 - ty);
        float w3 = (1 - tx) *      ty;
        float w4 =      tx  *      ty;
        // iterate through your data
        for(int i=radius; i < src.rows-radius;i++) {
            for(int j=radius;j < src.cols-radius;j++) {
                // calculate interpolated value
                float t = static_cast<float>(w1*src.at<_Tp>(i+fy,j+fx) + w2*src.at<_Tp>(i+fy,j+cx) + w3*src.at<_Tp>(i+cy,j+fx) + w4*src.at<_Tp>(i+cy,j+cx));
                // floating point precision, so check some machine-dependent epsilon
                dst.at<int>(i-radius,j-radius) += ((t > src.at<_Tp>(i,j)) || (std::abs(t-src.at<_Tp>(i,j)) < std::numeric_limits<float>::epsilon())) << n;
            }
        }
    }
}

static void elbp(InputArray src, OutputArray dst, int radius, int neighbors)
{
    int type = src.type();
    switch (type) {
    case CV_8SC1:   elbp_<char>(src,dst, radius, neighbors); break;
    case CV_8UC1:   elbp_<unsigned char>(src, dst, radius, neighbors); break;
    case CV_16SC1:  elbp_<short>(src,dst, radius, neighbors); break;
    case CV_16UC1:  elbp_<unsigned short>(src,dst, radius, neighbors); break;
    case CV_32SC1:  elbp_<int>(src,dst, radius, neighbors); break;
    case CV_32FC1:  elbp_<float>(src,dst, radius, neighbors); break;
    case CV_64FC1:  elbp_<double>(src,dst, radius, neighbors); break;
    default:
        string error_msg = format("Using Original Local Binary Patterns for feature extraction only works on single-channel images (given %d). Please pass the image data as a grayscale image!", type);
        CV_Error(CV_StsNotImplemented, error_msg);
        break;
    }
}

static Mat
histc_(const Mat& src, int minVal=0, int maxVal=255, bool normed=false)
{
    Mat result;
    // Establish the number of bins.
    int histSize = maxVal-minVal+1;
    // Set the ranges.
    float range[] = { static_cast<float>(minVal), static_cast<float>(maxVal+1) };
    const float* histRange = { range };
    // calc histogram
    calcHist(&src, 1, 0, Mat(), result, 1, &histSize, &histRange, true, false);
    // normalize
    if(normed) {
        result /= (int)src.total();
    }
    return result.reshape(1,1);
}

static Mat histc(InputArray _src, int minVal, int maxVal, bool normed)
{
    Mat src = _src.getMat();
    switch (src.type()) {
        case CV_8SC1:
            return histc_(Mat_<float>(src), minVal, maxVal, normed);
            break;
        case CV_8UC1:
            return histc_(src, minVal, maxVal, normed);
            break;
        case CV_16SC1:
            return histc_(Mat_<float>(src), minVal, maxVal, normed);
            break;
        case CV_16UC1:
            return histc_(src, minVal, maxVal, normed);
            break;
        case CV_32SC1:
            return histc_(Mat_<float>(src), minVal, maxVal, normed);
            break;
        case CV_32FC1:
            return histc_(src, minVal, maxVal, normed);
            break;
        default:
            CV_Error(CV_StsUnmatchedFormats, "This type is not implemented yet."); break;
    }
    return Mat();
}


static Mat spatial_histogram(InputArray _src, int numPatterns,
                             int grid_x, int grid_y, bool /*normed*/)
{
    Mat src = _src.getMat();
    // calculate LBP patch size
    int width = src.cols/grid_x;
    int height = src.rows/grid_y;
    // allocate memory for the spatial histogram
    Mat result = Mat::zeros(grid_x * grid_y, numPatterns, CV_32FC1);
    // return matrix with zeros if no data was given
    if(src.empty())
        return result.reshape(1,1);
    // initial result_row
    int resultRowIdx = 0;
    // iterate through grid
    for(int i = 0; i < grid_y; i++) {
        for(int j = 0; j < grid_x; j++) {
            Mat src_cell = Mat(src, Range(i*height,(i+1)*height), Range(j*width,(j+1)*width));
            Mat cell_hist = histc(src_cell, 0, (numPatterns-1), true);
            // copy to the result matrix
            Mat result_row = result.row(resultRowIdx);
            cell_hist.reshape(1,1).convertTo(result_row, CV_32FC1);
            // increase row count in result matrix
            resultRowIdx++;
        }
    }
    // return result as reshaped feature vector
    return result.reshape(1,1);
}


}  // end of namespace


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFRecoOCV::jhcFRecoOCV ()
{
  // current code version
  ver = 1.00;

  // set processing parameters
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcFRecoOCV::~jhcFRecoOCV ()
{
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFRecoOCV::Defaults (const char *fname)
{
  int ok = 1;

  ok &= lbp_params(fname);
  ok &= jhcFaceNorm::Defaults(fname);
  SetSizes();
  return ok;
}


//= Write current processing variable values to a file.

int jhcFRecoOCV::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= lps.SaveVals(fname);
  ok &= jhcFaceNorm::SaveVals(fname);
  return ok;
}


//= Parameters used for computing LBP face representation vector.
// SetSizes should be called any time basic parameters change

int jhcFRecoOCV::lbp_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("face_lbp", 0);
  ps->NextSpec4( &radius,  1, "LBP radius (pels)");  
  ps->NextSpec4( &pts,     8, "LBP sampling pts");  
  ps->NextSpec4( &uni,     0, "Drop uniform patterns");  
  ps->Skip();
  ps->NextSpec4( &xgrid,   5, "Face X grid divisions");  
  ps->NextSpec4( &ygrid,   9, "Face Y grid divisions");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Determine how many LBP patterns and number of histogram bins.
// this should be called any time basic parameters change

void jhcFRecoOCV::SetSizes ()
{
  nlbp = (int) pow(2.0, pts);
  hsz = xgrid * ygrid * nlbp;
  jhcFaceNorm::SetSizes();
}


///////////////////////////////////////////////////////////////////////////
//                          Signature Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

const char *jhcFRecoOCV::freco_version (char *spec) const
{
  sprintf(spec, "OpenCV 2.4.5 LBP face recognition %4.2f", ver);
  return spec;
}


//= Computes a signature vector given a cropped grayscale face image.
// image is 8 bit gray scanned left-to-right, bottom up
// returns negative for error, else vector size

int jhcFRecoOCV::freco_vect (float *hist, const unsigned char *img) const
{
  // load up input OpenCV matrix with source pixels (ignore flip)
  cv::Mat src(ih, iw, CV_8UC1, (void *) img);

  // get the spatial histogram from input image
  cv::Mat lbp_image;
  cv::elbp(src, lbp_image, radius, pts);
  cv::Mat query = cv::spatial_histogram(lbp_image, nlbp, xgrid, ygrid, true);                                        

  // extract values from resulting OpenCV histogram (32 bit floats)
  memcpy(hist, query.ptr(), hsz << 2);
  return hsz;
}


//= Computes a distance between two signature vectors (small is good).
// assumes signatures are both of correct length

double jhcFRecoOCV::freco_dist (const float *probe, const float *gallery) const
{
  // get histograms into OpenCV matrices
  cv::Mat a(1, hsz, CV_32FC1, (void *) probe);
  cv::Mat b(1, hsz, CV_32FC1, (void *) gallery);

  // do comparison 
  return cv::compareHist(b, a, CV_COMP_CHISQR_ALT);  // try cosine instead?
}


