#include <iostream>
#include <map>

#include <opencv2/opencv.hpp>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include <mqtt/async_client.h>

#include "modules/faceDetector/Faces.h"

#include "utils.hpp"
#include "NeuralNet.hpp"
#include "videoProcessing.hpp"
#include "dataProcessing.hpp"

using namespace std;
using namespace cv;
using namespace boost::asio;
using boost::asio::ip::tcp;

// Parameters ->

int port = 1234;
Size frameSize = Size(640, 480);

const string TW_addr = "tcp://test.test:4242";
const string TW_username = "mojordomo";
const string TW_pwd = "12345678";
const string TW_topic = "butler";
const string TW_id = "0";
const vector<string> TW_subscribe{"say", "command"};
const int mqtt_qos = 1;
const string mqtt_persist_dir = "data-persist";

const string faceDetectorDir = "../modules/faceDetector";
const string dataDir = "../data";
const string facesConfigFile = faceDetectorDir + "/models/deploy.prototxt";
const string facesWeightFile = faceDetectorDir + "/models/res10_300x300_ssd_iter_140000_fp16.caffemodel";
const string lmsPredictorFile = faceDetectorDir + "/models/shape_predictor_5_face_landmarks.dat";
const string descriptorsNetFIle = faceDetectorDir + "/models/dlib_face_recognition_resnet_model_v1.dat";
const string faceClassifiersFile = dataDir + "/classifiers.dat";
const string samplesDir = dataDir + "/samples";
const string faceChecker = dataDir + "/faceChecker.dat";
const string labelsFile = dataDir + "/labels.txt";
const string dnnDir = dataDir + "/ssd_mobilenet_v2_coco_2018_03_29";
const string configFile = dnnDir + "/config.pbtxt";
const string weightsFile = dnnDir + "/frozen_inference_graph.pb";
const string netLabelsFile = dnnDir + "/object_detection_classes_coco.txt";

const string intrinsicFile = dataDir + "/intrinsics.yml";
const string extrinsicFile = dataDir + "/extrinsics.yml";

bool recordVideo = true;

// <- Parameters

SharedQueue<pair<string, string>> mqttMessages;

// MQTT stuff ->

class action_listener : public virtual mqtt::iaction_listener {
    std::string name_;

    void on_failure(const mqtt::token &tok) override {
        log(ERROR, name_, "failed with return code", tok.get_return_code());
    }

    void on_success(const mqtt::token &tok) override {}

public:
    explicit action_listener(std::string name) : name_(std::move(name)) {}
};

class callback : public virtual mqtt::callback,
                 public virtual mqtt::iaction_listener {
    // Counter for the number of connection retries
    int nretry_;
    // The MQTT client
    mqtt::async_client &cli_;
    // Options to use if we need to reconnect
    mqtt::connect_options &connOpts_;
    // An action listener to display the result of actions.
    action_listener subListener_;

    void reconnect() {
        this_thread::sleep_for(std::chrono::milliseconds(2500));
        try {
            cli_.connect(connOpts_, nullptr, *this);
        }
        catch (const mqtt::exception &e) {
            log(ERROR, "Cannot reconnect to thingworx MQTT broker:", e.what());
        }
    }

    void on_failure(const mqtt::token &tok) override {
        log(ERROR, "Cannot connect to thingworx MQTT broker on", TW_addr);
        reconnect();
    }

    void on_success(const mqtt::token &tok) override {}

    void connected(const std::string &cause) override {
        for (const string &topic : TW_subscribe) {
            cli_.subscribe(TW_topic + "/" + TW_id + "/" + topic, mqtt_qos, nullptr, subListener_);
        }
    }

    void connection_lost(const std::string &cause) override {
        log(ERROR, "Connection to thingworx MQTT broker lost:", cause);
        log(INFO, "Reconnecting...");
        reconnect();
    }

    // Callback for when a message arrives.
    void message_arrived(mqtt::const_message_ptr msg) override {
        string topic = eraseSubStr(msg->get_topic(), TW_topic + "/" + TW_id + "/");
        mqttMessages.push_back({topic, msg->to_string()});
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
    callback(mqtt::async_client &cli, mqtt::connect_options &connOpts)
            : nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}
};

void publishData(mqtt::async_client &cli, const string &topic, const string &data) {
    mqtt::topic top(cli, TW_topic + "/" + TW_id + "/" + topic, mqtt_qos, true);
    top.publish(data);
}

// <- MQTT stuff


int main(int argc, char **argv) {
    if (argc > 1)
        port = stoi(argv[1]);

    // TCP server for receive data from robot ->
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    // <- TCP server for receive data from robot

    // Init face recognition module ->
    Faces::Faces faceRecognizer(facesConfigFile, facesWeightFile, lmsPredictorFile, "",
                                descriptorsNetFIle, faceClassifiersFile, faceChecker,
                                labelsFile);
    faceRecognizer.detector.confidenceThreshold = .95;
    faceRecognizer.recognition->setThreshold(.3);
    faceRecognizer.detectFreq = 3;
    faceRecognizer.recognizeFreq = 9;

    faceRecognizer.checker.threshold = -0.1;
    // <- Init face recognition module

    Stereo stereo(intrinsicFile, extrinsicFile, frameSize);

    // Init neural networks ->
    // load two same networks for left and right frames
    // it used in point triangulation
    NeuralNet net(weightsFile, configFile, netLabelsFile);
    // only person
    net.onlyLabels = {0};
    net.skip = 5;
    // <- Init neural networks

    VideoProcessor vidProc(faceRecognizer, stereo, net, samplesDir, faceClassifiersFile);

    // Connect to tihngworx mqtt ->
    mqtt::async_client twMqtt(TW_addr, TW_topic + "-" + TW_id, mqtt_persist_dir);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    connOpts.set_automatic_reconnect(true);
    connOpts.set_user_name(TW_username);
    connOpts.set_password(TW_pwd);
    callback cb(twMqtt, connOpts);
    twMqtt.set_callback(cb);
    try {
        twMqtt.connect(connOpts)->wait();
        log(INFO, "Connected to thingworx MQTT broker on", TW_addr);
    } catch (const mqtt::exception &e) {
        log(ERROR, "Cannot connect to thingworx MQTT broker on", TW_addr, ":", e.what());
        return EXIT_FAILURE;
    }
    // <- Connect to tihngworx mqtt

    // Video recording ->
    //VideoWriter writerL("camL.avi", VideoWriter::fourcc('M','J','P','G'), 10, frameSize);
    //VideoWriter writerR("camR.avi", VideoWriter::fourcc('M','J','P','G'), 10, frameSize);
    // <- Video recording

    thread([]() {
//        system("cd ~/projects/videoTrans/client/cmake-build-debug && ./videoTrans 127.0.0.1 12345");
    }).detach();

    int fps = 0, nFrames = 0;
    thread([&]() {
        while (true) {
            this_thread::sleep_for(chrono::seconds(1));
            fps = nFrames;
            nFrames = 0;
        }
    }).detach();

    log(INFO, "Server started on port", port);
    log(INFO, "Waiting for connections...");
    while (true) {
        // Receive image and data from robot
        tcp::socket socket(io_service);
        acceptor.accept(socket);
        log(INFO, "New connection");
        while (socket.is_open()) {
            Mat img;
            vector<uchar> encData;
            try {
                boost::array<char, 8> sizeBuf{};
                size_t recv = read(socket, buffer(sizeBuf), transfer_all());
                if (recv != 8)
                    continue;
                int dataSize = stoi((string(sizeBuf.begin(), sizeBuf.end())));

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

            img = imdecode(encData, IMREAD_COLOR);
            if (img.cols == 0 || img.rows == 0)
                continue;

            vector<Mat> imgs;
            for (int i = 0; i < img.cols / frameSize.width; i++) {
                Rect roi = Rect(frameSize.width * i, 0, frameSize.width, frameSize.height);
                imgs.emplace_back(img(roi));
            }

            if (imgs.empty()) {
                log(ERROR, "Cannot receive images");
                log(WARNING, "Closing connection");
                socket.close();
                break;
            }

/*            if(recordVideo) {
                if(imgs.size() >= 2)
                    writerL.write(imgs[1]);
                writerR.write(imgs[0]);
            }
*/

            map<string, string> todo;

            auto start = std::chrono::high_resolution_clock::now();
            vidProc.processImages(imgs, data, todo);
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
//            log(INFO, "Time for process images:", duration.count(), "; FPS:", fps);

            if (data.count("speech")) {
                publishData(twMqtt, "listened", data["speech"]);
            }
            if (data.count("faces") > 0) {
                publishData(twMqtt, "faces", data["faces"]);
            } else {
                publishData(twMqtt, "faces", "");
            }

            if (vidProc.humans.no_humans_frames == 40) {
                todo["command"] = "patrol";
            }
            if (vidProc.humans.no_humans_frames == 80) {
                publishData(twMqtt, "enable_security", "1");
            }

            while (!mqttMessages.empty()) {
                auto msg = mqttMessages.front();
                log(INFO, msg.first, ":", msg.second);
                todo[msg.first] = msg.second;
                mqttMessages.pop_front();
            }

            socket.send(buffer(string(serealizeData(todo) + "\n")));

            nFrames++;
        }
    }
}
