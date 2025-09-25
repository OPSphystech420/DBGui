#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <curl/curl.h>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "magic_enum/magic_enum.hpp"
#include <iomanip>
#include <utility>
#include <cctype>

enum class SQLInstruction { Select, Insert, Update, Delete, Call };

enum class TableName
{
    Users,
    ClientLogins,
    DatabaseConnections,
    ErrorLogs,
    Roles,
    UserActions,
    UserDevices,
    UserReviews,
    UserSettings
};

class CurlSqlClient : public Singleton<CurlSqlClient>
{
public:    
    rapidjson::Document SendSqlRequest(SQLInstruction Instruction, bool forcePost,
                                         std::initializer_list<std::pair<std::string, std::string>> paramsList)
    {
        std::string Params;
        bool first = true;
        for (const auto &p : paramsList)
        {
            if (!first)
                Params += "&";
            else
                first = false;
            Params += UrlEncode(p.first) + "=" + UrlEncode(p.second);
        }

        std::string Url;
        bool UsePost = false;
        switch (Instruction)
        {
            case SQLInstruction::Select:
                Url = "http://localhost:8080/select";
                
                if (forcePost || Params.length() > 1024)
                {
                    UsePost = true;
                }
                else if (!Params.empty())
                {
                    Url += "?" + Params;
                }
                break;
            case SQLInstruction::Insert:
                Url = "http://localhost:8080/insert";
                UsePost = true;
                break;
            case SQLInstruction::Update:
                Url = "http://localhost:8080/update";
                UsePost = true;
                break;
            case SQLInstruction::Delete:
                Url = "http://localhost:8080/delete";
                UsePost = true;
                break;
            case SQLInstruction::Call:
                Url = "http://localhost:8080/call";
                UsePost = true;
                break;
            default:
                throw std::invalid_argument("Unsupported SQLInstruction");
        }

        std::string Response = PerformCurlRequest(Url, UsePost, Params);

        rapidjson::Document Doc;
        Doc.Parse(Response.c_str());
        if (Doc.HasParseError())
        {
            std::cerr << "JSON parse error: "
                      << rapidjson::GetParseError_En(Doc.GetParseError())
                      << " (at offset " << Doc.GetErrorOffset() << ")" << std::endl;
        }
        return Doc;
    }

//    template<typename... Args>
//    rapidjson::Document SendSqlRequest(SQLInstruction Instruction, Args&&... ArgsPair)
//    {
//        static_assert(sizeof...(ArgsPair) % 2 == 0, "Parameters must be key-value pairs");
//
//        std::initializer_list<std::pair<std::string, std::string>> paramList = {
//            { ConvertToString(ArgsPair)... }
//        };
//        
//
//        return SendSqlRequest(Instruction, false, paramList);
//    }

    CurlSqlClient()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~CurlSqlClient()
    {
        curl_global_cleanup();
    }

private:
    std::string UrlEncode(const std::string &Value)
    {
        std::ostringstream Escaped;
        Escaped.fill('0');
        Escaped << std::hex;
        for (const char C : Value)
        {
            if (isalnum(static_cast<unsigned char>(C)) || C == '-' || C == '_' || C == '.' || C == '~')
            {
                Escaped << C;
            }
            else
            {
                Escaped << '%' << std::uppercase << std::setw(2)
                        << int(static_cast<unsigned char>(C)) << std::nouppercase;
            }
        }
        return Escaped.str();
    }

    std::string ConvertToString(const char* S)
    {
        return std::string(S);
    }

    std::string ConvertToString(const std::string &S)
    {
        return S;
    }

    template<typename T>
    std::string ConvertToString(const T &Value)
    {
        if constexpr (std::is_enum_v<T>)
        {
            return std::string(magic_enum::enum_name(Value));
        }
        else if constexpr (std::is_arithmetic_v<T>)
        {
            return std::to_string(Value);
        }
        else
        {
            std::ostringstream Oss;
            Oss << Value;
            return Oss.str();
        }
    }

    static size_t WriteCallback(void* Contents, size_t Size, size_t Nmemb, void* Userp)
    {
        size_t TotalSize = Size * Nmemb;
        std::string* Response = static_cast<std::string*>(Userp);
        Response->append(static_cast<char*>(Contents), TotalSize);
        return TotalSize;
    }

    std::string PerformCurlRequest(const std::string &Url, bool UsePost, const std::string &PostData)
    {
        std::string Response;
        CURL* Curl = curl_easy_init();
        if (Curl)
        {
            curl_easy_setopt(Curl, CURLOPT_URL, Url.c_str());
            curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Response);
            
            curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 10L);

            if (UsePost)
            {
                curl_easy_setopt(Curl, CURLOPT_POST, 1L);
                curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, PostData.c_str());
                
                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
                curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, headers);
            }

            CURLcode Res = curl_easy_perform(Curl);
            if (Res != CURLE_OK)
            {
                std::cerr << "Curl error: " << curl_easy_strerror(Res) << std::endl;
            }

            long httpCode = 0;
            curl_easy_getinfo(Curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode != 200)
            {
                std::cerr << "HTTP response code: " << httpCode << std::endl;
            }
            curl_easy_cleanup(Curl);
        }
        return Response;
    }
};
