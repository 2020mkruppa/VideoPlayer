#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <opencv2/core.hpp>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include "worker.h"

class Buffer {
public:
    Buffer(const std::string videoPath, int workerSegmentSize, int workerNumber, int numSegments);

    ~Buffer();

    int getTotalFrames() const;

    double getFPS() const;

    bool initializationFailed() const;

    int getStartBuff() const; //Steady state first frame num

    int getEndBuff() const; //Steady state end frame num

    int getBufferSize() const;

    cv::Mat* read(int index, bool updateLastRequested);

    void jump(int anchor);

    bool isBuffering() const;
private:
    void checkMoveBuffer();

    void addBehind();

    void addAhead();

    void jump();

    const int workerSegmentSize;
    const int numBufferSegments;

    bool stopped;
    int firstFrame;
    int lastRequestedFrame;

    std::mutex streamLock;
    std::thread thread;
    std::vector<Worker*> workers;
};

//numBufferSegments = 3, workers = 4, workerSegmentSize is size of single "1"

//        |       |        |
//|1|2|3|4|1|2|3|4|1|2|3|4|
//| | | | | | | | | | | | |
//| | | | | | | | | | | | |
//-------------------------
//        |       |       |
#endif // BUFFER_H_INCLUDED
