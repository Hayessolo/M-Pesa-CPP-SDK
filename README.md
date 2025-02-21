```markdown
# M-Pesa C++ SDK
This robust C++ SDK provides secure and efficient access to M-Pesa's [APIs](https://developer.safaricom.co.ke/), enabling developers to build high-performance applications for payments, transfers, and other financial services.

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

## 🔧 Configuration
### `.env` File
```ini
MPESA_CONSUMER_KEY=your_consumer_key
MPESA_CONSUMER_SECRET=your_consumer_secret
MPESA_ENVIRONMENT=sandbox # or "production"
```

### Environment Variables
```bash
export MPESA_CONSUMER_KEY="your_key"
export MPESA_CONSUMER_SECRET="your_secret"
```

## 📱 Usage
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

### 4. Callback Handling
Implement an HTTPS endpoint:
```cpp
mpesa::stk::CallbackHandler handler([](auto& callback) {
    if (callback.resultCode == 0) {
        // Process successful payment
    } else {
        // Handle failure
    }
});
// See examples/ for complete server implementation
```

### 5. Configuration
**Environment Variables**
```bash
MPESA_CONSUMER_KEY=your_key
MPESA_CONSUMER_SECRET=your_secret
MPESA_ENVIRONMENT=sandbox
```

**Config File (JSON):**
```json
{
    "consumer_key": "your_key",
    "consumer_secret": "your_secret",
    "sandbox": true
}
```

### 6. Examples
See [/examples](examples/) directory for:
- Complete STK Push implementation
- Callback server example
- Configuration loader
- Error handling patterns

## 📚 Documentation
- [Architecture Guide](DOCUMENTATION.md) - Provides insights into the SDK's design and structure.
- [Testing Framework](DOCUMENTATION.md#tests-directory-tests) - Describes the testing methodology and guidelines for contributions.

## 👥 Contributing
Contributions are welcome! Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on reporting bugs, suggesting features, and submitting code changes.

## 📜 License
Apache-2.0 © [Hayes Frank](https://github.com/Hayessolo).

M-Pesa® is a registered trademark of Safaricom PLC.

---

## 👨‍💻 Author
**Hayes Solo**

[![GitHub](https://img.shields.io/badge/GitHub-Hayes-blue)](https://github.com/Hayessolo)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Hayes_Frank-blue)](https://linkedin.com/in/hayes-frank-b48700174)

Not affiliated with Safaricom.

**Let's Connect!**
📧 Email: solohayes6@gmail.com
🐦 Twitter: [@Hayes Frank](https://twitter.com/@myworld_net)

---
```
