#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>

#include "wait.h"
#include "sem.h"

#define INGREDIENTS_NO 3
#define TOBACCO 1
#define MATCHES 2
#define PAPER 3

#define is_payment < 10
#define product_from_smoker(smoker) smoker + 10
#define get_basket(smoker) ctrl.basket[smoker-1]
#define next(smoker, n) (smoker - 1 + n) % INGREDIENTS_NO + 1

/**
 * Structure holding IDs of all IPC objects for
 * synchronisation of processes.
 */
typedef struct {
    /* ID of a shared memory segment, which holds prices of products. */
    int prices;
    /**
     * ID of a mutex used to control trade.
     * Either one of the smokers reads and uses prices for trade or the agent updates the stock prices.
     */
    int access;
    /**
     * An array of IDs of message queues serving as channels for trading products.
     * All smokers have their own baskets.
     */
    int basket[INGREDIENTS_NO + 1];
} Control;
/**
 * A global control structure, that will be accessible by all the processes.
 */
Control ctrl;

/**
 * Structure that describes form of a message
 * used in message queues used further in the application.
 */
typedef struct {
    long type;
    int value;
} Message;

int wallets[INGREDIENTS_NO + 1] = {0, 20, 20, 20};

/**
 * Attach a IPC System V's shared memory segment, so that it is accessible by a calling process.
 * @param memory_id – ID of the segment.
 * @return Pointer to the space in process's memory, from where the shared memory segment can be accessed.
 */
int *attach_memory(int memory_id) {
    return (int *) shmat(memory_id, NULL, 0);
}

/**
 * Get prices of products other than the one owned by the given smoker (caller of the function).
 * @param from pointer to the process's memory holding prices (previously attached shared memory segment); 
 * @param exclude_id ID (or role) of the smoker calling the function. 
 * @return Sum of prices of the rest of the products.
 */
int get_required_price(const int *from, int exclude_id) {
    int foo = from[next(exclude_id, 1)];
    int bar = from[next(exclude_id, 2)];
    return foo + bar;
}

/**
 * Agent functionality.
 * Role of the agent in this variant of the problem is to update the stock prices of all the products.
 * Agent works in an infinite loop, thus it does not return any value.
 * @param price pointer to agent process's memory that is linked to the shared memory segment holding the stock prices.
 */
_Noreturn void agent(int price[]) {
    srand(getpid());
    int agent_id = 0xA;
    printf("[%3x] Agent started...\n", agent_id);

    while (1) {
        lock(ctrl.access);
        for (int ingredient = 0; ingredient < INGREDIENTS_NO; ingredient++)
            price[ingredient + 1] = randomized(5, 15);
        printf("[%3x] Prices updated... Now they are: [%3i][%3i][%3i]\n", agent_id,
               price[TOBACCO],
               price[MATCHES],
               price[PAPER]
        );
        unlock(ctrl.access);
        randomised_sleep(agent_id);
    }
}

/**
 * Send payment via a message queue.
 * @param from identifier of the sending smoker;
 * @param to identifier of the receiving smoker (and their queue);
 * @param value the amount of money being send.
 * @return Status of the operation.
 */
int send_money(int from, int to, int value) {
    Message msg = {from, value};
    return msgsnd(ctrl.basket[to], &msg, sizeof(Message), 0);
}

/**
 * Send product via a message queue.
 * @param product_type type identifier of the product, linked to its producer;
 * @param to identifier of the receiving smoker (and their queue);
 * @return Status of the operation.
 */
int send_product(int product_type, int to) {
    Message msg = {(long) product_type, 1};
    return msgsnd(ctrl.basket[to], &msg, sizeof(Message), 0);
}

/**
 * Handle received payment by sending back product of the receiving smoker_id.
 * @param smoker_id identifier of the smoker_id, who received the payment;
 * @param payment message containing the payment (its value and sender).
 * @return Exit status of the sending operation.
 */
int handle_payment(const int smoker_id, const Message *payment) {
    printf("[%3i] payment answer handler, hello\n", smoker_id);
    wallets[smoker_id] += payment->value;
    printf("[%3i] updating my wallet, now it's %i...\n", smoker_id, wallets[smoker_id]);
    int product = product_from_smoker(smoker_id);
    int client = payment->type;
    return send_product(product, client);
}

/**
 * Handle all types of incoming payments.
 * @param smoker_id identifier of the smoker_id who is receiving communicates;
 * @param expected_products number of products that should be received by the smoker_id.
 */
void handle_receive(const int smoker_id, const int expected_products) {
    Message message;
    /* Identifier of smoker's own message queue. */
    int my_basket = ctrl.basket[smoker_id];
    /* Counter of how many products have been received_products, and thus how many should still be expected. */
    int received_products = 0;

    // perform non-blocking reads of smoker_id's message queue
    // respond to payments or collect products
    while (msgrcv(my_basket, &message, sizeof(Message), 0, IPC_NOWAIT) > 0)
        if (message.type is_payment) {
            if (handle_payment(smoker_id, &message) < 0)
                fprintf(stderr, "[%3x] smoker failed while receiving messages (non-blocking)", smoker_id);
        } else ++received_products;

    // perform blocking reads of the queue,
    // ensuring that the smoker has got all products they need
    while (received_products < expected_products) {
        msgrcv(my_basket, &message, sizeof(Message), 0, 0);
        if (message.type is_payment) {
            if (handle_payment(smoker_id, &message) < 0)
                fprintf(stderr, "[%3x] smoker failed while receiving messages (non-blocking)", smoker_id);
        } else ++received_products;
    }
}

/**
 * Smoker functionality.
 * All smokers share the code.
 * They are identified only by their IDs, which are also IDs of the product they products.
 * Smokers work in an infinite loop, thus they do not return any values.
 * @param id identifier of both the smoker and their product, which can be used to get all the other smoker's IDs.
 */
_Noreturn void smoker(const int id) {
    // initialise random numbers generation within each of the smokers' processes
    srand(getpid());

    /* Shared memory segment containing prices mapped into the smoker's process own memory. */
    int *prices = attach_memory(ctrl.prices);
    /**
     * No. of products this smoker will wait for.
     * This is either 0 when the smoker did not buy any products or 2 when they did.
     */
    int await_no;
    wallets[id] = randomized(15, 35);

    printf("[%3i] initialised with wallet: %i\n", id, wallets[id]);

    while (1) {
        randomised_sleep(id);
        lock(ctrl.access);
        int required_price = get_required_price(prices, id);
        if (required_price > wallets[id]) {
            await_no = 0;
            printf("[%3i] unable to pay %i! Had only %i\n", id, required_price, wallets[id]);
        } else {
            wallets[id] -= required_price;
            await_no = 2;

            // pay for product no. 1
            if (send_money(id, next(id, 1), prices[next(id, 1)]) < 0) {
                fprintf(stderr, "[%3x] smoker failed while sending money to %i", id, next(id, 1));
            }

            // pay for product no. 2
            if (send_money(id, next(id, 2), prices[next(id, 2)]) < 0) {
                fprintf(stderr, "[%3x] smoker failed while sending money to %i", id, next(id, 2));
            }

            printf("[%3i] payed... now my wallet is [%3i]\n", id, wallets[id]);
        }
        unlock(ctrl.access);

        handle_receive(id, await_no);
        printf("[%3i] finished... now my wallet is [%3i]... starting to smoke...\n", id, wallets[id]);
    }
}

int main(void) {
    ctrl.prices = shmget(0x10, (INGREDIENTS_NO + 1) * sizeof(int), IPC_CREAT | 0600);
    ctrl.access = semget(0x20, 1, IPC_CREAT | 0600);
    ctrl.basket[TOBACCO] = msgget(0x30 + TOBACCO, IPC_CREAT | 0600);
    ctrl.basket[MATCHES] = msgget(0x30 + MATCHES, IPC_CREAT | 0600);
    ctrl.basket[PAPER] = msgget(0x30 + PAPER, IPC_CREAT | 0600);

    // Initial prices of the products are set before the smokers start operating,
    // so that they don't read random values from the shared memory if any of them
    // happen to get through the semaphore before the agent does.
    int *prices = attach_memory(ctrl.prices);
    for (int ingredient = 0; ingredient < INGREDIENTS_NO; ingredient++)
        prices[ingredient + 1] = 10;

    seminit(ctrl.access, 1);

    if (fork() == 0) smoker(TOBACCO);
    if (fork() == 0) smoker(MATCHES);
    if (fork() == 0) smoker(PAPER);

    agent(prices);
}