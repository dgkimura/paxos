#ifndef __FIELDS_HPP_INCLUDED__
#define __FIELDS_HPP_INCLUDED__

#include <fstream>

#include <serialization.hpp>


template <typename T>
class Field
{
public:

    virtual T& Value() = 0;
};


template <typename T>
class VolatileField : public Field<T>
{
public:

    VolatileField(std::string fieldname_)
    {
        fieldname = fieldname_;
    }

    VolatileField(const T& value)
    {
        data = value;
    }

    T& Value()
    {
        return data;
    }

private:

    T data;

    std::string fieldname;
};


template <typename T>
class PersistentField : public Field<T>
{
public:

    PersistentField(std::string fieldname)
        : file(fieldname, std::ios::out | std::ios::in | std::ios::binary)
    {
    }

    PersistentField(const T& value)
    {
        file.seekp(0, std::ios::end);
        std::string element_as_string = Serialize<T>(value);
        file << element_as_string;
        file.flush();

        data = value;
    }

    T& Value()
    {
        if (!data)
        {
            file.seekg(0, std::ios::beg);
            data = Deserialize<T>(file);
        }

        return data;
    }

private:

    T data;

    std::fstream file;
};

#endif
