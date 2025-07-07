#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define TGC_CUSTOM_HASH
size_t tgc_hash(void *ptr) {
    const uintptr_t ad = (uintptr_t) ptr;
    return (size_t) (ad >> 12);
}

#include "../tgc.h"
#include "../tgc.c"

const int entity_count = 1024;
const int test_count = 300;

typedef struct p64_struct {
    struct p64_struct* values[8];
} p64_struct;

tgc_t gc;
p64_struct** ptrs = NULL;
int balance = 0;
int balance_expected = 0;

void dtor(void *){
    balance--;
}

#define EXPECT_EQ(A,B) if((A)!=(B)){printf("%s:%d Expected %ld == %ld\n",__FILE__, __LINE__,(size_t)A,(size_t)B);exit(1);}
#define EXPECT_NE(A,B) if((A)==(B)){printf("%s:%d Expected %ld == %ld\n",__FILE__, __LINE__,(size_t)A,(size_t)B);exit(1);}

// initialize test
void test0()
{
    for (int i = 0; i < entity_count; i++) {
        ptrs[i] = (p64_struct*)tgc_alloc_opt(&gc, sizeof(p64_struct), 0, dtor);
        EXPECT_NE(ptrs[i],(void*)0x0);
        memset(ptrs[i],0,sizeof(p64_struct));
        balance++;
        balance_expected++;
    }
    tgc_run(&gc);
    EXPECT_EQ(balance,balance_expected);
    EXPECT_EQ(balance,entity_count);
    EXPECT_EQ(gc.nitems,entity_count);
}

// add pointer to pointer
void test1()
{
    for (int i = 0; i < test_count;i++){
        long rng = rand() % entity_count;
        if(ptrs[rng]->values[0] == NULL)
        {
            ptrs[rng]->values[0] = (p64_struct*) tgc_alloc_opt(&gc, sizeof(p64_struct), 0, dtor);
            EXPECT_NE(ptrs[rng]->values[0],(void*)0x0);
            memset(ptrs[rng]->values[0],0,sizeof(p64_struct));
            balance++;
            balance_expected++;
        }
    }
    tgc_run(&gc);
    EXPECT_EQ(balance,balance_expected);
    EXPECT_EQ(gc.nitems,balance_expected);
}

// duplicate (must be no change)
void test2()
{
    for (int i = 0; i < test_count; i++){
        long rng = rand() % entity_count;
        tgc_add_ptr(&gc, ptrs[rng], sizeof(p64_struct), 0, dtor);
    }
    tgc_run(&gc);
    EXPECT_EQ(balance,balance_expected);
    EXPECT_EQ(gc.nitems,balance_expected);
}


// delete or pass (first Half)
void test3()
{
    for (int i = 0; i < entity_count/2; i++)
        if(rand()&0x7){
            if(ptrs[i]->values[0])
                balance_expected--;
            tgc_free(&gc, ptrs[i]);
            balance_expected--;
        }
    tgc_run(&gc);
    EXPECT_EQ(balance,balance_expected);
    EXPECT_EQ(gc.nitems,balance_expected);
}


// leave or pass (second Half)
void test4()
{
    for (int i = (entity_count/2+1); i < entity_count; i++)
        if(rand()&0x7){
            if(ptrs[i]->values[0])
                balance_expected--;
            ptrs[i] = NULL;
            balance_expected--;
        }
    tgc_run(&gc);
    EXPECT_EQ(balance,balance_expected);
    EXPECT_EQ(gc.nitems,balance_expected);
}



// finalize
void test5()
{
    memset(ptrs,0,sizeof(p64_struct*) *entity_count);
    tgc_run(&gc);
    EXPECT_EQ(balance,0);
    EXPECT_EQ(gc.nitems,0);
}

#define EXEC(N) N();puts("SUCCESS: " #N);
void test_run_all(){
    p64_struct *stack_memory[entity_count];
    ptrs = &stack_memory;
    EXEC(test0)
    EXEC(test1)
    EXEC(test2)
    EXEC(test3)
    EXEC(test4)
    EXEC(test5)
}

int main(int argc, char **argv) {
    void *top_stack=NULL;
    srand(time(NULL));
    tgc_start(&gc, &top_stack);
    test_run_all();
    tgc_stop(&gc);
    return 0;
}