#include "mpesa/auth.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>

namespace mpesa {

namespace {
    // Callback to write CURL response to string
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Base64 encoding using OpenSSL
    std::string base64_encode(const std::string& input) {
        BIO* bio, *b64;
        BUF_MEM* bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.c_str(), input.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string result(bufferPtr->data, bufferPtr->length);

        BIO_free_all(bio);

        return result;
    }

    // Map CURL errors to Auth errors
    AuthErrorCode map_curl_error(CURLcode code) {
        switch (code) {
            case CURLE_COULDNT_RESOLVE_HOST:
                return AuthErrorCode::DNS_ERROR;
            case CURLE_COULDNT_CONNECT:
                return AuthErrorCode::CONNECTION_ERROR;
            case CURLE_OPERATION_TIMEDOUT:
                return AuthErrorCode::TIMEOUT_ERROR;
            case CURLE_SSL_CONNECT_ERROR:
                return AuthErrorCode::SSL_ERROR;
            default:
                return AuthErrorCode::NETWORK_ERROR;
        }
    }

    // Map M-Pesa API errors to Auth errors
    AuthErrorCode map_mpesa_error(const nlohmann::json& response) {
        if (!response.contains("errorCode")) {
            return AuthErrorCode::SUCCESS;
        }

        const auto error_code = response["errorCode"].get<std::string>();
        // Specific M-Pesa error code mappings
        if (error_code == "400.008.02") {
            return AuthErrorCode::INVALID_GRANT_TYPE;      // Invalid grant type passed
        } 
        else if (error_code == "400.008.01") {
            return AuthErrorCode::INVALID_AUTH_TYPE;       // Invalid Authentication type (not Basic Auth)
        }
        else if (error_code == "401.002.01") {
            return AuthErrorCode::INVALID_CREDENTIALS;     // Invalid credentials
        } 
        else if (error_code == "500.001.1001") {
            return AuthErrorCode::SERVER_ERROR;           // Server error
        }
        // Add more M-Pesa error codes as needed

        return AuthErrorCode::API_ERROR;
    }
}
// Constructor to initialize Auth with consumer key and secret
Auth::Auth(std::string consumer_key, std::string consumer_secret, bool sandbox)
    : config_{std::move(consumer_key), std::move(consumer_secret), sandbox},
      last_error_{AuthErrorCode::SUCCESS} {}

// Constructor to initialize Auth with AuthConfig
Auth::Auth(const AuthConfig& config) 
    : config_(config),
      last_error_{AuthErrorCode::SUCCESS} {}

// Get access token, refresh if necessary
std::string Auth::getAccessToken() {
    std::lock_guard<std::mutex> lock(token_mutex_);
    
    if (!current_token_ || !isTokenValid()) {
        auto response = refreshToken();
        if (response.error_code != AuthErrorCode::SUCCESS) {
            throw AuthenticationError("Failed to get access token", response.error_code);
        }
    }
    
    return current_token_.value();
}

// Check if the current token is still valid
bool Auth::isTokenValid() const {
    return std::chrono::system_clock::now() < token_expiry_;
}

// Refresh the access token by making a request to the M-Pesa API
AuthResponse Auth::refreshToken() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        updateLastError(AuthErrorCode::INITIALIZATION_ERROR);
        return AuthResponse{.error_code = AuthErrorCode::INITIALIZATION_ERROR};
    }

    std::string url = getBaseUrl() + AUTH_ENDPOINT;
    std::string response_string;
    std::string auth_header = createAuthHeader();

    // Add grant_type parameter to URL
    url += "?grant_type=client_credentials";

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, auth_header.c_str());
    //headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        auto error = map_curl_error(res);
        updateLastError(error);
        return AuthResponse{.error_code = error};
    }

    // Handle HTTP errors
    if (http_code >= 400) {
        updateLastError(AuthErrorCode::HTTP_ERROR);
        return AuthResponse{.error_code = AuthErrorCode::HTTP_ERROR};
    }

    try {
        //std::cout<< "API Response: " << response_string << std::endl;
        auto json_response = nlohmann::json::parse(response_string);
        auto mpesa_error = map_mpesa_error(json_response);

         if (mpesa_error != AuthErrorCode::SUCCESS) {
            updateLastError(mpesa_error);
            return AuthResponse{.error_code = mpesa_error};
        }

        AuthResponse response;
        response.access_token = json_response["access_token"].get<std::string>();
        response.expires_in = std::chrono::seconds(std::stoi(json_response["expires_in"].get<std::string>()));
        
        // Update token data
        current_token_ = response.access_token;
        token_expiry_ = std::chrono::system_clock::now() + response.expires_in;
        
        updateLastError(AuthErrorCode::SUCCESS);
        return response;

    } catch (const nlohmann::json::exception& e) {
        updateLastError(AuthErrorCode::PARSE_ERROR);
    } 
}

// Get the base URL for the M-Pesa API based on the environment (sandbox or production)
std::string Auth::getBaseUrl() const {
    return config_.sandbox ? SANDBOX_URL : PRODUCTION_URL;
}

// Create the authorization header for the API request
std::string Auth::createAuthHeader() const {
    std::string credentials = config_.consumer_key + ":" + config_.consumer_secret;
    std::string encoded = base64_encode(credentials);
    return "Authorization: Basic " + encoded;
}

// Update the last error code
void Auth::updateLastError(AuthErrorCode code) {
    last_error_ = code;
}

// Get the last error code
AuthErrorCode Auth::getLastError() const {
    return last_error_;
}

// Load AuthConfig from a file 
AuthConfig AuthConfig::from_file(const std::string& path) {
    // Validate file path
    if (!std::filesystem::exists(path)) {
        throw AuthenticationError(
            "Configuration file not found: " + path, 
            AuthErrorCode::CONFIG_ERROR
        );
    }

    // Open and read the file
    std::ifstream file(path);
    if (!file.is_open()) {
        throw AuthenticationError(
            "Unable to open configuration file: " + path, 
            AuthErrorCode::CONFIG_ERROR
        );
    }

    // Parse JSON
    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception& e) {
        throw AuthenticationError(
            "Failed to parse JSON: " + std::string(e.what()), 
            AuthErrorCode::PARSE_ERROR
        );
    }

    // Extract values from JSON
    std::string consumer_key, consumer_secret;
    bool sandbox = true;  // Default to sandbox mode

    if (json.contains("consumer_key")) {
        consumer_key = json["consumer_key"].get<std::string>();
    } else {
        throw AuthenticationError(
            "Missing 'consumer_key' in config file", 
            AuthErrorCode::CONFIG_ERROR
        );
    }

    if (json.contains("consumer_secret")) {
        consumer_secret = json["consumer_secret"].get<std::string>();
    } else {
        throw AuthenticationError(
            "Missing 'consumer_secret' in config file", 
            AuthErrorCode::CONFIG_ERROR
        );
    }

    if (json.contains("sandbox")) {
        sandbox = json["sandbox"].get<bool>();
    }

    return AuthConfig{
        .consumer_key = consumer_key,
        .consumer_secret = consumer_secret,
        .sandbox = sandbox
    };
}
// Load AuthConfig from environment variables
AuthConfig AuthConfig::from_env() {
    // Helper function to get environment variable with error checking
    auto get_env = [](const char* name) -> std::optional<std::string> {
        const char* value = std::getenv(name);
        if (!value) {
            return std::nullopt;
        }
        return std::string(value);
    };

    // Get required environment variables
    auto consumer_key = get_env("MPESA_CONSUMER_KEY");
    auto consumer_secret = get_env("MPESA_CONSUMER_SECRET");
    auto env = get_env("MPESA_ENVIRONMENT");

    // Check for required credentials
    if (!consumer_key || !consumer_secret) {
        throw AuthenticationError(
            "Missing required environment variables. Please set MPESA_CONSUMER_KEY and MPESA_CONSUMER_SECRET",
            AuthErrorCode::CONFIG_ERROR
        );
    }

    // Set sandbox mode based on environment variable (default to sandbox if not set)
    bool sandbox = true;
    if (env && (*env == "production" || *env == "PRODUCTION")) {
        sandbox = false;
    }

    return AuthConfig{
        .consumer_key = *consumer_key,
        .consumer_secret = *consumer_secret,
        .sandbox = sandbox
    };
}

} // namespace mpesa