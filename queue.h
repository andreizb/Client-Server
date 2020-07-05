struct QNode { 
    void *key; 
    struct QNode* next; 
}; 

struct Queue { 
    struct QNode *front, *rear; 
};

struct Queue* createQueue();
void enQueue(struct Queue* q, void *k);
void deQueue(struct Queue* q);
