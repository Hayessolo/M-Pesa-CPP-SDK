# Contributing to M-Pesa C++ SDK

Thank you for your interest in contributing to the M-Pesa C++ SDK! This document provides guidelines and instructions to help you contribute effectively to this project.

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How to Contribute](#how-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Feature Requests](#feature-requests)
  - [Pull Requests](#pull-requests)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Documentation](#documentation)
- [Testing](#testing)
- [License](#license)

## Code of Conduct

This project adheres to a Code of Conduct that sets expectations for participation. By participating, you are expected to uphold this code. Please report unacceptable behavior to [hayes@frank.dev](mailto:hayes@frank.dev).

## Getting Started

Before contributing, please ensure you have:
- Read the [README.md](README.md) to understand the project's purpose and functionality
- Checked existing [Issues](https://github.com/Hayessolo/M-Pesa-CPP-SDK/issues) for related discussions
- Reviewed open [Pull Requests](https://github.com/Hayessolo/M-Pesa-CPP-SDK/pulls) to avoid duplicate work

## How to Contribute

### Reporting Bugs

If you find a bug in the SDK, please submit an issue using the bug report template. Ensure your report includes:

- A clear, descriptive title
- Detailed steps to reproduce the bug
- Expected behavior versus actual behavior
- Your environment (OS, compiler version, etc.)
- Any relevant logs or error messages

### Feature Requests

We welcome suggestions for new features or improvements. When submitting a feature request:

- Describe the problem your feature would solve
- Explain how your proposed solution would work
- Provide examples of how the feature would be used
- Consider the impact on existing functionality

### Pull Requests

1. Fork the repository
2. Create a new branch from `main`
3. Make your changes
4. Add or update tests as necessary
5. Ensure documentation is updated
6. Submit a pull request with a clear description of the changes

Pull requests should:
- Address a single concern
- Include appropriate tests
- Maintain or improve code coverage
- Follow the coding standards
- Include updated documentation

## Development Setup

### Prerequisites
- C++17 compatible compiler (GCC, Clang, MSVC)
- CMake 3.10 or newer
- libcurl and OpenSSL development libraries
- Doxygen (for building documentation)

### Building from Source
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
```

## Coding Standards

This project follows the Google C++ Style Guide with some modifications:

- Use descriptive variable and function names
- Write comments for complex logic
- Keep functions focused and concise
- Use exceptions for error handling where appropriate
- Follow the existing code formatting

A `.clang-format` file is provided for consistent formatting. Please run it before submitting pull requests:

```bash
clang-format -i src/*.cpp include/*.h
```

## Documentation

- All public APIs must be documented using Doxygen-style comments
- Update the README.md when adding new features
- Include usage examples for new functionality
- Keep documentation up-to-date with code changes

## Testing

- Write unit tests for new functionality
- Ensure existing tests pass before submitting a pull request
- Consider edge cases in your tests
- Test error conditions as well as success paths

## License

By contributing to this project, you agree that your contributions will be licensed under the project's [Apache-2.0 License](LICENSE).

---

Thank you for contributing to the M-Pesa C++ SDK!
