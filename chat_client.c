#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT "9034" // the port client will be connecting to 

#define MAX_DATASIZE 100 // max number of bytes we can get at once 
#define MAX_NAMESIZE 25 //max name size of a user
#define MAX_LIMIT 20 //max limit of customers in a room
#define MAX_ROOMNAME 10 //max size of a roomname
#define MAX_ROOMS 50 //max public rooms
#define MAX_PRIVROOMS 50 //max private rooms


//the thread function
void *receive_handler(void *);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct customer{ //customer struct
	char *_username;
	int id;
	int isAtRoom; // are they at a room or not
};

struct rooms{ //room struct
	int id;
	char _name[MAX_ROOMNAME];
	struct customer *customers[MAX_LIMIT]; //max 20 customers in a room
	struct customer *creator;
 
};

struct privateRoom{ //private room struct
	int id;
	char _name[MAX_ROOMNAME];
	struct customer *customers[MAX_LIMIT];
	char *_password;
	struct customer *creator;
};
void list();
void *createRoom(char *str);
void createPrivateRoom(char *str);
void enter(char *str);

struct customer cus;
struct customer *c;
struct rooms r[MAX_ROOMS];
struct privateRoom pr[MAX_PRIVROOMS];

int rID = 0, prID = 0;//room ids of current rooms that are created


int main(int argc, char *argv[])
{
    char str[MAX_LIMIT];
    char str2[MAX_LIMIT];
	int i = 0;
    int idCus = 0; //current customer's id
	
    char message[MAX_DATASIZE];
    char nickName[MAX_NAMESIZE];
    
    int sockfd;// numbytes
    
    char sBuf[MAX_DATASIZE]; 
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    //inputs from the current user   
    puts("nickname:");
    memset(&nickName, sizeof(nickName), 0);
    memset(&message, sizeof(message), 0); //clean message buffer
    fgets(nickName, MAX_NAMESIZE, stdin);  //catch nickname
 
	cus.id = idCus;
	cus._username = nickName;
	c = &cus;
	idCus++;
    
    
    //create new thread to keep receiving messages:
    pthread_t recv_thread;
    //new_sockfd = malloc(sizeof(int));
    //new_sockfd = sockfd;
    
    if( pthread_create(&recv_thread, NULL, receive_handler, (void*)(intptr_t) sockfd) < 0){   //passing (intptr_t) instead of (void*) sockfd to supress warnings
        perror("could not create thread");
        return 1;
    }    
    puts("Synchronous receive handler assigned");
    
    //send message to server:
    puts("Connected\n");
    puts("[Type '-exit' to exit]");

    for(;;){
		char temp[6];
		memset(&temp, sizeof(temp), 0);
        memset(&sBuf, sizeof(sBuf), 0); //clean sendBuffer
        fgets(sBuf, 100, stdin); //gets(message);

		if(sBuf[0] == '-' && sBuf[1] == 'e' && sBuf[2] == 'x' && sBuf[3] == 'i' && sBuf[4] == 't')
		    return 1;
		
	
	    int count = 0;
	
        while(count < strlen(nickName)){
            message[count] = nickName[count];
            count++;
        }
        count--;
        message[count] = ':';
        count++;

	
	    //prepend
        for(int i = 0; i < strlen(sBuf); i++){
            message[count] = sBuf[i];
            count++;
        }
        message[count] = '\0';
       
	    if(strstr(message, "-list") != NULL){ //if user command is -list
		    list();
	    }
	    else if (strstr(message, "-create") != NULL) { //if user command is -create room_name
		    int length = strlen(message)-strlen(nickName)-1;
		    char *sub = (char*) malloc(6);	
  		    strncpy(sub, message+strlen(nickName)+8, strlen(message));
		    createRoom(sub);
    	}
	    else if(strstr(message, "-pcreate") != NULL){ //if user command is -pcreate room_name
	    	char *sub = (char*) malloc(6);
  	    	strncpy(sub, message+strlen(nickName)+9, strlen(message));
	        createPrivateRoom(sub);
	    }
	    else if(strstr(message, "-enter") != NULL){ //if user command is -enter room_name
	    	char *sub = (char*) malloc(6);
  		    strncpy(sub,message+strlen(nickName)+7, strlen(message));
		    enter(sub);
	    }
	    else if(strstr(message, "-msg") != NULL){ //if user command is -msg message
	    	char *sub = (char*) malloc(6);
  	    	strncpy(sub,message+strlen(nickName)+5, strlen(message));
	    	if(c->isAtRoom == 1){
	    		puts(sub);
	    	}
	    	else //dont let the customer to send messages if they arent in a room
	    		printf("you are not in a room yet \n");
	   }
	    else{
		    printf("enter a valid command\n");
    	}
        //Send some data
        //if(send(sockfd, sBuf , strlen(sBuf) , 0) < 0)
        if(send(sockfd, message, strlen(message), 0) < 0){
            puts("Send failed");
            return 1;
        }
        memset(&sBuf, sizeof(sBuf), 0);
        
        /* receive message from client: 
         * moving this to a thread in order to get synchronous recv()s.
         */
    }
    
  
    pthread_join(recv_thread , NULL);
    close(sockfd);

    return 0;
}

//thread function
void *receive_handler(void *sock_fd){
	int sFd = (intptr_t) sock_fd;
    char buffer[MAX_DATASIZE];
    int nBytes;
    
    for(;;){
	bool theSameRoom = false, theSamePRoom = false;
        if ((nBytes = recv(sFd, buffer, MAX_DATASIZE-1, 0)) == -1){
            perror("recv");
            exit(1);
        }
        else{
		    int k = 0,i=0, l= 0;
		    char nickname[50];
		    char command[100];
		    while(buffer[i] != ':'){ //finding the other user's nickname
		    	nickname[i] = buffer[i];
		    	i++;
		        
		    }
		
		    //finding the user.
		    while (*r[k]._name != '\0'){
			    if(r[k].customers[l]->_username == nickname){
				    theSameRoom = true;
			    }
					
		    	k++;
		    }
		    k = 0, l = 0;
		    //finding the user. 
		    while (*pr[k]._name != '\0'){
		    	if(pr[k].customers[l]->_username == nickname){
		    		theSamePRoom = true;
		    	}
		    			
    			k++;
    		}
    	}
    	/* !!! the user cant see the rooms that other clients have created !!! */
    	if(theSameRoom & !theSamePRoom){ //if they are in the same room, show message   
            	buffer[nBytes] = '\0';
        	    printf("%s", buffer);
	    }
	    else if(!theSameRoom & theSamePRoom){ //if they are in the same private room, show message
            buffer[nBytes] = '\0';
    	    printf("%s", buffer);
    	}
    }
}
void list(){ //list all users and rooms

	printf("list of all the rooms and customers in it:\n");

	int l = 0, k = 0;
	while(*r[l]._name != '\0'){
		printf("room %s ------\n", r[l]._name);
		while(r[l].customers[k] != NULL){
			printf("\t\t customer %d: %s \n", k, r[l].customers[k]->_username);
			k++;
		}
		l++;

	}
}


void *createRoom(char *str){ //create a room

 	if(str != NULL & c->isAtRoom != 1){

		printf("room %s has been created.\n", str);

		r[rID].id = rID;
		strncpy(r[rID]._name, str,strlen(str));
  		r[rID].creator = c; //assign name, creator, id and customers
		rID++;	
		enter(str); //enter room after creating it
	}

	else{
		printf("enter a room name. the valid syntax is: -create room_name\n");
	}
    	
}
void createPrivateRoom(char *str){ //create private room

 	if(str != NULL){

		printf("private room %s has been created.\n", str);
		pr[prID].id = prID;

		strncpy(pr[prID]._name, str,strlen(str));

       		pr[prID].customers[0] = c;
		
  		pr[prID].creator = c;
		char *pass;
		printf("set a password for the private room:\n"); //same as create room with the addition of password
		scanf("%s", pass);
		pr[prID]._password = pass; 
		rID++;
		enter(str);//enter room after creating it
	}
	else{
		printf("enter a room name. the valid syntax is: -pcreate room_name\n");
	}

}

void enter(char *str){ //enter a room

 	if(str != NULL & c->isAtRoom != 1){

		
	 	int k = 0, l = 0;
		bool foundr = false, foundp = false;
		while(*r[k]._name!= '\0' | *pr[l]._name!= '\0') {
			
			if(strcmp(r[k]._name,str) == 0){
				foundr = true;
			}
			if(strcmp(pr[l]._name,str) == 0){
				foundp = true;
			}
			l++;
			k++;
		}

		if(!foundr & !foundp){ //cant find it
			printf("room %s does not exist.\n", str);
		}
		else if (foundr){ //found a common room
			printf("you are in room %s now.\n", str);
			int a = 0;
			while(r[k-1].customers[a] != NULL){
				a++;
			}
			r[k-1].customers[a] = c;
			c->isAtRoom = 1;
		}
		else if(foundp){ //found a private room
			char *pass;
			printf("enter the password for the private room %s \n", str);
			scanf("%s", pass);
			if(strcmp(pass, pr[l-1]._password)!=0 ){
				printf("you are in room %s now.\n", str);
				int a = 0;
				while(pr[l-1].customers[a] != NULL){
					a++;
				}
				pr[l-1].customers[a] = c;
				c->isAtRoom = 1;
			}

		}
	}
	else if(str != NULL & c->isAtRoom == 1){
		printf("you are already in a room.\n");
	}
	else{
		printf("enter a room name. the valid syntax is: -enter room_name\n");
	}

}

