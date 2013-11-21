#include "threadPool.h"

static void *tp_work_thread(void *pthread);
static void *tp_manage_thread(void *pthread);
static TPBOOL tp_init(tp_thread_pool *thisPool);
static void tp_close(tp_thread_pool *thisPool);
static void tp_process_job(tp_thread_pool *thisPool, tp_work *worker, tp_work_desc *job);
static int  tp_get_thread_by_id (tp_thread_pool *thisPool,int id);
static TPBOOL tp_add_thread(tp_thread_pool *thisPool);
static TPBOOL tp_delete_thread(tp_thread_pool *thisPool);
static int  tp_get_tp_status(tp_thread_pool *thisPool);

/**
  * user interface. creat thread pool.
  * para:
  * 	num: min thread number to be created in the pool
  * return:
  * 	thread pool struct instance be created successfully
  */
tp_thread_pool *creat_thread_pool(int min_num, int max_num)
{
    tp_thread_pool *thisPool;
    thisPool = (tp_thread_pool*)malloc(sizeof(tp_thread_pool));
    memset(thisPool, 0, sizeof(tp_thread_pool));

    //init member function ponter
    thisPool->init = tp_init;
    thisPool->close = tp_close;
    thisPool->process_job = tp_process_job;
    thisPool->get_thread_by_id = tp_get_thread_by_id;
    thisPool->add_thread = tp_add_thread;
    thisPool->delete_thread = tp_delete_thread;
    thisPool->get_tp_status = tp_get_tp_status;

    //init member var
    thisPool->min_th_num = min_num;
    thisPool->cur_th_num = thisPool->min_th_num;
    thisPool->max_th_num = max_num;
    pthread_mutex_init(&thisPool->tp_lock, NULL);

    //malloc mem for num thread info struct
    if(NULL != thisPool->thread_info)
        free(thisPool->thread_info);
    thisPool->thread_info = (tp_thread_info*)malloc(sizeof(tp_thread_info)*thisPool->max_th_num);

    return thisPool;
}


/**
  * member function reality. thread pool init function.
  * para:
  * 	thisPool: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
TPBOOL tp_init(tp_thread_pool *thisPool){
    int i;
    int err;

    //creat work thread and init work thread info
    for(i=0;i<thisPool->min_th_num;i++){
        pthread_cond_init(&thisPool->thread_info[i].thread_cond, NULL);
        pthread_mutex_init(&thisPool->thread_info[i].thread_lock, NULL);

        err = pthread_create(&thisPool->thread_info[i].thread_id, NULL, tp_work_thread, thisPool);
        if(0 != err){
            printf("tp_init: creat work thread failed\n");
            return FALSE;
        }
        printf("tp_init: creat work thread %d\n", (int)thisPool->thread_info[i].thread_id);
    }

    //creat manage thread
    err = pthread_create(&thisPool->manage_thread_id, NULL, tp_manage_thread, thisPool);
    if(0 != err){
        printf("tp_init: creat manage thread failed\n");
        return FALSE;
    }
    printf("tp_init: creat manage thread %d\n", (int)thisPool->manage_thread_id);

    return TRUE;
}

/**
  * member function reality. thread pool entirely close function.
  * para:
  * 	thisPool: thread pool struct instance ponter
  * return:
  */
void tp_close(tp_thread_pool *thisPool){
    int i;

    //close work thread
    for(i=0;i<thisPool->cur_th_num;i++){
        kill(thisPool->thread_info[i].thread_id, SIGKILL);
        pthread_mutex_destroy(&thisPool->thread_info[i].thread_lock);
        pthread_cond_destroy(&thisPool->thread_info[i].thread_cond);
        printf("tp_close: kill work thread %d\n", (int)thisPool->thread_info[i].thread_id);
    }

    //close manage thread
    kill(thisPool->manage_thread_id, SIGKILL);
    pthread_mutex_destroy(&thisPool->tp_lock);
    printf("tp_close: kill manage thread %d\n", (int)thisPool->manage_thread_id);

    //free thread struct
    free(thisPool->thread_info);
}

/**
  * member function reality. main interface opened.
  * after getting own worker and job, user may use the function to process the task.
  * para:
  * 	thisPool: thread pool struct instance ponter
  *	worker: user task reality.
  *	job: user task para
  * return:
  */
void tp_process_job(tp_thread_pool *thisPool, tp_work *worker, tp_work_desc *job){
    int i;
    int tmpid;

    //fill thisPool->thread_info's relative work key
    for(i=0;i<thisPool->cur_th_num;i++){
        pthread_mutex_lock(&thisPool->thread_info[i].thread_lock);////
        if(!thisPool->thread_info[i].is_busy){
            //thread state be set busy before work
            thisPool->thread_info[i].is_busy = TRUE;
            pthread_mutex_unlock(&thisPool->thread_info[i].thread_lock);

            thisPool->thread_info[i].th_work = worker;
            thisPool->thread_info[i].th_job = job;

            pthread_cond_signal(&thisPool->thread_info[i].thread_cond);

            return;
        }
        else
            pthread_mutex_unlock(&thisPool->thread_info[i].thread_lock);	////
    }//end of for

    //if all current thread are busy, new thread is created here
    pthread_mutex_lock(&thisPool->tp_lock);
    if( thisPool->add_thread(thisPool) ){
        i = thisPool->cur_th_num - 1;
        tmpid = thisPool->thread_info[i].thread_id;
        thisPool->thread_info[i].th_work = worker;
        thisPool->thread_info[i].th_job = job;
    }
    pthread_mutex_unlock(&thisPool->tp_lock);

    //send cond to work thread
    pthread_cond_signal(&thisPool->thread_info[i].thread_cond);
    return;
}

/**
  * member function reality. get real thread by thread id num.
  * para:
  * 	thisPool: thread pool struct instance ponter
  *	id: thread id num
  * return:
  * 	seq num in thread info struct array
  */
int tp_get_thread_by_id(tp_thread_pool *thisPool, int id){
    int i;

    for(i=0;i<thisPool->cur_th_num;i++){
        if(id == thisPool->thread_info[i].thread_id)
            return i;
    }

    return -1;
}

/**
  * member function reality. add new thread into the pool.
  * para:
  * 	thisPool: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
static TPBOOL tp_add_thread(tp_thread_pool *thisPool){
    int err;
    tp_thread_info *new_thread;

    if( thisPool->max_th_num <= thisPool->cur_th_num )
        return FALSE;

    //malloc new thread info struct
    new_thread = &thisPool->thread_info[thisPool->cur_th_num];

    //init new thread's cond & mutex
    pthread_cond_init(&new_thread->thread_cond, NULL);
    pthread_mutex_init(&new_thread->thread_lock, NULL);

    //init status is busy
    new_thread->is_busy = TRUE;

    //add current thread number in the pool.
    thisPool->cur_th_num++;

    err = pthread_create(&new_thread->thread_id, NULL, tp_work_thread, thisPool);
    if(0 != err){
        free(new_thread);
        return FALSE;
    }

    return TRUE;
}

/**
  * member function reality. delete idle thread in the pool.
  * only delete last idle thread in the pool.
  * para:
  * 	thisPool: thread pool struct instance ponter
  * return:
  * 	true: successful; false: failed
  */
static TPBOOL tp_delete_thread(tp_thread_pool *thisPool){
    //current thread num can't < min thread num
    if(thisPool->cur_th_num <= thisPool->min_th_num) return FALSE;

    //if last thread is busy, do nothing
    if(thisPool->thread_info[thisPool->cur_th_num-1].is_busy) return FALSE;

    //kill the idle thread and free info struct
    kill(thisPool->thread_info[thisPool->cur_th_num-1].thread_id, SIGKILL);
    pthread_mutex_destroy(&thisPool->thread_info[thisPool->cur_th_num-1].thread_lock);
    pthread_cond_destroy(&thisPool->thread_info[thisPool->cur_th_num-1].thread_cond);

    //after deleting idle thread, current thread num -1
    thisPool->cur_th_num--;

    return TRUE;
}

/**
  * member function reality. get current thread pool status:idle, normal, busy, .etc.
  * para:
  * 	thisPool: thread pool struct instance ponter
  * return:
  * 	0: idle; 1: normal or busy(don't process)
  */
static int  tp_get_tp_status(tp_thread_pool *thisPool){
    float busy_num = 0.0;
    int i;

    //get busy thread number
    for(i=0;i<thisPool->cur_th_num;i++){
        if(thisPool->thread_info[i].is_busy)
            busy_num++;
    }

    if(busy_num/(thisPool->cur_th_num) < BUSY_THRESHOLD)
        return 0;//idle status
    else
        return 1;//busy or normal status
}

/**
  * internal interface. real work thread.
  * para:
  * 	pthread: thread pool struct ponter
  * return:
  */
static void *tp_work_thread(void *pthread){
    pthread_t curid;//current thread id
    int nseq;//current thread seq in the thisPool->thread_info array
    tp_thread_pool *thisPool = (tp_thread_pool*)pthread;//main thread pool struct instance

    //get current thread id
    curid = pthread_self();

    //get current thread's seq in the thread info struct array.
    nseq = thisPool->get_thread_by_id(thisPool, curid);
    if(nseq < 0)
        return;

    //wait cond for processing real job.
    while( TRUE ){
        pthread_mutex_lock(&thisPool->thread_info[nseq].thread_lock);
        pthread_cond_wait(&thisPool->thread_info[nseq].thread_cond, &thisPool->thread_info[nseq].thread_lock);
        pthread_mutex_unlock(&thisPool->thread_info[nseq].thread_lock);

        tp_work *work = thisPool->thread_info[nseq].th_work;
        tp_work_desc *job = thisPool->thread_info[nseq].th_job;
        //process
        work->process_job(work, job);

        //thread state be set idle after work
        pthread_mutex_lock(&thisPool->thread_info[nseq].thread_lock);
        thisPool->thread_info[nseq].is_busy = FALSE;
        pthread_mutex_unlock(&thisPool->thread_info[nseq].thread_lock);
    }
}

/**
  * internal interface. manage thread pool to delete idle thread.
  * para:
  * 	pthread: thread pool struct ponter
  * return:
  */
static void *tp_manage_thread(void *pthread)
{
    tp_thread_pool *thisPool = (tp_thread_pool*)pthread;//main thread pool struct instance

    sleep(MANAGE_INTERVAL);

    do{
        if( thisPool->get_tp_status(thisPool) == 0 ){
            do{
                if( !thisPool->delete_thread(thisPool) )
                    break;
            }while(TRUE);
        }//end for if

        sleep(MANAGE_INTERVAL);
    }while(TRUE);
}


