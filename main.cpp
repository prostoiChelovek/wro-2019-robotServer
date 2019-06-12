#include <iostream>
#include <map>

#include "opencv2/opencv.hpp"

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "utils.hpp"
#include "videoProcessing.hpp"
#include "dataProcessing.hpp"
#include "speechParser.hpp"
#include "thingworx.hpp"

using namespace std;
using namespace cv;
using namespace boost::asio;
using boost::asio::ip::tcp;

int port = 1234;
Size frameSize = Size(640, 480);

const string TW_addr = "0.0.0.0:8008";
const string TW_key = "dd9face6-4dd4-4286-9524-6ab81644d596";
const string TW_thingName = "butler";

const string caffeConfigFile = "../modules/faceDetector/deploy.prototxt";
const string caffeWeightFile = "../modules/faceDetector/res10_300x300_ssd_iter_140000_fp16.caffemodel";
string imgsDir = "../images";
string imgsList = "../faces.csv";
string labelsFile = "../labels.txt";
string modelFile = "../model.yml";

int main(int argc, char **argv) {
    if (argc > 1)
        port = stoi(argv[1]);

    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

    FaceRecognizer::FaceRecognizer faceRecognizer(frameSize);
    if (!faceRecognizer.readNet(caffeConfigFile, caffeWeightFile)) {
        log(ERROR, "Cannot read face recognition net", caffeConfigFile, caffeWeightFile);
        return EXIT_FAILURE;
    }
    if (!faceRecognizer.readRecognitionModel(modelFile)) {
        log(WARNING, "Cannot open recognition model", modelFile);
    }
    if (!faceRecognizer.readLabels(labelsFile)) {
        log(ERROR, "Cannot read labels", labelsFile);
        return EXIT_FAILURE;
    }
    if (!faceRecognizer.readImageList(imgsList)) {
        log(ERROR, "Cannot read images list", imgsList);
        return EXIT_FAILURE;
    }
    VideoProcessor vidProc(faceRecognizer, imgsDir, imgsList, modelFile);

    vector<pair<string, vector<string>>> keyWords;
    SpeechParser sp(keyWords);
    sp.addKeyWord("rain", {"идёт дождь"});
    sp.addKeyWord("set", {"установи"});
    sp.addKeyWord("temp", {"температура", "температуру"});
    sp.addKeyWord("hum", {"влажность"});
    sp.addKeyWord("on", {"включи"});
    sp.addKeyWord("off", {"выключи"});
    sp.addKeyWord("outside", {"на улице", "за окном", "снаружи"});
    sp.addKeyWord("inside", {"внутри", "в помещении", "в доме"});
    sp.addKeyWord("light", {"свет", "освещение"});
    sp.addKeyWord("normal", {"нармальную", "нармальная"});

    map<string, string> lastData;

    log(INFO, "Server started on port", port);
    log(INFO, "Waiting for connections...");
    while (true) {
        tcp::socket socket(io_service);
        acceptor.accept(socket);
        log(INFO, "New connection");
        while (socket.is_open()) {
            Mat img;
            vector<uchar> encData;
            try {
                boost::array<char, 8> sizeBuf;
                size_t recv = read(socket, buffer(sizeBuf), transfer_all());
                if (recv != 8)
                    continue;
                int dataSize = atoi((string(sizeBuf.begin(), sizeBuf.end())).c_str());

                encData = vector<uchar>(dataSize);
                read(socket, buffer(encData));
                if (encData.empty())
                    continue;
            } catch (exception &e) {
                log(ERROR, "Cannot receive image:", e.what());
                log(WARNING, "Closing connection");
                socket.close();
                break;
            }
            map<string, string> data;
            try {
                boost::asio::streambuf dataBuf;
                read_until(socket, dataBuf, "\n");
                string dataStr = buffer_cast<const char *>(dataBuf.data());
                dataStr.pop_back();
                data = deserializeData(dataStr);
            } catch (exception &e) {
                log(ERROR, "Cannot receive data:", e.what());
                log(WARNING, "Closing connection");
                socket.close();
                break;
            }

            img = imdecode(encData, IMREAD_GRAYSCALE);
            if (img.cols == 0 || img.rows == 0)
                continue;

            vector<Mat> imgs;
            for (int i = 0; i < img.cols / frameSize.width; i++) {
                Rect roi = Rect(frameSize.width * i, 0, frameSize.width, frameSize.height);
                imgs.emplace_back(img(roi));
            }

            map<string, string> todo;
            todo = vidProc.processImages(imgs, data);
            string toSay;
            map<string, string> command;

            if (!toSay.empty())
                todo["say"] = toSay;
            if (data.count("speech")) {
                sp.parseSpeech(data["speech"], command);
                toSay = sendCommand(TW_addr, TW_key, TW_thingName, command);
                todo["say"] = data["speech"];
            }

            if (data != lastData) {
                sendData(TW_addr, TW_key, TW_thingName, data);
                lastData = data;
                if (data.count("faces") != 0)
                    cout << data["faces"] << endl;
            }

            socket.send(buffer(string(serealizeData(todo) + "\n")));
        }
    }

    return EXIT_SUCCESS;
}
