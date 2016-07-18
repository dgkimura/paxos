#include "gtest/gtest.h"

#include "fields.hpp"


TEST(FieldsTest, testCanCreateVolatileFieldWithFieldName)
{
    VolatileField<int> *f = new VolatileField<int>("AFieldName");

    ASSERT_TRUE(f != nullptr);
}


TEST(FieldsTest, testCanCreateFieldWithFieldName)
{
    Field<int> *f = new VolatileField<int>("AFieldName");

    ASSERT_TRUE(f != nullptr);
}


TEST(FieldsTest, testCanAssignValueToVolatileField)
{
    VolatileField<int> f("AFieldName");
    f = 1;

    ASSERT_EQ(f.Value(), 1);
}


TEST(FieldsTest, testCanUpdateValueOfVolatileField)
{
    VolatileField<int> f("AFieldName");
    f = 1;
    f = 3;

    ASSERT_EQ(f.Value(), 3);
}
