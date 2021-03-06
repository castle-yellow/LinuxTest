/* 
    Measure latency of IPC using tcp sockets


    Copyright (c) 2010 Erik Rigtorp <erik@rigtorp.com>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <netdb.h>


int main(int argc, char *argv[])
{
    int size;
    char *buf;
    int64_t count, i, delta, j;
    struct timeval start, stop;
    float latency[11];
    float sum = 0;
    ssize_t len;
    size_t sofar;
    
    int yes = 1;
    int ret;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints;
    struct addrinfo *res;
    int sockfd, new_fd;
    
    if (argc != 3) {
        printf ("usage: tcp_lat <message-size> <roundtrip-count>\n");
        return 1;
    }
    
    size = atoi(argv[1]);
    count = atol(argv[2]);
    
    buf = malloc(size);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    if ((ret = getaddrinfo("127.0.0.1", "3491", &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }
    
    printf("message size: %i octets\n", size);
    printf("roundtrip count: %lli\n", count);
    
    if (!fork()) {  /* child */
        
        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("socket");
            exit(1);
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            perror("bind");
            exit(1);
        }
        
        if (listen(sockfd, 1) == -1) {
            perror("listen");
            exit(1);
        }
        
        addr_size = sizeof their_addr;
        
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1) {
            perror("accept");
            exit(1);
        }
        
        for(j = 1; j <= 10; j++)
        {
            for (i = 0; i < count; i++) {
                
                for (sofar = 0; sofar < size; ) {
                    len = read(new_fd, buf, size - sofar);
                    if (len == -1) {
                        perror("read");
                        return 1;
                    }
                    sofar += len;
                }
                
                if (write(new_fd, buf, size) != size) {
                    perror("write");
                    return 1;
                }
            }
        }
        
    } else { /* parent */
        
        sleep(1);
        
        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("socket");
            exit(1);
        }
        
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            perror("connect");
            exit(1);
        }
        for (j = 1; j <= 10 ; j++) {
            
            gettimeofday(&start, NULL);
            
            for (i = 0; i < count; i++) {
                
                if (write(sockfd, buf, size) != size) {
                    perror("write");
                    return 1;
                }
                
                for (sofar = 0; sofar < size; ) {
                    len = read(sockfd, buf, size - sofar);
                    if (len == -1) {
                        perror("read");
                        return 1;
                    }
                    sofar += len;
                }
                
            }
            
            gettimeofday(&stop, NULL);
            
            delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1e6 +
                     stop.tv_usec - start.tv_usec);
            latency[j] = delta / (float)(count * 2);
            sum += latency[j];
            printf("The %d time latency: %f us\n", j , latency[j]);
            
            
        }
        printf("The average latency: %f us\n" , sum / 10);
    }
    
    return 0;
}
