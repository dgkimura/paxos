#include <functional>

#include "gtest/gtest.h"

#include "callback.hpp"


TEST(CallbackTest, testCallbackWithLambdaExpression)
{
    bool ran_callback = false;
    Message m;

    Callback callback([&](Message m) { ran_callback = true; });
    callback(m);

    ASSERT_TRUE(ran_callback);
}
