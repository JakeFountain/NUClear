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

#ifndef NUCLEAR_DSL_WORD_MAINTHREAD_HPP
#define NUCLEAR_DSL_WORD_MAINTHREAD_HPP

#include "nuclear_bits/util/main_thread_id.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @ingroup Options
             * @brief This option requires that this task executes using the main thread (usually for graphics related tasks)
             */
            struct MainThread {

                using task_ptr = std::unique_ptr<threading::ReactionTask>;

                template <typename DSL>
                static inline std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task) {

                    // If we are not the main thread, move us to the main thread
                    if(std::this_thread::get_id() != util::main_thread_id) {

                        // Submit to the main thread scheduler
                        task->parent.reactor.powerplant.submitMain(std::move(task));

                        // We took the task away so return null
                        return std::unique_ptr<threading::ReactionTask>(nullptr);
                    }
                    // Otherwise run!
                    else {
                        return std::move(task);
                    }
                }
            };

        }  // namespace word
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_MAINTHREAD_HPP
