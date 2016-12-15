#ifndef __HANDLER_HPP_INCLUDED__
#define __HANDLER_HPP_INCLUDED__

#include <string>
#include <vector>

#include "paxos/decree.hpp"


//
// Handler that will be executed after a decree has been accepted. It
// is expected to be idempotent.
//
using Handler = std::function<void(std::string entry)>;


class DecreeHandler
{
public:

    DecreeHandler();

    DecreeHandler(Handler handler);

    void Add(Handler handler);

    void operator()(std::string entry);

private:

    std::vector<Handler> handlers;
};

#endif
