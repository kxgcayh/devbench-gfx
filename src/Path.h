// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

// Splits "_root.a.0.b" on dots. Empty segments are dropped, so a stray dot cannot address an empty member name.
inline std::vector<std::string> SplitPath(const std::string& a_path)
{
    std::vector<std::string> out;
    std::string cur;
    for (const char c : a_path) {
        if (c == '.') {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

inline bool IsIndex(const std::string& a_seg)
{
    return !a_seg.empty() && a_seg.size() < 10 &&
           std::all_of(a_seg.begin(), a_seg.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}
