/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_EXTENSIONS_CHRONO_H
#define NUCLEAR_EXTENSIONS_CHRONO_H

#include "NUClear.h"

namespace NUClear {

    struct ChronoConfig {
        std::type_index type;
        std::function<void ()> emitter;
        clock::duration step;
    };

    template <int ticks, class period>
    struct Reactor::Exists<Internal::CommandTypes::Every<ticks, period>> {
        static void exists(Reactor* context) {

            // Direct emit our type to the ChronoConfiguration
            context->emit<Scope::DIRECT>(new ChronoConfig(typeid(Internal::CommandTypes::Every<ticks, period>),
                                                          [context] { context->emit(new Internal::CommandTypes::Every<ticks, period>(clock::now())); },
                                                          clock::duration(period(ticks))));
        }
    };

    template <int ticks, class period>
    struct PowerPlant::CacheMaster::Get<Internal::CommandTypes::Every<ticks, period>> {
        static std::shared_ptr<clock::time_point> get(PowerPlant* context) {
            return std::shared_ptr<clock::time_point>(new clock::time_point(ValueCache<Internal::CommandTypes::Every<ticks, period>>::get()->time));
        }
    };

    namespace Extensions {

        /**
         * @brief The Chronomaster is a Service thread that manages the Every class time emissions.
         *
         * @details
         *  TODO
         */
        class Chrono : public Reactor {
        public:
            /// @brief Construct a new ThreadMaster with our PowerPlant as context
            Chrono(PowerPlant* parent);

            /**
             * @brief Adds a new timeperiod to count and send out events for.
             *
             * @details
             *  TODO
             *
             * @tparam ticks    The number of ticks of the period to wait between emitting events
             * @tparam period   The period that the ticks are measured in (a type of std::chrono::duration)
             */
            template <int ticks, class period>
            void add() {
                // Check if we have not already loaded this type in
                if(loaded.find(typeid(NUClear::Internal::CommandTypes::Every<ticks, period>)) == std::end(loaded)) {

                    // Flag this type as loaded
                    loaded.insert(typeid(NUClear::Internal::CommandTypes::Every<ticks, period>));

                    std::function<void (clock::time_point)> emitter = [this](clock::time_point time){
                        emit(new NUClear::Internal::CommandTypes::Every<ticks, period>(time));
                    };

                    // Get our period in whatever time our clock measures
                    clock::duration step((period(ticks)));

                    // See if we already have one with this period
                    auto item = std::find_if(std::begin(steps), std::end(steps), [step](std::unique_ptr<Step>& find) {
                        return find->step.count() == step.count();
                    });

                    // If we don't then create a new one with our initial data
                    if(item == std::end(steps)) {
                        std::unique_ptr<Step> s(new Step());
                        s->step = step;
                        s->callbacks.push_back(emitter);
                        steps.push_back(std::move(s));
                    }

                    // Otherwise just add the callback to the existing element
                    else {
                        (*item)->callbacks.push_back(emitter);
                    }
                }
            }

        private:
            /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
            struct Step {
                /// @brief the size our step is measured in (the size our clock uses)
                clock::duration step;
                /// @brief the time at which we need to emit our next event
                clock::time_point next;
                /// @brief the callbacks to emit for this time (e.g. 1000ms and 1second will trigger on the same tick)
                std::vector<std::function<void (clock::time_point)>> callbacks;
            };

            /// @brief Our Run method for the task scheduler, starts the Every events emitting
            void run();
            /// @brief Our Kill method for the task scheduler, shuts down the chronomaster and stops emitting events
            void kill();

            /// @brief a mutex which is responsible for controlling if the system should continue to run
            std::timed_mutex execute;
            /// @brief a lock which will be unlocked when the system should finish executing
            std::unique_lock<std::timed_mutex> lock;
            /// @brief A vector of steps containing the callbacks to execute, is sorted regularly to maintain the order
            std::vector<std::unique_ptr<Step>> steps;
            /// @brief A list of types which have already been loaded (to avoid duplication)
            std::set<std::type_index> loaded;
        };
    }
}

#endif
