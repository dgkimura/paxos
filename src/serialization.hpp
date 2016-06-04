#include <sstream>

#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"

#include <decree.hpp>
#include <messages.hpp>
#include <replicaset.hpp>


template <typename Archive>
void serialize(Archive& ar, Decree& obj, const unsigned int version)
{
    ar & obj.author;
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
    ar & obj.decree;
    ar & obj.from;
    ar & obj.to;
    ar & obj.type;
}


template <typename T>
std::stringstream Serialize(T& object)
{
    std::stringstream stream;
    boost::archive::text_oarchive oa(stream);
    oa << object;
    return stream;
}


template <typename T>
T Deserialize(std::stringstream stream)
{
    T object;
    boost::archive::text_iarchive oa(stream);
    oa >> object;
    return object;
}
