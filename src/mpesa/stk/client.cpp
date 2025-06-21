/**
 * @file client.cpp
 * @brief STK Push Client Implementation
 * 
 * Implements the client interface for initiating STK Push transactions
 * with the M-Pesa API, including request handling, response processing,
 * and error management.
 * 
 * This implementation:
 * - Uses libcurl for HTTP communication
 * - Implements a RAII wrapper for CURL resources
 * - Handles authentication token passing from the Auth component
 * - Processes JSON requests and responses with nlohmann::json
 * - Manages asynchronous operations via std::future/std::async
 * - Provides thread-safe request counting
 */

#include "mpesa/stk/client.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <openssl/buffer.h>
#include <sstream>
#include <iostream>
#include <atomic>
#include <chrono>

namespace mpesa {
namespace stk {

/**
 * @brief RAII wrapper for CURL handle
 * 
 * Ensures proper cleanup of CURL resources even if exceptions occur.
 * This prevents resource leaks when using libcurl.
 */
class CurlWrapper {
public:
    /**
     * @brief Initialize CURL handle
     * @throws std::runtime_error if initialization fails
     * 
     * Initializes a new CURL handle for HTTP operations.
     * Throws an exception if initialization fails, preventing
     * operations with an invalid handle.
     */
    CurlWrapper() : curl(curl_easy_init()) {
        if (!curl) throw std::runtime_error("Failed to initialize CURL");
    }

    /**
     * @brief Clean up CURL handle
     * 
     * Automatically releases CURL resources when the wrapper
     * goes out of scope, preventing memory leaks.
     */
    ~CurlWrapper() { 
        if(curl) curl_easy_cleanup(curl); 
    }

    /**
     * @brief Get raw CURL handle
     * @return CURL* Raw handle for direct operations
     * 
     * Provides access to the underlying CURL handle for
     * configuring and performing HTTP operations.
     */
    CURL* get() { return curl; }

    /**
     * @brief Set operation timeouts
     * @param timeout Overall timeout in seconds
     * @param connectTimeout Connection timeout in seconds
     * 
     * Configures both the overall operation timeout and the
     * connection establishment timeout to prevent requests
     * from hanging indefinitely.
     */
    void setTimeouts(long timeout = 30L, long connectTimeout = 10L) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeout);
    }

private:
    CURL* curl; ///< Raw CURL handle
};

/**
 * @brief Callback for writing CURL response data
 * @param contents Pointer to received data
 * @param size Size of each element
 * @param nmemb Number of elements
 * @param userp User pointer (string buffer)
 * @return Total size of processed data
 * 
 * This callback is used by libcurl to write received data to a string buffer.
 * It's called repeatedly as data arrives from the network.
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Constructor
/**
 * @brief Initialize the STK Push client
 * @param auth Reference to an authenticated Auth instance
 * 
 * Initializes CURL global resources and stores a reference to the Auth
 * object for later token retrieval. Also generates a timestamp to be
 * used for all transactions processed by this client instance.
 * 
 * @note The Auth instance must remain valid for the lifetime of this client.
 * @note CURL global initialization is performed here, with corresponding cleanup
 *       in the destructor.
 */
STKPushClient::STKPushClient(const mpesa::Auth& auth)
        : auth_(auth), timestamp_(validation::TimestampGenerator::generate()) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

// Destructor
/**
 * @brief Clean up STK Push client resources
 * 
 * Performs CURL global cleanup to release resources initialized
 * in the constructor.
 */
STKPushClient::~STKPushClient() {
    curl_global_cleanup();
}

/**
 * @brief Generate password for STK Push request
 * 
 * Creates a base64 encoded password string from the input parameters
 * as required by the M-Pesa API. The password is created by concatenating
 * the business short code, passkey, and timestamp, then applying Base64 encoding.
 * 
 * @param businessShortCode The business short code from the request
 * @param passkey The STK passkey from authentication config
 * @param timestamp The timestamp in YYYYMMDDHHMMSS format
 * @return Base64 encoded password string
 */
std::string STKPushClient::generatePassword(
    const std::string& businessShortCode,
    const std::string& passkey,
    const std::string& timestamp
) {     
    std::string combined = businessShortCode + passkey + timestamp;
    return Base64Encode(combined);
}

/**
 * @brief Initiate an STK Push request asynchronously
 * 
 * Validates and sends an STK Push request to the M-Pesa API. The operation
 * is performed asynchronously and returns a future containing the result.
 * 
 * This method:
 * 1. Sets the password and timestamp on the request
 * 2. Launches an async task to send the request
 * 3. Validates request parameters
 * 4. Makes an HTTP request to the M-Pesa API
 * 5. Processes the response
 * 6. Returns success or failure information
 * 
 * @param request The STK Push request parameters
 * @return Future containing the request result
 * 
 * @note The timestamp used is the one generated during client construction,
 * not a fresh timestamp per request.
 * @note The auth token is retrieved from the Auth component for each request,
 * which may trigger a token refresh if needed.
 */
std::future<Result<STKPushResponse>> STKPushClient::initiateSTKPush(STKPushRequest request) {
    // Set password and timestamp on the request
    request.password = generatePassword(
        request.businessShortCode,
        auth_.getConfig().stk_passkey,
        timestamp_
    );
    request.timestamp = timestamp_;

    // Make a local copy for the async operation
    STKPushRequest localRequest = std::move(request);
    
    return std::async(std::launch::async, [this, localRequest]() mutable {
        try {
            // Validate request parameters
            auto validation = validation::Validator::validateSTKPushRequest(localRequest);
            if (!validation.isValid) {
                failureCount_++;
                return Result<STKPushResponse>(validation.error);
            }
            
            CurlWrapper curl;
            curl.setTimeouts();

            std::string url = auth_.getBaseUrl() + STKPUSH_AUTH_ENDPOINT;
            std::string response_string;
            std::string auth_header = createStkPushAuthHeader();

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, auth_header.c_str());
            headers = curl_slist_append(headers, "Content-Type: application/json");

            // Store the payload in a string to ensure its lifetime
            std::string payload_str = localRequest.toJson().dump();

            curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload_str.c_str()); // Use the stored string's c_str()
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_string);
            curl_easy_setopt(curl.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

            CURLcode res = curl_easy_perform(curl.get());
            
            long http_code = 0;
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
            
            curl_slist_free_all(headers);

            if (res != CURLE_OK) {
                failureCount_++;
                return Result<STKPushResponse>("CURL error: " + std::string(curl_easy_strerror(res)));
            }
           
            if (http_code >= 400) {
                failureCount_++;
                try {
                    auto error_json = nlohmann::json::parse(response_string);
                    return Result<STKPushResponse>("API Error: " + 
                        error_json["errorMessage"].get<std::string>() + 
                        " (Code: " + error_json["errorCode"].get<std::string>() + ")");
                } catch (...) {
                    return Result<STKPushResponse>("HTTP error: " + std::to_string(http_code));
                }
            }

            try {
                auto json = nlohmann::json::parse(response_string);
                
                STKPushResponse response;
                response.merchantRequestID = json["MerchantRequestID"];
                response.checkoutRequestID = json["CheckoutRequestID"];
                response.responseCode = json["ResponseCode"];
                response.responseDescription = json["ResponseDescription"];
                response.customerMessage = json["CustomerMessage"];

                successCount_++;
                return Result<STKPushResponse>(response);

            } catch (const nlohmann::json::exception& e) {
                failureCount_++;
                return Result<STKPushResponse>("JSON parse error: " + std::string(e.what()));
            }

        } catch (const std::exception& e) {
            failureCount_++;
            return Result<STKPushResponse>("Request error: " + std::string(e.what()));
        }
    });
}

/**
 * @brief Create Authorization header for STK Push
 * @return Header string with bearer token
 * 
 * Creates the Authorization header required for M-Pesa API requests
 * by retrieving the access token from the Auth component.
 * 
 * @note This method retrieves a fresh token (or uses a cached valid token)
 * from the Auth component for each call. This ensures that the token is
 * always valid, even if multiple requests are made over an extended period.
 * 
 * @note The format "Authorization: Bearer TOKEN" is critical - must include
 * the space after "Bearer" to avoid authentication failures.
 */
std::string STKPushClient::createStkPushAuthHeader() const {
    return "Authorization: Bearer " + auth_.getAccessToken();
}

/**
 * @brief Base64 encode a string
 * @param input String to encode
 * @return Base64 encoded string
 * 
 * Uses OpenSSL's BIO functions to perform Base64 encoding without linebreaks.
 * Handles empty input strings gracefully by returning an empty string.
 * 
 * @throws std::runtime_error If OpenSSL operations fail
 * 
 * @note This method implements proper resource management with RAII to
 * ensure that OpenSSL resources are released even if exceptions occur.
 */
std::string STKPushClient::Base64Encode(const std::string& input) {
    if (input.empty()) {
        return "";
    }

    BIO *bio = nullptr, *b64 = nullptr;
    BUF_MEM *bufferPtr = nullptr;

    try {
        // Create BIO chain for base64
        b64 = BIO_new(BIO_f_base64());
        if (!b64) {
            throw std::runtime_error("Failed to create base64 BIO");
        }
        
        // Don't use newlines
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        
        bio = BIO_new(BIO_s_mem());
        if (!bio) {
            BIO_free_all(b64);
            throw std::runtime_error("Failed to create memory BIO");
        }
        
        bio = BIO_push(b64, bio);

        // Write data
        BIO_write(bio, input.c_str(), input.length());
        BIO_flush(bio);

        // Get encoded data
        BIO_get_mem_ptr(bio, &bufferPtr);
        std::string result(bufferPtr->data, bufferPtr->length);
        
        // Cleanup
        BIO_free_all(bio);
        
        return result;

    } catch (...) {
        if (bio) BIO_free_all(bio);
        throw;
    }
}

} // namespace stk
} // namespace mpesa