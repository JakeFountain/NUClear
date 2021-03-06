/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <catch.hpp>

#include "nuclear"

namespace {

    struct CustomGet : public NUClear::dsl::operation::TypeBind<int> {

        template <typename DSL>
        static inline std::shared_ptr<int> get(NUClear::threading::Reaction&) {
            return std::make_shared<int>(5);
        }
    };

    class TestReactor : public NUClear::Reactor {
    public:

        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            on<CustomGet>().then([this] (const int& x) {

                REQUIRE(x == 5);

                powerplant.shutdown();
            });

            on<Startup>().then([this] {

                // Emit from message 4 to 1
                emit(std::make_unique<int>(10));

            });
        }
    };
}

TEST_CASE("Test a custom reactor that returns a type that needs dereferencing", "[api][custom_get]") {

    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
