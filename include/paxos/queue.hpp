#ifndef __QUEUE_HPP_INCLUDED__
#define __QUEUE_HPP_INCLUDED__

#include <fstream>
#include <mutex>

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

    virtual T Last() = 0;

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

    T Last()
    {
        T empty;
        return data.size() > 0 ? data[data.size() - 1 ] : empty;
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
        return data.size();
    }

    std::vector<T> data;
};


constexpr const int INDEX_SIZE = 10;


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
          insert_file((fs::path(dirname) / fs::path(filename)).string(),
                       std::ios::in | std::ios::out | std::ios::binary),
          insert_stream(insert_file),
          start_position(0),
          end_position(0),
          mutex()
    {
        SaveOffsets();
        LoadOffsets();
    }

    PersistentQueue(std::iostream& stream)
        : stream(stream),
          insert_stream(stream),
          start_position(0),
          end_position(0),
          mutex()
    {
        SaveOffsets();
        LoadOffsets();
    }

    ~PersistentQueue()
    {
        stream.flush();
    }

    void SaveOffsets()
    {
        stream.seekg(0, std::ios::end);
        if (stream.tellg() < INDEX_SIZE * 2)
        {
            start_position = INDEX_SIZE * 2;
            end_position = -1;
        }

        //insert_stream.seekp(0, std::ios::beg);
        insert_stream << std::setw(INDEX_SIZE) << start_position;
        insert_stream << std::setw(INDEX_SIZE) << end_position;
        insert_stream.flush();
    }

    void LoadOffsets()
    {
        stream.seekg(0, std::ios::beg);

        std::string buffer;
        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        start_position = std::stoi(buffer);

        stream.read(&buffer[0], INDEX_SIZE);
        boost::trim(buffer);
        end_position = std::stoi(buffer);
    }

    void Enqueue(T e)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        stream.seekg(0, std::ios::end);
        end_position = stream.tellg();

        insert_stream.seekp(INDEX_SIZE, std::ios::beg);
        insert_stream << std::setw(INDEX_SIZE) << end_position;
        insert_stream.flush();

        stream.seekp(0, std::ios::end);
        std::string element_as_string = Serialize<T>(e);
        stream << element_as_string;
        stream.flush();
    }

    void Dequeue()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        bool is_head = true;
        for (T element : *this)
        {
            if (is_head)
            {
                is_head = false;
                start_position += Serialize<T>(element).length();
                stream.seekp(0, std::ios::beg);
                stream << std::setw(INDEX_SIZE) << start_position;
            }
        }
    }

    T Last()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        T empty;
        stream.seekg(end_position, std::ios::beg);

        return end_position == -1 ? empty : Deserialize<T>(stream);
    }

private:

    T getElementAt(int index)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(index, std::ios::beg);
        return Deserialize<T>(stream);
    }

    int getElementSizeAt(int index)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(index, std::ios::beg);
        return Serialize<T>(Deserialize<T>(stream)).length();
    }

    int getFirstElementIndex()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        LoadOffsets();

        return start_position;
    }

    int getLastElementIndex()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        stream.seekg(0, std::ios::end);
        return stream.tellg();
    }

    std::fstream file;

    std::iostream& stream;

    std::fstream insert_file;

    std::iostream& insert_stream;

    int start_position;

    int end_position;

    std::recursive_mutex mutex;
};


#endif
