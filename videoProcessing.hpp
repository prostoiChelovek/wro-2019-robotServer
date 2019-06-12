//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_VIDEOPROCESSING_HPP
#define VIDEOTRANS_VIDEOPROCESSING_HPP

#include <string>

#include "opencv2/opencv.hpp"

#include "utils.hpp"

#include "modules/faceDetector/FaceRecognizer.h"

using namespace std;
using namespace cv;

class VideoProcessor {
public:
    const string &imgsDir, &imgsList, &modelFile;

    FaceRecognizer::FaceRecognizer &faceRecognizer;

    VideoProcessor(FaceRecognizer::FaceRecognizer &faceRecognizer,
                   const string &imgsDir, const string &imgsList, const string &modelFile)
            : faceRecognizer(faceRecognizer),
              imgsDir(imgsDir), imgsList(imgsList), modelFile(modelFile) {
        faceRecognizer.confidenceThreshold = .95;
        faceRecognizer.model->setThreshold(70);
    }

    map<string, string> processImages(vector<Mat> imgs, map<string, string> &data) {
        Mat bgrImg;
        cvtColor(imgs[0], bgrImg, COLOR_GRAY2BGR);
        faceRecognizer.detectFaces(bgrImg);

        if (!faceRecognizer.faces.empty())
            data["faces"] = "";
        for (auto &f : faceRecognizer.faces) {
            string label = "unknown";
            if (f.label != -1)
                label = faceRecognizer.labels[f.label];
            data["faces"] += label + ",";
        }
        if (!data["faces"].empty())
            data["faces"].pop_back();

        faceRecognizer.draw(imgs[0]);
        for (int i = 0; i < imgs.size(); i++) {
            imshow(to_string(i), imgs[i]);
        }

        map<string, string> todo;

        if (faceRecognizer.faces.size() == 1) {
            int offset = faceRecognizer.faces[0].offset.x;
            todo["head"] = to_string(offset);
        }

        faceRecognizer.lastFaces = faceRecognizer.faces;
        char key = waitKey(1);
        processKey(key, imgs[0]);
        return todo;
    }

    void processKey(char key, Mat &frame) {
        if (key == 'l') {
            string label = faceRecognizer.addLabel();
            log(INFO, "Label", label, "added with index", faceRecognizer.labels.size() - 1);
        }
        if (key == 'n') {
            if (faceRecognizer.currentLabel < faceRecognizer.labels.size() - 1)
                faceRecognizer.currentLabel++;
            else
                faceRecognizer.currentLabel = 0;
            log(INFO, "Current label is", faceRecognizer.currentLabel,
                "-", faceRecognizer.labels[faceRecognizer.currentLabel]);
        }
        if (key == 'p') {
            if (faceRecognizer.currentLabel > 0)
                faceRecognizer.currentLabel--;
            else
                faceRecognizer.currentLabel = faceRecognizer.labels.size() - 1;
            log(INFO, "Current label is", faceRecognizer.currentLabel,
                "-", faceRecognizer.labels[faceRecognizer.currentLabel]);
        }
        if (key == 's') {
            if (!faceRecognizer.labels.empty()) {
                if (!faceRecognizer.faces.empty()) {
                    Mat f = frame(faceRecognizer.faces[0].rect);
                    string path = faceRecognizer.addTrainImage(imgsDir, f);
                    log(INFO, "Recognizer train image", faceRecognizer.imgNum[faceRecognizer.currentLabel],
                        "saved to", path);
                } else log(ERROR, "There is no faces");
            } else log(ERROR, "Labels are empty! Press 'l' to add new");
        }
        if (key == 't') {
            bool ok = faceRecognizer.trainRecognizer(imgsList, modelFile);
            if (ok)
                log(INFO, "Face recognition model trained successful");
            else
                log(ERROR, "Cannot train face recognition model");
        }
    }

};


#endif //VIDEOTRANS_VIDEOPROCESSING_HPP
