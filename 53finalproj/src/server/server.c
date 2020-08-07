// Jimmy Wang
// Aaron Yao
#include "server.h"
#include "protocol.h"
#include <pthread.h>
#include <signal.h>
#define USAGE_MSG   "petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME"                                           \
                    "\n  -h    		  Displays the help menu, and returns EXIT_SUCCESS"                                                             \
                    "\n  -j N  		  Number of job threads. Default to 2."  \
                    "\n  AUDIT_FILENAME   File to output Audit Log messages to." \
  		    "\n  PORT_NUMBER      Port number to listen on.\n" \

/*

bugs: 3 rooms between 2 clients
	crash cause by blank message

*/

const char exit_str[] = "exit";
List_t * listClients;
List_t * listJobs;
List_t* listRooms;


char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock;

int total_num_msg = 0;
int listen_fd;

void sigint_handler(int sig) {
    printf("shutting down server\n");
    close(listen_fd);
    exit(0);
}

void *cthread(int client_fd)
{
    //printf("Client fd is %d\n", client_fd);
	//read mesg from client
	//insert msg/job in buffer
	petr_header * petr = malloc(sizeof(petr_header));
    JobInfo* new_job = malloc(sizeof(JobInfo));
    	
	// printf("im the client thread\n");
	while (1)
	{
        // printf("Blocing here?\n");
		int readmsg = rd_msgheader(client_fd, petr);
        if (petr->msg_len == 0 && petr->msg_type == 0) {
            close(client_fd);
            pthread_exit(NULL);
            break;
        }
        // printf("Readmsg return value is : %d", readmsg);
		// printf("petr header message msg_len: %d\n", petr->msg_len);
		// printf("petr header message msg_type: %d\n", petr->msg_type);
        if (petr->msg_len) {
            // Msg_len is not 0 so read message data from socket
            int rdarg = read(client_fd, buffer, petr->msg_len);
           // printf("Allocate new str\n");
            char * tempstr = malloc(strlen(buffer)+1); // possibly need +1 for \0
           // printf("Allocate new str2\n");
            strcpy(tempstr, buffer);
            
            new_job->message_body = tempstr;
            
           // printf("Allocate new str3\n");
            if (petr->msg_type == 32) {
                
                int findroom = findbyroom(listRooms, tempstr);
             //   printf("Allocate new str4\n");
                if (findroom == -1)
                {
                    petr->msg_type = 0x2A; // sending back room error msg
                    petr->msg_len = 0;
                    int errmsg = wr_msg(client_fd, petr, "ERMEXISTS"); //refer to piazza @890
                    //do we quit out here? not sure what to do after error msg.
                }
            }

        }
        new_job->filedes = client_fd;
        new_job->petr_info = petr;
    //    printf("Msg body is %s\n", new_job->message_body);
        pthread_mutex_lock(&buffer_lock);
		insertFront(listJobs, new_job); 
        pthread_mutex_unlock(&buffer_lock);

	}
	return NULL;
}
void *jthread(void *vargp)
{
    //printf("im job thread %ld\n", (long)vargp);
    while (1) {
        // printf("Looping in job thread here?\n");
        if (listJobs->head != NULL) {
            pthread_mutex_lock(&buffer_lock);
            JobInfo* retrieved_job = removeRear(listJobs);
            pthread_mutex_unlock(&buffer_lock);
	        char room_str[1024];
            int num_users = 0;
	        int num_rooms = 0;
            node_t* current = listClients->head;
            Room * temproom;
            int roomindex;
            char * saveptr;
            char * saveptr2;

            if (retrieved_job) {
                switch(retrieved_job->petr_info->msg_type) {
                    case (17): //LOGOUT 0x11
                        // 1. Remove from all chat rooms.
                        // 2. Close all chat rooms created by this user.
                        // 3. Remove user from user list.
                        // 4. Send OK
			// pthread_mutex_lock(&buffer_lock);
                        exit_chat_rooms(retrieved_job->filedes, retrieved_job, listRooms, listClients, &buffer_lock);
			// pthread_mutex_unlock(&buffer_lock);
                        break;
                        // pthread_exit(NULL);


                    case (32): //RMCREATE 0X20
                        printf("Creating new room\n");
                        // RMCREATE
                        pthread_mutex_lock(&buffer_lock);
                        Room* new_room = malloc(sizeof(Room));
			            new_room->creator_fd = retrieved_job->filedes;
                        new_room->people = malloc(sizeof(struct List_t*));
			            new_room->roomname = retrieved_job->message_body;
                        pthread_mutex_unlock(&buffer_lock);
                        // printUserList(listClients);
                      //  printf("%s----\n",((User*)(current->value))->uname);
                        // printf("Finished allocating new people list\n");

			            current = listClients->head;
                        while(current != NULL) {
                            if (retrieved_job->filedes == ((User *)(current->value))->filedes) {
                                new_room->creator = ((User *)(current->value))->uname;
                                pthread_mutex_lock(&buffer_lock);
                                insertFront((List_t*)new_room->people, &(new_room->creator));
                                pthread_mutex_unlock(&buffer_lock);
                              //  printf("creator: %s\n", new_room->creator);
				                // printUserList((List_t*)(new_room->people));
                                num_users++;
                            }
                            current = current->next;
                        }
                        retrieved_job->petr_info->msg_type = 0;
                        retrieved_job->petr_info->msg_len = 0;
                        pthread_mutex_lock(&buffer_lock);
                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");

                        
                        insertFront(listRooms, new_room); // insert room into linkedList
                        pthread_mutex_unlock(&buffer_lock);
                        //printf("The room creator is %s", new_room->creator);
                        break;
                    case (33): //RMDELETE 0X21
                        //remove from list
                        //send messages to client
                        //free space
		
                        current = listRooms->head;
                        roomindex = findroomindex(listRooms, retrieved_job->message_body);

                       // printf("Room index to be deleted is %d", roomindex);
                        if (roomindex == -1) //room not found.
                        {
                            retrieved_job->petr_info->msg_len = 0;
                            retrieved_job->petr_info->msg_type = 44;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMNOTFOUND");
                            break;
                        }
                        else
                        {
                            for (int i = 0; i < roomindex; i++) {
                                current = current->next;
                            }
                            temproom = ((Room*)(current->value));
                            if (strcmp(retrieved_job->message_body, temproom->roomname) == 0) // if this is entered, correct room is found.
                            {

                                //look by filedes for user.
                                if (retrieved_job->filedes != temproom->creator_fd) // if the user trying to delete is not the creator. checked by fd.
                                {
                                    retrieved_job->petr_info->msg_len = 0;
                                    retrieved_job->petr_info->msg_type = 45;
                                    wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMDENIED");
                                    break;
                                }
                                else // if u are the creator of the room.
                                {
                                    List_t* loop_users = (List_t*)(((Room*)(current->value))->people);
                                    node_t* person = loop_users->head;
                                    User* person_data;
                                    
                                    while(person != NULL) // go through participant list and send RMCLOSED
                                    {
	                                    //printf("user index: %d\n", userindex);
                                        person_data = (User*)(person->value);
                                        // printf("checking here\n");
                                  //      printf("Person is %s, Creator is %s\n", person_data->uname, temproom->creator);
                                        if (person_data->filedes != temproom->creator_fd) // sending to people who are not the creator
                                        {
                                            retrieved_job->petr_info->msg_type = 34;
                                            // retrieved_job->petr_info->msg_len = strlen(retrieved_job->message_body) + 1;
                                    //        printf("Msg len is %d, Msg type is %d\n", retrieved_job->petr_info->msg_len, retrieved_job->petr_info->msg_type);
                                    //        printf("Room name is %s\n", temproom->roomname);
                                            wr_msg(person_data->filedes, retrieved_job->petr_info, ("RMCLOSED%s", temproom->roomname));
                                        }
                                        else //send to creator.
                                        {
                                         //   printf("you are the creator. deleting the room\n");
                                            retrieved_job->petr_info->msg_type = 0;
                                            retrieved_job->petr_info->msg_len = 0;
                                            wr_msg(person_data->filedes, retrieved_job->petr_info, "OK");
                                        }
                                        person = person->next;
                                    }
                                    pthread_mutex_lock(&buffer_lock);
                                    removeByIndex(listRooms, roomindex);
                                    pthread_mutex_unlock(&buffer_lock);
                                }
                            }
                        }			
                        break;

                    case (35): //RMLIST 0X23
                        //server  respond with RMLIST 
                        // room: person 1, person 2
                        // no rooms  on server = RMLIST msg len 0

                        current = listRooms->head;
                        if (current == NULL)
                        {
                            retrieved_job->petr_info->msg_len = 0;
                            // retrieved_job->petr_info->msg_type = 0;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");
                            break;
                        }
                        List_t* loop_users = NULL;
                        int comma = 0;
                        while(current != NULL) {
                            loop_users = (List_t*)(((Room*)(current->value))->people);
                            node_t* loop_users_node = loop_users->head;
                            char * temp = ((Room*)(current->value))->roomname;
                            //printf("%s---\n", temp);
                            strcat(room_str, temp);
                            strcat(room_str, ": ");
                            num_rooms++;
                            while (loop_users_node != NULL) {
                                // printf("User is %s", ((User*)(loop_users_node->value))->uname);
                                
                                if (comma) {
                                    strcat(room_str, ",");
                                }
                                strcat(room_str, ((User*)(loop_users_node->value))->uname);
                                
                                loop_users_node = loop_users_node->next;
                                comma++;
                            }
                            strcat(room_str, "\n\0");
                            current = current->next;
                            comma = 0;
                        }
                        retrieved_job->petr_info->msg_len = strlen(room_str)+1;
                        // printf("RMLIST len is %d\n", retrieved_job->petr_info->msg_len);
                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, room_str);    
                        strcpy(room_str, "");
                        break;
                    
                    case (36): // RMJOIN 0x24
                        // TO DO:
                        // test limit-stuff later
                        current = listRooms->head;
                        roomindex = findroomindex(listRooms, retrieved_job->message_body);
                        if (roomindex == -1) //room not found.
                        {
                            retrieved_job->petr_info->msg_len = 0;
                            retrieved_job->petr_info->msg_type = 44;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMNOTFOUND");
                            break;
                        }
                        else
                        {
                            for (int i = 0; i < roomindex; i++) {
                                current = current->next;
                            }
                            temproom = ((Room*)(current->value));
                            if (strcmp(retrieved_job->message_body, temproom->roomname) == 0) // if this is entered, correct room is found.
                            {
                                // TO DO: check if room limit is met

                                node_t* list_user = listClients->head;

                                while(list_user != NULL) {
                                    if(retrieved_job->filedes == ((User *)(list_user->value))->filedes) {
                                        // Get name of user trying to join room.
                                        
                                        printf("%s joined room %s\n", ((User *)(list_user->value))->uname, temproom->roomname);
                                        pthread_mutex_lock(&buffer_lock);
                                        insertFront((List_t*)(temproom->people), &((User *)(list_user->value))->uname);
                                        pthread_mutex_unlock(&buffer_lock);

                                        retrieved_job->petr_info->msg_type = 0;
                                        retrieved_job->petr_info->msg_len = 0;
                                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");
                                        break;
                                    }
                                    list_user = list_user->next;
				}
                            }

                        }	
                        break;
                    case (37): // RMLEAVE 0x25

                        current = listRooms->head;
                        roomindex = findroomindex(listRooms, retrieved_job->message_body);
                        int user_index = 0;
                        int user_not_in_room = 1;
                        if (roomindex == -1) //room not found.
                        {
                            printf("Trying to leave a room that doesn't exist\n");
                            retrieved_job->petr_info->msg_len = 0;
                            retrieved_job->petr_info->msg_type = 44;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMNOTFOUND");
                            break;
                        }
                        else {
                            while (current != NULL) // searching for correct room, could be a for loop because we know the index.
                            {
                                temproom = ((Room*)(current->value));

                                if (strcmp(retrieved_job->message_body, temproom->roomname) == 0) // if this is entered, correct room is found.
                                {
                                    node_t* list_user = ((List_t*)(((Room*)(current->value))->people))->head;
                                    while(list_user != NULL) {
                                        if(retrieved_job->filedes == ((User *)(list_user->value))->filedes) {
                                            if (retrieved_job->filedes != temproom->creator_fd) {
                                                // You're in the room and not the creator
                                                pthread_mutex_lock(&buffer_lock);
                                                removeByIndex((List_t*)(((Room*)(current->value))->people), user_index);
                                                pthread_mutex_unlock(&buffer_lock);
                                                retrieved_job->petr_info->msg_type = 0;
                                                retrieved_job->petr_info->msg_len = 0;
                                                wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");
                                            }

                                            else {
                                                // You're are the room creator, no leaving
                                                retrieved_job->petr_info->msg_type = 0x2D;
                                                retrieved_job->petr_info->msg_len = 0;
                                                wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMDENIED");
                                            }
                                            user_not_in_room = 0;
                                            break;
                                        }
                                        list_user = list_user->next;
                                        user_index++;
                                    }
                                    // You're not in the room
                                    if (user_not_in_room) {
                                        printf("You are not in the room \n");
                                        retrieved_job->petr_info->msg_type = 0;
                                        retrieved_job->petr_info->msg_len = 0;
                                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");
                                        break;
                                    }
                                    
                                }
                                current = current->next;
                            }
                        }
                        break;
		    
                    case (38): //RMSEND 0x26
                    current = listRooms->head;
                    char * msg_roomname;
                    char * msg_msg;
                    msg_roomname = strtok_r(retrieved_job->message_body, "\r", &saveptr); //roomname first, then message.
                    msg_msg = strtok_r(NULL, "\r", &saveptr);
                    if (msg_msg == NULL)
                        strcpy(msg_msg, "\0");
                    //printf("room name!!!: %s\n", msg_roomname);
                    //printf("the messages: %s\n", msg_msg);

                    roomindex = findroomindex(listRooms, msg_roomname);
        //printf("msg body%s\n", retrieved_job->message_body);
                    
                    //printf("room index: %d\n", roomindex);
                    int validsender = 0;
                    char * sendername;
                    pthread_mutex_lock(&buffer_lock);
                    char * msg = malloc(1024 * sizeof(char*)); // uncertain, may need to malloc a different amt. malloc size for message here.
                    pthread_mutex_unlock(&buffer_lock);
                                if (roomindex == -1) //room not found.
                                {
                                    retrieved_job->petr_info->msg_len = 0;
                                    retrieved_job->petr_info->msg_type = 44;
                                    wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMNOTFOUND");
                                    break;
                                }
                                // room found
                            
                    for (int i = 0; i < roomindex; i++)
                    {
                        current = current->next;
                    //get to current room for room data
                    }
                            node_t* list_user = ((List_t*)(((Room*)(current->value))->people))->head; //list of users in the room
                            while(list_user != NULL) {
                        if (((User*)(list_user->value))->filedes == retrieved_job->filedes) // check if you are in the room.
                        {
                        //send message in here.
                    
                            
                            validsender=1;
                            sendername = ((User*)(list_user->value))->uname; // grab name of sender
                            break;
                        }	
                                list_user = list_user->next;

                    }

                    if (validsender == 1)
                    {
                        //send to everyone else except yourself

                        list_user = ((List_t*)(((Room*)(current->value))->people))->head;
                        while(list_user != NULL)
                        {
                            if (((User*)(list_user->value))->filedes != retrieved_job->filedes)
                            {
                                pthread_mutex_lock(&buffer_lock);
                                strcpy(msg,((Room*)(current->value))->roomname);
                                strcat(msg, "\r\n");
                                strcat(msg, sendername);
                                strcat(msg, "\r");
                                strcat(msg, msg_msg);
	                            printf("message %s\n", msg);
                                retrieved_job->petr_info->msg_type = 39;
                                retrieved_job->petr_info->msg_len  = strlen(msg)+1;
                                //printf("RMRECV len is %d\n", retrieved_job->petr_info->msg_len);
                                wr_msg(((User*)(list_user->value))->filedes, retrieved_job->petr_info, ("RMRECV %s", msg)); //send message
                                pthread_mutex_unlock(&buffer_lock);

                                
                            }
                            list_user = list_user->next;
                        }
                        retrieved_job->petr_info->msg_type = 0;
                        retrieved_job->petr_info->msg_len  = 0;
                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK"); //send ok to yourself.
                    }

                    else
                    {
                            retrieved_job->petr_info->msg_len = 0;
                            retrieved_job->petr_info->msg_type = 45;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "ERMDENIED");
                        //u are not in room, send the error msg
                    }        
                    break;

                    case (48): //USRSEND 0x30
                    //FIXING EDGE CASES: blank message and sending to self.
                    ;
                    char * msg_contents;
                    char * recipient;
                    User * recipient_data;
                    pthread_mutex_lock(&buffer_lock);
                    char * sender = malloc(1024 * sizeof(char*));
                    pthread_mutex_unlock(&buffer_lock);
                    //int recipient_index = 0;
                    recipient = strtok_r(retrieved_job->message_body, "\r", &saveptr2); //recipient name first, then message.
                    msg_contents = strtok_r(NULL, "\r", &saveptr2);
                    if (msg_contents == NULL)
                        strcpy(msg_contents, "\0");
		
                    if (findbyname(listClients, recipient) != -1)
                    {
                    //user does not exist.
                        retrieved_job->petr_info->msg_len = 0;
                        retrieved_job->petr_info->msg_type = 58;
                        wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "EUSRNOTFOUND");
                        break;
                    }
//////////current?
		    current = listClients->head;
                    while(current != NULL) // fetching sender's name
                    {
                        if(retrieved_job->filedes == ((User*)(current->value))->filedes)
                        {
                            strcpy(sender, ((User*)(current->value))->uname);
                            break;
                        }
                        current = current->next;
                    }	
                    current = listClients->head;
                    while(current != NULL) //finding the matching recipient so we can get their file des
                    {
                        recipient_data = ((User*)(current->value));
                        if (strcmp(recipient, recipient_data->uname) == 0)
                        {
                            pthread_mutex_lock(&buffer_lock);
                            strcat(sender, "\r");
                            strcat(sender, msg_contents);
                            retrieved_job->petr_info->msg_len = strlen(sender)+1;
                            retrieved_job->petr_info->msg_type = 49;
                            // printf("USRRECV Len is %d\n", retrieved_job->petr_info->msg_len);
		printf("message: %s\n", sender);
                            wr_msg(recipient_data->filedes, retrieved_job->petr_info, ("USRRECV%s", sender));
                            retrieved_job->petr_info->msg_len = 0;
                            retrieved_job->petr_info->msg_type = 0;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "OK");
                            pthread_mutex_unlock(&buffer_lock);
                            break;
                        }
                        current = current->next;
                    }

                    break;

                    case (50): //USRLIST 0x32
			            pthread_mutex_lock(&buffer_lock);
                        char * other_users = malloc(1024 * sizeof(char*));
                        strcpy(other_users, "");
                        pthread_mutex_unlock(&buffer_lock);

                        current = listClients->head;
                        while(current != NULL) {
                            if (retrieved_job->filedes != ((User *)(current->value))->filedes) {
                               // printf("%s ------------------\n", ((User *)(current->value))->uname);
                                pthread_mutex_lock(&buffer_lock);
                                strcat(other_users, ((User *)(current->value))->uname);
                                strcat(other_users, "\n");
                                num_users++;
                                pthread_mutex_unlock(&buffer_lock);
				//printf("num+users: %d\n", num_users);
                            }
                            current = current->next;
                        }

                        // There are other users online
                        if (num_users) {
                            retrieved_job->petr_info->msg_len = strlen(other_users) + 1;
                            // printf("msg_len size is %d\n", retrieved_job->petr_info->msg_len);
		//printf("other_users: %s\n", other_users);
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, other_users);
                        }
                        else {
                            retrieved_job->petr_info->msg_len = 0;
                            wr_msg(retrieved_job->filedes, retrieved_job->petr_info, "USRLIST");
                        }
                        
                        // strcpy(user_str, "");

                        // write(retrieved_job->filedes, &user_str, sizeof(user_str));
                        break;


                    default:
                        return NULL;
                     //   printf("Not any of the commands\n");
                }

            }
        }
    }
    
	//supposed to block while not performing a job
	return NULL;
}

int server_init(int server_port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } else
	{
		listJobs = (List_t*)(malloc(sizeof(List_t))); 
        listRooms = (List_t*)(malloc(sizeof(List_t)));
		listClients = (List_t*)(malloc(sizeof(List_t))); // creating  a list of clients connected to server based on username
       		printf("Server listening on port: %d.. Waiting for connection\n", server_port);
	}

    return sockfd;
}

//Function running in thread
void *process_client(void *clientfd_ptr) {
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    fd_set read_fds;
	pthread_t client_thread;

	petr_header * petr = malloc(sizeof(petr_header));
	int readmsg = rd_msgheader(client_fd, petr);
	if (readmsg == -1)
	{
		printf("rd_msgheader error: error reading header\n");
	//exit  fix later
	}
	//printf("petr header message msg_len: %d\n", petr->msg_len);
	//printf("petr header message msg_type: %d\n", petr->msg_type);
	
	
	int readuname = read(client_fd, buffer, petr->msg_len); // reading in username
//printf("petr name: %s\n", buffer);

	User * newU = malloc(sizeof(User));
	newU->filedes = client_fd;
	char * tempstr = malloc(strlen(buffer)+1); // possibly need +1 for \0
	strcpy(tempstr, buffer);
	newU->uname = tempstr;


	// printf("before\n");
	// printUserList(listClients);
	
	int findname = findbyname(listClients, tempstr);

	if (findname == -1)
	{
		petr->msg_type = 0x1A; // sending back login error msg
		petr->msg_len = 0;
		int errmsg = wr_msg(client_fd, petr, "EUSREXISTS"); //refer to piazza @890
		//do we quit out here? not sure what to do after error msg.
		return NULL;
	}
	else
	{
		petr->msg_type = 0;
        petr->msg_len = 0;
		wr_msg(client_fd, petr, 0);
        	insertFront(listClients, newU); // insert user into linkedList
		pthread_create(&client_thread, NULL, cthread(client_fd), NULL);//make a cthread
	}
	
	
	// printf("after\n");	
	// printUserList(listClients);


    int retval;
    while (1) {

        FD_ZERO(&read_fds);

        FD_SET(client_fd, &read_fds);

        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);

        if (retval != 1 && !FD_ISSET(client_fd, &read_fds)) {
            printf("Error with select() function\n");
            break;
        }

        pthread_mutex_lock(&buffer_lock);

        bzero(buffer, BUFFER_SIZE);
        received_size = read(client_fd, buffer, sizeof(buffer));
        if (received_size < 0) {
            printf("Receiving failed\n");
            break;
        } else if (received_size == 0) {
            continue;
        }

        if (strncmp(exit_str, buffer, sizeof(exit_str)) == 0) {
            printf("Client exit\n");
            break;
        }
        total_num_msg++;
        // print buffer which contains the client contents
        printf("Receive message from client: %s\n", buffer);
        printf("Total number of received messages: %d\n", total_num_msg);

        sleep(1); //mimic a time comsuming process

        // and send that buffer to client
        int ret = write(client_fd, buffer, received_size);
        pthread_mutex_unlock(&buffer_lock);

        if (ret < 0) {
            printf("Sending failed\n");
            break;
        }
        printf("Send the message back to client: %s\n", buffer);

    }
    // Close the socket at the end
    printf("Close current client connection\n");
    close(client_fd);
    return NULL;
}

void run_server(int server_port, int jthreads) {
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
	pthread_t job_thread; // job thread
long i = 0;
	//remember to default n job threads =2
	for (i = 0; i < jthreads; i++)
	{
		pthread_create(&job_thread, NULL, jthread, (void *)i);
	}

    pthread_t tid;
//listening for client connect requests.
    while (1) {
        // Wait and Accept the connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);
        if (*client_fd < 0) {
            printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Client connetion accepted\n");
            pthread_create(&tid, NULL, process_client, (void *)client_fd);
        }
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

int main(int argc, char *argv[]) {
    int opt;
    int counter = 0;
    unsigned int port = 0;
	int jthreads = 2;
    while ((opt = getopt(argc, argv, "hj:")) != -1) { //breaks command line args into variables.
	  //printf("opt: %d\n", opt);   
      counter++;   
	  switch (opt) {
        case 'j':
            counter++;
          //  printf("opt: %d\n", opt);   
            jthreads = atoi(argv[counter]);
            break;
        case 'h':
            printf(USAGE_MSG);
            exit(EXIT_SUCCESS);
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    counter++;   
    // for (int i = 0; i < argc; i++) {
    //     printf("%d, %s\n", i, argv[i]);
    // }
    // printf("Port numbere is %d\n", atoi(argv[counter]));
    port = atoi(argv[counter]);

    if (port == 0) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    run_server(port, jthreads);

    return 0;
}