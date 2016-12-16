#ifndef __SERIALIZATION_HPP_INCLUDED__
#define __SERIALIZATION_HPP_INCLUDED__

#include <fstream>
#include <sstream>

#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"

#include "paxos/decree.hpp"
#include "paxos/messages.hpp"
#include "paxos/replicaset.hpp"


template <typename Archive>
void serialize(Archive& ar, Decree& obj, const unsigned int version)
{
    ar & obj.author;
    ar & obj.number;
    ar & obj.type;
    ar & obj.content;
}


template <typename Archive>
void serialize(Archive& ar, SystemDecree& obj, const unsigned int version)
{
    ar & obj.operation;
    ar & obj.number;
    ar & obj.content;
}


template <typename Archive>
void serialize(Archive& ar, Replica& obj, const unsigned int version)
{
    ar & obj.hostname;
    ar & obj.port;
}


template <typename Archive>
void serialize(Archive& ar, Message& obj, const unsigned int version)
{
    ar & obj.from;
    ar & obj.to;
    ar & obj.type;
    ar & obj.decree;
}


template <typename T>
std::string Serialize(T object)
{
    std::stringstream stream;
    boost::archive::text_oarchive oa(stream);
    oa << object;
    return stream.str();
}


template <typename T>
T Deserialize(std::string string_obj)
{
    T object;
    std::stringstream stream(string_obj);
    boost::archive::text_iarchive oa(stream);
    oa >> object;
    return object;
}


template <typename T>
T Deserialize(std::fstream& stream)
{
    T object;
    boost::archive::text_iarchive oa(stream);
    oa >> object;
    return object;
}


#endif
