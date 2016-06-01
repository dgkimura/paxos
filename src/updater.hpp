#ifndef __UPDATER_HPP_INCLUDED__
#define __UPDATER_HPP_INCLUDED__


#include <context.hpp>
#include <receiver.hpp>
#include <sender.hpp>


struct UpdaterContext : public Context
{
};


void RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context);


void HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender);


#endif
