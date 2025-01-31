#include <iostream>
#include <mpesa/auth.h>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>

// Function to print detailed authentication error information
void print_error(const mpesa::AuthenticationError& e) {
    std::cerr << "Authentication Error: " << e.what() << std::endl;
    std::cerr << "Error Code: " << static_cast<int>(e.getErrorCode()) << std::endl;
}

int main() {
    try {
        // Inform the user that the program is attempting to load credentials
        std::cout << "Attempting to load M-PESA credentials from environment variables..." << std::endl;
        
        // Load authentication configuration from environment variables
        // This will throw an error if required environment variables are not set
        //auto config = mpesa::AuthConfig::from_env();

        // Load configuration from .env file
        auto config = mpesa::AuthConfig::from_file("../../../auth_credentials.env");
        
        // Create an Auth instance using the loaded configuration
        mpesa::Auth auth(config);
        
        // Inform the user that the program is requesting an access token
        std::cout << "Requesting access token..." << std::endl;
        
        // Attempt to get an access token
        std::string access_token = auth.getAccessToken();
        
        // Inform the user that the access token was successfully obtained
        std::cout << "Access Token Successfully Obtained!" << std::endl;
        std::cout << "Token: " << access_token.substr(0, 10) << "..." << std::endl;
        
        // Check if the obtained token is valid
        if (auth.isTokenValid()) {
            std::cout << "Token is valid." << std::endl;
        }
        
        // Demonstrate last error checking
        auto last_error = auth.getLastError();
        if (last_error != mpesa::AuthErrorCode::SUCCESS) {
            std::cout << "Last Error Code: " << static_cast<int>(last_error) << std::endl;
        }
        
    } catch (const mpesa::AuthenticationError& e) {
        // Print detailed authentication error information
        print_error(e);
        return 1;
    } catch (const std::exception& e) {
        // Print unexpected error information
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    // Return 0 to indicate successful execution
    return 0;
}