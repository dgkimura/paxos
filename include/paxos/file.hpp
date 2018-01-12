#ifndef __FILE_HPP_INCLUDED__
#define __FILE_HPP_INCLUDED__

#include <string>


namespace paxos
{


struct BootstrapMetadata
{
    std::string local;

    std::string remote;
};


struct BootstrapFile
{
    std::string name;

    std::string content;

    BootstrapFile()
        : name(), content()
    {
    }

    BootstrapFile(std::string name, std::string content)
        : name(name), content(content)
    {
    }
};


}


#endif
