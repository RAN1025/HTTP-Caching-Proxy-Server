FROM ubuntu:18.04

RUN mkdir /proxy
RUN apt-get update && apt-get -y install g++ make
WORKDIR /proxy
ADD . /proxy
RUN apt-get update && apt-get -y install sudo && sudo chown root ./start && \
	sudo chmod 4755 ./start

