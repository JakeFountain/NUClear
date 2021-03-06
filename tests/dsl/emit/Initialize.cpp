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

// Anonymous namespace to keep everything file local
namespace {

    struct ShutdownNowPlx {};

    class TestReactor : public NUClear::Reactor {
    public:

        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            emit<Scope::INITIALIZE>(std::make_unique<int>(5));

            on<Trigger<int>>().then([this](const int& v) {
                REQUIRE(v == 5);

                // We can't call shutdown here because
                // we haven't started yet. That's because
                // emits from Scope::INITIALIZE are not
                // considered fully "initialized"
                emit(std::make_unique<ShutdownNowPlx>());
            });

            on<Trigger<ShutdownNowPlx>>().then([this] {
                powerplant.shutdown();
            });
        }
    };
}

TEST_CASE("Testing the Initialize scope", "[api][emit][initialize]") {
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
