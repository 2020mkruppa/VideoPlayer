#include "semaphore.h"
#include <cassert>

Semaphore::Semaphore(unsigned long value): lock(), condition(), count(value){}

Semaphore::Semaphore(const Semaphore &s){
    count = s.count;
    assert(false);
}

Semaphore::~Semaphore(){
    lock.unlock();
    condition.notify_all();
}

void Semaphore::post() {
    Guard g(lock);
    ++count;
    condition.notify_one();
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> l(lock);
    while(!count) // Handle spurious wake-ups.
        condition.wait(l);
    --count;
}
