/**
 * @file auth.h
 * @brief Authentication Module for M-Pesa API
 * 
 * This module provides authentication functionality for the M-Pesa API, including:
 * - Access token acquisition and management
 * - Automatic token refresh when expired
 * - Error handling for authentication failures
 * - Configuration management for different environments (sandbox/production)
 * 
 * The Auth class is a key dependency for other SDK components like STKPushClient,
 * providing them with authenticated access to the M-Pesa API endpoints.
 * 
 * @note Thread-safety: The Auth class is thread-safe and can be safely used
 * from multiple threads concurrently.
 */

#ifndef MPESA_AUTH_H
#define MPESA_AUTH_H

#include <memory>
#include <string>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <mutex>
class CurlWrapper;
class TimeProvider;

namespace mpesa {

/**
 * @brief Authentication error codes
 * 
 * Comprehensive set of error codes that can occur during authentication operations.
 * These codes map specific errors to categories to help with troubleshooting and
 * appropriate error handling.
 */
enum class AuthErrorCode {
    SUCCESS = 0,            ///< Operation successful
    
    // Network and Connection Errors (100-199)
    NETWORK_ERROR = 100,    ///< Generic network error
    DNS_ERROR = 101,        ///< DNS resolution failed
    CONNECTION_ERROR = 102, ///< Connection establishment failed
    TIMEOUT_ERROR = 103,    ///< Operation timed out
    SSL_ERROR = 104,        ///< SSL/TLS error
    
    // Authentication Errors (200-299)
    INVALID_CREDENTIALS = 200,  ///< Invalid API credentials (401.002.01)
    INVALID_GRANT_TYPE = 201,   ///< Invalid grant type passed (400.008.02)
    INVALID_AUTH_TYPE = 202,    ///< Invalid Authentication type (400.008.01)
    TOKEN_EXPIRED = 203,        ///< Token has expired
    
    // Server Errors (300-399)
    SERVER_ERROR = 300,    ///< Generic server error (500.001.1001)
    HTTP_ERROR = 301,      ///< HTTP error response
    API_ERROR = 302,       ///< M-Pesa API specific error (unknown error codes)
    
    // Client Errors (400-499)
    INITIALIZATION_ERROR = 400, ///< Failed to initialize CURL
    CONFIG_ERROR = 401,        ///< Configuration error
    PARSE_ERROR = 402,         ///< JSON parsing error
    
    // System Errors (500-599)
    INTERNAL_ERROR = 500      ///< Internal system error
};

/**
 * @brief Authentication response structure
 * 
 * Contains the result of an authentication operation, including the access token
 * and its expiration period, or error information if the operation failed.
 */
struct AuthResponse {
    std::string access_token;                    ///< The access token used for API authentication
    std::chrono::seconds expires_in{3600};       ///< Duration until token expires
    AuthErrorCode error_code{AuthErrorCode::SUCCESS}; ///< Error code from operation
};

/**
 * @brief Authentication exception
 * 
 * Exception thrown when authentication operations fail. Contains both an error
 * message and a specific error code to help with programmatic error handling.
 */
class AuthenticationError : public std::runtime_error {
public:
    /**
     * @brief Construct a new Authentication Error exception
     * @param message Descriptive error message
     * @param code Specific error code
     */
    AuthenticationError(const std::string& message, AuthErrorCode code) 
        : std::runtime_error(message), error_code_(code) {}

    /**
     * @brief Get the error code
     * @return AuthErrorCode The specific error code
     */
    AuthErrorCode getErrorCode() const { return error_code_; }

private:
    AuthErrorCode error_code_; ///< The specific error code for the exception
};

/**
 * @brief Authentication configuration
 * 
 * Stores API credentials and environment settings required for authentication.
 * Can be loaded from a configuration file or environment variables.
 */
struct AuthConfig {
    std::string consumer_key;     ///< The API consumer key
    std::string consumer_secret;  ///< The API consumer secret
    bool sandbox{true};           ///< Flag to indicate sandbox (test) mode
    std::string stk_passkey;      ///< The STK passkey for generating transaction passwords

    /**
     * @brief Load configuration from a JSON file
     * @param path Path to the configuration file
     * @return AuthConfig Populated configuration structure
     * @throws std::runtime_error If file cannot be read or has invalid format
     * 
     * The file should contain a JSON object with the following structure:
     * {
     *   "consumer_key": "your_consumer_key",
     *   "consumer_secret": "your_consumer_secret",
     *   "sandbox": true,
     *   "stk_passkey": "your_stk_passkey"
     * }
     */
    static AuthConfig from_file(const std::string& path);

    /**
     * @brief Load configuration from environment variables
     * @return AuthConfig Populated configuration structure
     * @throws std::runtime_error If required variables are not set
     * 
     * Looks for the following environment variables:
     * - MPESA_CONSUMER_KEY: API consumer key
     * - MPESA_CONSUMER_SECRET: API consumer secret
     * - MPESA_ENVIRONMENT: "sandbox" or "production"
     * - MPESA_STK_PASSKEY: STK passkey for transactions
     */
    static AuthConfig from_env();
};

/**
 * @brief M-Pesa API authentication manager
 * 
 * Handles authentication with the M-Pesa API, including token acquisition,
 * automatic refreshing of expired tokens, and error handling.
 * 
 * This class is a core dependency for other SDK components that need
 * authenticated access to M-Pesa API endpoints.
 * 
 * @note Thread-safety: This class is thread-safe and can be safely shared
 * across multiple threads.
 */
class Auth {
public:
    /**
     * @brief Construct with individual parameters
     * @param consumer_key API consumer key
     * @param consumer_secret API consumer secret
     * @param stk_passkey STK passkey for transactions
     * @param sandbox Whether to use sandbox environment (default: true)
     */
    explicit Auth(std::string consumer_key, std::string consumer_secret, 
                 std::string stk_passkey, bool sandbox = true);

    /**
     * @brief Construct with configuration structure
     * @param config Authentication configuration
     */
    explicit Auth(const AuthConfig& config);

    /**
     * @brief Advanced constructor with dependency injection
     * @param config Authentication configuration
     * @param curl Custom CURL implementation (for testing)
     * @param time Custom time provider (for testing)
     * 
     * This constructor is primarily used for testing with mock dependencies.
     */
    Auth(const AuthConfig& config, 
         std::shared_ptr<CurlWrapper> curl,
         std::shared_ptr<TimeProvider> time)
        : config_(config)
        , curl_(std::move(curl))
        , time_(std::move(time)) {}

    /**
     * @brief Get the current access token
     * @return Current valid access token
     * @throws AuthenticationError If token acquisition fails
     * 
     * This method automatically refreshes the token if expired.
     * It is thread-safe and can be called from multiple threads.
     * 
     * @note This is the primary method used by other SDK components
     * (like STKPushClient) to obtain an authenticated token for API requests.
     */
    std::string getAccessToken() const;

    /**
     * @brief Check if current token is valid
     * @return true if token exists and not expired
     */
    bool isTokenValid() const;

    /**
     * @brief Get the API base URL
     * @return Base URL for API requests
     * 
     * Returns either sandbox or production URL based on configuration.
     */
    std::string getBaseUrl() const;

    /**
     * @brief Get the current configuration
     * @return Current AuthConfig
     */
    const AuthConfig& getConfig() const { return config_; }

    /**
     * @brief Get the most recent error code
     * @return Last recorded error code
     */
    AuthErrorCode getLastError() const;

    /**
     * @brief Authentication API endpoint
     * 
     * The endpoint path for token requests, appended to the base URL.
     */
    static constexpr const char* AUTH_ENDPOINT = "/oauth/v1/generate";

    /**
     * @brief Sandbox environment base URL
     */
    static constexpr const char* SANDBOX_URL = "https://sandbox.safaricom.co.ke";
    
    /**
     * @brief Production environment base URL
     */
    static constexpr const char* PRODUCTION_URL = "https://api.safaricom.co.ke";

private:
    /**
     * @brief Refresh the access token
     * @return Authentication response with token or error
     * 
     * Makes an API request to obtain a new access token.
     * Updates internal token state on success.
     */
    AuthResponse refreshToken();

    /**
     * @brief Create authentication header
     * @return Basic auth header string
     * 
     * Creates the Authorization header with Base64-encoded credentials.
     */
    std::string createAuthHeader() const;

    /**
     * @brief Update the last error code
     * @param code Error code to store
     */
    void updateLastError(AuthErrorCode code);

    AuthConfig config_;                           ///< Authentication configuration
    mutable std::optional<std::string> current_token_; ///< Current access token
    mutable std::chrono::system_clock::time_point token_expiry_; ///< Token expiration time
    mutable std::mutex token_mutex_;             ///< Mutex for thread safety
    mutable AuthErrorCode last_error_{AuthErrorCode::SUCCESS}; ///< Last error code
    
    // Injected dependencies (or default implementations)
    std::shared_ptr<CurlWrapper> curl_;          ///< HTTP client
    std::shared_ptr<TimeProvider> time_;         ///< Time provider
};

} // namespace mpesa
#endif // MPESA_AUTH_H