#include <stdlib.h>
#include <stdio.h>
#include <pty.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MAX_CLIENTS 256

struct client
{
    unsigned id;
    FILE *f;
    pthread_t thread;
};

struct client clients[MAX_CLIENTS];

static void failed(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "usage: multipty count\n");
    fprintf(stderr, "count - number of pty clients, less then %d\n", MAX_CLIENTS);
    exit(EXIT_FAILURE);
}

static void *handle_client(void *arg)
{
    struct client *client = arg;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, client->f)) != -1)
    {
        printf("%d> ", client->id);
        fwrite(line, nread, 1, stdout);
    }

    free(line);
    return NULL;
}

int main(int argc, char *argv[])
{
    int count;
    int aserver;
    int aclient;
    char client_name[1024] = {'\0'};
    char client_opt[sizeof(client_name) + 80];
    char client_title[256];
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char buf[1024];

    if (argc <= 1)
        usage();

    count = atoi(argv[1]);
    if (count <= 0)
        usage();

    for (int client_index = 0; client_index < count; client_index++)
    {
        struct client *client = &clients[client_index];
        client->id = client_index;

        if (openpty(&aserver, &aclient, client_name, 0, 0))
            failed("openpty");

        if (client_name[sizeof(client_name) - 1])
            failed("Too long client name");

        sprintf(client_opt, "-S%s/%d", client_name, aserver);
        sprintf(client_title, "Client - %d", client_index);

        if (!fork())
        {
            execlp("xterm", "xterm", client_opt, "-title", client_title, (char *)0);
            _exit(0);
        }

        client->f = fdopen(aclient, "r+");
        if (client->f == 0)
            failed("fdopen");

        pthread_create(&client->thread, NULL, handle_client, client);
    }

/*
    while ((nread = getline(&line, &len, stdin)) != -1)
    {
        int id;
        if (sscanf(line, "%d> %s", &id, buf) == 2 &&
            id < count) {

            fwrite(buf, strlen(buf), 1, clients[id].f);
        }
    }
*/

    for (int client_index = 0; client_index < count; client_index++)
    {
        pthread_join(clients[client_index].thread, NULL);
    }

    return EXIT_SUCCESS;
}
