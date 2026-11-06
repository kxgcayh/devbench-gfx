// SPDX-License-Identifier: GPL-3.0-or-later
#include "Path.h"

#include <cassert>

int main()
{
    using V = std::vector<std::string>;
    assert((SplitPath("_root.a.0.b") == V{ "_root", "a", "0", "b" }));
    assert((SplitPath("_root") == V{ "_root" }));
    assert((SplitPath("") == V{}));
    assert((SplitPath(".a..b.") == V{ "a", "b" }));
    assert(IsIndex("0") && IsIndex("123"));
    assert(!IsIndex("") && !IsIndex("a1") && !IsIndex("1234567890"));
    return 0;
}
