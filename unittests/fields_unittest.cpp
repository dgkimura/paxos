#include "gtest/gtest.h"

#include "fields.hpp"


TEST(FieldsTest, testCanCreateFieldWithVolatileStore)
{
    Field<int> field(std::make_shared<VolatileStorage<int>>());
}


TEST(FieldsTest, testCanAssignValueToField)
{
    Field<int> field(std::make_shared<VolatileStorage<int>>());
    field = 1;

    ASSERT_EQ(field.Value(), 1);
}


TEST(FieldsTest, testCanAssignLocatorValueToField)
{
    Field<int> field(std::make_shared<VolatileStorage<int>>());
    int a_number = 1;
    field = a_number;

    ASSERT_EQ(field.Value(), 1);
}


TEST(FieldsTest, testCanUpdateValueOfField)
{
    Field<int> field(std::make_shared<VolatileStorage<int>>());
    field = 1;
    field = 3;

    ASSERT_EQ(field.Value(), 3);
}


TEST(FieldsTest, testCanDeclareFieldInsideStruct)
{
    struct Object{
        Field<int> a_field;

        Object()
            : a_field(std::make_shared<VolatileStorage<int>>())
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
        DecreeField a_field;

        Object()
            : a_field(std::make_shared<VolatileDecree>())
        {
        }
    };
}
