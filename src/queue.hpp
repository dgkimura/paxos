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

    VolatileQueue(std::string name)
        : data()
    {
    }

    VolatileQueue()
        : VolatileQueue("default-volatile-queue-name")
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

        Iterator(Iterator&& other)
            : file(other.file),
              offset(other.offset)
        {
        }

        Iterator(std::fstream& file_, int offset_)
            : file(file_),
              offset(offset_)
        {
        }

        T operator*()
        {
            file.seekg(offset, std::ios::beg);
            return Deserialize<T>(file);
        }

        Iterator& operator++()
        {
            file.seekg(offset, std::ios::beg);
            offset += Serialize(Deserialize<T>(file)).length();
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return offset != iterator.offset;
        }

    private:

        int offset;

        std::fstream& file;
    };

    std::fstream file;

public:

    PersistentQueue(std::string name)
        : file(name, std::ios::out | std::ios::in | std::ios::app | std::ios::binary)
    {
    }

    PersistentQueue()
        : PersistentQueue("default-persistent-queue-name")
    {
    }

    void Enqueue(T e)
    {
        file.seekp(0, std::ios::end);
        std::string element_as_string = Serialize(e);
        file << element_as_string;
        file.flush();
    }

    void Dequeue()
    {
        file.seekg(0, std::ios::beg);

        T first_element = Deserialize<T>(file);
        std::string first_element_serialized = Serialize(first_element);
        int offset = first_element_serialized.length();

        std::fstream tmpfile("persistent-queue.tmp");
        for (T element : *this)
        {
            tmpfile << Serialize(element);
        }
        file << tmpfile.rdbuf();
    }

    int Size()
    {
        int size = 0;
        for (auto e : *this)
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
        file.seekg(0, std::ios::end);
        int end = file.tellg();
        return Iterator(file, file.tellg());
    }
};


#endif
