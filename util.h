#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdint.h>

uint64_t encodeAdd(uint64_t startIndex, uint64_t numFrames, bool atFront);

int getStartIndex(uint64_t coded);

int getNumFrames(uint64_t coded);

bool isAddJob(uint64_t coded);

uint64_t encodeRemove(bool atFront);

bool isAtFront(uint64_t coded);

uint64_t getDummyJob();

bool isDummyJob(uint64_t j);

int min(int a, int b);

int max(int a, int b);

#endif // UTIL_H_INCLUDED
