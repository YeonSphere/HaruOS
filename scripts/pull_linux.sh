#!/bin/bash

# Configuration
TARGET_DIR="/home/dae/YeonSphere/HaruOS"
LINUX_SOURCE="${TARGET_DIR}/core/kernel/kernel-for-hardware/linux-6.12.1"

# Verify source directory exists
if [ ! -d "${LINUX_SOURCE}" ]; then
    echo "Error: Linux source directory not found at ${LINUX_SOURCE}"
    exit 1
fi

# Create necessary directories
echo "Creating directories..."
sudo mkdir -p "${TARGET_DIR}/core/include/linux/device"
sudo mkdir -p "${TARGET_DIR}/core/include/uapi/linux"
sudo mkdir -p "${TARGET_DIR}/core/arch/x86_64/include/asm"
sudo mkdir -p "${TARGET_DIR}/core/arch/aarch64/include/asm"
sudo mkdir -p "${TARGET_DIR}/core/include/asm-generic"

# Copy essential headers and their dependencies
echo "Copying essential headers..."
# First copy device headers
sudo cp -r ${LINUX_SOURCE}/include/linux/device/{driver.h,bus.h,class.h} \
    "${TARGET_DIR}/core/include/linux/device/"

# Then copy all other Linux headers
sudo cp -r ${LINUX_SOURCE}/include/linux/{compiler*.h,types.h,kernel.h,init.h,errno.h,string.h,\
bug.h,list.h,slab.h,mm.h,sched.h,wait.h,kthread.h,linkage.h,cache.h,stringify.h,\
cpumask.h,threads.h,jump_label.h,kasan-checks.h,swait.h,completion.h,\
dev_printk.h,uaccess.h,mmdebug.h,pgtable.h,mem_encrypt.h,device.h,\
platform_device.h,mod_devicetable.h,module.h,firmware.h,of_device.h,\
of.h,of_fdt.h,of_address.h,of_irq.h,of_platform.h,ioport.h,io.h,io-pgtable.h,\
iommu.h,irq.h,dma-mapping.h,container_of.h,build_bug.h,kobject.h,args.h,\
cleanup.h,err.h,stdarg.h} \
    "${TARGET_DIR}/core/include/linux/"

# Copy UAPI headers
echo "Copying UAPI headers..."
sudo cp -r ${LINUX_SOURCE}/include/uapi/linux/{sched.h,types.h,errno.h,kernel.h} \
    "${TARGET_DIR}/core/include/uapi/linux/"

# Copy architecture headers
echo "Copying architecture headers..."
# x86_64
sudo cp -r ${LINUX_SOURCE}/arch/x86/include/asm/{io.h,pgtable*.h,barrier.h,smp.h,spinlock.h,\
cpufeatures.h,processor.h,msr.h,msr-index.h,special_insns.h,fpu/api.h,segment.h,percpu.h,\
page.h,page_types.h,alternative.h,asm.h,cmpxchg.h,atomic.h,current.h,desc.h,div64.h,\
extable.h} \
    "${TARGET_DIR}/core/arch/x86_64/include/asm/"

# ARM64
sudo cp -r ${LINUX_SOURCE}/arch/arm64/include/asm/{io.h,pgtable*.h,barrier.h,smp.h,spinlock.h,\
alternative.h,cpufeature.h,cputype.h,sysreg.h,memory.h,page.h,page-def.h,\
processor.h,thread_info.h,ptrace.h,percpu.h,atomic.h,cmpxchg.h,current.h} \
    "${TARGET_DIR}/core/arch/aarch64/include/asm/"

# Copy asm-generic headers
sudo cp -r ${LINUX_SOURCE}/include/asm-generic/{io.h,barrier.h,qspinlock*.h,atomic*.h,bug.h,rwonce.h} \
    "${TARGET_DIR}/core/include/asm-generic/"

# Create symlinks for rwonce.h
echo "Creating architecture-specific symlinks..."
sudo ln -sf "${TARGET_DIR}/core/include/asm-generic/rwonce.h" "${TARGET_DIR}/core/arch/x86_64/include/asm/rwonce.h"
sudo ln -sf "${TARGET_DIR}/core/include/asm-generic/rwonce.h" "${TARGET_DIR}/core/arch/aarch64/include/asm/rwonce.h"

# Fix permissions
sudo chown -R $(whoami):$(whoami) "${TARGET_DIR}/core"

echo "Linux kernel headers integration complete!"
