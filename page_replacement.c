/*
    COMP3511 Fall 2022
    PA3: Page-Replacement Algorithms

    Your name: Aditya Padia
    Your ITSC email:       apadia    @connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Constants
#define UNFILLED_FRAME -1
#define MAX_QUEUE_SIZE 10
#define MAX_FRAMES_AVAILABLE 10
#define MAX_REFERENCE_STRING 30
#define ALGORITHM_FIFO "FIFO"
#define ALGORITHM_OPT "OPT"
#define ALGORITHM_LRU "LRU"

// Keywords (to be used when parsing the input)
#define KEYWORD_ALGORITHM "algorithm"
#define KEYWORD_FRAMES_AVAILABLE "frames_available"
#define KEYWORD_REFERENCE_STRING_LENGTH "reference_string_length"
#define KEYWORD_REFERENCE_STRING "reference_string"

// Useful string template used in printf()
// We will use diff program to auto-grade the submissions
// Please use the following templates in printf to avoid formatting errors
//
// Example:
//
//   printf(template_total_page_fault, 0)    # Total Page Fault: 0 is printed on the screen
//   printf(template_no_page_fault, 0)       # 0: No Page Fault is printed on the screen

const char template_total_page_fault[] = "Total Page Fault: %d\n";
const char template_no_page_fault[] = "%d: No Page Fault\n";

// Assume that we only need to support 2 types of space characters:
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// Global variables
char algorithm[10];
int reference_string[MAX_REFERENCE_STRING];
int reference_string_length;
int frames_available;
int frames[MAX_FRAMES_AVAILABLE];

// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line)
{
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch))
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line)
{
    if (is_blank(line))
        return 1;
    char *ch = line;
    while (*ch != '\0')
    {
        if (!isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input()
{
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2];                                 // buffer for 2 tokens
    char *reference_string_tokens[MAX_REFERENCE_STRING]; // buffer for the reference string
    int numTokens = 0, n = 0, i = 0;
    char equal_plus_spaces_delimiters[5] = "";

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters, SPACE_CHARS);

    while ((nread = getline(&line, &len, fp)) != -1)
    {
        if (is_skip(line) == 0)
        {
            line = strtok(line, "\n");

            if (strstr(line, KEYWORD_ALGORITHM))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    strcpy(algorithm, two_tokens[1]);
                }
            }
            else if (strstr(line, KEYWORD_FRAMES_AVAILABLE))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &frames_available);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING_LENGTH))
            {
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2)
                {
                    sscanf(two_tokens[1], "%d", &reference_string_length);
                }
            }
            else if (strstr(line, KEYWORD_REFERENCE_STRING))
            {

                parse_tokens(two_tokens, line, &numTokens, "=");
                // printf("Debug: %s\n", two_tokens[1]);
                if (numTokens == 2)
                {
                    parse_tokens(reference_string_tokens, two_tokens[1], &n, SPACE_CHARS);
                    for (i = 0; i < n; i++)
                    {
                        sscanf(reference_string_tokens[i], "%d", &reference_string[i]);
                    }
                }
            }
        }
    }
}
// Helper: Display the parsed values
void print_parsed_values()
{
    int i;
    printf("%s = %s\n", KEYWORD_ALGORITHM, algorithm);
    printf("%s = %d\n", KEYWORD_FRAMES_AVAILABLE, frames_available);
    printf("%s = %d\n", KEYWORD_REFERENCE_STRING_LENGTH, reference_string_length);
    printf("%s = ", KEYWORD_REFERENCE_STRING);
    for (i = 0; i < reference_string_length; i++)
        printf("%d ", reference_string[i]);
    printf("\n");
}

// A simple integer queue implementation using a fixed-size array
// Helper functions:
//   queue_init: initialize the queue
//   queue_is_empty: return true if the queue is empty, otherwise false
//   queue_is_full: return true if the queue is full, otherwise false
//   queue_peek: return the current front element of the queue
//   queue_enqueue: insert one item at the end of the queue
//   queue_dequeue: remove one item from the beginning of the queue
//   queue_print: display the queue content, it is useful for debugging
struct Queue
{
    int values[MAX_QUEUE_SIZE];
    int front, rear, count;
};
void queue_init(struct Queue *q)
{
    q->count = 0;
    q->front = 0;
    q->rear = -1;
}
int queue_is_empty(struct Queue *q)
{
    return q->count == 0;
}
int queue_is_full(struct Queue *q)
{
    return q->count == MAX_QUEUE_SIZE;
}

int queue_peek(struct Queue *q)
{
    return q->values[q->front];
}
void queue_enqueue(struct Queue *q, int new_value)
{
    if (!queue_is_full(q))
    {
        if (q->rear == MAX_QUEUE_SIZE - 1)
            q->rear = -1;
        q->values[++q->rear] = new_value;
        q->count++;
    }
}
void queue_dequeue(struct Queue *q)
{
    q->front++;
    if (q->front == MAX_QUEUE_SIZE)
        q->front = 0;
    q->count--;
}
void queue_print(struct Queue *q)
{
    int c = q->count;
    printf("size = %d\n", c);
    int cur = q->front;
    printf("values = ");
    while (c > 0)
    {
        if (cur == MAX_QUEUE_SIZE)
            cur = 0;
        printf("%d ", q->values[cur]);
        cur++;
        c--;
    }
    printf("\n");
}

// Helper function:
// This function is useful for printing the fault frames in this format:
// current_frame: f0 f1 ...
//
// For example: the following 4 lines can use this helper function to print
//
// 7: 7
// 0: 7 0
// 1: 7 0 1
// 2: 2 0 1
//
// For the non-fault frames, you should use template_no_page_fault (see above)
//
void display_fault_frame(int current_frame)
{
    int j;
    printf("%d: ", current_frame);
    for (j = 0; j < frames_available; j++)
    {
        if (frames[j] != UNFILLED_FRAME)
            printf("%d ", frames[j]);
        else
            printf("  ");
    }
    printf("\n");
}

// Helper Function : Frame Search
// This function a frame (int) as an input and returns a integer
// Return value : 1 if the frame is present in frames
// Return value : 0 if the frame is not present in frames

int frame_search(int f)
{
    for (int i = 0; i < frames_available; i++)
    {
        if (frames[i] == f)
        {
            return 1;
            break;
        }
    }

    return 0;
}

//Helper Function : Frame Insert
// This function takes a frame (int) and inserts it in the first available position
void frame_insert(int f)
{
    for (int i = 0; i < frames_available; i++)
    {
        if (frames[i] == -1)
        {
            frames[i] = f;
            break;
        }
    }
}

// Helper Function : Frame Erase
// This function takes a frame (int) and removes it from frames
// Does not return any value

void frame_erase(int f)
{
    for (int i = 0; i < frames_available; i++)
    {
        if (frames[i] == f)
        {
            frames[i] = -1;
            break;
        }
    }
}

// Helper Function : Last Frame Used
// This function takes the current_index in the reference string
// This functon loops through the frame table and returns the index of the page that is used last
int last_frame_used(int current_index)
{
    int last_frame_index = 0;
    int curr_frame_index = 0;
    int lowest_frame = MAX_FRAMES_AVAILABLE + 1;
    int flag = 0; //Flag to check if there are no more instances of the frame
    int frame_tracker = -1; //Keeps track of the lowest frame in case of tie

    for (int frame = 0; frame < frames_available; frame++)
    {
        //Now we have each frame in the frame table
        for (int i = current_index; i <= reference_string_length; i++)
        {
            if (i == reference_string_length) //If no more future calls, flag and break
            {
                flag = 1;
                break;
            }
            if (reference_string[i] == frames[frame])
            {
                curr_frame_index = i;
                if (curr_frame_index > last_frame_index)
                {
                    last_frame_index = curr_frame_index;
                }
                break;
            }
        }
        if (flag == 1)
        {
            if (frames[frame] < lowest_frame)
            {
                lowest_frame = frames[frame];
                flag = 0; //Resestting flag for follwing iterations
                frame_tracker = frame; //Settting lowest frame number in frame_tracker
            }
            continue;
        }
    }

    if (frame_tracker >= 0)
    {
        return frame_tracker; //Return the lowest frame number in case of a tie
    }


    int frame = reference_string[last_frame_index];

    for (int i = 0; i < frames_available; i++)
    {
        if (frames[i] == frame)
        {
            return i;
        }
    }
}

// Helper Function : Lowest Frame Count
// This function takes the frame_count array (int []) as input
// The function loops through the frame and returns the least recently used frame
int lowest_frame_count(int frame_count_array[])
{
    int lowest_frame_count = reference_string_length + 1;
    int lowest_frame;
    
    for (int i = 0; i < frames_available; i++)
    {
        if (frame_count_array[frames[i]] < lowest_frame_count)
        {
            lowest_frame_count = frame_count_array[frames[i]];
            lowest_frame = frames[i];
        }
    }

    return lowest_frame;
}

// Helper Function : Print Frame Count Array
// This function takes the frame_count_array (int []) as input
// This function has no return value, it is for debugging
void print_frame_count_array(int frame_count_array[])
{
    for (int i = 0; i < MAX_FRAMES_AVAILABLE; i++)
    {
        printf("i : %d \t Frame Count : %d\n", i , frame_count_array[i]);
    }
}

void algorithm_FIFO()
{
    // TODO: Implement the FIFO algorithm here


    // Let's think about the algo bro
    // 1. Start traversing the pages
    //  i) if frameCount is less than framesAvailable
    //      a) Insert page into frames one by one until capacity is reached or all pages are processed
    //      b) Maintain pages order in FIFO queue
    //      c) Increment Page Fault, increment frameCount
    // ii) Else
    //      If current page is there in the frames : print no page fault (do nothing)
    //      Else
    //          a) Remove the first page of from the queue as it was first to enter memory
    //          b) Replace the first page in the queue with current page
    //          c) Store current page in the queue
    //          d) Increment Page Fault

    
    struct Queue q;
    queue_init(&q);

    int frameCount = 0; // Keep track of number of processes in the frame
    int pageFault = 0;

    for (int current_page = 0; current_page < reference_string_length; current_page++)
    {
        if (frameCount < frames_available)
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Don't need to do anything
                printf(template_no_page_fault, reference_string[current_page]);
                continue;
            }

            frame_insert(reference_string[current_page]);
            frameCount++;
            pageFault++;
            display_fault_frame(reference_string[current_page]);
            queue_enqueue(&q, reference_string[current_page]);
        }
        else
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Don't need to do anything
                printf(template_no_page_fault, reference_string[current_page]);
            }
            else
            {
                // Curent Page is not in frame
                // Replacement is necessary
                int val = queue_peek(&q);
                queue_dequeue(&q);
                frame_erase(val);
                frame_insert(reference_string[current_page]);
                queue_enqueue(&q, reference_string[current_page]);
                pageFault++;
                display_fault_frame(reference_string[current_page]);
            }
        }
    }

    printf(template_total_page_fault, pageFault);
}

void algorithm_OPT()
{
    // TODO: Implement the OPT algorithm here

    // Let's think about the algo bro
    // 1. Start traversing the pages
    //  i) if frameCount is less than framesAvailable
    //      a) Insert page into frames one by one until capacity is reached or all pages are processed
    //      b) Increment Page Fault, increment frameCount
    // ii) Else
    //      If current page is there in the frames : print no page fault (do nothing)
    //      Else
    //          a) Find the page that is not used for the longest period of time
    //          b) In case of conflict take the lower numeber frame
    //          c) Increment Page Fault

    int frameCount = 0; // Keep track of number of processes in the frame
    int pageFault = 0;

    for (int current_page = 0; current_page < reference_string_length; current_page++)
    {
        if (frameCount < frames_available)
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Don't need to do anything
                printf(template_no_page_fault, reference_string[current_page]);
                continue;
            }

            frame_insert(reference_string[current_page]);
            frameCount++;
            pageFault++;
            display_fault_frame(reference_string[current_page]);
        }
        else 
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Don't need to do anything
                printf(template_no_page_fault, reference_string[current_page]);
                continue;
            }
            else
            {
                // Current Page is not in frame
                // Replacement is Necessary
                // Algo : 
                //  1. Need to find the frame that is not used for the longest time
                //  2. Replace that page in the frame
                int index = last_frame_used(current_page);
                frames[index] = reference_string[current_page];
                frameCount++;
                pageFault++;
                display_fault_frame(reference_string[current_page]);
            }
        }
    }

    printf(template_total_page_fault, pageFault);

}

void algorithm_LRU()
{
    // TODO: Implement the LRU algorithm here

    // Let's think about the algo bro
    // 1. Start traversing the pages
    //  i) if frameCount is less than framesAvailable
    //      a) Insert page into frames one by one until capacity is reached or all pages are processed
    //      b) Increment Page Fault, increment frameCount
    // ii) Else
    //      If current page is there in the frames : print no page fault (do nothing)
    //      Else
    //          a) Find the frame in the array with the highest count
    //          b) In case of conflict take the lower numeber frame
    //          c) Increment Page Fault

    int frame_count_array[MAX_FRAMES_AVAILABLE] = {};

    for (int i = 0; i < MAX_FRAMES_AVAILABLE; i++)
    {
        frame_count_array[i] = MAX_FRAMES_AVAILABLE;
    }

    int frameCount = 0; // Keep track of number of processes in the frame
    int pageFault = 0;

    for (int current_page = 0; current_page < reference_string_length; current_page++)
    {
        if (frameCount < frames_available)
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Update count array
                printf(template_no_page_fault, reference_string[current_page]);
                frame_count_array[reference_string[current_page]] = current_page;
                continue;
            }

            frame_insert(reference_string[current_page]);
            frame_count_array[reference_string[current_page]] = current_page;
            // print_frame_count_array(frame_count_array);
            frameCount++;
            pageFault++;
            display_fault_frame(reference_string[current_page]);
        }
        else 
        {
            if (frame_search(reference_string[current_page]) == 1)
            {
                // Current Page is in Frame
                // Update count array
                printf(template_no_page_fault, reference_string[current_page]);
                frame_count_array[reference_string[current_page]] = current_page;
                // print_frame_count_array(frame_count_array);
                continue;
            }
            else
            {
                // Current Page is not in frame
                // Replacement is Necessary
                // Algo : 
                // 1. Traverse the frames
                // 2. Traverse the frame count array
                // 3. Find the frame that is the least recently used
                // 4. Replace the farme and update the values in the frame_count_table

                int lowest_frame = lowest_frame_count(frame_count_array);
                frame_erase(lowest_frame);
                frame_insert(reference_string[current_page]);
                frame_count_array[reference_string[current_page]] = current_page;
                frameCount++;
                pageFault++;
                display_fault_frame(reference_string[current_page]);
            }
        }
    }
    printf(template_total_page_fault, pageFault);
}

void initialize_frames()
{
    int i;
    for (i = 0; i < frames_available; i++)
        frames[i] = UNFILLED_FRAME;
}

int main()
{
    parse_input();
    print_parsed_values();
    initialize_frames();
    if (strcmp(algorithm, ALGORITHM_FIFO) == 0)
    {
        algorithm_FIFO();
    }
    else if (strcmp(algorithm, ALGORITHM_OPT) == 0)
    {
        algorithm_OPT();
    }
    else if (strcmp(algorithm, ALGORITHM_LRU) == 0)
    {
        algorithm_LRU();
    }
    return 0;
}