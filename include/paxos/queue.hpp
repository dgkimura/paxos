#ifndef __QUEUE_HPP_INCLUDED__
#define __QUEUE_HPP_INCLUDED__

#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "serialization.hpp"

namespace fs = boost::filesystem;


template <typename T>
class BaseQueue
{
private:

    virtual T getElementAt(int index) = 0;

    virtual int getElementSizeAt(int index) = 0;

    virtual int getFirstElementIndex() = 0;

    virtual int getLastElementIndex() = 0;

    class Iterator
    {
    public:

        Iterator(BaseQueue<T> *queue_, int index_)
            : queue(queue_), index(index_)
        {
        }

        T operator*()
        {
            return queue->getElementAt(index);
        }

        Iterator& operator++()
        {
            int size = queue->getElementSizeAt(index);
            index += size;
            return *this;
        }

        bool operator!=(const Iterator& iterator)
        {
            return index != iterator.index;
        }

    private:

        BaseQueue<T> *queue;

        int index;
    };

public:

    virtual void Enqueue(T e) = 0;

    virtual void Dequeue() = 0;

    virtual int Size() = 0;

    Iterator begin()
    {
        return Iterator(this, getFirstElementIndex());
    }

    Iterator end()
    {
        return Iterator(this, getLastElementIndex());
    }
};


template <typename T>
class VolatileQueue : public BaseQueue<T>
{
public:

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

private:

    T getElementAt(int index)
    {
        return data[index];
    }

    int getElementSizeAt(int index)
    {
        return 1;
    }

    int getFirstElementIndex()
    {
        return 0;
    }

    int getLastElementIndex()
    {
        return Size();
    }

    std::vector<T> data;
};


template <typename T>
class PersistentQueue : public BaseQueue<T>
{
public:

    PersistentQueue(std::string filename)
        : PersistentQueue(".", filename)
    {
    }

    PersistentQueue(std::string dirname, std::string filename)
        : file((fs::path(dirname) / fs::path(filename)).string(),
               std::ios::out | std::ios::in | std::ios::app | std::ios::binary),
          stream(file),
          start_position(0)
    {
    }

    PersistentQueue(std::iostream& stream)
        : stream(stream),
          start_position(0)
    {
        stream.seekg(0, std::ios::end);
        if (stream.tellg() < sizeof(int))
        {
            stream << std::setw(sizeof(int)) << sizeof(int);
            start_position = sizeof(int);
        }
        else
        {
            stream.seekg(0, std::ios::beg);

            std::string buffer;
            stream.read(&buffer[0], sizeof(int));
            boost::trim(buffer);
            start_position = std::stoi(buffer);
        }
    }

    ~PersistentQueue()
    {
        stream.flush();
    }

    void Enqueue(T e)
    {
        stream.seekp(0, std::ios::end);
        std::string element_as_string = Serialize<T>(e);
        stream << element_as_string;
        stream.flush();
    }

    void Dequeue()
    {
        bool is_head = true;
        for (T element : *this)
        {
            if (is_head)
            {
                is_head = false;
                start_position += Serialize<T>(element).length();
                stream.seekp(0, std::ios::beg);
                stream << std::setw(sizeof(int)) << start_position;
            }
        }
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

private:

    T getElementAt(int index)
    {
        stream.seekg(index, std::ios::beg);
        return Deserialize<T>(stream);
    }

    int getElementSizeAt(int index)
    {
        stream.seekg(index, std::ios::beg);
        return Serialize<T>(Deserialize<T>(stream)).length();
    }

    int getFirstElementIndex()
    {
        return start_position;
    }

    int getLastElementIndex()
    {
        stream.seekg(0, std::ios::end);
        return stream.tellg();
    }

    std::fstream file;

    std::iostream& stream;

    int start_position;
};


#endif
