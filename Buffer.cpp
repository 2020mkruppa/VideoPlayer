#include "buffer.h"
#include <iostream>

Buffer::Buffer(const std::string videoPath, int bufferSize, float thresholdValue):
    stream(videoPath), maxSize(bufferSize), threshold(thresholdValue),
    frameDict(), stopped(false), baseFrame(0), lastRequestedFrame(0), jumpFlagValue(-1), thread(){
    totalFrames = (int)stream.get(cv::CAP_PROP_FRAME_COUNT);
    addAhead();
    thread = std::thread(update, this);
}


Buffer::~Buffer() {
    stopped = true;
    thread.join();
    stream.release();
    for(std::unordered_map<int, cv::Mat*>::iterator iter = frameDict.begin(); iter != frameDict.end(); iter++) {
        delete iter->second;
        iter->second = nullptr;
    }
}

bool Buffer::initializationFailed() const {
    return !stream.isOpened();
}

int Buffer::getTotalFrames() const {
    return totalFrames;
}

double Buffer::getFPS() const {
    return stream.get(cv::CAP_PROP_FPS);
}

int Buffer::getStartBuff() const {
    return baseFrame;
}

int Buffer::getEndBuff() const {
    return baseFrame + frameDict.size() - 1;
}

cv::Mat* Buffer::read(int index) {
    lastRequestedFrame = index;
    if(frameDict.find(index) == frameDict.end())
        return nullptr;
    return frameDict[index];
}

int min(int a, int b) {
    if(a < b)
        return a;
    return b;
}

int max(int a, int b) {
    if(a < b)
        return b;
    return a;
}

void Buffer::addAhead() {
    if(baseFrame + (int)frameDict.size() >= totalFrames - 1)
        return;

    int newEndFrameNum = min(totalFrames - 1, lastRequestedFrame + (int)(maxSize / 2));
    stream.set(cv::CAP_PROP_POS_FRAMES, baseFrame + (int)frameDict.size());

    for(int pos = baseFrame + (int)frameDict.size(); pos < newEndFrameNum; pos++) {
        cv::Mat* frame = new cv::Mat();
        if(!stream.read(*frame))
            std::cout << "Bad Read" << std::endl;
        if(frame->empty())
            std::cout << frame->empty() << std::endl;
        frameDict[pos] = frame;
    }

    int bottomFrame = baseFrame;
    while((int)frameDict.size() > maxSize) {
        delete frameDict[bottomFrame];
        frameDict.erase(bottomFrame);
        bottomFrame++;
    }
    baseFrame = bottomFrame;
}

void Buffer::addBehind(){
    if(baseFrame == 0)
      return;
    int newStartFrameNum = max(0, lastRequestedFrame - (int)(maxSize / 2));
    stream.set(cv::CAP_PROP_POS_FRAMES, newStartFrameNum);

    for(int pos = newStartFrameNum; pos < baseFrame; pos++) {
        cv::Mat* frame = new cv::Mat();
        if(!stream.read(*frame))
            std::cout << "Bad Read" << std::endl;
        frameDict[pos] = frame;
    }

    baseFrame = newStartFrameNum;
    int topFrameNum = baseFrame + (int)frameDict.size() - 1;
    while((int)frameDict.size() > maxSize) {
        delete frameDict[topFrameNum];
        frameDict.erase(topFrameNum);
        topFrameNum--;
    }
}

void Buffer::jump(int anchor){
    jumpFlagValue = anchor;
}

int Buffer::getJump() const {
    return jumpFlagValue;
}

void Buffer::checkJump(){
    if(jumpFlagValue == -1){
        return;
    }
    while((int)frameDict.size() > 0) {
        delete frameDict[baseFrame];
        frameDict.erase(baseFrame);
        baseFrame++;
    }

    baseFrame = max(0, min(totalFrames - 1 - maxSize, jumpFlagValue - (maxSize / 2)));
    stream.set(cv::CAP_PROP_POS_FRAMES, baseFrame);
    for(int p = 0; p < maxSize; p++) {
        cv::Mat* frame = new cv::Mat();
        if(!stream.read(*frame))
            std::cout << "Bad Read" << std::endl;
        if(frame->empty())
            std::cout << frame->empty() << std::endl;
        frameDict[p + baseFrame] = frame;
    }
    lastRequestedFrame = jumpFlagValue;
    jumpFlagValue = -1;
}


void Buffer::update(){
    while(true) {
        if(stopped)
            return;
        checkJump();
        if(lastRequestedFrame - baseFrame < threshold * ((int)frameDict.size())) //Need more behind
            addBehind();
        if(lastRequestedFrame - baseFrame > (1 - threshold) * ((int)frameDict.size()))  //Need more ahead
            addAhead();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}


