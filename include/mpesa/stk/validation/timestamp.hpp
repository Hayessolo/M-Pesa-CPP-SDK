/**
 * @file timestamp.hpp
 * @brief Timestamp Generation and Validation
 * 
 * Utilities for generating and validating timestamps in the format
 * required by the M-Pesa API for STK Push transactions.
 */

#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace mpesa {
namespace validation {

/**
 * @brief Timestamp handling utilities
 * 
 * Provides functionality for generating and validating timestamps
 * in the format YYYYMMDDHHMMSS as required by M-Pesa API.
 */
class TimestampGenerator {
public:
    /**
     * @brief Generate current timestamp in M-Pesa format
     * @return String containing timestamp in format YYYYMMDDHHMMSS
     * @throws Nothing (noexcept)
     */
    static std::string generate() noexcept {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        
#ifdef _WIN32
        gmtime_s(&timeinfo, &time);
#else
        gmtime_r(&time, &timeinfo);
#endif
        
        // Format timestamp without spaces, timezone info, or special characters
        char buffer[15];
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", &timeinfo);
        std::string timestamp(buffer);
        
        return timestamp;
    }

    /**
     * @brief Validate a timestamp string
     * @param timestamp String to validate
     * @return true if timestamp is valid, false otherwise
     * @throws Nothing (noexcept)
     * 
     * Checks if the timestamp:
     * - Is exactly 14 characters
     * - Contains only digits
     * - Has valid values for year, month, day, hour, minute, second
     */
    static bool isValid(const std::string& timestamp) noexcept {
        try {
            if (timestamp.length() != 14) return false;
            if (!std::all_of(timestamp.begin(), timestamp.end(), ::isdigit)) return false;

            int year = std::stoi(timestamp.substr(0, 4));
            int month = std::stoi(timestamp.substr(4, 2));
            int day = std::stoi(timestamp.substr(6, 2));
            int hour = std::stoi(timestamp.substr(8, 2));
            int minute = std::stoi(timestamp.substr(10, 2));
            int second = std::stoi(timestamp.substr(12, 2));

            if (month < 1 || month > 12) return false;
            if (day < 1 || day > 31) return false;
            if (hour < 0 || hour > 23) return false;
            if (minute < 0 || minute > 59) return false;
            if (second < 0 || second > 59) return false;
            
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace validation
} // namespace mpesa