git clone https://github.com/Teemsploit/SoberFFlagEditor.git
cd SoberFFlagEditor
gcc $(pkg-config --cflags --libs json-c) -o fflag SoberFFlagEditor.c
sudo cp fflag /usr/local/bin/
sudo chmod +x /usr/local/bin/fflag
