#!/bin/bash
# ============================================================
# 079OS Debugger — static + runtime analysis via QEMU monitor
# ============================================================
set -E
trap 'echo "SCRIPT ERROR at line $LINENO (exit $?)" >&2' ERR

BUILD_DIR="build"
KERNEL_ELF="$BUILD_DIR/kernel.elf"
OS_IMG="$BUILD_DIR/os.img"
QEMU_LOG="$BUILD_DIR/qemu.log"
RUNTIME_LOG="$BUILD_DIR/debug_output.log"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

PASS=0
FAIL=0
WARN=0

pass()  { echo -e "  ${GREEN}[PASS]${NC} $1"; PASS=$((PASS+1)); }
fail()  { echo -e "  ${RED}[FAIL]${NC} $1"; FAIL=$((FAIL+1)); }
warn()  { echo -e "  ${YELLOW}[WARN]${NC} $1"; WARN=$((WARN+1)); }
info()  { echo -e "  ${CYAN}[INFO]${NC} $1"; }
header(){ echo -e "\n${CYAN}=== $1 ===${NC}"; }

# ============================================================
header "079OS DEBUGGER v1.0"
echo "  Static analysis + QEMU runtime verification"
# ============================================================

# ----------------------------------------------------------
# PHASE 1: Static binary analysis
# ----------------------------------------------------------
header "PHASE 1: Static Binary Analysis"

if [ ! -f "$KERNEL_ELF" ]; then
    echo "Building kernel..."
    make -s
fi

# 1.1 Section layout
info "Kernel ELF sections:"
size "$KERNEL_ELF" | tail -1 | while read text data bss dec hex file; do
    info "  .text=$text  .data=$data  .bss=$bss  total=$dec bytes"
done

# 1.2 BSS vs VGA overlap check
BSS_END_HEX=$(nm "$KERNEL_ELF" | grep '__bss_end' | awk '{print $1}')
BSS_START_HEX=$(nm "$KERNEL_ELF" | grep '__bss_start' | awk '{print $1}')
BSS_END=$((16#$BSS_END_HEX))
BSS_START=$((16#$BSS_START_HEX))
VGA_START=$((16#A0000))

BSS_SIZE=$((BSS_END - BSS_START))
info "BSS range: 0x$BSS_START_HEX - 0x$BSS_END_HEX ($BSS_SIZE bytes)"

if [ $BSS_END -lt $VGA_START ]; then
    MARGIN=$((VGA_START - BSS_END))
    pass "BSS ends below VGA hole (0xA0000) — margin: $MARGIN bytes ($((MARGIN/1024)) KB)"
else
    OVERLAP=$((BSS_END - VGA_START))
    fail "BSS OVERLAPS VGA hole by $OVERLAP bytes! Files will be corrupted!"
fi

# 1.3 Check node_pool placement
POOL_ADDR=$(nm "$KERNEL_ELF" | grep 'node_pool' | grep -v 'index\|head' | head -1 | awk '{print $1}')
POOL_IDX=$(nm "$KERNEL_ELF" | grep 'node_pool_index' | head -1 | awk '{print $1}')
POOL_FREE=$(nm "$KERNEL_ELF" | grep 'free_list_head' | head -1 | awk '{print $1}')

if [ -n "$POOL_ADDR" ] && [ -n "$POOL_IDX" ]; then
    POOL_START=$((16#$POOL_ADDR))
    POOL_END=$((16#$POOL_IDX))
    POOL_SIZE=$((POOL_END - POOL_START))
    NODE_SIZE=$((POOL_SIZE / 128))
    info "node_pool: 0x$POOL_ADDR - 0x$POOL_IDX ($POOL_SIZE bytes, $NODE_SIZE bytes/node)"

    if [ $POOL_END -lt $VGA_START ]; then
        pass "node_pool fits below VGA hole"
    else
        fail "node_pool extends into VGA hole!"
    fi

    if [ -n "$POOL_FREE" ]; then
        FREE_ADDR=$((16#$POOL_FREE))
        if [ $FREE_ADDR -lt $VGA_START ]; then
            pass "free_list_head (0x$POOL_FREE) below VGA hole"
        else
            fail "free_list_head in VGA memory!"
        fi
    fi
fi

# 1.4 Check kernel stack placement
STACK_BOT=$(nm "$KERNEL_ELF" | grep 'kernel_stack_bottom' | awk '{print $1}')
STACK_TOP=$(nm "$KERNEL_ELF" | grep 'kernel_stack_top' | awk '{print $1}')
if [ -n "$STACK_BOT" ] && [ -n "$STACK_TOP" ]; then
    STACK_SIZE=$(( 16#$STACK_TOP - 16#$STACK_BOT ))
    info "Kernel stack: 0x$STACK_BOT - 0x$STACK_TOP ($((STACK_SIZE/1024)) KB)"
    if [ $((16#$STACK_TOP)) -lt $VGA_START ]; then
        pass "Kernel stack below VGA hole"
    else
        fail "Kernel stack overlaps VGA hole!"
    fi
fi

# 1.5 Check kernel binary size vs bootloader sectors
KERNEL_BIN="$BUILD_DIR/kernel.bin"
if [ -f "$KERNEL_BIN" ]; then
    BIN_SIZE=$(stat -c%s "$KERNEL_BIN")
    SECTORS_NEEDED=$(( (BIN_SIZE + 511) / 512 ))
    info "kernel.bin: $BIN_SIZE bytes ($SECTORS_NEEDED sectors)"

    if [ $SECTORS_NEEDED -le 200 ]; then
        pass "Kernel fits in 200 boot sectors (uses $SECTORS_NEEDED)"
    else
        fail "Kernel needs $SECTORS_NEEDED sectors but bootloader loads only 200!"
    fi
fi

# 1.6 Check content buffer size
CONTENT_SIZE=$((NODE_SIZE - 64 - 4 - 4 - 4 - 4 - 4 - 4))
info "FileNode: $NODE_SIZE bytes/node, content buffer: ~$CONTENT_SIZE bytes"

# 1.7 Check for undefined symbols
UNDEF=$(nm "$KERNEL_ELF" | grep ' U ' | wc -l)
if [ "$UNDEF" -eq 0 ]; then
    pass "No undefined symbols"
else
    fail "$UNDEF undefined symbols found"
    nm "$KERNEL_ELF" | grep ' U ' | head -5 | while read line; do
        info "  $line"
    done
fi

# 1.8 Verify entry point
ENTRY=$(nm "$KERNEL_ELF" | grep ' _start$' | awk '{print $1}')
if [ -n "$ENTRY" ]; then
    info "Entry point _start at 0x$ENTRY"
    if [ "$((16#$ENTRY))" -eq "$((16#10000))" ]; then
        pass "Entry point matches linker base (0x10000)"
    else
        warn "Entry point 0x$ENTRY != expected 0x10000"
    fi
fi

# ----------------------------------------------------------
# PHASE 2: Runtime analysis via QEMU monitor
# ----------------------------------------------------------
header "PHASE 2: Runtime Analysis (QEMU monitor)"

# Key addresses
ROOT_DIR=$(nm "$KERNEL_ELF" | grep ' [BbDd] .*FileSystem.*root_dir' | head -1 | awk '{print $1}')
CUR_DIR=$(nm "$KERNEL_ELF" | grep ' [BbDd] .*FileSystem.*current_dir' | head -1 | awk '{print $1}')

info "Key symbols:"
info "  root_dir:         0x$ROOT_DIR"
info "  current_dir:      0x$CUR_DIR"
info "  node_pool:        0x$POOL_ADDR"
info "  node_pool_index:  0x$POOL_IDX"
info "  free_list_head:   0x$POOL_FREE"

QEMU_MON="/tmp/qemu-079os-mon.sock"
rm -f "$RUNTIME_LOG" "$QEMU_MON" gdb.txt

# Launch QEMU with monitor on a Unix socket
info "Starting QEMU (headless, monitor socket)..."
qemu-system-i386 \
    -drive file="$OS_IMG",format=raw,index=0,media=disk \
    -display none \
    -no-reboot \
    -S \
    -monitor unix:$QEMU_MON,server,nowait \
    -d cpu_reset \
    -D "$QEMU_LOG" \
    &
QEMU_PID=$!
sleep 1

if ! kill -0 $QEMU_PID 2>/dev/null; then
    fail "QEMU failed to start"
else
    info "QEMU running (PID $QEMU_PID)"

    # Helper: send monitor command and capture output
    # Strip ANSI escape codes (QEMU readline echoes input char-by-char with cursor/erase codes)
    qmon() {
        echo "$1" | socat -t2 - UNIX-CONNECT:$QEMU_MON 2>/dev/null | \
            sed $'s/\x1b\[[0-9;]*[A-Za-z]//g; s/\r//g' | \
            grep -v '^(qemu)' | grep -v '^QEMU' | grep -v '^\s*$' || true
    }

    # Boot the OS
    # kernel_main: boot_sequence() has delay loops, then system_ready() waits
    # for X keypress. We must send the keypress via monitor to proceed.
    info "Booting OS..."
    qmon "cont"
    sleep 15         # Wait for boot_sequence delay loops to finish
    qmon "sendkey x" # Press X to pass system_ready() screen
    sleep 3
    qmon "sendkey x" # Retry in case first was too early
    sleep 8          # Wait for main_interface + FS init

    # Stop the CPU for memory inspection
    qmon "stop"
    sleep 1
    info "CPU stopped, inspecting memory..."

    # Dump key memory via QEMU monitor 'xp' command
    qmon "xp /1xw 0x$ROOT_DIR"  > "$BUILD_DIR/mon_root_dir.txt"
    qmon "xp /1xw 0x$CUR_DIR"   > "$BUILD_DIR/mon_cur_dir.txt"
    qmon "xp /1xw 0x$POOL_IDX"  > "$BUILD_DIR/mon_pool_idx.txt"
    qmon "xp /1xw 0x$POOL_FREE" > "$BUILD_DIR/mon_free_head.txt"

    # Get the root_dir pointer value for deeper inspection
    ROOT_PTR_HEX=$(qmon "xp /1xw 0x$ROOT_DIR" | grep ':' | grep -oP '0x[0-9a-f]+' | head -1)

    CHILD_COUNT=0
    if [ -n "$ROOT_PTR_HEX" ]; then
        info "root_dir -> $ROOT_PTR_HEX"
        ROOT_PTR_DEC=$((ROOT_PTR_HEX))

        # Read root node name (16 words = 64 bytes)
        qmon "xp /16xw $ROOT_PTR_HEX" > "$BUILD_DIR/mon_root_name.txt"

        # Read root type (offset +64)
        ROOT_TYPE_ADDR=$(printf "0x%x" $((ROOT_PTR_DEC + 64)))
        qmon "xp /1xw $ROOT_TYPE_ADDR" > "$BUILD_DIR/mon_root_type.txt"

        # Read root first_child (offset = 64+4+4+content_sz+4+4)
        FC_OFF=$((64 + 4 + 4 + CONTENT_SIZE + 4 + 4))
        ROOT_FC_ADDR=$(printf "0x%x" $((ROOT_PTR_DEC + FC_OFF)))
        qmon "xp /1xw $ROOT_FC_ADDR" > "$BUILD_DIR/mon_root_child.txt"

        # Walk children via next_sibling pointers
        NS_OFF=$((64 + 4 + 4 + CONTENT_SIZE + 4 + 4 + 4))
        CHILD_HEX=$(qmon "xp /1xw $ROOT_FC_ADDR" | grep ':' | grep -oP '0x[0-9a-f]+' | head -1)

        while [ -n "$CHILD_HEX" ] && [ "$CHILD_HEX" != "0x00000000" ] && [ $CHILD_COUNT -lt 20 ]; do
            CHILD_DEC=$((CHILD_HEX))
            CHILD_NS_ADDR=$(printf "0x%x" $((CHILD_DEC + NS_OFF)))
            CHILD_NS_OUT=$(qmon "xp /1xw $CHILD_NS_ADDR")
            CHILD_HEX=$(echo "$CHILD_NS_OUT" | grep ':' | grep -oP '0x[0-9a-f]+' | head -1)
            CHILD_COUNT=$((CHILD_COUNT + 1))
        done
    fi

    # BSS zero check: read 3 words from pool[50], pool[100], pool[120]
    BSS_CHECK=""
    for idx in 50 100 120; do
        ADDR=$(printf "0x%x" $((0x$POOL_ADDR + idx * NODE_SIZE)))
        BSS_CHECK+=$(qmon "xp /3xw $ADDR")$'\n'
    done
    echo "$BSS_CHECK" > "$BUILD_DIR/mon_bss_check.txt"

    # Register dump for ESP check
    qmon "info registers" > "$BUILD_DIR/mon_regs.txt"
    ESP_HEX=$(grep -oP 'ESP=\K[0-9a-f]+' "$BUILD_DIR/mon_regs.txt" || true)

    info "Memory dumped. Analyzing..."

    # ---- Analyze dumped data and produce results ----
    {
        echo "====== POST-INIT FILESYSTEM CHECK ======"

        # Parse xp output: find lines with ':' (data lines), extract first 0x value
        xpval() { grep ':' "$1" | head -1 | grep -oP '0x[0-9a-f]+' | head -1; }
        ROOT_PTR=$(xpval "$BUILD_DIR/mon_root_dir.txt")
        CUR_PTR=$(xpval "$BUILD_DIR/mon_cur_dir.txt")
        PIDX=$(xpval "$BUILD_DIR/mon_pool_idx.txt")
        FHEAD=$(xpval "$BUILD_DIR/mon_free_head.txt")

        echo "root_dir     = $ROOT_PTR"
        echo "current_dir  = $CUR_PTR"
        echo "node_pool_index = $((PIDX))"
        echo "free_list_head  = $FHEAD"

        # Check root_dir in pool
        ROOT_DEC=$((ROOT_PTR))
        POOL_S=$((0x$POOL_ADDR))
        POOL_E=$((0x$POOL_IDX))
        if [ $ROOT_DEC -ge $POOL_S ] && [ $ROOT_DEC -lt $POOL_E ]; then
            echo "PASS: root_dir points into node_pool"
        else
            echo "FAIL: root_dir OUTSIDE node_pool"
        fi

        # Check root name (first byte should be '/' = 0x2f)
        # QEMU xp output format: "ADDR: 0xVAL 0xVAL ..." — extract first data value
        FIRST_WORD=$(grep ':' "$BUILD_DIR/mon_root_name.txt" | head -1 | grep -oP '0x[0-9a-f]+' | head -1)
        if [ -n "$FIRST_WORD" ]; then
            FIRST_BYTE=$((FIRST_WORD & 0xFF))
            if [ $FIRST_BYTE -eq 47 ]; then
                echo "PASS: root_dir name is '/'"
            else
                echo "FAIL: root_dir name first byte = $FIRST_BYTE (expected 47='/')"
            fi
        fi

        # Check root type (should be 1 = DIRECTORY)
        ROOT_TYPE=$(xpval "$BUILD_DIR/mon_root_type.txt")
        ROOT_TYPE_DEC=$((ROOT_TYPE))
        if [ $ROOT_TYPE_DEC -eq 1 ]; then
            echo "PASS: root_dir type = DIRECTORY (1)"
        else
            echo "FAIL: root_dir type = $ROOT_TYPE_DEC, expected 1"
        fi

        # Check first_child
        ROOT_CHILD=$(xpval "$BUILD_DIR/mon_root_child.txt")
        ROOT_CHILD_DEC=$((ROOT_CHILD))
        if [ $ROOT_CHILD_DEC -ge $POOL_S ] && [ $ROOT_CHILD_DEC -lt $POOL_E ]; then
            echo "PASS: first_child ($ROOT_CHILD) points into node_pool"
        elif [ $ROOT_CHILD_DEC -eq 0 ]; then
            echo "FAIL: root has NO children after init!"
        else
            echo "FAIL: first_child $ROOT_CHILD OUTSIDE node_pool!"
        fi

        # Children count
        echo "Root children: $CHILD_COUNT"
        if [ $CHILD_COUNT -ge 3 ]; then
            echo "PASS: root has $CHILD_COUNT children"
        else
            echo "FAIL: root has only $CHILD_COUNT children (expected >=3)"
        fi

        # Pool index check
        PIDX_DEC=$((PIDX))
        if [ $PIDX_DEC -gt 0 ]; then
            echo "PASS: $PIDX_DEC nodes allocated during init"
        else
            echo "FAIL: node_pool_index = 0 after init!"
        fi

        # Free list check
        FHEAD_DEC=$((FHEAD))
        if [ $FHEAD_DEC -eq 0 ]; then
            echo "PASS: free_list empty (no deletions yet)"
        else
            echo "WARN: free_list not empty ($FHEAD)"
        fi

        echo ""
        echo "====== MEMORY REGION CHECK ======"
        LAST_END=$((POOL_S + 128 * NODE_SIZE))
        echo "node_pool ends at: $(printf '0x%x' $LAST_END)"
        if [ $LAST_END -le $((0xA0000)) ]; then
            echo "PASS: All nodes below VGA boundary"
        else
            echo "FAIL: node_pool extends into VGA memory!"
        fi

        echo ""
        echo "====== BSS ZERO CHECK ======"
        for idx in 50 100 120; do
            ADDR=$((0x$POOL_ADDR + idx * NODE_SIZE))
            ADDR_HEX=$(printf '0x%x' $ADDR)
            VALS=$(grep "$ADDR_HEX" "$BUILD_DIR/mon_bss_check.txt" | grep ':' | grep -oP '0x[0-9a-f]+' | head -3)
            ALL_ZERO=true
            for v in $VALS; do
                if [ "$v" != "0x00000000" ]; then
                    ALL_ZERO=false
                fi
            done
            if $ALL_ZERO; then
                echo "PASS: pool[$idx] at $ADDR_HEX is zeroed"
            else
                echo "FAIL: pool[$idx] at $ADDR_HEX NOT zeroed: $VALS"
            fi
        done

        echo ""
        echo "====== STACK CHECK ======"
        if [ -n "$ESP_HEX" ]; then
            ESP_DEC=$((16#$ESP_HEX))
            echo "ESP = 0x$ESP_HEX"
            SBOT=$((0x$STACK_BOT))
            STOP=$((0x$STACK_TOP))
            if [ $ESP_DEC -ge $SBOT ] && [ $ESP_DEC -le $STOP ]; then
                USED=$((STOP - ESP_DEC))
                TOTAL=$((STOP - SBOT))
                PCT=$((USED * 100 / TOTAL))
                echo "PASS: Stack within kernel_stack region"
                echo "  Stack used: $USED bytes / $TOTAL bytes ($PCT%)"
            else
                echo "WARN: ESP 0x$ESP_HEX outside kernel_stack (0x$STACK_BOT-0x$STACK_TOP)"
            fi
        else
            echo "WARN: Could not read ESP"
        fi

        echo ""
        echo "====== DEBUG COMPLETE ======"
    } > "$RUNTIME_LOG" 2>&1

    # Kill QEMU
    kill $QEMU_PID 2>/dev/null || true
    wait $QEMU_PID 2>/dev/null || true
fi

# ----------------------------------------------------------
# PHASE 3: Parse and display results
# ----------------------------------------------------------
header "PHASE 3: Results"

if [ -f "$RUNTIME_LOG" ]; then
    RT_PASS=$(grep -c 'PASS' "$RUNTIME_LOG" 2>/dev/null) || RT_PASS=0
    RT_FAIL=$(grep -c 'FAIL' "$RUNTIME_LOG" 2>/dev/null) || RT_FAIL=0
    RT_WARN=$(grep -c 'WARN' "$RUNTIME_LOG" 2>/dev/null) || RT_WARN=0

    PASS=$((PASS + RT_PASS))
    FAIL=$((FAIL + RT_FAIL))
    WARN=$((WARN + RT_WARN))

    echo ""
    echo "--- Runtime Analysis Log ---"
    cat "$RUNTIME_LOG"
    echo "--- End Runtime Log ---"
else
    warn "No runtime analysis output found"
fi

# Final summary
header "SUMMARY"
echo -e "  ${GREEN}PASS: $PASS${NC}"
echo -e "  ${YELLOW}WARN: $WARN${NC}"
echo -e "  ${RED}FAIL: $FAIL${NC}"
echo ""

if [ $FAIL -gt 0 ]; then
    echo -e "  ${RED}RESULT: ISSUES FOUND${NC}"
    exit 1
else
    echo -e "  ${GREEN}RESULT: ALL CHECKS PASSED${NC}"
    exit 0
fi
