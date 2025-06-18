/**
 * @file request.hpp
 * @brief STK Push (Lipa Na M-Pesa Online) Request Definitions
 * 
 * Defines structures and types for initiating STK Push requests to the M-Pesa API.
 * STK Push allows merchants to prompt customers for payment through their mobile phones.
 * 
 * This file provides:
 * 1. The STKPushRequest structure with all required API parameters
 * 2. Transaction type enumeration for different transaction categories
 * 3. Phone number formatting utilities
 * 4. JSON configuration file loading functionality
 * 
 * @note The structures in this file are designed to match the M-Pesa API
 * specification exactly, including field naming conventions and data formats.
 */

#pragma once

#include <string>
#include <optional>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace mpesa::stk {

/**
 * @brief Transaction type for STK Push
 * 
 * Defines the types of transactions that can be performed with STK Push.
 * The type affects how the transaction appears on the customer's statement
 * and may have different processing rules.
 */
enum class TransactionType {
    CustomerPayBillOnline,    ///< Payment to a business PayBill account
    CustomerBuyGoodsOnline    ///< Payment to a business Till number
};

/**
 * @brief Convert TransactionType to string representation
 * @param type TransactionType enum value
 * @return String representation for API requests
 */
inline std::string transactionTypeToString(TransactionType type) {
    switch (type) {
        case TransactionType::CustomerPayBillOnline:
            return "CustomerPayBillOnline";
        case TransactionType::CustomerBuyGoodsOnline:
            return "CustomerBuyGoodsOnline";
        default:
            return "CustomerPayBillOnline"; // Default to PayBill
    }
}

/**
 * @brief Format and validate a phone number for M-Pesa API
 * @param phone Phone number to format
 * @return Formatted phone number with country code (254XXXXXXXXX)
 * @throws std::runtime_error if phone number is invalid
 * 
 * Formats a phone number to meet the M-Pesa API requirements:
 * - Must be in the format 254XXXXXXXXX (9 digits after 254)
 * - Removes any non-digit characters
 * - Handles numbers with or without the country code
 * - Converts +254 or 0 prefixes to 254
 * 
 * @note The M-Pesa API is strict about phone number format. This function
 * ensures that phone numbers comply with the required format.
 */
inline std::string formatPhoneNumber(const std::string& phone) {
    std::string formatted = phone;
    
    // Remove any non-digit characters
    formatted.erase(
        std::remove_if(formatted.begin(), formatted.end(), 
                      [](unsigned char c) { return !std::isdigit(c); }),
        formatted.end()
    );
    
    // Handle Kenyan phone number formats
    if (formatted.substr(0, 3) == "254") {
        // Already in international format, do nothing
    } else if (formatted.substr(0, 1) == "0") {
        // Convert 0XX... to 254XX...
        formatted = "254" + formatted.substr(1);
    } else if (formatted.length() == 9) {
        // Assume it's a number without prefix, add 254
        formatted = "254" + formatted;
    } else {
        throw std::runtime_error("Invalid phone number format. Expected format: 254XXXXXXXXX");
    }
    
    // Final validation: Must be exactly 12 digits (254 + 9 digits)
    if (formatted.length() != 12) {
        throw std::runtime_error("Invalid phone number length. Must be 12 digits in format 254XXXXXXXXX");
    }
    
    return formatted;
}

/**
 * @brief Validate account reference length
 * @param reference Account reference to validate
 * @return true if valid, false otherwise
 * 
 * Account reference must be between 1 and 12 characters.
 */
inline bool validate_account_reference_length(const std::string& reference) {
    return reference.length() > 0 && reference.length() <= 12;
}

/**
 * @brief Validate transaction description length
 * @param desc Transaction description to validate
 * @return true if valid, false otherwise
 * 
 * Transaction description must be between 1 and 13 characters.
 */
inline bool validate_transaction_desc_length(const std::string& desc) {
    return desc.length() > 0 && desc.length() <= 13;
}

/**
 * @brief STK Push transaction request parameters
 * 
 * Contains all required and optional parameters for initiating an STK Push
 * request through the M-Pesa API. This prompts the customer's phone with
 * a payment authorization dialog.
 * 
 * @note This structure is serialized to JSON when sent to the M-Pesa API.
 * The field names match the API's expected parameter names exactly.
 */
struct STKPushRequest {
    /**
     * @brief Organization's shortcode (5-6 digits)
     * 
     * This is the organization's PayBill or Till Number that customers
     * use to make payments. Must be 5-6 digits.
     * 
     * @required Yes
     */
    std::string businessShortCode;

    /**
     * @brief Encrypted security credential
     * 
     * Base64 encoded string of BusinessShortCode + Passkey + Timestamp.
     * This is automatically generated by STKPushClient::generatePassword().
     * 
     * @required Yes
     * @note This is typically not set directly but generated from other parameters
     */
    std::string password;
    
    /**
     * @brief Request timestamp (YYYYMMDDHHMMSS)
     * 
     * This is a timestamp of the transaction in the format YYYYMMDDHHMMSS.
     * This is automatically generated by TimestampGenerator::generate().
     * 
     * @required Yes
     * @note This is typically not set directly but generated automatically
     */
    std::string timestamp;
    
    /**
     * @brief Type of transaction
     * 
     * Specifies whether this is a PayBill (CustomerPayBillOnline) or
     * Till Number (CustomerBuyGoodsOnline) transaction.
     * 
     * @required Yes
     */
    TransactionType transactionType = TransactionType::CustomerPayBillOnline;
    
    /**
     * @brief Transaction amount
     * 
     * The amount to be charged. Must be a positive integer.
     * 
     * @required Yes
     */
    std::string amount;
    
    /**
     * @brief Customer's phone number
     * 
     * The phone number making the payment. Must be in format 254XXXXXXXXX.
     * 
     * @required Yes
     */
    std::string partyA;
    
    /**
     * @brief Organization's shortcode receiving payment
     * 
     * This is typically the same as businessShortCode. Must be 5-6 digits.
     * 
     * @required Yes
     */
    std::string partyB;
    
    /**
     * @brief Phone number to receive STK PIN prompt
     * 
     * The mobile number to receive the STK prompt. Usually same as partyA.
     * Must be in format 254XXXXXXXXX.
     * 
     * @required Yes
     */
    std::string phoneNumber;
    
    /**
     * @brief Callback URL for transaction result
     * 
     * A valid secure URL that will receive transaction completion notification.
     * 
     * @required Yes
     */
    std::string callBackURL;
    
    /**
     * @brief Account reference
     * 
     * An identifier of the transaction for the CustomerPayBillOnline transaction type.
     * Maximum of 12 characters.
     * 
     * @required Yes
     */
    std::string accountReference;
    
    /**
     * @brief Transaction description
     * 
     * A description of the transaction. Maximum of 13 characters.
     * 
     * @required Yes
     */
    std::string transactionDesc;

    /**
     * @brief Serialize the request to JSON
     * @return JSON object containing all request parameters
     * 
     * Converts transaction type enum to string and includes all fields.
     */
    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"BusinessShortCode", businessShortCode},
            {"Password", password},
            {"Timestamp", timestamp},
            {"TransactionType", transactionTypeToString(transactionType)},
            {"Amount", amount},
            {"PartyA", partyA},
            {"PartyB", partyB},
            {"PhoneNumber", phoneNumber},
            {"CallBackURL", callBackURL},
            {"AccountReference", accountReference},
            {"TransactionDesc", transactionDesc}
        };
        return j;
    }
};

/**
 * @brief Load an STK Push request from a JSON file
 * @param filepath Path to the JSON configuration file
 * @return Populated STKPushRequest structure
 * @throws std::runtime_error if file cannot be read or parsed
 * 
 * The JSON file should contain fields that match the STKPushRequest structure.
 * This function will attempt to format phone numbers and validate other fields.
 * 
 * Example JSON file:
 * @code
 * {
 *   "BusinessShortCode": "174379",
 *   "Amount": "1",
 *   "PartyA": "254712345678",
 *   "PartyB": "174379",
 *   "PhoneNumber": "254712345678",
 *   "CallBackURL": "https://example.com/callback",
 *   "AccountReference": "Test",
 *   "TransactionDesc": "Test Payment",
 *   "TransactionType": "CustomerPayBillOnline"
 * }
 * @endcode
 * 
 * @note The timestamp and password fields are not required in the file
 * as they are automatically generated during request processing.
 */
inline STKPushRequest load_request_from_file(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filepath);
        }
        
        nlohmann::json j;
        file >> j;
        
        STKPushRequest request;
        
        // Required fields
        request.businessShortCode = j["BusinessShortCode"];
        request.amount = j["Amount"];
        request.partyA = formatPhoneNumber(j["PartyA"]);
        request.partyB = j["PartyB"];
        request.phoneNumber = formatPhoneNumber(j["PhoneNumber"]);
        request.callBackURL = j["CallBackURL"];
        request.accountReference = j["AccountReference"];
        request.transactionDesc = j["TransactionDesc"];
        
        // Optional fields with defaults
        if (j.contains("TransactionType")) {
            std::string txType = j["TransactionType"];
            if (txType == "CustomerBuyGoodsOnline") {
                request.transactionType = TransactionType::CustomerBuyGoodsOnline;
            } else {
                request.transactionType = TransactionType::CustomerPayBillOnline;
            }
        }
        
        return request;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load request: " + std::string(e.what()));
    }
}

} // namespace mpesa::stk