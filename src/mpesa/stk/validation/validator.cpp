/**
 * @file validator.cpp
 * @brief STK Push Request Validation Implementation
 * 
 * Implements validation logic for STK Push request parameters to ensure
 * they conform to M-Pesa API requirements before submission.
 */

#include "mpesa/stk/validation/validator.hpp"
#include <regex>

namespace mpesa {
namespace validation {

ValidationResult Validator::validateSTKPushRequest(const stk::STKPushRequest& request) {
    // Precompiled regex patterns for better performance
    static const std::regex business_short_code_regex(R"(^\d{5,6}$)");
    static const std::regex phone_regex(R"(^254\d{9}$)");
    static const std::regex amount_regex(R"(^[1-9]\d*$)");
    static const std::regex url_regex(
        R"(^https?://)"
        R"(([a-zA-Z0-9]([a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.)+[a-zA-Z]{2,6})"
        R"(/[a-zA-Z0-9\-\._~:/\?#\[\]@!$&'\(\)\*\+,;=]*$)"
    );

    // Validate BusinessShortCode format (5-6 digits)
    if (!std::regex_match(request.businessShortCode, business_short_code_regex)) {
        return ValidationResult::failure("Invalid BusinessShortCode format - must be 5-6 digits");
    }

    // Validate Amount format (positive integer)
    if (!std::regex_match(request.amount, amount_regex)) {
        return ValidationResult::failure("Invalid amount format - must be positive integer");
    }

    // Validate Phone Numbers (254XXXXXXXXX format)
    if (!std::regex_match(request.partyA, phone_regex)) {
        return ValidationResult::failure("Invalid PartyA phone number - must be format: 254XXXXXXXXX");
    }

    if (!std::regex_match(request.phoneNumber, phone_regex)) {
        return ValidationResult::failure("Invalid phone number - must be format: 254XXXXXXXXX");
    }

    // Validate PartyB matches BusinessShortCode
    if (!std::regex_match(request.partyB, business_short_code_regex)) {
        return ValidationResult::failure("Invalid PartyB format - must match BusinessShortCode format");
    }

    // Validate CallbackURL format
    if (!std::regex_match(request.callBackURL, url_regex)) {
        return ValidationResult::failure("Invalid callback URL format");
    }

    // Validate field lengths
    if (!validate_account_reference_length(request.accountReference)) {
        return ValidationResult::failure("AccountReference exceeds maximum length of 12 characters");
    }

    if (!validate_transaction_desc_length(request.transactionDesc)) {
        return ValidationResult::failure("TransactionDesc exceeds maximum length of 13 characters");
    }

    return ValidationResult::success();
}

} // namespace validation
} // namespace mpesa