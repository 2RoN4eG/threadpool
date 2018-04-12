#include "Worker.h"

Worker::Worker () :
    _enabled (true),
    _functions (),
    _thread (&Worker::ThreadFunction, this)
{
}

Worker::Worker (unsigned int id) :
    _id (id)
{
}

Worker::~Worker()
{
    _enabled = false;
    _cv.notify_one ();
    _thread.join ();
}

void Worker::Append(Worker::TFunction funtion)
{
    std::unique_lock <std::mutex> locker (_mutex);

    _functions.push (funtion);

    _cv.notify_one ();
}

size_t Worker::TaskCount()
{
    std::unique_lock <std::mutex> locker (_mutex);

    return _functions.size ();
}

bool Worker::IsEmpty()
{
    std::unique_lock <std::mutex> locker (_mutex);

    return _functions.empty ();
}

void Worker::ThreadFunction()
{
    while (_enabled)
    {
        std::unique_lock <std::mutex> locker (_mutex);

        _cv.wait (locker, [&] (){ return !_functions.empty () || !_enabled; });

        while (!_functions.empty ())
        {
            TFunction function = _functions.front ();

            locker.unlock ();

            function ();

            locker.lock ();

            _functions.pop ();
        }
    }
}
