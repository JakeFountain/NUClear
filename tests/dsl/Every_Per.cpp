/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include <numeric>

#include "nuclear"

namespace {
    
    class TestReactorPer : public NUClear::Reactor {
    public:
        // Store our times
        std::vector<NUClear::clock::time_point> times;
        
        static constexpr uint NUM_LOG_ITEMS = 100;
        
        static constexpr uint CYCLES_PER_SECOND = 100;
        
        TestReactorPer(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            // Trigger every 10 milliseconds
            on<Every<CYCLES_PER_SECOND, Per<std::chrono::seconds>>>().then([this]() {
                
                // Start logging our times each time an emit happens
                times.push_back(NUClear::clock::now());
                
                // Once we have enough items then we can do our statistics
                if (times.size() == NUM_LOG_ITEMS) {
                    
                    // Build up our difference vector
                    std::vector<double> diff;
                    
                    for(uint i = 0; i < times.size() - 1; ++i) {
                        std::chrono::nanoseconds delta = times[i + 1] - times[i];
                        
                        // Store our difference in seconds
                        diff.push_back(double(delta.count()) / double(std::nano::den));
                    }
                    
                    // Normalize our differences to jitter
                    for(double& d : diff) {
                        d -= 1.0/double(CYCLES_PER_SECOND);
                    }
                    
                    // Calculate our mean, range, and stddev for the set
                    double sum = std::accumulate(std::begin(diff), std::end(diff), 0.0);
                    double mean = sum / double(diff.size());
                    double variance = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
                    double stddev = std::sqrt(variance / double(diff.size()));
                    
                    INFO("Sum: " << sum);
                    INFO("Mean: " << mean);
                    INFO("Var: " << variance);
                    INFO("Stddev: " << stddev);
                    
                    // As time goes on the average wait should be 0 (we accept less then 0.5ms for this test)
                    REQUIRE(fabs(mean) < 0.0005);
                    
                    // Require that 95% (ish) of all results are within 3ms
                    WARN("This (mean: " + std::to_string(mean * 1000000) + "\u00b5s sd: " + std::to_string(stddev * 1000000) + "\u00b5s) is far too high error, need to reduce it somehow in the future");
                    REQUIRE(fabs(mean + stddev * 2) < 0.003);
                }
                // Once we have more then enough items then we shutdown the powerplant
                else if(times.size() > NUM_LOG_ITEMS) {
                    // We are finished the test
                    this->powerplant.shutdown();
                }
            });
        }
    };
}

TEST_CASE("Testing the Every<> Smart Type using Per", "[api][every][per]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactorPer>();
    
    plant.start();
}
