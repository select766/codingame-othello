#ifndef _DNN_EVALUATOR_SOCKET_
#define _DNN_EVALUATOR_SOCKET_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "dnn_evaluator.hpp"

class DNNEvaluatorSocket : public DNNEvaluator
{
    int sock;

public:
    DNNEvaluatorSocket(const char *ip_addr, int port)
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;
        addr.sin_port = htons((unsigned short)port);
        addr.sin_addr.s_addr = inet_addr(ip_addr);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            throw runtime_error("DNNEvaluatorSocket: could not create socket");
        }
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            throw runtime_error("DNNEvaluatorSocket: could not connect to server");
        }
    }

    ~DNNEvaluatorSocket()
    {
        if (sock >= 0)
        {
            close(sock);
            sock = -1;
        }
    }

    DNNEvaluatorResult evaluate(const Board &board)
    {
        DNNEvaluatorRequest req = make_request(board);
        auto send_size = send(sock, &req, sizeof(req), 0);
        if (send_size != sizeof(req))
        {
            throw runtime_error("DNNEvaluatorSocket: send error");
        }

        DNNEvaluatorResult res;
        unsigned char *p = reinterpret_cast<unsigned char *>(&res);
        ssize_t remain_size = sizeof(res);
        while (remain_size > 0)
        {
            ssize_t recv_size = recv(sock, p, remain_size, 0);
            if (recv_size <= 0)
            {
                throw runtime_error("DNNEvaluatorSocket: recv error");
            }
            remain_size -= recv_size;
            p += recv_size;
        }

        return res;
    }
};
#endif
