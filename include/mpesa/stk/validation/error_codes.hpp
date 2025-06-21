/**
 * @file error_codes.hpp
 * @brief STK Push Error Codes and Descriptions
 * 
 * Defines the complete set of error codes that can be returned by the M-Pesa API
 * during STK Push transactions, along with helper utilities for error handling.
 */

#pragma once
#include <string>
#include <type_traits>

namespace mpesa {
namespace validation {

/**
 * @brief Error codes for STK Push transactions
 * 
 * Comprehensive set of error codes that can be returned by the M-Pesa API
 * during STK Push operations. Each code maps to a specific failure scenario.
 */
enum class STKPushErrorCode : int {
    Success = 0,              ///< Transaction completed successfully
    
    // Balance Errors
    InsufficientBalance = 1,  ///< Customer has insufficient funds
    
    // Authorization Errors
    InvalidInitiator = 2001,  ///< Invalid API credentials
    
    // Transaction State Errors
    TransactionExpired = 1019,  ///< Customer did not respond in time
    SubscriberLocked = 1001,    ///< Customer account is locked
    UserCanceled = 1032,        ///< Customer rejected the transaction
    
    // System Errors
    PushRequestError = 1025,    ///< Failed to send push request
    SystemError = 9999,         ///< Generic system error
    
    // Timeout Errors
    DSTimeout = 1037,           ///< Delivery system timeout
    
    // Unknown Error
    Unknown = -1                ///< Unrecognized error code
};

/**
 * @brief Convert error code to integer
 * @param code STKPushErrorCode to convert
 * @return Integer representation of the error code
 */
constexpr int to_int(STKPushErrorCode code) noexcept {
    return static_cast<int>(code);
}

/**
 * @brief Convert integer to error code
 * @param code Integer error code
 * @return Corresponding STKPushErrorCode
 */
constexpr STKPushErrorCode from_int(int code) noexcept {
    switch (code) {
        case 0:    return STKPushErrorCode::Success;
        case 1:    return STKPushErrorCode::InsufficientBalance;
        case 2001: return STKPushErrorCode::InvalidInitiator;
        case 1019: return STKPushErrorCode::TransactionExpired;
        case 1001: return STKPushErrorCode::SubscriberLocked;
        case 1032: return STKPushErrorCode::UserCanceled;
        case 1025: return STKPushErrorCode::PushRequestError;
        case 9999: return STKPushErrorCode::SystemError;
        case 1037: return STKPushErrorCode::DSTimeout;
        default:   return STKPushErrorCode::Unknown;
    }
}

/**
 * @brief Error description utilities
 * 
 * Provides human-readable descriptions for STK Push error codes.
 */
class ErrorDescription {
public:
    /**
     * @brief Get description for error code
     * @param code STKPushErrorCode to describe
     * @return Human-readable description of the error
     */
    static constexpr const char* getDescription(STKPushErrorCode code) noexcept {
        switch (code) {
            case STKPushErrorCode::Success:
                return "The service request is processed successfully.";
            case STKPushErrorCode::InsufficientBalance:
                return "The balance is insufficient for the transaction.";
            case STKPushErrorCode::InvalidInitiator:
                return "The initiator information is invalid.";
            case STKPushErrorCode::TransactionExpired:
                return "Transaction has expired.";
            case STKPushErrorCode::SubscriberLocked:
                return "Unable to lock subscriber, a transaction is already in process.";
            case STKPushErrorCode::UserCanceled:
                return "The request was canceled by the user.";
            case STKPushErrorCode::PushRequestError:
                return "An error occurred while sending a push request.";
            case STKPushErrorCode::SystemError:
                return "A system error occurred.";
            case STKPushErrorCode::DSTimeout:
                return "DS timeout, user cannot be reached.";
            default:
                return "Unknown error occurred.";
        }
    }
};

} // namespace validation
} // namespace mpesa