#include "buffer.h"
#include <iostream>
#include "util.h"
#include <cassert>

Buffer::Buffer(const std::string videoPath, int workerSegmentSize, int workerNumber, int numSegments):
    workerSegmentSize(workerSegmentSize), numBufferSegments(numSegments), stopped(false),
    firstFrame(0), lastRequestedFrame(0), streamLock(), thread(){
    for(int i = 0; i < workerNumber; i++){
        Worker* w = new Worker(videoPath, &streamLock);
        workers.push_back(w);
    }

    //workers.push_back(new Worker("Mike 800m0.mp4", &streamLock));
    //workers.push_back(new Worker("Mike 800m1.mp4", &streamLock));
    //workers.push_back(new Worker("Mike 800m2.mp4", &streamLock));
    //workers.push_back(new Worker("Mike 800m3.mp4", &streamLock));

    for(int j = 0; j < numSegments; j++){
        for(int i = 0; i < workerNumber; i++)
            workers.at(i)->addFrames((workerSegmentSize * i) + (workerNumber * workerSegmentSize * j), workerSegmentSize, true);
    }
    thread = std::thread(checkMoveBuffer, this);
}

Buffer::~Buffer() {
    stopped = true;
    for(unsigned i = 0; i < workers.size(); i++){
        delete workers.at(i);
        workers.at(i) = nullptr;
    }
    thread.join();
}

int Buffer::getTotalFrames() const {
    return workers.at(0)->getTotalFrames();
}

double Buffer::getFPS() const {
    return workers.at(0)->getFPS();
}

bool Buffer::initializationFailed() const {
    for(unsigned i = 0; i < workers.size(); i++){
        if(workers.at(i)->initializationFailed())
            return true;
    }
    return false;
}

int Buffer::getStartBuff() const {
    return firstFrame;
}

int Buffer::getEndBuff() const {
    return getStartBuff() + workerSegmentSize * workers.size() * numBufferSegments;
}

int Buffer::getBufferSize() const {
    int s = 0;
    for(unsigned i = 0; i < workers.size(); i++){
        s += workers.at(i)->getBufferSize();
    }
    return s;
}

cv::Mat* Buffer::read(int index, bool updateLastRequested) {
    if(updateLastRequested)
        lastRequestedFrame = index;
    for(unsigned i = 0; i < workers.size(); i++){
        cv::Mat* p = workers.at(i)->read(index);
        if(p != nullptr)
            return p;
    }
    return nullptr;
}

bool Buffer::isBuffering() const {
    for(unsigned i = 0; i < workers.size(); i++){
        if(workers.at(i)->isBuffering())
            return true;
    }
    return false;
}

void Buffer::addAhead() {
    if(getEndBuff() >= getTotalFrames() - 1)
        return;

    for(unsigned i = 0; i < workers.size(); i++){
        int newStart = getEndBuff() + (workerSegmentSize * i);
        if(newStart >= getTotalFrames())
            continue; //Entirely out of range
        if(newStart < getTotalFrames() - workerSegmentSize)
            workers.at(i)->addFrames(newStart, workerSegmentSize, true);
        else
            workers.at(i)->addFrames(newStart, getTotalFrames() - newStart, true);
        workers.at(i)->removeFrames(false);
    }
    firstFrame += workerSegmentSize * workers.size();
}

void Buffer::addBehind(){
    if(firstFrame <= 0)
      return;
    for(unsigned i = 0; i < workers.size(); i++){
        int newStart = getStartBuff() + (workerSegmentSize * i) - (workers.size() * workerSegmentSize);
        if(newStart < 0)
            continue; //Entirely out of range
        workers.at(i)->addFrames(newStart, workerSegmentSize, false);
        workers.at(i)->removeFrames(true);
    }
    firstFrame -= workerSegmentSize * workers.size();
}

void Buffer::jump(int anchor){
    lastRequestedFrame = anchor;
}

void Buffer::jump(){
    firstFrame = lastRequestedFrame - ((getEndBuff() - getStartBuff()) / 2);
    for(int j = 0; j < numBufferSegments; j++){
        for(unsigned i = 0; i < workers.size(); i++){
            workers.at(i)->removeFrames(true); //Clear all buffers
        }
    }
    for(int j = 0; j < numBufferSegments; j++){ //Need separate loop to clear all buffers before adding new
        for(unsigned i = 0; i < workers.size(); i++){
            workers.at(i)->addFrames(firstFrame + (workerSegmentSize * i) + (workers.size() * workerSegmentSize * j),
                                    workerSegmentSize, true);
        }
    }
}

void Buffer::checkMoveBuffer(){
    while(true) {
        if(stopped)
            return;
        int totalCapacity = getEndBuff() - getStartBuff();
        if(lastRequestedFrame - getEndBuff() > totalCapacity || getStartBuff() - lastRequestedFrame > totalCapacity)
            jump();
        //Move if lastRequested is on the last segment
        if((getEndBuff() - lastRequestedFrame) * numBufferSegments > (numBufferSegments - 1) * totalCapacity) //Need more behind
            addBehind();
        if((lastRequestedFrame - getStartBuff()) * numBufferSegments > (numBufferSegments - 1) * totalCapacity)
             addAhead();
        //Need more ahead
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

