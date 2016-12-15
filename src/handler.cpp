#include "paxos/handler.hpp"


DecreeHandler::DecreeHandler()
{
}


DecreeHandler::DecreeHandler(Handler handler)
{
    handlers.push_back(handler);
}


void
DecreeHandler::Add(Handler handler)
{
    handlers.push_back(handler);
}


void
DecreeHandler::operator()(std::string entry)
{
    for (auto h : handlers)
    {
        h(entry);
    }
}
