//=============================================================================//
//
// Purpose: Implementation of the pylon server backend.
//
// $NoKeywords: $
//=============================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <tier2/curlutils.h>
#include <networksystem/pylon.h>
#include <engine/server/server.h>

//-----------------------------------------------------------------------------
// Purpose: checks if the server listing fields are valid.
// Input  : &value - 
// Output : true on success, false on failure.
//-----------------------------------------------------------------------------
static bool IsServerListingValid(const rapidjson::Value& value)
{
    if (value.HasMember("name")        && value["name"].IsString()        &&
        value.HasMember("description") && value["description"].IsString() &&
        value.HasMember("hidden")      && value["hidden"].IsString()      && // TODO: Bool???
        value.HasMember("map")         && value["map"].IsString()         &&
        value.HasMember("playlist")    && value["playlist"].IsString()    &&
        value.HasMember("ip")          && value["ip"].IsString()          &&
        value.HasMember("port")        && value["port"].IsString()        && // TODO: Int32???
        value.HasMember("key")         && value["key"].IsString()         &&
        value.HasMember("checksum")    && value["checksum"].IsString()    && // TODO: Uint32???
        value.HasMember("playerCount") && value["playerCount"].IsString() && // TODO: Int32???
        value.HasMember("maxPlayers")  && value["maxPlayers"].IsString())//  && // TODO: Int32???
    {
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: gets a vector of hosted servers.
// Input  : &outMessage - 
// Output : vector<NetGameServer_t>
//-----------------------------------------------------------------------------
vector<NetGameServer_t> CPylon::GetServerList(string& outMessage) const
{
    vector<NetGameServer_t> vecServers;

    rapidjson::Document requestJson;
    requestJson.SetObject();
    requestJson.AddMember("version", SDK_VERSION, requestJson.GetAllocator());

    rapidjson::StringBuffer stringBuffer;
    JSON_DocumentToBufferDeserialize(requestJson, stringBuffer);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/servers", requestJson, responseJson,
        outMessage, status, "server list error"))
    {
        return vecServers;
    }

    if (!responseJson.HasMember("servers"))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return vecServers;
    }

    const rapidjson::Value& servers = responseJson["servers"];

    for (rapidjson::Value::ConstValueIterator itr = servers.Begin();
        itr != servers.End(); ++itr)
    {
        const rapidjson::Value& obj = *itr;

        if (!IsServerListingValid(obj))
        {
            // Missing details; skip this server listing.
            continue;
        }

        vecServers.push_back(
            NetGameServer_t
            {
                obj["name"].GetString(),
                obj["description"].GetString(),
                V_strcmp(obj["hidden"].GetString(), "true") == NULL, // TODO: Bool???
                obj["map"].GetString(),
                obj["playlist"].GetString(),
                obj["ip"].GetString(),
                obj["port"].GetString(), // TODO: Int32???
                obj["key"].GetString(),
                obj["checksum"].GetString(), // TODO: Uint32???
                SDK_VERSION,
                obj["playerCount"].GetString(), // TODO: Int32???
                obj["maxPlayers"].GetString(), // TODO: Int32???
                -1,
            }
        );
    }

    return vecServers;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the server by token string.
// Input  : &outGameServer - 
//			&outMessage - 
//			&token - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::GetServerByToken(NetGameServer_t& outGameServer,
    string& outMessage, const string& token) const
{
    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();
    requestJson.AddMember("token", rapidjson::Value(token.c_str(), requestJson.GetAllocator()), allocator);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/server/byToken", requestJson, responseJson,
        outMessage, status, "server not found"))
    {
        return false;
    }

    if (!responseJson.HasMember("server"))
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    const rapidjson::Value& serverJson = responseJson["server"];

    if (!IsServerListingValid(serverJson))
    {
        outMessage = Format("Invalid server listing data!");
        return false;
    }

    outGameServer = NetGameServer_t
    {
        serverJson["name"].GetString(),
        serverJson["description"].GetString(),
        V_strcmp(serverJson["hidden"].GetString(), "true") == NULL, // TODO: Bool???
        serverJson["map"].GetString(),
        serverJson["playlist"].GetString(),
        serverJson["ip"].GetString(),
        serverJson["port"].GetString(), // TODO: Int32???
        serverJson["key"].GetString(),
        serverJson["checksum"].GetString(), // TODO: Uint32???
        SDK_VERSION,
        serverJson["playerCount"].GetString(), // TODO: Int32???
        serverJson["maxPlayers"].GetString(), // TODO: Int32???
        -1,
    };

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sends host server POST request.
// Input  : &outMessage - 
//			&outToken - 
//			&netGameServer - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::PostServerHost(string& outMessage, string& outToken, const NetGameServer_t& netGameServer) const
{
    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    requestJson.AddMember("name",        rapidjson::Value(netGameServer.m_svHostName.c_str(),       allocator), allocator);
    requestJson.AddMember("description", rapidjson::Value(netGameServer.m_svDescription.c_str(),    allocator), allocator);
    requestJson.AddMember("hidden",      netGameServer.m_bHidden,                                   allocator);
    requestJson.AddMember("map",         rapidjson::Value(netGameServer.m_svHostMap.c_str(),        allocator), allocator);
    requestJson.AddMember("playlist",    rapidjson::Value(netGameServer.m_svPlaylist.c_str(),       allocator), allocator);
    requestJson.AddMember("ip",          rapidjson::Value(netGameServer.m_svIpAddress.c_str(),      allocator), allocator);
    requestJson.AddMember("port",        rapidjson::Value(netGameServer.m_svGamePort.c_str(),       allocator), allocator); // TODO: Int32???
    requestJson.AddMember("key",         rapidjson::Value(netGameServer.m_svEncryptionKey.c_str(),  allocator), allocator);
    requestJson.AddMember("checksum",    rapidjson::Value(netGameServer.m_svRemoteChecksum.c_str(), allocator), allocator); // TODO: Uint32???
    requestJson.AddMember("version",     rapidjson::Value(netGameServer.m_svSDKVersion.c_str(),     allocator), allocator);
    requestJson.AddMember("playerCount", rapidjson::Value(netGameServer.m_svPlayerCount.c_str(),    allocator), allocator); // TODO: Int32???
    requestJson.AddMember("maxPlayers",  rapidjson::Value(netGameServer.m_svMaxPlayers.c_str(),     allocator), allocator); // TODO: Int32???
    requestJson.AddMember("timeStamp",   netGameServer.m_nTimeStamp,                                allocator);

    rapidjson::Document responseJson;
    CURLINFO status;

    if (!SendRequest("/servers/add", requestJson, responseJson, outMessage, status, "server host error"))
    {
        return false;
    }

    if (netGameServer.m_bHidden)
    {
        if (!responseJson.HasMember("token") || !responseJson["token"].IsString())
        {
            outMessage = Format("Invalid response with status: %d", int(status));
            outToken.clear();
            return false;
        }

        outToken = responseJson["token"].GetString();
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks a list of clients for their banned status.
// Input  : &inBannedVec - 
//			&outBannedVec  - 
// Output : True on success, false otherwise.
//-----------------------------------------------------------------------------
bool CPylon::GetBannedList(const CBanSystem::BannedList_t& inBannedVec, CBanSystem::BannedList_t& outBannedVec) const
{
    rapidjson::Document requestJson;
    requestJson.SetArray();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    FOR_EACH_VEC(inBannedVec, i)
    {
        const CBanSystem::Banned_t& banned = inBannedVec[i];

        rapidjson::Value player(rapidjson::kObjectType);
        player.AddMember("id", banned.m_NucleusID, allocator);
        player.AddMember("ip", rapidjson::Value(banned.m_Address.String(), allocator), allocator);
        requestJson.PushBack(player, allocator);
    }

    rapidjson::Document responseJson;

    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/bulkCheck", requestJson, responseJson, outMessage, status, "banned bulk check error"))
    {
        return false;
    }

    if (!responseJson.HasMember("bannedPlayers") || !responseJson["bannedPlayers"].IsArray())
    {
        outMessage = Format("Invalid response with status: %d", int(status));
        return false;
    }

    const rapidjson::Value& bannedPlayers = responseJson["bannedPlayers"];
    for (const rapidjson::Value& obj : bannedPlayers.GetArray())
    {
        CBanSystem::Banned_t banned(
            obj.HasMember("reason") ? obj["reason"].GetString() : "#DISCONNECT_BANNED",
            obj.HasMember("id") && obj["id"].IsUint64() ? obj["id"].GetUint64() : NucleusID_t(NULL)
        );
        outBannedVec.AddToTail(banned);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if client is banned on the comp server.
// Input  : &ipAddress - 
//			nucleusId  - 
//			&outReason - <- contains banned reason if any.
// Output : True if banned, false if not banned.
//-----------------------------------------------------------------------------
bool CPylon::CheckForBan(const string& ipAddress, const uint64_t nucleusId, const string& personaName, string& outReason) const
{
    rapidjson::Document requestJson;
    requestJson.SetObject();

    rapidjson::Document::AllocatorType& allocator = requestJson.GetAllocator();

    requestJson.AddMember("name", rapidjson::Value(personaName.c_str(), allocator), allocator);
    requestJson.AddMember("id", nucleusId, allocator);
    requestJson.AddMember("ip", rapidjson::Value(ipAddress.c_str(), allocator), allocator);

    rapidjson::Document responseJson;
    string outMessage;
    CURLINFO status;

    if (!SendRequest("/banlist/isBanned", requestJson, responseJson, outMessage, status, "banned check error"))
    {
        return false;
    }

    if (responseJson.HasMember("banned") && responseJson["banned"].IsBool())
    {
        if (responseJson["banned"].GetBool())
        {
            outReason = responseJson.HasMember("reason") ? responseJson["reason"].GetString() : "#DISCONNECT_BANNED";
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sends request to Pylon Master Server.
// Input  : *endpoint -
//			&requestJson -
//			&responseJson -
//			&outMessage -
//			&status -
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::SendRequest(const char* endpoint, const rapidjson::Document& requestJson,
    rapidjson::Document& responseJson, string& outMessage, CURLINFO& status, const char* errorText) const
{
    rapidjson::StringBuffer stringBuffer;
    JSON_DocumentToBufferDeserialize(requestJson, stringBuffer);

    string responseBody;
    if (!QueryServer(endpoint, stringBuffer.GetString(), responseBody, outMessage, status))
    {
        return false;
    }

    if (status == 200) // STATUS_OK
    {
        responseJson.Parse(responseBody.c_str());

        if (responseJson.HasParseError())
        {
            Warning(eDLL_T::ENGINE, "%s: JSON parse error at position %zu: %s\n", __FUNCTION__,
                responseJson.GetErrorOffset(), rapidjson::GetParseError_En(responseJson.GetParseError()));

            return false;
        }

        if (!responseJson.IsObject())
        {
            Warning(eDLL_T::ENGINE, "%s: JSON root was not an object\n", __FUNCTION__);
            return false;
        }

        if (pylon_showdebuginfo->GetBool())
        {
            LogBody(responseJson);
        }

        if (responseJson.HasMember("success") &&
            responseJson["success"].IsBool() &&
            responseJson["success"].GetBool())
        {
            return true;
        }
        else
        {
            ExtractError(responseJson, outMessage, status);
            return false;
        }
    }
    else
    {
        ExtractError(responseBody, outMessage, status, errorText);
        return false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Sends query to master server.
// Input  : *endpoint    - 
//          *request     - 
//          &outResponse - 
//          &outMessage  - <- contains an error message on failure.
//          &outStatus   - 
// Output : True on success, false on failure.
//-----------------------------------------------------------------------------
bool CPylon::QueryServer(const char* endpoint, const char* request,
    string& outResponse, string& outMessage, CURLINFO& outStatus) const
{
    const bool showDebug = pylon_showdebuginfo->GetBool();
    const char* hostName = pylon_matchmaking_hostname->GetString();

    if (showDebug)
    {
        Msg(eDLL_T::ENGINE, "Sending request to '%s' with endpoint '%s':\n%s\n",
            hostName, endpoint, request);
    }

    string finalUrl;
    CURLFormatUrl(finalUrl, hostName, endpoint);

    finalUrl += Format("?language=%s", this->m_Language);

    CURLParams params;

    params.writeFunction = CURLWriteStringCallback;
    params.timeout = curl_timeout->GetInt();
    params.verifyPeer = ssl_verify_peer->GetBool();
    params.verbose = curl_debug->GetBool();

    curl_slist* sList = nullptr;
    CURL* curl = CURLInitRequest(finalUrl.c_str(), request, outResponse, sList, params);
    if (!curl)
    {
        return false;
    }

    CURLcode res = CURLSubmitRequest(curl, sList);
    if (!CURLHandleError(curl, res, outMessage,
        !IsDedicated(/* Errors are already shown for dedicated! */)))
    {
        return false;
    }

    outStatus = CURLRetrieveInfo(curl);

    if (showDebug)
    {
        Msg(eDLL_T::ENGINE, "Host '%s' replied with status: '%d'\n",
            hostName, outStatus);
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the result json.
// Input  : &resultJson - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const rapidjson::Document& resultJson, string& outMessage,
    CURLINFO status, const char* errorText) const
{

    if (resultJson.IsObject() && resultJson.HasMember("error") &&
        resultJson["error"].IsString())
    {
        outMessage = resultJson["error"].GetString();
    }
    else
    {
        if (!errorText)
        {
            errorText = "unknown error";
        }

        outMessage = Format("Failed with status: %d (%s)",
            int(status), errorText);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the error from the response buffer.
// Input  : &response   - 
//          &outMessage - 
//          status      - 
//          *errorText  - 
//-----------------------------------------------------------------------------
void CPylon::ExtractError(const string& response, string& outMessage,
    CURLINFO status, const char* errorText) const
{
    if (!response.empty())
    {
        rapidjson::Document resultBody;
        resultBody.Parse(response.c_str());

        ExtractError(resultBody, outMessage, status, errorText);
    }
    else if (status)
    {
        outMessage = Format("Failed server query: %d", int(status));
    }
    else
    {
        outMessage = Format("Failed to reach server: %s",
            "connection timed out");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Logs the response body if debug is enabled.
// Input  : &responseJson -
//-----------------------------------------------------------------------------
void CPylon::LogBody(const rapidjson::Document& responseJson) const
{
    rapidjson::StringBuffer stringBuffer;

    JSON_DocumentToBufferDeserialize(responseJson, stringBuffer);
    Msg(eDLL_T::ENGINE, "\n%s\n", stringBuffer.GetString());
}

///////////////////////////////////////////////////////////////////////////////
CPylon* g_pMasterServer(new CPylon());
