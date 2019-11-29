#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

struct rec {
    struct rec *next;
    int id,count,timestamp,running,argCount;
    char **arg;
};
void add_to_list(struct rec **head_ptr, int id, int count, int timestamp, char *args[],int argCount) {
    struct rec *node = malloc(sizeof(struct rec)); 
    node->id = id;
    node->count = count;
    node->argCount = argCount;
    node->timestamp=timestamp;
    node->next = *head_ptr;
    node->running=1;
    node->arg=args;
    *head_ptr = node;
}
void print_list(struct rec *head) {
    while(head != NULL) {
        printf("Count:%d ID:%d Timestamp:%d Command %s\n", head->count, head->id,head->timestamp,head->arg[0]);

        head = head->next;
    }
}
char* getCommand(struct rec *head,int pid) {
        while(head != NULL) {
            if(head->id==pid){
            return head->arg[0];   
            }
            head = head->next;
        }
        return NULL;
}
void updatePid(struct rec *head,int pid,int newPid) {
    while(head != NULL) {
        if(head->id==pid){
         head->id=newPid;
        }
        head = head->next;
    }
}
void updateTime(struct rec *head,int pid,int time) {
    while(head != NULL) {
        if(head->id==pid){
         head->timestamp=time;
        }
        head = head->next;
    }
}
char** getArgs(struct rec *head,int pid) {
    while(head != NULL) {
        if(head->id==pid){
            if(head->argCount<=1){
               char **none={NULL};
               return none;
            }
            else{
                char **i=head->arg;
                char** j=&i[0];
            return j;
            }
        }
        head = head->next;
    }
return NULL;
}
int gettimestamp(struct rec *head,int pid) {
    while(head != NULL) {
        if(head->id==pid){
         return head->timestamp;   
        }
        head = head->next;
    }
return 0;
}
int isRunning(struct rec *head) {
    while(head != NULL) {
        if(head->running==1){
         return 1;   
        }
        head = head->next;
    }
    return 0;
}
int getcount(struct rec *head,int pid) {
    while(head != NULL) {
        if(head->id==pid){
         return head->count;   
        }
        head = head->next;
    }
            return 0;

}
int isDone(struct rec *head,int pid,int value) {
    while(head != NULL) {
        if(head->id==pid){
         head->running=value;   
        }
        head = head->next;
    }
return 0;
}
int main(int argc,char *argv[]) {
    struct rec *head = NULL;
    time_t startTime;
    time(&startTime);
    int dotin[argc];
    dotin[0]=0;
    int arrayTracker=1;
    for(int i=0;i<argc;i++){
        if(strcmp(argv[i],".")==0){
            dotin[arrayTracker]=i;
            arrayTracker++;
        }
    }
    dotin[arrayTracker]=argc;
    int i=0;
    int start=0;
    int count=0;
    
    while(dotin[i]<=argc){
        int catchSize=dotin[i]-start;
        if(catchSize>0){
            count++;
        }
        start=dotin[i]+1;
        i++;
    }
    
    i=0;
    start=0;
    int assignCount=1;
    pid_t pid=1;
    while(dotin[i]<=argc){
        int catchSize=dotin[i]-start;
        if(catchSize>0){
            char** catch=malloc(sizeof(char *)*(catchSize+1));
                for(int j=start;j<dotin[i];j++){
                    catch[j-start]=argv[j];
                }
            if(pid!=0){
                pid=fork();
                if(pid!=0){ //parent add to list
                    time(&startTime);
                    add_to_list(&head,pid,assignCount,startTime,catch,catchSize);
                } 
                else{
                    char out[22];
                    sprintf(out,"%d.out",(assignCount-1));
                    char err[22];
                    sprintf(err,"%d.err",(assignCount-1));
                    dup2(open(out,O_CREAT|O_WRONLY|O_APPEND,0777),STDOUT_FILENO);
                    dup2(open(err,O_CREAT|O_WRONLY|O_APPEND,0777),STDERR_FILENO);
                    char *noargs[]={NULL};
                    if(catchSize>1){
                        execvp(catch[0],&catch[0]);
                        perror("execv");
                        exit(2);
                    }
                    else{
                        execvp(catch[0],noargs);
                        perror("execv");
                        exit(2);
                    }
                perror("execv");
                exit(2);
                }
            }
            assignCount++;
        }
    start=dotin[i]+1;
    i++;    
    }
    int returncode;
    int finishpid;
    time_t currTime;
    int exitStatus;
    while(isRunning(head)==1){   
        finishpid=wait(&returncode);
        exitStatus=WEXITSTATUS(returncode);        
        if(finishpid==-1){
            printf("%s\n",strerror(errno));   
        }
        time(&currTime);
        int finishtime=(int)currTime-gettimestamp(head,finishpid);
        int finishedcount=getcount(head,finishpid);
        char string[22];
        sprintf(string,"%d.out",(finishedcount-1));
        FILE *fp;
        char errstring[22];
        sprintf(errstring,"%d.err",(finishedcount-1));
        FILE *fp2;
        char exitString[22];
        sprintf(exitString,"exited rc=%d\n",exitStatus);
        fp2=fopen(errstring,"a+");
        fputs(exitString,fp2);
        if(WIFSIGNALED(returncode)!=0){
            int signal=WTERMSIG(returncode);
            char exitSignal[22];
            sprintf(exitSignal,"Signal %d\n",signal);
            fputs(exitSignal,fp2);
        }
        fclose(fp2);
        if(finishtime<2){
            fp=fopen(errstring,"a+");
            fputs("Spawning Too Fast\n",fp);
            fclose(fp);
            isDone(head,finishpid,0);
        }
        else {
            pid=fork();
            if(pid==0){
                dup2(open(string,O_CREAT|O_WRONLY|O_APPEND,0777),STDOUT_FILENO);    
                dup2(open(errstring,O_CREAT|O_WRONLY|O_APPEND,0777),STDERR_FILENO);
                execvp(getCommand(head,finishpid),getArgs(head,finishpid));
                perror("execv");
                exit(2);
            }
            else{
                time(&startTime);
                updateTime(head,finishpid,(int)startTime);
                updatePid(head,finishpid,pid);
            }
        }
    }
}
    
 
