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

#ifndef NUCLEAR_DSL_FUSION_GETFUSION_HPP
#define NUCLEAR_DSL_FUSION_GETFUSION_HPP

#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/util/tuplify.hpp"
#include "nuclear_bits/threading/ReactionHandle.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_get.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /**
             * @brief This is our Function Fusion wrapper class that allows it to call get functions
             *
             * @tparam Function the get function that we are wrapping for
             * @tparam DSL      the DSL that we pass to our get function
             */
            template <typename Function, typename DSL>
            struct GetCaller {
                static inline auto call(threading::Reaction& reaction)
                -> decltype(Function::template get<DSL>(reaction)) {
                    return Function::template get<DSL>(reaction);
                }
            };

            template<typename, typename = std::tuple<>>
            struct GetWords;

            /**
             * @brief Metafunction that extracts all of the Words with a get function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TGetWords The words we have found with get functions
             */
            template <typename TWord, typename... TRemainder, typename... TGetWords>
            struct GetWords<std::tuple<TWord, TRemainder...>, std::tuple<TGetWords...>>
            : public std::conditional_t<has_get<TWord>::value,
                /*T*/ GetWords<std::tuple<TRemainder...>, std::tuple<TGetWords..., TWord>>,
                /*F*/ GetWords<std::tuple<TRemainder...>, std::tuple<TGetWords...>>> {};

            /**
             * @brief Termination case for the GetWords metafunction
             *
             * @tparam TGetWords The words we have found with get functions
             */
            template <typename... TGetWords>
            struct GetWords<std::tuple<>, std::tuple<TGetWords...>> {
                using type = std::tuple<TGetWords...>;
            };

            /// Type that redirects types without a get function to their proxy type
            template <typename TWord>
            struct Get {
                using type = std::conditional_t<has_get<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };

            // Default case where there are no get words
            template <typename TWords>
            struct GetFuser {};

            // Case where there is at least one get word
            template <typename TFirst, typename... TWords>
            struct GetFuser<std::tuple<TFirst, TWords...>> {

                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::Reaction& reaction)
                -> decltype(util::FunctionFusion<std::tuple<TFirst, TWords...>
                           , decltype(std::forward_as_tuple(reaction))
                           , GetCaller
                           , std::tuple<DSL>
                           , 1>::call(reaction)) {

                    // Perform our function fusion
                    return util::FunctionFusion<std::tuple<TFirst, TWords...>
                    , decltype(std::forward_as_tuple(reaction))
                    , GetCaller
                    , std::tuple<DSL>
                    , 1>::call(reaction);
                }
            };

            template <typename TFirst, typename... TWords>
            struct GetFusion
            : public GetFuser<typename GetWords<std::tuple<typename Get<TFirst>::type, typename Get<TWords>::type...>>::type> {
            };

        }  // namespace fusion
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_GETFUSION_HPP
