//
// DBGUI
// DBGUI.cpp
//
// Advanced Immediate mode GUI for Data Base management, using ImGui (v1.91.8 +)
// Made by OPSphystech420 2025 (c)
//

#include "DBGUI.hpp"

#include <map>

using namespace ImGui;

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

ImVec2 operator/(const ImVec2 &lhs, float rhs) {
  return ImVec2(lhs.x / rhs, lhs.y / rhs);
}

ImVec2 operator*(const ImVec2 &lhs, float rhs) {
  return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

ImVec2 operator*(float lhs, const ImVec2 &rhs) {
  return ImVec2(lhs * rhs.x, lhs * rhs.y);
}

static int InputTextCallback(ImGuiInputTextCallbackData* data) {
    InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        std::string* str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char*)str->c_str();
    }
    else if (user_data->ChainCallback)
    {
        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

ImVec3 DBGui::HSLtoRGB(float h, float s, float l) {
    float r, g, b;

    auto hue2rgb = [](float p, float q, float t) {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };

    if (s == 0.0f) {
        r = g = b = l;
    } else {
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hue2rgb(p, q, h + 1.0f / 3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f / 3.0f);
    }

    return ImVec3(r, g, b);
}

void DBGui::SeparatorText(const char* label, float thickness)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label_size.x, g.FontSize);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) {
        return;
    }

    window->DrawList->AddText(pos, GetColorU32(ImGuiCol_TextDisabled), label);

    if (thickness > 0)
        window->DrawList->AddLine(pos + ImVec2(label_size.x + style.ItemInnerSpacing.x, size.y / 2), pos + ImVec2(size.x, size.y / 2), GetColorU32(ImGuiCol_Border), thickness);
}

bool DBGui::RadioButton(const char* label, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 3.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    bool active = *v == current_id;

    // Colors
    ImVec4 colFrameMain = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull = colFrameMain; 
    colFrameNull.w = 0.0f;
    ImVec4 colFrame = (active ? colFrameMain : colFrameNull);

    ImVec4 colText = GetStyleColorVec4((active || hovered || held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    // Animations
    struct RadioButtonState {
        ImVec4 Frame;
        ImVec4 Label;
    };

    static std::map<ImGuiID, RadioButtonState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.emplace(id, RadioButtonState{
            colFrame, // Initial Frame color
            colText   // Initial Text color
        });
        it_anim = anim.find(id);
    }

    // Interpolate Frame Color
    it_anim->second.Frame = ImLerp(
        it_anim->second.Frame,
        colFrame,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );

    // Interpolate Text Color
    it_anim->second.Label = ImLerp(
        it_anim->second.Label,
        colText,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );

    // Render
    RenderNavHighlight(bb, id);

    // Draw Frame
    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(it_anim->second.Frame), style.FrameRounding);

    // Optionally, add a border if desired
    // window->DrawList->AddRect(pos, pos + size, GetColorU32(ImGuiCol_Border), style.FrameRounding, 0, style.FrameBorderSize);

    // Draw Text
    window->DrawList->AddText(
        pos + ImVec2(
            (size.x - label_size.x) * 0.5f,
            (size.y - label_size.y) * 0.5f
        ),
        GetColorU32(it_anim->second.Label),
        label
    );

    if (pressed)
        *v = current_id;

    return pressed;
}

bool DBGui::RadioButtonIcon(const char* std_id, const char* icon, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(std_id);
    const ImVec2 icon_size = CalcTextSize(icon, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, icon_size.x + style.FramePadding.x * 2.0f, icon_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    bool active = *v == current_id;

    float border_size = style.FrameBorderSize;

    // Define a state struct for animations
    struct RadioButtonIconState {
        ImVec4 FrameBg;
        ImVec4 Border;
        ImVec4 Text;
    };

    // Animate Colors
    static std::map<ImGuiID, RadioButtonIconState> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.emplace(id, RadioButtonIconState{
            GetStyleColorVec4(ImGuiCol_FrameBg), // Initial FrameBg
            GetStyleColorVec4(ImGuiCol_Border),   // Initial Border
            GetStyleColorVec4(ImGuiCol_Text)      // Initial Text
        });
        it_anim = anim.find(id);
    }

    // Interpolate Frame Background Color
    ImVec4 frameColDefault = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 frameColNull = frameColDefault;
    frameColNull.w = 0.0f;
    it_anim->second.FrameBg = ImLerp(
        it_anim->second.FrameBg,
        active ? frameColDefault : frameColNull,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );
    ImU32 frameCol = GetColorU32(it_anim->second.FrameBg);
    
    // Interpolate Border Color
    ImVec4 borderColDefault = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 borderColNull = borderColDefault;
    borderColNull.w = 0.0f;
    it_anim->second.Border = ImLerp(
        it_anim->second.Border,
        active ? borderColDefault : borderColNull,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );
    ImU32 borderCol = GetColorU32(it_anim->second.Border);
    
    // Interpolate Text Color
    ImVec4 targetTextColor = active
        ? GetStyleColorVec4(ImGuiCol_SliderGrab)
        : (held || hovered)
            ? GetStyleColorVec4(ImGuiCol_Text)
            : GetStyleColorVec4(ImGuiCol_TextDisabled);
    it_anim->second.Text = ImLerp(
        it_anim->second.Text,
        targetTextColor,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );
    ImU32 textCol = GetColorU32(it_anim->second.Text);

    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + size, frameCol, size.y / 2.0f);
    if (border_size > 0.0f)
        window->DrawList->AddRect(pos, pos + size, borderCol, size.y / 2.0f, 0, border_size);

    window->DrawList->AddText(
        pos + ImVec2(
            size.x / 2.0f - icon_size.x / 2.0f,
            size.y / 2.0f - icon_size.y / 2.0f
        ),
        textCol,
        icon
    );

    if (pressed)
        *v = current_id;

    return pressed;
}

bool DBGui::RadioFrameIcon(const char* label, const char* icon, ImFont* icon_font, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 label_size = CalcTextSize(label, NULL, true);
    PushFont(icon_font);
    ImVec2 icon_size = CalcTextSize(icon, NULL, true);
    PopFont();

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + icon_size.y + style.FramePadding.y * 3.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held, active = *v == current_id;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *v = current_id;

    // Colors
    ImVec4 colFrameMain = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 colFrameNull = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame = (active ? colFrameMain : colFrameNull);

    ImVec4 colIcon = GetStyleColorVec4(active ? ImGuiCol_SliderGrab : (hovered || held) ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    ImVec4 colLabel = GetStyleColorVec4((hovered || held || active) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    // Animations
    struct stColors_State { 
        ImVec4 Frame;
        ImVec4 Label;
        ImVec4 Icon;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Label = colLabel;
        it_anim->second.Icon = colIcon;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Label = ImLerp(it_anim->second.Label, colLabel, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Icon = ImLerp(it_anim->second.Icon, colIcon, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + size, GetColorU32(it_anim->second.Frame), style.FrameRounding);
    window->DrawList->AddText(icon_font, icon_font->FontSize, pos + ImVec2(size.x / 2 - icon_size.x / 2, style.FramePadding.y), GetColorU32(it_anim->second.Icon), icon);
    window->DrawList->AddText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y - label_size.y - style.FramePadding.y), GetColorU32(it_anim->second.Label), label);

    return pressed;
}

bool DBGui::InputText(const char* label, const char* text, char* buf, size_t buf_size, float width, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 text_size = CalcTextSize(text, NULL, true);

    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()

    bool result = false;
    bool has_label = label_size.x > 0;

    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;

    BeginGroup();
    {
        if (has_label) {
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.ItemInnerSpacing.y));
            ImGui::Text("%s", label);
        }
        {
            ImVec2 pos = GetCursorScreenPos();
            PushItemWidth(w);
            {
                result |= InputTextEx(std::string("##" + std::string(label)).c_str(), NULL, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
            }
            PopItemWidth();

            if (text_size.x > 0)
            {
                if (!ImGui::IsItemActive() && !strlen(buf)) {
                    ImGui::SetCursorScreenPos(pos + style.FramePadding);
                    ImGui::TextDisabled("%s", text);
                }
            }
        }
        if (has_label) {
            PopStyleVar();
        }
    }
    EndGroup();

    return result;
}

bool DBGui::InputTextStr(const char* label, const char* text, std::string* str, float width, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return DBGui::InputText(label, text, (char*)str->c_str(), str->capacity() + 1, width, flags, InputTextCallback, &cb_user_data);
}

bool DBGui::Button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Colors
    ImVec4 colFrame = GetStyleColorVec4((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    // Animations
    struct stColors_State {
        ImVec4 Frame;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);
    RenderFrame(bb.Min, bb.Max, GetColorU32(it_anim->second.Frame), true, style.FrameRounding);

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    return pressed;
}

bool DBGui::ButtonAccent(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Colors
    ImVec4 colFrame = GetStyleColorVec4((!held && hovered) ? ImGuiCol_SliderGrab : held ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrabActive);
    ImVec4 colBorder = GetStyleColorVec4((held && hovered) ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Border = colBorder;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);
    PushStyleColor(ImGuiCol_Border, it_anim->second.Border);
    RenderFrame(bb.Min, bb.Max, GetColorU32(it_anim->second.Frame), true, style.FrameRounding);
    PopStyleColor();

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    return pressed;
}

bool DBGui::ToggleButtonClassic(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    float height = g.FontSize * 1.5f;
    float width = height * 1.65f;
    float rounding = height / 2;

    float grab_radius = height * 0.45f;
    float grab_padding = (height - grab_radius * 2) / 2;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + width : width, height);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *v = !*v;

    ImVec4 colFrame = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_Border);
    ImVec4 colGrab = GetStyleColorVec4(*v ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
        ImVec4 Grab;
        float GrabPos;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Border = colBorder;
        it_anim->second.Grab = colGrab;
        it_anim->second.GrabPos = *v ? (width - height + grab_padding) : grab_padding;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Grab = ImLerp(it_anim->second.Grab, colGrab, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.GrabPos = ImLerp<float>(
        it_anim->second.GrabPos,
        *v ? (width - grab_radius - grab_padding) : grab_padding + grab_radius,
        1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime
    );

    RenderNavHighlight(bb, id);


    window->DrawList->AddRectFilled(pos, pos + ImVec2(width, size.y), GetColorU32(it_anim->second.Frame), rounding);

    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + ImVec2(width, size.y), GetColorU32(it_anim->second.Border), rounding, 0, style.FrameBorderSize);

    window->DrawList->AddCircleFilled(pos + ImVec2(it_anim->second.GrabPos, rounding), grab_radius, GetColorU32(it_anim->second.Grab), 16);

    if (label_size.x > 0)
    {
        RenderText(pos + ImVec2(width + style.ItemInnerSpacing.x, (size.y - label_size.y) / 2), label);
    }

    return pressed;
}

bool DBGui::ToggleButton(const char* label, const char* description, bool* v, float spacing_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 description_size = CalcTextSize(description, NULL, true);

    float height = g.FontSize * 1.5f;
    float width = height * 1.65f;
    float rounding = height / 2;

    float spacing = ImGui::CalcItemSize(ImVec2(spacing_arg > 0 ? spacing_arg : -0.1f, 0), label_size.x > 0 ? (label_size.x + style.ItemInnerSpacing.x) : 0, 0).x;

    float grab_radius = height * 0.45f;
    float grab_padding = (height - grab_radius * 2) / 2;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + width : width, g.FontSize * 2 + style.ItemInnerSpacing.y);

    const ImRect bb(pos, pos + ImVec2(spacing, size.y));
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *v = !*v;

    // Colors
    ImVec4 colFrame = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : ImGuiCol_Border);
    ImVec4 colGrab = GetStyleColorVec4(*v ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
        ImVec4 Grab;
        float GrabPos;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Border = colBorder;
        it_anim->second.Grab = colGrab;
        it_anim->second.GrabPos = *v ? 1.0f : 0.0f;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Grab = ImLerp(it_anim->second.Grab, colGrab, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.GrabPos = ImLerp<float>(it_anim->second.GrabPos, *v ? (width - rounding) : rounding, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);

    float pad = (size.y - height) / 2;
    window->DrawList->AddRectFilled(pos + ImVec2(spacing - width, pad), pos + ImVec2(spacing, size.y - pad), GetColorU32(it_anim->second.Frame), rounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos + ImVec2(spacing - width, pad), pos + ImVec2(spacing, size.y - pad), GetColorU32(it_anim->second.Border), rounding, 0, style.FrameBorderSize);

    float grab_pos = *v ? (width - rounding * 2) : 0;

    window->DrawList->AddCircleFilled(pos + ImVec2(spacing - width, pad) + ImVec2(it_anim->second.GrabPos, rounding), grab_radius, GetColorU32(it_anim->second.Grab), 16);

    if (label_size.x > 0)
    {
        RenderText(pos, label);
        PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
        RenderText(pos + ImVec2(0, g.FontSize + style.ItemInnerSpacing.y), description);
        PopStyleColor();
    }

    return pressed;
}

bool DBGui::CheckBox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    float height = g.FontSize;

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(label_size.x > 0 ? label_size.x + style.ItemInnerSpacing.x + height : height, height);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
        *v = !*v;

    // Colors
    ImVec4 colFrame = GetStyleColorVec4(*v ? ImGuiCol_SliderGrabActive : (hovered && !held) ? ImGuiCol_FrameBgHovered : held ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
    ImVec4 colBorder = GetStyleColorVec4(*v ? ImGuiCol_SliderGrab : ImGuiCol_Border);
    ImVec4 colCheckMarkMain = GetStyleColorVec4(ImGuiCol_CheckMark);
    ImVec4 colCheckMarkNull = colCheckMarkMain; colCheckMarkNull.w = 0.0f;
    ImVec4 colCheckMark = (*v ? colCheckMarkMain : colCheckMarkNull);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
        ImVec4 CheckMark;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Border = colBorder;
        it_anim->second.CheckMark = colCheckMark;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.CheckMark = ImLerp(it_anim->second.CheckMark, colCheckMark, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + ImVec2(height, size.y), GetColorU32(it_anim->second.Frame), style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(pos, pos + ImVec2(height, size.y), GetColorU32(it_anim->second.Border), style.FrameRounding, 0, style.FrameBorderSize);

    float pad = ImMax(1.0f, IM_TRUNC(height / 3.6f));
    RenderCheckMark(window->DrawList, pos + ImVec2(pad, pad), GetColorU32(it_anim->second.CheckMark), height - pad * 2.0f);

    if (label_size.x > 0)
    {
        RenderText(pos + ImVec2(height + style.ItemInnerSpacing.x, 0), label);
    }

    return pressed;
}

bool DBGui::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, float width, const char* format)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;
    w -= label_size.x > 0 ? 0 : CalcTextSize(value_buf).x + style.ItemInnerSpacing.x;

    ImVec2 pos = window->DC.CursorPos;

    const float label_height = label_size.x > 0 ? g.FontSize + style.ItemInnerSpacing.y : 0.0f;
    const ImRect frame_bb(pos + ImVec2(0, label_height), pos + ImVec2(w, g.FontSize + label_height));
    const ImRect total_bb(pos, pos + ImVec2(w, g.FontSize + label_height));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags); // InFlags patched to ItemFlags
    const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id); // patched params, added ImGuiInputFlags_None as second
    const bool make_active = (clicked || g.NavActivateId == id);

    if (make_active)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    // Colors
    ImVec4 colFrame = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 colGrab = GetStyleColorVec4(g.ActiveId == id ? ImGuiCol_SliderGrab : ImGuiCol_SliderGrabActive);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Grab;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Grab = colGrab;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Grab = ImLerp(it_anim->second.Grab, colGrab, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(it_anim->second.Frame), true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, 0, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    ImVec2 grab_padding = ImVec2(grab_bb.Min.y - frame_bb.Min.y, 0); grab_padding.y = grab_padding.x;

    // Render grab
    if (grab_bb.Max.x > grab_bb.Min.x)
    {
        window->DrawList->AddRectFilled(frame_bb.Min, grab_bb.Max + grab_padding, GetColorU32(it_anim->second.Grab), style.FrameRounding);

        if (style.FrameBorderSize > 0)
        {
            window->DrawList->AddRect(frame_bb.Min, grab_bb.Max + grab_padding, GetColorU32(ImGuiCol_SliderGrab), style.FrameRounding, 0, style.FrameBorderSize);
        }
    }

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    float extra_width;
    if (label_size.x > 0) {
        extra_width = -ImGui::CalcTextSize(value_buf).x;
    }
    else {
        extra_width = style.ItemInnerSpacing.x;
    }

    window->DrawList->AddText(pos + ImVec2(w + extra_width, 0), GetColorU32(ImGuiCol_TextDisabled), value_buf);

    if (label_size.x > 0.0f)
        RenderText(pos, label);

    return value_changed;
}

bool DBGui::SliderFloat(const char* label, float* v, float v_min, float v_max, float width, const char* format)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, width, format);
}

bool DBGui::SliderInt(const char* label, int* v, int v_min, int v_max, float width, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, width, format);
}

bool DBGui::ColorEdit3(const char* label, float col[3], bool ShowName)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec4 col_v4(col[0], col[1], col[2], 1.0f);

    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label_size.x + style.ItemInnerSpacing.x + g.FontSize * 2, g.FontSize);

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;

    BeginGroup();

    if (ShowName && label_size.x > 0) {
        Text("%s", label);
        SameLine(size.x - g.FontSize * 2);
    }
    
    ImGuiID popup_id = window->GetID(label);

    bool result = ColorButton(label, col_v4, flags, ImVec2(g.FontSize * 2, g.FontSize));
    if (result)
    {
        OpenPopup(popup_id);
    }

    if (BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
        ColorPicker3(label, col, flags);
        EndPopup();
    }

    EndGroup();

    return result;
}

bool DBGui::ColorEdit4(const char* label, float col[4], bool ShowName)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec4 col_v4(col[0], col[1], col[2], col[3]);

    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label_size.x + style.ItemInnerSpacing.x + g.FontSize * 2, g.FontSize);

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;

    BeginGroup();

    if (ShowName && label_size.x > 0) {
        Text("%s", label);
        SameLine(size.x - g.FontSize * 2);
    }
    
    ImGuiID popup_id = window->GetID(label);

    bool result = ColorButton(label, col_v4, flags, ImVec2(g.FontSize * 2, g.FontSize));
    if (result)
    {
        OpenPopup(popup_id);
    }

    if (BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
    {
        ColorPicker4(label, col, flags);
        EndPopup();
    }

    EndGroup();

    return result;
}

bool DBGui::Selectable(const char* label, bool selected, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    float borderSize = style.FrameBorderSize;

    // Colors
    ImVec4 colFrameMain = GetStyleColorVec4((hovered && !selected) ? held ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered : ImGuiCol_Header);
    ImVec4 colFrameNull = colFrameMain; colFrameNull.w = 0.0f;
    ImVec4 colFrame = ((!hovered && !selected) ? colFrameNull : colFrameMain);

    ImVec4 colBorderMain = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 colBorderNull = colBorderMain; colBorderNull.w = 0.0f;
    ImVec4 colBorder = (selected ? colBorderMain : colBorderNull);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Border;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Border = colBorder;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Border = ImLerp(it_anim->second.Border, colBorder, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.Frame), style.FrameRounding);

    if (borderSize > 0)
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(it_anim->second.Border), style.FrameRounding, 0, borderSize);

    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    return pressed;
}

bool DBGui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiNextWindowDataFlags backup_next_window_data_flags = g.NextWindowData.HasFlags; // Flags patched to HasFlags
    g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together
    if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT(((ImGuiComboFlagsPrivate_)flags & ((ImGuiComboFlagsPrivate_)ImGuiComboFlags_NoPreview | (ImGuiComboFlagsPrivate_)ImGuiComboFlags_CustomPreview)) == 0);

    const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float preview_width = ((flags & ImGuiComboFlags_WidthFitPreview) && (preview_value != NULL)) ? CalcTextSize(preview_value, NULL, true).x : 0.0f;
    const float w = (flags & ImGuiComboFlags_NoPreview) ? arrow_size : ((flags & ImGuiComboFlags_WidthFitPreview) ? (arrow_size + preview_width + style.FramePadding.x * 2.0f) : CalcItemWidth());

    const ImRect bb(window->DC.CursorPos + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f), window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f));
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f) + ImVec2(0.0f, label_size.x > 0 ? label_size.y + style.ItemInnerSpacing.y : 0.0f));

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &bb))
        return false;

    // Open on click
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    // Colors
    ImVec4 colFrame = GetStyleColorVec4(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImVec4 colText = GetStyleColorVec4((popup_open || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled);

    // Animations
    struct stColors_State {
        ImVec4 Frame;
        ImVec4 Text;
    };

    static std::map<ImGuiID, stColors_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, stColors_State() });
        it_anim = anim.find(id);

        it_anim->second.Frame = colFrame;
        it_anim->second.Text = colText;
    }

    it_anim->second.Frame = ImLerp(it_anim->second.Frame, colFrame, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);
    it_anim->second.Text = ImLerp(it_anim->second.Text, colText, 1.0f / DBGUI_ANIMATIONS_SPEED * ImGui::GetIO().DeltaTime);

    // Render shape
    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    RenderNavHighlight(bb, id);
    if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.Frame), style.FrameRounding);
    if (!(flags & ImGuiComboFlags_NoArrowButton))
    {
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
            RenderArrow(window->DrawList, ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y), GetColorU32(it_anim->second.Text), ImGuiDir_Down, 1.0f);
    }
    RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Custom preview
    if (flags & ImGuiComboFlags_CustomPreview)
    {
        g.ComboPreviewData.PreviewRect = ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
    }

    // Render preview and label
    if (preview_value != NULL && !(flags & ImGuiComboFlags_NoPreview))
    {
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
        RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
        PopStyleColor();
    }
    if (label_size.x > 0)
        RenderText(total_bb.Min, label);

    if (!popup_open)
        return false;

    g.NextWindowData.HasFlags = backup_next_window_data_flags; // Flags patched to HasFlags
    return BeginComboPopup(popup_id, bb, flags);
}

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

bool DBGui::BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    if (!IsPopupOpen(popup_id, ImGuiPopupFlags_None))
    {
        g.NextWindowData.ClearFlags();
        return false;
    }

    // Set popup size
    float w = bb.GetWidth();
    if (g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint) // Flags patched to HasFlags
    {
        g.NextWindowData.SizeConstraintRect.Min.x = ImMax(g.NextWindowData.SizeConstraintRect.Min.x, w);
    }
    else
    {
        if ((flags & ImGuiComboFlags_HeightMask_) == 0)
            flags |= ImGuiComboFlags_HeightRegular;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiComboFlags_HeightMask_)); // Only one
        int popup_max_height_in_items = -1;
        if (flags & ImGuiComboFlags_HeightRegular)     popup_max_height_in_items = 8;
        else if (flags & ImGuiComboFlags_HeightSmall)  popup_max_height_in_items = 4;
        else if (flags & ImGuiComboFlags_HeightLarge)  popup_max_height_in_items = 20;
        ImVec2 constraint_min(0.0f, 0.0f), constraint_max(FLT_MAX, FLT_MAX);
        if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.x <= 0.0f) // Don't apply constraints if user specified a size  // Flags patched to HasFlags
            constraint_min.x = w;
        if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSize) == 0 || g.NextWindowData.SizeVal.y <= 0.0f) // Flags patched to HasFlags
            constraint_max.y = CalcMaxPopupHeightFromItemCount(popup_max_height_in_items);
        SetNextWindowSizeConstraints(constraint_min, constraint_max);
    }

    // This is essentially a specialized version of BeginPopupEx()
    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

    // Set position given a custom constraint (peak into expected window size so we can position it)
    // FIXME: This might be easier to express with an hypothetical SetNextWindowPosConstraints() function?
    // FIXME: This might be moved to Begin() or at least around the same spot where Tooltips and other Popups are calling FindBestWindowPosForPopupEx()?
    if (ImGuiWindow* popup_window = FindWindowByName(name))
        if (popup_window->WasActive)
        {
            // Always override 'AutoPosLastDirection' to not leave a chance for a past value to affect us.
            ImVec2 size_expected = CalcWindowNextAutoFitSize(popup_window);
            popup_window->AutoPosLastDirection = (flags & ImGuiComboFlags_PopupAlignLeft) ? ImGuiDir_Left : ImGuiDir_Down; // Left = "Below, Toward Left", Down = "Below, Toward Right (default)"
            ImRect r_outer = GetPopupAllowedExtentRect(popup_window);
            ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
            SetNextWindowPos(pos + ImVec2(0, g.Style.ItemSpacing.y));
        }

    // We don't use BeginPopupEx() solely because we have a custom name string, which we could make an argument to BeginPopupEx()
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.FramePadding);
    PushStyleVar(ImGuiStyleVar_PopupRounding, g.Style.FrameRounding);
    bool ret = Begin(name, NULL, window_flags);
    PopStyleVar(2);
    if (!ret)
    {
        EndPopup();
        IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
        return false;
    }
    return true;
}

bool DBGui::Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;

    // Call the getter to obtain the preview string which is a parameter to BeginCombo()
    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    // The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
    if (popup_max_height_in_items != -1 && !(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint)) // Flags patched to HasFlags
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
        return false;

    // Display items
    // FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
    bool value_changed = false;
    for (int i = 0; i < items_count; i++)
    {
        const char* item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";

        PushID(i);
        const bool item_selected = (i == *current_item);
        if (Selectable(item_text, item_selected, ImVec2(-0.1f, 0)) && *current_item != i)
        {
            value_changed = true;
            *current_item = i;
            //CloseCurrentPopup();
        }
        if (item_selected)
            SetItemDefaultFocus();
        PopID();
    }

    EndCombo();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

// Getter for the old Combo() API: const char*[]
static const char* Items_ArrayGetter(void* data, int idx)
{
    const char* const* items = (const char* const*)data;
    return items[idx];
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
static const char* Items_SingleStringGetter(void* data, int idx)
{
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    return *p ? p : NULL;
}

bool DBGui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
    return value_changed;
}

bool DBGui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }

    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;
    PushItemWidth(w);
    bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    PopItemWidth();

    return value_changed;
}

bool DBGui::BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = size_arg;
    size.x -= style.FramePadding.x * 2;
    
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;
    
    // Render
    const float circleStart = size.x * 0.7f;
    const float circleEnd = size.x;
    const float circleWidth = circleEnd - circleStart;
    
    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
    window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart*value, bb.Max.y), fg_col);
    
    const float t = g.Time;
    const float r = size.y / 2;
    const float speed = 1.5f;
    
    const float a = speed*0;
    const float b = speed*0.333f;
    const float c = speed*0.666f;
    
    const float o1 = (circleWidth+r) * (t+a - speed * (int)((t+a) / speed)) / speed;
    const float o2 = (circleWidth+r) * (t+b - speed * (int)((t+b) / speed)) / speed;
    const float o3 = (circleWidth+r) * (t+c - speed * (int)((t+c) / speed)) / speed;
    
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
    window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
    return true;
}

bool DBGui::Spinner(const char* label, float radius, int thickness, const ImU32& color) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);
    
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;
    
    // Render
    window->DrawList->PathClear();
    
    int num_segments = 30;
    int start = std::abs(static_cast<float>(ImSin(g.Time * 1.8f) * (num_segments - 5))); // abs(ImSin(g.Time*1.8f)*(num_segments-5));

    
    const float a_min = IM_PI*2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI*2.0f * ((float)num_segments-3) / (float)num_segments;
    
    const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);
    
    for (int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a+g.Time*8) * radius,
                                            centre.y + ImSin(a+g.Time*8) * radius));
    }
    
    window->DrawList->PathStroke(color, false, thickness);
    return true;
}
