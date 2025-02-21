# M-Pesa C++ SDK
```
```
A robust C++ SDK that provides secure and efficient access to [M-Pesa's APIs](https://developer.safaricom.co.ke/), enabling developers to build high-performance applications for payments, transfers, and other financial services.

## API Credentials
### Getting Started
1. **Register** at the [Safaricom Developer Portal](https://developer.safaricom.co.ke)
2. **Create App** to receive:
   - `CONSUMER_KEY`
   - `CONSUMER_SECRET`
3. **Sandbox Testing**: Use predefined test credentials:
   - Shortcode: `174379`
   - Passkey: `YOUR_SANDBOX_PASSKEY` (found in portal)
   - Test phone: `254708374149`

### Production Setup
To go live:
1. Submit **Going Live Request** through portal
2. Receive production credentials
3. Generate production certificate via [M-Pesa Portal](https://developer.safaricom.co.ke/docs#going-live)

### Example: Authentication
```cpp
#include <mpesa/auth.h>
int main() {
    try {
        auto config = mpesa::AuthConfig::from_env(); // Load from environment variables
        mpesa::Auth auth(config);
        std::string token = auth.getAccessToken();
        std::cout << "Access Token: " << token.substr(0, 10) << "...\n";
    } catch (const mpesa::AuthenticationError& e) {
        std::cerr << "Error: " << e.what() << " (Code: " << static_cast<int>(e.getErrorCode()) << ")\n";
    }
    return 0;
}
```

## Configuration
### `.env` File
```ini
MPESA_CONSUMER_KEY=your_consumer_key
MPESA_CONSUMER_SECRET=your_consumer_secret
MPESA_ENVIRONMENT=sandbox # or "production"
MPESA_PASSKEY=your_passkey
```

### Environment Variables
```bash
# Required
export MPESA_CONSUMER_KEY="YOUR_CONSUMER_KEY"
export MPESA_CONSUMER_SECRET="YOUR_CONSUMER_SECRET"
# Optional (default: sandbox)
export MPESA_ENVIRONMENT="production"
export MPESA_PASSKEY="YOUR_PASSKEY"
```

### Config File (config.json)
```json
{
    "consumer_key": "YOUR_KEY",
    "consumer_secret": "YOUR_SECRET",
    "business_shortcode": "174379",
    "passkey": "YOUR_PASSKEY",
    "sandbox": true
}
```

## Usage
**C++17 or newer required**

### 1. Installation
Add to your CMake project:
```cmake
add_subdirectory(M-Pesa-CPP-SDK)
target_link_libraries(your_target PRIVATE mpesa)
```

### 2. Authentication
Initialize with credentials:
```cpp
// From environment variables
auto config = mpesa::AuthConfig::from_env(); 
// Or config file
auto config = mpesa::AuthConfig::from_file("config.json");
mpesa::Auth auth_client(config);
std::string token = auth_client.getAccessToken();
```

### 3. Core Services
#### M-Pesa Express (STK Push)
```cpp
mpesa::stk::STKPushClient client(auth_client);
mpesa::stk::STKPushRequest request{
    .businessShortCode = "174379",
    .transactionType = mpesa::stk::TransactionType::CustomerPayBillOnline,
    .amount = "100",
    .partyA = "254712345678",
    .partyB = "174379",
    .phoneNumber = "254712345678",
    .callBackURL = "https://yourdomain.com/callback",
    .accountReference = "Order123",
    .transactionDesc = "Payment"
};
auto result = client.initiateSTKPush(request).get();
if (result.isSuccess()) {
    auto response = result.getValue();
    std::cout << "Checkout ID: " << response.checkoutRequestID;
} else {
    std::cerr << "Error: " << result.getError();
}
```

### 4. Examples
See [/examples](examples/) directory for:
- Complete STK Push implementation
- Callback server example
- Configuration loader
- Error handling patterns

##  Documentation
- [Architecture Guide](DOCUMENTATION.md) - Provides insights into the SDK's design and structure.
- [Testing Framework](DOCUMENTATION.md#tests-directory-tests) - Describes the testing methodology and guidelines for contributions.

##  Contributing
Contributions are welcome! Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on reporting bugs, suggesting features, and submitting code changes.

##  License
Apache-2.0 © [Hayes Frank](https://github.com/Hayessolo).

M-Pesa® is a registered trademark of Safaricom PLC.

---

##  Author
**Hayes Solo**

[![GitHub](https://img.shields.io/badge/GitHub-Hayes-blue)](https://github.com/Hayessolo)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Hayes_Frank-blue)](https://linkedin.com/in/hayes-frank-b48700174)

Not affiliated with Safaricom.

**Let's Connect!**
 Email: [hayes@frank.dev](solohayes6@gmail.com)
 Twitter: [@Hayes Frank](https://twitter.com/@myworld_net)

---
```
