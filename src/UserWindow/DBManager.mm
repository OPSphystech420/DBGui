//
//  DBManager.mm
//  DBGui
//
// Made by OPSphystech420 2025 (c)
//

#include "DBManager.h"

#if TARGET_OS_OSX
    #import <CoreImage/CoreImage.h>
#endif

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>

#include "SqlCurl.hpp"

std::string GetDeviceIdentifier();
std::string GetPrimaryIPAddress(bool preferIPv6 = false);

// void InsertSelectAll(CTextEditor& ed, const std::string& table);

inline void HelpMarker(const char *desc) {
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.79f, 0.06f, 1.0f));
  ImGui::Text("(?)");
  ImGui::PopStyleColor();
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

void GetImTextureViaURL(NSString* const urlString, void*& outTextureID)
{
    outTextureID = nullptr;

    NSURL* const url = [NSURL URLWithString:urlString];
    if (!url)
        return;

    __block void** const blockTextureID = &outTextureID;

    NSURLSessionDataTask *downloadTask = [[NSURLSession sharedSession] dataTaskWithURL:url
                                                                     completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        if (error || !data)
            return;

#if TARGET_OS_OSX
        CIImage* const image = [CIImage imageWithData:data];
#else
        UIImage* const image = [UIImage imageWithData:data];
#endif
        id<MTLDevice> const device = MTLCreateSystemDefaultDevice();
        if (!image || !device)
            return;

#if TARGET_OS_OSX
        CIContext *ciContext = [CIContext contextWithOptions:nil];
        CGImageRef cgImage = [ciContext createCGImage:image fromRect:image.extent];
        if (!cgImage)
            return;

        MTKTextureLoader * const textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
        if (!textureLoader) {
            CGImageRelease(cgImage);
            return;
        }
#else
        MTKTextureLoader * const textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
        if (!textureLoader)
            return;
#endif

        NSDictionary * const textureLoaderOptions = @{
            MTKTextureLoaderOptionSRGB : @NO,
            MTKTextureLoaderOptionTextureUsage : @(MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget),
            MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
        };

        NSError *textureError = nil;

#if TARGET_OS_OSX
        id<MTLTexture> const metalTexture = [textureLoader newTextureWithCGImage:cgImage
                                                                          options:textureLoaderOptions
                                                                            error:&textureError];

        CGImageRelease(cgImage);
        
        if (!metalTexture || textureError)
            return;
        
        void* imguiTextureID = (void*)CFBridgingRetain(metalTexture);
#else
        id<MTLTexture> const metalTexture = [textureLoader newTextureWithCGImage:image.CGImage
                                                                          options:textureLoaderOptions
                                                                            error:&textureError];

        if (!metalTexture || textureError)
            return;

        void* const imguiTextureID = (__bridge_retained void *)(metalTexture);
#endif

        dispatch_async(dispatch_get_main_queue(), ^{
            *blockTextureID = imguiTextureID;
        });
    }];

    [downloadTask resume];
}

void GetImTextureViaFile(NSString* filePath, void*& outTextureID)
{
    outTextureID = nullptr;

    NSData *fileData = [NSData dataWithContentsOfFile:filePath];
    if (!fileData) {
        NSLog(@"Error: Could not load file data from %@", filePath);
        return;
    }

#if TARGET_OS_OSX
    CIImage* const image = [CIImage imageWithData:fileData];
#else
    UIImage* const image = [UIImage imageWithData:fileData];
#endif
    if (!image)
    {
        NSLog(@"Error: Could not create image from file data.");
        return;
    }
    
    // Create a Metal device.
    id<MTLDevice> const device = MTLCreateSystemDefaultDevice();
    if (!device)
    {
        NSLog(@"Error: Could not create Metal device.");
        return;
    }
    
    // Create the Metal texture loader.
    MTKTextureLoader* const textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
    if (!textureLoader)
    {
        NSLog(@"Error: Could not create MTKTextureLoader.");
        return;
    }
    
    // Set loader options.
    NSDictionary * const textureLoaderOptions = @{
        MTKTextureLoaderOptionSRGB : @NO,
        MTKTextureLoaderOptionTextureUsage : @(MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget),
        MTKTextureLoaderOptionTextureStorageMode : @(MTLStorageModePrivate)
    };
    
    NSError *textureError = nil;
    
#if TARGET_OS_OSX
    // On macOS, create a CGImage from the CIImage using a CIContext.
    CIContext *ciContext = [CIContext contextWithOptions:nil];
    CGImageRef cgImage = [ciContext createCGImage:image fromRect:image.extent];
    if (!cgImage)
    {
        NSLog(@"Error: Could not create CGImage from CIImage.");
        return;
    }
    
    // Create the Metal texture using the CGImage.
    id<MTLTexture> const metalTexture = [textureLoader newTextureWithCGImage:cgImage
                                                                      options:textureLoaderOptions
                                                                        error:&textureError];
    // Release the CGImage when done.
    CGImageRelease(cgImage);
    
    if (!metalTexture || textureError)
    {
        NSLog(@"Error: Could not create Metal texture (%@).", textureError);
        return;
    }
    
    void* const imguiTextureID = (void*)CFBridgingRetain(metalTexture);
#else
    id<MTLTexture> const metalTexture = [textureLoader newTextureWithCGImage:((UIImage *)image).CGImage
                                                                      options:textureLoaderOptions
                                                                        error:&textureError];
    if (!metalTexture || textureError)
    {
        NSLog(@"Error: Could not create Metal texture (%@).", textureError);
        return;
    }
    
    void* const imguiTextureID = (__bridge_retained void *)(metalTexture);
#endif
    
    outTextureID = imguiTextureID;
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
    
    // extra coloring    ....
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
    
    SetColors();
    
    style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
}

void* menuLogo = 0;

#define IMAGE_URL @"https://raw.githubusercontent.com/OPSphystech420/DBGui/refs/heads/main/images/_DBGUI.png"

inline void ShowLogo(){
    ImGui::BeginChild("Logo", ImVec2(0, 180), ImGuiChildFlags_Border,
                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration); {
        if (menuLogo)
        {
            ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(180, 170)) * 0.5f);
            ImGui::Image((ImTextureID)menuLogo, ImVec2(180, 170));

        }
        
        if (!menuLogo)
        {
            /* GetImTextureViaURL(
                   IMAGE_URL,
                   menuLogo
            ); */
            
            NSString* IMAGE_PATH = [[NSBundle mainBundle] pathForResource:@"_DBGUI" ofType:@"png"];
            
            if (!IMAGE_PATH)
            {
                NSLog(@"Could not locate IMAGE_PATH in app bundle");
                abort();
            }
            
            GetImTextureViaFile(
                   IMAGE_PATH,
                   menuLogo
            );
        }
        
    } ImGui::EndChild();
}

void DBManager::ShowSqlDatabaseEditor()
{
//    if (AuthStatus != AuthState::LoggedIn) {
//        ShowAuthWindow();
//        return;
//    }
    
    DBManager& self = *this;
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar;
    
    static bool wasConnected = false;
    bool nowConnected = IsConnected.load();
    if (nowConnected != wasConnected) {
        ImGui::SetNextWindowSize(ImVec2(nowConnected ? 1100.f : 900.f, 600), ImGuiCond_Always);
        wasConnected = nowConnected;
    } else ImGui::SetNextWindowSize(ImVec2(900.f, 600), ImGuiCond_Once);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13.0f, 13.0f));
    ImGui::Begin("Database SQL Editor", nullptr, window_flags);
    ImGui::PopStyleVar();
    
    if (IsConnected.load()) {
        ImGui::BeginChild("SchemaSidebar", ImVec2(220.0f, 0),
                          ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX); {
            
            if (self.IsConnected.load())
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4,4));
                if (ImGui::CollapsingHeader(self.DatabaseBuffer,
                                            ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (self.TablesReady.load())
                    {
                        std::vector<std::string> tables;
                        {
                            std::lock_guard<std::mutex> lock(self.TablesMutex);
                            tables = self.TableNames;
                        }
                        
                        for (const auto& t : tables)
                        {
                            bool busy = self.QueryInProgress.load();
                            if (busy) ImGui::BeginDisabled();

                            if (ImGui::Selectable(t.c_str(), false))
                                self.RunQueryAsync("SELECT * FROM `" + t + "`;"); //  LIMIT 100

                            if (busy) ImGui::EndDisabled();
                        }
                    }
                    else
                    {
                        // ImGui::TextUnformatted("Loading");
                        RefreshTableListAsync();
                    }
                }
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::TextUnformatted("Not connected.");
            }
            
        } ImGui::EndChild();
        ImGui::SameLine();
    }
    ImGui::BeginGroup(); {
        
        static int activeTab = 1;
        DBGui::RadioFrameIcon("Set Connection", ICON_FA_DATABASE, FiraCode, &activeTab, 1, ImVec2(0,0)); ImGui::SameLine();
        DBGui::RadioFrameIcon("Settings", ICON_FA_GEAR, FiraCode, &activeTab, 2, ImVec2(0,0));
       // ImGui::Separator();
        
        ImGui::SameLine();
        
        ImGui::SetCursorPosX( ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(AuthMsg).x - 10);
        
        ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "%s", AuthMsg);
        
        if (activeTab == 1 || activeTab == 2)
        {
            DBGui::SeparatorText(ICON_FA_WINDOW_RESTORE);
            
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked())
                activeTab = 0;
        } else {
            ImGui::Separator();
        }
        
        if (activeTab == 2)
        {
            ImGui::PushItemWidth(300);
            if (DBGui::Button("Logout"))
            {
                AuthStatus = AuthState::LoggedOut;
                CurrentUserId = 0;
                memset(AuthMsg, 0, sizeof(AuthMsg));
            } ImGui::SameLine();
            
            if (DBGui::Button("Change Role"))
            {
                bChangedRole = true;
                ImGui::OpenPopup("ChangeRole");
            }
            
            
            if (ImGui::BeginPopupModal("ChangeRole", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (DBGui::Button("Community User", ImVec2(200, 0))) {
                    auto doc = CurlSqlClient::GetInstance().SendSqlRequest(
                                                                           SQLInstruction::Update, true, {
                                                                               {"table",      "user_roles"},
                                                                               {"role_id",    "3"},
                                                                               {"where",      "user_id=" + std::to_string(CurrentUserId)}
                                                                           });
                    
                    if (doc.HasMember("rows_affected") && doc["rows_affected"].IsInt()) {
                        UserRole = UserRoles::Community;
                        ImGui::CloseCurrentPopup();
                    } else bChangedRole = false;
                } ImGui::SameLine(); HelpMarker(
                                                "You will be granted with access to review your exprience. \n"
                                                "All your actions, including database connections and \n"
                                                "application errors will be logged for private gathering \n"
                                                "to improve this product!"
                                                );
                if (DBGui::Button("Beta Tester", ImVec2(200, 0))) {
                    auto doc = CurlSqlClient::GetInstance().SendSqlRequest(
                                                                           SQLInstruction::Update, true, {
                                                                               {"table",      "user_roles"},
                                                                               {"role_id",    "2"},
                                                                               {"where",      "user_id=" + std::to_string(CurrentUserId)}
                                                                           });
                    
                    if (doc.HasMember("rows_affected") && doc["rows_affected"].IsInt()) {
                        UserRole = UserRoles::Beta;
                        ImGui::CloseCurrentPopup();
                    } else bChangedRole = false;
                } ImGui::SameLine(); HelpMarker(
                                                "Same content and conditions as community user, \n"
                                                "but access to the latest beta functionality, \n"
                                                "which may expose worser experiance for daily \n"
                                                "usage with overhead of program errors");
                // if (DBGui::Button("Administrator", ImVec2(200, 0))) {  }
                if (DBGui::Button("Cancel", ImVec2(200, 0)))
                    ImGui::CloseCurrentPopup();
                
                if (!bChangedRole) ImGui::Text("Role is currently not avaliable.");
                ImGui::EndPopup();
            }
            
            ImGui::SameLine();
            
            switch (UserRole) {
                case UserRoles::Undefined:
                    ImGui::Text("Your role is not set!");
                    break;
                case UserRoles::Beta:
                    ImGui::Text("Your role is %s User!", magic_enum::enum_name(UserRole).data());
                    break;
                case UserRoles::Community:
                    ImGui::Text("Your role is %s User!", magic_enum::enum_name(UserRole).data());
                    break;
                case UserRoles::Admin:
                    ImGui::Text("Your role is %s!", magic_enum::enum_name(UserRole).data());
                    break;
                default:
                    break;
            }
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            ShowLogo();
            
            
            ImGui::Separator();
        }
        
        if (activeTab == 1)
        {
            ImGui::BeginGroup(); {
                ImGui::PushItemWidth(300);
                ImGui::InputText("Host", self.HostBuffer, sizeof(self.HostBuffer));
                ImGui::InputText("Username", self.UsernameBuffer, sizeof(self.UsernameBuffer));
                ImGui::InputText("Password", self.PasswordBuffer, sizeof(self.PasswordBuffer), ImGuiInputTextFlags_Password);
                ImGui::InputText("Database", self.DatabaseBuffer, sizeof(self.DatabaseBuffer));
                ImGui::InputText("Port", self.PortBuffer, sizeof(self.PortBuffer));
                ImGui::PopItemWidth();
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ShowLogo();
            
            
            if (DBGui::Button("Connect"))
            {
                self.ConnectToDatabase();
            }
            ImGui::SameLine();
            if (DBGui::Button("Disconnect")) {
                self.DisconnectFromDatabase();
            }
            ImGui::SameLine();
            ImGui::Text("%s", self.ConnectionStatus);
            ImGui::Separator();
        }
        
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
            int totalPages = (totalRows + rowsPerPage - 1) / rowsPerPage;
            
            if (totalPages > 1) {
                if (DBGui::Button("Previous") && currentPage > 0)
                    currentPage--;
                ImGui::SameLine();
                if (DBGui::Button("Next") && currentPage < totalPages - 1)
                    currentPage++;
                ImGui::SameLine();
                ImGui::Text("Page %d/%d", currentPage + 1, totalPages);
            }
            
            ImGui::BeginChild("Result", ImVec2(2 * (ImGui::GetWindowWidth() - style.ItemSpacing.x - style.WindowPadding.x * 2) / 5, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX); // ImGuiWindowFlags_MenuBar
            {
                if (self.QueryFinished.load())
                {
                    totalRows = 0;
                    
                    std::vector<std::string> Columns;
                    std::vector<std::vector<std::string>> Rows;
                    {
                        std::lock_guard<std::mutex> Lock(self.QueryMutex);
                        Columns = self.QueryColumns;
                        Rows = self.QueryRows;
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
            if (self.QueryInProgress.load())
            {
                DBGui::Spinner("##spinoff", 7, 3, ImColor(255, 255, 0));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.8f, 0.1f, 1.0f));
                if (DBGui::Button(ICON_FA_PLAY))
                {
                    
                    if (!self.IsConnected.load())
                    {
                        snprintf(self.ConnectionStatus, sizeof(self.ConnectionStatus), "Not connected to database.");
                    }
                    else if (!self.QueryInProgress.load())
                    {
                        std::string QueryStr = Editor.GetText();
                        NSString *SqlQuery = [NSString stringWithUTF8String: QueryStr.c_str()];
                        self.ExecuteSqlQueryAsync(SqlQuery);
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
            
            ImGui::PushFont(FiraCode);
            Editor.Render("SQL Editor", false, ImVec2(0, 0), true, true);
            ImGui::PopFont();
            
        } ImGui::EndGroup();
    } ImGui::EndGroup();
    ImGui::End();
}

// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /

//void InsertSelectAll(CTextEditor& ed, const std::string& table)
//{
//    std::string newText = ed.GetText();
//    newText += "SELECT * FROM `" + table + "`;\n";
//    ed.SetText(newText);
//}

void DBManager::RunQueryAsync(const std::string& sql)
{
    NSString *ns = [NSString stringWithUTF8String:sql.c_str()];
    ExecuteSqlQueryAsync(ns);
}

void DBManager::RefreshTableListAsync()
{
    if (TablesLoading.exchange(true))
           return;
    
    TablesReady.store(false);
    std::thread t(&DBManager::RefreshTableListWorker, this);
    t.detach();
}

void DBManager::RefreshTableListWorker()
{
    @autoreleasepool {
        if (!IsConnected.load() || Client == nil)
            return;

        NSError  *Error = nil;
        NSString *sql   = @"SHOW TABLES";
        MariaDBResultSet *rs = [Client executeQuery:sql error:&Error];

        std::vector<std::string> tables;
        if (rs != nil)
        {
            while ([rs next:&Error])
            {
                id obj = [rs objectForColumnIndex:0];
                if (obj && obj != [NSNull null])
                    tables.emplace_back([[obj description] UTF8String]);
            }
        }

        {
            std::lock_guard<std::mutex> lock(TablesMutex);
            TableNames = std::move(tables);
        }
        TablesReady.store(true);
        TablesLoading.store(false);
    }
}





// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /
// // / / // / /// // / / // / / // // / / // / / // // / / // / /

namespace {
    static void trim(std::string &s) {
        auto not_space = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    }

    static bool isValidChars(const std::string &s) {
        static const std::string allowed = R"(!@#$%^&*()_+\-=\[\]{};':"\\|,.<>\/?`~)";
        for (char c : s) {
            if (std::isalpha(c) || std::isdigit(c) || allowed.find(c) != std::string::npos)
                continue;
            return false;
        }
        return true;
    }
}

int CurrentAuthTab = 3;


void DBManager::ShowAuthWindow()
{
    ImVec2 vp = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos((vp - ImVec2(400,620)) * 0.5f, ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400,620), ImGuiCond_Once);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f,0.12f,0.12f,0.95f));

    if (ImGui::Begin("User Authentication", nullptr,
                     ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse))
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ShowLogo();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20,8));
        if (DBGui::RadioButton("Register", &CurrentAuthTab, 0, ImVec2(113, 0))) {
            if (CurrentAuthTab == 1) AuthMsg[0] = '\0';
            CurrentAuthTab = 0;
        }
        ImGui::SameLine();
        if (DBGui::RadioButton("Login", &CurrentAuthTab, 1, ImVec2(113, 0))) {
            if (CurrentAuthTab == 0) AuthMsg[0] = '\0';
            CurrentAuthTab = 1;
        }
        ImGui::SameLine();
        if (DBGui::RadioButton("About", &CurrentAuthTab, 2, ImVec2(113, 0))) {
            AuthMsg[0] = '\0';
            CurrentAuthTab = 2;
        }
        ImGui::PopStyleVar();
        ImGui::Spacing();

        if (CurrentAuthTab == 0)
            ShowRegisterTab();
        else if (CurrentAuthTab == 1)
            ShowLoginTab();
        else
            ImGui::Text(
                    "\n\n\n\n\n\n                   Welcome to DBGUI! \n\n\n"
                    "     Officialy made by OPSphystech420, introduces\n"
                    "     an example project of database managment tool."
                        );

        ImGui::End();
    }
    else
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::End();
    }
}

void DBManager::ShowRegisterTab()
{
    ImGui::PushItemWidth(-1);
    DBGui::InputText("Username", "Enter username...", LoginUsername, sizeof(LoginUsername)); // RegUsername
    DBGui::InputText("Email",    "you@example.com",    RegEmail,    sizeof(RegEmail));
    DBGui::InputText("Password", "********",           LoginPassword, sizeof(LoginPassword), 0.0, // RegPassword
                     ImGuiInputTextFlags_Password);
    DBGui::InputText("Full Name","First Last",         RegFullName, sizeof(RegFullName));
    ImGui::PopItemWidth();
    
    ImGui::Spacing();

    if (DBGui::Button("Create Account", ImVec2(-1, 0)) && RegisterUser())
    {
        bChangedRole = true;
        ImGui::OpenPopup("RegSuccess");
    }

    if (ImGui::BeginPopupModal("RegSuccess", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (AuthMsg[0] != '\0') {
            ImGui::TextWrapped("%s", AuthMsg);
            ImGui::Separator();
            ImGui::Spacing();
            if (DBGui::Button("OK", ImVec2(300, 0))) AuthMsg[0] = '\0';
            
        } else {
            ImGui::Text("You may choose your role for this Project!");
            if (DBGui::Button("Community User", ImVec2(200, 0))) {
                auto doc = CurlSqlClient::GetInstance().SendSqlRequest(
                    SQLInstruction::Update, true, {
                        {"table",      "user_roles"},
                        {"role_id",    "3"},
                        {"where",      "user_id=" + std::to_string(CurrentUserId)}
                });

                if (doc.HasMember("rows_affected") && doc["rows_affected"].IsInt()) {
                    CurrentAuthTab = 1;
                    ImGui::CloseCurrentPopup();
                } else bChangedRole = false;
            } ImGui::SameLine(); HelpMarker(
                         "You will be granted with access to review your exprience. \n"
                         "All your actions, including database connections and \n"
                         "application errors will be logged for private gathering \n"
                         "to improve this product!"
                         );
            if (DBGui::Button("Beta Tester", ImVec2(200, 0))) {
                auto doc = CurlSqlClient::GetInstance().SendSqlRequest(
                    SQLInstruction::Update, true, {
                        {"table",      "user_roles"},
                        {"role_id",    "2"},
                        {"where",      "user_id=" + std::to_string(CurrentUserId)}
                });
                
                if (doc.HasMember("rows_affected") && doc["rows_affected"].IsInt()) {
                    CurrentAuthTab = 1;
                    ImGui::CloseCurrentPopup();
                } else bChangedRole = false;
            } ImGui::SameLine(); HelpMarker(
                         "Same content and conditions as community user, \n"
                         "but access to the latest beta functionality, \n"
                         "which may expose worser experiance for daily \n"
                         "usage with overhead of program errors");
            // if (DBGui::Button("Administrator", ImVec2(200, 0))) {  }
            if (DBGui::Button("I don't need a role!", ImVec2(200, 0))) {
                CurrentAuthTab = 1;
                ImGui::CloseCurrentPopup();
            }
            
            if (!bChangedRole) ImGui::Text("Role is currently not avaliable.");
        }
        ImGui::EndPopup();
    }
    
    ImGui::Spacing();
    if (AuthStatus != AuthState::LoggedIn && AuthMsg[0] != '\0' && !CurrentUserId) ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "%s", AuthMsg);
}

void DBManager::ShowLoginTab()
{
    ImGui::PushItemWidth(-1);
    DBGui::InputText("Username", "Enter username...", LoginUsername, sizeof(LoginUsername));
    DBGui::InputText("Password", "********", LoginPassword, sizeof(LoginPassword), 0.0, ImGuiInputTextFlags_Password);
    
    ImGui::PopItemWidth();

    ImGui::Spacing();
    if (DBGui::Button("Sign In", ImVec2(-1, 0)) && LoginUser())
    {
        // ?
    }
    
    ImGui::Spacing();
    if (AuthStatus != AuthState::LoggedIn && AuthMsg[0] != '\0') ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "%s", AuthMsg);
}


// // / / // / /// // / / // / / // // / / // / / // // / / // / /

bool DBManager::RegisterUser()
{
    std::string user(LoginUsername), email(RegEmail), // RegUsername
                pass(LoginPassword), fullname(RegFullName); // RegPassword
    trim(user); trim(email); trim(pass); trim(fullname);
    
    if (user.empty() || email.empty() || pass.empty() || fullname.empty()) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Please input your data");
        return false;
    }
    if (user.size() > 50) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Username must be at most 50 characters");
        return false;
    }
    if (!std::all_of(user.begin(), user.end(), ::isalnum)) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Username may only contain English letters and digits");
        return false;
    }
    if (email.size() > 100) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Email must be at most 100 characters");
        return false;
    }
    static const boost::regex email_re(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"
    );
    if (!boost::regex_match(email, email_re)) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Email format is invalid");
        return false;
    }

    if (pass.size() > 255) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Password too long (%zu > 255)", pass.size());
        return false;
    }
    
    CurlSqlClient& cli = CurlSqlClient::GetInstance();
    auto doc = cli.SendSqlRequest(SQLInstruction::Call, false, {
        {"routine",     "dbgui.sp_user_register"},
        {"p_username",  user},
        {"p_email",     email},
        {"p_password",  pass},
        {"p_full_name", fullname}
    });

    if (doc.IsArray() && !doc.Empty()) {
        const auto& row = doc[0];

        if (row.HasMember("status") && std::strcmp(row["status"].GetString(), "ok") == 0) {
            int uid = row["user_id"].GetInt();          // >0 guaranteed
            snprintf(AuthMsg, sizeof(AuthMsg),
                     "Registration successful!");
            CurrentUserId = uid;
            return true;
        }
        if (row.HasMember("error")) {
            const char* err = row["error"].GetString();
            if (std::strcmp(err, "username_exists") == 0)
                snprintf(AuthMsg, sizeof(AuthMsg), "Username already taken");
            else if (std::strcmp(err, "email_exists") == 0)
                snprintf(AuthMsg, sizeof(AuthMsg), "Email already registered");
            else
                snprintf(AuthMsg, sizeof(AuthMsg), "Registration failed!");
            return false;
        }
    }

    snprintf(AuthMsg, sizeof(AuthMsg), "Registration failed!");
    return false;
}

bool DBManager::LoginUser()
{
    std::string user(LoginUsername), pass(LoginPassword);
    trim(user); trim(pass);

    if (user.empty() || pass.empty()) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Please input your data");
        return false;
    }
    if (user.size() > 50 || pass.size() > 255) {
        std::snprintf(AuthMsg, sizeof(AuthMsg),
                      "Username or password too long");
        return false;
    }
    
    CurlSqlClient& cli = CurlSqlClient::GetInstance();

    std::string DeviceName = "MacBook-Pro 16";
    std::string IpAddress = "ip123";
    
    auto doc = cli.SendSqlRequest(SQLInstruction::Call, false, {
        {"routine",      "dbgui.sp_user_login"},
        {"p_username",     LoginUsername},
        {"p_password",     LoginPassword},
        {"p_device_name",  GetDeviceIdentifier()},
        {"p_ip_address",   GetPrimaryIPAddress()}
    });
    
    if (doc.IsArray() && !doc.Empty() && doc[0].HasMember("user_id"))
    {
        int uid = doc[0]["user_id"].GetInt();
        
        auto& roleNode = doc[0]["role_id"];
        if (!roleNode.IsNull()) {
            switch (roleNode.GetInt()) {
                case 1:
                    UserRole = UserRoles::Admin;
                    break;
                case 2:
                    UserRole = UserRoles::Beta;
                    break;
                case 3:
                    UserRole = UserRoles::Community;
                    break;
                case 4:
                    UserRole = UserRoles::Undefined;
                    break;
                default:
                    break;
            }
        }
        
        if (uid > 0)
        {
            CurrentUserId = uid;
            AuthStatus    = AuthState::LoggedIn;
            snprintf(AuthMsg, sizeof(AuthMsg), "Welcome back, %s!", LoginUsername);
            return true;
        }
    }
    snprintf(AuthMsg, sizeof(AuthMsg), "Invalid credentials");
    return false;
}


// // / / // / /// // / / // / / // // / / // / / // // / / // / / IP/Device grabber

#include <ifaddrs.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

inline std::string GetDeviceIdentifier()
{
#ifdef __APPLE__
#   if TARGET_OS_OSX
        char model[256] = {};
        size_t len = sizeof(model);
        if (sysctlbyname("hw.model", model, &len, nullptr, 0) == 0)
            return std::string(model, len - 1);
#   endif
        struct utsname uts{};
        if (uname(&uts) == 0)
            return std::string(uts.machine);
#endif
    return "unknown";
}

inline std::string GetPrimaryIPAddress(bool preferIPv6)
{
    std::string ip = "0.?.0.0:????";
    struct ifaddrs *ifaddr = nullptr;

    if (getifaddrs(&ifaddr) != 0)
        return ip;

    int familyWanted = preferIPv6 ? AF_INET6 : AF_INET;

    for (auto *ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr) continue;
        int fam = ifa->ifa_addr->sa_family;

        if (strcmp(ifa->ifa_name, "lo0") == 0) continue;
        if (fam != familyWanted) continue;

        char host[NI_MAXHOST] = {};
        if (getnameinfo(ifa->ifa_addr,
                        (fam == AF_INET)  ? sizeof(sockaddr_in)
                                          : sizeof(sockaddr_in6),
                        host, sizeof(host),
                        nullptr, 0, NI_NUMERICHOST) == 0)
        {
            ip = host;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}
