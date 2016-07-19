#ifndef __FIELDS_HPP_INCLUDED__
#define __FIELDS_HPP_INCLUDED__

#include <fstream>

#include <serialization.hpp>


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

    PersistentStorage(std::string fieldname)
        : file(fieldname,
               std::ios::out |
               std::ios::in |
               std::ios::trunc |
               std::ios::binary
          )
    {
    }

    T Get()
    {
        T data;
        file.seekg(0, std::ios::end);
        if (file.tellg() > 0)
        {
            file.seekg(0, std::ios::beg);
            data = Deserialize<T>(file);
        }

        return data;
    }

    void Put(T value)
    {
        file.seekp(0, std::ios::beg);
        std::string element_as_string = Serialize<T>(value);
        file << element_as_string;
        file.flush();
    }

private:

    std::fstream file;
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
