#include <iostream>
#include <sys/socket.h>
#include <string.h> // bzero
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char* argv[]){
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;

    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    int ret;
    ret = bind(listenfd, (sockaddr*)&addr, sizeof addr);
    assert(ret != -1);

    ret = listen(listenfd, 20);
    assert(ret != -1);

    sockaddr_in client;
    socklen_t client_addr_len = sizeof client;

    int max_event_num = 128;
    epoll_event events[max_event_num];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd, listenfd);

    while (1) {
        int number = epoll_wait(epollfd, events, max_event_num, -1);
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (listenfd == sockfd) {
                int connfd = accept(listenfd, (sockaddr*)&client, &client_addr_len);
                char tmp[64];
                fprintf(stdout, "client ip:%s\n", inet_ntop(AF_INET, (void*)(&client.sin_addr), tmp, INET_ADDRSTRLEN));
                fprintf(stdout, "client port:%d\n", htons(client.sin_port));
                addfd(epollfd, connfd);
            } else if (events[i].events & EPOLLIN) {
                char buf[128];
                while (1) {
                    memset(buf, '\0', sizeof buf);
                    ret = recv(sockfd, buf, sizeof buf - 1, 0);
                    if (ret < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;
                        close(sockfd);
                        break; // 这里只是推出接收循环，服务器依旧处在监听状态
                    } else if (ret == 0) { // 客户端断开连接
                        close(sockfd);
                        break;
                    }
                    else {
                        cout << buf << endl;
                        char send_buf[] = "I'm server!";
                        send(sockfd, send_buf, sizeof send_buf, 0);
                    }
                }
            }
        }
    }
    close(listenfd);
    return 0;
}
