#ifndef WORKER_H
#define WORKER_H


#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>

class Worker
{
public:
    typedef std::function <void ()> TFunction;

public:
    Worker ();
    Worker (unsigned int id);

    ~Worker ();

    void Append (TFunction funtion);

    size_t TaskCount ();

    bool IsEmpty ();

private:

    bool                        _enabled;

    std::queue <TFunction>		_functions;

    std::condition_variable     _cv;
    std::mutex                  _mutex;
    std::thread                 _thread;

public:
    unsigned int                _id   = 0;

    void ThreadFunction ();
};

#endif // WORKER_H
