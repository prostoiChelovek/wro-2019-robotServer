//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_VIDEOPROCESSING_HPP
#define VIDEOTRANS_VIDEOPROCESSING_HPP

#include <string>

#include <opencv2/opencv.hpp>

#include "utils.hpp"

#include "modules/faceDetector/Faces.h"

using namespace std;
using namespace cv;

class VideoProcessor {
public:
    Faces::Faces faces;

    string faceSamplesDir, faceRecognizerFIle;

    VideoProcessor(Faces::Faces faceRecognizer, string faceSamplesDir, string faceRecognizerFIle)
            : faces(faceRecognizer), faceSamplesDir(faceSamplesDir),
              faceRecognizerFIle(faceRecognizerFIle) {

    }

    void processImages(vector<Mat> imgs, map<string, string> &data, map<string, string> &todo) {
        Mat bgrImg;
        cvtColor(imgs[0], bgrImg, COLOR_GRAY2BGR);
        faces(bgrImg);

        if (!faces.detector.faces.empty())
            data["faces"] = "";
        for (auto &f : faces.detector.faces) {
            string label = "unknown";
            if (f.label != -1)
                label = faces.recognition->labels[f.label];
            data["faces"] += label + ",";
        }
        if (!data["faces"].empty())
            data["faces"].pop_back();

        faces.draw(imgs[0]);
        for (int i = 0; i < imgs.size(); i++) {
            imshow(to_string(i), imgs[i]);
        }

        if (faces.detector.faces.size() == 1) {
            int offset = faces.detector.faces[0].offset.x;
            todo["head"] = to_string(offset);
        }

        faces.update();
        char key = waitKey(1);
        processKey(key, imgs[0]);
    }

    void processKey(char key, Mat &frame) {
        if (key == 'l') {
            std::cout << "Label: ";
            std::string lbl;
            std::getline(std::cin, lbl);
            faces.recognition->addLabel(lbl);
            log(INFO, "Label", lbl, "added with index", faces.recognition->labels.size() - 1);
        }
        if (key == 'n') {
            if (faces.recognition->currentLabel < faces.recognition->labels.size() - 1)
                faces.recognition->currentLabel++;
            else
                faces.recognition->currentLabel = 0;
            log(INFO, "Current label is", faces.recognition->currentLabel, "-",
                faces.recognition->labels[faces.recognition->currentLabel]);
        }
        if (key == 'p') {
            if (faces.recognition->currentLabel > 0)
                faces.recognition->currentLabel--;
            else
                faces.recognition->currentLabel = faces.recognition->labels.size() - 1;
            log(INFO, "Current label is", faces.recognition->currentLabel, "-",
                faces.recognition->labels[faces.recognition->currentLabel]);
        }
        if (key == 's') {
            if (!faces.recognition->labels.empty()) {
                if (!faces.detector.faces.empty()) {
                    string path = faces.recognition->addSample(faceSamplesDir, faces.detector.faces[0]);
                    log(INFO, "Detector train image", faces.recognition->imgNum[faces.recognition->currentLabel],
                        "saved to", path);
                } else log(ERROR, "There is no faces");
            } else log(ERROR, "Labels are empty! Press 'l' to add new");
        }
        if (key == 't') {
            faces.recognition->train(faceSamplesDir);
            faces.recognition->save(faceRecognizerFIle);
            log(INFO, "Recognition model trained");
        }
    }

};


#endif //VIDEOTRANS_VIDEOPROCESSING_HPP
