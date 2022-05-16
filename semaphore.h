#ifndef SEMAPHORE_H_INCLUDED
#define SEMAPHORE_H_INCLUDED

#include <mutex>
#include <condition_variable>
#include "guard.h"

class Semaphore {
public:
    Semaphore(unsigned long value);

    Semaphore(const Semaphore &s);

    ~Semaphore();

    void post();

    void wait();

private:
    std::mutex lock;
    std::condition_variable condition;
    unsigned long count = 0; // Initialized as locked.
};

#endif // SEMAHPHORE_H_INCLUDED
