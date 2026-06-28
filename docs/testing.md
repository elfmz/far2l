[← Architecture](architecture.md) · [Back to README](../README.md) · [Next: Plugins →](plugins.md)

# Testing

far2l uses two testing approaches:

1. **Smoke Tests** — JavaScript-based UI interaction tests
2. **Unit Tests** — GoogleTest-based internal component tests

## Smoke Tests

Smoke tests simulate user interactions with the far2l UI using a JavaScript testing framework.

### Running Smoke Tests

**Build with testing support:**
```bash
cmake -DTESTING=Yes -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc --all)
```

**Run all smoke tests:**
```bash
./far2l-smoke-run.sh /path/to/far2l/binary
```

**Run a specific test:**
```bash
./far2l-smoke path/to/far2l testing/tests/0001-run-and-exit/test.js
```

**Clean test directories:**
```bash
./far2l-smoke-run.sh clean
```

### Writing Smoke Tests

Tests are located in `testing/tests/` and use these key functions:

```javascript
// Start far2l with profile
mydir = WorkDir()
profile = mydir + "/profile"
StartApp(["--tty", "--nodetect", "--mortal", "-u", profile])

// Wait for expected text
ExpectString("Help - FAR2L", 0, 0, -1, -1, 10000)

// Send keypresses
TypeFKey(10)  // F10 to exit

// Wait for exit
ExpectAppExit(0, 10000)
```

### Test Framework Functions

| Function | Description |
|----------|-------------|
| `StartApp(args)` | Launch far2l with arguments |
| `ExpectString(s, x, y, w, h, timeout)` | Wait for string in area |
| `ExpectAppExit(code, timeout)` | Wait for app to exit |
| `TypeFKey(n)` | Press function key |
| `TypeText(s)` | Type text character by character |
| `TypeEnter()` / `TypeEscape()` | Special keys |
| `WorkDir()` | Get test working directory |
| `Log(s)` | Write to test output |

### Test Naming

Tests are numbered for execution order:
```
testing/tests/
├── 0001-run-and-exit/test.js
├── 0002-file-operations/test.js
└── 0003-editor/test.js
```

## Unit Tests

Unit tests are located in `testing/unit/` and use GoogleTest.

### Building Unit Tests

Unit tests are built with `-DTESTING=Yes` and run via CTest:
```bash
cd _build/testing/unit
ctest --output-on-failure
```

### Running Specific Unit Tests

```bash
./testing/unit/test_utils --gtest_filter=TestName*
```

## Running GitHub Actions Locally with `act`

The `act` tool allows running GitHub Actions workflows locally in Docker containers. This is useful for testing CI/CD pipelines before pushing to GitHub.

### Installation

```bash
# Using Homebrew (macOS/Linux)
brew install act

# Using Go
go install github.com/nektos/act@latest

# Using script
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sh
```

### Listing Available Workflows

```bash
# List all available jobs from workflow files
act --dryrun -l

# Or just list jobs
act -l
```

### Running Workflows Locally

```bash
# Run a specific job
act unit-tests-linux

# Run a specific workflow file
act --workflows test.yml

# Dry run (no execution)
act unit-tests-linux -n

# Run with specific event
act -e event.json push
```

### Prerequisites

- Docker must be running
- GitHub Actions runner images need to be pulled:
  ```bash
  docker pull ghcr.io/nektos/act-environments-ubuntu:22.04
  ```

### Known Limitations

- Some workflows may not work perfectly locally due to:
  - macOS-specific jobs can only run on macOS runners
  - Custom actions or composite actions may have compatibility issues
  - Large dependencies may not be cached the same way as GitHub runners
  - Workflows using secrets require environment variables or `--secret` flag

### Configuration

Create `~/.config/act/actrc` to customize behavior:

```
# Use specific runner image
-P ubuntu-latest=ghcr.io/nektos/act-environments-ubuntu:22.04

# Use act's medium image (smaller download)
-P ubuntu-latest=cattime/ubuntu:22.04
```

## See Also

- [Getting Started](getting-started.md) — Installation
- [Architecture](architecture.md) — System design
- [Plugins](plugins.md) — Plugin development
- [testing/README.md](../testing/README.md) — Full testing framework reference
- [act documentation](https://nektosact.com) — Local GitHub Actions runner