#!/bin/bash

# USB Crypto Driver Build and Installation Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if running as root for driver operations
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root for driver operations"
        echo "Usage: sudo $0"
        exit 1
    fi
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for kernel headers
    if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
        print_error "Kernel headers not found. Please install kernel headers."
        print_status "On Ubuntu/Debian: sudo apt-get install linux-headers-$(uname -r)"
        print_status "On CentOS/RHEL: sudo yum install kernel-devel"
        exit 1
    fi
    
    # Check for gcc
    if ! command -v gcc &> /dev/null; then
        print_error "GCC compiler not found. Please install build-essential."
        print_status "On Ubuntu/Debian: sudo apt-get install build-essential"
        exit 1
    fi
    
    # Check for make
    if ! command -v make &> /dev/null; then
        print_error "Make utility not found. Please install build-essential."
        exit 1
    fi
    
    print_success "All dependencies are satisfied"
}

# Function to build the kernel module
build_driver() {
    print_status "Building USB crypto driver..."
    
    if [ ! -f "usb_crypto_driver.c" ]; then
        print_error "Driver source file 'usb_crypto_driver.c' not found"
        exit 1
    fi
    
    if [ ! -f "Makefile" ]; then
        print_error "Makefile not found"
        exit 1
    fi
    
    # Clean previous builds
    make clean > /dev/null 2>&1 || true
    
    # Build the module
    if make; then
        print_success "Driver compiled successfully"
    else
        print_error "Driver compilation failed"
        exit 1
    fi
}

# Function to build user application
build_application() {
    print_status "Building user application..."
    
    if [ ! -f "crypto_app.c" ]; then
        print_error "Application source file 'crypto_app.c' not found"
        exit 1
    fi
    
    if gcc -o crypto_app crypto_app.c; then
        print_success "Application compiled successfully"
        chmod +x crypto_app
    else
        print_error "Application compilation failed"
        exit 1
    fi
}

# Function to load the driver
load_driver() {
    print_status "Loading USB crypto driver..."
    
    # Unload if already loaded
    if lsmod | grep -q "usb_crypto_driver"; then
        print_warning "Driver already loaded, unloading first..."
        rmmod usb_crypto_driver || true
    fi
    
    # Load the new driver
    if insmod usb_crypto_driver.ko; then
        print_success "Driver loaded successfully"
        
        # Check if proc entry was created
        sleep 1
        if [ -e "/proc/usb_crypto" ]; then
            print_success "Proc interface created: /proc/usb_crypto"
        else
            print_warning "Proc interface not found. Driver may not be working correctly."
        fi
        
        # Show driver info
        print_status "Driver information:"
        modinfo usb_crypto_driver.ko | head -10
        
    else
        print_error "Failed to load driver"
        exit 1
    fi
}

# Function to unload the driver
unload_driver() {
    print_status "Unloading USB crypto driver..."
    
    if lsmod | grep -q "usb_crypto_driver"; then
        if rmmod usb_crypto_driver; then
            print_success "Driver unloaded successfully"
        else
            print_error "Failed to unload driver"
            exit 1
        fi
    else
        print_status "Driver is not loaded"
    fi
}

# Main section to handle command-line arguments
case "$1" in
    build)
        check_dependencies
        build_driver
        build_application
        ;;
    load)
        check_root
        load_driver
        ;;
    unload)
        check_root
        unload_driver
        ;;
    reload)
        check_root
        unload_driver
        load_driver
        ;;
    install)
        check_dependencies
        build_driver
        build_application
        check_root
        load_driver
        ;;
    uninstall)
        print_status "Cleaning build artifacts..."
        make clean || true
        rm -f crypto_app
        print_warning "Uninstalling the driver..."
        check_root
        unload_driver
        print_success "Driver uninstalled successfully"
        ;;
    reinstall)
        check_dependencies
        build_driver
        build_application
        check_root
        unload_driver
        load_driver
        ;;
    remove)
        check_root
        unload_driver
        ;;
    clean)
        print_status "Cleaning build artifacts..."
        make clean || true
        rm -f crypto_app
        print_success "Cleaned successfully"
        ;;
    *)
        echo "Usage: $0 {build|load|unload|reload|install|reinstall|remove|clean}"
        exit 1
        ;;
esac