#!/bin/bash
set -e

echo "============================================"
echo "  FreeRTOS + UVM-SystemC Environment Setup"
echo "============================================"

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# ==================== 1. APT PACKAGES ====================
echo "[1/5] Installing apt packages..."
sudo apt-get update
sudo apt-get install -y \
  build-essential gcc g++ gdb cmake git python3 python3-pip bc \
  libpthread-stubs0-dev libstdc++-12-dev \
  autoconf automake libtool pkg-config bison flex


# ==================== 2. SYSTEMC ====================
echo "[2/5] Building SystemC 3.0.2..."
if [ ! -d "systemc" ]; then
  git clone https://github.com/accellera-official/systemc.git --depth 1
fi
cd systemc
mkdir -p build && cd build
cmake .. \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-std=c++17 -fPIC -O2"
make -j$(nproc)
sudo make install
sudo ldconfig
cd "$PROJECT_ROOT"

# ==================== 3. UVM-SYSTEMC ====================
echo "[3/5] Building UVM-SystemC 1.0-beta6..."
if [ ! -d "uvm-systemc" ]; then
  git clone https://github.com/accellera-official/uvm-systemc.git --depth 1
fi
cd uvm-systemc

# Bootstrap autotools
libtoolize --force
aclocal -I config
autoheader
automake --add-missing --copy
autoconf

mkdir -p build && cd build
../configure \
  --prefix=/usr/local \
  --with-systemc=/usr/local \
  CXXFLAGS="-std=c++17 -O2 -fPIC" \
  LDFLAGS="-L/usr/local/lib -L/usr/local/lib-linux64"
make -j$(nproc)
sudo make install
sudo ldconfig
cd "$PROJECT_ROOT"

# ==================== 4. LD CONFIG ====================
echo "[4/5] Configuring shared library paths..."
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/systemc.conf > /dev/null
echo "/usr/local/lib-linux64" | sudo tee /etc/ld.so.conf.d/uvm-systemc.conf > /dev/null
sudo ldconfig

# ==================== 5. FREERTOS ====================
echo "[5/5] Cloning FreeRTOS POSIX..."
if [ ! -d "FreeRTOS" ]; then
  git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules --depth 1 --shallow-submodules
fi

echo ""
echo "============================================"
echo "  Environment setup complete!"
echo "============================================"
echo ""
echo "Run ./verify_env.sh to confirm installation."
