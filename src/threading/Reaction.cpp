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

#include "nuclear_bits/threading/Reaction.hpp"

namespace NUClear {
    namespace threading {

        // Initialize our reaction source
        std::atomic<uint64_t> Reaction::reactionIdSource(0);

        Reaction::Reaction(Reactor& reactor
                           , std::vector<std::string> identifier
                           , std::function<std::pair<int, std::function<std::unique_ptr<ReactionTask> (std::unique_ptr<ReactionTask>&&)>> (Reaction&)> generator
                           , std::function<void (Reaction&)>&& unbinder)
          : reactor(reactor)
          , identifier(identifier)
          , reactionId(++reactionIdSource)
          , activeTasks(0)
          , enabled(true)
          , generator(generator)
          , unbinder(unbinder) {
        }

        void Reaction::unbind() {
            // Unbind
            unbinder(*this);
        }

        std::unique_ptr<ReactionTask> Reaction::getTask() {

            // If we are not enabled, don't run
            if (!enabled) {
                return std::unique_ptr<ReactionTask>(nullptr);
            }

            // Run our generator to get a functor we can run
            int priority;
            std::function<std::unique_ptr<ReactionTask> (std::unique_ptr<ReactionTask>&&)> func;
            std::tie(priority, func) = generator(*this);

            // If our generator returns a valid function
            if(func) {
                return std::unique_ptr<ReactionTask>(new ReactionTask(*this, priority, func));
            }
            // Otherwise we return a null pointer
            else {
                return std::unique_ptr<ReactionTask>(nullptr);
            }
        }

        bool Reaction::isEnabled() {
            return enabled;
        }
    }
}
