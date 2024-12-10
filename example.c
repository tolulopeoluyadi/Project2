#define _GNU_SOURCE // Ensure GNU extensions for mmap and other features
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

#ifdef __APPLE__ // macOS uses MAP_ANON instead of MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define SEM_NAME "/sem_bankaccount"
#define BANK_ACCOUNT_INIT 100 // Initial balance

// Shared memory and semaphore
int *BankAccount;
sem_t *sem_bank;

// Function prototypes
void dear_old_dad();
void lovable_mom();
void poor_student();
void cleanup();
void handle_signal(int signal);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_parents> <num_children>\n", argv[0]);
        exit(1);
    }

    int num_parents = atoi(argv[1]);
    int num_children = atoi(argv[2]);

    if (num_parents < 1 || num_children < 1) {
        printf("Error: Number of parents and children must be at least 1.\n");
        exit(1);
    }

    srand(time(NULL)); // Seed random number generator

    // Set up shared memory for BankAccount
    BankAccount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (BankAccount == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }
    *BankAccount = BANK_ACCOUNT_INIT; // Initialize balance

    // Set up semaphore for BankAccount
    sem_bank = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem_bank == SEM_FAILED) {
        perror("sem_open failed");
        munmap(BankAccount, sizeof(int));
        exit(1);
    }

    // Handle Ctrl+C for cleanup
    signal(SIGINT, handle_signal);

    // Fork processes for parents
    for (int i = 0; i < num_parents; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            cleanup();
            exit(1);
        }

        if (pid == 0) { // Child process
            if (i == 0) {
                dear_old_dad(); // Parent 1: Dear Old Dad
            } else {
                lovable_mom();  // Parent 2+: Lovable Mom
            }
            exit(0); // Exit child process after running parent logic
        }
    }

    // Fork processes for children
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            cleanup();
            exit(1);
        }

        if (pid == 0) {
            poor_student(); // Each child runs this
            exit(0); // Child process exits
        }
    }

    // Parent process waits for all children to finish
    while (wait(NULL) > 0);
    cleanup();

    return 0;
}

void dear_old_dad() {
    while (1) {
        sleep(rand() % 6); // Sleep for 0-5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(sem_bank); // Enter critical section
        int localBalance = *BankAccount;

        int random_number = rand();
        if (random_number % 2 == 0) { // Even number
            if (localBalance < 100) {
                int deposit = rand() % 101; // Random amount 0-100
                *BankAccount += deposit;
                printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", deposit, *BankAccount);
            } else {
                printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else { // Odd number
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(sem_bank); // Exit critical section
    }
}

void lovable_mom() {
    while (1) {
        sleep(rand() % 11); // Sleep for 0-10 seconds
        printf("Lovable Mom: Attempting to Check Balance\n");

        sem_wait(sem_bank); // Enter critical section
        int localBalance = *BankAccount;

        if (localBalance <= 100) {
            int deposit = rand() % 126; // Random amount 0-125
            *BankAccount += deposit;
            printf("Lovable Mom: Deposits $%d / Balance = $%d\n", deposit, *BankAccount);
        } else {
            printf("Lovable Mom: Thinks the balance is sufficient ($%d)\n", localBalance);
        }
        sem_post(sem_bank); // Exit critical section
    }
}

void poor_student() {
    while (1) {
        sleep(rand() % 6); // Sleep for 0-5 seconds
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(sem_bank); // Enter critical section
        int localBalance = *BankAccount;

        int random_number = rand();
        if (random_number % 2 == 0) { // Even number
            int need = rand() % 51; // Random amount 0-50
            printf("Poor Student needs $%d\n", need);
            if (need <= localBalance) {
                *BankAccount -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, *BankAccount);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
        } else { // Odd number
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }
        sem_post(sem_bank); // Exit critical section
    }
}

void cleanup() {
    // Clean up shared memory and semaphore
    munmap(BankAccount, sizeof(int));
    sem_close(sem_bank);
    sem_unlink(SEM_NAME);
    printf("Cleaned up resources. Exiting...\n");
    exit(0);
}

void handle_signal(int signal) {
    if (signal == SIGINT) {
        cleanup();
    }
}
