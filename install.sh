systemctl disable flytocamera
cp flytocamera.service /etc/systemd/system
systemctl enable flytocamera
systemctl start flytocamera

