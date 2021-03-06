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

#ifndef NUCLEAR_REACTOR_HPP
#define NUCLEAR_REACTOR_HPP

#include <string>
#include <sstream>
#include <functional>
#include <vector>
#include <typeindex>
#include <chrono>
#include <atomic>

#include "nuclear_bits/util/Sequence.hpp"
#include "nuclear_bits/util/tuplify.hpp"
#include "nuclear_bits/util/CallbackGenerator.hpp"
#include "nuclear_bits/Environment.hpp"

#include "nuclear_bits/dsl/Parse.hpp"

#include "nuclear_bits/threading/Reaction.hpp"
#include "nuclear_bits/threading/ReactionHandle.hpp"
#include "nuclear_bits/LogLevel.hpp"

namespace NUClear {

    // Domain specific language forward declaration
    namespace dsl {
        namespace word {

            struct Always;

            struct Priority;

            struct IO;

            struct UDP;

            struct TCP;

            template <typename...>
            struct Optional;

            template <size_t, typename...>
            struct Last;

            struct MainThread;

            template <typename>
            struct Network;

            struct NetworkSource;

            template <typename>
            struct Trigger;

            template <typename...>
            struct With;

            struct Startup;

            struct Shutdown;

            template <int, typename>
            struct Every;

            template <typename>
            struct Per;

            struct Single;

            template <int>
            struct Buffer;

            template <typename>
            struct Sync;

            namespace emit {
                template <typename TData>
                struct Local;
                template <typename TData>
                struct Direct;
                template <typename TData>
                struct Initialize;
                template <typename TData>
                struct Network;
                template <typename TData>
                struct UDP;
            }
        }
    }

    /**
     * @brief Base class for any system that wants to react to events/data from the rest of the system.
     *
     * @details
     *  Provides functionality for binding callbacks to incoming data events. Callbacks are executed
     *  in a transparent, multithreaded manner. TODO needs to be expanded and updated
     *
     * @author Jake Woods
     * @author Trent Houliston
     * @version 1.1
     * @date 2-Apr-2013
     */
    class Reactor {
    public:
        friend class PowerPlant;

        Reactor(std::unique_ptr<Environment> environment)
          : reactionHandles()
          , powerplant(environment->powerplant)
          , reactorName(environment->reactorName)
          , logLevel(environment->logLevel) {
        }

        ~Reactor() {

            // Unbind everything when we destroy the reactor
            for (auto& handle : reactionHandles) {
                handle.unbind();
            }
        }

    private:
        std::vector<threading::ReactionHandle> reactionHandles;

    public:
        /// @brief TODO
        PowerPlant& powerplant;

        /// @brief The demangled string name of this reactor
        const std::string reactorName;

    protected:

        /// @brief The level that this reactor logs at
        LogLevel logLevel;

        /***************************************************************************************************************
         * The types here are imported from other contexts so that when extending from the Reactor type in normal      *
         * usage there does not need to be any namespace declarations on the used types. This affords a simpler API    *
         * for the user.                                                                                               *
         **************************************************************************************************************/

        /// @copydoc dsl::word::Trigger
        template <typename... TTriggers>
        using Trigger = dsl::word::Trigger<TTriggers...>;

        /// @copydoc dsl::word::Priority
        using Priority = dsl::word::Priority;

        /// @copydoc dsl::word::Always
        using Always = dsl::word::Always;

        /// @copydoc dsl::word::IO
        using IO = dsl::word::IO;

        /// @copydoc dsl::word::UDP
        using UDP = dsl::word::UDP;

        /// @copydoc dsl::word::TCP
        using TCP = dsl::word::TCP;

        /// @copydoc dsl::word::With
        template <typename... TWiths>
        using With = dsl::word::With<TWiths...>;

        /// @copydoc dsl::word::Optional
        template <typename... TDSL>
        using Optional = dsl::word::Optional<TDSL...>;

        /// @copydoc dsl::word::Last
        template <size_t len, typename... TDSL>
        using Last = dsl::word::Last<len, TDSL...>;

        /// @copydoc dsl::word::MainThread
        using MainThread = dsl::word::MainThread;

        /// @copydoc dsl::word::Startup
        using Startup = dsl::word::Startup;

        /// @copydoc dsl::word::Network
        template <typename TData>
        using Network = dsl::word::Network<TData>;

        /// @copydoc dsl::word::Network
        using NetworkSource = dsl::word::NetworkSource;

        /// @copydoc dsl::word::Shutdown
        using Shutdown = dsl::word::Shutdown;

        /// @copydoc dsl::word::Every
        template <int ticks = 0, class period = std::chrono::milliseconds>
        using Every = dsl::word::Every<ticks, period>;

        /// @copydoc dsl::word::Per
        template <class period>
        using Per = dsl::word::Per<period>;

        /// @copydoc dsl::word::Sync
        template <typename TSync>
        using Sync = dsl::word::Sync<TSync>;

        /// @copydoc dsl::word::Single
        using Single = dsl::word::Single;

        /// @copydoc dsl::word::Buffer
        template <int N>
        using Buffer = dsl::word::Buffer<N>;

        struct Scope {
            /// @copydoc dsl::word::emit::Local
            template <typename TData>
            using LOCAL = dsl::word::emit::Local<TData>;

            /// @copydoc dsl::word::emit::Direct
            template <typename TData>
            using DIRECT = dsl::word::emit::Direct<TData>;

            /// @copydoc dsl::word::emit::Initialize
            template <typename TData>
            using INITIALIZE = dsl::word::emit::Initialize<TData>;

            /// @copydoc dsl::word::emit::Network
            template <typename TData>
            using NETWORK = dsl::word::emit::Network<TData>;

            /// @copydoc dsl::word::emit::Network
            template <typename TData>
            using UDP = dsl::word::emit::UDP<TData>;
        };

        /// @brief This provides functions to modify how an on statement runs after it has been created
        using ReactionHandle = threading::ReactionHandle;

        template <typename DSL, typename... TArgs>
        struct Binder {
        private:
            Reactor& reactor;
            std::tuple<TArgs...> args;

            // On a reaction handle add it to our list
            void addReactionHandle(ReactionHandle& r) {
                reactor.reactionHandles.push_back(r);
            }

            // Do nothing if it's not a reaction handle
            void addReactionHandle(...) {}

            template <size_t i = 0, typename... Tp>
            std::enable_if_t<i == sizeof...(Tp)> addReactionHandles(std::tuple<Tp...>&) {}

            template <size_t i = 0, typename... Tp>
            std::enable_if_t<i < sizeof...(Tp)> addReactionHandles(std::tuple<Tp...>& t) {
                addReactionHandle(std::get<i>(t));
            }

            template <typename TFunc, int... Index>
            auto then(const std::string& label, TFunc&& callback, const util::Sequence<Index...>&)
            -> decltype(util::detuplify(DSL::bind(reactor, label, std::forward<TFunc>(callback), std::get<Index>(args)...))) {

                // Get our tuple from binding our reaction
                auto tuple = DSL::bind(reactor, label, util::CallbackGenerator<DSL, TFunc>(std::forward<TFunc>(callback)), std::get<Index>(args)...);

                // Get all reaction handles from the tuple and put them into our global list so we can debind them on destruction
                addReactionHandles(tuple);

                // Return the arguments to the user (if there is only 1 we unwrap it for them since this is the most common case)
                return util::detuplify(std::move(tuple));
            }

        public:
            Binder(Reactor& r, TArgs&&... args)
            : reactor(r)
            , args(args...) {}

            template <typename TLabel, typename TFunc>
            auto then(TLabel&& label, TFunc&& callback)
            -> decltype(std::declval<Binder>().then(label, std::forward<TFunc>(callback), util::GenerateSequence<0, sizeof...(TArgs)>())) {
                return then(std::forward<TLabel>(label), std::forward<TFunc>(callback), util::GenerateSequence<0, sizeof...(TArgs)>());
            }

            template <typename TFunc>
            auto then(TFunc&& callback)
            -> decltype(std::declval<Binder>().then(std::declval<std::string>(), std::forward<TFunc>(callback))) {
                return then("", std::forward<TFunc>(callback));
            }
        };

        // FUNCTIONS

        /**
         * @brief The on function is the method used to create a reaction in the NUClear system.
         *
         * @details
         *  This function is used to create a Reaction in the system. By providing the correct
         *  template parameters, this function can modify how and when this reaction runs.
         *
         * @tparam TDSL     The NUClear domain specific language information
         * @tparam TArgs    The types of the arguments passed into the function
         *
         * @param args      The arguments that will be passed to each of the binding DSL words in order
         *
         * @return A Binder object that can be used to bind callbacks to this DSL statement
         */
        template <typename... TDSL, typename... TArgs>
        Binder<dsl::Parse<TDSL...>, TArgs...> on(TArgs&&... args) {

            // There must be some parameters
            static_assert(sizeof...(TDSL) > 0, "You must have at least one parameter in an on");

            return Binder<dsl::Parse<TDSL...>, TArgs...>(*this, std::forward<TArgs>(args)...);
        }

        /**
         * @brief Emits data into the system so that other reactors can use it.
         *
         * @details
         *  This function emits data to the rest of the system so that it can be used.
         *  This results in it being the new data used when a with is used, and triggering
         *  any reaction that is set to be triggered on this data type.
         *
         *
         * @tparam THandlers    The handlers for this emit (e.g. LOCAL, NETWORK etc)
         * @tparam TData        The type of the data we are emitting
         *
         * @param data The data to emit
         */
        template <template <typename> class... THandlers, typename TData, typename... TArgs>
        void emit(std::unique_ptr<TData>&& data, TArgs&&... args) {
            powerplant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data), std::forward<TArgs>(args)...);
        }
        template <template <typename> class... THandlers, typename TData, typename... TArgs>
        void emit(std::unique_ptr<TData>& data, TArgs&&... args) {
            powerplant.emit<THandlers...>(std::forward<std::unique_ptr<TData>>(data), std::forward<TArgs>(args)...);
        }

        /**
         * @brief Log a message through NUClear's system.
         *
         * @details
         *  Logs a message through the system so the various log handlers
         *  can access it.
         *
         * @tparam level The level to log at (defaults to DEBUG)
         * @tparam TArgs The types of the arguments we are logging
         *
         * @param args The arguments we are logging
         */
        template <enum LogLevel level = DEBUG, typename... TArgs>
        void log(TArgs&&... args) {

            // If the log is above or equal to our log level
            if (level >= logLevel) {
                powerplant.log<level>(std::forward<TArgs>(args)...);
            }
        }
    };

}  // namespace NUClear

// Domain Specific Language
#include "nuclear_bits/dsl/word/Always.hpp"
#include "nuclear_bits/dsl/word/IO.hpp"
#include "nuclear_bits/dsl/word/UDP.hpp"
#include "nuclear_bits/dsl/word/TCP.hpp"
#include "nuclear_bits/dsl/word/Trigger.hpp"
#include "nuclear_bits/dsl/word/Priority.hpp"
#include "nuclear_bits/dsl/word/With.hpp"
#include "nuclear_bits/dsl/word/Startup.hpp"
#include "nuclear_bits/dsl/word/Network.hpp"
#include "nuclear_bits/dsl/word/Shutdown.hpp"
#include "nuclear_bits/dsl/word/Optional.hpp"
#include "nuclear_bits/dsl/word/Last.hpp"
#include "nuclear_bits/dsl/word/MainThread.hpp"
#include "nuclear_bits/dsl/word/Every.hpp"
#include "nuclear_bits/dsl/word/Single.hpp"
#include "nuclear_bits/dsl/word/Buffer.hpp"
#include "nuclear_bits/dsl/word/Sync.hpp"
#include "nuclear_bits/dsl/word/emit/Local.hpp"
#include "nuclear_bits/dsl/word/emit/Initialize.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/dsl/word/emit/Network.hpp"
#include "nuclear_bits/dsl/word/emit/UDP.hpp"

#endif  // NUCLEAR_REACTOR_HPP
