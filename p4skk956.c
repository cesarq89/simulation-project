/******************************************************************************
cs2123p4.c by David Jeong
Purpose:
    Read from input file, goes through simulation, and prints each person at
    each event
Command Parameters:
    N/A
Input:
    int argc        default
    char* argv[]    default
Results:
    Prints the information of the events in the simulation, gives statistics
Returns:
    N/A
Notes:
    N/A
*******************************************************************************/


//Include files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
/*******************************************************************************
cs2123p4.h
Purpose:
    Defines constants and typedefs used in this file
Notes:
    Reference for data types
*******************************************************************************/
#include "cs2123p4.h"

/******************** main *****************************************************
int main()
Purpose:
    Creates a new simulation, runs the simulation, and frees the simulation.
Parameters:
    N/A
Notes:
    N/A
Return Value:
    0   default
*******************************************************************************/
int main(int argc, char* argv[])
{
    int bVerbose = 0;
    processCommandSwitches(argc, argv, &bVerbose);
    
    LinkedList list = newLinkedList();
    Simulation sim = (Simulation) malloc(sizeof(SimulationImp));    // new simulation func malloc linkedlist and set
    sim->bVerbose = bVerbose;
    sim->eventList = list;
    sim->lSystemTimeSum = 0;
    sim->lWidgetCount = 0;

    runSimulationC(sim);
    
    free(sim->eventList);
    free(sim);
    printf("Successfully terminated the simulation \n");
    
    return 0;
}

/******************** runSimulation ********************************************
void runSimulation()
Purpose:
    Goes through the linkedlist of the simulation, and prints each event's
    information.
Parameters:
    I/O Simulation sim      The simulation for arrival/departure
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
void runSimulationC(Simulation sim)
{
    sim->cRunType = 'A';
    if(sim->bVerbose)
        printf("%6s %6s %s \n", "Time", "Widget", "Event");
    
    Event event;
    Queue queueX = newQueue("Queue X");
    queueX->lQueueWaitSum = 0;
    queueX->lQueueWidgetTotalCount = 0;
    Queue queueW = newQueue("Queue W");
    queueW->lQueueWidgetTotalCount = 0;
    queueW->lQueueWaitSum = 0;
    
    Server serverX = (Server) malloc(sizeof(ServerImp));
    strcpy(serverX->szServerName, "Server X");
    serverX->lWidgetCount = 0;
    serverX->lServerTimeSum = 0;
    
    Server serverW = (Server) malloc(sizeof(ServerImp));
    strcpy(serverW->szServerName, "Server W");
    serverW->lWidgetCount = 0;
    serverW->lServerTimeSum = 0;
    
    generateArrival(sim, 0);
    while (removeLL(sim->eventList, &event))
    {
        sim->iClock = event.iTime;  // advance clock
        switch(event.iEventType)
        {
            case EVT_ARRIVAL:
                arrival(sim, &event);
                queueUp(sim, queueX, &event);
                seize(sim, queueX, serverX);
                break;
            
            case EVT_SERVERX_COMPLETE:
                release(sim, queueX, serverX, &event);
                queueUp(sim, queueW, &event);
                seize(sim, queueW, serverW);
                break;
                
            case EVT_SERVERW_COMPLETE:
                release(sim, queueW, serverW, &event);
                leaveSystem(sim, &event);
                break;
                
            default:
                ErrExit(ERR_ALGORITHM, "Unknown event type: %d\n", event.iEventType);
        }
    }
    
    printf("\nAverage Queue Time for Server X = %.1lf \n", (queueX->lQueueWaitSum / (double) sim->lWidgetCount));
    printf("Average Queue Time for Server W = %.1lf \n", (queueW->lQueueWaitSum / (double) sim->lWidgetCount));
    printf("Average Time in System = %.1lf \n", (sim->lSystemTimeSum / (double) sim->lWidgetCount));
    
    free(queueX);
    free(queueW);
    free(serverX);
    free(serverW);
}

/******************** generateArrival ******************************************
int generateArrival(Simulation sim, int iCurTime);
Purpose:
    Creates the first node to the simulation linked list, then inserts it into the
    list.
Parameters:
    I/O Simulation sim      The simulation for arrival/departure
    I   int iCurTime        Initial time
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int generateArrival(Simulation sim, int iCurTime)
{
    Event event;
    event.iEventType = EVT_ARRIVAL;
    event.iTime = iCurTime;
    
    insertOrderedLL(sim->eventList, event);
}

/******************** arrival *************************************************
int arrival(Simulation sim, Event event);
Purpose:
    Scans in data from stdin, sets widget arrival time, set up for next event
Parameters:
    Simulation sim          The simulation for arrival/departure
    Event *event            The current event that is popped out from LL
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int arrival(Simulation sim, Event *event)
{
    char szInputBuffer[MAX_LINE_SIZE + 1];
    int iScanfCount;
    int iArriveDelta;
    Event nextArrival;
    
    // Reading from file
    if(fgets(szInputBuffer, MAX_LINE_SIZE, stdin) == NULL)
        return 0;
            
    iScanfCount = sscanf(szInputBuffer, "%ld %d %d %d %d", &(event->widget.lWidgetNr), &(event->widget.iStep1tu), &(event->widget.iStep2tu), &iArriveDelta, &(event->widget.iWhichServer));
    if(iScanfCount < 5)
    {
        ErrExit(ERR_BAD_INPUT, "Bad Input");
    }
    
    sim->lWidgetCount += 1;
    
    // Set widget's arrival time to the current clock time
    event->widget.iArrivalTime = sim->iClock;
    
    // Next arrival event set up
    nextArrival.iEventType = EVT_ARRIVAL;
    nextArrival.iTime = sim->iClock + iArriveDelta;
    nextArrival.widget.lWidgetNr = '\0';
    insertOrderedLL(sim->eventList, nextArrival);
    
    if(sim->bVerbose)
        printf("%6i %6ld %s \n", sim->iClock, event->widget.lWidgetNr, "Arrived");
}

/******************** queueUp *************************************************
int queueUp(Simulation sim, Queue queueTeller, Event *event);
Purpose:
    Queues up the widget for the next seizing of a server
Parameters:
    Simulation sim          The simulation for arrival/departure
    Queue queueTeller       The current queue that needs to be altered
    Event *event            The current event that is popped out from LL
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int queueUp(Simulation sim, Queue queueTeller, Event *event)
{
    if(event->widget.lWidgetNr == 0)
        return 0;
    // Set up an element
    QElement newElement;
    newElement.iEnterQTime = sim->iClock;
    newElement.widget = event->widget;
    
    // Insert a node into queue
    insertQ(queueTeller, newElement);
    
    queueTeller->lQueueWidgetTotalCount += 1;
    
    if(sim->bVerbose)
        printf("%6i %6ld Enter %s \n", sim->iClock, newElement.widget.lWidgetNr, queueTeller->szQName);
}

/******************** seize ***************************************************
int seize(Simulation sim, Queue queueTeller, Server serverTeller);
Purpose:
    Checks if a server is busy, and if not, seizes the server
Parameters:
    Simulation sim          The simulation for arrival/departure
    Queue queueTeller       The current queue that needs to be altered
    Server serverTeller     The current server that needs to be seized
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int seize(Simulation sim, Queue queueTeller, Server serverTeller)
{
    QElement curElement;
    Event completeEvent;
    
    // Check if the server is already busy
    if(serverTeller->bBusy)
        return 0;
    
    // Check for empty queue
    if(!removeQ(queueTeller, &curElement))
        return 0;
        
    queueTeller->lQueueWaitSum += (sim->iClock - curElement.iEnterQTime);
    
    if(strcmp(queueTeller->szQName, "Queue X") == 0)
    {
        completeEvent.iEventType = EVT_SERVERX_COMPLETE;
        completeEvent.iTime = sim->iClock + curElement.widget.iStep1tu;
    }
    if(strcmp(queueTeller->szQName, "Queue W") == 0)
    {
        completeEvent.iEventType = EVT_SERVERW_COMPLETE;
        completeEvent.iTime = sim->iClock + curElement.widget.iStep2tu;
    }
    completeEvent.widget = curElement.widget;
    insertOrderedLL(sim->eventList, completeEvent);

    serverTeller->lWidgetCount += 1;
    serverTeller->bBusy = TRUE;
    serverTeller->lLastServerBeginTime = sim->iClock;
    
    if(sim->bVerbose)
    {
        printf("%6i %6ld Leave %s, waited %2i \n", sim->iClock, curElement.widget.lWidgetNr, queueTeller->szQName, (sim->iClock - curElement.iEnterQTime));
        printf("%6i %6ld Seized %s \n", sim->iClock, curElement.widget.lWidgetNr, serverTeller->szServerName);
    }
}

/******************** release **************************************************
int release(Simulation sim, Queue queueTeller, Server serverTeller, Event *event);
Purpose:
    
Parameters:
    Simulation sim          The simulation for arrival/departure
    Queue queueTeller       The current queue that needs to be altered
    Server serverTeller     The current server that needs to be seized
    Event *event            The current event that is popped out from LL
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int release(Simulation sim, Queue queueTeller, Server serverTeller, Event *event)
{
    if(!serverTeller->bBusy)
        return 0;
        
    if(event->widget.lWidgetNr == 0)
        return 0;
        
    serverTeller->lServerTimeSum += (sim->iClock - serverTeller->lLastServerBeginTime);
    serverTeller->bBusy = FALSE;
    
    if(sim->bVerbose)
        printf("%6i %6ld Released %s \n", sim->iClock, event->widget.lWidgetNr, serverTeller->szServerName);

    seize(sim, queueTeller, serverTeller);
}

/******************** leaveSystem **********************************************
int leaveSystem(Simulation sim, Event *event);
Purpose:
    Calculates the total system time
Parameters:
    Simulation sim          The simulation for arrival/departure
    Event *event            The current event that is popped out from LL
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int leaveSystem(Simulation sim, Event *event)
{
    
    if(event->widget.lWidgetNr == 0)
        return 0;
        
    sim->lSystemTimeSum += (sim->iClock - event->widget.iArrivalTime);
    
    if(sim->bVerbose)
        printf("%6i %6ld Exit %s, in system %2i \n", sim->iClock, event->widget.lWidgetNr, "System", (sim->iClock - event->widget.iArrivalTime));
}

/******************** removeQ *************************************************
int removeQ(Queue queue, QElement *pFromQElement);
Purpose:
    Removes a node from a queue
Parameters:
    Queue queue             The current queue that needs to be altered
    QElement *pFromQElement Qelement pointer that needs to be removed and returned
                            by parameter
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int removeQ(Queue q, QElement *pFromQElement)
{
    NodeQ *p;
    // check for empty
    if (q->pHead == NULL)
        return FALSE;
    p = q->pHead;
    *pFromQElement = p->element;
    q->pHead = p->pNext;
    // Removing the node could make the list empty.
    // See if we need to update pFoot, due to empty list
    if(q->pHead == NULL)
        q->pFoot = NULL;
    free(p);
    return TRUE;
}

/******************** insertQ **************************************************
void  insertQ(Queue queue, QElement element);
Purpose:
    Inserts an element into a queue
Parameters:
    Queue queue             The current queue that needs to be altered
    QElement element        The element that is being inserted into the queue
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
void  insertQ(Queue q, QElement element)
{
    NodeQ *pNew;
    pNew = allocNodeQ(q, element);
    // check for empty
    if(q->pHead == NULL)
    {
        // Insert at the head
        q->pHead = pNew;
        q->pFoot = pNew;
    }

    else
    {
        q->pFoot->pNext = pNew;
        q->pFoot = pNew;
    }

}

/******************** allocNodeQ ***********************************************
NodeQ *allocNodeQ(Queue queue, QElement value);
Purpose:
    Allocates a node for possible insertion into a queue
Parameters:
    Queue queue             The current queue that needs to be altered
    QElement value          The element value that is assigned to the new node
Notes:
    N/A
Return Value:
    ??
*******************************************************************************/
NodeQ *allocNodeQ(Queue queue, QElement value)
{
    NodeQ *pNew;
    pNew = (NodeQ *)malloc(sizeof(NodeQ));
    if (pNew == NULL)
        ErrExit(ERR_ALGORITHM, "No available memory for queue");
    pNew->element = value;
    pNew->pNext = NULL;
    return pNew;
}

/******************** newQueue *************************************************
Queue newQueue(char szQueueNm[]);
Purpose:
    Creates a new queue
Parameters:
    char szQueueNm[]            The name of the queue
Notes:
    N/A
Return Value:
    q                           The newly created queue
*******************************************************************************/
Queue newQueue(char szQueueNm[])
{
    Queue q = (Queue) malloc(sizeof(QueueImp));
    // Mark the list as empty
    q->pHead = NULL;   // empty list
    q->pFoot = NULL;   // empty list
    strcpy(q->szQName, szQueueNm);
    return q;
}

/******************** removeLL ************************************************
int removeLL(LinkedList list, Event  *pValue);
Purpose:
    Removes a node and return by reference
Parameters:
    I/O LinkedList list     List passed in to remove the node from
    I/O Event *pValue       Event that will be removed form the list
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
int removeLL(LinkedList list, Event  *pValue)
{
    NodeLL *p;
    
    if (list->pHead == NULL)
        return FALSE;
        
    *pValue = list->pHead->event;
    p = list->pHead;
    list->pHead = list->pHead->pNext;
    free(p);
    
    return TRUE;
}

/******************** insertOrderedLL ******************************************
NodeLL *insertOrderedLL(LinkedList list, Event value);
Purpose:
    Inserts a node in to the appropriate place within the list
Parameters:
    I/O LinkedList list     List passed in to remove the node from
    I/O Event *pValue       Event that will be removed form the list
Notes:
    N/A
Return Value:
    pNew    The node that was inserted
*******************************************************************************/
NodeLL *insertOrderedLL(LinkedList list, Event value)
{
    NodeLL *pNew, *pFind, *pPrecedes;
    
    // see if it already exists
    // Notice that we are passing the address of pPrecedes so that 
    // searchLL can change it.
    pFind = searchLL(list, value.iTime, &pPrecedes);

    // doesn't already exist.  Allocate a node and insert.
    pNew = allocateNodeLL(list, value);

    // Check for inserting at the beginning of the list
    // this will also handle when the list is empty
    if (pPrecedes == NULL)
    {
        pNew->pNext = list->pHead;
        list->pHead = pNew;
    }
    else
    {
        pNew->pNext = pPrecedes->pNext;
        pPrecedes->pNext = pNew;
    }
    return pNew;

}

/******************** searchLL *************************************************
NodeLL *searchLL(LinkedList list, int match, NodeLL **ppPrecedes);
Purpose:
    Searches for the appropriate place the node belongs to
Parameters:
    I/O LinkedList list     List passed in to remove the node from
    I   int match           Value that it is looking for
    I/O NodeLL **ppPrecedes The precedes node that is passed in/out
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
NodeLL *searchLL(LinkedList list, int match, NodeLL **ppPrecedes)
{
    NodeLL *p;
    
    // used when the list is empty or we need to insert at the beginning
    *ppPrecedes = NULL;

    // Traverse through the list looking for where the key belongs or
    // the end of the list.
    for (p = list->pHead; p != NULL; p = p->pNext)
    {
        if (match == p->event.iTime)
            return p;
        if (match < p->event.iTime)
            return NULL;
        *ppPrecedes = p;
    }

    // Not found return NULL
    return NULL;
}

/******************** newLinkedList *******************************************
LinkedList newLinkedList();
Purpose:
    Create a new Linked List
Parameters:
    N/A
Notes:
    N/A
Return Value:
    LinkedList list     A newly created linked list
*******************************************************************************/
LinkedList newLinkedList()
{
    LinkedList list = (LinkedList) malloc(sizeof(LinkedListImp));
    list->pHead = NULL;
    return list;
}

/******************** allocateNodeLL *******************************************
NodeLL *allocateNodeLL(LinkedList list, Event value);
Purpose:
    Allocates a node
Parameters:
    I/O LinkedList list     List passed in to remove the node from
    I/O Event value         Event that will be removed form the list
Notes:
    N/A
Return Value:
    N/A
*******************************************************************************/
NodeLL *allocateNodeLL(LinkedList list, Event value)
{
    NodeLL *pNew;
    pNew = (NodeLL *) malloc(sizeof(NodeLL));
    if (pNew == NULL)
        ErrExit(ERR_ALGORITHM, "No available memory for linked list");
    pNew->event = value;
    pNew->pNext = NULL;
    return pNew;
}

/******************** processCommandSwitches *****************************
void processCommandSwitches(int argc, char *argv[], char **ppszCourseFileName
        , char **ppszQueryFileName)
Purpose:
    Checks the syntax of command line arguments and returns the filenames.  
    If any switches are unknown, it exits with an error.
Parameters:
    I   int argc                        count of command line arguments
    I   char *argv[]                    array of command line arguments
    O   char **ppszCourseFileName     Course File Name to return
    O   char **ppszQueryFileName        Query File Name to return 
Notes:
    If a -? switch is passed, the usage is printed and the program exits
    with USAGE_ONLY.
    If a syntax error is encountered (e.g., unknown switch), the program
    prints a message to stderr and exits with ERR_COMMAND_LINE_SYNTAX.
**************************************************************************/
void processCommandSwitches(int argc, char *argv[], int *bVerbose)
{
    int i;
    // Examine each of the command arguments other than the name of the program.
    for (i = 1; i < argc; i++)
    {
        // check for a switch
        if (argv[i][0] != '-')
            continue;
        // determine which switch it is
        switch (argv[i][1])
        {
        case 'v':                   // bVerbose
            if (++i >= argc)
            *bVerbose = TRUE;
            break;
        case '?':
            exitUsage(USAGE_ONLY, "", "");
            break;
        default:
            exitUsage(i, ERR_EXPECTED_SWITCH, argv[i]);
        }
    }
}

/******************** getToken **************************************
char * getToken (char *pszInputTxt, char szToken[], int iTokenSize)
Purpose:
    Examines the input text to return the next token.  It also
    returns the position in the text after that token.  This function
    does not skip over white space, but it assumes the input uses 
    spaces to separate tokens.
Parameters:
    I   char *pszInputTxt       input buffer to be parsed
    O   char szToken[]          Returned token.
    I   int iTokenSize          The size of the token variable.  This is used
                                to prevent overwriting memory.  The size
                                should be the memory size minus 1 (for
                                the zero byte).
Returns:
    Functionally:
        Pointer to the next character following the delimiter after the token.
        NULL - no token found.
    szToken parm - the returned token.  If not found, it will be an
        empty string.
Notes:
    - If the token is larger than iTokenSize, we return a truncated value.
    - If a token isn't found, szToken is set to an empty string
    - This function does not skip over white space occurring prior to the token.
**************************************************************************/
char * getToken(char *pszInputTxt, char szToken[], int iTokenSize)
{
    int iDelimPos;                      // found position of delim
    int iCopy;                          // number of characters to copy
    char szDelims[20] = " \n\r";        // delimiters
    szToken[0] = '\0';

    // check for NULL pointer 
    if (pszInputTxt == NULL)
        ErrExit(ERR_ALGORITHM
        , "getToken passed a NULL pointer");

    // Check for no token if at zero byte
    if (*pszInputTxt == '\0')
        return NULL;

    // get the position of the first delim
    iDelimPos = strcspn(pszInputTxt, szDelims);

    // if the delim position is at the first character, return no token.
    if (iDelimPos == 0)
        return NULL;

    // see if we have more characters than target token, if so, trunc
    if (iDelimPos > iTokenSize)
        iCopy = iTokenSize;             // truncated size
    else
        iCopy = iDelimPos;

    // copy the token into the target token variable
    memcpy(szToken, pszInputTxt, iCopy);
    szToken[iCopy] = '\0';              // null terminate

    // advance the position
    pszInputTxt += iDelimPos;
    if (*pszInputTxt == '\0')
        return pszInputTxt;
    else
        return pszInputTxt + 1;
}

/******************** ErrExit **************************************
  void ErrExit(int iexitRC, char szFmt[], ... )
Purpose:
    Prints an error message defined by the printf-like szFmt and the
    corresponding arguments to that function.  The number of 
    arguments after szFmt varies dependent on the format codes in
    szFmt.  
    It also exits the program with the specified exit return code.
Parameters:
    I   int iexitRC             Exit return code for the program
    I   char szFmt[]            This contains the message to be printed
                                and format codes (just like printf) for 
                                values that we want to print.
    I   ...                     A variable-number of additional arguments
                                which correspond to what is needed
                                by the format codes in szFmt. 
Returns:
    Returns a program exit return code:  the value of iexitRC.
Notes:
    - Prints "ERROR: " followed by the formatted error message specified 
      in szFmt. 
    - Prints the file path and file name of the program having the error.
      This is the file that contains this routine.
    - Requires including <stdarg.h>
**************************************************************************/
void ErrExit(int iexitRC, char szFmt[], ... )
{
    va_list args;               // This is the standard C variable argument list type
    va_start(args, szFmt);      // This tells the compiler where the variable arguments
                                // begins.  They begin after szFmt.
    printf("ERROR: ");
    vprintf(szFmt, args);       // vprintf receives a printf format string and  a
                                // va_list argument
    va_end(args);               // let the C environment know we are finished with the
                                // va_list argument
    printf("\n\tEncountered in file %s\n", __FILE__);  // this 2nd arg is filled in by
                                // the pre-compiler
    exit(iexitRC);
}

/******************** exitUsage *****************************
    void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
Purpose:
    In general, this routine optionally prints error messages and diagnostics.
    It also prints usage information.

    If this is an argument error (iArg >= 0), it prints a formatted message 
    showing which argument was in error, the specified message, and
    supplemental diagnostic information.  It also shows the usage. It exits 
    with ERR_COMMAND_LINE.

    If this is a usage error (but not specific to the argument), it prints 
    the specific message and its supplemental diagnostic information.  It 
    also shows the usage and exist with ERR_COMMAND_LINE. 

    If this is just asking for usage (iArg will be -1), the usage is shown.
    It exits with USAGE_ONLY.
Parameters:
    I int iArg                      command argument subscript or control:
                                    > 0 - command argument subscript
                                    0 - USAGE_ONLY - show usage only
                                    -1 - USAGE_ERR - show message and usage
    I char *pszMessage              error message to print
    I char *pszDiagnosticInfo       supplemental diagnostic information
Notes:
    This routine causes the program to exit.
**************************************************************************/
void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
{
    switch (iArg)
    {
        case USAGE_ERR:
            fprintf(stderr, "Error: %s %s\n"
                , pszMessage
                , pszDiagnosticInfo);
            break;
        case USAGE_ONLY:
            break;
        default:
            fprintf(stderr, "Error: bad argument #%d.  %s %s\n"
                , iArg
                , pszMessage
                , pszDiagnosticInfo);
    }
    // print the usage information for any type of command line error
    fprintf(stderr, "p2 -c courseFileName -q queryFileName\n");
    if (iArg == USAGE_ONLY)
        exit(USAGE_ONLY); 
    else 
        exit(ERR_COMMAND_LINE);
}
