//
//  DBManager.mm
//  DBGui
//
// Made by OPSphystech420 2025 (c)
//


#include "DBManager.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <CoreImage/CoreImage.h>

void GetImTextureViaURL(NSString* const urlString, ImTextureID& outTextureID)
{
    outTextureID = 0;

    NSURL* const url = [NSURL URLWithString:urlString];
    if (!url)
        return;

    __block ImTextureID* const blockTextureID = &outTextureID;

    NSURLSessionDataTask *downloadTask = [[NSURLSession sharedSession] dataTaskWithURL:url
                                                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error || !data)
            return;

        CIImage* const image = [CIImage imageWithData:data];
        id<MTLDevice> const device = MTLCreateSystemDefaultDevice();
        if (!image || !device)
            return;

        CIContext *ciContext = [CIContext contextWithOptions:nil];
        CGImageRef cgImage = [ciContext createCGImage:image fromRect:image.extent];
        if (!cgImage)
            return;

        MTKTextureLoader * const textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
        if (!textureLoader) {
            CGImageRelease(cgImage);
            return;
        }

        NSDictionary * const textureLoaderOptions = @{
            MTKTextureLoaderOptionSRGB : @NO,
            MTKTextureLoaderOptionTextureUsage : @(MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget),
            MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
        };

        NSError *textureError = nil;
        id<MTLTexture> const metalTexture = [textureLoader newTextureWithCGImage:cgImage
                                                                          options:textureLoaderOptions
                                                                            error:&textureError];
        CGImageRelease(cgImage);

        if (!metalTexture || textureError)
            return;

        ImTextureID imguiTextureID = (ImTextureID)CFBridgingRetain(metalTexture);

        dispatch_async(dispatch_get_main_queue(), ^{
            *blockTextureID = imguiTextureID;
        });
    }];

    [downloadTask resume];
}


struct ColorEntry {
    ImGuiCol idx;
    ImVec4 color;
    const char* name;
};

constexpr ColorEntry C0L0R[] = {
    { ImGuiCol_WindowBg,             DBGui::HexToColorVec4(0x303030, 1.0f),  "Window" },
    { ImGuiCol_PopupBg,              DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Popup" },

    { ImGuiCol_Text,                 DBGui::HexToColorVec4(0xFFFFFF, 1.0f),  "Text" },
    { ImGuiCol_CheckMark,            DBGui::HexToColorVec4(0xFFFFFF, 1.0f),  "Check Mark" },
    { ImGuiCol_TextDisabled,         DBGui::HexToColorVec4(0x828282, 1.0f),  "Text Disabled" },
    
    { ImGuiCol_SliderGrab,           DBGui::HexToColorVec4(0x81b1dd, 1.0f),  "Slider Grab" },
    { ImGuiCol_SliderGrabActive,     DBGui::HexToColorVec4(0x81b1dd, 1.0f),  "Slider Active" },
    
    { ImGuiCol_ScrollbarBg,          DBGui::HexToColorVec4(0x000000, 0.0f),  "Scrollbar Bg" },
    { ImGuiCol_ScrollbarGrab,        DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Scrollbar" },
    { ImGuiCol_ScrollbarGrabHovered, DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Scrollbar Hovered" },
    { ImGuiCol_ScrollbarGrabActive,  DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Scrollbar Active" },
    
    { ImGuiCol_Border,               DBGui::HexToColorVec4(0x81b1dd, 1.0f),  "Border" },
    
    { ImGuiCol_FrameBg,              DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Frame" },
    { ImGuiCol_FrameBgHovered,       DBGui::HexToColorVec4(0x1A1A1A, 0.7f),  "Frame Hovered" },
    { ImGuiCol_FrameBgActive,        DBGui::HexToColorVec4(0x1A1A1A, 0.5f),  "Frame Active" },
    
    { ImGuiCol_Button,               DBGui::HexToColorVec4(0x1A1A1A, 1.0f),  "Button" },
    { ImGuiCol_ButtonHovered,        DBGui::HexToColorVec4(0x1A1A1A, 0.7f),  "Button Hovered" },
    { ImGuiCol_ButtonActive,         DBGui::HexToColorVec4(0x1A1A1A, 0.5f),  "Button Active" },
    
    { ImGuiCol_Header,               DBGui::HexToColorVec4(0x2F2F2F, 1.0f),  "Header" },
    { ImGuiCol_HeaderHovered,        DBGui::HexToColorVec4(0x2F2F2F, 0.7f),  "Header Hovered" },
    { ImGuiCol_HeaderActive,         DBGui::HexToColorVec4(0x2F2F2F, 0.5f),  "Header Active" }
};

void DBManager::SetColors() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    for (const auto& entry : C0L0R)
        style.Colors[entry.idx] = entry.color;
}

void DBManager::LoadOnce() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4;
    style.ChildRounding     = 3;
    style.FrameRounding     = 2;
    style.GrabRounding      = 2;
    style.PopupRounding     = 2;
    style.TabRounding       = 2;
    
    style.ButtonTextAlign   = { 0.5f, 0.5f };
    style.WindowTitleAlign  = { 0.5f, 0.5f };
    style.FramePadding      = { 8.0f, 8.0f };
    style.WindowPadding     = { 10.0f, 10.0f };
    style.ItemSpacing       = style.WindowPadding;
    style.ItemInnerSpacing  = { style.WindowPadding.x, 4 };
    
    style.WindowBorderSize  = 0;
    style.FrameBorderSize   = 0;
    style.PopupBorderSize   = 0;
    
    //style.ScrollbarSize     = 18.f;
    style.GrabMinSize       = style.FrameRounding;
    
    SetColors();
    
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
}

ImTextureID menuLogo = 0;

#define IMAGE_URL @"https://raw.githubusercontent.com/OPSphystech420/DBGui/refs/heads/main/DBGUI.png"

void DBManager::ShowSqlDatabaseEditor()
{
    DBManager &DbManager = DBManager::GetInstance();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13.0f, 13.0f));
    ImGui::Begin("Database SQL Editor", nullptr, window_flags);
    ImGui::PopStyleVar();
    
    
    ImGui::BeginGroup(); {
        ImGui::InputText("Host", DbManager.HostBuffer, sizeof(DbManager.HostBuffer));
        ImGui::InputText("Username", DbManager.UsernameBuffer, sizeof(DbManager.UsernameBuffer));
        ImGui::InputText("Password", DbManager.PasswordBuffer, sizeof(DbManager.PasswordBuffer), ImGuiInputTextFlags_Password);
        ImGui::InputText("Database", DbManager.DatabaseBuffer, sizeof(DbManager.DatabaseBuffer));
        ImGui::InputText("Port", DbManager.PortBuffer, sizeof(DbManager.PortBuffer));
    }
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginChild("Logo", ImVec2(0, 180), ImGuiChildFlags_Border,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration); {
        if (menuLogo) {
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(190, 190)) * 0.5f);
            ImGui::Image(menuLogo, ImVec2(190, 190));

        }
        
        if (!menuLogo) {
            GetImTextureViaURL(
                               IMAGE_URL,
                               menuLogo
                               );
        }
    } ImGui::EndChild();

    
    if (DBGui::Button("Connect"))
    {
        DbManager.ConnectToDatabase();
    }
    ImGui::SameLine();
    if (DBGui::Button("Disconnect")) {
        DbManager.DisconnectFromDatabase();
    }
    ImGui::SameLine();
    ImGui::Text("%s", DbManager.ConnectionStatus);
    
    ImGui::Separator();
    
    static CTextEditor Editor;
    static bool EditorInitialized = false;
    if (!EditorInitialized)
    {
        Editor.SetLanguageDefinition(CTextEditor::LanguageDefinitionId::Sql);
        Editor.SetPalette(CTextEditor::PaletteId::Dark);
        Editor.SetText("-- SQL query\nSELECT * FROM your_table;\n");
        EditorInitialized = true;
    }
    

    
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::BeginGroup(); {
        static int currentPage = 0;
        const int rowsPerPage = 30;
        static int totalRows = 0;
        const int totalPages = (totalRows + rowsPerPage - 1) / rowsPerPage;
        
        if (totalPages > 1) {
            if (DBGui::Button("Previous") && currentPage > 0)
                currentPage--;
            ImGui::SameLine();
            if (DBGui::Button("Next") && currentPage < totalPages - 1)
                currentPage++;
            ImGui::SameLine();
            ImGui::Text("Page %d/%d", currentPage + 1, totalPages);
        }
    
        ImGui::BeginChild("Result", ImVec2((ImGui::GetWindowWidth() - style.ItemSpacing.x - style.WindowPadding.x * 2) / 3, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX); // ImGuiWindowFlags_MenuBar
        {
            if (DbManager.QueryFinished.load())
            {
                totalRows = 0;
                
                std::vector<std::string> Columns;
                std::vector<std::vector<std::string>> Rows;
                {
                    std::lock_guard<std::mutex> Lock(DbManager.QueryMutex);
                    Columns = DbManager.QueryColumns;
                    Rows = DbManager.QueryRows;
                }
                
                totalRows = static_cast<int>(Rows.size());

                if (currentPage >= totalPages)
                    currentPage = totalPages > 0 ? totalPages - 1 : 0;
                
                if (ImGui::BeginTable("ResultsTable", (int)Columns.size() + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                {
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
                    for (size_t i = 0; i < Columns.size(); i++)
                        ImGui::TableSetupColumn(Columns[i].c_str());
                    ImGui::TableHeadersRow();

                    const int startRow = currentPage * rowsPerPage;
                    const int endRow = std::min(startRow + rowsPerPage, totalRows);
                    for (int r = startRow; r < endRow; r++)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%d", r + 1);
                        for (size_t c = 0; c < Columns.size(); c++)
                        {
                            ImGui::TableSetColumnIndex((int)c + 1);
                            ImGui::TextWrapped("%s", Rows[r][c].c_str());
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();
    
        
    ImGui::SameLine();
    
    
    ImGui::BeginGroup(); {
        if (DbManager.QueryInProgress.load())
        {
            DBGui::Spinner("##spinoff", 7, 3, ImColor(255, 255, 0));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.8f, 0.1f, 1.0f));
            if (DBGui::Button(ICON_FA_PLAY))
            {
                
                if (!DbManager.IsConnected.load())
                {
                    snprintf(DbManager.ConnectionStatus, sizeof(DbManager.ConnectionStatus), "Not connected to database.");
                }
                else if (!DbManager.QueryInProgress.load())
                {
                    std::string QueryStr = Editor.GetText();
                    NSString *SqlQuery = [NSString stringWithUTF8String: QueryStr.c_str()];
                    DbManager.ExecuteSqlQueryAsync(SqlQuery);
                }
            } ImGui::PopStyleColor();
        } ImGui::SameLine();
        
        bool undoDisabled = !Editor.CanUndo();
        if (undoDisabled)
            ImGui::BeginDisabled();

        if (DBGui::Button(ICON_FA_ARROW_LEFT))
            Editor.Undo();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Undo (Ctrl+Z)");

        if (undoDisabled)
            ImGui::EndDisabled();

        ImGui::SameLine();

        bool redoDisabled = !Editor.CanRedo();
        if (redoDisabled)
            ImGui::BeginDisabled();

        if (DBGui::Button(ICON_FA_ARROW_RIGHT))
            Editor.Redo();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Redo (Ctrl+Y)");

        if (redoDisabled)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (DBGui::Button(ICON_FA_SCISSORS))
            Editor.Cut();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Cut (Ctrl+X)");

        ImGui::SameLine();

        if (DBGui::Button(ICON_FA_COPY))
            Editor.Copy();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Copy (Ctrl+C)");

        ImGui::SameLine();

        if (DBGui::Button(ICON_FA_PASTE))
            Editor.Paste();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Paste (Ctrl+V)");

        ImGui::SameLine();

        if (DBGui::Button(ICON_FA_FILE_LINES))
            Editor.SelectAll();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select All (Ctrl+A)");

        
        Editor.Render("SQL Editor", false, ImVec2(0, 0), true, true);
    }
    ImGui::EndGroup();
    
    ImGui::End();
}
