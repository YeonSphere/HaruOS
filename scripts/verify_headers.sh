#!/bin/bash

# Configuration
TARGET_DIR="/home/dae/YeonSphere/HaruOS"
TEMP_DIR="/tmp/haruos_header_test"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Create temporary directory for tests
rm -rf "${TEMP_DIR}"
mkdir -p "${TEMP_DIR}"

# Function to check header dependencies
check_header() {
    local header=$1
    
    echo -e "${YELLOW}Testing ${header}...${NC}"
    
    # Create test file
    cat > "${TEMP_DIR}/test.c" << EOF
#include <stdio.h>
#define __KERNEL__
#include "${header}"

int main() {
    return 0;
}
EOF
    
    # Try to compile with all necessary include paths
    gcc -I"${TARGET_DIR}/include" \
        -I"${TARGET_DIR}/include/uapi" \
        -I"${TARGET_DIR}/arch/x86/include" \
        -I"${TARGET_DIR}/arch/arm64/include" \
        -c "${TEMP_DIR}/test.c" -o "${TEMP_DIR}/test.o" 2> "${TEMP_DIR}/errors.txt"
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ ${header} - OK${NC}"
    else
        echo -e "${RED}✗ ${header} - Failed${NC}"
        echo "Missing dependencies:"
        grep "No such file or directory" "${TEMP_DIR}/errors.txt" | sed 's/.*fatal error: \(.*\): No.*/\1/' | sort -u
    fi
}

# Function to test a group of headers
test_header_group() {
    local group_name=$1
    shift
    echo -e "\n=== Testing ${group_name} Headers ==="
    for header in "$@"; do
        check_header "$header"
    done
}

# Test essential headers
test_header_group "Essential" "linux/firmware.h" "linux/module.h" "linux/device.h" "linux/pci.h" "linux/irq.h" \
                               "linux/dma-mapping.h" "linux/compiler.h" "linux/types.h" "linux/kernel.h" \
                               "linux/init.h" "linux/errno.h" "linux/string.h" "linux/bug.h" "linux/list.h" \
                               "linux/slab.h" "linux/mm.h" "linux/sched.h" "linux/wait.h" "linux/kthread.h"

# Test device headers
test_header_group "Device" "linux/device/driver.h" "linux/device/bus.h" "linux/device/class.h"

# Test UAPI headers
test_header_group "UAPI" "linux/sched.h" "linux/types.h" "linux/errno.h"

# Test architecture headers (x86)
test_header_group "x86 Architecture" "asm/io.h" "asm/page.h" "asm/barrier.h" "asm/smp.h" "asm/spinlock.h"

# Test architecture headers (arm64)
test_header_group "ARM64 Architecture" "asm/io.h" "asm/barrier.h" "asm/smp.h" "asm/spinlock.h"

# Test asm-generic headers
test_header_group "ASM Generic" "asm-generic/io.h" "asm-generic/barrier.h" "asm-generic/qspinlock.h" \
                                "asm-generic/atomic.h" "asm-generic/bug.h"

# Test driver infrastructure
test_header_group "Driver Infrastructure" "linux/device.h" "linux/platform_device.h" "linux/mod_devicetable.h" \
                                          "linux/module.h" "linux/firmware.h" "linux/of_device.h"

# Test compatibility headers
test_header_group "Compatibility" "linux/compat.h" "linux/syscalls.h" "linux/fs.h" "linux/slab.h" \
                                  "linux/mm.h" "linux/sched.h" "linux/completion.h"

# Cleanup
rm -rf "${TEMP_DIR}"
