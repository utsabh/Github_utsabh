#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
char *paths[80];
char *filein;
int count1=1;
int fflag=0,flag=0,tc=0;

char** getcd(char *a,char *s)
{
  char *buff=strdup(a);
  char **arr3=(char **)malloc(200*sizeof(char *));
  char *token = strtok(buff,s);
  int q=0;
  while( token != NULL )
    {
      arr3[q]=strdup(token);
      q++;
      token = strtok(NULL,s);
    }
  arr3[q]=NULL;
  return arr3;
}
void err()
{
  char error_message[40] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}
void runc(char *a)
{
  char* ss=" \t\n";
  char** buf=getcd(a,ss);
  int t=0;
  char *pointer;
  while(paths[t]!=NULL)
    {
      pointer=strdup(paths[t]);
      strcat(pointer,"/");
      strcat(pointer,buf[0]);
      if(access(pointer,X_OK)==0)
	{
	  break;
	}
      t++;
    }
  execv(pointer,buf);
  
  err();
  exit(0);
}
void builtin(char **arr2)
{   
  if(strcmp(arr2[0],"exit")==0 && arr2[1]==NULL)
    {
      exit(0);
    }
  else if(strcmp(arr2[0],"cd")==0 && arr2[1]!=NULL && arr2[2]==NULL)
    {
      chdir(arr2[1]);
    }
  else if(strcmp(arr2[0],"path")==0)
    {
      if(arr2[1]!=NULL)
        {        
          for(int p=1;arr2[p]!=NULL;p++)
        {
          paths[count1]=arr2[p];
	  count1++;
        }
      paths[count1]=NULL;
        }
      else
        {
          for(int p=0;paths[p]!=NULL;p++)
            {
              paths[p]=NULL;
	      p++;
            }
      count1=0;
        }
    }
  else
    {
      err();
    }
}
void play(char *buffer)
{
  int count=0;
  char *sp;
  int i=0;
  while(buffer[i]!='\0')
    {
      if(buffer[i]=='&')
	{
	  count++;
	}
      i++;
    }
  if(count>0)
    {
      sp="&\n";
    }
  else
    {
      sp=" \n\t";
    }
  char **ar=getcd(buffer,sp);
  if(ar[0]==NULL)
    {
      exit(0);
    }
  else if(count>0)
    {
      if(ar[0]==NULL)
	{
	  exit(0);
	}
      else
    {
      for(int i=0;i<=count;i++)
        {
          int rc=fork();
          if(rc==0)
        {
          if(tc==1)
	    {
	      int fd = open(filein, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	      dup2(fd, 1);
	      dup2(fd, 2);
	      close(fd);
	    }
          runc(ar[i]);
        }
          else
	    {
	      wait(NULL);
	    }
        }
    }
 }
  else if(strcmp(ar[0],"exit")!=0 && strcmp(ar[0],"cd")!=0 && strcmp(ar[0],"path")!=0)
    {
      int rc=fork();
      if(rc==0)
    {
      if(tc==1)
	{
	   int fd = open(filein, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	   dup2(fd, 1);
	   dup2(fd, 2);
	   close(fd);
	}
      runc(buffer);
    }
      else
    {
      wait(NULL);
    }
    }
  else if(strcmp(ar[0],"exit")==0 || strcmp(ar[0],"cd")==0 || strcmp(ar[0],"path")==0)
    {
    builtin(ar);
    }
  tc=0;
  filein=NULL;
}

void redirection(char *buffer)
{
  char *sp=">\n";
  char **pointer;
  char **arr=getcd(buffer,sp);
  if(arr[1]!=NULL)
    {
      pointer=getcd(arr[1]," \n");
    }
  if(arr[2]!=NULL || arr[0]==NULL || arr[1]==NULL || pointer[1]!=NULL)
    {
      err();
    }
  else
    {
      filein=strdup(pointer[0]);
      tc=1;
      play(arr[0]);
    }
}
void p_redirection(char *buffer)
{
  char *sp="&\n";
  char **arr=getcd(buffer,sp);
  for(int i=0;arr[i]!=NULL;i++)
    {
      redirection(arr[i]);
    }
}
int main(int argc, char*argv[])
{
  paths[0]=strdup("/bin");
  paths[1]=NULL;
    fflag=0;
    flag=0;
    int i;
  if(argc==1)
    {
      while(1)
    {
      printf("wish> ");
      fflush(stdout);
      char *buffer1;
      int f=0;
      size_t buff_size = 100;
      buffer1=(char *)malloc(buff_size * sizeof(char));
      getline(&buffer1,&buff_size,stdin);
      for(int i=0;buffer1[i]!='\n';i++)
        {
        if(buffer1[i]!=' ')
          {
	    f=1;
	    break;
	  }
        }
      if(f==0)
	{
	  continue;
	}
  for(int i=0;buffer1[i]!='\0';i++)
    {
      if(buffer1[i]=='>')
	{
	  fflag++;
	}
    }
  for(i=0;buffer1[i]!='\0';i++)
    {
      if(buffer1[i]=='>')
	{
	  break;
	}
    }
  for(int j=i;buffer1[j]!='\0';j++)
    {
      if(buffer1[j]=='&')
    {
      flag=1;
      break;
    }
    }
      if(flag==1)
	{
	  p_redirection(buffer1);
	}
      else if(fflag>0)
	{
	  redirection(buffer1);
	}
      else
        play(buffer1);
    }
      
    }
  else if(argv[1]!=NULL)
    {
      if(argv[2]!=NULL)
    {
      err();
      return 1;
    }
      else
    {
      FILE *fp= fopen(argv[1],"r");
      if(fp==NULL)
        {
	  err();
	 exit(1);
	}
      else
        {
          size_t ps=100;
          char *ptr=malloc(ps * sizeof(char));
          while(getline(&ptr,&ps,fp)!=-1)
        {
          fflush(stdout);
          int f=0;
          for(int i=0;ptr[i]!='\n';i++)
            {
            if(ptr[i]!=' ')
              {
		f=1;
		break;
	      }
            }
          if(f==0)
	    {
	      continue;
	    }
         fflag=0;
	 flag=0;
  for(int i=0;ptr[i]!='\0';i++)
    {
      if(ptr[i]=='>')
	{
	fflag++;
	}
    }
  for(i=0;ptr[i]!='\0';i++)
    {
      if(ptr[i]=='>')
	{
	  break;
	}
    }
  for(int j=i;ptr[j]!='\0';j++)
  {
      if(ptr[j]=='&')
    {
      flag=1;
      break;
    }
    }
          if(flag==1)
	    {
	      p_redirection(ptr);
	    }
          else if(fflag>0)
	    {
	      redirection(ptr);
	    }
          else
            play(ptr);
        }
        }
    }
  }
  return 0;
}
