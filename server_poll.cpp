#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <iostream>

#define USER_LIMIT 5

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stdout, "请输入ip:port\n");
        exit(1);
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int ret;
    ret = bind(listenfd, (sockaddr*)&addr, sizeof addr);
    assert(ret != -1);

    ret = listen(listenfd, 20);
    assert(ret != -1);

    sockaddr_in client_addr;
    bzero(&client_addr, sizeof client_addr);
    socklen_t client_addr_len;
    
    pollfd fd_list[USER_LIMIT + 1];
    bzero(&fd_list, sizeof fd_list);

    fd_list[0].fd = listenfd;
    fd_list[0].events = POLLIN | POLLERR;
    
    int user_count = 0;
    fprintf(stdout, "开始事件循环====\n");
    while (1) {
        ret = poll(fd_list, user_count + 1, -1); // 第二个参数为监听列表长度，一开始只监听服务端listenfd
        assert(ret >= 0);
        // poll需要手动去遍历监听列表
        for (int i = 0; i < user_count + 1; ++i) {
            if (fd_list[i].fd == listenfd && fd_list[i].revents & POLLIN) {
                int connfd = accept(listenfd, (sockaddr*)&client_addr, &client_addr_len);
                if (connfd < 0) {
                    fprintf(stdout, "connect error\n");
                    continue;
                }
                if (user_count >= USER_LIMIT) {
                    fprintf(stdout, "to  many users!\n");
                    close(connfd);
                    continue;
                }
                user_count++;
                fd_list[user_count].fd = connfd;
                fd_list[user_count].events = POLLIN | POLLRDHUP | POLLERR;
                fd_list[user_count].revents = 0; // mark!: 之前这个位置可能被设置了但client退出了，新连接到来时revents不为0，所以需要设置为0
                fprintf(stdout, "new client!\n");
            } else if (fd_list[i].revents & POLLRDHUP) { // 客户端close或shutdown
                close(fd_list[i].fd);
                fd_list[i] = fd_list[user_count];
                i--;
                user_count--;
                fprintf(stdout, "a client left\n");
            } else if (fd_list[i].revents & POLLIN) {
                char buf[1024];
                bzero(&buf, sizeof buf);
                ret = recv(fd_list[i].fd, buf, sizeof buf, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        close(fd_list[i].fd);
                        fd_list[i] = fd_list[user_count];
                        i--;
                        user_count--;
                    }
                } else if (ret == 0) {
                    continue;
                } else {
                    fprintf(stdout, "%s\n", buf);
                    char send_buf[] = "I'm server!";
                    send(fd_list[i].fd, send_buf, sizeof send_buf, 0);
                }
            } 
        }
    }

    return 0;
}