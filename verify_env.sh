#!/bin/bash

echo "=== FreeRTOS + UVM-SystemC Environment Verification ==="
echo "Workspace: $PWD"
echo "Date: $(date)"
echo ""

# ==================== 1. OS ====================
echo "[1] OS Information:"
cat /etc/os-release | grep -E "PRETTY_NAME|VERSION"
echo ""

# ==================== 2. Toolchain ====================
echo "[2] Toolchain:"
gcc --version | head -1
g++ --version | head -1
cmake --version | head -1
python3 --version
echo ""

# ==================== 3. SystemC ====================
echo "[3] SystemC:"
[ -f /usr/local/include/systemc.h ] && echo "  [OK] systemc.h found" || echo "  [FAIL] systemc.h MISSING"

ls /usr/local/lib/libsystemc.so* >/dev/null 2>&1 && echo "  [OK] libsystemc.so found" || echo "  [FAIL] libsystemc.so MISSING"
echo ""

# ==================== 4. UVM-SystemC ====================
echo "[4] UVM-SystemC:"
[ -f /usr/local/include/uvm.h ] && echo "  [OK] uvm.h found" || echo "  [FAIL] uvm.h MISSING"

ls /usr/local/lib-linux64/libuvm-systemc*.so >/dev/null 2>&1 && echo "  [OK] libuvm-systemc.so found" || echo "  [FAIL] libuvm-systemc.so MISSING"
echo ""

# ==================== 5. FreeRTOS ====================
echo "[5] FreeRTOS POSIX:"
[ -f FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c ] && echo "  [OK] FreeRTOS POSIX port found" || echo "  [FAIL] FreeRTOS POSIX port MISSING"
echo ""

# ==================== 6. Library Cache ====================
echo "[6] Shared Libraries (ldconfig):"
ldconfig -p | grep -q "libuvm-systemc" && echo "  [OK] UVM-SystemC in ldconfig" || echo "  [FAIL] UVM-SystemC not in ldconfig"
ldconfig -p | grep -q "libsystemc" && echo "  [OK] SystemC in ldconfig" || echo "  [WARN] SystemC not in ldconfig"
echo ""

echo "=== Verification Complete ==="
echo ""
echo "If you see [OK] marks, your environment is ready!"