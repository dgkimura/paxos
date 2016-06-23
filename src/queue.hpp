#ifndef __QUEUE_HPP_INCLUDED__
#define __QUEUE_HPP_INCLUDED__

#include <fstream>

#include <serialization.hpp>


template <typename T>
class VolatileQueue
{
private:

    class Iterator
    {
    public:

        Iterator(std::vector<T> data_, int index_)
            : data(data_),
              index(index_)
        {
        }

        T& operator*()
        {
            return data[index];
        }

        Iterator& operator++()
        {
            index += 1;
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return index != iterator.index;
        }

    private:

        int index;

        std::vector<T> data;
    };

    std::vector<T> data;

public:

    VolatileQueue()
        : data()
    {
    }

    void Enqueue(T e)
    {
        data.push_back(e);
    }

    void Dequeue()
    {
        if (!data.empty())
        {
            data.erase(data.begin());
        }
    }

    int Size()
    {
        return data.size();
    }

    Iterator begin()
    {
        return Iterator(data, 0);
    }

    Iterator end()
    {
        return Iterator(data, data.size());
    }
};


template <typename T>
class PersistentQueue
{
private:

    class Iterator
    {
    public:

        Iterator(std::fstream& file_, int offset_)
            : file(std::move(file_)),
              offset(offset_)
        {
        }

        T operator*()
        {
            std::string element_as_string;
            file >> element_as_string;
            T element = Deserialize<T>(element_as_string);
            return element;
        }

        Iterator& operator++()
        {
            int element_size;
            file >> element_size;
            offset += element_size;
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return offset != iterator.offset;
        }

    private:

        int offset;

        std::fstream file;
    };

    int lastoffset;

    std::fstream file;

public:

    PersistentQueue()
        : lastoffset(0)
    {
        file.open("persistent-queue-data", std::ios::app | std::ios::binary);
    }

    void Enqueue(T e)
    {
        file.seekg(0, std::ios::beg);
        std::string element_as_string = Serialize(e);
        file << element_as_string;
        lastoffset += element_as_string.size();
    }

    void Dequeue()
    {
        file.seekg(0, std::ios::beg);

        T first_element = Deserialize<T>(file);
        std::string first_element_serialized = Serialize(first_element);
        int offset = first_element_serialized.size();

        std::fstream tmpfile("persistent-queue.tmp");
        for (T element : this)
        {
            tmpfile << Serialize(element);
        }
        file << tmpfile.rdbuf();
    }

    int Size()
    {
        int size = 0;
        for (auto e : this)
        {
            size += 1;
        }
        return size;
    }

    Iterator begin()
    {
        return Iterator(file, 0);
    }

    Iterator end()
    {
        return Iterator(file, lastoffset);
    }
};


#endif
