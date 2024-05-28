#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;

vector<int> List;

void usage() {
	printf("syntax: echo-server <port> [-e[-b]]\n");
	printf("sample: echo-server 1234 -e -b\n");
}

struct Param {
	bool echo{false};
	bool broadcast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		if(argc < 2  || argc > 4){
			return 0;
		}
		port = stoi(argv[1]);
		for (int i = 2; i < argc; i++) {
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				continue;
			}
			else if(strcmp(argv[i], "-b") == 0){
				broadcast = true;
				continue;
			}
		}
		return port != 0;
	}
} param;

void recvThread(int sd) {
	printf("connected\n");
	fflush(stdout);
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = ::recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			fprintf(stderr, "recv return %zd", res);
			perror(" ");
			break;
		}
		buf[res] = '\0';
		printf("%s\n", buf);
		fflush(stdout);
		if (param.echo) {
			res = ::send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				fprintf(stderr, "send return %zd", res);
				perror(" ");
				break;
			}
		}
		else if(param.broadcast){
			for(auto ssd: List){
				res = ::send(ssd, buf, res, 0);
				if (res == 0 || res == -1) {
					fprintf(stderr, "send return %zd", res);
					perror(" ");
					break;
				}
			}
		}
	}
	printf("disconnected\n");
	fflush(stdout);
	::close(sd);
	for(int i=0;i<List.size();i++){
		if(List[i] == sd){
			List.erase(List.begin()+i);
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	int sd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int optval = 1;
	int res = ::setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int newsd = ::accept(sd, (struct sockaddr *)&cli_addr, &len);
		List.push_back(newsd);
		if (newsd == -1) {
			perror("accept");
			break;
		}
		std::thread* t = new std::thread(recvThread, newsd);
		t->detach();
	}
	close(sd);
}
