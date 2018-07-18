// demo_logger.cpp created on 2018-03-02, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <xci/util/log.h>
using namespace xci::util::log;

#include <ostream>
using std::ostream;


class ArbitraryObject {};
ostream& operator<<(ostream& stream, const ArbitraryObject&) {
    return stream << "I am arbitrary!";
}


int main()
{
    ArbitraryObject obj;

    log_debug("{} {}!", "Hello", "World");
    log_info("float: {} int: {}!", 1.23f, 42);
    log_warning("arbitrary object: {}", obj);
    log_error("beware");

    TRACE("trace message");
}
