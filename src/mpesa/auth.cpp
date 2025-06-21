/**
 * @file auth.cpp
 * @brief Implements the authentication logic for the M-Pesa C++ SDK.
 *
 * This file contains the implementation of the Auth class, which is responsible
 * for obtaining and managing OAuth 2.0 access tokens required for interacting
 * with the M-Pesa API. It handles token requests, automatic refresh,
 * and error management related to authentication. It also includes helper
 * functions for tasks like Base64 encoding and error mapping.
 * Configuration can be loaded from files or environment variables.
 */
#include "mpesa/auth.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>

namespace mpesa {

namespace {
    /**
     * @brief libcurl write callback function.
     *
     * This function is called by libcurl to deliver received data. It appends
     * the received data to a user-provided std::string.
     *
     * @param contents Pointer to the delivered data.
     * @param size Size of each data element.
     * @param nmemb Number of data elements.
     * @param userp Pointer to the std::string buffer to store the data.
     * @return The total number of bytes processed (size * nmemb).
     */
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    /**
     * @brief Encodes a string using Base64.
     *
     * This utility function uses OpenSSL's BIO library to perform Base64 encoding.
     * It ensures that no newline characters are included in the output.
     *
     * @param input The string to be Base64 encoded.
     * @return The Base64 encoded string.
     * @throws std::runtime_error if OpenSSL BIO operations fail, although this specific implementation
     *         does not explicitly throw, OpenSSL functions might have their own error handling
     *         that could lead to termination or undefined behavior on failure if not checked.
     *         Robust implementation should check return values of BIO_new, BIO_write etc.
     */
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

    /**
     * @brief Maps a CURLcode to an AuthErrorCode.
     *
     * Translates common libcurl error codes into SDK-specific authentication
     * error codes for consistent error handling.
     *
     * @param code The CURLcode returned by a libcurl operation.
     * @return The corresponding AuthErrorCode.
     */
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

    /**
     * @brief Maps M-Pesa API error responses to an AuthErrorCode.
     *
     * Parses a JSON error response from the M-Pesa API and translates
     * known M-Pesa error codes into SDK-specific authentication error codes.
     *
     * @param response The JSON response object from the M-Pesa API.
     * @return The corresponding AuthErrorCode. Returns AuthErrorCode::SUCCESS
     *         if no "errorCode" field is found in the response, assuming it's a success response.
     */
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

/**
 * @brief Constructs an Auth object with explicit credentials and settings.
 *
 * Initializes the authentication manager with the consumer key, consumer secret,
 * STK passkey, and environment type (sandbox or production).
 *
 * @param consumer_key The M-Pesa API consumer key.
 * @param consumer_secret The M-Pesa API consumer secret.
 * @param stk_passkey The STK push passkey.
 * @param sandbox True if using the sandbox environment, false for production. Defaults to true.
 */
Auth::Auth(std::string consumer_key, std::string consumer_secret, std::string stk_passkey, bool sandbox)
    : config_{
          .consumer_key = std::move(consumer_key),
          .consumer_secret = std::move(consumer_secret),
          .sandbox = sandbox,
          .stk_passkey = std::move(stk_passkey)
      } {
    last_error_ = AuthErrorCode::SUCCESS;
}

// Constructor to initialize Auth with AuthConfig
Auth::Auth(const AuthConfig& config) 
    : config_(config),
      last_error_{AuthErrorCode::SUCCESS} {}

/**
 * @brief Retrieves the current valid access token.
 *
 * If the current token is invalid or expired, this method automatically
 * attempts to refresh it by calling refreshToken().
 *
 * @return A string containing the valid access token.
 * @throws AuthenticationError if token refresh fails. The error code within
 *         AuthenticationError will indicate the specific reason for failure.
 * @note This method is thread-safe due to the use of std::lock_guard.
 */
std::string Auth::getAccessToken() const {
    std::lock_guard<std::mutex> lock(token_mutex_);
    
    if (!current_token_ || !isTokenValid()) {
        auto response = const_cast<Auth*>(this)->refreshToken();
        if (response.error_code != AuthErrorCode::SUCCESS) {
            throw AuthenticationError("Failed to get access token", response.error_code);
        }
    }
    
    return current_token_.value();
}

/**
 * @brief Checks if the current access token is still valid.
 *
 * Compares the token's expiry time with the current system time.
 *
 * @return True if the token is valid (i.e., has been fetched and not expired), false otherwise.
 * @note This method itself doesn't use locks but is typically called from within
 *       a critical section in getAccessToken().
 */
bool Auth::isTokenValid() const {
    return std::chrono::system_clock::now() < token_expiry_;
}

/**
 * @brief Refreshes the OAuth access token.
 *
 * Makes an API call to the M-Pesa authentication endpoint to obtain a new
 * access token using the configured consumer key and secret. Updates the
 * internal token state (`current_token_`, `token_expiry_`) upon success.
 * Also updates `last_error_` with the status of the operation.
 *
 * @return An AuthResponse object containing the new token details (access_token, expires_in)
 *         and an error_code. AuthResponse.error_code will be AuthErrorCode::SUCCESS on success.
 * @note This method is not const because it modifies the internal token state.
 *       It's called internally by getAccessToken() when a token refresh is needed.
 */
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
        return AuthResponse{.error_code = AuthErrorCode::PARSE_ERROR};
    } 
}

/**
 * @brief Gets the base URL for M-Pesa API requests.
 *
 * Returns either the sandbox or production API base URL based on the
 * `sandbox` flag in the AuthConfig.
 *
 * @return The base URL string (e.g., "https://sandbox.safaricom.co.ke" or "https://api.safaricom.co.ke").
 */
std::string Auth::getBaseUrl() const {
    return config_.sandbox ? SANDBOX_URL : PRODUCTION_URL;
}

/**
 * @brief Creates the HTTP Authorization header for token requests.
 *
 * The header uses Basic Authentication, with credentials formed by
 * Base64 encoding the "consumer_key:consumer_secret" string.
 *
 * @return The "Authorization: Basic <encoded_credentials>" header string.
 */
std::string Auth::createAuthHeader() const {
    std::string credentials = config_.consumer_key + ":" + config_.consumer_secret;
    std::string encoded = base64_encode(credentials);
    return "Authorization: Basic " + encoded;
}

/**
 * @brief Updates the last recorded authentication error.
 * @param code The AuthErrorCode to set as the last error.
 */
void Auth::updateLastError(AuthErrorCode code) {
    last_error_ = code;
}

/**
 * @brief Retrieves the last recorded authentication error code.
 * @return The last AuthErrorCode.
 */
AuthErrorCode Auth::getLastError() const {
    return last_error_;
}

/**
 * @brief Loads authentication configuration from a JSON file.
 *
 * Parses a JSON file specified by `path` and populates an AuthConfig object.
 * The JSON file is expected to contain "consumer_key", "consumer_secret",
 * "stk_passkey", and an optional "sandbox" boolean (defaults to true if missing).
 *
 * @param path The filesystem path to the JSON configuration file.
 * @return An AuthConfig object populated from the file.
 * @throws AuthenticationError if the file is not found, cannot be opened,
 *         or if JSON parsing fails, or if required keys ("consumer_key", "consumer_secret", "stk_passkey") are missing.
 *         The error code will be AuthErrorCode::CONFIG_ERROR or AuthErrorCode::PARSE_ERROR.
 */
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
    std::string consumer_key, consumer_secret, stk_passkey;
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

    if (json.contains("stk_passkey")) {
        stk_passkey = json["stk_passkey"].get<std::string>();
    } else {
        throw AuthenticationError(
            "Missing 'stk_passkey' in config file", 
            AuthErrorCode::CONFIG_ERROR
        );
    }

    if (json.contains("sandbox")) {
        sandbox = json["sandbox"].get<bool>();
    }

    return AuthConfig{
        .consumer_key = consumer_key,
        .consumer_secret = consumer_secret,
        .sandbox = sandbox,
        .stk_passkey = stk_passkey
    };
}

/**
 * @brief Loads authentication configuration from environment variables.
 *
 * Populates an AuthConfig object using the following environment variables:
 * - MPESA_CONSUMER_KEY: The M-Pesa API consumer key. (Required)
 * - MPESA_CONSUMER_SECRET: The M-Pesa API consumer secret. (Required)
 * - MPESA_STK_PASSKEY: The STK push passkey. (Required)
 * - MPESA_ENVIRONMENT: (Optional) Set to "production" (case-insensitive) for the production environment;
 *                      defaults to sandbox if not set or has a different value.
 *
 * @return An AuthConfig object populated from environment variables.
 * @throws AuthenticationError if any of the required environment variables (MPESA_CONSUMER_KEY,
 *         MPESA_CONSUMER_SECRET, MPESA_STK_PASSKEY) are not set.
 *         The error code will be AuthErrorCode::CONFIG_ERROR.
 */
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
    auto stk_passkey = get_env("MPESA_STK_PASSKEY");
    auto env = get_env("MPESA_ENVIRONMENT");

    // Check for required credentials
    if (!consumer_key || !consumer_secret || !stk_passkey) {
        throw AuthenticationError(
            "Missing required environment variables. Please set MPESA_CONSUMER_KEY, MPESA_CONSUMER_SECRET, and MPESA_STK_PASSKEY",
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
        .sandbox = sandbox,
        .stk_passkey = *stk_passkey
    };
}

} // namespace mpesa