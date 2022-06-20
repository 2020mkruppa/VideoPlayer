#include "worker.h"
#include <cassert>
#include <iostream>
#include <thread>

Worker::Worker(const std::string videoPath, std::mutex* parentLock):
    stream(videoPath), frameDict(), addJobs(), stopped(false), sem(0), jobQueue(), queueLock(), streamLock(parentLock), t(checkQueue, this){
    totalFrames = (int)stream.get(cv::CAP_PROP_FRAME_COUNT);
    //thread = std::thread(update, std::ref(videoPath));
}

Worker::Worker(const Worker &w): sem(w.sem){
    totalFrames = 0;
    assert(false);
}

Worker::~Worker() {
    stopped = true;
    stream.release();
    queueLock.unlock();
    sem.post();
    for(std::unordered_map<int, cv::Mat*>::iterator iter = frameDict.begin(); iter != frameDict.end(); iter++) {
        delete iter->second;
        iter->second = nullptr;
    }
    t.join();
}

int Worker::getTotalFrames() const {
    return totalFrames;
}

double Worker::getFPS() const {
    return stream.get(cv::CAP_PROP_FPS);
}

int Worker::getBufferSize() const {
    return frameDict.size(); //Thread safe const method can be called concurrently
}

bool Worker::initializationFailed() const {
    return !stream.isOpened();
}

cv::Mat* Worker::read(int index) {
    cv::Mat* returnPointer = nullptr;
    if(frameDict.find(index) != frameDict.end()){
        returnPointer = frameDict[index];
    }
    return returnPointer;
}

bool Worker::isBuffering() const {
    return jobQueue.size() > 0;
}
//Does some index validation
void Worker::addFrames(int addStartIndex, int numFrames, bool atFront) {
    Guard g(queueLock);
    int validatedStart = min(max(0, addStartIndex), totalFrames);
    jobQueue.push(encodeAdd((uint64_t)validatedStart, (uint64_t)min(totalFrames - validatedStart, numFrames), atFront));
    sem.post();
    jobQueue.push(getDummyJob()); //Dummy job -> if this is processed, then all buffering is done
    sem.post();
}

void Worker::removeFrames(bool atFront) {
    Guard g(queueLock);
    jobQueue.push(encodeRemove(atFront));
    sem.post();
    jobQueue.push(getDummyJob()); //Dummy job -> if this is processed, then all buffering is done
    sem.post();
}

//Reads numFrames, starting at startIndex inclusive
void Worker::processJob(uint64_t job) {
    if(isDummyJob(job)){
        return;
    }
    if(isAddJob(job)){
        int startIndex = getStartIndex(job);
        int numFrames = getNumFrames(job);
        {
            //Guard guard(*streamLock);
            stream.set(cv::CAP_PROP_POS_FRAMES, startIndex);
            //std::cout << std::this_thread::get_id() << ":" << &stream << std::endl;
        }
        for(int pos = startIndex; pos < startIndex + numFrames; pos++) {
            cv::Mat* frame = new cv::Mat();
            if(stream.read(*frame))
                frameDict[pos] = frame;
            else
                delete frame;
        }

        if(isAtFront(job))
            addJobs.push_back(job);
        else
            addJobs.push_front(job);
    } else {
        uint64_t removeJob;
        if(isAtFront(job)){
            removeJob = addJobs.back();
            addJobs.pop_back();
        } else {
            removeJob = addJobs.front();
            addJobs.pop_front();
        }
        int startIndex = getStartIndex(removeJob);
        int numFrames = getNumFrames(removeJob);

        for(int i = startIndex; i < startIndex + numFrames; i++) {
            delete frameDict[i];
            frameDict.erase(i);
        }
    }
}

void Worker::checkQueue(){
    while(true) {
        if(stopped)
            return;
        sem.wait();
        uint64_t job;
        {
            Guard g(queueLock);
            job = jobQueue.front();
            jobQueue.pop();
        }
        processJob(job);
    }
}
