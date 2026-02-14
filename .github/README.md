# GitHub Actions CI/CD

This directory contains GitHub Actions workflows for automated building and packaging.

## Workflows

### build.yml
- Builds the application on Ubuntu 22.04
- Runs all tests
- Creates packages:
  - Flatpak bundle (`.flatpak`)
  - Debian package (`.deb`)
  - Binary executable
- Automatically creates GitHub releases when tags are pushed

## Usage

### Building locally (Debian/Ubuntu)

```bash
# Install build dependencies
sudo apt-get install -y build-essential debhelper meson \
  libgtk-4-dev libadwaita-1-dev libsqlite3-dev libx11-dev gettext

# Build package
dpkg-buildpackage -us -uc -b

# Install
sudo dpkg -i ../gtimer_3.1.0-1_amd64.deb
```

### Building Flatpak locally

```bash
# Install flatpak and flathub
sudo apt install flatpak
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install SDK
flatpak install flathub org.gnome.Platform//45 org.gnome.Sdk//45

# Build
flatpak-builder --force-clean build-dir flatpak/us.k5n.GTimer.json

# Create bundle
flatpak build-export repo build-dir
flatpak build-bundle repo gtimer.flatpak us.k5n.GTimer

# Install
flatpak install gtimer.flatpak
```

### Triggering a release

Push a version tag:
```bash
git tag v3.1.0
git push origin v3.1.0
```

The workflow will automatically:
1. Build all packages
2. Create a GitHub release
3. Attach all artifacts to the release

## Package formats

### Flatpak (Recommended)
**Pros:**
- Works on all Linux distributions
- Sandboxed with all dependencies
- Can be published to Flathub
- Automatic updates

**Cons:**
- Larger download size (~50MB)
- Sandboxing may limit some features

### Debian Package (.deb)
**Pros:**
- Native integration
- Small download size
- System-wide installation

**Cons:**
- Only works on Debian/Ubuntu
- Dependency on system libraries

### AppImage
**Pros:**
- Single file download
- No installation required
- Works on any distro

**Cons:**
- No automatic updates
- Larger file size

## Publishing to Flathub

To publish on Flathub for easy discovery:

1. Fork https://github.com/flathub/flathub
2. Create a new branch with your app ID (e.g., `us.k5n.GTimer`)
3. Add your manifest file
4. Submit a pull request

See: https://github.com/flathub/flathub/blob/master/CONTRIBUTING.md
