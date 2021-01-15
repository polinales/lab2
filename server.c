#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 1024

/*
struct sigaction {
    void (*sa_handler)(int); //обработчик сигнала или действие
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask; //сигналы, которые следует блокировать  int sa_flags; // флаги
    void (*sa_restorer)(void); // устаревшее поле, не соответствует POSIX
};
*/

int getopt_long(int argc,
                char * const argv[],
                const char *optstring,
                const struct option *longopts,
                int *longindex);

char *Version = "0.4";
struct option *long_options;
int sock;
struct sockaddr_in serv_addr;
struct hostent *server;

const char* short_options = "-:a:p:vhw:dl:";
char *ipv4_addr_str;
char *port_str;
int ip_flag = 0;
int port_flag = 0;
char buffer[256];
char *log_file_addr;
int log_file_flag = 0;
int daemon_mode = 0;
int wait_mode = 0;
int wait_mode_flag = 0;
int all_calls = 0;
int success_calls = 0;
FILE *logfile;

int show_version(int x)
{
    printf("Version %s\n", Version);
    return 0;
}

int show_help(int x)
{
    printf("-----------------DESCRIPTION----------------\n");
    printf("This is server which makes anogramas from\n");
    printf("clients messages\n");
    printf("-------------------OPTIONS------------------\n");
    printf("-h\t shows this page\n");
    printf("-v\t show version of program\n");
    printf("-a\t choose IP address\n");
    printf("-p\t choose port\n");
    printf("-l\t add path to log file\n");
    printf("-w\t simulation of waiting in threads\n");
    printf("-d\t daemon mode\n");
    printf("--------------------------------------------\n");
    return 0;
}

void check(int ret)
{
    if (ret < 0) {
        //printf(stderr, "Error: failed to set proc mask: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int show_statistics(int x)
{
    fprintf(logfile, "-----------------STATISTICS----------------\n");
    fprintf(logfile, "ALL: %d\n", all_calls);
    fprintf(logfile, "SUCCESSFUL: %d\n", success_calls);

    if (daemon_mode == 0)
    {
        printf("-----------------STATISTICS----------------\n");
        printf("ALL: %d\n", all_calls);
        printf("SUCCESSFUL: %d\n", success_calls);
    }
    return 0;
}

typedef struct {
    char *in_str;
    int out_socket;
} thread_dataS;

typedef struct
{
    char c;
    int n;
}alphabet;


struct tm *ptr;
char tbuf[80];
time_t now;
char * getTime() //--функция определения времени в нужном формате
{
    char *ret;
    ret=(char*)malloc(100);
    bzero(ret,100);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,80,"%e-%B-%Y %H:%M:%S",ptr);
    ret=tbuf;
    return (ret);
}

int annogramme(char *str, char *dest)
{
    if (str == NULL)
    {
        return 1;
    }
    srand(time(NULL));
    //printf("str = %s\n", str);
    int r;
    int fl;
    int k = 0;
    //printf("in ann\n");
    alphabet *a = malloc(sizeof(alphabet) * strlen(str));
    //printf("NO SEGMENTATION FAULT ON ALPHABET word = %s\n", str);
    for (int i = 0; i < strlen(str); i++)
    {
        a[i].n = 0;
        a[i].c = 0;
    }
    if (a == NULL)
    {
        return 1;
    }
    for (int i = 0; i < strlen(str); i++)
    {
        fl = 0;
        for (int j = 0; j < strlen(str); j++)
        {
            if (str[i] == a[j].c)
            {
                a[j].n++;
                fl++;
                break;
            }
        }
        if (fl == 0)
        {
            a[k].c = str[i];
            a[k].n++;
            k++;
        }
    }

    //printf("--------\n");
    //printf("k = %d\n", k);
    for (int i = 0; i < k; i++)
    {
        //printf("- %c\t%d\n", a[i].c, a[i].n);
    }
    //printf("--------\n");

    char *bass = malloc(sizeof(char) * strlen(str));
    int nd;
    //printf("str len = %d\n", k);
    for (int i = 0; i < strlen(str); i++)
    {
        nd = 0;
        while (nd == 0)
        {
            r = rand() % k;
            if (a[r].n > 0)
            {
                bass[i] = a[r].c;
                nd++;
                a[r].n--;
            }
        }
    }
    //printf("bass = %s\n", bass);
    strncat(dest, bass, strlen(str));
    //printf("after strncat dest = %s\n", dest);
    strncat(dest, " ", 1);
    free(a);
    return 0;
}

void* threadFunc(void* thread_data)
{
    //printf("process started\n");
    fprintf(logfile, "[%s] Thread started\n", getTime());

    if (wait_mode != 0)
    {
        sleep(wait_mode);
        fprintf(logfile, "[%s] waiting for %d seconds\n", getTime(), wait_mode);

    }

    thread_dataS *data = (thread_dataS *) thread_data;


    //printf("%s\n", data->in_str);
    //sleep(5);
    //printf("woke up\n");
    //printf("---------\n");
    //printf("%s\n", data->in_str);

    //printf("after this lol %s\n", data->in_str);
    char *buffer = malloc(sizeof(char) * strlen(data->in_str));
    if (buffer == NULL)
    {
        fprintf(logfile, "[%s] Error: NULL pointer buffer\n", getTime());
        exit(1);
    }
    int n;
    //printf("after creating buffer\n");
    //strcat(buffer, "new");
    //printf("data->in_str = %s, len = %d\n", data->in_str, strlen(data->in_str));
    strncpy(buffer, data->in_str, strlen(data->in_str));
    //printf("after copying buffer\n");

    char *string_out = malloc(sizeof(char) * strlen(buffer));
    if (string_out == NULL)
    {
        fprintf(logfile, "[%s] Error: NULL pointer string_out\n", getTime());
        exit(1);
    }
    //bzero(word, sizeof(char));
    //printf("after creating string out\n");
    char **dict;
    int wordcount = 0;
    for (int i = 0; i < strlen(buffer); i++)
    {
        if (buffer[i] == ' ')
        {
            wordcount++;
        }
    }
    wordcount++;
    ////printf("%d\n", wordcount);
    ////printf("strlen =  %d\n", strlen(buffer));
    //sleep(5);
    dict = malloc(sizeof(char*) * wordcount);
    if (dict == NULL)
    {
        fprintf(logfile, "[%s] Error: NULL pointer dict\n", getTime());
        exit(1);
    }
    for (int i = 0; i < wordcount; i++)
    {
        dict[i] = malloc(sizeof(char) * 100);
        //printf("%s\n", dict[i]);
        /*if (dict[i] == NULL)
        {
            fprintf(logfile, "[%s] Error: NULL pointer dict[i]\n", getTime());
            exit(1);
        }*/
    }
    //printf("after dict creation\n");
    int wordlen;
    int k;
    int wordcount2 = 0;
    char *al = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	//printf("<< %s\n", buffer);
    for (int i = 0; i < strlen(buffer); i++)
    {
        k = i;
        wordlen = 0;
        ////printf("!!! %c\n", buffer[i]);
        if ((strchr(al, buffer[i]) == NULL) || (buffer[i] == '\n'))
        {
        	break;
        }
        while ((buffer[i] != ' ') && (buffer[i] != '\n') && (i < strlen(buffer)))
        {
            //printf("!!! %c\n", buffer[i]);
            if (i == strlen(buffer))
            {
                break;
            }
            if (strchr(al, buffer[i]) != NULL)
        	{
        	wordlen++;
        	}
            
            i++;
        }
        //printf("== %d\n", wordcount2);
        //dict[wordcount2] = strdup(&buffer[k]);
        //printf(">%s\n", dict[wordcount2]);
        strncpy(dict[wordcount2], &buffer[k], wordlen);
        //printf(">>%s\n", dict[wordcount2]);
        //printf("-- %s\n", dict[wordcount2]);
        wordcount2++;
    }
    //printf("after dict added\n");

    fprintf(logfile, "[%s] Making anagram\n", getTime());

    for (int i = 0; i < wordcount; i++)
    {
        ////printf("%s\n", dict[i]);
        annogramme(dict[i], string_out);
        free(dict[i]);
    }
    //printf("after dict anno\n");
    //printf("->> %s\n", string_out);

    ////printf("%s\n", buffer);
    //sleep(5);
    n = write(data->out_socket, string_out, strlen(buffer));
    if (n == -1)
    {
        fprintf(logfile, "[%s] Error: writing to socket\n", getTime());
        //exit(1);
    }
    else
    {
        fprintf(logfile, "[%s] Writing to client: %s\n", getTime(), string_out);

        success_calls++;
    }

    for (int i = 0; i < wordcount; i++)
    {
        //free(dict[i]);
    }
    free(dict);
    //free(string_out);
    //free(buffer);
    //printf("<<- %d\n", data->out_socket);
    close(data->out_socket);
    //printf("after thread\n");
    //printf("----------------\n");
    free(thread_data);
    pthread_detach(pthread_self());
    pthread_exit(0);
}

int daemon_func(void)
{


    int connfd, n;
    char *str_in;


    while(1)
    {
        ////printf("jk\n");
        str_in = malloc(sizeof(char) * 255);
        if (str_in == NULL)
        {
            fprintf(logfile, "[%s] Error: NULL pointer str_in\n", getTime());
            exit(1);
        }
        connfd = accept(sock, (struct sockaddr*)NULL, NULL);
        if (connfd == -1)
        {
            fprintf(logfile, "[%s] Error: while accepting client\n", getTime());
            exit(1);
        }
        n = read(connfd, str_in, 255);
        all_calls++;
        if (n == -1)
        {
            fprintf(logfile, "[%s] Error: reading from socket\n", getTime());
            continue;
        }
        fprintf(logfile, "[%s] Message accepted: %s\n", getTime(), str_in);

        //printf(" >> %s\n", str_in);
        //printf("connfd = %d\n", connfd);

        int length = 0;
        for (int i = 0; i < strlen(str_in); i++)
        {
            if (str_in[i] == '\n')
            {
                break;
            }
            else
            {
                length++;
            }
        }

        //printf("length until n = %d\n", length);
        pthread_t *new_thread = malloc(sizeof(pthread_t));
        if (new_thread == NULL)
        {
            fprintf(logfile, "[%s] Error: NULL pointer new_thread\n", getTime());
            exit(1);
        }
        thread_dataS *td_in = malloc(sizeof(thread_dataS));
        if (td_in == NULL)
        {
            fprintf(logfile, "[%s] Error: NULL pointer td_in\n", getTime());
            exit(1);
        }
        td_in->in_str = malloc(sizeof(char) * length);
        if (td_in->in_str == NULL)
        {
            fprintf(logfile, "[%s] Error: NULL pointer td_in->in_str\n", getTime());
            exit(1);
        }
        td_in->out_socket = connfd;


        strncpy(td_in->in_str, str_in, length);

        //printf("<<- %d\n", connfd);
        int rr;
        rr = pthread_create(&(new_thread), NULL, threadFunc, td_in);
        if (rr != 0)
        {
            fprintf(logfile, "[%s] Error: creating thread\n", getTime());
            exit(1);
        }
        //printf("was %s", td_in->in_str);
        //printf("len = %d\n", strlen(td_in->in_str));

        //sn//printf(buffer, sizeof(buffer), "new msg\n");
        //n = write(connfd, buffer, strlen(buffer));
        bzero(buffer, 256);
        ////printf("after bzero\n");
        //close(connfd);
        sleep(1);
        //pthread_join(new_thread, NULL);
        //free(new_thread);
        free(str_in);
        //free(td_in);
        //str_in = "";
    }
    close(sock);

    return 0;
}

static void SIG_handler(int signum, siginfo_t *s, void *c) {
    //check(sigaction(SIGINT, &sa, NULL));
    //char * taunt = taunt_arr[ rand() % (sizeof(taunt_arr) / sizeof(taunt_arr[0]))];
    //write(1, taunt, strnlen(taunt, BUF_SIZE));
    //write(1, "\n", 1);
    //printf("\n");
    if (signum == SIGINT)
    {
        fprintf(logfile, "[%s] Signal received: SIGINT\n", getTime());
        fprintf(logfile, "[%s] Ending...\n", getTime());
        if (daemon_mode == 0)
        {
            printf("Signal received: SIGINT\n");
            printf("Ending...\n");
        }
        close(sock);
        exit(0);
    }
    if (signum == SIGQUIT)
    {
        fprintf(logfile, "[%s] Signal received: SIGQUIT\n", getTime());
        fprintf(logfile, "[%s] Ending...\n", getTime());
        if (daemon_mode == 0)
        {
            printf("Signal received: SIGQUIT\n");
            printf("Ending...\n");
        }

        close(sock);
        exit(0);
    }
    if (signum == SIGTERM)
    {
        fprintf(logfile, "[%s] Signal received: SIGTERM\n", getTime());
        fprintf(logfile, "[%s] Ending...\n", getTime());
        if (daemon_mode == 0)
        {
            printf("Signal received: SIGTERM\n");
            printf("Ending...\n");
        }
        close(sock);
        exit(0);
    }
    if (signum == SIGUSR1)
    {
        fprintf(logfile, "[%s] Signal received: SIGUSR1\n", getTime());
        if (daemon_mode == 0)
        {
            printf("Signal received: SIGUSR1\n");
        }

        show_statistics(0);
    }
}

int main(int argc, char* argv[])
{
    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sa.sa_sigaction = SIG_handler;
    sa.sa_flags |= (SA_SIGINFO | SA_RESTART); // mind SA_RESTART here!

    check(sigaction(SIGINT, &sa, NULL));
    check(sigaction(SIGQUIT, &sa, NULL));
    check(sigaction(SIGUSR1, &sa, NULL));
    check(sigaction(SIGTERM, &sa, NULL));

    int rez;
    int option_index = -1;
    while ((rez = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1)
    {
        ////printf("%c\n", rez);
        switch (rez)
        {
            case 'h':
            {
                show_help(0);
                exit(0);
            };
            case 'v':
            {
                show_version(0);
                exit(0);
            };
            case 'w':
            {
                if (optarg)
                {
                    wait_mode = atoi(optarg);
                    if (wait_mode == 0)
                    {
                        printf("Error: wrong argument in -w\n");
                        exit(1);
                    }
                }
                else
                {
                    printf("Error: option -w required argument\n");
                    exit(1);
                }
                wait_mode_flag++;
                break;
            };
            case 'd':
            {
                daemon_mode++;
                break;
            };

            case 'l':
            {
                if (optarg)
                {
                    log_file_addr = optarg;
                }
                else
                {
                    printf("Error: option -l required argument\n");
                    exit(1);
                }
                log_file_flag++;
                break;
            };

            case 'a':
            {
                if (optarg)
                {
                    ipv4_addr_str = optarg;
                }
                else
                {
                    printf("Error: option -a required argument\n");
                    exit(1);
                }
                ip_flag++;
                break;
            };

            case 'p':
            {
                if (optarg)
                {
                    port_str = optarg;
                }
                else
                {
                    printf("Error: option -p required argument\n");
                    exit(1);
                }
                port_flag++;
                break;
            }
            case ':':
            {
                printf("Error: missing argument\n");
                exit(1);
            }
            case '?':
            {
                printf("Error: unknown option\n");
                exit(1);
            }
            default:
            {
                break;
            };
        };
    };

    //printf("1 %s\n", log_file_addr);
    if (log_file_flag == 0)
    {
        log_file_addr = getenv("L2LOGFILE");
        if (log_file_addr == NULL)
        {
            log_file_addr = "log.txt";
            //TODO log file here
        }
    }

    //printf("2 %s\n", log_file_addr);
    logfile = fopen(log_file_addr, "w");
    fprintf(logfile, "[%s] Started\n", getTime());
    //printf("af\n");
    if (ip_flag == 0)
    {
        ipv4_addr_str = getenv("L2ADDR");
    }
    if (port_flag == 0)
    {
        port_str = getenv("L2PORT");
    }
    if (ipv4_addr_str == NULL)
    {
    	printf("IP address not found\n");
    	exit(1);
    }
    if (port_str == NULL)
    {
    	printf("Port not found\n");
    	exit(1);
    }

    if (wait_mode_flag == 0)
    {
        if (getenv("L2WAIT") == NULL)
        {
            wait_mode = 0;
        }
        else
        {
            wait_mode = atoi(getenv("L2WAIT"));
            if (wait_mode == 0)
            {
                printf("Error: wrong argument in L2WAIT\n");
                fprintf(logfile, "[%s] Error: wrong argument in L2WAIT\nExit...\n", getTime());
                exit(1);
            }
        }
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Error wile creating socket\n");
        fprintf(logfile, "[%s] Error wile creating socket\nExit...\n", getTime());
        exit(1);
    }

    int port = atoi(port_str);
    if (port == 0)
    {
        printf("Error: wrong format of port number\n");
        fprintf(logfile, "[%s] Error: wrong format of port number\nExit...\n", getTime());
        close(sock);
        exit(1);
    }

    server = gethostbyname(ipv4_addr_str);
    if (server == NULL)
    {
        printf("Error: no such host\n");
        fprintf(logfile, "[%s] Error: no such host\nExit...\n", getTime());
        close(sock);
        exit(1);
    }
    int res;

    res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    if (res != 0)
    {
        printf("Error: wrong sock options\n");
        fprintf(logfile, "[%s] Error: wrong sock options\nExit...\n", getTime());
        exit(1);
    }

    // SIG




    //memset(&serv_addr, '0', sizeof(serv_addr));
    //memset(buffer, '0', sizeof(buffer));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipv4_addr_str);

    //bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    //server->h_length);
    serv_addr.sin_port = htons(port);

    //printf("%s\n", ipv4_addr_str);
    //printf("%s\n", (char *)&serv_addr.sin_addr.s_addr);

    int x;
    x = bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (x != 0)
    {
        printf("Error: while binding\n");
        fprintf(logfile, "[%s] Error: while binding\nExit...\n", getTime());
        exit(1);
    }
    //printf("%d\n", x);

    listen(sock, 10);

    pid_t parpid;
    if (daemon_mode == 0)
    {
        daemon_func();
        fprintf(logfile, "[%s] Starting in console mode\n", getTime());
    }
    else
    {
        if ((parpid = fork()) < 0)
        {
            printf("Error: creating fork with daemon\n");
            fprintf(logfile, "[%s] Error: creating fork with daemon\nExit...\n", getTime());
            exit(1);
        }
        else
        {
            if (parpid != 0) {
                exit(0);
            }
            setsid();
            fprintf(logfile, "[%s] Starting in daemon mode\n", getTime());
            daemon_func();
            chdir("/");
            close(stdin);
            close(stdout);
            close(stderr);
        }
    }

    //daemon HERE


}
