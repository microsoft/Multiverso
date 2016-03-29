#build docker
sudo docker build -t multiverso-next .
#run test.sh
sudo docker run -t -i multiverso-next /etc/bootstrap.sh 
