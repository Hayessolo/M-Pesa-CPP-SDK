/**
 * @file client.hpp
 * @brief STK Push Client Implementation
 * 
 * Provides the main client interface for initiating STK Push (Lipa Na M-Pesa Online)
 * transactions with the M-Pesa API.
 * Handles request validation, API communication, response processing,
 * and error management.
 * 
 * This client depends on the mpesa::Auth component for authentication tokens and
 * uses them to make authorized API requests to the M-Pesa STK Push endpoint.
 * The Auth component must be properly initialized with valid credentials before
 * using this client.
 * 
 * Key features:
 * - Asynchronous API for non-blocking transaction initiation
 * - Thread-safe implementation for concurrent transaction processing
 * - Automatic request validation before submission
 * - Comprehensive error reporting
 * - Request and response logging
 * - Automatic token refresh handling via Auth integration
 * 
 * @note This client reuses the token management functionality from the Auth class
 * and does not handle token acquisition directly.
 * 
 * @warning The STKPushClient instance requires the Auth reference to remain valid
 * throughout its lifecycle. Destroying the Auth instance while the STKPushClient
 * is still in use will result in undefined behavior.
 */

#pragma once

#include "request.hpp"
#include "response.hpp"
#include "validation/timestamp.hpp"
#include "validation/validator.hpp"
#include "../auth.h"
#include <memory>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <string>
#include <nlohmann/json.hpp>
#include <future>
#include <atomic>

namespace mpesa::stk {

/**
 * @brief Generic result wrapper for asynchronous operations
 * 
 * Provides a type-safe wrapper for operation results that includes
 * success/failure status and error information. Used to return results
 * from asynchronous operations via std::future.
 * 
 * @tparam T The type of the successful result
 */
template<typename T>
class Result {
public:
    /**
     * @brief Construct a successful result
     * @param value The successful result value
     */
    explicit Result(T value) : value_(std::move(value)), success_(true) {}

    /**
     * @brief Construct a failed result
     * @param error Description of the failure
     */
    explicit Result(std::string error) : error_(std::move(error)), success_(false) {}

    /**
     * @brief Check if operation was successful
     * @return true if operation succeeded
     */
    bool success() const noexcept { return success_; }

    /**
     * @brief Get the success value
     * @return Reference to the contained value
     * @throws std::runtime_error if operation failed
     * 
     * @note Always check success() before calling this method to avoid exceptions
     */
    const T& value() const {
        if (!success_) throw std::runtime_error("Cannot access value of failed result");
        return value_;
    }

    /**
     * @brief Get the error message
     * @return Reference to the error message
     * @throws std::runtime_error if operation succeeded
     * 
     * @note Always check !success() before calling this method to avoid exceptions
     */
    const std::string& error() const {
        if (success_) throw std::runtime_error("Cannot access error of successful result");
        return error_;
    }

private:
    T value_;               ///< Success value
    std::string error_;     ///< Error message
    bool success_;          ///< Operation status
};

/**
 * @brief Main STK Push client
 * 
 * Handles all STK Push related operations including request validation,
 * API communication, and response processing. Thread-safe and supports
 * asynchronous operations.
 * 
 * This client depends on an instance of mpesa::Auth for authentication tokens
 * which are used on each API request. The Auth instance must remain valid
 * throughout the lifetime of this client.
 */
class STKPushClient {
public:
    /**
     * @brief Construct a new STK Push client
     * @param auth Reference to an authenticated Auth instance
     * 
     * @note The Auth instance must remain valid for the lifetime of this client.
     * The client does not take ownership of the Auth object, only holds a reference.
     * 
     * @note This constructor initializes internal CURL resources and generates a timestamp
     * that will be used for requests. A new timestamp will not be generated for each request.
     */
    explicit STKPushClient(const ::mpesa::Auth& auth);

    /**
     * @brief Clean up resources
     * 
     * Releases CURL resources and performs cleanup.
     */
    ~STKPushClient();

    // Prevent copying to avoid resource management issues
    STKPushClient(const STKPushClient&) = delete;
    STKPushClient& operator=(const STKPushClient&) = delete;

    /**
     * @brief Initiate an STK Push request asynchronously
     * @param request The STK Push request parameters (passed by value to allow async processing)
     * @return Future containing the result of the operation
     * 
     * This method launches an asynchronous task to:
     * 1. Validate the request parameters
     * 2. Generate the security password using the timestamp and passkey
     * 3. Fetch an access token from the Auth component
     * 4. Construct and send the API request
     * 5. Process the response
     * 
     * The method returns immediately with a future that will eventually
     * contain either a successful response or an error message.
     * 
     * @note This method is thread-safe and can be called from multiple threads.
     * @note The timestamp used for this request is set during client construction,
     *       not per-request. For applications that need to send requests over
     *       extended periods, consider creating a new client periodically.
     * 
     * Example usage:
     * @code
     * auto future = client.initiateSTKPush(request);
     * auto status = future.wait_for(std::chrono::seconds(30));
     * if (status == std::future_status::ready) {
     *     auto result = future.get();
     *     if (result.success()) {
     *         // Handle successful response
     *     } else {
     *         // Handle error
     *     }
     * }
     * @endcode
     */
    std::future<Result<STKPushResponse>> initiateSTKPush(STKPushRequest request);
    
    /**
     * @brief Generate password for STK Push request
     * @param businessShortCode The business short code from request
     * @param passkey The STK passkey from auth config
     * @param timestamp The current timestamp
     * @return Base64 encoded password string
     * 
     * Creates the password required for STK Push requests by concatenating
     * the business short code, passkey, and timestamp, then Base64 encoding
     * the result.
     * 
     * @note Password generation follows the M-Pesa API requirement:
     * base64_encode(businessShortCode + passkey + timestamp)
     */
    std::string generatePassword(
        const std::string& businessShortCode,
        const std::string& passkey,
        const std::string& timestamp);
    
    /**
     * @brief Base64 encode a string
     * @param input String to encode
     * @return Base64 encoded string
     * @throws std::runtime_error if OpenSSL operations fail
     * 
     * Performs Base64 encoding using OpenSSL. Handles empty strings
     * gracefully by returning an empty string.
     */
    static std::string Base64Encode(const std::string& input);

    /**
     * @brief Get count of successful requests
     * @return Number of successful requests
     * 
     * Thread-safe counter of successful STK Push requests.
     */
    int getSuccessCount() const noexcept { return successCount_.load(); }

    /**
     * @brief Get count of failed requests
     * @return Number of failed requests
     * 
     * Thread-safe counter of failed STK Push requests.
     */
    int getFailureCount() const noexcept { return failureCount_.load(); }

private:
    const ::mpesa::Auth& auth_;                   ///< Authentication reference
    std::string timestamp_ = validation::TimestampGenerator::generate();  ///< Current timestamp

    /// STK Push API endpoint
    static constexpr const char* STKPUSH_AUTH_ENDPOINT = "/mpesa/stkpush/v1/processrequest";

    /// Success/failure counters
    std::atomic<int> successCount_{0};
    std::atomic<int> failureCount_{0};

    /**
     * @brief Create Authorization header
     * @return Header string with bearer token
     * 
     * Creates the Authorization header for API requests using the
     * token obtained from the Auth component.
     * 
     * @note This method fetches a fresh token on each call via auth_.getAccessToken(),
     * which may trigger a token refresh if the current token is expired.
     */
    std::string createStkPushAuthHeader() const;

    /**
     * @brief Log a message
     * @param message Message to log
     * 
     * Internal logging utility used for debugging and audit purposes.
     */
    void log(const std::string& message) const;
};

} // namespace mpesa::stk