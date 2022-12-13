/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

template <typename  T>
struct remove_pointer {
    typedef T type;
};

template <typename T>
struct remove_pointer<T*> {
    typedef T type;
};

template <typename T>
struct remove_reference {
    typedef T type;
};

template <typename T>
struct remove_reference<T&> {
    typedef T type;
};

template <typename T>
struct remove_const {
    typedef T type;
};

template <typename T>
struct remove_const<const T> {
    typedef T type;
};

template <typename T>
struct remove_const <const T *> {
    typedef T* type;
};

template <typename FROM, typename TO, bool ENABLE>
struct narrowing {
    static_assert(   std::is_arithmetic<FROM>::value
                  && std::is_arithmetic<TO>::value
                 );

    using common_t = typename std::common_type<FROM, TO>::type;
    static constexpr bool value =    ENABLE
                                  && (    (   std::is_floating_point<FROM>::value
                                           && std::is_integral<TO>::value
                                          ) 
                                       || (   std::is_signed<FROM>::value
                                           && std::is_unsigned<TO>::value
                                          )
                                       || ( !std::is_same<FROM,TO>::value
                                            && (   static_cast<common_t>(std::numeric_limits<FROM>::max()) > static_cast<common_t>(std::numeric_limits<TO>::max())
                                                || static_cast<common_t>(std::numeric_limits<FROM>::min()) < static_cast<common_t>(std::numeric_limits<TO>::min())
                                               )
                                          )
                                     )
                                  ;
};

template <typename TYPE, std::intmax_t VAL>
struct in_signed_range {
    // Until C++ 20
    static_assert(   std::is_integral<TYPE>::value
                  && std::is_signed<TYPE>::value
                 );

    using common_t = typename std::common_type<TYPE, decltype(VAL)>::type;
    static constexpr bool value =    static_cast<common_t>(VAL) >= static_cast<common_t>(std::numeric_limits<TYPE>::min())
                                  && static_cast<common_t>(VAL) <= static_cast<common_t>(std::numeric_limits<TYPE>::max())
                                  ;
};

template <typename TYPE, std::uintmax_t VAL>
struct in_unsigned_range {
    // Until C++ 20
    static_assert(   std::is_integral<TYPE>::value
                  && std::is_unsigned<TYPE>::value
                 );

    using common_t = typename std::common_type<TYPE, decltype(VAL)>::type;
    static constexpr bool value =    static_cast<common_t>(VAL) >= static_cast<common_t>(std::numeric_limits<TYPE>::min())
                                  && static_cast<common_t>(VAL) <= static_cast<common_t>(std::numeric_limits<TYPE>::max())
                                  ;
};

// Suppress compiler warnings of unused (parameters)
// Omitting the name is sufficient but a search on this keyword provides easy access to the location
template <typename T>
void silence(T&&) {}
