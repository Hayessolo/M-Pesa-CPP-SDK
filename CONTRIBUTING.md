# Contributing to M-Pesa C++ SDK

Thank you for your interest in contributing to the M-Pesa C++ SDK! This document provides guidelines and instructions to help you contribute effectively to this project that powers Africa's largest fintech ecosystem.

## Table of Contents
- [Types of Contributions](#types-of-contributions)
- [Reporting Bugs](#reporting-bugs)
- [Requesting Features](#requesting-features)
- [Development Setup](#development-setup)
- [Code & Documentation Standards](#code--documentation-standards)
- [Pull Request Process](#pull-request-process)
- [Code of Conduct](#code-of-conduct)

## Types of Contributions

We welcome various types of contributions:
- **Code**: New features, bug fixes, and performance improvements
- **Documentation**: Corrections, clarifications, and additional examples
- **Tests**: Additional test coverage, especially for edge cases
- **Feedback**: Suggestions on API design and usability

## Reporting Bugs

When reporting bugs, please use our [Bug Report Template](.github/ISSUE_TEMPLATE/bug_report.md) and include:
- Your environment details (OS, compiler, dependencies)
- Steps to reproduce the issue
- Expected behavior vs. actual behavior
- Relevant logs or error messages

Please search existing issues first to avoid duplicates.

## Requesting Features

For feature requests, please use our [Feature Request Template](.github/ISSUE_TEMPLATE/feature_request.md) and explain:
- The problem your feature would solve
- Your proposed solution and API design (if applicable)
- Any alternatives you've considered

Consider opening a discussion first to gauge interest in major feature additions.

## Development Setup

### Prerequisites
- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.10 or newer
- libcurl and OpenSSL development libraries
- Doxygen (for building documentation)

### Build & Test
```bash
# Clone the repository
git clone https://github.com/Hayessolo/M-Pesa-CPP-SDK.git
cd M-Pesa-CPP-SDK

# Create a build directory
mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Debug -DMPESA_BUILD_TESTS=ON ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure

# Build documentation (requires Doxygen)
cmake --build . --target docs
```

## Code & Documentation Standards

### Code Guidelines
- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Use clang-format for consistent formatting (config provided in `.clang-format`)
- Write meaningful commit messages that explain why changes were made

### Documentation Requirements
- Document all public APIs using Doxygen-style comments:
  ```cpp
  /// @brief Fetch M-Pesa access token.
  /// @throws AuthenticationError If credentials are invalid.
  std::string getAccessToken();
  ```
- Every PR that changes behavior must update relevant documentation
- Use examples liberally (see the `examples/` directory)
- Keep documentation concise yet thorough—assume readers are new to M-Pesa

## Pull Request Process

1. Fork the repository and create a branch from `main`
2. Make your changes with appropriate tests and documentation
3. Ensure all tests and CI checks pass
4. Submit a pull request using our [Pull Request Template](.github/PULL_REQUEST_TEMPLATE.md)

Your pull request should:
- Link to any related issues (e.g., "Fixes #12")
- Include tests for new functionality or bug fixes
- Update documentation as needed
- Pass all CI checks (formatting, tests, linters)

## Code of Conduct

This project follows our [Code of Conduct](.github/CODE_OF_CONDUCT.md) to ensure a welcoming and inclusive environment for all contributors. By participating, you agree to uphold these standards.

The Code of Conduct is based on the [Contributor Covenant 2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/) and recognizes that M-Pesa serves 61M+ daily users—our community should mirror that inclusivity.

---

Thank you for contributing to the M-Pesa C++ SDK and helping power Africa's fintech revolution!

– Hayes Solo & Contributors
