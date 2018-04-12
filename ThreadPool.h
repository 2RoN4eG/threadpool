#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

template <class TResult>
struct Task
{
    bool        _ready;
    TResult     _result;

public:
    Task ():
        _ready (false)
    {
    }

    TResult Result ()
    {
        ///

        return _result;
    }
};

class ThreadPool
{
public:
    class Worker
    {
    public:
        typedef std::function <void ()> TFunction;

    public:
        Worker () :
            _enabled (true),
            _functions (),
            _thread (&Worker::ThreadFunction, this)
        {
        }

        ~Worker ()
        {
            _enabled = false;
            _cv.notify_one ();
            _thread.join ();
        }

        void Append (TFunction funtion)
        {
            std::unique_lock <std::mutex> locker (_mutex);

            _functions.push (funtion);

            _cv.notify_one ();
        }

        size_t TaskCount ()
        {
            std::unique_lock <std::mutex> locker (_mutex);

            return _functions.size ();
        }

        bool IsEmpty ()
        {
            std::unique_lock <std::mutex> locker (_mutex);

            return _functions.empty ();
        }

    private:

        bool                        _enabled;
        std::condition_variable     _cv;
        std::queue <TFunction>		_functions;
        std::mutex                  _mutex;
        std::thread                 _thread;

        void ThreadFunction ()
        {
            while (_enabled)
            {
                std::unique_lock <std::mutex> locker (_mutex);
                // Îæèäàåì óâåäîìëåíèÿ, è óáåäèìñÿ ÷òî ýòî íå ëîæíîå ïðîáóæäåíèå
                // Ïîòîê äîëæåí ïðîñíóòüñÿ åñëè î÷åðåäü íå ïóñòàÿ ëèáî îí âûêëþ÷åí

                _cv.wait (locker, [&] (){ return !_functions.empty () || !_enabled; });

                while (!_functions.empty ())
                {
                    TFunction function = _functions.front ();

                    // Ðàçáëîêèðóåì ìþòåêñ ïåðåä âûçîâîì ôóíêòîðà
                    locker.unlock ();

                    function ();

                    // Âîçâðàùàåì áëîêèðîâêó ñíîâà ïåðåä âûçîâîì fqueue.empty ()
                    locker.lock ();

                    _functions.pop ();
                }
            }
        }
    };

    ThreadPool (size_t threads = 8)
    {
        while (threads -- > 0)
        {
            WorkerPointer worker = std::make_shared <Worker> ();

            _workers.push_back (worker);
        }
    }

    ~ThreadPool () {}

    template <class TResult, class TFunction, class... TArguments>
    std::shared_ptr <Task <TResult>> RunAsync (TFunction function, TArguments... arguments)
    {
        WorkerPointer worker = FreeWorker ();

        std::shared_ptr <Task <TResult>> task (new Task <TResult> ());

        worker->Append ([=] ()
        {
            std::function <TResult ()> _function = std::bind (function, arguments...);

            task->_result   = _function ();
            task->_ready    = true;
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

private:
    typedef std::shared_ptr <Worker> WorkerPointer;

    std::vector <WorkerPointer> _workers;
};

#endif // THREADPOOL_H
