//
// Created by prostoichelovek on 01.08.2019.
//

#ifndef VIDEOTRANS_FUCKSTEREO_HPP
#define VIDEOTRANS_FUCKSTEREO_HPP

#include <iostream>
#include <map>
#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>
#include <opencv2/calib3d.hpp>

#include "utils.hpp"

using namespace std;
using namespace cv;
using namespace ximgproc;

class Stereo {
public:
    // Stereo matcher params
    int win_size = 9;
    int min_disp = 0;
    int num_disp = 80;
    int uniquenessRatio = 0;
    int speckleRange = 2;
    int speckleWindowSize = 150;
    int disp12MaxDiff = 10;
    int preFilterCap = 3;

    // wls filter params
    double lmbda = 8000;
    double sigma = 1.5;

    Mat M1, D1, M2, D2;
    Mat R, T, R1, P1, R2, P2;
    Mat Q;
    Rect roi1, roi2;
    Mat map11, map12, map21, map22;

    Ptr<StereoSGBM> matcherL;
    Ptr<StereoMatcher> matcherR;
    Ptr<DisparityWLSFilter> wls_filter;

    bool ok = true;

    Stereo(const string &intrinsic, const string &extrinsic, const Size &img_size) {
        // Read parameters ->
        FileStorage fs(intrinsic, FileStorage::READ);
        if (!fs.isOpened()) {
            log(ERROR, "Cannot read intrinsic from", intrinsic);
            ok = false;
        }
        fs["M1"] >> M1;
        fs["D1"] >> D1;
        fs["M2"] >> M2;
        fs["D2"] >> D2;

        fs.open(extrinsic, FileStorage::READ);
        if (!fs.isOpened()) {
            log(ERROR, "Cannot read extrinsic from", extrinsic);
            ok = false;
        }
        fs["R"] >> R;
        fs["T"] >> T;
        // <- Read parameters

        // Rectification ->
        stereoRectify(M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q,
                      CALIB_ZERO_DISPARITY, 0, img_size, &roi1, &roi2);

        initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
        initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);
        // <- Rectification

        // Init matcher ->
        matcherL = StereoSGBM::create();
        matcherL->setBlockSize(win_size);
        matcherL->setMinDisparity(min_disp);
        matcherL->setNumDisparities(num_disp);
        matcherL->setUniquenessRatio(uniquenessRatio);
        matcherL->setSpeckleRange(speckleRange);
        matcherL->setSpeckleWindowSize(speckleWindowSize);
        matcherL->setDisp12MaxDiff(disp12MaxDiff);
        matcherL->setPreFilterCap(preFilterCap);
        matcherL->setP1(8 * 3 * win_size * win_size);
        matcherL->setP2(32 * 3 * win_size * win_size);
        // <- Init matcher

        // Init filtering ->
        matcherR = createRightMatcher(matcherL);

        wls_filter = createDisparityWLSFilter(matcherL);
        wls_filter->setLambda(lmbda);
        wls_filter->setSigmaColor(sigma);
        // <- Init filtering
    }

    void prepare(Mat &imgL, Mat &imgR) {
        remap(imgL, imgL, map11, map12, INTER_LINEAR);
        remap(imgR, imgR, map21, map22, INTER_LINEAR);
        /*for(int y = 0; y < imgL.rows; y += 50) {
            line(imgL, Point(0, y), Point(imgL.cols, y), Scalar(0, 255, 0));
            line(imgR, Point(0, y), Point(imgR.cols, y), Scalar(0, 255, 0));
        }*/
    }

    Mat compute(Mat &imgL, Mat &imgR,
                Mat &dispL8, Mat &dispR8) {
        prepare(imgL, imgR);

        Mat grayL, grayR;
        cvtColor(imgL, grayL, COLOR_BGR2GRAY);
        cvtColor(imgR, grayR, COLOR_BGR2GRAY);

        Mat dispL, dispR, dispFiltered, dispFiltered8;

        matcherL->compute(grayL, grayR, dispL);
        normalize(dispL, dispL8, 0, 255, NORM_MINMAX, CV_8U);

        matcherR->compute(grayR, grayL, dispR);
        normalize(dispR, dispR8, 0, 255, NORM_MINMAX, CV_8U);

        wls_filter->filter(dispL, grayL, dispFiltered, dispR, Rect(), grayR);
        normalize(dispFiltered, dispFiltered8, 0, 255, NORM_MINMAX, CV_8U);

        return dispFiltered8;
    }
};


#endif //VIDEOTRANS_FUCKSTEREO_HPP
