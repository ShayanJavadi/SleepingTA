/*
 * sleepingTA.c
 *
 *  Created on: Oct 26, 2016
 *      Author: stanley
 */

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_SLEEP_TIME 3
#define NUM_OF_STUDENTS 4
#define NUM_OF_HELPS 2
#define NUM_OF_SEATS 2

pthread_mutex_t mutex_lock;
sem_t students_sem;
sem_t ta_sem;
int waiting_students;
int students_finished;

void *student(void *args) {
	/* index of student thread in the thread id array */
	int index = (int) args;

	/* seed for random wait time function */
	int seed = (index + 1) * MAX_SLEEP_TIME * NUM_OF_STUDENTS * NUM_OF_HELPS * NUM_OF_SEATS;

	/* amount of times the student has been helped */
	int help_count = 0;

	/* loop until student has been helped two times */
	while (help_count < NUM_OF_HELPS) {
		/* student programs for random amount of time */
		int rand_wait_time = (rand_r(&seed) % MAX_SLEEP_TIME) + 1;
		printf("\tStudent %d programming for %d seconds\n", index, rand_wait_time);
		sleep(rand_wait_time);

		/* critical section is locked */
		pthread_mutex_lock(&mutex_lock);
		if (waiting_students < NUM_OF_SEATS) {
			/* increments amount of students waiting in seats */
			waiting_students++;
			printf("\t\tStudent %d takes a seat, # of waiting students = %d\n", index, waiting_students);

			/* critical section is unlocked */
			pthread_mutex_unlock(&mutex_lock);

			/* signals ta thread to unblock */
			sem_post(&students_sem);

			/* blocks until a signal is received from ta */
			sem_wait(&ta_sem);
			printf("Student %d receiving help\n", index);

			/* increment help counter for student */
			help_count++;
		} else {
			pthread_mutex_unlock(&mutex_lock);
			printf("\t\t\tStudent %d will try later\n", index);
		}
	}
	/* increments amount of students finished; student thread exits */
	students_finished++;
	pthread_exit(NULL);
}

void *ta(void *args) {
	int seed = 5 * MAX_SLEEP_TIME * NUM_OF_STUDENTS * NUM_OF_HELPS * NUM_OF_SEATS;

	while (1) {
		/* blocks until a signal is received from student */
		sem_wait(&students_sem);

		/* critical section is locked */
		pthread_mutex_lock(&mutex_lock);

		/* decrements amount of students waiting in seats */
		waiting_students--;
		
		int rand_wait_time = (rand_r(&seed) % MAX_SLEEP_TIME) + 1;
		printf("Helping a student for %d seconds, # of waiting students = %d\n", rand_wait_time, waiting_students);

		/* critical section is unlocked */
		pthread_mutex_unlock(&mutex_lock);

		/* waits for random amount of time while helping student; signals student thread to unblock */
		sem_post(&ta_sem);
		sleep(rand_wait_time);
	}
}

int main() {
	printf("CS149 SleepingTA from Stanley Plagata\n");

	/* initialize student threads and ta thread */
	pthread_t students_tid[NUM_OF_STUDENTS];
	pthread_t ta_tid;

	/* initialize the mutex lock */
	pthread_mutex_init(&mutex_lock, NULL);

	/* initialize the semaphores */
	sem_init(&ta_sem, 0, 0);
	sem_init(&students_sem, 0, 0);

	/* creates the ta thread */
	pthread_create(&ta_tid, NULL, ta, NULL);

	/* creates the student threads */
	for (int i = 0; i < NUM_OF_STUDENTS; i++) pthread_create(&students_tid[i], NULL, student, (void *) i);

	/* joins the student threads */
	for (int i = 0; i < NUM_OF_STUDENTS; i++) pthread_join(students_tid[i], NULL);

	/* loops until all students are finished; then cancels ta thread */
	while (1) {
		if (students_finished == NUM_OF_STUDENTS) {
			pthread_cancel(ta_tid);
			break;
		}
	}

	/* destroys the mutex lock */
	pthread_mutex_destroy(&mutex_lock);

	/* destroys the semaphores */
	sem_destroy(&students_sem);
	sem_destroy(&ta_sem);
}
