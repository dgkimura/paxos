#ifndef __FIELDS_HPP_INCLUDED__
#define __FIELDS_HPP_INCLUDED__

#include <fstream>

#include <boost/filesystem.hpp>

#include "serialization.hpp"

namespace fs = boost::filesystem;


template <typename T>
class Storage
{
public:

    virtual T Get() = 0;

    virtual void Put(T data) = 0;
};


template <typename T>
class VolatileStorage : public Storage<T>
{
public:

    T Get()
    {
        return data;
    }

    void Put(T data_)
    {
        data = data_;
    }

private:

    T data;
};


template <typename T>
class PersistentStorage : public Storage<T>
{
public:

    PersistentStorage(std::string filename)
        : PersistentStorage(".", filename)
    {
    }

    PersistentStorage(std::string dirname, std::string filename)
        : file((fs::path(dirname) / fs::path(filename)).string(),
               std::ios::out |
               std::ios::in |
               std::ios::trunc |
               std::ios::binary
          ),
          stream(file)
    {
    }

    PersistentStorage(std::iostream& stream)
        : stream(stream)
    {
    }

    T Get()
    {
        T data;
        stream.seekg(0, std::ios::end);
        if (stream.tellg() > 0)
        {
            stream.seekg(0, std::ios::beg);
            data = Deserialize<T>(stream);
        }

        return data;
    }

    void Put(T value)
    {
        stream.seekp(0, std::ios::beg);
        std::string element_as_string = Serialize<T>(value);
        stream << element_as_string;
        stream.flush();
    }

private:

    std::fstream file;

    std::iostream& stream;
};


template <typename T>
class Field
{
public:

    Field(std::shared_ptr<Storage<T>> store_)
        : store(store_)
    {
    }

    Field& operator=(T&& value)
    {
        store->Put(value);
        return *this;
    }

    Field& operator=(T& value)
    {
        store->Put(value);
        return *this;
    }

    T Value()
    {
        return store->Get();
    }

private:

    std::shared_ptr<Storage<T>> store;
};


using PersistentDecree = PersistentStorage<Decree>;

using VolatileDecree = VolatileStorage<Decree>;

using DecreeField = Field<Decree>;


#endif
