# GitHub Actions CI/CD Setup

I've created a complete CI/CD pipeline for building and packaging GTimer. Here's what's been set up:

## Files Created

### `.github/workflows/build.yml`
Main GitHub Actions workflow that:
- Builds on Ubuntu 22.04
- Runs all tests
- Creates multiple package formats:
  - **Flatpak** (universal Linux package)
  - **Debian package** (.deb for Ubuntu/Debian)
  - Binary executable
- Automatically creates GitHub releases when you push version tags

### `flatpak/us.k5n.GTimer.json`
Flatpak manifest for building sandboxed packages that work on all Linux distributions.

### `debian/`
Debian packaging files:
- `control` - Package metadata and dependencies
- `rules` - Build instructions
- `changelog` - Version history
- `compat` - Debhelper version
- `source/format` - Source package format

## How to Use

### Automatic Release (Recommended)

When you're ready to release version 3.1.0:

```bash
git add .
git commit -m "Add CI/CD workflows and packaging"
git tag v3.1.0
git push origin v3.1.0
```

GitHub Actions will automatically:
1. Build the application
2. Run all tests
3. Create Flatpak, Debian, and binary packages
4. Create a GitHub release with all artifacts

### Building Debian Package Locally

```bash
# Install dependencies
sudo apt-get install -y build-essential debhelper meson \
  libgtk-4-dev libadwaita-1-dev libsqlite3-dev libx11-dev gettext

# Build package
cd gtimer
dpkg-buildpackage -us -uc -b

# Install
sudo dpkg -i ../gtimer_3.1.0-1_amd64.deb
```

### Building Flatpak Locally

```bash
# Install flatpak
sudo apt install flatpak
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install SDK
flatpak install flathub org.gnome.Platform//45 org.gnome.Sdk//45

# Build
cd gtimer
flatpak-builder --force-clean build-dir flatpak/us.k5n.GTimer.json

# Install
flatpak build-export repo build-dir
flatpak build-bundle repo gtimer.flatpak us.k5n.GTimer
flatpak install gtimer.flatpak
```

## Package Format Recommendations

### For End Users
1. **Flatpak** (Best choice)
   - Works on all Linux distros
   - Automatic updates
   - Sandboxed security
   - Can be published to Flathub for easy discovery

2. **AppImage** (Not yet set up)
   - Single file, no installation
   - Good for portable use

3. **Native packages** (Debian/RPM)
   - Best integration with system
   - Smaller size
   - Distro-specific

### For Developers
Use the binary executable from the GitHub Actions artifacts for testing.

## Next Steps

1. **Push these changes to GitHub**
2. **Test the workflow** by pushing a tag
3. **Publish to Flathub** (optional but recommended)
   - Fork https://github.com/flathub/flathub
   - Submit your app manifest
   - Users can then install with: `flatpak install flathub us.k5n.GTimer`

4. **Set up AppImage** (optional)
   - Add appimage-builder to the workflow
   - Creates single-file executable

## Questions?

The workflow will run on every push to main and on every pull request. It only creates releases when you push tags starting with 'v' (like v3.1.0).
