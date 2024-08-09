#include <iostream>
#include <string>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>

// Structure to represent a to-do item
struct ToDoItem {
    char description[256];
    bool completed;
};

// Shared memory segment structure
struct SharedMemory {
    int numItems;
    ToDoItem items[10]; // Fixed-size array for simplicity
    sem_t sem; // Semaphore for synchronization
};

// Function to add a new to-do item to the shared memory segment
void addToDoItem(SharedMemory* shmPtr, const char* description) {
    sem_wait(&shmPtr->sem); // Acquire the semaphore
    strcpy(shmPtr->items[shmPtr->numItems].description, description);
    shmPtr->items[shmPtr->numItems].completed = false;
    shmPtr->numItems++;
    sem_post(&shmPtr->sem); // Release the semaphore
}

// Function to complete a to-do item
void completeToDoItem(SharedMemory* shmPtr, int index) {
    sem_wait(&shmPtr->sem); // Acquire the semaphore
    if (index < shmPtr->numItems) {
        shmPtr->items[index].completed = true;
    }
    sem_post(&shmPtr->sem); // Release the semaphore
}

// Function to print the to-do list
void printToDoList(SharedMemory* shmPtr) {
    sem_wait(&shmPtr->sem); // Acquire the semaphore
    std::cout << "To-Do List:" << std::endl;
    for (int i = 0; i < shmPtr->numItems; i++) {
        std::cout << i + 1 << ". " << shmPtr->items[i].description 
                  << " [" << (shmPtr->items[i].completed ? "Completed" : "Pending") << "]" << std::endl;
    }
    sem_post(&shmPtr->sem); // Release the semaphore
}

int main() {
    // Create shared memory segment
    int shmId = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmId == -1) {
        perror("shmget");
        return 1;
    }

    // Attach to the shared memory segment
    SharedMemory* shmPtr = (SharedMemory*) shmat(shmId, NULL, 0);
    if (shmPtr == (void*) -1) {
        perror("shmat");
        return 1;
    }

    // Initialize shared memory
    shmPtr->numItems = 0;
    sem_init(&shmPtr->sem, 1, 1); // Initialize semaphore for shared memory

    // User interaction loop
    while (true) {
        std::string command;
        std::cout << "Enter command (add, complete, view, exit): ";
        std::cin >> command;

        if (command == "add") {
            std::string description;
            std::cout << "Enter description: ";
            std::cin.ignore();
            std::getline(std::cin, description);
            addToDoItem(shmPtr, description.c_str());
        } else if (command == "complete") {
            int index;
            std::cout << "Enter item number to complete: ";
            std::cin >> index;
            completeToDoItem(shmPtr, index - 1);
            } else if (command == "view") {
            printToDoList(shmPtr);
        } else if (command == "exit") {
            break;
        } else {
            std::cout << "Invalid command." << std::endl;
        }
    }

    // Clean up
    sem_destroy(&shmPtr->sem);
    shmdt(shmPtr);
    shmctl(shmId, IPC_RMID, 0);

    return 0;
}
