/* 
    Measure throughput of IPC using pipes


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
#include <time.h>
#include <stdint.h>


int main(int argc, char *argv[])
{
    int fds[2];
    
    int size;
    char *buf;
    int64_t count, i, delta , j;
    int64_t throughput_msg[11];
    int64_t throughput[11];
    int64_t sum = 0 ;
    int64_t sum_msg = 0 ;
    struct timeval start, stop;
    
    if (argc != 3) {
        printf ("usage: pipe_thr <message-size> <message-count>\n");
        exit(1);
    }
    
    size = atoi(argv[1]);
    count = atol(argv[2]);
    
    buf = malloc(size);
    if (buf == NULL) {
        perror("malloc");
        exit(1);
    }
    
    printf("message size: %i octets\n", size);
    printf("message count: %lli\n", count);
    
    if (pipe(fds) == -1) {
        perror("pipe");
        exit(1);
    }
    
    if (!fork()) {
        /* child */
        
        for(j = 0 ; j <= 10 ; j++)
        {
            for (i = 0; i < count; i++) {
                if (read(fds[0], buf, size) != size) {
                    perror("read");
                    exit(1);
                }
            }
        }
    } else {
        /* parent */
        
        for(j = 1 ; j <= 10 ; j++ ) {
            gettimeofday(&start, NULL);
            
            for (i = 0; i < count; i++) {
                if (write(fds[1], buf, size) != size) {
                    perror("write");
                    exit(1);
                }
            }
            
            gettimeofday(&stop, NULL);
            
            delta = ((stop.tv_sec - start.tv_sec) * (int64_t) 1e6 +
                     stop.tv_usec - start.tv_usec);
            throughput_msg[j] = (count * (int64_t) 1e6) / delta ;
            throughput[j] = (((count * (int64_t) 1e6) / delta) * size ) / (int64_t) 1e6 ;
            sum_msg += throughput_msg[j];
            sum += throughput[j];
            printf("The %d time throughput: %lli msg/s\n", j , throughput_msg[j]);
            printf("The %d time throughput: %lli MB/s\n", j, throughput[j]);
        }
        printf("The average throughput: %lli msg/s\n" ,  sum_msg);
        printf("The average throughput: %lli MB/s\n" , sum / 10);
    }
    
    return 0;
}
