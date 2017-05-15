#ifndef __CUSTOMHASH_HPP_INCLUDED__
#define __CUSTOMHASH_HPP_INCLUDED__

#include "paxos/messages.hpp"


namespace std
{
    template<> struct hash<MessageType>
    {
        std::size_t operator()(MessageType const& m) const
        {
            return static_cast<std::size_t>(m);
        }
    };

    template<> struct hash<DecreeType>
    {
        std::size_t operator()(DecreeType const& d) const
        {
            return static_cast<std::size_t>(d);
        }
    };
}


#endif
