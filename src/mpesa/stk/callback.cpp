/**
 * @file callback.cpp
 * @brief STK Push Callback Processing
 * 
 * Implements the processing of callback data received from M-Pesa after
 * an STK Push transaction completes or fails.
 */

#include "mpesa/stk/response.hpp"
#include <nlohmann/json.hpp>

namespace mpesa {
namespace stk {

/**
 * @brief Parser for STK Push callbacks
 * 
 * Handles parsing and validation of callback data received from
 * M-Pesa after a transaction is processed.
 */
class CallbackParser {
public:
    /**
     * @brief Parse raw callback JSON data
     * @param json Raw JSON callback data
     * @return Populated STKCallback structure
     * @throws std::runtime_error if parsing fails
     */
    static STKCallback parseCallback(const std::string& jsonStr) {
        try {
            auto json = nlohmann::json::parse(jsonStr);
            auto stkCallback = json["Body"]["stkCallback"];

            STKCallback callback;
            callback.merchantRequestID = stkCallback["MerchantRequestID"];
            callback.checkoutRequestID = stkCallback["CheckoutRequestID"];
            callback.resultCode = static_cast<validation::STKPushErrorCode>(
                stkCallback["ResultCode"].get<int>());
            callback.resultDesc = stkCallback["ResultDesc"];

            // Parse callback metadata if present
            if (stkCallback.contains("CallbackMetadata") && 
                stkCallback["CallbackMetadata"].contains("Item")) {
                std::vector<CallbackMetadataItem> items;
                
                for (const auto& item : stkCallback["CallbackMetadata"]["Item"]) {
                    CallbackMetadataItem metadataItem;
                    metadataItem.name = item["Name"];
                    
                    // Handle different value types
                    if (item["Value"].is_number_float()) {
                        metadataItem.value = item["Value"].get<double>();
                    } else if (item["Value"].is_number_integer()) {
                        metadataItem.value = item["Value"].get<int64_t>();
                    } else if (item["Value"].is_string()) {
                        metadataItem.value = item["Value"].get<std::string>();
                    } else {
                        // Fallback: serialise the raw JSON value
                        metadataItem.value = item["Value"].dump();
                    }
                    
                    items.push_back(metadataItem);
                }
                
                callback.callbackMetadata = std::move(items);
            }

            return callback;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to parse callback: ") + e.what());
        }
    }
};

} // namespace stk
} // namespace mpesa