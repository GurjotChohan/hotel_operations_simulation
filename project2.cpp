/**************************************************************************************
 * Project Name: Exploring Multiple Threads
 * Brief desc: A hotel simulation with threads and semaphores 
***************************************************************************************/


#include <iostream>
#include <random>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

#include <unistd.h>
#include <pthread.h>

#include "LineInfo.h"

void* front_desk(void*);
void* guest(void*);
void* bell_hop(void*);

const int CUST = 25; 
sem_t guest_queue_mutex; 
sem_t front_queue_mutex; 
sem_t bellhop_queue_mutex; 
sem_t bellhop_queue_mutex2; 
sem_t guest_ready; 
sem_t front_desk_ready; 
sem_t front_desk_cord_done[CUST];  
sem_t guest_reached_room[CUST];  
sem_t bell_hop_ready; 
sem_t guest_ready_bellhop; 
sem_t bellhopdeliversbags; 
sem_t bellhop_recives_bags; 
sem_t bell_hop_left; 
pthread_t guest_thread[CUST];
pthread_t font_desk_thread[2];
pthread_t bell_hop_thread[2];
queue<int> frontdesk_line;
queue<int> bellhop_line; 
int room_number[CUST] = {-1}; 
int front_desk_m[CUST] = {-1}; 
int bagassignment[CUST] = {-1};
int bellhop_assign[CUST] = {-1};
int room_number_var = 0; 

/**************************************************************************************
 * Function name: guest
 * Function args: void *ptr
 * Function return: void
 * Function This is the funtion for the guest thread all the operations of the guest are
 * done through this thread. 
***************************************************************************************/

void *guest(void *ptr){
  int guestid = *(int *)ptr; 
  if(bagassignment[guestid] == 1){
    printf("Guest %d enters hotel with %d bag \n", guestid, bagassignment[guestid]); 
    fflush(stdout);
  }
  else {
    printf("Guest %d enters hotel with %d bags \n", guestid, bagassignment[guestid]);
    fflush(stdout);
  }
  sem_wait(&guest_queue_mutex);
  frontdesk_line.push(guestid); 
  sem_post(&guest_queue_mutex);  // protect the push operation

  sem_post(&guest_ready); // guest is ready for the front desk
  sem_wait(&front_desk_cord_done[guestid]); // make sure the right guest finishies
  printf("Guest %d receives room key for room %d from front desk employee %d \n", guestid, room_number[guestid], front_desk_m[guestid]); 
  fflush(stdout);
  sem_post(&front_desk_ready); // free the front desk

  if(bagassignment[guestid] <= 2)
    printf("Guest %d enters room %d \n", guestid, room_number[guestid]);
  else 
  {
    sem_wait(&bell_hop_ready); // make sure that the bell_hop is free
    printf("Guest %d requests help with bags \n", guestid); 
    fflush(stdout);
    sem_wait(&bellhop_queue_mutex);
    bellhop_line.push(guestid); 
    sem_post(&bellhop_queue_mutex); // protect the push operation

    sem_post(&guest_ready_bellhop); // the guest is ready for the bell hop

    sem_wait(&bellhop_recives_bags); // wait to drop off the bags

    printf("Guest %d enters room %d \n", guestid, room_number[guestid]); 
    fflush(stdout);
    sem_post(&guest_reached_room[guestid]);  // go to room after drop off
    sem_wait(&bellhopdeliversbags); // wait for bags
    printf("Guest %d receives bags from bellhop %d and gives tip \n", guestid, bellhop_assign[guestid]); 
    fflush(stdout);
    sem_post(&bell_hop_left); // realse the bellhop
  }

  printf("Guest %d retires for the evening \n", guestid); // last statment
  fflush(stdout);

return NULL; 

}
/**************************************************************************************
 * Function name: front_desk
 * Function args: void *ptr
 * Function return: void
 * Function This is the funtion for the front_desk thread all the operations of the front employee
 * are done through this thread. 
***************************************************************************************/
void *front_desk(void *ptr){

  int front_desk_id = *(int *)ptr; 
  int guestid; 
  while(true)
  {
    sem_wait(&front_desk_ready); // make sure the guest is ready
    sem_wait(&guest_ready);  // make sure the front is ready
    
    sem_wait(&front_queue_mutex); 

    guestid = frontdesk_line.front(); 
    frontdesk_line.pop();
    room_number_var++;
    room_number[guestid] = room_number_var; 
    sem_post(&front_queue_mutex); // protect the pop operation
    
    front_desk_m[guestid] = front_desk_id; // assign an front desk person
    printf("Front desk employee %d registers guest %d and assigns room %d \n", front_desk_m[guestid], guestid, room_number[guestid]);
    fflush(stdout);
    sem_post(&front_desk_cord_done[guestid]); // emp part is done
  }

return NULL; 

}
/**************************************************************************************
 * Function name: bell_hop
 * Function args: void *ptr
 * Function return: void
 * Function This is the funtion for the bellhop thread all the operations of the bellhops
 * are done through this thread. 
***************************************************************************************/
void *bell_hop(void *ptr){

  int bellhop_id = *(int *)ptr; 
  int guest_id; 

  while(true)
  {
    sem_wait(&guest_ready_bellhop); // wait for the guest to be ready

    sem_wait(&bellhop_queue_mutex2);  // protect the pop operation
    guest_id = bellhop_line.front(); 
    bellhop_line.pop();
    bellhop_assign[guest_id] = bellhop_id; 
    sem_post(&bellhop_queue_mutex2); 

    
    printf("Bellhop %d receives bags from guest %d \n", bellhop_assign[guest_id],guest_id);
    fflush(stdout);

    sem_post(&bellhop_recives_bags);  // let the guest thread know that the bags are recived
    sem_wait(&guest_reached_room[guest_id]);  // wait for the guest to reach room
    printf("Bellhop %d delivers bags to guest %d \n", bellhop_assign[guest_id], guest_id); 
    fflush(stdout);
    sem_post(&bellhopdeliversbags);  // delivery done
    sem_wait(&bell_hop_left); // after the tip is recived
    sem_post(&bell_hop_ready); // free the bellhop
  }

return NULL; 

}

int main(int argc, char* argv[]) {

  try{

    if (argc !=1)
        throw domain_error(LineInfo("Usage: ./main", __FILE__, __LINE__));


    int thread_check = -1;
    int ids[CUST]; 
    int front_ids[2]; 
    int bellhop_ids[2]; 

    //intilizing the array of semaphores
    for(int i = 0; i < CUST; i++)
    {
      if (sem_init (&front_desk_cord_done[i], 0, 0) == -1)
      {
        printf("Init front_desk_cord_done\n");
        exit(1);
      }

      if (sem_init (&guest_reached_room[i], 0, 0) == -1)
      {
        printf("Init guest_reached_room\n");
        exit(1);
      }
    }

    //intilizing semaphores
    if (sem_init (&bell_hop_left, 0, 0) == -1)
    {
      printf("Init bell_hop_left\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&bell_hop_ready, 0, 2) == -1)
    {
      printf("Init bell_hop_ready\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&guest_ready_bellhop, 0, 0) == -1)
    {
      printf("Init guest_ready_bellhop\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&bellhop_queue_mutex, 0, 1) == -1)
    {
      printf("Init bellhop_queue_mutex\n");
      exit(1);
    }

    //intilizing semaphores
    if (sem_init (&bellhopdeliversbags, 0, 0) == -1)
    {
      printf("Init bellhopdeliversbags\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&bellhop_recives_bags, 0, 0) == -1)
    {
      printf("Init bellhop_recives_bags\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&bellhop_queue_mutex2, 0, 1) == -1)
    {
      printf("Init bellhop_queue_mutex2\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&guest_queue_mutex, 0, 1) == -1)
    {
      printf("Init guest_queue_mutex\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&front_queue_mutex, 0, 1) == -1)
    {
      printf("Init front_queue_mutex\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&front_desk_ready, 0, 2) == -1)
    {
      printf("Init front_desk_ready\n");
      exit(1);
    }
    //intilizing semaphores
    if (sem_init (&guest_ready, 0, 0) == -1)
    {
      printf("Init guest_ready\n");
      exit(1);
    }

    //assigning the random number of bags for each guest

    for(int i =0; i< CUST; i++){
        std:: random_device rd; 
        std:: uniform_int_distribution<int> dist(0,5);  
        bagassignment[i] = dist(rd); 
    }

    printf("Simulation starts \n");

    // making the front_desk threads
    for(int i = 0; i < 2; i++)
    {
      front_ids[i]=i;
      if( (thread_check == pthread_create( &font_desk_thread[i], NULL, front_desk, &front_ids[i])) )
          printf("Thread creation failed: %d\n", thread_check);
      else {
          printf("Front desk employee %d created \n", i);
          fflush(stdout);
      }
    }
    // making the bellhop threads
    for(int i = 0; i < 2; i++)
    {
      bellhop_ids[i]=i;
      if( (thread_check == pthread_create( &bell_hop_thread[i], NULL, bell_hop, &bellhop_ids[i])) )
          printf("Thread creation failed: %d\n", thread_check);
      else {
          printf("Bellhop %d created \n", i);
          fflush(stdout);
      }
    }
    // making the guest threads
    for(int i = 0; i < CUST; i++)
    {
      ids[i]=i;
      printf("Guest %d created \n", i);
      fflush(stdout);
      thread_check = pthread_create( &guest_thread[i], NULL, guest, &ids[i]);

      if(thread_check == -1 )
      {
         printf("Guest creation failed: %d\n", thread_check);
         fflush(stdout);
      }
    }

    // joining the guest threads
    for(int i = 0; i<CUST; i++){

      thread_check = pthread_join(guest_thread[i], NULL);
      if(thread_check != -1 )
      {
        printf("Guest %d joined\n", i); 
        fflush(stdout);
      }
    }
    printf("Simulation ends\n"); 
    
  }
  catch (exception& e) {
      cout << e.what() << endl;
      exit(EXIT_FAILURE);
  }//catch

  exit(EXIT_SUCCESS);
}