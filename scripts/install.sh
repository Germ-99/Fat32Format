#!/bin/bash

set -e

REPO_URL="https://github.com/Germ-99/Fat32Format.git"
INSTALL_DIR="Fat32Format"


check_command() {
    command -v "$1" >/dev/null 2>&1
}

detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    elif [ -f /etc/arch-release ]; then
        echo "arch"
    else
        echo "unknown"
    fi
}

print_install_commands() {
    local os="$1"
    
    echo ""
    echo "Missing dependencies detected!"
    echo ""
    echo "Please install the required packages:"
    echo ""
    
    case "$os" in
        ubuntu|debian|linuxmint|pop)
            echo "  sudo apt update"
            echo "  sudo apt install -y build-essential git"
            ;;
        fedora)
            echo "  sudo dnf install -y gcc make git"
            ;;
        rhel|centos|rocky|almalinux)
            echo "  sudo yum install -y gcc make git"
            ;;
        arch|manjaro|endeavouros)
            echo "  sudo pacman -S --needed base-devel git"
            ;;
        opensuse|suse)
            echo "  sudo zypper install -y gcc make git"
            ;;
        alpine)
            echo "  sudo apk add gcc make git musl-dev linux-headers"
            ;;
        gentoo)
            echo "  sudo emerge --ask dev-vcs/git sys-devel/gcc sys-devel/make"
            ;;
        void)
            echo "  sudo xbps-install -S gcc make git"
            ;;
        *)
            echo "  Unknown distribution. Please install:"
            echo "    - gcc (C compiler)"
            echo "    - make (build tool)"
            echo "    - git (version control)"
            ;;
    esac
    
    echo ""
    echo "After installing dependencies, run this script again."
    echo ""
}

check_dependencies() {
    local missing=0
    local os=$(detect_os)
    
    echo "Checking dependencies..."
    
    if ! check_command git; then
        echo "git - NOT FOUND"
        missing=1
    else
        echo "git - found"
    fi
    
    if ! check_command gcc; then
        echo "gcc - NOT FOUND"
        missing=1
    else
        echo "gcc - found"
    fi
    
    if ! check_command make; then
        echo "make - NOT FOUND"
        missing=1
    else
        echo "make - found"
    fi
    
    echo ""
    
    if [ $missing -eq 1 ]; then
        print_install_commands "$os"
        exit 1
    fi
    
    echo "All dependencies satisfied!"
    echo ""
}

check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        echo "Error: This installation script is for Linux only."
        echo "For Windows, please download the .exe from GitHub releases."
        exit 1
    fi
}

cleanup_old_install() {
    if [ -d "$INSTALL_DIR" ]; then
        echo "Found existing installation directory."
        read -p "Remove it and perform fresh install? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo "Removing old directory..."
            rm -rf "$INSTALL_DIR"
        else
            echo "Installation cancelled."
            exit 0
        fi
    fi
}

check_linux
check_dependencies
cleanup_old_install

echo "Cloning repository..."
if ! git clone "$REPO_URL" "$INSTALL_DIR"; then
    echo ""
    echo "Error: Failed to clone repository."
    echo "Please check your internet connection and try again."
    exit 1
fi

echo ""
echo "Building fttf..."
cd "$INSTALL_DIR"

if ! make; then
    echo ""
    echo "Error: Build failed."
    echo "Please check the error messages above."
    exit 1
fi

echo ""
echo "Installing fttf to /usr/local/bin..."
echo "This requires root privileges."
echo ""

if ! make install; then
    echo ""
    echo "Error: Installation failed."
    echo "You may need to run: sudo make install"
    exit 1
fi

echo ""
echo "========================================="
echo "Installation Complete!"
echo "========================================="
echo ""
echo "fttf has been installed to /usr/local/bin"
echo ""
echo "Usage:"
echo "  sudo fttf /dev/sdX"
echo ""
echo "For help:"
echo "  fttf --help"
echo ""
echo "WARNING: Always double-check the device path before formatting!"
echo ""