#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>

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

    fd_set read_fds;
    FD_ZERO(&read_fds);
    int client[64];
    int user_count = 0;
    
    fprintf(stdout, "开始事件循环======\n");
    while (1) {
        // select需要在每次循环开始清空重置fds
        FD_ZERO(&read_fds);
        FD_SET(listenfd, &read_fds);
        for (int i = 1; i <= user_count; ++i) {
            FD_SET(client[i], &read_fds);
        }
        ret = select(32, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            fprintf(stdout, "selection failure\n");
            break;
        }
        if (FD_ISSET(listenfd, &read_fds)) {
            int connfd = accept(listenfd, (sockaddr*)&client_addr, &client_addr_len);
            if (connfd < 0) {
                fprintf(stdout, "errno is: %d\n", errno);
                close(listenfd);
                break;
            }
            fprintf(stdout, "new client!\n");
            user_count++;
            client[user_count] = connfd;
        }
        for (int i = 1; i <= user_count; ++i) {
            if (FD_ISSET(client[i], &read_fds)) {
                char buf[128];
                ret = recv(client[i], buf, sizeof(buf) - 1, 0);
                if (ret < 0) {
                    break;
                } else if (ret == 0) {
                    fprintf(stdout, "client leave!\n");
                    close(client[i]);
                    client[i] = client[user_count];
                    user_count--;
                    i--;
                } else {
                    fprintf(stdout, "%s\n", buf);
                    char send_buf[] = "I'm server!";
                    send(client[i], send_buf, sizeof send_buf, 0);
                }
            }
        }
    }

    return 0;
}