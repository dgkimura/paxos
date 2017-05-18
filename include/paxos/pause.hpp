#ifndef __PAUSE_HPP_INCLUDED__
#define __PAUSE_HPP_INCLUDED__

#include <chrono>


class Pause
{
public:

    virtual void Start() = 0;
};


class NoPause : public Pause
{
public:

    virtual void Start() override;
};


class RandomPause : public Pause
{
public:

    RandomPause(std::chrono::milliseconds max);

    virtual void Start() override;

private:

    std::chrono::milliseconds max;
};


#endif
