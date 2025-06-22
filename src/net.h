#ifndef NET_H
#define NET_H

#include <string>
#include <vector>
#include "types.h"

bool initialize_network(const std::string& endpoint);

std::string nativePostRequest(const std::string& endpoint, const std::string& jsonPayload, const std::string& apiKey);

std::string nativeGetRequest(const std::string& url, const std::string& apiKey);

std::string nativePostRequestWithImage(const std::string& endpoint, const std::string& prompt, const std::string& base64Image, const std::string& model, const std::string& apiKey);

std::vector<std::string> fetch_models(const std::string& endpoint, const std::string& apiKey);

#endif 