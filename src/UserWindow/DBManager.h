//
//  DBManager.h
//  DBGui
//
// Made by OPSphystech420 2025 (c)
//

#pragma once

#import <Foundation/Foundation.h>
#import <MariaDBKit/MariaDBKit.h>

#include <openssl/sha.h>

#include "imgui.h"
#include "Fonts.h"
#include "CTextEditor.h"
#include "DBGUI.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <cstring>

class DBManager {
public:
    static DBManager& GetInstance() {
        static DBManager Instance;
        return Instance;
    }
    
    void ShowSqlDatabaseEditor();
    
    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;
    
    char HostBuffer[128];
    char UsernameBuffer[64];
    char PasswordBuffer[64];
    char DatabaseBuffer[64];
    char PortBuffer[6];

    char ConnectionStatus[256];
    
    std::mutex QueryMutex;
    std::atomic_bool QueryInProgress;
    std::atomic_bool QueryFinished;
    std::vector<std::string> QueryColumns;
    std::vector<std::vector<std::string>> QueryRows;
    
    MariaDBClient *Client;
    std::atomic_bool IsConnected;
    
    void ConnectToDatabase() {
        if (IsConnected.load()) {
            std::string CurrentHost(HostBuffer);
            std::string CurrentUsername(UsernameBuffer);
            std::string CurrentDatabase(DatabaseBuffer);
            std::string CurrentPort(PortBuffer);
            if (CurrentHost != OldHost || CurrentUsername != OldUsername ||
                CurrentDatabase != OldDatabase || CurrentPort != OldPort)
            {
                DisconnectFromDatabase();
            }
            else {
                snprintf(ConnectionStatus, sizeof(ConnectionStatus), "Already connected.");
                return;
            }
        }
        
        OldHost = HostBuffer;
        OldUsername = UsernameBuffer;
        OldDatabase = DatabaseBuffer;
        OldPort = PortBuffer;
        
        snprintf(ConnectionStatus, sizeof(ConnectionStatus), "Connecting to the database...");
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            @autoreleasepool {
                NSError *Error = nil;
                NSString *Host     = [NSString stringWithUTF8String: HostBuffer];
                NSString *Username = [NSString stringWithUTF8String: UsernameBuffer];
                NSString *Password = [NSString stringWithUTF8String: PasswordBuffer];
                NSString *Database = [NSString stringWithUTF8String: DatabaseBuffer];
                NSUInteger Port    = (NSUInteger)atoi(PortBuffer);
                
                if (!IsConnected.load()) {
                    if (!Client) {
                        Client = [[MariaDBClient alloc] init];
                    }
                    BOOL Connected = [Client connect:Host
                                            username:Username
                                            password:Password
                                            database:Database
                                                port:Port
                                               error:&Error];
                    IsConnected.store(Connected);
                    if (Connected) {
                        snprintf(ConnectionStatus, sizeof(ConnectionStatus), "Connected successfully!");
                    } else {
                        snprintf(ConnectionStatus, sizeof(ConnectionStatus), "Connect error: %s", Error ? [[Error localizedDescription] UTF8String] : "Unknown");
                    }
                }
            }
        });
    }
    
    void DisconnectFromDatabase() {
        if (Client != nil) {
            Client = nil;
        }
        IsConnected.store(false);
        snprintf(ConnectionStatus, sizeof(ConnectionStatus), "Disconnected from database.");
    }
    
    void ExecuteSqlQueryAsync(NSString *SqlQuery) {
        QueryInProgress.store(true);
        QueryFinished.store(false);
        std::thread QueryThread(&DBManager::ExecuteQueryThread, this, SqlQuery);
        QueryThread.detach();
    }
    
private:
    void LoadOnce();
    void SetColors();
    
    std::string OldHost;
    std::string OldUsername;
    std::string OldDatabase;
    std::string OldPort;
    
    DBManager()
    {
        LoadOnce();
        
        strcpy(HostBuffer, "localhost");
        strcpy(UsernameBuffer, "username");
        strcpy(PasswordBuffer, "");
        strcpy(DatabaseBuffer, "database");
        strcpy(PortBuffer, "3306");
        
        ConnectionStatus[0] = '\0';
        
        Client = nil;
        
        IsConnected.store(false);
        QueryInProgress.store(false);
        QueryFinished.store(false);
    }
    
    void ExecuteQueryThread(NSString *SqlQuery) {
        @autoreleasepool {
            NSError *Error = nil;
            NSString *Host     = [NSString stringWithUTF8String: HostBuffer];
            NSString *Username = [NSString stringWithUTF8String: UsernameBuffer];
            NSString *Password = [NSString stringWithUTF8String: PasswordBuffer];
            NSString *Database = [NSString stringWithUTF8String: DatabaseBuffer];
            NSUInteger Port    = (NSUInteger)atoi(PortBuffer);
            
            if (!IsConnected.load()) {
                if (!Client) {
                    Client = [[MariaDBClient alloc] init];
                }
                BOOL Connected = [Client connect:Host
                                        username:Username
                                        password:Password
                                        database:Database
                                            port:Port
                                           error:&Error];
                
                IsConnected.store(Connected);
                if (!Connected) {
                    NSLog(@"Connection failed: %@", [Error localizedDescription]);
                }
            }
            
            MariaDBResultSet *ResultSet = [Client executeQuery:SqlQuery error:&Error];
            std::vector<std::vector<std::string>> Rows;
            std::vector<std::string> Columns;
            
            if (ResultSet != nil) {
                NSArray *ColNames = ResultSet.columnNames;
                for (NSString *Col in ColNames) {
                    Columns.push_back(std::string([Col UTF8String]));
                }
                
                while ([ResultSet next:&Error]) {
                    std::vector<std::string> Row;
                    for (NSUInteger i = 0; i < ColNames.count; i++) {
                        id Obj = [ResultSet objectForColumnIndex:i];
                        NSString *Str = (Obj == [NSNull null]) ? @"NULL" : [Obj description];
                        Row.push_back(std::string([Str UTF8String]));
                    }
                    Rows.push_back(Row);
                }
            } else {
                Columns.push_back("Error");
                std::string ErrStr = Error ? std::string([[Error localizedDescription] UTF8String]) : "Unknown error";
                Rows.push_back({ ErrStr });
            }
            
            {
                std::lock_guard<std::mutex> Lock(QueryMutex);
                QueryColumns = Columns;
                QueryRows = Rows;
                QueryFinished.store(true);
            }
            QueryInProgress.store(false);
        }
    }
};
