# Contributing to HaruOS

Thank you for your interest in contributing to HaruOS! We're excited to have you join our community in building a privacy-focused mobile operating system.

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct. We expect all contributors to:
- Be respectful and inclusive
- Focus on constructive feedback
- Maintain professional communication
- Support a welcoming environment

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/YOUR-USERNAME/HaruOS.git`
3. Create a new branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Test your changes
6. Submit a pull request

## Development Process

### 1. Setting Up Development Environment

Ensure you have the following installed:
- LLVM/Clang toolchain
- Rust toolchain
- Linux headers
- Android AOSP tools (for compatibility layer)

### 2. Building the Project

```bash
# Install dependencies
./scripts/setup.sh

# Build the system
make haruos

# Create a bootable image
make image

# Run tests
make test
```

### 3. Code Style

- Follow the existing code style
- Use descriptive variable and function names
- Comment complex logic
- Keep functions focused and concise
- Write unit tests for new features

### 4. Commit Guidelines

- Use clear, descriptive commit messages
- Start with a verb (Add, Fix, Update, etc.)
- Reference issues when applicable
- Keep commits focused and atomic

Example:
```
Add memory management module for kernel

- Implement basic page allocation
- Add memory mapping functions
- Include unit tests for allocation
- Update documentation

Fixes #123
```

### 5. Pull Request Process

1. Update documentation for new features
2. Add or update tests as needed
3. Ensure all tests pass
4. Update the changelog if applicable
5. Submit a detailed pull request description

## Areas for Contribution

- Kernel development
- Driver compatibility
- Security features
- UI/UX improvements
- Documentation
- Testing and QA
- Android compatibility layer

## Communication

- Discord: [Join our community](https://discord.gg/yeonsphere)
- GitHub Issues: Technical discussions and bug reports
- Email: daedaevibin@naver.com for sensitive inquiries

## Review Process

1. Initial review within 1-2 weeks
2. Technical review by core team
3. Feedback and iteration if needed
4. Final approval and merge

## Documentation

- Update relevant documentation with changes
- Include code comments for complex logic
- Add examples for new features
- Update API documentation if applicable

## Testing

- Write unit tests for new features
- Update existing tests as needed
- Ensure all tests pass before submitting
- Include integration tests when applicable

## License

By contributing to HaruOS, you agree that your contributions will be licensed under the YeonSphere Universal Open Source License (YUOSL). See [LICENSE](LICENSE) for details.

## Questions?

Feel free to:
- Open an issue for technical questions
- Join our Discord for community discussions
- Email daedaevibin@naver.com for private inquiries

Thank you for contributing to HaruOS!
