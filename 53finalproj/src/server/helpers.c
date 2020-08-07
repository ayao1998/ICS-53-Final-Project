#include "helpers.h"
void exit_chat_rooms(int fileds, JobInfo* job_info, List_t* rooms, List_t* users, pthread_mutex_t* lock) {
    
    node_t* room_list_node = rooms->head;
    node_t* participant_search;
    Room* current_room;
    List_t* participants;
    int participant_index = 0;
    int user_index = 0;

    // printUserList(users);
    while (room_list_node != NULL) {
        // Go through room by room in listRooms
        current_room = (Room*)(room_list_node->value);
        participants = (List_t*)(current_room->people);
	    participant_search = participants->head;  
        if (current_room->creator_fd == fileds) {
            // printf("You are the creator\n");
            // User is the creator of the room
            while(participant_search != NULL) // go through participant list and send RMCLOSED
            {
                // printf("The creator fd is %d\n", current_room->creator_fd);
                // printf("Your fd is %d\n", ((User*)(participant_search->value))->filedes);
                if (((User*)(participant_search->value))->filedes != current_room->creator_fd) // sending to people who are not the creator
                {
                  
                    job_info->petr_info->msg_type = 34;
		            job_info->petr_info->msg_len = strlen(current_room->roomname)+1;
                    pthread_mutex_lock(lock);
                    wr_msg(((User*)(participant_search->value))->filedes, job_info->petr_info, ("RMCLOSED%s", current_room->roomname));
                    pthread_mutex_unlock(lock);

                }
                participant_search = participant_search->next;
            }
            int room_index = findroomindex(rooms, current_room->roomname);
            pthread_mutex_lock(lock);
            removeByIndex(rooms, room_index);
            pthread_mutex_unlock(lock);

        }
        else {
            // printf("You are not the creator\n");
            // If you arent the creator
            while (participant_search != NULL) {
                if (((User*)(participant_search->value))->filedes == fileds) {
                    pthread_mutex_lock(lock);
                    removeByIndex((List_t*)(participants), participant_index);
                    pthread_mutex_unlock(lock);
                    break;
                    
                }
                participant_search = participant_search->next;
                participant_index++;
            }
            participant_index = 0;
        }

        room_list_node = room_list_node->next;
    }

    // Remove yourself from clients
    // printf("Removing from listClients\n");
    node_t* user_list_node = users->head;
    while (user_list_node != NULL) {
        if (((User*)(user_list_node->value))->filedes == fileds) {
            pthread_mutex_lock(lock);
            removeByIndex(users, user_index);
            pthread_mutex_unlock(lock);
        }
        user_list_node = user_list_node->next;
        user_index++;
    }

    
    job_info->petr_info->msg_type = 0;
    job_info->petr_info->msg_len = 0;
    pthread_mutex_lock(lock);
    wr_msg(job_info->filedes, job_info->petr_info, "OK");
    pthread_mutex_unlock(lock);
    // printf("Succesfully logged out\n");
    return;
}