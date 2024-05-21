#include "unp.h"

// 实现了对子进程退出信号的处理，能够监控子进程的退出状态，并输出相应的信息
void sig_chld(int signo) //signal action
{
    printf("signal\n");
    pid_t pid;
    int stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    {
        if(WIFEXITED(stat))
        {
            printf("child %d normal termination, exit status = %d\n",pid,  WEXITSTATUS(stat));
        }
        else if(WIFSIGNALED(stat))
        {
            printf("child %d abnormal termination, singal number  = %d%s\n", pid, WTERMSIG(stat),
            #ifdef WCOREDUMP
                   WCOREDUMP(stat)? " (core file generated) " : ""); // 是否有core文件生成
            #else
                   "");
            #endif
        }
    }
    return;
}

void * Calloc(size_t n, size_t size)
{
    void *ptr;
    if( (ptr = calloc(n, size)) == NULL)
    {
        errno = ENOMEM;
        err_quit("Calloc error");
    }
    return ptr;
}


Sigfunc *Signal(int signo, Sigfunc * func)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(signo == SIGALRM){
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    } else {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    if(sigaction(signo, &act, &oact) < 0)
    {
        return SIG_ERR;
    }
    return oact.sa_handler;
}
