/**
 * @file validator.hpp
 * @brief STK Push Request Validation
 * 
 * Provides validation logic for STK Push request parameters to ensure
 * they meet M-Pesa API requirements before submission.
 */

#pragma once
#include <string>
#include <regex>
#include <string_view>
#include "mpesa/stk/request.hpp"
#include "timestamp.hpp"

namespace mpesa {
namespace validation {

/**
 * @brief Request parameter validator
 * 
 * Validates STK Push request parameters against M-Pesa API requirements,
 * including format validation, length restrictions, and content rules.
 */
class Validator {
public:
    /**
     * @brief Validation result structure
     * 
     * Contains the result of a validation operation, including
     * success/failure status and error message if applicable.
     */
    struct ValidationResult {
        bool isValid;           ///< Whether validation passed
        std::string error;      ///< Error message if validation failed
        
        /**
         * @brief Create successful validation result
         * @return ValidationResult indicating success
         */
        static ValidationResult success() noexcept {
            return {true, ""};
        }
        
        /**
         * @brief Create failed validation result
         * @param message Error message describing the failure
         * @return ValidationResult indicating failure with message
         */
        static ValidationResult failure(std::string message) {
            return {false, std::move(message)};
        }
    };

    /**
     * @brief Validate an STK Push request
     * @param request The request to validate
     * @return ValidationResult indicating success/failure and any error
     * 
     * Validates all fields of an STK Push request, including:
     * - Phone number format
     * - Amount validity
     * - Reference lengths
     * - Required field presence
     */
    static ValidationResult validateSTKPushRequest(const stk::STKPushRequest& request) {
        // Precompiled regex patterns
        static const std::regex business_short_code_regex(R"(^\d{5,6}$)");
        static const std::regex amount_regex(R"(^[1-9]\d*$)");
        static const std::regex phone_regex(R"(^2547\d{8}$)");  // Must start with 254 followed by 7 and 8 more digits
        static const std::regex url_regex(
            R"(^https:\/\/)"  // Must be HTTPS
            R"(([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)+)"  // Domain parts
            R"([a-zA-Z]{2,})"  // TLD
            R"(\/[a-zA-Z0-9\-\._~:\/\?#\[\]@!$&'\(\)\*\+,;=]*$)"  // Path
        );

        // Validate BusinessShortCode
        if (!std::regex_match(request.businessShortCode, business_short_code_regex)) {
            return ValidationResult::failure("Invalid BusinessShortCode format");
        }

        // Validate Password
        if (request.password.empty()) {
            return ValidationResult::failure("Password cannot be empty");
        }

        // Validate Timestamp
        if (!TimestampGenerator::isValid(request.timestamp)) {
            return ValidationResult::failure("Invalid timestamp format");
        }

        // Validate Phone Numbers
        if (!std::regex_match(request.partyA, phone_regex)) {
            return ValidationResult::failure("Invalid PartyA phone number format. Must be 12 digits starting with 2547");
        }

        if (!std::regex_match(request.phoneNumber, phone_regex)) {
            return ValidationResult::failure("Invalid phone number format. Must be 12 digits starting with 2547");
        }

        // Validate PartyB (must match BusinessShortCode for PayBill)
        if (request.partyB != request.businessShortCode) {
            return ValidationResult::failure("PartyB must match BusinessShortCode for PayBill transactions");
        }

        // Validate CallBackURL
        if (!std::regex_match(request.callBackURL, url_regex)) {
            return ValidationResult::failure("Invalid callback URL format. Must be HTTPS with valid domain");
        }

        // Validate Amount (must be positive number)
        if (!std::regex_match(request.amount, amount_regex)) {
            return ValidationResult::failure("Amount must be a positive number");
        }

        // Validate AccountReference length
        if (request.accountReference.empty() || request.accountReference.length() > 12) {
            return ValidationResult::failure("AccountReference must not be empty and cannot exceed 12 characters");
        }

        // Validate TransactionDesc length
        if (request.transactionDesc.empty() || request.transactionDesc.length() > 13) {
            return ValidationResult::failure("TransactionDesc must not be empty and cannot exceed 13 characters");
        }

        return ValidationResult::success();
    }
};

} // namespace validation
} // namespace mpesa