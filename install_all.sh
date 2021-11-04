#Script has already privilege in order to avoid ask password for each command ('sudo chmod +s ./install_all.sh')
#Launch with 'sudo ./install_all.sh'

apt-get --assume-yes update
apt-get --assume-yes install libsdl2-image-dev tortoisehg valgrind libglfw3-dev snapd build-essential meld python-iniparse
apt --assume-yes autoremove

#If clion snap update is needed, uncomment below
#snap install core
## if a proxy error occurrs, uncomment below and rerun snap install core
## snap set system proxy.http=socks5://127.0.0.1:9050
## snap set system proxy.https=socks5://127.0.0.1:9050
## iptables -I OUTPUT 3 -d 127.0.0.1/32 -o lo -p tcp -m tcp --dport 9050 --tcp-flags FIN,SYN,RST,ACK SYN -m owner --uid-owner 0 -j ACCEPT
#snap install clion --classic
#snap run clion &

export CLION_FOLDER="<PATH TO CLION>"
"$CLION_FOLDER"/bin/clion.sh &

