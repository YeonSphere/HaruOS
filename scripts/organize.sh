#!/bin/bash

# Create directory structure
mkdir -p core/arch/{aarch64,x86_64}/{boot,mm}
mkdir -p core/drivers/{base,net,storage}
mkdir -p core/fs
mkdir -p core/kernel
mkdir -p core/lib
mkdir -p core/mm
mkdir -p core/net
mkdir -p core/security
mkdir -p core/syscalls

# Move architecture-specific files
mv core/kernel/arch/* core/arch/

# Move core kernel files
mv core/kernel/{hardware,nanocore,scheduler}.seo core/kernel/

# Move memory management
mv core/kernel/memory.seo core/mm/

# Move network stack
mv core/kernel/net/protocols.seo core/net/

# Move system calls
mv core/kernel/syscall.seo core/syscalls/

# Create symlinks for Linux compatibility
ln -s /lib/firmware firmware
ln -s /lib/modules modules

# Set permissions
chmod +x scripts/*.sh
find . -type f -name "*.seo" -exec chmod 644 {} \;
