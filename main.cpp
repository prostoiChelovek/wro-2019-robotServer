#include <iostream>
#include <map>

#include "opencv2/opencv.hpp"

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <utility>

#include <mqtt/async_client.h>

#include "modules/faceDetector/Faces.h"

#include "utils.hpp"
#include "videoProcessing.hpp"
#include "dataProcessing.hpp"

using namespace std;
using namespace cv;
using namespace boost::asio;
using boost::asio::ip::tcp;

int port = 1234;
Size frameSize = Size(640, 480);

const string TW_addr = "tcp://test.test:4242";
const string TW_username = "mojordomo";
const string TW_pwd = "12345678";
const string TW_topic = "butler";
const string TW_id = "0";
vector<string> TW_subscribe{"say"};
const int mqtt_qos = 1;
const string mqtt_persist_dir = "data-persist";

string faceDetectorDir = "../modules/faceDetector";
const string configFile = faceDetectorDir + "/models/deploy.prototxt";
const string weightFile = faceDetectorDir + "/models/res10_300x300_ssd_iter_140000_fp16.caffemodel";
string lmsPredictorFile = faceDetectorDir + "/models/shape_predictor_5_face_landmarks.dat";
string descriptorsNetFIle = faceDetectorDir + "/models/dlib_face_recognition_resnet_model_v1.dat";
string faceClassifiersFile = "../classifiers.dat";
string samplesDir = "../samples";
string labelsFile = "../labels.txt";

SharedQueue<pair<string, string>> mqttMessages;

class action_listener : public virtual mqtt::iaction_listener {
    std::string name_;

    void on_failure(const mqtt::token &tok) override {
        log(ERROR, name_, "failed with return code", tok.get_return_code());
    }

    void on_success(const mqtt::token &tok) override {}

public:
    explicit action_listener(std::string name) : name_(std::move(name)) {}
};

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
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
        for (string &topic : TW_subscribe) {
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

int main(int argc, char **argv) {
    if (argc > 1)
        port = stoi(argv[1]);

    // TCP server for receive data from robot ->
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    // <- TCP server for receive data from robot

    // Init face recognition module ->
    Faces::Faces faceRecognizer(configFile, weightFile, lmsPredictorFile, "",
                                descriptorsNetFIle, faceClassifiersFile,
                                labelsFile);
    faceRecognizer.detector.confidenceThreshold = .95;
    faceRecognizer.recognition->setThreshold(70);
    // <- Init face recognition module

    VideoProcessor vidProc(faceRecognizer, samplesDir, faceClassifiersFile);

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

            map<string, string> todo;

            vidProc.processImages(imgs, data, todo);
            if (data.count("speech")) {
                publishData(twMqtt, "listened", data["speech"]);
            }
            if (data.count("faces") > 0) {
                if (data["faces"].empty())
                    data.erase("faces");
                else
                    publishData(twMqtt, "faces", data["faces"]);
            }

            while (!mqttMessages.empty()) {
                auto msg = mqttMessages.front();
                todo[msg.first] = msg.second;
                mqttMessages.pop_front();
            }

            socket.send(buffer(string(serealizeData(todo) + "\n")));
        }
    }
}
