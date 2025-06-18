/**
 * @file response.hpp
 * @brief STK Push Response and Callback Structures
 * 
 * Defines the response structures for both the initial STK Push request and
 * the subsequent transaction callback. These structures encapsulate all possible
 * response data from the M-Pesa API.
 */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <algorithm>
#include <optional>
#include "validation/error_codes.hpp"

namespace mpesa {
namespace stk {

/**
 * @brief Initial STK Push API response
 * 
 * Contains the response data received immediately after initiating an
 * STK Push request. This indicates whether the push was successfully
 * sent to the user's phone.
 */
struct STKPushResponse {
    std::string merchantRequestID;     ///< Unique merchant request identifier
    std::string checkoutRequestID;     ///< Unique checkout request identifier
    std::string responseCode;          ///< Response status code
    std::string responseDescription;   ///< Human readable response description
    std::string customerMessage;       ///< Message displayed to customer
};

/**
 * @brief Metadata item in transaction callback
 * 
 * Individual piece of metadata returned in the transaction callback.
 * Can contain different types of values (string/number).
 */
struct CallbackMetadataItem {
    std::string name;    ///< Metadata field name
    std::variant<std::string, double, int64_t> value;  ///< Field value of varying type
};

/**
 * @brief Transaction result callback structure
 * 
 * Complete callback data received after the customer responds to the
 * STK Push prompt on their phone. Contains the transaction result
 * and associated metadata.
 */
struct STKCallback {
    std::string merchantRequestID;     ///< Matches initial request ID
    std::string checkoutRequestID;     ///< Matches initial checkout ID
    validation::STKPushErrorCode resultCode;  ///< Transaction result code
    std::string resultDesc;            ///< Result description
    std::optional<std::vector<CallbackMetadataItem>> callbackMetadata;  ///< Optional transaction metadata

    /**
     * @brief Get the transaction amount
     * @return Optional containing amount if available
     */
    std::optional<double> getAmount() const;

    /**
     * @brief Get the M-Pesa receipt number
     * @return Optional containing receipt number if available
     */
    std::optional<std::string> getMpesaReceiptNumber() const;

    /**
     * @brief Get the transaction date
     * @return Optional containing timestamp if available
     */
    std::optional<int64_t> getTransactionDate() const;

    /**
     * @brief Get the customer phone number
     * @return Optional containing phone number if available
     */
    std::optional<std::string> getPhoneNumber() const;

private:
    /**
     * @brief Generic metadata value getter
     * @tparam T Expected type of the metadata value
     * @param name Name of the metadata field
     * @return Optional containing value if found and of correct type
     */
    template<typename T>
    std::optional<T> getMetadataValue(const std::string& name) const;
};

} // namespace stk
} // namespace mpesa