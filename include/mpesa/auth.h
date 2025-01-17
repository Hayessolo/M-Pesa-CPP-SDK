#ifndef MPESA_AUTH_H
#define MPESA_AUTH_H

#include <string>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <mutex>
#include <filesystem>

namespace mpesa {

// Enum representing various authentication error codes
enum class AuthErrorCode {
    SUCCESS = 0,             // Operation successful
    INVALID_CREDENTIALS = 100, // Incorrect API credentials provided
    NETWORK_ERROR = 200,     // Network-related error (e.g., no internet)
    TOKEN_EXPIRED = 300,     // Token has expired
    CONFIG_ERROR = 400,      // Configuration-related issue
    SERVER_ERROR = 500       // Server responded with an error
};

// Structure to hold the authentication response from M-Pesa API
struct AuthResponse {
    std::string access_token;                // The access token used for API authentication
    std::chrono::seconds expires_in{3600};   // Duration (in seconds) until the token expires
    AuthErrorCode error_code{AuthErrorCode::SUCCESS}; // The error code (if any) from the operation
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
    std::string consumer_key;                // The API consumer key
    std::string consumer_secret;             // The API consumer secret
    bool sandbox{true};                      // Flag to indicate sandbox (test) mode

    // Load configuration from a JSON file
    static AuthConfig from_file(const std::string& path);

    // Load configuration from environment variables
    static AuthConfig fromEnv();
};

// The main class handling M-Pesa API authentication
class Auth {
public:
    // Constructor accepting individual configuration parameters
    explicit Auth(std::string consumer_key, 
                  std::string consumer_secret,
                  bool sandbox = true);

    // Constructor accepting an AuthConfig structure
    explicit Auth(const AuthConfig& config);

    // Retrieve the current access token (fetches a new one if expired)
    std::string getAccessToken();

    // Check if the current access token is still valid
    bool isTokenValid() const;

    // Request a new access token from the M-Pesa API
    AuthResponse refreshToken();

    // Retrieve the base URL for the API based on the environment (sandbox/production)
    std::string getBaseUrl() const;

    // Get the last error code encountered by the Auth instance
    AuthErrorCode getLastError() const;

private:
    AuthConfig config_;                      // Stores the API configuration settings

    // Token management members
    std::optional<std::string> current_token; // Correct syntax for optional

    std::chrono::system_clock::time_point token_expiry_; // Token expiry time
    AuthErrorCode last_error_{AuthErrorCode::SUCCESS}; // The last error code encountered
    mutable std::mutex token_mutex_;         // Mutex for thread-safe access to token data

    // Environment-specific API URLs
    static constexpr const char* SANDBOX_URL = "https://sandbox.safaricom.co.ke";
    static constexpr const char* PRODUCTION_URL = "https://api.safaricom.co.ke";
    static constexpr const char* AUTH_ENDPOINT = "/oauth/v1/generate";

    // Helper methods
    std::string createAuthHeader() const;    // Generate the "Authorization" header using Base64 encoding
    void updateLastError(AuthErrorCode code); // Update the last error code encountered
};

} // namespace mpesa

#endif // MPESA_AUTH_H