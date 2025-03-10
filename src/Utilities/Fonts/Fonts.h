#pragma once

#include "IconsFontAwesome6.h"
#include "nsmfont.h"

#include "../Singleton.hpp"
#include "imgui.h"

class FontRanges : public SingletonDestroyProbe<FontRanges> {
public:
    static constexpr ImWchar latin_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x1E00, 0x1EFF, // Latin Extended Additional
        0
    };

    static constexpr ImWchar icons_ranges_max[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};

public:
    friend class SingletonDestroyProbe<FontRanges>;

protected:
    FontRanges() = default;
    ~FontRanges() = default;
};
