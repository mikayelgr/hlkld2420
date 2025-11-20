# Contributing to HLK-LD2420 Library

Thank you for your interest in contributing!

## Getting Started

### Prerequisites

- Git, CMake 3.13+, C compiler
- For Pico: Pico SDK and ARM toolchain

### Setting Up Development Environment

1. Fork the repository on GitHub
2. Clone your fork:

   ```bash
   git clone https://github.com/YOUR_USERNAME/hlkld2420.git
   cd hlkld2420
   ```

3. Add the upstream repository:

   ```bash
   git remote add upstream https://github.com/mikayelgr/hlkld2420.git
   ```

4. Create a feature branch:

   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Guidelines

### Code Style

**C Code Standards:**

- Use C99 standard
- 4 spaces for indentation (no tabs)
- Opening braces on the same line for functions
- Clear, descriptive variable names
- Comment complex logic and algorithms

**Naming Conventions:**

- Functions: `ld2420_module_action()` (lowercase with underscores)
- Types: `ld2420_type_t` (lowercase with `_t` suffix)
- Constants: `LD2420_CONSTANT_NAME` (uppercase with underscores)
- Enums: `LD2420_ENUM_VALUE` (uppercase with underscores)

**Example:**

```c
ld2420_status_t ld2420_parse_rx_buffer(
    const uint8_t *in_raw_rx_buffer,
    const uint8_t in_raw_rx_buffer_size,
    uint16_t *out_frame_size,
    uint16_t *out_cmd_echo,
    uint16_t *out_status)
{
    if (!in_raw_rx_buffer || !out_frame_size) {
        return LD2420_STATUS_ERROR_INVALID_ARGUMENTS;
    }
    
    // Implementation
    return LD2420_STATUS_OK;
}
```

### Documentation

**Code Documentation:**

- All public functions must have header comments
- Describe parameters, return values, and behavior
- Include usage examples for complex functions
- Document error conditions and edge cases

**Example:**

```c
/**
 * Parse a single complete LD2420 RX buffer (one-shot parsing).
 *
 * Input:
 * - in_raw_rx_buffer: Pointer to contiguous bytes beginning at header.
 * - in_raw_rx_buffer_size: Total number of bytes in the packet.
 *
 * Output:
 * - out_frame_size: Intra-frame data size from the length field.
 * - out_cmd_echo: Command echo field (16-bit).
 * - out_status: Status word from the device.
 *
 * Return:
 * - LD2420_STATUS_OK on success.
 * - Error codes on invalid header/footer or bad length.
 */
```

**Markdown Documentation:**

- Keep README files up to date
- Add examples for new features
- Update API documentation when changing interfaces
- Include troubleshooting information

### Testing

**Core Library Tests:**

All core functionality must have unit tests:

```bash
cd core
cmake -B build -DLD2420_CORE_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

**Test Guidelines:**

- Write tests for new features before implementation (TDD encouraged)
- Test error conditions and edge cases
- Ensure tests pass on both little-endian and big-endian systems
- Add integration tests for platform-specific code

**Example Test:**

```c
void test_parse_valid_frame(void) {
    uint8_t buffer[] = {
        0xFD, 0xFC, 0xFB, 0xFA,  // Header
        0x06, 0x00,              // Length
        0xFF, 0x00, 0x01, 0x00,  // Data
        0x00, 0x00,
        0x04, 0x03, 0x02, 0x01   // Footer
    };
    
    uint16_t frame_size, cmd_echo, status;
    ld2420_status_t result = ld2420_parse_rx_buffer(
        buffer, sizeof(buffer),
        &frame_size, &cmd_echo, &status);
    
    TEST_ASSERT_EQUAL(LD2420_STATUS_OK, result);
    TEST_ASSERT_EQUAL(0x0006, frame_size);
}
```

## Types of Contributions

### Bug Fixes

1. Search existing issues to avoid duplicates
2. Create an issue describing the bug (if not exists)
3. Reference the issue in your commit
4. Add regression tests if applicable

### New Features

1. Discuss the feature in an issue first
2. Ensure it aligns with project goals
3. Keep changes focused and minimal
4. Update documentation
5. Add comprehensive tests

### Platform Support

Adding support for a new platform:

1. Create `platform/YOUR_PLATFORM/` directory
2. Implement platform-specific UART handling
3. Follow existing platform structure (see `platform/pico/`)
4. Add comprehensive README with:
   - Hardware requirements
   - Build instructions
   - Wiring diagrams
   - Troubleshooting guide
5. Create working example in `examples/YOUR_PLATFORM/`

**Platform Implementation Checklist:**

- [ ] UART initialization and configuration
- [ ] Interrupt-driven or polling-based RX
- [ ] Thread-safe TX function
- [ ] Frame assembly using streaming parser
- [ ] Platform-specific documentation
- [ ] Complete working example
- [ ] Build instructions and prerequisites

### Documentation Improvements

- Fix typos and grammatical errors
- Clarify confusing sections
- Add missing information
- Improve code examples
- Update outdated content

## Pull Request Process

### Before Submitting

1. Ensure all tests pass
2. Update documentation
3. Follow code style guidelines
4. Write clear commit messages
5. Rebase on latest upstream main

### Commit Messages

Use clear, descriptive commit messages:

```text
feat: add support for ESP32 platform

- Implement UART driver for ESP-IDF
- Add platform-specific initialization
- Include working example
- Add documentation

Closes #42
```

**Commit Prefixes:**

- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation only
- `test:` - Adding or updating tests
- `refactor:` - Code refactoring
- `perf:` - Performance improvement
- `chore:` - Maintenance tasks

### Submitting Pull Request

1. Push your branch to your fork
2. Open a pull request against `main`
3. Fill out the PR template completely
4. Link related issues
5. Respond to review feedback promptly

### PR Checklist

- [ ] Tests pass locally
- [ ] Code follows style guidelines
- [ ] Documentation updated
- [ ] Commit messages are clear
- [ ] No merge conflicts with main
- [ ] Changes are focused and minimal

## Code Review

Reviewers check for correctness, style, tests, documentation, and memory safety. Address feedback promptly and ask questions when needed.

## Community Guidelines

### Communication

- Be respectful and professional
- Assume good intentions
- Provide constructive feedback
- Help others learn and grow
- Keep discussions on-topic

### Getting Help

- Check existing documentation first
- Search closed issues for solutions
- Ask clear, specific questions
- Provide minimal reproducible examples
- Include relevant system information

## Architecture Overview

### Core Library Design

- **Platform-Agnostic**: No platform-specific code in core
- **Zero Dependencies**: Pure C with standard library only
- **Fixed Memory**: No dynamic allocation
- **Thread-Unsafe**: Callers handle synchronization
- **Explicit Error Handling**: Status codes for all operations

### Platform Layer Responsibilities

- UART hardware abstraction
- Interrupt handling
- Buffer management
- Thread synchronization (if needed)
- Platform-specific error handling

### Separation of Concerns

```text
Application Code
       ↓
Platform Layer (ld2420_pico, ld2420_esp32, etc.)
       ↓
Core Library (ld2420_core)
       ↓
Protocol Specification
```

## Versioning

We follow [Semantic Versioning](https://semver.org/). Maintainers handle releases.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

If you have questions not covered in this guide:

- Open a discussion on GitHub
- Check the documentation in `docs/`
- Review closed issues for similar questions
- Contact the maintainers

Thank you for contributing to the HLK-LD2420 library!
