#!/bin/bash

# Configuration
LINUX_VERSION="6.6.8"  # Latest stable as of now
LINUX_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-${LINUX_VERSION}.tar.xz"
WORK_DIR="/tmp/linux-pull"
TARGET_DIR="/home/dae/YeonSphere/HaruOS"

# Required directories
mkdir -p "${WORK_DIR}"
cd "${WORK_DIR}"

# Download and extract Linux kernel
echo "Downloading Linux kernel ${LINUX_VERSION}..."
wget "${LINUX_URL}"
tar xf "linux-${LINUX_VERSION}.tar.xz"
cd "linux-${LINUX_VERSION}"

# Create target directories
mkdir -p "${TARGET_DIR}/core/compat/linux"
mkdir -p "${TARGET_DIR}/core/drivers/linux"
mkdir -p "${TARGET_DIR}/core/include/linux"
mkdir -p "${TARGET_DIR}/core/arch/aarch64/include/asm"
mkdir -p "${TARGET_DIR}/core/arch/x86_64/include/asm"

# Copy essential headers
echo "Copying essential headers..."
cp -r include/linux/{firmware.h,module.h,device.h,pci.h,irq.h,dma-mapping.h,\
compiler.h,types.h,kernel.h,init.h,errno.h,string.h,bug.h} \
    "${TARGET_DIR}/core/include/linux/"

# Copy architecture-specific headers
echo "Copying architecture headers..."
cp -r arch/arm64/include/asm/{io.h,memory.h,mmu.h,pgtable*.h,barrier.h} \
    "${TARGET_DIR}/core/arch/aarch64/include/asm/"
cp -r arch/x86/include/asm/{io.h,memory.h,mmu.h,pgtable*.h,barrier.h} \
    "${TARGET_DIR}/core/arch/x86_64/include/asm/"

# Copy firmware interface
echo "Copying firmware interface..."
cp -r drivers/base/firmware_loader \
    "${TARGET_DIR}/core/drivers/linux/"

# Copy essential driver infrastructure
echo "Copying driver infrastructure..."
cp -r drivers/base/{base.h,dd.h,init.h,driver.h,device.h} \
    "${TARGET_DIR}/core/drivers/linux/"

# Copy compatibility layer components
echo "Copying compatibility components..."
cp -r include/linux/{compat.h,syscalls.h,fs.h,slab.h,mm.h} \
    "${TARGET_DIR}/core/compat/linux/"

# Create symlinks for firmware and modules
echo "Creating symlinks..."
ln -sf /lib/firmware "${TARGET_DIR}/firmware"
ln -sf "/lib/modules/${LINUX_VERSION}" "${TARGET_DIR}/modules"

# Clean up
echo "Cleaning up..."
cd /
rm -rf "${WORK_DIR}"

echo "Done! Linux kernel files have been integrated into HaruOS."
