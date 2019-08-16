//
// Created by prostoichelovek on 15.08.19.
//

#ifndef VIDEOTRANS_HUMAN_HPP
#define VIDEOTRANS_HUMAN_HPP

#include <opencv2/opencv.hpp>

#include "modules/faceDetector/Face.h"
#include "NeuralNet.hpp"

using namespace std;
using namespace cv;

struct Human {
    Rect rect;
    Faces::Face *face = nullptr;

    Human() = default;

    Human(Rect rect, Faces::Face *face = nullptr) : rect(rect), face(face) {}
};

class Humans {

};


#endif //VIDEOTRANS_HUMAN_HPP
