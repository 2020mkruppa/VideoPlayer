#ifndef GUARD_H_INCLUDED
#define GUARD_H_INCLUDED

#include <mutex>

class Guard {
public:
  Guard(std::mutex &m) : lock(m) {
    lock.lock();
  }

  ~Guard() {
    lock.unlock();
  }

private:
  Guard(const Guard &);
  Guard &operator=(const Guard &);
  std::mutex &lock;
};

#endif // GUARD_H_INCLUDED
