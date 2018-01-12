#ifndef __FIELDS_HPP_INCLUDED__
#define __FIELDS_HPP_INCLUDED__

#include <fstream>

#include <boost/filesystem.hpp>

#include "serialization.hpp"


namespace paxos
{


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
        : file((boost::filesystem::path(dirname) /
                boost::filesystem::path(filename)).string(),
               std::ios::out |
               std::ios::in |
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
class PersistentStorageWrapper : public Storage<T>
{
public:

    PersistentStorageWrapper(std::string dirname, std::string filename)
    {
        std::string fullpath((boost::filesystem::path(dirname) /
                              boost::filesystem::path(filename)).string());
        if (!boost::filesystem::exists(fullpath))
        {
            std::fstream f(fullpath, std::ios::out | std::ios::trunc);
        }
        store = std::make_shared<PersistentStorage<T>>(dirname, filename);
    }

    T Get()
    {
        return store->Get();
    }

    void Put(T value)
    {
        store->Put(value);
    }

private:

    std::shared_ptr<PersistentStorage<T>> store;
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


using PersistentDecree = PersistentStorageWrapper<Decree>;

using VolatileDecree = VolatileStorage<Decree>;

using DecreeField = Field<Decree>;


}


#endif
