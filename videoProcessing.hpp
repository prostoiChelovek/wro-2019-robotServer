//
// Created by prostoichelovek on 06.06.19.
//

#ifndef VIDEOTRANS_VIDEOPROCESSING_HPP
#define VIDEOTRANS_VIDEOPROCESSING_HPP

#include <string>

#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>

#include "utils.hpp"

#include "modules/faceDetector/Faces.h"

#include "Stereo.hpp"
#include "NeuralNet.hpp"

using namespace std;
using namespace cv;

class VideoProcessor {
public:
    Faces::Faces &faces;
    Stereo &stereo;
    NeuralNet &netL;

    string faceSamplesDir, faceRecognizerFIle;

    VideoProcessor(Faces::Faces &faceRecognizer, Stereo &stereo, NeuralNet &netL,
                   string faceSamplesDir, string faceRecognizerFIle)
            : faces(faceRecognizer), stereo(stereo), netL(netL),
              faceSamplesDir(faceSamplesDir),
              faceRecognizerFIle(faceRecognizerFIle) {

    }

    void processImages(vector<Mat> imgs, map<string, string> &data, map<string, string> &todo) {
        Mat bgrImg;
//        cvtColor(imgs[0], bgrImg, COLOR_GRAY2BGR);
        int imgNum = 0;

        Mat disp, dispL, dispR;
        std::future<void> future_disp;
        if (imgs.size() > 1) {
            imgNum = 1;
            future_disp = std::async(std::launch::async, [&] {
                disp = stereo.compute(imgs[1], imgs[0], dispL, dispR);
            });
        }
        std::future<void> future_faces = std::async(std::launch::async, [&] {
            faces(imgs[imgNum]);
        });

        std::future<void> future_detect = std::async(std::launch::async, [&] {
            netL.predict(imgs[imgNum]);
        });

        if (imgs.size() > 1) {
            future_disp.wait();
        }
        future_faces.wait();
        if (imgs.size() > 1) {
            for (Faces::Face &f : faces.detector.faces) {
                cv::Mat faceDisp = disp(f.rect);
                cv::resize(faceDisp, faceDisp, faces.faceSize);
                bool real = faces.checker.check(faceDisp);
                if (!real)
                    f.setLabel(-4);
            }
        }

        if (!faces.detector.faces.empty())
            data["faces"] = "";
        for (auto &f : faces.detector.faces) {
            string label;
            if (f.getLabel() > -1) {
                label = faces.recognition->labels[f.label];
            } else if (f.getLabel() == -1) {
                label = "unknown";
            } else if (f.getLabel() == -4) {
                label = "fake";
            } else {
                continue;
            }
            data["faces"] += label + ",";
        }
        if (!data["faces"].empty())
            data["faces"].pop_back();

        if (faces.detector.faces.size() == 1) {
            int offset = faces.detector.faces[0].offset.x;
            todo["head"] = to_string(offset);
        }

        future_detect.wait();

        faces.update();

        show(imgs, disp, dispL, dispR, imgNum);

        char key = waitKey(1);
        processKey(key, imgs[0]);
        if (imgs.size() >= 2) {
            if (!faces.detector.faces.empty()) {
                Faces::Face &f = faces.detector.faces[0];
                Mat faceDisp = disp(f.rect);
                if (key == 'f') {
                    faces.checker.addTrainSample(faceDisp, faceSamplesDir, false);
                    log(INFO, "Fake face sample saved to", faceSamplesDir);
                }
                if (key == 'r') {
                    faces.checker.addTrainSample(faceDisp, faceSamplesDir, true);
                    log(INFO, "Real face sample saved to", faceSamplesDir);
                }
            }
        }
    }

    void show(vector<Mat> &imgs, Mat &disp, Mat &dispL, Mat &dispR, int imgNum) {
        faces.draw(imgs[imgNum]);

        netL.draw(imgs[imgNum]);
        if (imgs.size() > 1) {
            Mat disps;
            vconcat(dispL, dispR, disps);
            resize(disps, disps, Size(disps.cols, disps.rows) / 2);
            Mat allDisps;
            hconcat(disp, disps, allDisps);
            imshow("Disparites", allDisps);

            // Draw face disparities ->
            int max_vert = imgs[0].rows / faces.detector.faceSize.height;
            int vert = 0, hor = 0;
            int cols = (faces.detector.faces.size() / max_vert) * faces.detector.faceSize.width;
            if (cols == 0 && !faces.detector.faces.empty())
                cols = faces.detector.faceSize.width;
            cv::Mat facesDisps(imgs[0].rows, cols, 16, cv::Scalar(255, 255, 255));
            for (Faces::Face &f : faces.detector.faces) {
                if (vert < max_vert) {
                    cv::Rect r = cv::Rect(
                            hor * faces.detector.faceSize.width,
                            vert * faces.detector.faceSize.height,
                            faces.detector.faceSize.width,
                            faces.detector.faceSize.height
                    );
                    Mat faceDisp = disp(f.rect);
                    cv::resize(faceDisp, faceDisp, faces.detector.faceSize);
                    Mat dispBGR;
                    cv::cvtColor(faceDisp, dispBGR, cv::COLOR_GRAY2BGR);
                    dispBGR.copyTo(facesDisps(r));

                    vert++;
                } else {
                    hor++;
                    vert = 0;
                }
            }
            cv::hconcat(vector<cv::Mat>{imgs[imgNum], facesDisps}, imgs[imgNum]);
            // <- Draw face disparities
        }

        for (int i = 0; i < imgs.size(); i++) {
            imshow(to_string(i), imgs[i]);
        }
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
            faces.checker.train(faceSamplesDir);
            faces.checker.save();
            log(INFO, "Models trained");
        }
    }

};


#endif //VIDEOTRANS_VIDEOPROCESSING_HPP
