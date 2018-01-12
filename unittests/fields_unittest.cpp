#include "gtest/gtest.h"

#include "paxos/fields.hpp"


TEST(FieldsTest, testCanCreateFieldWithVolatileStore)
{
    paxos::Field<int> field(std::make_shared<paxos::VolatileStorage<int>>());
}


TEST(FieldsTest, testCanAssignValueToField)
{
    paxos::Field<int> field(std::make_shared<paxos::VolatileStorage<int>>());
    field = 1;

    ASSERT_EQ(field.Value(), 1);
}


TEST(FieldsTest, testCanAssignLValueToField)
{
    paxos::Field<int> field(std::make_shared<paxos::VolatileStorage<int>>());
    int a_number = 1;
    field = a_number;

    ASSERT_EQ(field.Value(), 1);
}


TEST(FieldsTest, testCanUpdateValueOfField)
{
    paxos::Field<int> field(std::make_shared<paxos::VolatileStorage<int>>());
    field = 1;
    field = 3;

    ASSERT_EQ(field.Value(), 3);
}


TEST(FieldsTest, testCanDeclareFieldInsideStruct)
{
    struct Object{
        paxos::Field<int> a_field;

        Object()
            : a_field(std::make_shared<paxos::VolatileStorage<int>>())
        {
        }
    };

    Object o;
    o.a_field = 3;

    ASSERT_EQ(o.a_field.Value(), 3);
}


TEST(FieldsTest, testCanUseConvenienceFieldMacros)
{
    struct Object{
        paxos::DecreeField a_field;

        Object()
            : a_field(std::make_shared<paxos::VolatileDecree>())
        {
        }
    };
}


TEST(FieldsTest, testCanAssignValueToPersistentStorageField)
{
    std::stringstream file;

    paxos::Field<int> field(std::make_shared<paxos::PersistentStorage<int>>(file));
    field = 1;

    ASSERT_EQ(field.Value(), 1);
}


TEST(FieldsTest, testCanUpdateValueOfPersistentStorageField)
{
    std::stringstream file;

    paxos::Field<int> field(std::make_shared<paxos::PersistentStorage<int>>(file));
    field = 1;
    field = 3;

    ASSERT_EQ(field.Value(), 3);
}
