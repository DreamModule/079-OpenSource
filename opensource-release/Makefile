# QUICKS OS Makefile

# Tools
ASM = nasm
CC = gcc
CXX = g++
LD = ld

# Flags
ASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs -Wall -Wextra -c
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS = -m elf_i386 -T kernel/linker.ld
INCLUDES = -I include

# Directories
BUILD_DIR = build
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
LIB_DIR = lib

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_ASM_SRC = $(KERNEL_DIR)/arch/x86/boot.asm
KERNEL_CPP_SRC = $(KERNEL_DIR)/core/kernel.cpp
VGA_SRC = $(DRIVERS_DIR)/vga/vga.cpp
KEYBOARD_SRC = $(DRIVERS_DIR)/keyboard/keyboard.cpp
MOUSE_SRC = $(DRIVERS_DIR)/mouse/mouse.cpp
GRAPHICS_SRC = $(DRIVERS_DIR)/graphics/graphics.cpp
BMP_SRC = $(DRIVERS_DIR)/graphics/bmp.cpp
SCP079_FACE_SRC = $(DRIVERS_DIR)/graphics/scp079_face.cpp
SCP079_FACE2_SRC = $(DRIVERS_DIR)/graphics/scp079_face2.cpp
COMPILER_SRC = $(KERNEL_DIR)/core/compiler.cpp
FS_SRC = fs/fs.cpp
LIB_SRC = $(LIB_DIR)/string.cpp

# Object files
BOOT_BIN = $(BUILD_DIR)/boot.bin
KERNEL_ASM_OBJ = $(BUILD_DIR)/boot.o
KERNEL_CPP_OBJ = $(BUILD_DIR)/kernel.o
VGA_OBJ = $(BUILD_DIR)/vga.o
KEYBOARD_OBJ = $(BUILD_DIR)/keyboard.o
MOUSE_OBJ = $(BUILD_DIR)/mouse.o
GRAPHICS_OBJ = $(BUILD_DIR)/graphics.o
BMP_OBJ = $(BUILD_DIR)/bmp.o
SCP079_FACE_OBJ = $(BUILD_DIR)/scp079_face.o
SCP079_FACE2_OBJ = $(BUILD_DIR)/scp079_face2.o
COMPILER_OBJ = $(BUILD_DIR)/compiler.o
FS_OBJ = $(BUILD_DIR)/fs.o
LIB_OBJ = $(BUILD_DIR)/string.o

# Output
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_IMG = $(BUILD_DIR)/os.img

# Default target
all: $(OS_IMG)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile bootloader
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

# Compile kernel assembly
$(KERNEL_ASM_OBJ): $(KERNEL_ASM_SRC) | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

# Compile kernel C++
$(KERNEL_CPP_OBJ): $(KERNEL_CPP_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile VGA driver
$(VGA_OBJ): $(VGA_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile keyboard driver
$(KEYBOARD_OBJ): $(KEYBOARD_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile mouse driver
$(MOUSE_OBJ): $(MOUSE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile graphics driver
$(GRAPHICS_OBJ): $(GRAPHICS_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile BMP loader
$(BMP_OBJ): $(BMP_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile SCP-079 face data
$(SCP079_FACE_OBJ): $(SCP079_FACE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile SCP-079 face2 data
$(SCP079_FACE2_OBJ): $(SCP079_FACE2_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile compiler
$(COMPILER_OBJ): $(COMPILER_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile file system
$(FS_OBJ): $(FS_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Compile string library
$(LIB_OBJ): $(LIB_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@

# Link kernel
$(KERNEL_BIN): $(KERNEL_ASM_OBJ) $(KERNEL_CPP_OBJ) $(COMPILER_OBJ) $(VGA_OBJ) $(KEYBOARD_OBJ) $(MOUSE_OBJ) $(GRAPHICS_OBJ) $(BMP_OBJ) $(SCP079_FACE_OBJ) $(SCP079_FACE2_OBJ) $(FS_OBJ) $(LIB_OBJ)
	$(LD) $(LDFLAGS) $^ -o $(BUILD_DIR)/kernel.elf
	objcopy -O binary $(BUILD_DIR)/kernel.elf $@

# Create OS image
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	# Pad to at least 1.44MB for floppy
	truncate -s 1440K $@

# ISO image output
OS_ISO = $(BUILD_DIR)/os.iso

# Run in QEMU (floppy mode)
run: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,media=disk

# Run ISO in QEMU (CD-ROM mode)
run-iso: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

# Run with debugging
debug: $(OS_IMG)
	qemu-system-i386 -drive file=$(OS_IMG),format=raw,index=0,media=disk -s -S

# Create bootable ISO image for CD-ROM burning
# Uses El Torito boot specification (supported by all BIOS since ~1995)
iso: $(OS_IMG)
	mkdir -p $(BUILD_DIR)/iso/boot
	cp $(OS_IMG) $(BUILD_DIR)/iso/boot/079os.img
	# Create El Torito boot catalog
	xorriso -as mkisofs \
		-o $(OS_ISO) \
		-b boot/079os.img \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-V "079OS" \
		-publisher "QUICKS" \
		-A "079OS Bootable CD" \
		$(BUILD_DIR)/iso
	@echo ""
	@echo "ISO created: $(OS_ISO)"
	@echo "Burn with: cdrecord -v dev=/dev/sr0 $(OS_ISO)"
	@echo "Or use any CD burning software (ImgBurn, Brasero, etc.)"

# Create hybrid ISO (boots from both CD-ROM and USB)
iso-hybrid: iso
	# Make ISO also bootable from USB (hybrid mode)
	dd if=$(OS_IMG) of=$(OS_ISO) conv=notrunc bs=512 count=2880
	@echo "Hybrid ISO created - boots from both CD and USB!"

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Install to USB/floppy (DANGEROUS - use with care!)
install: $(OS_IMG)
	@echo "=========================================="
	@echo "To install to USB flash drive or floppy:"
	@echo "=========================================="
	@echo ""
	@echo "Linux/Mac:"
	@echo "  sudo dd if=$(OS_IMG) of=/dev/sdX bs=512"
	@echo "  (replace /dev/sdX with your USB device)"
	@echo ""
	@echo "Windows:"
	@echo "  Use Rufus or Win32DiskImager"
	@echo ""
	@echo "To burn CD-ROM:"
	@echo "  make iso"
	@echo "  Then burn build/os.iso with your favorite burning software"

.PHONY: all run run-iso debug clean install iso iso-hybrid
