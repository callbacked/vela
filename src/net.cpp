#include "net.h"
#include <psp2/net/http.h>
#include <psp2/libssl.h>
#include <string>
#include <vector>
#include <cstring>
#include <jsoncpp/json/json.h>

#include "config.h"
#include "settings.h"

std::string nativePostRequest(const std::string& url, const std::string& postdata, const std::string& apiKey) {
    int tpl = -1, conn = -1, req = -1;
    std::string response_string;

    tpl = sceHttpCreateTemplate("vela_http_template", 2, 1);
    if (tpl < 0) {
        return "Error: sceHttpCreateTemplate failed";
    }

    sceHttpAddRequestHeader(tpl, "Content-Type", "application/json", SCE_HTTP_HEADER_ADD);
    if (!apiKey.empty()) {
        std::string bearer_token = "Bearer " + apiKey;
        sceHttpAddRequestHeader(tpl, "Authorization", bearer_token.c_str(), SCE_HTTP_HEADER_ADD);
    }

    conn = sceHttpCreateConnectionWithURL(tpl, url.c_str(), 1);
    if (conn < 0) {
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpCreateConnectionWithURL failed";
    }

    req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_POST, url.c_str(), postdata.length());
    if (req < 0) {
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpCreateRequestWithURL failed";
    }

    if (sceHttpSendRequest(req, postdata.c_str(), postdata.length()) < 0) {
        sceHttpDeleteRequest(req);
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpSendRequest failed";
    }

    char buffer[4096];
    int n, offset = 0;
    std::vector<char> response_data;

    while ((n = sceHttpReadData(req, buffer, sizeof(buffer))) > 0) {
        response_data.insert(response_data.end(), buffer, buffer + n);
    }

    if (!response_data.empty()) {
        response_string = std::string(response_data.begin(), response_data.end());
    } else {
        response_string = "Error: No data received";
    }

    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tpl);

    return response_string;
}

std::string nativeGetRequest(const std::string& url, const std::string& apiKey) {
    int tpl = -1, conn = -1, req = -1;
    std::string response_string;

    tpl = sceHttpCreateTemplate("vela_http_template_get", 2, 1);
    if (tpl < 0) {
        return "Error: sceHttpCreateTemplate failed";
    }

    if (!apiKey.empty()) {
        std::string bearer_token = "Bearer " + apiKey;
        sceHttpAddRequestHeader(tpl, "Authorization", bearer_token.c_str(), SCE_HTTP_HEADER_ADD);
    }

    conn = sceHttpCreateConnectionWithURL(tpl, url.c_str(), 1);
    if (conn < 0) {
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpCreateConnectionWithURL failed";
    }

    req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, url.c_str(), 0);
    if (req < 0) {
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpCreateRequestWithURL failed";
    }

    if (sceHttpSendRequest(req, NULL, 0) < 0) {
        sceHttpDeleteRequest(req);
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tpl);
        return "Error: sceHttpSendRequest failed";
    }

    char buffer[4096];
    std::vector<char> response_data;

    while (true) {
        int n = sceHttpReadData(req, buffer, sizeof(buffer));
        if (n < 0) {
            response_string = "Error: sceHttpReadData failed";
            break;
        }
        if (n == 0) {
            break; // End of response
        }
        response_data.insert(response_data.end(), buffer, buffer + n);
    }

    if (response_string.empty() && !response_data.empty()) {
        response_string = std::string(response_data.begin(), response_data.end());
    } else if (response_data.empty()) {
        response_string = "Error: No data received";
    }

    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tpl);

    return response_string;
}

std::string nativePostRequestWithImage(const std::string& url, const std::string& text_prompt, const std::string& base64_image, const std::string& model, const std::string& apiKey) {
    Json::Value root;
    Json::Value messages(Json::arrayValue);
    Json::Value user_message;
    Json::Value content(Json::arrayValue);

    // Text part
    Json::Value text_part;
    text_part["type"] = "text";
    text_part["text"] = text_prompt;
    content.append(text_part);

    // Image part
    Json::Value image_part;
    image_part["type"] = "image_url";
    Json::Value image_url;
    image_url["url"] = "data:image/png;base64," + base64_image;
    image_part["image_url"] = image_url;
    content.append(image_part);
    
    user_message["role"] = "user";
    user_message["content"] = content;
    messages.append(user_message);

    root["model"] = model;
    root["messages"] = messages;

    Json::StreamWriterBuilder writer;
    std::string json_payload = Json::writeString(writer, root);

    return nativePostRequest(url, json_payload, apiKey);
}

std::vector<std::string> fetch_models(const std::string& endpoint, const std::string& apiKey) {
    std::vector<std::string> models;
    std::string models_url;


    Settings settings = load_settings();
    if (settings.models_endpoint_overrides.count(endpoint) > 0) {
        // Use the override URL directly
        models_url = settings.models_endpoint_overrides[endpoint];
    } else {
        // No override, use the default logic (pulls models from openai and webui style endpoints)
        
        const std::string openai_chat_suffix = "/v1/chat/completions";
        const std::string webui_chat_suffix = "/api/chat/completions";

        if (endpoint.size() > openai_chat_suffix.size() && endpoint.rfind(openai_chat_suffix) == endpoint.size() - openai_chat_suffix.size()) {
            // openai style suffix, replace it with the models path
            models_url = endpoint.substr(0, endpoint.size() - openai_chat_suffix.size()) + "/v1/models";
        } else if (endpoint.size() > webui_chat_suffix.size() && endpoint.rfind(webui_chat_suffix) == endpoint.size() - webui_chat_suffix.size()) {
            // OWUI suffix, replace it with the models path
            models_url = endpoint.substr(0, endpoint.size() - webui_chat_suffix.size()) + "/api/models";
        } else {
            return models;
        }
    }

    std::string response_text = nativeGetRequest(models_url, apiKey);

    Json::Value root;
    Json::CharReaderBuilder reader_builder;
    std::unique_ptr<Json::CharReader> const reader(reader_builder.newCharReader());
    JSONCPP_STRING errs;

    if (reader->parse(response_text.c_str(), response_text.c_str() + response_text.length(), &root, &errs)) {
        if (root.isObject() && root.isMember("data") && root["data"].isArray()) {
            for (const auto& model_obj : root["data"]) {
                if (model_obj.isObject() && model_obj.isMember("id")) {
                    models.push_back(model_obj["id"].asString());
                }
            }
        }
    }
    return models;
} 