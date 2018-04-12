#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <condition_variable>

#include <Worker.h>

#define VERSION_1

template <class TResult>
struct Task
{
// #ifdef VERSION_1
    std::mutex                  _mutex;
    std::condition_variable     _cv;
// #endif // VERSION_1

    TResult                     _result;

    bool                        _ready      = false;

    unsigned int                _workerId   = 0;

public:
    Task () = default;

    TResult Result ()
    {
        std::unique_lock <std::mutex> locker (_mutex);

        std::cout << "Worker ID "
                  << _workerId
                  << " waiting result" << std::endl;

        if (_ready == false)
        {
            _cv.wait (locker, [&] (){ return _ready == false; });
        }

        return _result;
    }
};

class ThreadPool
{
    typedef std::shared_ptr <Worker> WorkerPointer;

    std::vector <WorkerPointer> _workers;

public:
    ThreadPool (size_t threads = 8)
    {
        while (threads -- > 0)
        {
            WorkerPointer worker = std::make_shared <Worker> ();

            _workers.push_back (worker);
        }
    }

    template <class TResult, class TFunction, class... TArguments>
    std::shared_ptr <Task <TResult>> RunAsync (TFunction function, TArguments... arguments)
    {
        WorkerPointer worker = FreeWorker ();

        std::shared_ptr <Task <TResult>> task (new Task <TResult> ());

        worker->Append ([=] ()
        {
#ifndef VERSION_1
            std::function <TResult ()> _function = std::bind (function, arguments...);


            task->_result = _function ();
            task->_ready  = true;
#endif // VERSION_1

#ifdef VERSION_1
            std::function <TResult ()> _function = std::bind (function, arguments...);

            std::cout << "Function ended" << std::endl;

            TResult result = _function ();

            {
                std::unique_lock <std::mutex> locker (task->_mutex);

                task->_result   = result;
                task->_ready    = true;
            }

            task->_cv.notify_one ();
#endif // VERSION_1
        });

        return task;
    }

    template <class TFunction, class... TArguments>
    void RunAsync (TFunction function, TArguments... arguments)
    {
        std::shared_ptr <Worker> worker = FreeWorker ();

        worker->Append (std::bind (function, arguments...));
    }

private:
    WorkerPointer FreeWorker ()
    {
        size_t          tasks = 0xFFFFFFFF;
        WorkerPointer   worker;

        for (WorkerPointer & _worker : _workers)
        {
            if (_worker->IsEmpty ())
            {
                return _worker;
            }
            else if (tasks > _worker->TaskCount ())
            {
                tasks    = _worker->TaskCount ();
                worker      = _worker;
            }
        }

        return worker;
    }
};

#endif // THREADPOOL_H
