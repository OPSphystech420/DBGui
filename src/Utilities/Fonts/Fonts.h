#pragma once

#include "imgui.h"
#include <functional>

/* * * [ Font data ] * * */

// 'FontAwesome'
#include "IconsFontAwesome6.h"
extern const unsigned int fa6_solid_compressed_size;
extern const unsigned int fa6_solid_compressed_data[];

// 'NotoSansMedium.ttf' (583016 bytes)
extern const unsigned int FiraCode_compressed_size;
extern const unsigned int FiraCode_compressed_data[];
/* * * * * * * * * * * * */

#include "../Singleton.hpp"

class IOFont : public SingletonDestructPattern<IOFont>
{
public:
    
    enum class Font {
        NotoSansMedium,
        FontAwesome,
        // ...
    };
    
    /* * * [ Font ranges ] * * */
    
    static constexpr ImWchar nSfont[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x1E00, 0x1EFF, // Latin Extended Additional
        0
    };

    static constexpr ImWchar fAfont[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    /* * * * * * * * * * * * * */
    
    ImFont* AddFont(Font font, float fontSize, bool forceMerge = false, const ImWchar rangeS[] = {}, std::function<void(ImFontConfig&)> configModifier = nullptr)
    {
        FontDescript details = GetData(font, rangeS);
        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;
        if (forceMerge && !io.Fonts->Fonts.empty())
        {
            config.MergeMode = true;
            config.PixelSnapH = true;
        }
        if (configModifier)
        {
            configModifier(config);
        }
        return io.Fonts->AddFontFromMemoryCompressedTTF(
            details.fontData,
            details.dataSize,
            fontSize,
            &config,
            details.glyphRanges
        );
    }

    void AddFontDefault() { io.Fonts->AddFontDefault(); }
    
public:
    friend class SingletonDestructPattern<IOFont>;
    
private:
    ImGuiIO& io;
    
    struct FontDescript {
        const void* fontData;
        uint32_t dataSize;
        const ImWchar* glyphRanges;
    };
    
    FontDescript GetData(Font font, const ImWchar rangeS[])
    {
        FontDescript details;
        switch (font)
        {
            case Font::NotoSansMedium:
                details.fontData = FiraCode_compressed_data;
                details.dataSize = FiraCode_compressed_size;
                details.glyphRanges = rangeS == 0 ? nSfont : rangeS;
                break;
            case Font::FontAwesome:
                details.fontData = fa6_solid_compressed_data;
                details.dataSize = fa6_solid_compressed_size;
                details.glyphRanges = rangeS == 0 ? fAfont : rangeS;
                break;
            default:
                details.fontData = nullptr;
                details.dataSize = 0;
                details.glyphRanges = nullptr;
                break;
        }
        return details;
    }

protected:
    IOFont() : io(ImGui::GetIO())
    {
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;     // Enable Gamepad Controls
        
        io.Fonts->Clear();
    }
    
    ~IOFont() = default;
};
