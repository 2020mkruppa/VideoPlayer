#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "buffer.h"
#include <thread>
#include <windows.h>
#include <winuser.h>

using namespace cv;

using std::cout;
using std::endl;
using std::cin;

std::vector<Point> ptsSimple;
std::vector<Point> ptsHard;
int dragOldSimple = -1;
int dragOldHard = -1;

int index = 0;
int step = 1;
int start = 0;
int waitKeyTime = 1;
int run = 0;
bool bufferingInAction = false;
Buffer* bufferPointer;
bool exitFlag = false;
int resolutionOption = 1;
int top = 0;
int left = 0;
Mat* dataPointer;

void millisecondsFormat(std::stringstream& ss, int t) {
    ss << (t < 100 ? ".0" : ".") << (t < 10 ? "0" : "") << t;
}

void twoDigits(std::stringstream& ss, int t) {
    ss << (t < 10 ? "0" : "") << t;
}

std::string format(double t){
    int time = t * 1000;
    std::stringstream ss;
    if (time < 0) {
        ss << "-";
        time *= -1;
    }

    int milliseconds = time % 1000;
    int total_seconds = time / 1000;
    if(total_seconds < 60){
        ss << total_seconds;
        millisecondsFormat(ss, milliseconds);
    }
    else if(total_seconds < 3600) {
        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;
        ss << minutes << ":";
        twoDigits(ss, seconds);
        millisecondsFormat(ss, milliseconds);
    }
    else{ //Leave hours to overflow -> 25:00:00.000
        int total_minutes = total_seconds / 60;
        int hours = total_minutes / 60;
        int minutes = total_minutes % 60;
        int seconds = total_seconds % 60;
        ss << hours << ":";
        twoDigits(ss, minutes);
        ss << ":";
        twoDigits(ss, seconds);
        millisecondsFormat(ss, milliseconds);
    }
    return ss.str();
}

void printHelp() {
    cout << "Supply video path as first argument" << endl;
    cout << "Supply optional arguments to resize to width/height" << endl;
    cout << "For example, VideoPlayer.exe video.mp4 1920 1080" << endl;
    cout << "Press q to quit, closing window will not work" << endl;
    cout << "Press f and b to move forward and backward respectively" << endl;
    cout << "Press s to set the time offset at current time, press again to reset it" << endl;
    cout << "Press 1, 2, 4, or 8 to set how many frames to move every f or b" << endl;
    cout << "Click and drag to draw points / lines / measure angle" << endl;
    cout << "You can drag a point after it has been placed to move it" << endl;
    cout << "Press '.' to jump 1 minute ahead, and ',' to jump a minute back" << endl;
    cout << "Press '>' to jump 15 minutes ahead, and '<' to jump 15 minutes back" << endl;
}

double length(Point p){
    return sqrt((p.x * p.x) + (p.y * p.y));
}

static void mouseHandler(int event, int x, int y, int, void*){
    if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP){
        dragOldSimple = -1;
        dragOldHard = -1;
    } else if(dragOldSimple > -1){
        ptsSimple[dragOldSimple].x = x;
        ptsSimple[dragOldSimple].y = y;
    } else if(dragOldHard > -1){
        ptsHard[dragOldHard].x = x;
        ptsHard[dragOldHard].y = y;
    } else if(event == EVENT_RBUTTONDOWN){
        for(int i = 0; i < (int)ptsSimple.size(); i++){
            if(length(Point(x - ptsSimple[i].x, y - ptsSimple[i].y)) < 12){
                dragOldSimple = i;
                return;
            }
        }
        ptsSimple.push_back(Point(x, y));
        dragOldSimple = (int)ptsSimple.size() - 1;
    } else if(event == EVENT_LBUTTONDOWN){
        for(int i = 0; i < (int)ptsHard.size(); i++){
            if(length(Point(x - ptsHard[i].x, y - ptsHard[i].y)) < 12){
                dragOldHard = i;
                return;
            }
        }
        ptsHard.push_back(Point(x, y));
        dragOldHard = (int)ptsHard.size() - 1;
    }
}

void getVectorsFrom3Points(Point* a, Point* b, Point* c, Point* out1, Point* out2){
    *out1 = Point((*a).x - (*b).x, (*a).y - (*b).y);
    *out2 = Point((*c).x - (*b).x, (*c).y - (*b).y);
}

double getPlaneAngle(Point* a, Point* b){
    double cosine = ((*a).x*(*b).x + (*a).y*(*b).y) / (length(*a) * length(*b));
    return acos(cosine) * 180.0 / 3.14159;
}

void drawText(double angle, Mat* img, Point* anchor, Point* a, Point* b){
    Point scaleA((*a).x * length(*b), (*a).y * length(*b));
    Point scaleB((*b).x * length(*a), (*b).y * length(*a));

    Point bisector((scaleA.x + scaleB.x) / 100.0, (scaleA.y + scaleB.y) / 100.0);
    Point text((-25 * bisector.x / length(bisector)) + (*anchor).x - 20, (-25 * bisector.y / length(bisector)) + (*anchor).y + 10);

    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << angle;
    cv::putText(*img, stream.str(), text, cv::FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0), 2, LINE_8);
    cv::putText(*img, stream.str(), text, cv::FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1, LINE_8);

}

void applySimplePoints(Mat* img){
    for(int x = 0; x < (int)ptsSimple.size(); x++){
        cv::circle(*img, ptsSimple[x], 4, Scalar(0, 0, 0), FILLED, LINE_8);
        cv::circle(*img, ptsSimple[x], 3, Scalar(0, 255, 0), FILLED, LINE_8);
    }

    for(int x = 0; x < (int)ptsSimple.size(); x+=3){
        if(x + 1 < (int)ptsSimple.size()){
            cv::line(*img, ptsSimple[x], ptsSimple[x + 1], Scalar(0, 255, 0), 1, LINE_8);
        }
        if(x + 2 < (int)ptsSimple.size()){
            cv::line(*img, ptsSimple[x + 1], ptsSimple[x + 2], Scalar(0, 255, 0), 1, LINE_8);
            Point a, b;
            getVectorsFrom3Points(&ptsSimple[x], &ptsSimple[x + 1], &ptsSimple[x + 2], &a, &b);
            drawText(getPlaneAngle(&a, &b), img, &ptsSimple[x + 1], &a, &b);
        }
    }
}

void applyHomography(Mat* h, Point* original, Point* converted){
    Mat raw = (Mat_<double>(3, 1) << (*original).x, (*original).y, 1);
    Mat convertedMat = (*h) * raw;
    convertedMat /= convertedMat.at<double>(2);
    *converted = Point((int)convertedMat.at<double>(0), (int)convertedMat.at<double>(1));
}

double getHardAngle(Point* fA, Point* fAnchor, Point* fB, Point* aA, Point* aAnchor, Point* aB){
    Point frameA, frameB;
    getVectorsFrom3Points(fA, fAnchor, fB, &frameA, &frameB);
    Point frame4(frameA.x + frameB.x + (*fAnchor).x, frameA.y + frameB.y + (*fAnchor).y);

    int centerX = (frame4.x + (*fAnchor).x) / 2;
    int centerY = (frame4.y + (*fAnchor).y) / 2;

    int len = (int)length(Point(frame4.x - (*fAnchor).x, frame4.y - (*fAnchor).y)) / 2;

    std::vector<Point2f> corners1{*fAnchor, *fA, *fB, frame4};
    std::vector<Point2f> corners2{Point2f(centerX - len, centerY - len), Point2f(centerX + len, centerY - len),
                    Point2f(centerX - len, centerY + len), Point2f(centerX + len, centerY + len)};

    Mat h = findHomography(corners1, corners2);

    Point convertedA, convertedAnchor, convertedB;
    applyHomography(&h, aA, &convertedA);
    applyHomography(&h, aAnchor, &convertedAnchor);
    applyHomography(&h, aB, &convertedB);

    Point a, b;
    getVectorsFrom3Points(&convertedA, &convertedAnchor, &convertedB, &a, &b);
    return getPlaneAngle(&a, &b);
}

void applyHardPoints(Mat* img){
    for(int x = 0; x < (int)ptsHard.size(); x++){
        cv::circle(*img, ptsHard[x], 4, Scalar(0, 0, 0), FILLED, LINE_8);
        cv::circle(*img, ptsHard[x], 3, Scalar(x % 6 < 3 ? 255 : 0, 0, x % 6 < 3 ? 0 : 255), FILLED, LINE_8);
    }

    for(int x = 0; x < (int)ptsHard.size(); x+=6){
        if(x + 1 < (int)ptsHard.size()){
            cv::line(*img, ptsHard[x], ptsHard[x + 1], Scalar(255, 0, 0), 1, LINE_8);
        }
        if(x + 2 < (int)ptsHard.size()){
            cv::line(*img, ptsHard[x + 1], ptsHard[x + 2], Scalar(255, 0, 0), 1, LINE_8);
        }
        if(x + 4 < (int)ptsHard.size()){
            cv::line(*img, ptsHard[x + 3], ptsHard[x + 4], Scalar(0, 0, 255), 1, LINE_8);
        }
        if(x + 5 < (int)ptsHard.size()){
            cv::line(*img, ptsHard[x + 4], ptsHard[x + 5], Scalar(0, 0, 255), 1, LINE_8);
            Point a, b;
            getVectorsFrom3Points(&ptsHard[x + 3], &ptsHard[x + 4], &ptsHard[x + 5], &a, &b);
            double angle = getHardAngle(&ptsHard[x], &ptsHard[x+1], &ptsHard[x+2], &ptsHard[x+3], &ptsHard[x+4], &ptsHard[x+5]);
            drawText(angle, img, &ptsHard[x + 4], &a, &b);
        }
    }
}

bool isInside(int x, int y, int topLeftX, int topLeftY){
    return x >= topLeftX && x <= (topLeftX + 40) && y >= topLeftY && y <= (topLeftY + 40);
}

void drawStepButtons(int selected){
    cv::rectangle(*dataPointer, Point(15, 250), Point(55, 290), Scalar(selected == 1 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(15, 250), Point(55, 290), Scalar(80), 5);
    cv::putText(*dataPointer, "1", Point(30, 275), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(65, 250), Point(105, 290), Scalar(selected == 2 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(65, 250), Point(105, 290), Scalar(80), 5);
    cv::putText(*dataPointer, "2", Point(80, 275), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(115, 250), Point(155, 290), Scalar(selected == 4 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(115, 250), Point(155, 290), Scalar(80), 5);
    cv::putText(*dataPointer, "4", Point(130, 275), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(165, 250), Point(205, 290), Scalar(selected == 8 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(165, 250), Point(205, 290), Scalar(80), 5);
    cv::putText(*dataPointer, "8", Point(180, 275), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);
}

void drawJumpButtons(){
    cv::rectangle(*dataPointer, Point(15, 300), Point(55, 340), Scalar(130), -1);
    cv::rectangle(*dataPointer, Point(15, 300), Point(55, 340), Scalar(80), 5);
    cv::putText(*dataPointer, "<<", Point(23, 325), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(65, 300), Point(105, 340), Scalar(130), -1);
    cv::rectangle(*dataPointer, Point(65, 300), Point(105, 340), Scalar(80), 5);
    cv::putText(*dataPointer, "<", Point(80, 325), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(115, 300), Point(155, 340), Scalar(130), -1);
    cv::rectangle(*dataPointer, Point(115, 300), Point(155, 340), Scalar(80), 5);
    cv::putText(*dataPointer, ">", Point(130, 325), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(165, 300), Point(205, 340), Scalar(130), -1);
    cv::rectangle(*dataPointer, Point(165, 300), Point(205, 340), Scalar(80), 5);
    cv::putText(*dataPointer, ">>", Point(173, 325), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);
}

void drawPlayButtons(int selected){
    cv::rectangle(*dataPointer, Point(15, 350), Point(55, 390), Scalar(selected == 1 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(15, 350), Point(55, 390), Scalar(80), 5);
    cv::putText(*dataPointer, "1", Point(30, 375), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(65, 350), Point(105, 390), Scalar(selected == 2 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(65, 350), Point(105, 390), Scalar(80), 5);
    cv::putText(*dataPointer, "1/2", Point(69, 375), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(115, 350), Point(155, 390), Scalar(selected == 3 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(115, 350), Point(155, 390), Scalar(80), 5);
    cv::putText(*dataPointer, "1/4", Point(119, 375), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(165, 350), Point(205, 390), Scalar(selected == 4 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(165, 350), Point(205, 390), Scalar(80), 5);
    cv::putText(*dataPointer, "1/8", Point(169, 375), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);
}

void drawControlButtons(int selected){
    cv::rectangle(*dataPointer, Point(215, 275), Point(255, 315), Scalar(selected == 1 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(215, 275), Point(255, 315), Scalar(80), 5);
    cv::putText(*dataPointer, "Set", Point(222, 300), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(215, 325), Point(255, 365), Scalar(130), -1);
    cv::rectangle(*dataPointer, Point(215, 325), Point(255, 365), Scalar(80), 5);
    cv::putText(*dataPointer, "Quit", Point(220, 350), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);
}

void drawResolutionButtons(int selected){
    cv::rectangle(*dataPointer, Point(15, 400), Point(55, 440), Scalar(selected == 1 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(15, 400), Point(55, 440), Scalar(80), 5);
    cv::putText(*dataPointer, "Orig", Point(20, 425), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(65, 400), Point(105, 440), Scalar(selected == 2 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(65, 400), Point(105, 440), Scalar(80), 5);
    cv::putText(*dataPointer, "W", Point(80, 425), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);

    cv::rectangle(*dataPointer, Point(115, 400), Point(155, 440), Scalar(selected == 3 ? 255 : 130), -1);
    cv::rectangle(*dataPointer, Point(115, 400), Point(155, 440), Scalar(80), 5);
    cv::putText(*dataPointer, "H", Point(130, 425), cv::FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0), 1, LINE_AA);
}

void jumpHelper(int newIndex){
    if(!bufferingInAction){
        index = newIndex;
        bufferPointer->jump(index);
        bufferingInAction = true;
        ptsSimple.clear();
        ptsHard.clear();
    }
}

void playHelper(int selectIndex, float fpsScale){
    run = 1;
    ptsSimple.clear();
    ptsHard.clear();
    waitKeyTime = (int)(fpsScale * bufferPointer->getFPS());
    drawStepButtons(0);
    drawPlayButtons(selectIndex);
}

void stepHelper(int select){
    step = select;
    run = 0;
    waitKeyTime = 1;
    drawStepButtons(select);
    drawPlayButtons(0);
}

void resolutionHelper(int selectIndex){
    resolutionOption = selectIndex;
    ptsSimple.clear();
    ptsHard.clear();
    drawResolutionButtons(selectIndex);
}

static void mouseHandlerData(int event, int x, int y, int, void*){
    if (event == EVENT_LBUTTONUP || event == EVENT_RBUTTONUP){
        if(isInside(x, y, 15, 250)) {
            stepHelper(1);
        } else if(isInside(x, y, 65, 250)) {
            stepHelper(2);
        } else if(isInside(x, y, 115, 250)) {
            stepHelper(4);
        } else if(isInside(x, y, 165, 250)) {
            stepHelper(8);
        } else if(isInside(x, y, 15, 300)) {
            jumpHelper(max(index - ((int)bufferPointer->getFPS() * 900), 0));
        } else if(isInside(x, y, 65, 300)) {
            jumpHelper(max(index - ((int)bufferPointer->getFPS() * 60), 0));
        } else if(isInside(x, y, 115, 300)) {
            jumpHelper(min(index + ((int)bufferPointer->getFPS() * 60), bufferPointer->getTotalFrames() - 2));
        } else if(isInside(x, y, 165, 300)) {
            jumpHelper(min(index + ((int)bufferPointer->getFPS() * 900), bufferPointer->getTotalFrames() - 2));
        } else if(isInside(x, y, 15, 350)) {
            playHelper(1, 0.85);
        } else if(isInside(x, y, 65, 350)) {
            playHelper(2, 1.7);
        } else if(isInside(x, y, 115, 350)) {
            playHelper(3, 3.4);
        } else if(isInside(x, y, 165, 350)) {
            playHelper(4, 6.8);
        } else if(isInside(x, y, 215, 275)) {
            if(start == 0){
                start = index;
                drawControlButtons(1);
            } else {
                start = 0;
                drawControlButtons(0);
            }
        } else if(isInside(x, y, 215, 325)) {
            exitFlag = true;
        } else if(isInside(x, y, 15, 400)) {
            resolutionHelper(1);
        } else if(isInside(x, y, 65, 400)) {
            resolutionHelper(2);
            moveWindow("Frame", left, top);
        } else if(isInside(x, y, 115, 400)) {
            resolutionHelper(3);
            moveWindow("Frame", left, top);
        }
    }
}

int main(int argc, char** argv){
    if ( argc < 2 ){
        printHelp();
        return 1;
    }

    Buffer buffer(argv[1], 80, 1, 4); //threshold should be < 0.5, when buffer adds to front or back
    bufferPointer = &buffer;
    if(buffer.initializationFailed()) {
        cout << "Can't find video" << endl << endl;
        printHelp();
        return 2;
    }

    int displayWidth = 1920;
    int displayHeight = 1080;

    SetConsoleTitle("Command Prompt - Video Player");
    Sleep(40);
    HWND window = FindWindow(NULL, "Command Prompt - Video Player");

    HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
    MONITORINFO target;
    target.cbSize = sizeof(MONITORINFO);
    if(monitor != NULL){
        GetMonitorInfo(monitor, &target);
        displayWidth = target.rcMonitor.right - target.rcMonitor.left;
        displayHeight = target.rcMonitor.bottom - target.rcMonitor.top;
    } else {
        cout << "Can't find cmd window" << endl;
    }
    top = target.rcMonitor.top;
    left = target.rcMonitor.left;

    int totalFrames = buffer.getTotalFrames();
    double timePerFrame = 1.0 / buffer.getFPS();


    namedWindow("Data");
    namedWindow("Frame");
    moveWindow("Data", 40, 40);
    moveWindow("Frame", 20, 20);
    setWindowProperty("Data", cv::WND_PROP_TOPMOST, 1);
    setMouseCallback("Frame", mouseHandler, 0);
    setMouseCallback("Data", mouseHandlerData, 0);

    Mat data(457, 270, CV_8UC1, Scalar(0));
    dataPointer = &data;
    std::string elapsedTime = format(index * timePerFrame);
    std::string setTime = format((index - start) * timePerFrame);
    cv::putText(data, "Total: ", Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
    cv::putText(data, "Set: ", Point(15, 70), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
    cv::putText(data, "Start: ", Point(15, 110), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
    cv::putText(data, "Frame: ", Point(15, 150), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
    cv::putText(data, "End: ", Point(15, 190), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
    cv::putText(data, "Size: ", Point(15, 230), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);

    drawStepButtons(1);
    drawJumpButtons();
    drawPlayButtons(0);
    drawControlButtons(0);
    drawResolutionButtons(resolutionOption);

    double aspect = 0;
    while(true){
        int v = waitKey(waitKeyTime) & 0xFF;
        if(v == 'q' || exitFlag)
            break;
        if(v == 'f'){
            stepHelper(step);
            index = min(index + step, totalFrames - 2);
            ptsSimple.clear();
            ptsHard.clear();
        }
        if(v == 'b'){
            stepHelper(step);
            index = max(index - step, 0);
            ptsSimple.clear();
            ptsHard.clear();
        }
        index += run;
        bufferingInAction = buffer.isBuffering();


        std::string elapsedTime = format(index * timePerFrame);
        std::string setTime = format((index - start) * timePerFrame);

        cv::rectangle(data, Point(105, 0), Point(270, 240), Scalar(0), -1);
        cv::putText(data, "" + elapsedTime, Point(105, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "" + setTime, Point(105, 70), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "" + std::to_string(buffer.getStartBuff()), Point(105, 110), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "" + std::to_string(index), Point(105, 150), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "" + std::to_string(buffer.getEndBuff()), Point(105, 190), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "" + std::to_string(buffer.getBufferSize()), Point(105, 230), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);

        imshow("Data", data);          //ALSO NEED GET SIZE

        cv::Mat* frame = buffer.read(index, true);
        if(frame == nullptr || frame->empty()) {
            continue;
        }

        if(aspect == 0){
            aspect = ((double)(*frame).cols) / (*frame).rows;
        }

        Mat resized;
        if(resolutionOption == 1){
            resized = frame->clone();
        } else if(resolutionOption == 2){
            cv::resize(*frame, resized, Size(displayWidth, (int)(displayWidth / aspect)));
        } else {
            cv::resize(*frame, resized, Size((int)(displayHeight * aspect), displayHeight));
        }

        applySimplePoints(&resized);
        applyHardPoints(&resized);
        imshow("Frame", resized);
    }
    destroyAllWindows();
    return 0;
}
