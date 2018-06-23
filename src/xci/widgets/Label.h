// Label.h created on 2018-06-23, part of XCI toolkit
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

#ifndef XCIKIT_LABEL_H
#define XCIKIT_LABEL_H

#include <xci/widgets/Widget.h>
#include <xci/text/Layout.h>

namespace xci {
namespace widgets {


class Label: public Widget {
public:
    explicit Label(const std::string &string);

    void resize(View& view) override;
    void draw(View& view, State state) override;

private:
    text::Layout m_layout;
    float m_padding = 0.02f;
};


}} // namespace xci::widgets

#endif // XCIKIT_LABEL_H
