#include<stdlib.h>
#include<stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include<string.h>

// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Inodes start at block 2.

#define T_DIR       1   // Directory
#define T_FILE      2   // File
#define T_DEV       3   // Special device

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Bits per block
#define BPB           (BSIZE*8)


const char *err0="image not found.";
const char *err1="ERROR: bad inode.";
const char *err2="ERROR: bad direct address in inode.";
const char *err21="ERROR: bad indirect address in inode.";
const char *err3="ERROR: root directory does not exist.";
const char *err4="ERROR: directory not properly formatted.";
const char *err5="ERROR: address used by inode but marked free in bitmap.";
const char *err6="ERROR: bitmap marks block in use but it is not in use.";
const char *err7="ERROR: direct address used more than once.";
const char *err8="ERROR: indirect address used more than once.";
const char *err9="ERROR: inode marked use but not found in a directory.";
const char *err10="ERROR: inode referred to in directory but marked free.";
const char *err11="ERROR: bad reference count for file.";
const char *err12="ERROR: directory appears more than once in file system.";



// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};


int fsfd;
void rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, 512) != 512){
    perror("read");
    exit(1);
  }
}

struct superblock *sb;
struct dinode *init_ptr;
char *map_img;
void *b_map;
uint *temp;
int inodeBlock;
int *inodeArray;
uint *i_array;
uint *i_dir_array;
uint *d_adr;
uint *i_adr;
char buff[512];
char buffer[512];

void freeall(){
  free(inodeArray);
  free(i_array);
  free(d_adr);
  free(i_adr);
  free(i_dir_array);
}

void check_validity(uint adr, const char * err)
{
    if(adr==0)
        return;
    if(adr<(sb->ninodes/IPB +sb->nblocks/BPB +3) || adr > (sb->ninodes/IPB +sb->nblocks/BPB +3 + sb->nblocks))
    {
        fprintf(stderr,"%s\n",err);
        //freeall();
        exit(1);
    }
}



int main(int argc,char *argv[])
{
  
    if (argc < 2) {
	    printf("Usage: xcheck <file_system_image>\n");
	    exit(1);
    }
  
    fsfd=open(argv[1],O_RDONLY);
    if(fsfd==-1)
    {
        fprintf(stderr,"%s\n",err0);
        exit(1);
    }
    
    struct stat buf;
    if(fstat(fsfd,&buf)==-1){
        printf("fstat error\n");
        exit(1);
    } 

    //Map the data into the memory.
    map_img= (void *)mmap(NULL,buf.st_size,PROT_READ,MAP_PRIVATE,fsfd,0);
  
    //Get the superblock
    sb= (struct superblock *)(map_img+BSIZE);  
     
    //First dinode pointer.   
    init_ptr = (struct dinode *) (map_img + 2 * BSIZE);                 
  
    //Bit map.
    b_map = (void *) (map_img + (sb->ninodes / IPB + 3) * BSIZE); 
  
  
    //Direct Address
    d_adr=(uint *)malloc(sizeof(uint)*sb->ninodes*(NDIRECT+1));
    if(d_adr==0){
        fprintf(stderr, "Unable to allocate memory");
    }
  
    //Indirect Address
    i_adr=(uint *)malloc(sizeof(uint)*sb->ninodes*(NINDIRECT+1));
    if(i_adr==0){
        fprintf(stderr, "Unable to allocate memory");
    }
      
    //Inode Array
    inodeArray=(int*)malloc(sizeof(uint)*sb->ninodes);
    if(inodeArray==0){
        fprintf(stderr, "Unable to allocate memory");
    }
    
    i_array=(uint*)malloc(sizeof(int)*sb->ninodes*(12*(NINDIRECT/sizeof(uint))+NINDIRECT*(NINDIRECT/sizeof(uint))));
    if(i_array==0){
        fprintf(stderr, "Unable to allocate memory");
    }
    
    i_dir_array=(uint*)malloc(sizeof(uint)*sb->ninodes*(12*30+NINDIRECT*30));
    if(i_dir_array==0){
        fprintf(stderr, "Unable to allocate memory");
    }

    struct dinode *pointer1=init_ptr;
    //struct dinode *pointer2=init_ptr+1;
    int count1=0;
    int count2=0;
    int count3=0;
    int count4=0;
    int count5=0;
    
    for(int i=0;i<sb->ninodes;i++){
        if(pointer1->type==0){
            pointer1++;
            continue;
        }
        for(int x=0;x<NDIRECT;x++){
            if(pointer1->addrs[x]==0)
                continue;
            d_adr[count1++]=pointer1->addrs[x];
            
            if(pointer1->type==1){
                rsect(pointer1->addrs[x],buff);
                struct dirent *dirent=(struct dirent*)buff;
                for(int j=1;j<=(NINDIRECT/sizeof(uint));j++){
                    if(dirent->inum!=0){
                        i_array[count3++]=dirent->inum;
                    }              
                    if(dirent->inum!=0 && strcmp(dirent->name,".")!=0 && 
                        strcmp(dirent->name,"..")!=0){
                            i_dir_array[count5++]=dirent->inum;
                    }
                    dirent++;
                }
            }
        }
	if(pointer1->addrs[NDIRECT]!=0)
	{
        d_adr[count1++]=pointer1->addrs[NDIRECT];
	}
        rsect(pointer1->addrs[NDIRECT],buff);
        uint *temp=(uint*)buff;
        for(int j=1;j<=NINDIRECT;j++){
            if(*temp==0){
                temp++;
                continue;
            }
        
            if(pointer1->type==1){
                rsect(*temp,buffer);
                struct dirent *dirent=(struct dirent*)buffer;
                for(int y=1;y<=(NINDIRECT/sizeof(uint));y++){
                    if(dirent->inum!=0){
                        i_array[count3++]=dirent->inum;
                    }
                    if(dirent->inum!=0 && strcmp(dirent->name,".")!=0 && 
                        strcmp(dirent->name,"..")!=0){
                            i_dir_array[count5++]=dirent->inum;
                    }
                    dirent++;
                }
            }
            i_adr[count2++]=*temp;
            temp++;  
        }
        inodeArray[count4++]=i;
        pointer1++;
    }
    
    
    struct dinode *super=init_ptr;
    for(int i=0;i<sb->ninodes;i++){
	//check 1
        if(super->type==0){
            super++;
            continue;
        }
        else if(super->type>3){
            fprintf(stderr,"%s\n",err1);
            freeall();
            exit(1);
        }
    //check 3
    else if(init_ptr+1==NULL|| (init_ptr+1)->type!=1){
        fprintf(stderr,"%s\n",err3);
        freeall();
        exit(1);
    }
    else{
        int flag=0;
        for(int i=0;i<NDIRECT;i++){
            rsect((init_ptr+1)->addrs[i],buff);
            struct dirent *dirent=(struct dirent*)buff;
            for(int j=1;j<=(NINDIRECT/sizeof(uint));j++){
                if(strcmp(dirent->name,"..")==0 &&
                    dirent->inum==1){
                        flag=1;
                        break;
                    }
                    dirent++;
            }
            if(flag==1){
                break;
            }
        }
        if(flag==0){
            fprintf(stderr,"%s\n",err3);
            freeall();
            exit(1);
        }
    }
        
	//check 2
        for(int y=0;y<NDIRECT;y++){
            check_validity(super->addrs[y],err2);
        }
        

        rsect(super->addrs[NDIRECT],buff);
        uint *tempbuf=(uint*)buff;
        for(int j=1;j<=NINDIRECT;j++){
            check_validity(*tempbuf,err21);
            tempbuf++;
        }

    
	//check 4
        if(super->type==1){
            int flag1=0;
	    int	flag2=0;
            for(int y=0;y<NDIRECT;y++){
                rsect(super->addrs[y],buff);
                struct dirent *dirent=(struct dirent*)buff;
                for(int j=1;j<=(NINDIRECT/sizeof(uint));j++){
                    if(strcmp(dirent->name,".")==0 &&
                        dirent->inum==i){
                            flag1=1;
                        }
                    if(strcmp(dirent->name,"..")==0){
                        flag2=1;
                    }
                    dirent++;
                }
                if(flag1==1 && flag2==1)
                    break;
            }
            if(flag1==0 && flag2==0){
                fprintf(stderr,"%s\n",err4);
                freeall();
                exit(1);
            }
        }

        //check 5
        for(int y=0;y<NDIRECT;y++){
            if(super->addrs[y]==0)
                continue;
            uint byte=super->addrs[y]/8;
            uint bit=super->addrs[y]%8;
            void *bmap_ptr=b_map;
            for(int z=1;z<=byte;z++)
                bmap_ptr++;
            if((*(int*)bmap_ptr & (1 << bit)) == 0){
                fprintf(stderr,"%s\n",err5);
                freeall();
                exit(1);
            } 
        }
        char mapbuf[512];
        rsect(super->addrs[NDIRECT],mapbuf);
        temp=(uint*)mapbuf;
        for(int z=1;z<=NINDIRECT;z++){
            if(*temp==0)
                continue;
            uint byte=*temp/8;
            uint bit=*temp%8;
            void *bmap_ptr=b_map;
            for(int z=1;z<=byte;z++)
                bmap_ptr++;
            if((*(int*)bmap_ptr & (1 << bit)) == 0){
                fprintf(stderr,"%s\n",err5);
                freeall();
                exit(1);
            }
            temp++;
        }

	//check 9
        int flag=0;
        for(int y=0;y<count3;y++){
            if(i==i_array[y]){
                flag=1;
                break;
            }
        }
        if(flag==0){
            fprintf(stderr,"%s\n",err9);
            freeall();
            exit(1);
        }

	//Check 10
    for(int i=0;i<count3;i++){
        int flag=0;
        int ino=i_array[i];
        for(int j=0;j<count4;j++){
            if(ino==inodeArray[j]){
                flag=1;
                break;
            }
        }
        if(flag==0){
            fprintf(stderr,"%s\n",err10);
            freeall();
            exit(1);
        }
    } 
	//check 11
        if(super->type==2){
            int temp3=0;
            for(int y=0;y<count3;y++){
                if(i==i_array[y])
                    temp3++;
            }
            if(temp3!=super->nlink){
                fprintf(stderr,"%s\n",err11);
                freeall();
                exit(1);
            }
        }
	//check 12
        if(super->type==1){
            int temp3=0;
            for(int y=0;y<count5;y++){
                if(i==i_dir_array[y])
                    temp3++;
            }
            if(temp3>1){
                fprintf(stderr,"%s\n",err12);
                freeall();
                exit(1);
            }
        }

        super++;
    }

	//check 6
	int block_no=sb->ninodes/IPB + sb->nblocks/BPB + 4;
  	int counter=block_no/8;
  	void *bm_ptr=b_map; // start of the bitmap block
  	bm_ptr+=counter;
  	while(counter<=(sb->nblocks/8)){
  	  for(int i=(block_no % 8);i < 8;i++){
      	if((*(char*)bm_ptr & (1 << i)) > 0){
        	int flag_bm=0;
        	for(int j=0;j<count1;j++){
          		if(block_no==d_adr[j]){
            		flag_bm=1;
            		break;
          		}
        	}
        	if(flag_bm!=1){
          	for(int j=0;j<count2;j++){
            		if(block_no==i_adr[j]){
              		flag_bm=1;
              		break;
            		}
          	}
        }
        if(flag_bm==0){
          fprintf(stderr,"%s\n",err6);
          freeall();
          exit(1);
        }
      }
      block_no++;
    }
    bm_ptr++;
    counter++;
  }
	//Check 7 and 8
	for(int i=0;i<count1;i++){
        uint ui=d_adr[i];
        for(int j=i+1;j<count1;j++){
            if(ui==d_adr[j]){
                fprintf(stderr,"%s\n",err7);
                freeall();
                exit(1);
            }
        }
    }


    for(int i=0;i<count2;i++){
        uint ui=i_adr[i];
        for(int j=i+1;j<count2;j++){
            if(ui==i_adr[j]){
                fprintf(stderr,"%s\n",err8);
                freeall();
                exit(1);
            }
        }
    }
	
    freeall();
    return 0;
}
