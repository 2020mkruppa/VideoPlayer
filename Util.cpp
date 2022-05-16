#include "util.h"

//Encodes according to
// 49(at front) 48(1) 47 - numFrames - 32   31 - startIndex - 0
uint64_t encodeAdd(uint64_t startIndex, uint64_t numFrames, bool atFront){
    return startIndex | (numFrames << 32) | (0x1ULL << 48) | (atFront ? 0x1ULL << 49 : 0);
}

int getStartIndex(uint64_t coded){
    return (int)(coded & 0xFFFFFFFFUL);
}

int getNumFrames(uint64_t coded){
    return (int)((coded >> 32) & 0xFFFFUL);
}

bool isAddJob(uint64_t coded){
    return ((coded >> 48) & 1) == 1;
}

//48th bit is 0, 49th toggles atFront
uint64_t encodeRemove(bool atFront){
    return 0x0ULL | (atFront ? 0x1ULL << 49 : 0);
}

bool isAtFront(uint64_t coded){
    return ((coded >> 49) & 1) == 1;
}

uint64_t getDummyJob(){
    return 0xFFFFFFFFFFFFFFFFULL;
}

bool isDummyJob(uint64_t j){
    return j == 0xFFFFFFFFFFFFFFFFULL;
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
