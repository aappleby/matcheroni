echo "FIXME - we are pointed at hancho/v020/hancho.py and not main"
if [ ! -f "hancho.py" ]; then
  echo "Fetching Hancho build script"
  wget -q https://raw.githubusercontent.com/aappleby/hancho/v020/hancho.py
fi
python3 hancho.py
