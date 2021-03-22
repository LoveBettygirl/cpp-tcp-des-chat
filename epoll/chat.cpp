#include "server.h"
#include "client.h"
#include <regex>
#include <getopt.h>
#include <signal.h>
using namespace std;

volatile bool running = true;

void showVersion()
{
    cout << "v1.0.0" << endl;
    exit(SUCCESS);
}

void showUsage()
{
    cout << "Options:" << endl;
    cout << " -o, --option               Run as server(s) or client(c) mode. (Required)" << endl;
    cout << " -a, --address=IP_ADDRESS   The IP address of the server." << endl;
    cout << " -p, --port=PORT            The port of the server." << endl;
    cout << " -v, --version              Print the version number and exit." << endl;
    cout << " -h, --help                 Print this message and exit." << endl;
    exit(INVALID_OPTION);
}

void stop(int signo)
{
    running = false;
    close(0); // close stdin
}

int main(int argc, char **argv)
{
    int c;
    bool client = false;
    int port = 2000;
    string ip = "0.0.0.0";
    regex r("^(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)\\."
            "(1\\d{2}|2[0-4]\\d|25[0-5]|[1-9]\\d|\\d)$");

    if (argc == 1)
        showUsage();

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"option", required_argument, 0, 'o'},
            {"address", required_argument, 0, 'a'},
            {"port", required_argument, 0, 'p'},
            {"version", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "o:a:p:vh",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'o':
            if (strcmp(optarg, "s") == 0)
                client = false;
            else if (strcmp(optarg, "c") == 0)
                client = true;
            else
            {
                cerr << "The option value must be \"s\" or \"c\"." << endl;
                exit(INVALID_OPTION);
            }
            break;

        case 'a':
            ip = string(optarg);
            if (!regex_match(ip, r))
            {
                cerr << "Invaild IP address format." << endl;
                exit(INVALID_OPTION);
            }
            break;

        case 'p':
            port = atoi(optarg);
            if (port < 0 || port > 65535)
            {
                cerr << "The port " << port << " out of range." << endl;
                exit(INVALID_OPTION);
            }
            break;

        case 'v':
            showVersion();
            break;

        case 'h':
        case '?':
            showUsage();
            break;

        default:
            showUsage();
        }
    }

    if (optind < argc)
        showUsage();

    signal(SIGINT, stop); // Register ctrl+c handler

    if (client)
    {
        Client client(ip, port);
        client.start();
        client.doEpoll();
    }
    else
    {
        Server server(ip, port);
        server.start();
        server.doEpoll();
    }

    return SUCCESS;
}
