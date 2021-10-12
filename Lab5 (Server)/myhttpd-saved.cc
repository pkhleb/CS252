const char * usage =
"                                                               \n"
"myhttp:                                                \n"
"                                                               \n"
"Simple web server that can send websites to a browser.       \n"
"                                            \n"
"                                                               \n"
"To use it, in one window type:                                  \n"
"                                                               \n"
"   ./myhttp [-f|-t|-p] <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"Open your browser and go to the address                        \n"
"                                                               \n"
"   data.cs.purdue.edu:<port>                                   \n"
"                                                               \n"
"<port> is the port number you used when you run   \n"
"myhttp. If not port is given, defaults to 5555         \n"
"                                                               \n"
"                                                               \n";
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include <dlfcn.h>
#include <chrono>
#include <arpa/inet.h>

#define DEFAULTPORT 5555
#define NOCONC -1
#define FCONC 0
#define TCONC 1
#define PCONC 2

#define GIF "image/gif"
#define PNG "image/png"
#define JPG "image/jpeg"
#define HTML "text/html"
#define SVG "image/svg+xml"
#define XBM "image/xbm"

#define LOGFILE "http-root-dir/htdocs/log"

int accepted=0;

class Timer
{
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
        using second_t = std::chrono::duration<double, std::ratio<1>>;
        using clock_t = std::chrono::high_resolution_clock;
    public:
        Timer() : m_beg(std::chrono::high_resolution_clock::now())
        {
        }
        void reset() {
            m_beg = std::chrono::high_resolution_clock::now();
        }
        double elapsed() const
        {
            return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
        }
};

int QueueLength = 5;

int requests = 0;

Timer t;

pthread_mutex_t mlog;

pthread_mutex_t maxPT;
double maxProcessTime = 0;
pthread_mutex_t minPT;
double minProcessTime = 10000000;
struct thread_struct {
    int port;
    int serverSocket;
};

struct PTRdata {
    int socket;
    struct in_addr IP;
};

double processTimeRequest(void* context);
std::string filePathConvert(std::string s);
void sendFileToSocket(int fd, std::string filepath, std::string doctype);
void send404(int fd);
void forkversion(int port, int serverSocket);
void iterversion(int port, int serverSocket);
void threadversion(int port, int serverSocket);

extern "C" void killZombie(int sig) {
    (void) sig;
    pid_t pid;
    bool killedzombie = false;
    while ((pid = waitpid(-1,NULL, WNOHANG)) > 0) {
        
    }
}


int concurancy = NOCONC;
void poolSlave(int socket);

int
main(int argc, char **argv)
{
    if (pthread_mutex_init(&maxPT, NULL) != 0) {
        perror("mutex maxPT");
        exit(1);
    }
    if (pthread_mutex_init(&minPT, NULL) != 0) {
        perror("mutex minPT");
        exit(1);
    }
    if (pthread_mutex_init(&mlog, NULL) != 0) {
        perror("mutex mlog");
        exit(1);
    }
    int port = DEFAULTPORT;
    int portarg = 1;
    // Add your HTTP implementation here
    if (argc != 1) {
        if (argc > 3) {
            fprintf(stderr, "%s", usage);
            exit(-1);
        }
        if (argv[1][0] == '-') {
            portarg++;
            std::string ctype = argv[1];
            if (ctype == "-f") concurancy = FCONC;
            else if (ctype == "-t") concurancy = TCONC;
            else if (ctype == "-p") concurancy = PCONC;
            else {
                fprintf(stderr, "%s", usage);
                exit(-1);
            }
        }

        // Get the port from the arguments
        if (argc == portarg+1) {
            port = atoi(argv[portarg]);
        }
    }
    //printf("port: %d\nconcurancy type: %d\n", port, concurancy);

    struct sigaction zombiesa;
    zombiesa.sa_handler = killZombie;
    sigemptyset(&zombiesa.sa_mask);
    zombiesa.sa_flags = SA_RESTART;

    struct sockaddr_in serverIPAddress;
    memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
    serverIPAddress.sin_family = AF_INET;
    serverIPAddress.sin_addr.s_addr = INADDR_ANY;
    serverIPAddress.sin_port = htons((u_short) port);

    // Allocate a socket
    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if ( serverSocket < 0 ) {
        perror("socket");
        exit( -1 );
    }

    // Set socket options to reuse port. Otherwise we will
    // have to wait about 2 minutes before reusing the same port number
    int optval = 1;
    int err = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &optval, sizeof(int));

    // Bind the socket to the IP address and port
    int error = bind(serverSocket, (struct sockaddr *) &serverIPAddress,
                     sizeof(serverIPAddress));
    if (error) {
        perror("bind");
        exit(-1);
    }
    
    // Put socket in listening mode and set the
    // size of the queue of unprocessed connections
    error = listen(serverSocket, QueueLength);
    if (error) {
        perror("bind");
        exit(-1);
    }
    if (sigaction(SIGCHLD, &zombiesa, NULL)) {
        perror("zombie sigaction");
        exit(2);
    }
    if (concurancy == NOCONC) {
        iterversion(port, serverSocket);
    }
    if (concurancy == FCONC) {
        forkversion(port, serverSocket);
    }
    else if (concurancy == TCONC) {
        struct thread_struct data;
        data.port = port;
        data.serverSocket = serverSocket;
        //printf("serverSocket: %d\n", serverSocket);
        void* dataptr = &data;
        threadversion(port, serverSocket);
    }
    else {

        pthread_t tid[5];
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
        for (int i = 0; i < 5; i++) {
            pthread_create(&tid[i], &attr, (void *(*)(void *)) poolSlave, (void *) (intptr_t) serverSocket);
        }
        pthread_join(tid[0],NULL);
    }
}

void poolSlave(int socket) {
    while (1) {
        struct sockaddr_in clientIPAddress;
        int alen = sizeof(clientIPAddress);
        int clientSocket = accept( (int) socket, (struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
        if (clientSocket < 0) {
            perror("accept");
            exit(-1);
        }
        requests++;
        //processTimeRequest(clientSocket);
        struct PTRdata data;
        data.socket = clientSocket;
        data.IP = clientIPAddress.sin_addr;
        double processTime = processTimeRequest(&data);
        pthread_mutex_lock(&maxPT);
        if (processTime > maxProcessTime) {
            maxProcessTime = processTime;
        }
        pthread_mutex_unlock(&maxPT);
        pthread_mutex_lock(&minPT);
        if (processTime < minProcessTime) {
            minProcessTime = processTime;
        }
        pthread_mutex_unlock(&minPT);
        close(clientSocket);
    }
}

void iterversion(int port, int serverSocket) {

    while (1) {
        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int clientSocket = accept( serverSocket, (struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
        if (clientSocket < 0) {
            perror("accept");
            exit(-1);
        }
        requests++;
        struct PTRdata data;
        data.socket = clientSocket;
        data.IP = clientIPAddress.sin_addr;
        double processTime = processTimeRequest(&data);
        pthread_mutex_lock(&maxPT);
        if (processTime > maxProcessTime) {
            maxProcessTime = processTime;
        }
        pthread_mutex_unlock(&maxPT);
        pthread_mutex_lock(&minPT);
        if (processTime < minProcessTime) {
            minProcessTime = processTime;
        }
        pthread_mutex_unlock(&minPT);
        close(clientSocket);
    }

}

void forkversion(int port, int serverSocket) {

    while (1) {
        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int clientSocket = accept( serverSocket, (struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
        if (clientSocket < 0) {
            perror("accept");
            exit(-1);
        }
        //int fd[2];
        //pipe(fd);
        pid_t slave = fork();
        requests++;
        if (slave == 0) {
          //close(fd[0]);
          struct PTRdata data;
          data.socket = clientSocket;
          data.IP = clientIPAddress.sin_addr;
          double processTime = processTimeRequest(&data);
          std::string strTime = std::to_string(processTime);
          //write(fd[1], strTime.c_str(), strTime.size());
          //close(fd[1]);
          close(clientSocket);
          exit(EXIT_SUCCESS);
        }
        /*double processTime;
        char buf[100];
        //close(fd[1]);
        //read(fd[0], buf, 100);
        //close(fd[0]);
        std::string strTime = buf;
        //processTime = std::stod(strTime);
        if (processTime > maxProcessTime) {
            maxProcessTime = processTime;
        }
        if (processTime < minProcessTime) {
            minProcessTime = processTime;
        }*/

        close(clientSocket);
    }
}

void threadversion(int port, int serverSocket) {
    while (1) {
        // Accept incoming connections
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int clientSocket = accept( serverSocket, (struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
        printf("accepted: %d\n", ++accepted);
        requests++;
        if (clientSocket < 0) {
            perror("accept");
            exit(-1);
        }
        pthread_t t1;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        //printf("before pthread_create\n");
        struct PTRdata data;
        data.socket = clientSocket;
        data.IP = clientIPAddress.sin_addr;
        pthread_create(&t1, &attr, (void * (*)(void *)) processTimeRequest, (void*) (intptr_t) clientSocket);
    }
}

std::string getRequest(int fd) {
    const int MaxData = 4096;
    char data[MaxData+1];
    int dataLength = 0;
    int n;

    char buf[1];
    unsigned char newChar;
    unsigned char lastChar = 0;
    unsigned char thirdChar = 0;
    unsigned char fourthChar = 0;

    while ( dataLength < MaxData && ( n = read( fd, buf, sizeof(buf) ) ) > 0 ) {
        newChar = buf[0];
        if (fourthChar == '\015' && thirdChar == '\012' && 
            lastChar == '\015' && newChar == '\012') {
            // Discard previous <CR> from data
            dataLength = dataLength - 3;
            break;
        }

        data[ dataLength ] = newChar;
        dataLength++;
        fourthChar = thirdChar;
        thirdChar = lastChar;
        lastChar = newChar;
    }
    
    // Add null character at the end of the string
    data[dataLength]  = 0;
    std::string strdata = data;

    return strdata;
}

bool isAuth(std::string clientstring, int fd) {
    if (clientstring.find("Authorization: Basic cGtobGViOnpuNEt2MGw=") == -1) {
        //printf("No Authorization\n");
        std::string authreq = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"pkhleb-cs252\"\r\n";
        write(fd, authreq.c_str(), authreq.size());
        if (concurancy == TCONC) {
            close(fd);
        }
        return false;
    }
    return true;
}

std::string getDoctype(std::string docreq) {
    std::string doctype;
    if (docreq.find(".gif") != -1) {
        doctype = GIF;
    }
    else if (docreq.find(".png") != -1) {
        doctype = PNG;
    }
    else if (docreq.find(".jpg") != -1) {
        doctype = JPG;
    }
    else if (docreq.find(".html") != -1) {
        doctype = HTML;
    }
    else if (docreq.find(".svg") != -1) {
        doctype = SVG;
    }
    else if (docreq.find(".xbm") != -1) {
        doctype = XBM;
    }
    else if (docreq.find(".ico") != -1) {
        doctype = "image/x-icon";
    }
    else {
        doctype = HTML;
    }
    return doctype;
}

class filestats {
    public:
        std::string _name;
        long int _lastModified;
        int _size;
        std::string _description;
        std::string _filetype;
        filestats(std::string path, struct stat stats, std::string desc) {
            _name = path.substr(path.find_last_of("/")+1);
            if (_name.find("?") != -1) {
                _name = _name.substr(0,_name.find("?"));
            }
            _lastModified = stats.st_mtime;
            struct tm lt;
            localtime_r(&_lastModified,&lt);
            char timbuf[80];
            strftime(timbuf, sizeof(timbuf), "%c", &lt);
            _size = stats.st_size;
            _description = "";
            if (stats.st_mode & S_IFDIR) {
                //Is Directory
                _filetype = "dir";
                _size = -1;
            } else if (stats.st_mode & S_IFREG) {
                if (_name.find(".gif") != -1) {
                    _filetype = "gif";
                }
                else {
                    _filetype = "unk";
                }
            }
        }
};

void printDirectory(int fd, std::string path, std::string doctype, std::string flags) {
    //printf("flags: %s\n", flags.c_str());
    
    std::string prefix = "HTTP/1.1 200 Document follows\r\nServer: CS252 lab5\r\n";
    std::string contenttype = "Content-type: " + doctype + "\r\n";
    write(fd, prefix.c_str(), prefix.size());
    //printf("%s", prefix.c_str());
    write(fd, contenttype.c_str(), contenttype.size());
    //printf("%s", contenttype.c_str());
    write(fd, "\r\n", 2);
    //printf("\r\n");
    
    std::string line = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n";
    write(fd, line.c_str(), line.size());
    line = "<html>\r\n";
    write(fd, line.c_str(), line.size());
    line = " <head>\r\n";
    write(fd, line.c_str(), line.size());
    line = "  <title>Index of " + path + "</title>\r\n";
    write(fd, line.c_str(), line.size());
    line = " </head>\r\n";
    write(fd, line.c_str(), line.size());
    line = " <body>\r\n";
    write(fd, line.c_str(), line.size());
    line = "<h1>Index of " + path + "</h1>\r\n";
    write(fd, line.c_str(), line.size());
    line = "  <table>\r\n";
    write(fd, line.c_str(), line.size());
    line = "   <tr><th valign=\"top\"><img src=\"/icons/blank.gif\" alt=\"[ICO]\"></th><th><a href=\"?";
    if (flags == "C=N;O=A" || flags == "") {
        line = line + "C=N;O=D";
    } else {
        line = line + "C=N;O=A";
    } 
    line = line + "\">Name</a></th><th><a href=\"?";
    if (flags == "C=M;O=A") {
        line = line + "C=M;O=D";
    } else {
        line = line + "C=M;O=A";
    }
    line = line + "\">Last modified</a></th><th><a href=\"?";
    
    if (flags == "C=S;O=A") {
        line = line + "C=S;O=D";
    } else {
        line = line + "C=S;O=A";
    }
    line = line + "\">Size</a></th><th><a href=\"?C=D;O=A\">Description</a></th></tr>\r\n";
    write(fd, line.c_str(), line.size());
    line = "   <tr><th colspan=\"5\"><hr></th></tr>\r\n";
    write(fd, line.c_str(), line.size());

    std::vector<filestats> files;
    DIR* dfd;
    struct dirent *epdf;
    struct stat s;

    dfd = opendir(path.c_str());
    if (dfd != NULL) {
        while (epdf = readdir(dfd)) {
                std::string statstring = path + epdf->d_name;
                if( stat(statstring.c_str(),&s) < 0) {
                    perror("stat");
                }
                filestats newfile(epdf->d_name,s,"");
                files.push_back(newfile);
        }
    }
    if (flags == "C=N;O=A") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._name < rhs._name;
        });
    }
    else if (flags == "C=N;O=D") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._name > rhs._name;
        });
    }
    else if (flags == "C=M;O=A") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._lastModified < rhs._lastModified;
        });
    }
    else if (flags == "C=M;O=D") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._lastModified > rhs._lastModified;
        });
    }
    else if (flags == "C=S;O=A") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._size < rhs._size;
        });
    }
    else if (flags == "C=S;O=D") {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._size > rhs._size;
        });
    } else {
        sort(files.begin(), files.end(), [](const filestats& lhs, const filestats& rhs) {
                return lhs._name < rhs._name;
        });
    }

    line = "<tr><td valign=\"top\">";
    line = line + "<img src=\"/icons/back.gif\" alt=\"[   ]\"></td><td>";
    std::string uppath = path.substr(path.find_last_of("/")+1) + "../";
    line = line + "<a href=\"" + uppath + "\">" + ".." + "</a></td>";
    line = line + "<td align=\"right\">";
    line = line + " </td><td align=\"right\"> - </td><td>&nbsp;</td></tr>\r\n";
    write(fd, line.c_str(), line.size());
    for (auto it = files.begin(); it != files.end(); it++) {
        if (it->_name != "." && it->_name != "..") {
            line = "<tr><td valign=\"top\">";
            if (it->_filetype == "dir") {
                line = line + "<img src=\"/icons/folder.gif\" alt=\"[   ]\"></td><td>";
            }
            else if (it->_filetype == "gif") {
                line = line + "<img src=\"/icons/image.gif\" alt=\"[   ]\"></td><td>";
            }
            else {
                line = line + "<img src=\"/icons/unknown.gif\" alt=\"[   ]\"></td><td>";
            }
            if (it->_name == "..") {
                std::string uppath = path.substr(path.find_last_of("/")+1) + "../";
                line = line + "<a href=\"" + uppath + "\">" + it->_name + "</a></td>";
            } else {
                line = line + "<a href=\"" + it->_name + "\">" + it->_name + "</a></td>";
            }
            line = line + "<td align=\"right\">";
            struct tm lt;
            localtime_r(&(it->_lastModified),&lt);
            char timbuf[80];
            strftime(timbuf, sizeof(timbuf), "%c", &lt);
            std::string timebuf = timbuf;
            std::string fsize = std::to_string(it->_size);
            line = line + timebuf + " </td><td align=\"right\">";
            if (it->_filetype != "dir") {
                line = line + fsize + " </td><td>&nbsp;</td></tr>\r\n";
            } else {
                line = line + " - </td><td>&nbsp;</td></tr>\r\n";
            }
            write(fd, line.c_str(), line.size());
        }
    }

    line = "<tr><th colspan=\"5\"><hr></th></tr>\r\n";
    write(fd, line.c_str(), line.size());
    line = "</table>\r\n";
    write(fd, line.c_str(), line.size());
    line = "</body></html>\r\n";
    write(fd, line.c_str(), line.size());
}

void redirect(int fd, std::string path, std::string flags) {
    //printf("path: %s\n", path.c_str());
    if (path.find("..") != -1) exit(0);
    path = path.substr(path.find_last_of("/")+1);
    std::string line = "HTTP/1.1 302 Found\r\nLocation: " + path + "/" + flags + "\r\n\r\n";
    write(fd, line.c_str(), line.size());
}

void CGIBINtoSocket(int fd, std::string path, std::string flags) {
    int ret = fork();
    if (ret == 0) {
        // Child
        int tempfd = fd;
        dup2(fd,1);
        close(fd);
        setenv("REQUEST_METHOD","GET",1);
        setenv("QUERY_STRING",flags.c_str(),1);
        std::string line;
        char* argv[1];
        argv[0] = (char*) path.c_str();
        execl(path.c_str(),path.c_str(),flags.c_str(),NULL);
        perror("execl");
        exit(1);

    }
    else {
        //parent
    }
}

typedef void (*httprunfunc)(int ssock, const char* querystring);

std::vector<std::string> modules;
std::vector<void*> handles;

void loadModule(int fd, std::string path, std::string flags) {
    std::string prefix = "HTTP/1.1 200 Document follows\r\nServer: CS252 lab5\r\n";
    write(fd, prefix.c_str(), prefix.size());
    httprunfunc httprun;
    void* handle;
    auto it = find(modules.begin(), modules.end(), path);
    if (it != modules.end()) {
        int index = it-modules.begin();
        handle = handles[index];
    } else {
        handle = dlopen(path.c_str(), RTLD_LAZY);
        if (handle == NULL) {
            fprintf(stderr, "library %s not found\n", path.c_str());
            perror("dlopen");
            exit(1);
        }
        
        modules.push_back(path);
        handles.push_back(handle);
    }

    httprun = (httprunfunc) dlsym(handle, "httprun");
    if (httprun == NULL) {
        perror("dlsym: httprun not found:");
        exit(1);
    }
    //printf("before httprun\n");
    httprun(fd, flags.c_str());
    //printf("after httprun\n");
  
}

void sendStatFile(int fd) {
    std::string line = "HTTP/1.1 200 Document follows\r\nServer: CS252 lab5\r\n";
    write(fd, line.c_str(), line.size());
    line = "Content-type: text/plain\r\n\r\n";
    write(fd, line.c_str(), line.size());
    line = "Server statistics:\r\n";
    write(fd, line.c_str(), line.size());
    line = "Author: Peter Khlebnikov (pkhlebni)\r\n";
    write(fd, line.c_str(), line.size());

    std::string elapsedtime = std::to_string(t.elapsed());
    line = "Server Uptime: " + elapsedtime + "\r\n";
    write(fd, line.c_str(), line.size());

    std::string strrequests = std::to_string(requests);
    line = "Number of Requests: " + strrequests + "\r\n";
    write(fd, line.c_str(), line.size());

    pthread_mutex_lock(&minPT);
    std::string minservicetime = std::to_string(minProcessTime);
    pthread_mutex_unlock(&minPT);
    pthread_mutex_lock(&maxPT);
    std::string maxservicetime = std::to_string(maxProcessTime);
    pthread_mutex_unlock(&maxPT);

    line = "Minimum service time: " + minservicetime + "\r\n";
    write(fd, line.c_str(), line.size());

    line = "Maximum service time: " + maxservicetime + "\r\n";
    write(fd, line.c_str(), line.size());

    line = "\r\n";
    write(fd, line.c_str(), line.size());
}

void sendLogFile(int fd) {

}

int ptrcount = 0;

double processTimeRequest(void* context) {
    printf("processTimeRequest called %d times\n", ++ptrcount);
    Timer processTime;
    int fd;
    struct in_addr IP;
    if (concurancy != TCONC) {
        struct PTRdata *data = (struct PTRdata*) context;
        fd = data->socket;
        IP = data->IP;
    } else {
        fd = reinterpret_cast<long>(context);
    }
    //printf("in PTR\n");

    // Get Document Requested
    std::string clientstring = getRequest(fd);

    //printf("GET request:\n%s\n", clientstring.c_str());

    if (!isAuth(clientstring,fd)) return processTime.elapsed();

    std::string docreq = clientstring.substr(clientstring.find(" ")+1);
    //printf("docreq: %s\n", docreq.c_str());
    docreq = docreq.substr(0,docreq.find(" "));
    printf("docreq:%s waiting for file mutex\n", docreq.c_str());

    if (concurancy != TCONC) {
    pthread_mutex_lock(&mlog);
    std::ofstream file;
    file.open(LOGFILE,std::ios::app);
    std::string tologfile = "<p>";
    tologfile = tologfile + inet_ntoa(IP);
    tologfile = tologfile + "\t" + docreq + "</p>";
    file << tologfile;
    file.close();
    pthread_mutex_unlock(&mlog);
    }

    printf("docreq:%s opened file mutex\n", docreq.c_str());

    std::string doctype = getDoctype(docreq);

    // Convert to file path
    std::string path = filePathConvert(docreq);
    //printf("path requested: %s\n", path.c_str());

    std::string flags = "";
    if (docreq.find("?") != -1) {
        flags = docreq.substr(docreq.find("?")+1);
        path = path.substr(0,path.find("?"));
    }

    // Check if the file path is a directory
    struct stat s;
    if (stat(path.c_str(),&s) == 0) {
        if (s.st_mode & S_IFDIR) {
            // it's a directory
            if (path.back() != '/') {
                redirect(fd, path, flags);
                return processTime.elapsed();
            }
            printDirectory(fd, path, doctype, flags);
        } 
        else if (s.st_mode & S_IFREG) {
            // it's a file
            // Write to the client
            if (path.find(".so") != -1) {
                loadModule(fd, path, flags);
            }
            else if (path.find("cgi-bin") != -1) {
                std::string line = "HTTP/1.1 200 Document follows\r\n";
                write(fd, line.c_str(), line.size());
                line = "Server: CS 252 lab5\r\n";
                write(fd, line.c_str(), line.size());
                int tempfd = fd;
                CGIBINtoSocket(fd, path, flags);
                dup2(fd, tempfd);
                close(fd); 
                
            }
            else {
                sendFileToSocket(fd, path, doctype);
            }
        } 
        else {
            // something else
        }
    }
    else if (docreq == "/stat") {
        //printf("sending stat\n");
        sendStatFile(fd);
    }
    else {
        perror("stat");
    }



    //printf("delivered: %s\n", docreq.c_str());
    if (concurancy == TCONC) {
        //printf("unlocking mutexes\n");
        printf("docreq:%s waiting on maxPT mutex\n", docreq.c_str());
        pthread_mutex_lock(&maxPT);
        double processTimed = processTime.elapsed();
        if (processTimed > maxProcessTime) {
            maxProcessTime = processTimed;
        }
        pthread_mutex_unlock(&maxPT);
        printf("docreq:%s unlocked maxPT mutex\n", docreq.c_str());
        printf("docreq:%s waiting on minPT mutex\n", docreq.c_str());
        pthread_mutex_lock(&minPT);
        if (processTimed < minProcessTime) {
            minProcessTime = processTimed;
        }
        pthread_mutex_unlock(&minPT);
        printf("docreq:%s unlocked minPT mutex\n", docreq.c_str());
        //printf("done with processtime\n");
    }
    if (concurancy == TCONC || concurancy == PCONC) {
        printf("closing file descriptor for: %s\n", docreq.c_str());
        close(fd);
    }
    return processTime.elapsed();
}

std::string filePathConvert(std::string s) {
    //printf("original file request: %s\n", s.c_str());
    if (s.find("/../..") != -1) {
        return "";
    }
    else if (s.find("/icons") == 0) {
        std::string root = "http-root-dir";
        return root+s;
    }
    else if (s == "/") {
        return "http-root-dir/htdocs/index.html";
    } 
    else if (s.find("/cgi-bin") == 0) {
        std::string root = "http-root-dir";
        return root+s;
    }
    else if (s[0] == '/') {
        std::string root = "http-root-dir/htdocs";
        return root+s;
    }
    return "";
}

void sendFileToSocket(int fd, std::string filepath, std::string doctype) {
    if (filepath == "") {
        send404(fd);
        return;
    }

    std::string prefix = "HTTP/1.1 200 Document follows\r\nServer: CS252 lab5\r\n";
    std::string contenttype = "Content-type: " + doctype + "\r\n";

    int count;
    //printf("filepath: %s\n", filepath.c_str());
    FILE *file;
    if (!(file = fopen(filepath.c_str(), "rb"))){
        send404(fd);
        return;
    }
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    //printf("filesize: %d\n", size);
    fseek(file, 0L, SEEK_SET);

    char * buffer = (char*) malloc((size+1)*sizeof(char*));

    write(fd, prefix.c_str(), prefix.size());
    //printf("%s", prefix.c_str());
    write(fd, contenttype.c_str(), contenttype.size());
    //printf("%s", contenttype.c_str());
    write(fd, "\r\n", 2);
    //printf("\r\n");
    while (count = fread(buffer, size, 1, file)) {
        //printf("count: %d\n", count);
        if (write(fd, buffer, size) != count) {
            perror("write");
        }
        //printf("%s", buffer);
    }
    //printf("Done sending file\n");
    fclose(file);
    free(buffer);
}

void send404(int fd) {
    const char* notFound = "File not Found";
    const char* protocol = "HTTP/1.1 404 File Not Found\r\n";
    const char* server = "Server: CS 252 lab5\r\n";
    const char* contenttype = "Content-type: text/plain\r\n";
    write(fd, protocol, strlen(protocol));
    write(fd, server, strlen(server));
    write(fd, contenttype, strlen(contenttype));
    write(fd, "\r\n", 2);
    write(fd, notFound, strlen(notFound));
}
