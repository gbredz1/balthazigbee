. $HOME/esp/esp-idf/export.sh
idf.py set-target esp32c6
idf.py build erase-flash flash monitor -p /dev/ttyUSB0