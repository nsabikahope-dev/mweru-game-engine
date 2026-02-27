# Installing Lua for Scripting Support

To enable Lua scripting in the engine, you need to install Lua 5.4 development files:

```bash
sudo apt-get install -y liblua5.4-dev lua5.4
```

After installation, rebuild the engine:
```bash
cd build
cmake ..
make -j$(nproc)
```

## Verifying Installation

Check if Lua is installed:
```bash
lua -v
```

Should output: `Lua 5.4.x`

## Alternative: Manual Installation

If you can't use apt-get, download and build Lua from source:
```bash
cd vendor
wget https://www.lua.org/ftp/lua-5.4.6.tar.gz
tar -xzf lua-5.4.6.tar.gz
cd lua-5.4.6
make linux
sudo make install
```
