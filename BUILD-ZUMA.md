```
git clone https://github.com/kyle-sylvestre/PvZ-Portable --branch zuma
cd PvZ-Portable
bash scripts/create_flat_directory.sh
git clone https://github.com/kyle-sylvestre/CircleShootApp.git src/CircleShoot
cmake -S . -B build
cmake --build build
```
