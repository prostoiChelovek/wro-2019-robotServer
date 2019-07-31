#include <iostream>
#include <map>

#include "opencv2/opencv.hpp"

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "modules/faceDetector/Faces.h"

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

const string TW_addr = "http://niks.duckdns.org:8080";
const string TW_key = "dd9face6-4dd4-4286-9524-6ab81644d596";
const string TW_thingName = "butler";

string faceDetectorDir = "../modules/faceDetector";
const string configFile = faceDetectorDir + "/models/deploy.prototxt";
const string weightFile = faceDetectorDir + "/models/res10_300x300_ssd_iter_140000_fp16.caffemodel";
string lmsPredictorFile = faceDetectorDir + "/models/shape_predictor_5_face_landmarks.dat";
string descriptorsNetFIle = faceDetectorDir + "/models/dlib_face_recognition_resnet_model_v1.dat";
string modelFile = "../model.yml";
string faceClassifiersFile = "../classifiers.dat";
string samplesDir = "../samples";
string labelsFile = "../labels.txt";

int main(int argc, char **argv) {
    if (argc > 1)
        port = stoi(argv[1]);

    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));

    Faces::Faces faceRecognizer(configFile, weightFile, lmsPredictorFile, "",
                                descriptorsNetFIle, faceClassifiersFile,
                                labelsFile);
    faceRecognizer.detector.confidenceThreshold = .95;
    faceRecognizer.recognition->setThreshold(70);

    VideoProcessor vidProc(faceRecognizer, samplesDir, faceClassifiersFile);

    vector<pair<string, vector<string>>> keyWords;
    SpeechParser sp(keyWords);
    sp.addKeyWord("rain", {"идёт дождь"});
    sp.addKeyWord("set", {"установи"});
    sp.addKeyWord("temp", {"температура", "температуру"});
    sp.addKeyWord("hum", {"влажность"});
    sp.addKeyWord("wtrFreq", {"частоту полива"});
    sp.addKeyWord("open", {"открой"});
    sp.addKeyWord("close", {"закрой"});
    sp.addKeyWord("grower", {"теплицу"});
    sp.addKeyWord("on", {"включи"});
    sp.addKeyWord("off", {"выключи"});
    sp.addKeyWord("tank", {"танк"});
    sp.addKeyWord("goodbye", {"увидимся позже", "до завтра"});
    sp.addKeyWord("outside", {"на улице", "за окном", "снаружи"});
    sp.addKeyWord("inside", {"внутри", "в помещении", "в доме"});
    sp.addKeyWord("light", {"свет", "освещение"});
    sp.addKeyWord("normal", {"нармальную", "нармальная"});
    sp.addKeyWord("fwd", {"вперёд"});
    sp.addKeyWord("back", {"назад"});
    sp.addKeyWord("stop", {"стоп"});
    sp.addKeyWord("left", {"влево"});
    sp.addKeyWord("right", {"вправо"});

    map<string, string> lastData;

    Thingworx tw(TW_addr, TW_thingName, TW_key);

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
            string todoStr = tw.execService("getData", {{}});
            for (string &s : split(todoStr, ";")) {
                vector<string> cmd;
                for (string &c : split(s, ":")) {
                    cmd.emplace_back(c);
                }
                if (cmd.size() == 2) {
                    todo[cmd[0]] = cmd[1];
                }
            }

            vidProc.processImages(imgs, data, todo);
            string toSay;
            map<string, string> command;

            if (!toSay.empty())
                todo["say"] = toSay;
            if (data.count("speech")) {
                sp.parseSpeech(data["speech"], command);
                map<string, string> cmd2send;
                for (const auto &c : command) {
                    cmd2send["comName"] = c.first;
                    if (!c.second.empty())
                        cmd2send["rawValue"] = c.second;
                }
                toSay = tw.execService("post_command", cmd2send);
//                toSay = tw.execService("process_speech", {{"speech", data["speech"]}});
                todo["say"] = toSay;
            }

            if (data.count("faces") > 0) {
                if (data["faces"].empty())
                    data.erase("faces");
            }

            if (data != lastData) {
                tw.execService("post_data", data);
                lastData = data;
            }

            socket.send(buffer(string(serealizeData(todo) + "\n")));
        }
    }

    return EXIT_SUCCESS;
}
