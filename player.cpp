#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <opencv2/imgproc.hpp>
#include "buffer.h"
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
using namespace cv;

using std::cout;
using std::endl;
using std::cin;

std::vector<Point> ptsSimple;
std::vector<Point> ptsHard;
int dragOldSimple = -1;
int dragOldHard = -1;

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

int main(int argc, char** argv){
    if ( argc < 2 ){
        printHelp();
        return 1;
    }

    cout << "Loading" << endl;

    Buffer buffer(argv[1], 300, 0.3); //threshold should be < 0.5, when buffer adds to front or back
    if(buffer.initializationFailed()) {
        cout << "Can't find video" << endl << endl;
        printHelp();
        return 2;
    }

    int width = 1920;
    int height = 1080;
    if(argc >= 4) {
        try{
            width = std::stoi(argv[2]);
            height = std::stoi(argv[3]);
        } catch (std::invalid_argument& e) {
            cout << "Bad input" << endl << endl;
            printHelp();
            return 3;
        }
        if(width < 1 || height < 1 || width > 10000 || height > 10000) {
            cout << "Bad input" << endl << endl;
            printHelp();
            return 4;
        }
    }

    int totalFrames = buffer.getTotalFrames();
    double timePerFrame = 1.0 / buffer.getFPS();
    int index = 0;
    int step = 1;
    int start = 0;

    namedWindow("Data");
    namedWindow("Frame");
    moveWindow("Data", 40, 40);
    moveWindow("Frame", 20, 20);
    setWindowProperty("Data", cv::WND_PROP_TOPMOST, 1);
    setMouseCallback("Frame", mouseHandler, 0);
    int run = 0;
    bool bufferingInAction = false;
    while(true){
        int v = waitKey(1) & 0xFF;
        if(v == 'q')
            break;
        if(v == 'f'){
            index = min(index + step, totalFrames - 2);
            ptsSimple.clear();
            ptsHard.clear();
        }
        if(v == 'p'){
            run = 1;
            ptsSimple.clear();
            ptsHard.clear();
        }
        index += run;
        if(v == 'b'){
            index = max(index - step, 0);
            ptsSimple.clear();
            ptsHard.clear();
        }
        if(v == 's'){
            if(start == 0)
                start = index;
            else
                start = 0;
        }
        if(v == '1')
            step = 1;
        if(v == '2')
            step = 2;
        if(v == '4')
            step = 4;
        if(v == '8')
            step = 8;
        if(buffer.getJump() == -1){
            bufferingInAction = false;
        }
        if(v == '.'){
            if(!bufferingInAction){
                index = min(index + ((int)buffer.getFPS() * 60), totalFrames - 2);
                buffer.jump(index);
                bufferingInAction = true;
                ptsSimple.clear();
                ptsHard.clear();
            }
            continue;
        }
        if(v == '>'){
            if(!bufferingInAction){
                index = min(index + ((int)buffer.getFPS() * 900), totalFrames - 2);
                buffer.jump(index);
                bufferingInAction = true;
                ptsSimple.clear();
                ptsHard.clear();
            }
            continue;
        }
        if(v == ','){
            if(!bufferingInAction){
                index = max(index - ((int)buffer.getFPS() * 60), 0);
                buffer.jump(index);
                bufferingInAction = true;
                ptsSimple.clear();
                ptsHard.clear();
            }
            continue;
        }
        if(v == '<'){
            if(!bufferingInAction){
                index = max(index - ((int)buffer.getFPS() * 900), 0);
                buffer.jump(index);
                bufferingInAction = true;
                ptsSimple.clear();
                ptsHard.clear();
            }
            continue;
        }


        Mat* frame = buffer.read(index);
        if(frame == nullptr) {
            continue;
        }
        if(frame->empty()) {
            continue;
        }

        Mat data(205, 260, CV_8UC1, Scalar(0));
        std::string elapsedTime = format(index * timePerFrame);
        std::string setTime = format((index - start) * timePerFrame);
        cv::putText(data, "Total: " + elapsedTime, Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "Set: " + setTime, Point(15, 70), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "Start: " + std::to_string(buffer.getStartBuff()), Point(15, 110), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "Frame: " + std::to_string(index), Point(15, 150), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);
        cv::putText(data, "End: " + std::to_string(buffer.getEndBuff()), Point(15, 190), cv::FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 1, LINE_AA);


        Mat resized;
        cv::resize(*frame, resized, Size(width, height));
        applySimplePoints(&resized);
        applyHardPoints(&resized);
        imshow("Frame", resized);

        imshow("Data", data);
    }
    destroyAllWindows();
    return 0;
}
