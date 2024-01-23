/*
This code, written by SET, simulates a preemptive priority scheduler.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 512
// I designed instruction with only data field which is executionTime
struct instruction {
    int executionTime;
};

// There is process struct with different data fields.
struct process {
    struct instruction instructions[15];
    char name[2]; //name
    long arrivalTime; // time to arrival
    int priority; // priority
    int type; // 1 is SILVER, 2 is GOLD, 3 is PLATINUM
    int executionNumber; // executionNumber represents the number of time quantums that a process was in CPU.
    int completedInstructionNumber; // how many instruction process has done.
    int finishTime; // finish time
    int usedQuantum; // this represents the time slot that process used in current quantum
    bool isAction; // true if process came to CPU and in ready queue but did not finish. False if currentTime < arrivalTime
    bool isFinished; // true if process finished
};


// compare function for sorting by Name
int compareProcessesByName(const void *a, const void *b) {
    const struct process *processA = (const struct process *) a;
    const struct process *processB = (const struct process *) b;

    return strcmp(processA->name, processB->name);
}

// sorting function as names. I do not want the processes that will enter the round robin to be prioritized according to their names.
// That's why I didn't do this in my main sort function, but if they come at the same time, the one with the smallest name should be entered first,
// so I wrote this function.
void sortProcessesByName(struct process *arr, size_t size) {
    qsort(arr, size, sizeof(struct process), compareProcessesByName);
}

// main comparing function
int compareProcesses(const void *a, const void *b) {
    const struct process *processA = (const struct process *) a;
    const struct process *processB = (const struct process *) b;
    // if process is not active or it has finished earlier put them into end of the array
    if (processA->isAction == false || processA->isFinished == true) {
        if (processB->isFinished == true || processB->isAction == false) {
            return 0;
        }
        return 1; // Process B should come first
    } else if (processB->isAction == false || processB->isFinished == true) {
        return -1; // Process A should come first
    }

    if (processA->type == 3) {
        if (processB->type != 3) {
            return -1; // Process A should come first if A is platinum and B is not.
        } else {
            if ((processA->priority < processB->priority)) {
                return 1; // If both are platinum then B's priority is higher so B goes first.
            } else if ((processA->priority > processB->priority)) {
                return -1; // If both are platinum then A's priority is higher so A goes first.
            } else {
                if (processA->arrivalTime < processB->arrivalTime) {
                    return -1; // If both are platinum with equal priorities then first come first serves.
                } else if (processA->arrivalTime > processB->arrivalTime) {
                    return 1; // If both are platinum with equal priorities then first come first serves.
                } else {
                    return strcmp(processA->name, processB->name); // If arrivalTime is equal then we check names.
                }
            }
        }
    } else if (processB->type == 3) {
        return 1;
    }

    if (processA->priority < processB->priority) {
        return 1; // Process B should be first
    } else if (processA->priority > processB->priority) {
        return -1; // Process A should be first
    }

    return 0;
}


// thanks to C. qsort does everything for me to sort processes.
void sortProcesses(struct process *arr, size_t size) {
    qsort(arr, size, sizeof(struct process), compareProcesses);
}

// After sorting by name, there may be a problem like this:
// If process 1 comes first, 2 comes in 30ms, 4 comes in 20ms, 5 comes in 10 ms, and the priority of all of these processes is the same.
// Since the first instruction of Process 1 takes longer than 30ms, all remaining processes must be activated when this instruction is finished.
// Since they are activated from the beginning to the end of the array, process 2 was activated before process 4 and 5.
// This is something I don't want because 5 and 4 came before 2. I prevent this with this function.
void fixSortingIfArrivalTimeProblemOccurs(struct process *arr, size_t size) {
    for (int j = 0; j < size; j++) {
        for (int i = 0; i < size - 1; i++) {
            if (arr[i].priority == arr[i + 1].priority && arr[i].arrivalTime > arr[i +
                                                                                   1].arrivalTime) { // if priority is same and arrival time is greater I change them
                struct process temp = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = temp;
            }
        }
    }

}
// I need a flag in algorithm. This flag returns true if only one process is active.
bool isOnlyActive(struct process *arr, size_t size) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (arr[i].priority == arr[0].priority && arr[i].isAction == true) {
            count++;
        }
    }
    if (count > 1) {
        return false;
    } else {
        return true;
    }
}

// I shift my first element to end of the array and remains comes one step ahead
void shiftArrayLeft(struct process *arr, size_t size) {
    if (size <= 1) {
        return;
    }

    struct process temp = arr[0];

    for (size_t i = 0; i < size - 1; ++i) {
        arr[i] = arr[i + 1];
    }

    arr[size - 1] = temp;
}

// I shift element in the given index to the end. I need that function when there is a preemption because of PLAT or higher priority
void shiftProcessToEnd(struct process *arr, size_t size, size_t index) {
    if (index >= size) {
        printf("Invalid Index\n");
        return;
    }

    struct process temp = arr[index];

    for (size_t i = index; i < size - 1; ++i) {
        arr[i] = arr[i + 1];
    }

    arr[size - 1] = temp;
}

// I write that function to change type of process but then I realized I do not need. I did not use it.
struct process levelUpType(struct process process) {
    if (process.type == 1 && process.executionNumber >= 3) {
        process.type = 2;
        process.executionNumber -= 3;
    } else if (process.type == 2 && process.executionNumber >= 5) {
        process.type = 3;
        process.executionNumber -= 5;
    }
    return process;
}

// If all process have finished algorithm break the while loop and program should terminate itself.
bool checkAllFinished(const struct process *arr, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (!arr[i].isFinished) {
            return true;  // If at least one process has not finished yet, continue
        }
    }
    return false;  // If all processes have done stop
}

int main() {

/* *************************** INITIALIZATION PART **************************** */
// I know it is not absolutely a clean code but I wrote like this. I'm sorry for disturbing your eye health.
    struct instruction instr1;
    struct instruction instr2;
    struct instruction instr3;
    struct instruction instr4;
    struct instruction instr5;
    struct instruction instr6;
    struct instruction instr7;
    struct instruction instr8;
    struct instruction instr9;
    struct instruction instr10;
    struct instruction instr11;
    struct instruction instr12;
    struct instruction instr13;
    struct instruction instr14;
    struct instruction instr15;
    struct instruction instr16;
    struct instruction instr17;
    struct instruction instr18;
    struct instruction instr19;
    struct instruction instr20;
    struct instruction exit;
    struct instruction nullInstruction;
    instr1.executionTime = 90;
    instr2.executionTime = 80;
    instr3.executionTime = 70;
    instr4.executionTime = 60;
    instr5.executionTime = 50;
    instr6.executionTime = 40;
    instr7.executionTime = 30;
    instr8.executionTime = 20;
    instr9.executionTime = 30;
    instr10.executionTime = 40;
    instr11.executionTime = 50;
    instr12.executionTime = 60;
    instr13.executionTime = 70;
    instr14.executionTime = 80;
    instr15.executionTime = 90;
    instr16.executionTime = 80;
    instr17.executionTime = 70;
    instr18.executionTime = 60;
    instr19.executionTime = 50;
    instr20.executionTime = 40;
    exit.executionTime = 10;
    nullInstruction.executionTime = 0;

    struct process p1;
    struct process p2;
    struct process p3;
    struct process p4;
    struct process p5;
    struct process p6;
    struct process p7;
    struct process p8;
    struct process p9;
    struct process p10;

    p1.instructions[0] = instr1;
    p1.instructions[1] = instr19;
    p1.instructions[2] = instr15;
    p1.instructions[3] = instr18;
    p1.instructions[4] = instr3;
    p1.instructions[5] = instr2;
    p1.instructions[6] = instr20;
    p1.instructions[7] = instr15;
    p1.instructions[8] = instr18;
    p1.instructions[9] = instr3;
    p1.instructions[10] = instr2;
    p1.instructions[11] = exit;
    for (int i = 12; i < 15; i++) {
        p1.instructions[i] = nullInstruction;
    }
    p1.executionNumber = 0;
    p1.completedInstructionNumber = 0;
    p1.usedQuantum = 0;
    strcpy(p1.name, "P1");
    p1.isAction = false;
    p1.isFinished = false;

    p2.instructions[0] = instr18;
    p2.instructions[1] = instr2;
    p2.instructions[2] = instr5;
    p2.instructions[3] = instr6;
    p2.instructions[4] = instr5;
    p2.instructions[5] = instr6;
    p2.instructions[6] = instr5;
    p2.instructions[7] = instr6;
    p2.instructions[8] = exit;
    for (int i = 9; i < 15; i++) {
        p2.instructions[i] = nullInstruction;
    }
    p2.executionNumber = 0;
    p2.completedInstructionNumber = 0;
    p2.usedQuantum = 0;
    strcpy(p2.name, "P2");
    p2.isAction = false;
    p2.isFinished = false;

    p3.instructions[0] = instr8;
    p3.instructions[1] = instr7;
    p3.instructions[2] = instr12;
    p3.instructions[3] = instr11;
    p3.instructions[4] = instr13;
    p3.instructions[5] = instr16;
    p3.instructions[6] = instr19;
    p3.instructions[7] = instr8;
    p3.instructions[8] = instr7;
    p3.instructions[9] = exit;
    for (int i = 10; i < 15; i++) {
        p3.instructions[i] = nullInstruction;
    }
    p3.executionNumber = 0;
    p3.usedQuantum = 0;
    p3.completedInstructionNumber = 0;
    strcpy(p3.name, "P3");
    p3.isAction = false;
    p3.isFinished = false;

    p4.instructions[0] = instr9;
    p4.instructions[1] = instr2;
    p4.instructions[2] = instr19;
    p4.instructions[3] = instr9;
    p4.instructions[4] = instr2;
    p4.instructions[5] = exit;
    for (int i = 6; i < 15; i++) {
        p4.instructions[i] = nullInstruction;
    }
    p4.executionNumber = 0;
    p4.usedQuantum = 0;
    p4.completedInstructionNumber = 0;
    strcpy(p4.name, "P4");
    p4.isAction = false;
    p4.isFinished = false;

    p5.instructions[0] = instr9;
    p5.instructions[1] = instr2;
    p5.instructions[2] = instr19;
    p5.instructions[3] = instr9;
    p5.instructions[4] = instr2;
    p5.instructions[5] = instr2;
    p5.instructions[6] = instr19;
    p5.instructions[7] = instr9;
    p5.instructions[8] = instr2;
    p5.instructions[9] = instr19;
    p5.instructions[10] = exit;
    for (int i = 11; i < 15; i++) {
        p5.instructions[i] = nullInstruction;
    }
    p5.executionNumber = 0;
    p5.completedInstructionNumber = 0;
    p5.usedQuantum = 0;
    strcpy(p5.name, "P5");
    p5.isAction = false;
    p5.isFinished = false;

    p6.instructions[0] = instr10;
    p6.instructions[1] = instr9;
    p6.instructions[2] = instr20;
    p6.instructions[3] = instr11;
    p6.instructions[4] = instr4;
    p6.instructions[5] = instr5;
    p6.instructions[6] = instr7;
    p6.instructions[7] = instr10;
    p6.instructions[8] = instr9;
    p6.instructions[9] = instr20;
    p6.instructions[10] = instr11;
    p6.instructions[11] = instr4;
    p6.instructions[12] = instr5;
    p6.instructions[13] = instr7;
    p6.instructions[14] = exit;
    p6.executionNumber = 0;
    p6.completedInstructionNumber = 0;
    p6.usedQuantum = 0;
    strcpy(p6.name, "P6");
    p6.isAction = false;
    p6.isFinished = false;

    p7.instructions[0] = instr8;
    p7.instructions[1] = instr1;
    p7.instructions[2] = instr10;
    p7.instructions[3] = instr11;
    p7.instructions[4] = instr2;
    p7.instructions[5] = exit;
    for (int i = 6; i < 15; i++) {
        p7.instructions[i] = nullInstruction;
    }
    p7.executionNumber = 0;
    p7.usedQuantum = 0;
    p7.completedInstructionNumber = 0;
    strcpy(p7.name, "P7");
    p7.isAction = false;
    p7.isFinished = false;

    p8.instructions[0] = instr14;
    p8.instructions[1] = instr4;
    p8.instructions[2] = instr3;
    p8.instructions[3] = instr1;
    p8.instructions[4] = exit;
    for (int i = 5; i < 15; i++) {
        p8.instructions[i] = nullInstruction;
    }
    p8.executionNumber = 0;
    p8.completedInstructionNumber = 0;
    p8.usedQuantum = 0;
    strcpy(p8.name, "P8");
    p8.isAction = false;
    p8.isFinished = false;

    p9.instructions[0] = instr19;
    p9.instructions[1] = instr12;
    p9.instructions[2] = instr9;
    p9.instructions[3] = instr1;
    p9.instructions[4] = instr7;
    p9.instructions[5] = exit;
    for (int i = 6; i < 15; i++) {
        p9.instructions[i] = nullInstruction;
    }
    p9.executionNumber = 0;
    p9.completedInstructionNumber = 0;
    p9.usedQuantum = 0;
    strcpy(p9.name, "P9");
    p9.isAction = false;
    p9.isFinished = false;

    p10.instructions[0] = instr20;
    p10.instructions[1] = instr3;
    p10.instructions[2] = instr19;
    p10.instructions[3] = instr5;
    p10.instructions[4] = instr2;
    p10.instructions[5] = instr11;
    p10.instructions[6] = instr8;
    p10.instructions[7] = instr13;
    p10.instructions[8] = instr14;
    p10.instructions[9] = exit;
    for (int i = 10; i < 15; i++) {
        p10.instructions[i] = nullInstruction;
    }
    p10.executionNumber = 0;
    p10.completedInstructionNumber = 0;
    p10.usedQuantum = 0;
    strcpy(p10.name, "P10");
    p10.isAction = false;
    p10.isFinished = false;

/* *************************** CODE PART BEGINS HERE ************************** */


    // Here I read the file and initialize my array(actually, which is my queue)
    FILE *defFile;
    defFile = fopen("definition.txt", "r");
    char input[MAX_INPUT_SIZE];
    struct process processes[10];
    int processCount = 0;
    int currentTime = 0;
    while (fgets(input, sizeof(input), defFile) != NULL) {
        if (strcmp(input, "\n") == 0 || strcmp(input, "") == 0) {
            continue;
        }
        char *newInput;
        if (strncmp(input, "P1", 2) == 0) {
            processes[processCount] = p1;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P2", 2) == 0) {
            processes[processCount] = p2;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P3", 2) == 0) {
            processes[processCount] = p3;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P4", 2) == 0) {
            processes[processCount] = p4;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P5", 2) == 0) {
            processes[processCount] = p5;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P6", 2) == 0) {
            processes[processCount] = p6;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P7", 2) == 0) {
            processes[processCount] = p7;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P8", 2) == 0) {
            processes[processCount] = p8;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P9", 2) == 0) {
            processes[processCount] = p9;
            processCount++;
            newInput = input + 3;
        } else if (strncmp(input, "P10", 3) == 0) {
            processes[processCount] = p10;
            processCount++;
            newInput = input + 4;
        }

        char *token = strtok(newInput, " ");
        processes[processCount - 1].priority = atoi(token);

        token = strtok(NULL, " ");
        processes[processCount - 1].arrivalTime = strtol(token, NULL, 10);

        token = strtok(NULL, " ");
        if (strncmp(token, "GOLD", 4) == 0) {
            processes[processCount - 1].type = 2;
        } else if (strncmp(token, "SILVER", 6) == 0) {
            processes[processCount - 1].type = 1;
        } else if (strncmp(token, "PLATINUM", 8) == 0) {
            processes[processCount - 1].type = 3;
        }
    }


    // after the initialization of array I sort by name
    sortProcessesByName(processes, processCount);

    // As I mentioned earlier I check for a specific type error
    fixSortingIfArrivalTimeProblemOccurs(processes, processCount);


    // I use lastProcessOnCPU for checking context switches
    char lastProcessOnCPU[3] = "";

    // main while loop
    while (checkAllFinished(processes, processCount)) {
        sortProcesses(processes, processCount);
        bool flag = isOnlyActive(processes, processCount);

        // activating processes when their arrivalTime >= currentTime
        for (int i = 0; i < processCount; i++) {
            if (currentTime >= processes[i].arrivalTime && processes[i].isFinished != true) {
                processes[i].isAction = true;
            }
        }
        // I sort again because maybe newcomers are PLAT ore have higher priority
        sortProcesses(processes, processCount);

        //  There is no active process, however all processes have not finished.
        if (processes[0].isAction == false) {
            //printf("\n %d", currentTime);
            currentTime += 1;
            continue;
        }

        // Checking if there is a context switch
        if (strcmp(lastProcessOnCPU, processes[0].name) != 0) {
            currentTime += 10; // add cost of context switch

            int index = 0; // find previous one to check preemption
            for (int i = 1; i < processCount; i++) {
                if (strcmp(lastProcessOnCPU, processes[i].name) == 0) {
                    index = i;
                }
            }
            if (processes[index].type == 2 && index !=
                                              0) { // I check previous one for preemption, because I need to increment its total quantum number.
                if (processes[0].type == 3) {
                    processes[index].executionNumber++; // increment quantum count
                    if (processes[index].executionNumber >= 5) { //check if it becomes PLAT
                        processes[index].type = 3;
                    }
                    processes[index].usedQuantum = 0;
                    shiftProcessToEnd(processes, processCount, index); // because its turn is done, put it to end
                } else if (processes[index].priority < processes[0].priority) { // same
                    processes[index].executionNumber++;
                    if (processes[index].executionNumber >= 5) {
                        processes[index].type = 3;
                    }
                    processes[index].usedQuantum = 0;
                    shiftProcessToEnd(processes, processCount, index);
                }
            } else if (processes[index].type == 1 && index != 0) { // same but for silver processes
                if (processes[0].type == 3) {
                    processes[index].executionNumber++;
                    if (processes[index].executionNumber >= 3) {
                        processes[index].type = 2;
                        processes[index].executionNumber -= 3;
                    }
                    processes[index].usedQuantum = 0;
                    shiftProcessToEnd(processes, processCount, index);
                } else if (processes[index].priority < processes[0].priority) {
                    processes[index].executionNumber++;
                    if (processes[index].executionNumber >= 3) {
                        processes[index].type = 2;
                        processes[index].executionNumber -= 3;
                    }
                    processes[index].usedQuantum = 0;
                    shiftProcessToEnd(processes, processCount, index);
                }
            }
            strcpy(lastProcessOnCPU, processes[0].name);
        } else if (processes[0].completedInstructionNumber != 0 &&
                   processes[0].type != 3) { // here I need to check if quantum is done or not

            if (processes[0].type == 2 && processes[0].usedQuantum >= 120) { // gold and quantum done
                processes[0].executionNumber++; // increment quantum number
                processes[0].usedQuantum = 0;
                if (processes[0].executionNumber >= 5) {
                    processes[0].type = 3; // change gold one to PLATINUM
                }
                shiftArrayLeft(processes,
                               processCount); // put it end because there might be newcomer plat with higher priority
                continue;
            } else if (processes[0].type == 1 && processes[0].usedQuantum >= 80) { // same operations for silvers
                processes[0].executionNumber++;
                if (processes[0].executionNumber >= 3) {
                    processes[0].type = 2;
                    processes[0].executionNumber -= 3;
                }
                processes[0].usedQuantum = 0;
                if (flag == true) {
                    continue;
                }
                shiftArrayLeft(processes, processCount);
                continue;
            }
        }
        // execution part
        if (processes[0].type == 3) { // plat process
            for (int i = processes[0].completedInstructionNumber; i < 15; i++) { // there is no preemption for plats
                //printf("\n%d, %s, %d, %d", currentTime, processes[0].name, processes[0].instructions[i].executionTime, processes[0].type);
                currentTime += processes[0].instructions[i].executionTime;
            }
            processes[0].isFinished = true;
            processes[0].isAction = false;
            processes[0].finishTime = currentTime;
        } else if (processes[0].type == 2) { // gold
            //printf("\n%d, %s, %d, %d", currentTime, processes[0].name, processes[0].instructions[processes[0].completedInstructionNumber].executionTime, processes[0].type);
            currentTime += processes[0].instructions[processes[0].completedInstructionNumber].executionTime;
            processes[0].usedQuantum += processes[0].instructions[processes[0].completedInstructionNumber].executionTime;
            //processes[0].usedQuantum += processes[0].instructions[processes[0].completedInstructionNumber].executionTime;
            processes[0].completedInstructionNumber += 1;
            if (processes[0].instructions[processes[0].completedInstructionNumber - 1].executionTime ==
                10) { //if exit is executed
                processes[0].isFinished = true;
                processes[0].isAction = false;
                processes[0].finishTime = currentTime;
            }

            if (processes[0].executionNumber >= 5) {
                processes[0].type = 3;
                for (int i = processes[0].completedInstructionNumber; i < 15; i++) {
                    currentTime += processes[0].instructions[i].executionTime;
                    //printf("\n%d, %s, %d, %d", currentTime, processes[0].name, processes[0].instructions[i].executionTime, processes[0].type);
                }
                processes[0].isFinished = true;
                processes[0].isAction = false;
                processes[0].finishTime = currentTime;
            }
        } else { // silver
            //printf("\n%d, %s, %d, %d", currentTime, processes[0].name, processes[0].instructions[processes[0].completedInstructionNumber].executionTime, processes[0].type);
            currentTime += processes[0].instructions[processes[0].completedInstructionNumber].executionTime;
            processes[0].usedQuantum += processes[0].instructions[processes[0].completedInstructionNumber].executionTime;
            processes[0].completedInstructionNumber += 1;
            if (processes[0].instructions[processes[0].completedInstructionNumber - 1].executionTime ==
                10) { //if exit is executed
                processes[0].isFinished = true;
                processes[0].isAction = false;
                processes[0].finishTime = currentTime;
            }

        }

    }

    // when I reach here that means I executed all processes
    // rest is easy calculation part
    double totalWaitingTime = 0;
    double totalTurnaroundTime = 0;
    for (int i = 0; i < processCount; i++) {
        int burstTme = 0;
        for (int j = 0; j < 15; j++) {
            burstTme += processes[i].instructions[j].executionTime;
        }
        //printf("\n%d %d", burstTme, processes[i].finishTime);
        totalTurnaroundTime += processes[i].finishTime - processes[i].arrivalTime;
        totalWaitingTime += processes[i].finishTime - processes[i].arrivalTime - burstTme;
    }
    double averageWaiting = totalWaitingTime / processCount;
    double averageTurnaround = totalTurnaroundTime / processCount;

    // I print the console only 1 digit after point
    printf("%.1f\n", averageWaiting);
    printf("%.1f\n", averageTurnaround);

//    for (int i = 0; i < processCount; i++) {
//        //printf("\n%s, %d, %ld, %d, %ld, %d", processes[i].name ,processes[i].priority, processes[i].arrivalTime, processes[i].type,
//               //processes[i].arrivalTime, processes[i].finishTime);
//    }


    return 0;
}
