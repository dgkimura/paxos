#ifndef __FILE_HPP_INCLUDED__
#define __FILE_HPP_INCLUDED__

#include <string>


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


#endif
