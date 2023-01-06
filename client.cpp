#include <iostream>
#include <sys/socket.h>
#include <string.h> // bzero
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <sys/select.h>
using namespace std;

int main(int argc, char* argv[]){
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    cout << "server ip:" << ip << " port:" << port << endl;

    int sockfd = socket(PF_INET, SOCK_STREAM, 0); // PF_INET用于IPV4 SOCK_STREAM数据流服务 Tcp
    if (sockfd < 0) {
        cout << "socket create failed!" << endl;
    }

    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;

    inet_pton(AF_INET, ip, &addr.sin_addr); // 将点分十进制ip转为整数表示的网络字节序 inte_pton先将字符串转为二进制 再将二进制转为网络字节序
    addr.sin_port = htons(port);  // host to network short 将主机字节序转为网络字节序 参数为short型
    int ret;
    cout << "start connect..." << endl;
    if (connect(sockfd, (sockaddr*)&addr, sizeof addr) != 0) {
        cout << "failed!" << endl;
    } else {
        const char* data = "hello";
        ret = send(sockfd, data, strlen(data), 0);
        if (ret == -1) {
            cout << "send error" << endl;
        } else {
            cout << "send " << ret << " bytes!" << endl;
        }
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        ret = select(32, &read_fds, NULL, NULL, NULL); // timeout设置为NULL会一直阻塞
        if (ret < 0) {
            fprintf(stdout, "selection failure\n");
            exit(1);
        }
        if (FD_ISSET(sockfd, &read_fds)) {
            char buf[1024];
            ret = recv(sockfd, buf, sizeof buf - 1, 0);
            if (ret < 0) {
                fprintf(stdout, "recv error!\n");
                exit(1);
            } else {
                fprintf(stdout, "received msg from server:%s\n", buf);
            }
        }
    }
    cout << "sleep..." << endl;
    sleep(2);
    close(sockfd);
}