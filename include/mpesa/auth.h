#ifndef MPESA_AUTH_H
#define MPESA_AUTH_H

#include <string>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <mutex>

namespace mpesa {
// Enum representing various authentication error codes
enum class AuthErrorCode {
    SUCCESS = 0,            // Operation successful
    
    // Network and Connection Errors (100-199)
    NETWORK_ERROR = 100,    // Generic network error
    DNS_ERROR = 101,        // DNS resolution failed
    CONNECTION_ERROR = 102, // Connection establishment failed
    TIMEOUT_ERROR = 103,    // Operation timed out
    SSL_ERROR = 104,        // SSL/TLS error
    
    // Authentication Errors (200-299)
    INVALID_CREDENTIALS = 200,  // Invalid API credentials (401.002.01)
    INVALID_GRANT_TYPE = 201,   // Invalid grant type passed (400.008.02)
    INVALID_AUTH_TYPE = 202,    // Invalid Authentication type (400.008.01)
    TOKEN_EXPIRED = 203,        // Token has expired
    
    // Server Errors (300-399)
    SERVER_ERROR = 300,    // Generic server error (500.001.1001)
    HTTP_ERROR = 301,      // HTTP error response
    API_ERROR = 302,       // M-Pesa API specific error (unknown error codes)
    
    // Client Errors (400-499)
    INITIALIZATION_ERROR = 400, // Failed to initialize CURL
    CONFIG_ERROR = 401,        // Configuration error
    PARSE_ERROR = 402,         // JSON parsing error
    
    // System Errors (500-599)
    INTERNAL_ERROR = 500      // Internal system error
};

// Structure to hold the authentication response from M-Pesa API
struct AuthResponse {
    std::string access_token;                    // The access token used for API authentication
    std::chrono::seconds expires_in{3600};       // Duration until token expires
    AuthErrorCode error_code{AuthErrorCode::SUCCESS}; // Error code from operation
};

// Custom exception class for authentication-related errors
class AuthenticationError : public std::runtime_error {
public:
    // Constructor accepting an error message and error code
    AuthenticationError(const std::string& message, AuthErrorCode code) 
        : std::runtime_error(message), error_code_(code) {}

    // Retrieve the error code associated with the exception
    AuthErrorCode getErrorCode() const { return error_code_; }

private:
    AuthErrorCode error_code_; // The specific error code for the exception
};

// Configuration structure for storing API credentials and environment settings
struct AuthConfig {
    std::string consumer_key;     // The API consumer key
    std::string consumer_secret;  // The API consumer secret
    bool sandbox{true};          // Flag to indicate sandbox (test) mode

    // Load configuration from a JSON file
    static AuthConfig from_file(const std::string& path);

    // Load configuration from environment variables
    static AuthConfig from_env();
};

// The main class handling M-PESA API authentication
class Auth {
public:

    // Constructor accepting individual configuration parameters
    explicit Auth(std::string consumer_key, std::string consumer_secret, bool sandbox = true);

    // Constructor accepting an AuthConfig structure
    explicit Auth(const AuthConfig& config);
    
    // Retrieve the current access token (fetches a new one if expired)
    std::string getAccessToken();

    // Check if the current access token is still valid
    bool isTokenValid() const;

    // Request a new access token from the M-PESA API
    AuthResponse refreshToken();

    // Retrieve the base URL for the API based on the environment (sandbox/production)
    std::string getBaseUrl() const;

    // Get the last error code encountered by the Auth instance
    AuthErrorCode getLastError() const;

private:

    AuthConfig config_;        // Stores the API configuration settings
    
    // Token management members
    std::optional<std::string> current_token_;  // Current access token (if available)
    std::chrono::system_clock::time_point token_expiry_; // Token expiry time
    AuthErrorCode last_error_{AuthErrorCode::SUCCESS};  // Last error code encountered
    mutable std::mutex token_mutex_;            // Mutex for thread-safe access to token data

    // Environment-specific API URLs
    static constexpr const char* SANDBOX_URL = "https://sandbox.safaricom.co.ke";
    static constexpr const char* PRODUCTION_URL = "https://api.safaricom.co.ke";
    static constexpr const char* AUTH_ENDPOINT = "/oauth/v1/generate";

    // Helper methods
    std::string createAuthHeader() const;    // Generate the "Authorization" header
    void updateLastError(AuthErrorCode code); // Update the last error code
};

} // namespace mpesa

#endif // MPESA_AUTH_H   