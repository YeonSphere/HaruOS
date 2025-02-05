# HaruOS (하루OS)

[![CI](https://github.com/YeonSphere/HaruOS/actions/workflows/ci.yml/badge.svg)](https://github.com/YeonSphere/HaruOS/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-YUOSL-purple)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Mobile-green.svg)](https://yeonsphere.github.io/haruos)

A comprehensive, privacy-focused mobile operating system designed to replace Android, developed by YeonSphere.

## Overview

HaruOS ("하루" meaning "day" in Korean) is a modern mobile operating system that prioritizes user privacy, security, and control. Built on a microkernel architecture with Linux driver compatibility, it offers a fresh approach to mobile computing.

## Key Features

- **Privacy-First Design**: Built-in privacy controls and data protection
- **Microkernel Architecture**: Enhanced security and stability
- **Linux Driver Compatibility**: Broad hardware support
- **Modern Security Model**: Advanced permission system and sandboxing
- **AI Integration**: Native AI capabilities for enhanced user experience
- **Custom UI/UX**: Intuitive and efficient interface design
- **App Ecosystem**: Android app compatibility layer (planned)

## Project Structure

```
/HaruOS
├── kernel/              # Microkernel core
│   ├── core/           # Core kernel functionality
│   ├── memory/         # Memory management
│   ├── process/        # Process management
│   ├── ipc/           # Inter-process communication
│   └── hal/           # Hardware abstraction layer
├── drivers/            # Device drivers
│   ├── android/       # Android driver compatibility
│   └── native/        # Native HaruOS drivers
├── system/             # System components
│   ├── security/      # Security framework
│   ├── services/      # System services
│   └── ui/            # User interface components
├── frameworks/         # Development frameworks
│   ├── native/        # Native HaruOS frameworks
│   └── compatibility/ # Android compatibility layer
└── apps/              # Core system applications
    ├── settings/      # System settings
    └── launcher/      # Home screen launcher
```

## Development Roadmap

### Phase 1: Foundation (Current)
- Microkernel development
- Hardware abstraction layer
- Basic driver support
- Memory management system

### Phase 2: System Services
- Process management
- IPC mechanisms
- Security framework
- Basic UI system

### Phase 3: User Interface
- Custom UI framework
- Core system applications
- Settings management
- Home screen implementation

### Phase 4: App Ecosystem
- Android compatibility layer
- Native app framework
- App store infrastructure
- Developer tools

## Building from Source

### Prerequisites
- LLVM/Clang toolchain
- Rust toolchain
- Linux headers
- Android AOSP tools (for compatibility layer)

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/YeonSphere/HaruOS.git
cd HaruOS

# Install dependencies
./scripts/setup.sh

# Build the system
make haruos

# Create a bootable image
make image
```

## Contributing

We welcome contributions! Please read our [Contributing Guidelines](CONTRIBUTING.md) before submitting pull requests.

## Documentation

Visit [yeonsphere.github.io/haruos](https://yeonsphere.github.io/haruos) for:
- Installation Guide
- Developer Documentation
- Architecture Details
- API Reference
- Design Guidelines

## License

This project is licensed under the YeonSphere Universal Open Source License (YUOSL). Key points:
- Free for personal use
- Commercial terms negotiable
- 30-day response window for inquiries
- Contact @daedaevibin for commercial licensing
- See the [LICENSE](LICENSE) file for full details

## Contact

- Primary Contact: Jeremy Matlock (@daedaevibin)
- Discord: [Join our community](https://discord.gg/yeonsphere)
- Email: daedaevibin@naver.com
- GitHub Issues: [Report bugs or request features](https://github.com/YeonSphere/HaruOS/issues)
