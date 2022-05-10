#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <unordered_map>
#include <string>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <chrono>
#include <thread>

class Buffer {
public:
    Buffer(const std::string videoPath, int bufferSize, float thresholdValue);

    ~Buffer();

    int getTotalFrames() const;

    double getFPS() const;

    int getStartBuff() const;

    int getEndBuff() const;

    cv::Mat* read(int index);

    bool initializationFailed() const;

    void jump(int anchor);

    int getJump() const;
private:
    void update();

    void addBehind();

    void addAhead();

    void checkJump();
    cv::VideoCapture stream;
    const int maxSize;
    const float threshold;
    std::unordered_map<int, cv::Mat*> frameDict;
    bool stopped;
    int baseFrame;
    int lastRequestedFrame;
    int totalFrames;
    int jumpFlagValue;
    std::thread thread;

};

#endif // BUFFER_H_INCLUDED
