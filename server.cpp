#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t get_sig = 0;

void handler(int sig)
{
    if (sig == SIGHUP)
    {
    	printf("Received SIGHUP signal\n");
        get_sig = 1;
    }
    else
    {
    	printf("Error: handler failed");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_FAILURE);
}

int main()
{
    //установка обработчика сигнала
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
    	printf("Error: SIGHUP failed");
        exit(EXIT_FAILURE);
    }
    
    //создание сокета для приёма 
    int message_sock = -1, client_sock;
    char buffer[1024];
    
    //создание сокета для сервера
    int server_sock, port = 8080;
    struct sockaddr_in addr;
    socklen_t size_addr = sizeof(addr);
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Error:  socket failed\n");
        exit(EXIT_FAILURE);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
    	perror("Error: bind failed\n");
    	exit(EXIT_FAILURE);
    }
    
    if (listen(server_sock, 1) < 0)
    {
    	perror("Error: listen failed\n");
    	exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", port);
    
    fd_set read_fds;		//инициализация файлового дескриптора			
    FD_ZERO(&read_fds);  				
    
    sigset_t blocked_mask, mask;
    sigemptyset(&blocked_mask);
    sigemptyset(&mask);
    sigaddset(&blocked_mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blocked_mask, &mask);

    while(true)
    {
    	if (get_sig == 1)
    	{
	        if (message_sock != -1)
	        {
                close(message_sock);
	        
	            message_sock = -1;
                get_sig = 0;
	        }
    	}
    	
    	//добавляем server_socket в группу
    	FD_SET(server_sock, &read_fds);
    	
	    struct timespec tv;	// ожидание данных при вызове select
        tv.tv_sec = 1;
        tv.tv_nsec = 0;
    	
    	int result = pselect(server_sock + 1, &read_fds, NULL, NULL, &tv, &mask);
    	
        if (result == -1)
        {
            perror("Error in pselect.\n");
        }
    	else 
    	{
    	    if (FD_ISSET(server_sock, &read_fds))
    	    {
                client_sock = accept(server_sock, (struct sockaddr *)&addr, &size_addr);
                if (client_sock < 0)
    	        {
    	            perror("Error: accept failed\n");
    	        }
    	        printf("Connection accepted\n");
    	        
    	        
    	        if (message_sock == -1)
    	            message_sock = client_sock;
    	        else
    	            close(client_sock);
    	        
    	        size_t bytes_rec = recv(message_sock, buffer, sizeof(buffer), 0);
    	        
    	        if (bytes_rec > 0)
    	        {
    	            buffer[bytes_rec] = '\0';
    	            printf("Received %zu bytes.\n", bytes_rec);
    	        }
    	        else if (bytes_rec == 0)
    	        {
    	            printf("Received 0 bytes.\n");
    	            close(message_sock);
    	            message_sock = -1;
    	        }
    	        else
    	            perror("Error: receiving data\n");
    	    }
    	}
    }
    
    return 0;
}