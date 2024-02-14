#include <cstdio>
#include <cmath>

// Type definitions
typedef short Q;
typedef short BYTES_USED_TYPE;
typedef int SHIFT_CAST_TYPE;

// Constants
const short UNUSED_QUEUE = -1;
const unsigned short DATA_ARRAY_SIZE = 2048;
const unsigned short MAX_QUEUES = 64;

const unsigned short BYTES_USED_INDEX = sizeof(Q) * MAX_QUEUES;
const unsigned short DATA_SHIFT_SIZE = sizeof(SHIFT_CAST_TYPE);
const unsigned short PADDING_SIZE = DATA_SHIFT_SIZE - 1;
const unsigned short DATA_START_OFFSET = BYTES_USED_INDEX + sizeof(BYTES_USED_TYPE);

// Shared data array for fifo queues
unsigned char SHARED_MEMORY[DATA_ARRAY_SIZE];

// Function prototypes

// Failure function for out of memory
void on_out_of_memory()
{
    std::printf("Out of memory!\n");
}

// Failure function for illegal operation
void on_illegal_operation()
{
    std::printf("Illegal operation!\n");
}

int get_num_copies(int a)
{
    return std::ceil((float)a / (float)DATA_SHIFT_SIZE);
}

void initialize_data()
{
    for (auto i = 0u; i < MAX_QUEUES; ++i)
    {
        *reinterpret_cast<Q *>(&SHARED_MEMORY[sizeof(Q) * i]) = UNUSED_QUEUE;
    }
    *reinterpret_cast<BYTES_USED_TYPE *>(SHARED_MEMORY + BYTES_USED_INDEX) = DATA_START_OFFSET;
}

// Function to create a new queue and return its handle
Q *create_queue()
{
    auto *bytes_used = reinterpret_cast<BYTES_USED_TYPE *>(SHARED_MEMORY + BYTES_USED_INDEX);
    if (*bytes_used >= DATA_ARRAY_SIZE - PADDING_SIZE - 2)
        on_out_of_memory();

    auto *q_size = reinterpret_cast<Q *>(SHARED_MEMORY);
    Q offset = DATA_START_OFFSET;

    for (auto i = 0u; i < MAX_QUEUES; ++i)
    {
        if (*q_size == UNUSED_QUEUE)
        {
            auto num_copies = get_num_copies(*bytes_used - offset);
            auto *src = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + *bytes_used - DATA_SHIFT_SIZE);
            auto *dst = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + *bytes_used - 1);

            for (auto i = 0; i < num_copies; i++)
            {
                *(dst--) = *(src--);
            }

            *q_size = PADDING_SIZE;
            *bytes_used += PADDING_SIZE;
            return q_size;
        }
        offset += *(q_size++);
    }
    on_illegal_operation();
    return nullptr;
}

// Function to destroy a queue
void destroy_queue(Q *q)
{
    auto *bytes_used = reinterpret_cast<BYTES_USED_TYPE *>(SHARED_MEMORY + BYTES_USED_INDEX);
    auto *q_size = reinterpret_cast<Q *>(SHARED_MEMORY);
    Q offset = DATA_START_OFFSET;

    if (!q || *q == UNUSED_QUEUE)
        on_illegal_operation();

    while (q_size != q)
    {
        if (*q_size > 0)
            offset += *q_size;
        q_size++;
    }

    auto num_copies = get_num_copies(*bytes_used - offset - *q);
    auto *src = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + offset + *q);
    auto *dst = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + offset);

    for (auto i = 0; i < num_copies; i++)
    {
        *(dst++) = *(src++);
    }

    *bytes_used -= *q;
    *q = UNUSED_QUEUE;
}

// Function to add a byte to a queue
void enqueue_byte(Q *q, unsigned char b)
{
    auto *bytes_used = reinterpret_cast<BYTES_USED_TYPE *>(SHARED_MEMORY + BYTES_USED_INDEX);
    auto *q_size = reinterpret_cast<Q *>(SHARED_MEMORY);
    Q offset = DATA_START_OFFSET;

    if (!q)
        on_illegal_operation();

    if (*bytes_used >= DATA_ARRAY_SIZE)
        on_out_of_memory();

    while (q_size != q)
    {
        if (*q_size > 0)
            offset += *q_size;
        q_size++;
    }
    offset += *q_size;

    auto num_copies = get_num_copies(*bytes_used - offset);
    auto *src = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + *bytes_used - DATA_SHIFT_SIZE);
    auto *dst = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + *bytes_used - (DATA_SHIFT_SIZE - 1));

    for (auto i = 0; i < num_copies; i++)
    {
        *(dst--) = *(src--);
    }

    SHARED_MEMORY[offset - PADDING_SIZE] = b;
    *bytes_used += 1;
    *q += 1;
}

// Function to remove and return a byte from a queue
unsigned char dequeue_byte(Q *q)
{
    auto *bytes_used = reinterpret_cast<BYTES_USED_TYPE *>(SHARED_MEMORY + BYTES_USED_INDEX);
    auto *q_size = reinterpret_cast<Q *>(SHARED_MEMORY);
    Q offset = DATA_START_OFFSET;

    if (!q || *q <= PADDING_SIZE)
        on_illegal_operation();

    while (q_size != q)
    {
        if (*q_size > 0)
            offset += *q_size;
        q_size++;
    }

    unsigned char ret_char = SHARED_MEMORY[offset];
    auto num_copies = get_num_copies(*bytes_used - offset - 1);
    auto *src = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + offset + 1);
    auto *dst = reinterpret_cast<SHIFT_CAST_TYPE *>(SHARED_MEMORY + offset);

    for (auto i = 0; i < num_copies; i++)
    {
        *(dst++) = *(src++);
    }

    *bytes_used -= 1;
    *q -= 1;
    return ret_char;
}

int main()
{
    initialize_data();

    // Example usage
    Q *q0 = create_queue();
    enqueue_byte(q0, 0);
    enqueue_byte(q0, 1);
    Q *q1 = create_queue();
    enqueue_byte(q1, 3);
    enqueue_byte(q0, 2);
    enqueue_byte(q1, 4);
    std::printf("%d ", dequeue_byte(q0));
    std::printf("%d\n", dequeue_byte(q0));

    enqueue_byte(q0, 5);
    enqueue_byte(q1, 6);
    std::printf("%d ", dequeue_byte(q0));
    std::printf("%d\n", dequeue_byte(q0));
    destroy_queue(q0);

    std::printf("%d ", dequeue_byte(q1));
    std::printf("%d ", dequeue_byte(q1));
    std::printf("%d\n", dequeue_byte(q1));
    destroy_queue(q1);

    return 0;
}
