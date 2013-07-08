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

#ifndef NUCLEAR_INTERNAL_REACTION_H
#define NUCLEAR_INTERNAL_REACTION_H

#include <functional>
#include <chrono>
#include <atomic>
#include <typeindex>
#include <queue>
#include <vector>
#include <mutex>
#include <memory>
#include <string>

#include "NUClear/Internal/CommandTypes/CommandTypes.h"

namespace NUClear {
    
    struct NUClearTaskEvent {
        std::string name;
        std::uint64_t reactionId;
        std::uint64_t taskId;
        std::chrono::time_point<std::chrono::steady_clock> emitted;
        std::chrono::time_point<std::chrono::steady_clock> started;
        std::chrono::time_point<std::chrono::steady_clock> finished;
        std::vector<std::pair<std::type_index, std::shared_ptr<void>>> args;
        std::exception_ptr exception;
    };
    
namespace Internal {
    
    // Forward declare the SyncQueue type
    struct SyncQueue;
    
    /**
     * @brief This type is used to identify reactions (every reaction has a unique identifier).
     */
    typedef std::uint64_t reactionId_t;
    
    /**
     * @brief This class holds the definition of a Reaction (call signature)
     *
     * @details
     *  A reaction holds the information about a callback. It holds the options as to how to process it in the scheduler,
     *  it also holds a function which is used to generate databound Task objects (callback with the function arguments
     *  already loaded and ready to run)
     *
     * @author Trent Houliston
     */
    class Reaction {
        public:
            
            // Forward declare Task
            class Task;
        
        // TODO add ability to enable/disable reactions
        
            /**
             * @brief This struct holds all of the options which are specified in the Options<> part of a call.
             *
             * @details
             *  This struct is populated with the information which is specified in the Options section of a callback specifier.
             *  the options stored here influence how the scheduler handles this object (when it is scheduled in the thread pool)
             *
             * @author Trent Houliston
             */
            struct Options {
                /**
                 * @brief Creates a new ReactionOptions with the default options.
                 *
                 * @details
                 *  This creates a new ReactionOptions with the default optoins. These default options are a Sync type of
                 *  nullptr, a non single case, and DEFAULT priority.
                 */
                Options() : m_syncQueue(nullptr), m_single(false), m_priority(DEFAULT) {}
                
                /// @brief the sync type to use, acts as a compile time mutex
                std::shared_ptr<SyncQueue> m_syncQueue;
                /// @brief if only a single event of this type should be queued or running at any time
                bool m_single;
                /// @brief the priority with which to execute
                EPriority m_priority;
            };
        
            /**
             * @brief This is a databound call of a Reaction ready to be executed.
             *
             * @details
             *  This class holds a reaction that is ready to be executed. It is a Reaction object which has had it's callback
             *  parameters bound with data. This can then be executed as a function to run the call inside it.
             *
             * @author Trent Houliston
             */
            class Task {
                private:
                    /// @brief a source for reactionIds, atomically creates longs
                    static std::atomic<std::uint64_t> taskIdSource;
                public:
                    /**
                     * @brief Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
                     *
                     * @param parent    the Reaction object that spawned this ReactionTask
                     * @param task      the data bound callback to be executed in the threadpool
                     */
                    Task(Reaction* parent, std::function<void ()> task);
                    
                    /**
                     * @brief Runs the internal data bound task and times it.
                     *
                     * @details
                     *  This runs the internal data bound task and times how long the execution takes. These figures can then be
                     *  used in a debugging context to calculate how long callbacks are taking to run.
                     */
                    void operator()();
                    
                    /// @brief the data bound callback to be executed
                    std::function<void ()> m_callback;
                    /// @brief the parent Reaction object which spawned this
                    Reaction* m_parent;
                    // TODO document
                    reactionId_t m_taskId;
                    /// @brief the statistics object that persists after this for information and debugging
                    std::shared_ptr<NUClearTaskEvent> m_stats;
                    /// @brief the time that this event was created and sent to the queue
                    std::chrono::time_point<std::chrono::steady_clock> m_emitTime;
            };
        
            /**
             * @brief Constructs a new Reaction with the passed callback generator and options
             *
             * @param callback  the callback generator function (creates databound callbacks)
             * @param options   the options to use in Tasks
             */
            Reaction(std::string name, std::function<std::function<void ()> ()> callback, Reaction::Options options);
        
            /**
             * @brief creates a new databound callback task that can be executed.
             *
             * @return a unique_ptr to a Task which has the data for it's call bound into it
             */
            std::unique_ptr<Task> getTask();
            ///
            const std::string m_name;
            /// @brief the options for this Reaction (decides how Tasks will be scheduled)
            Reaction::Options m_options;
            /// @brief the unique identifier for this Reaction object
            const reactionId_t m_reactionId;
            /// @brief if this reaction is currently enqueued or running
            volatile bool m_running;
        private:
            /// @brief a source for reactionIds, atomically creates longs
            static std::atomic<std::uint64_t> reactionIdSource;
            /// @brief the callback generator function (creates databound callbacks)
            std::function<std::function<void ()> ()> m_callback;
    };
    
    // TODO document
    struct SyncQueue {
        SyncQueue(const std::type_index type, bool active) : m_type(type), m_active(active) {}
        const std::type_index m_type;
        volatile bool m_active;
        std::mutex m_mutex;
        std::priority_queue<std::unique_ptr<Internal::Reaction::Task>> m_queue;
    };
    
    // This is declared here so that by the time we allocate our SyncQueue it is not an incomplete type
    template <typename TSync>
    const std::shared_ptr<SyncQueue> CommandTypes::Sync<TSync>::queue = std::shared_ptr<Internal::SyncQueue>(new SyncQueue(typeid(TSync), false));
}
}
#endif
