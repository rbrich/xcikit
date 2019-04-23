// Form.h created on 2018-06-22, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#ifndef XCI_WIDGETS_FORM_H
#define XCI_WIDGETS_FORM_H

#include <xci/widgets/Widget.h>

namespace xci::widgets {


class Form: public Composite {
public:
    // High-level interface
    void add_input(const std::string& label, std::string& text_input);
    void add_input(const std::string& label, bool& checkbox);

    // Low-level interface
    enum class Hint {
        None,
        NextColumn,
        NextRow,
    };
    void add_hint(Hint hint) { add_hint(m_child.size(), hint); }
    void add_hint(size_t child_index, Hint hint);

    // Override Composite
    void resize(View& view) override;

private:
    ViewportCoords m_margin = {0.02f, 0.02f};

    struct ChildHint {
        size_t child_index;
        Hint hint;

        bool operator<(const ChildHint& rhs) const {
            return child_index < rhs.child_index;
        }
    };
    std::vector<ChildHint> m_hint;
};


} // namespace xci::widgets

#endif // XCI_WIDGETS_FORM_H
