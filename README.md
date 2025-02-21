

```markdown
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

## 📚 Documentation
- [Architecture Guide](DOCUMENTATION.md) -  Provides insights into the SDK's design and structure.
- [Testing Framework](DOCUMENTATION.md#tests-directory-tests) - Describes the testing methodology and guidelines for contributions.

## 👥 Contributing
Contributions are welcome! Please refer to [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on reporting bugs, suggesting features, and submitting code changes.  
## 📜 License
Apache-2.0 © [Hayes Fank](https://github.com/Hayessolo).  
M-Pesa® is a registered trademark of Safaricom PLC.

---

## 👨💻 Author
**Hayes Solo**  
[![GitHub](https://img.shields.io/badge/GitHub-Hayes-blue)](https://github.com/Hayessolo)
[![LinkedIn](https://img.shields.io/badge/LinkedIn)](https://linkedin.com/in/hayes-frank-b48700174)
Not affiliated with Safaricom.
**Let’s Connect!**  
📧 Email: solohayes6@gmail.comn  
🐦 Twitter: [@Hayes Frank](https://twitter.com/@myworld_net)

---


```
