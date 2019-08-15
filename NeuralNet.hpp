//
// Created by prostoichelovek on 14.08.19.
//

#ifndef VIDEOTRANS_NEURALNET_HPP
#define VIDEOTRANS_NEURALNET_HPP

#include <string>
#include <utility>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include "modules/faceDetector/utils.hpp"

using namespace std;
using namespace cv;
using namespace dnn;

struct Prediction {
    Rect rect;
    float conf;
    int classId;

    Prediction(Rect rect, float conf, int classId)
            : rect(std::move(rect)), conf(conf), classId(classId) {}
};

class NeuralNet {
public:
    float confThreshold = 0.5;
    float nmsThreshold = 0.4;
    float scale = 1.0;
    Scalar Mean = Scalar(127.5, 127.5, 127.5);
    bool swapRB = true;
    Size inpSize = Size(300, 300);

    vector<string> labels;
    std::vector<String> outNames;

    vector<Prediction> predictions;

    Net net;

    bool ok = true;

    NeuralNet(const string &modelFile, const string &configFIle, const string &labelsFile = "") {
        net = readNet(modelFile, configFIle);
        if (net.empty()) {
            log(ERROR, "Cannot load neural network from", modelFile, "and", configFIle);
            ok = false;
        } else {
            outNames = net.getUnconnectedOutLayersNames();
        }

        if (!labelsFile.empty()) {
            if (!loadLabels(labelsFile))
                log(WARNING, "Cannot load labels from", labelsFile);
        }
    }

    bool loadLabels(const string &file) {
        ifstream ifs(file.c_str());
        if (!ifs.is_open()) {
            cerr << "Could not open labels file " << file << endl;
            return false;
        }

        string line;
        while (getline(ifs, line)) {
            labels.push_back(line);
        }

        return !labels.empty();
    }

    void predict(const Mat &img) {
        if (!ok)
            return;

        Mat blob;
        blobFromImage(img, blob, scale, inpSize, Mean, swapRB, false);

        net.setInput(blob);

        std::vector<Mat> outs;
        net.forward(outs, outNames);

        std::vector<int> classIds;
        std::vector<float> confidences;
        std::vector<Rect> boxes;

        if (outs.empty()) {
            log(WARNING, "Something went wrong when tried to run neural network and output is empty");
            return;
        }

        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        for (auto &out : outs) {
            auto *data = (float *) out.data;
            for (size_t i = 0; i < out.total(); i += 7) {
                float confidence = data[i + 2];
                if (confidence > confThreshold) {
                    auto left = (int) (data[i + 3] * img.cols);
                    auto top = (int) (data[i + 4] * img.rows);
                    auto right = (int) (data[i + 5] * img.cols);
                    auto bottom = (int) (data[i + 6] * img.rows);
                    int width = right - left + 1;
                    int height = bottom - top + 1;
                    classIds.push_back((int) (data[i + 1]) - 1);  // Skip 0th background class id.
                    boxes.emplace_back(left, top, width, height);
                    confidences.push_back(confidence);
                }
            }
        }

        std::vector<int> indices;
        NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);
        predictions.clear();
        for (int idx : indices)
            predictions.emplace_back(boxes[idx], confidences[idx], classIds[idx]);
    }

    void draw(Mat &img, Scalar color = Scalar(0, 255, 0)) {
        for (auto &pred : predictions) {
            rectangle(img, pred.rect, color);
            std::string label = format("%.2f", pred.conf);
            if (!labels.empty() && pred.classId >= 0 && pred.classId < labels.size()) {
                label = labels[pred.classId] + ": " + label;
            }

            int baseLine;
            Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

            rectangle(img, Point(pred.rect.x, pred.rect.y - labelSize.height),
                      Point(pred.rect.x + labelSize.width, pred.rect.y + baseLine), Scalar::all(255), FILLED);
            putText(img, label, pred.rect.tl(), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
        }
    }

};


#endif //VIDEOTRANS_NEURALNET_HPP
