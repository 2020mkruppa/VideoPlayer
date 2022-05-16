#ifndef WORKER_H_INCLUDED
#define WORKER_H_INCLUDED

#include <unordered_map>
#include <deque>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include "semaphore.h"
#include "util.h"

class Worker {
public:
    Worker(const std::string videoPath, std::mutex* parentLock);

    Worker(const Worker &w);

    ~Worker();

    int getTotalFrames() const;

    double getFPS() const;

    int getBufferSize() const;

    cv::Mat* read(int index);

    bool initializationFailed() const;

    void addFrames(int addStartIndex, int numFrames, bool atFront);

    void removeFrames(bool atFront);

    bool isBuffering() const;
private:
    void checkQueue();

    void processJob(uint64_t job);

    cv::VideoCapture stream;

    std::unordered_map<int, cv::Mat*> frameDict;
    std::deque<uint64_t> addJobs;
    bool stopped;

    Semaphore sem;
    std::queue<uint64_t> jobQueue;
    std::mutex queueLock;
    std::mutex* streamLock;
    std::thread t;

    int totalFrames;
};

#endif // WORKER_H_INCLUDED
