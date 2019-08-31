// TypeResolver.h created on 2019-06-13, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCI_SCRIPT_TYPECHECKER_H
#define XCI_SCRIPT_TYPECHECKER_H

#include "AST.h"

namespace xci::script {


class TypeResolver: public ast::BlockProcessor {
public:
    void process_block(Function& func, const ast::Block& block) final;
};


} // namespace xci::script

#endif // include guard
