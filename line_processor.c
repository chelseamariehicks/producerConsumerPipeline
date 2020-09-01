/**********************************************************************************
 * Name: Chelsea Marie Hicks
 * ONID 931286984
 * Course: CS 344
 * Assignment: Assignment #4
 * Due Date: August 14, 2020 by 11:59 PM
 * 
 * Description: Program for a multi-threaded producer consumer pipeline that reads 
 *      in lines of characters from standard input and writes to standard output
 *      lines that are exactly 80 chars in length with threas in the program that 
 *      replace every newline character with a space and every adjacent pair of 
 *      plus signs "++" with "^"
***********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

//Constant size for buffers and program end marker
#define SIZE 1000
#define TERM_LINE "DONE\n"

//Buffers for producer/consumer pipeline
char buffer1[SIZE];
char buffer2[SIZE];
char buffer3[SIZE];

//Tracker to call end of program 
bool done = false;

//Tracker to call end of input gathering due to encountering TERM_LINE
bool end = false;

//Initialize the mutex
pthread_mutex_t mutex_buffer1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer3 = PTHREAD_MUTEX_INITIALIZER;

//Initialize the condition variables 
pthread_cond_t buf1HasContents = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf2HasContents = PTHREAD_COND_INITIALIZER;
pthread_cond_t buf3HasContents = PTHREAD_COND_INITIALIZER;    

//Function gets input on a line by line basis, checks for the terminating line
//"DONE\n" and if not encountered, sends the data to the shared resource buffer1
void *getInput() {

    //Temporary buffer to put data entered before sending to buffer1
    char line[SIZE];

    //While the termination line hasn't been read, continue reading in lines
    do {
        //Read input from standard input
        fgets(line, sizeof(line), stdin);

        //Check if line is equal to the terminating line and if so, set end = true
        //This means that "DONE\n" will not be passed to buffer1
        if(strcmp(line, TERM_LINE) == 0) { 
            end = true;
        }
        //Line is not the terminating line, so send data to buffer1
        else {
            //Lock the mutex
            pthread_mutex_lock(&mutex_buffer1);

            //Send the line to buffer1
            strcat(buffer1, line);

            //Signal that buffer is full
            pthread_cond_signal(&buf1HasContents);

            //Unlock the mutex
            pthread_mutex_unlock(&mutex_buffer1);
        }

    } while(end == false);

    return NULL;
}

//Function reads input in buffer1, replaces newline characters with a space,
//and sends this altered data to buffer2
void *replaceNewline() {

    while(done == false) {
        //Temporary buffer to move contents of buffer1 into
        char newlineTempBuf[SIZE];

        //Get contents from buffer1 into temp buffer
        //Lock mutex
        pthread_mutex_lock(&mutex_buffer1);

        while(strlen(buffer1) < 1) {
            //Wait for getInput to signal that the buffer has contents
            pthread_cond_wait(&buf1HasContents, &mutex_buffer1);
        }

        //Place contents of buffer1 at the end of newlineTempBuffer
        strcat(newlineTempBuf, buffer1);

        //Clear out contents of buffer1 that have been read in
        memset(buffer1, 0, sizeof(buffer1));

        //Unlock the mutex
        pthread_mutex_unlock(&mutex_buffer1);

        //Loop through newlineTempBuf for "\n"
        int i;
        for(i = 0; i < strlen(newlineTempBuf); i++) {
            //When "\n" found, replace with " "
            if(newlineTempBuf[i] == '\n') {
                newlineTempBuf[i] = ' ';
            }   
        }

        //Send contents of newlineTempBuf with the line separators replaced to buffer2
        //Lock mutex
        pthread_mutex_lock(&mutex_buffer2);

        //Place the altered contents of newlineTempBuf in buffer2
        strcat(buffer2, newlineTempBuf);

        //Clear out contents of temporary buffer that have been read in
        memset(newlineTempBuf, 0, sizeof(newlineTempBuf));

        //Signal that buffer is full
        pthread_cond_signal(&buf2HasContents);

        //Unlock the mutex
        pthread_mutex_unlock(&mutex_buffer2); 

        //Check if the end of the program has been reached
        if(end == true) {
            done = true;
            break;
        }
    }

    return NULL;
}


//Function reads input from buffer2, replaces two adjacent "++" signs with "^",
//and sends altered data to buffer3
void *replacePlusSigns() {

    while(done == false) {

        //Temporary buffer to move contents of buffer2 into
        char plusSignBuf[SIZE];

        //Get contents from buffer2 into temp buffer
        //Lock mutex
        pthread_mutex_lock(&mutex_buffer2);

        while(strlen(buffer2) < 1) {
            //Wait for newline to signal that the buffer has contents
            pthread_cond_wait(&buf2HasContents, &mutex_buffer2);
        }

        //Place the contents of buffer2 into plusSignBuf
        strcat(plusSignBuf, buffer2);

        //Clear out contents of buffer2 that have been read in
        memset(buffer2, 0, sizeof(buffer2));

        //Unlock the mutex
        pthread_mutex_unlock(&mutex_buffer2);
    
        //Read contents in plusSignBuf to scan for "++"
        int i;
        for(i = 0; i < strlen(plusSignBuf); i++) {
            //When "++" found, replace with "^" 
            if(plusSignBuf[i] == '+' && plusSignBuf[i+1] == '+') {
                plusSignBuf[i] = '^';

                //Shift contents of plusSignBuf by one due to deletion of additional "+"
                int j;
                for(j = i + 1; j < strlen(plusSignBuf); j++) {
                    plusSignBuf[j] = plusSignBuf[j+1];
                }
            }
        }

        //Send contents of plusSignBuf with the "++" replaced with "^" to buffer3
        //Lock mutex
        pthread_mutex_lock(&mutex_buffer3);

        //Place contents of plusSignBuf into buffer3
        strcat(buffer3, plusSignBuf);

        //Signal that buffer is full
        pthread_cond_signal(&buf3HasContents);

        //Clear out contents of temporary buffer that have been read in
        memset(plusSignBuf, 0, sizeof(plusSignBuf));

        //Unlock the mutex
        pthread_mutex_unlock(&mutex_buffer3);

        //Check if term line has been read and if thread should exit
        if(end == true) {
            done = true;
            break;
        }
    }

    return NULL;
}

//Function writes output lines in 80 char chunks 
void *printOutput() {
    //Buffer for getting output to display
    char outputBuffer[SIZE];

    //Buffer to send to stdout that has space for 80 chars, newline and terminating char
    char outputReady[81];

    while(done == false) { 

        //Get contents from buffer3 into temp buffer
        //Lock mutex
        pthread_mutex_lock(&mutex_buffer3);

        while(strlen(buffer3) < 1) {
            //Wait for plussign to signal that the buffer has contents
            pthread_cond_wait(&buf3HasContents, &mutex_buffer3);
        }

        //Add contents of buffer3 to the end of outputBuffer
        strcat(outputBuffer, buffer3);

        //Clear out contents of buffer3 that have been read in
        memset(buffer3, 0, sizeof(buffer3));

        //Unlock the mutex
        pthread_mutex_unlock(&mutex_buffer3);

        //While loop will check if the temp buffer has more than 80 characters and print them if so
        while(strlen(outputBuffer) > 80) {
            memcpy(outputReady, outputBuffer, 80);
            //Add newline and terminating character to end of array
            outputReady[80] = '\n';
            outputReady[81] = '\0';

            //Print 80 characters to stdout
            printf("%s", outputReady);

            //Shift contents of outputBuffer by 80
            int i;
            for(i = 0; i < strlen(outputBuffer); i++) {
                outputBuffer[i] = outputBuffer[i+80];
            }

            //Check if DONE\n has been read by input when outputBuffer is less than 80
            if(strlen(outputBuffer) < 80) {
                if(end == true) {
                    done = true;
                    break;
                }
            }
        }   
    }
    return NULL;
}


int main(int argc, char *argv[]) {

    //Create the 4 threads for the program, input, line separator, plus sign convert, and output
    pthread_t input, lineSep, plusSign, output;
    pthread_create(&input, NULL, (void *)getInput, NULL);
    pthread_create(&lineSep, NULL, (void *)replaceNewline, NULL);
    pthread_create(&plusSign, NULL, (void *)replacePlusSigns, NULL);
    pthread_create(&output, NULL, (void *)printOutput, NULL);
    
    //Call join to wait for the threads to finish
    pthread_join(input, NULL);
    pthread_join(lineSep, NULL);
    pthread_join(plusSign, NULL);
    pthread_join(output, NULL);

    return 0;
}
