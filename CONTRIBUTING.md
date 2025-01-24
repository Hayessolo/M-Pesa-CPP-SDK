# Contributing to M-Pesa C++ SDK

🌟 **First, thank you!** By contributing, you’re helping build tools that power Africa’s largest fintech ecosystem (61M+ daily transactions!). Let’s make this SDK bulletproof. 🚀

## 📋 Table of Contents
- [Types of Contributions](#-types-of-contributions)
- [Development Setup](#-development-setup)
- [Code & Documentation Standards](#-code--documentation-standards)
- [Pull Request Guidelines](#-pull-request-guidelines)
- [Code of Conduct](#-code-of-conduct)

## 🌟 Types of Contributions
We welcome:
- **Code**: Features, bug fixes, performance improvements.
- **Documentation**: Fix typos, improve guides, add examples.
- **Tests**: Expand coverage for edge cases.
- **Feedback**: API design suggestions, usability reports.

### 🐛 Reporting Bugs
1. **Search existing issues** to avoid duplicates.
2. Use the [Bug Report Template](.github/ISSUE_TEMPLATE/bug_report.md).
3. Include:
   - Environment (OS, compiler, dependencies).
   - Steps to reproduce.
   - Expected vs. actual behavior.

### 🚀 Proposing Features
1. **Open a discussion** to gauge interest.
2. Use the [Feature Request Template](.github/ISSUE_TEMPLATE/feature_request.md).
3. Explain:
   - The problem your feature solves.
   - Proposed API design (if applicable).

## 🛠 Development Setup

### Prerequisites
- **C++17+** (GCC/Clang/MSVC)
- **CMake 3.10+**
- **libcurl** & **OpenSSL**
- **Doxygen** (for documentation builds)

### Build & Test
```bash
git clone https://github.com/yourusername/mpesa-cpp-sdk.git
cd mpesa-cpp-sdk
mkdir build && cd build
cmake -DMPESA_BUILD_TESTS=ON ..  # Enable tests
make

# Run tests
ctest --output-on-failure

# Build documentation (requires Doxygen)
make docs
📜 Code & Documentation Standards
Code Guidelines
Follow Google C++ Style Guide.

Use clang-format for consistency (config in .clang-format).

Document public APIs with Doxygen:

cpp
Copy
/// @brief Fetch M-Pesa access token.
/// @throws AuthenticationError If credentials are invalid.
std::string getAccessToken();
Documentation Principles
Every PR must update documentation if it changes behavior.

Use examples liberally (see examples/ directory).

Keep prose concise but thorough—assume readers are new to M-Pesa.

🔍 Pull Request Guidelines
Target the main branch.

Link to related issues (e.g., "Fixes #12").

Include:

Tests: For bugs/features.

Documentation: Updated README, code comments, or guides.

Benchmarks: If claiming performance improvements.

Pass all CI checks (formatting, tests, linters).

🕊 Code of Conduct
Be respectful and inclusive.
Read our full Code of Conduct before participating.

Thank you for powering Africa’s fintech revolution! 💡
– Hayes Solo & Contributors