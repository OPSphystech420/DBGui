//
// DBGUI
// DBGUI.hpp
//
// Advanced Immediate mode GUI for Data Base management, using ImGui (v1.91.8 +)
// Made by OPSphystech420 2025 (c)
//

#ifndef DB_GUI_H
#define DB_GUI_H

#include <string>

#include "imgui.h"
#include "imgui_internal.h"

#define DBGUI_ANIMATIONS_SPEED 0.05f

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs);
ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs);
ImVec2 operator/(const ImVec2 &lhs, float rhs);
ImVec2 operator*(const ImVec2 &lhs, float rhs);
ImVec2 operator*(float lhs, const ImVec2 &rhs);

struct InputTextCallback_UserData
{
    std::string*            Str;
    ImGuiInputTextCallback  ChainCallback;
    void*                   ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData* data);

namespace DBGui
{
    // Helpers
    ImVec3  HSLtoRGB(float h, float s, float l);

    constexpr ImVec4 HexToColorVec4(uint32_t hex_color, float alpha) {
        ImVec4 color;

        color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
        color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
        color.z = (hex_color & 0xFF) / 255.0f;
        color.w = alpha;

        return color;
    }

    // Separator
    void    SeparatorText(const char* label, float thickness = 1.0f);

    // Inputs, Buttons
    bool    RadioButton(const char* label, int* v, int current_id, const ImVec2& size_arg = ImVec2(0, 0));
    bool    RadioButtonIcon(const char* std_id, const char* icon, int* v, int current_id, const ImVec2& size_arg = ImVec2(0, 0));
    bool    RadioFrameIcon(const char* label, const char* icon, ImFont* icon_font, int *v, int current_id, const ImVec2& size_arg = ImVec2(0, 0));

    bool    InputText(const char* label, const char* text, char* buf, size_t buf_size, float width = 0.0, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
    bool    InputTextStr(const char* label, const char* text, std::string* str, float width, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	bool    Button(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);
	bool    ButtonAccent(const char* label, const ImVec2& size_arg = ImVec2(0, 0), ImGuiButtonFlags flags = 0);

    // Toggle, Checkbox
    bool    ToggleButtonClassic(const char* label, bool* v);
    bool    ToggleButton(const char* label, const char* description, bool* v, float width = -0.1f);
    bool    CheckBox(const char* label, bool* v);
    
    // Slider
    bool    SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, float width, const char* format = NULL);
    bool    SliderFloat(const char* label, float* v, float v_min, float v_max, float width = -0.1f, const char* format = "%.1f");
    bool    SliderInt(const char* label, int* v, int v_min, int v_max, float width = -0.1f, const char* format = "%d");

    // Color
    bool    ColorEdit3(const char* label, float col[3], bool ShowName = true);
    bool    ColorEdit4(const char* label, float col[4], bool ShowName = true);

    // Combo
    bool    Selectable(const char* label, bool selected = false, const ImVec2& size_arg = ImVec2(0, 0));
    bool    BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
    bool	BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags);
    bool    Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
    bool    Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width = -0.1f, int popup_max_height_in_items = -1);
    bool    Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items = -1);

    bool	KeyBindEx(const char* str_id, int* k, float custom_width = 0);
    bool	KeyBind(const char* label, int* k, float width = -0.1f);

    // Animation
    bool    BufferingBar(const char* label, float value,  const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);
    bool    Spinner(const char* label, float radius, int thickness, const ImU32& color);
}


#include <functional>

namespace DBGuiExternal {

    static void* gActiveButton = nullptr;

    struct ButtonState {
        bool   isDragging    = false;
        bool   pressStarted  = false;
        ImVec2 dragOffset    = ImVec2(0, 0);
        ImVec2 pressStartPos = ImVec2(0, 0);
    };

    template<typename StateT>
    void Button(
        const char*                  label,
        bool                         movable,
        ImVec2&                      pos,
        StateT&                      state,
        const std::function<void()>& onClick
    )
    {
        ImU32 colorNormal  = ImGui::GetColorU32(ImGuiCol_Button);
        ImU32 colorHovered = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        ImU32 colorActive  = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        ImU32 colorText    = ImGui::GetColorU32(ImGuiCol_Text);

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 buttonSize(50.0f, 50.0f);

        ImGuiIO& io          = ImGui::GetIO();
        ImVec2  mousePos     = io.MousePos;
        bool    mouseDown    = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool    mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool    mouseReleased= ImGui::IsMouseReleased(ImGuiMouseButton_Left);

        ImVec2 buttonMin = pos;
        ImVec2 buttonMax = ImVec2(pos.x + buttonSize.x, pos.y + buttonSize.y);

        bool hovered = (mousePos.x >= buttonMin.x && mousePos.x <= buttonMax.x &&
                        mousePos.y >= buttonMin.y && mousePos.y <= buttonMax.y);

        const float dragThreshold = 5.0f;

        if (gActiveButton && gActiveButton != static_cast<void*>(&state))
        {
            //
        }
        else
        {

            if (!state.isDragging && hovered && mouseClicked)
            {
                state.pressStarted  = true;
                state.pressStartPos = mousePos;
                state.dragOffset    = ImVec2(mousePos.x - pos.x, mousePos.y - pos.y);
                gActiveButton = static_cast<void*>(&state);
            }

            if (state.pressStarted && !state.isDragging && mouseDown)
            {
                float distX = mousePos.x - state.pressStartPos.x;
                float distY = mousePos.y - state.pressStartPos.y;
                float distSqr = distX * distX + distY * distY;
                if (distSqr > (dragThreshold * dragThreshold) && movable)
                {
                    state.isDragging = true;
                }
            }

            if (state.isDragging && mouseDown)
            {
                if (movable)
                {
                    pos.x = mousePos.x - state.dragOffset.x;
                    pos.y = mousePos.y - state.dragOffset.y;
                }
            }

            if (mouseReleased)
            {
                float distX = mousePos.x - state.pressStartPos.x;
                float distY = mousePos.y - state.pressStartPos.y;
                float distSqr = distX * distX + distY * distY;
                const float clickThreshold = 5.0f;

                if (state.isDragging)
                {
                    state.isDragging = false;
                }

                else if (state.pressStarted && hovered && distSqr < (clickThreshold * clickThreshold))
                {
                    if (onClick)
                        onClick();
                }
                state.pressStarted = false;

                if (gActiveButton == static_cast<void*>(&state))
                {
                    gActiveButton = nullptr;
                }
            }
        }

        ImU32 buttonColor = colorNormal;
        if (state.isDragging)
            buttonColor = colorActive;
        else if (hovered)
            buttonColor = colorHovered;
        
        drawList->AddRectFilled(buttonMin, buttonMax, buttonColor, 6.0f);
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos  = ImVec2(
            pos.x + (buttonSize.x - textSize.x) * 0.5f,
            pos.y + (buttonSize.y - textSize.y) * 0.5f
        );
        drawList->AddText(textPos, colorText, label);
    }
}


#endif /* DB_GUI_H */
