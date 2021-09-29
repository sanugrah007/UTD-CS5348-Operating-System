/* ***************

 Author: Anugrah Sharma and Aman Sharma

* UTD ID: AXS200119 AND AXS200055

* CS 5348.001 Operating Systems

* Prof. S Venkatesan

* Project - 1*

***************

* Compilation :-$ gcc -o proj1 project1.c -std=gnu99

* Run using :- $ ./proj1

******************/



#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
/////Additional libraries
#include <signal.h>         /////For Signal call
#include <sys/wait.h>    
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>       ////Using in superBlock node



//GLOBAL CONSTANTS
#define FREE_SIZE 100 
#define I_SIZE 100
#define BLOCK_SIZE 512
#define ISIZE 32
#define inode_alloc 0100000        //Flag value to allocate an inode
#define pfile 000000               //To define file as a plain file
#define lfile 010000               //To define file as a large file
#define directory 040000           //To define file as a Directory
#define max_array 256
#define null_size 512


//GLOBAL VARIABLES
int fd ;                //file descriptor
int rootfd;
const char *rootname;
unsigned short chainarray[max_array];



// superBlock block 
typedef struct {
unsigned short isize;
unsigned short fsize;
unsigned short nfree;
unsigned short free[FREE_SIZE];
unsigned short ninode;
unsigned short inode[I_SIZE];
char flock;
char ilock;
char fmod;
unsigned short time[2];
}superblock_type;

superblock_type superBlock;

typedef struct {
unsigned short flags;
char nlinks;
char uid;
char gid;
char size0;
unsigned short size1;
unsigned short addr[8];
unsigned short actime[2];
unsigned short modtime[2];
} inode_type; 

inode_type inode;

// directory
typedef struct 
{
        unsigned short inode;
        char filename[13];
        }dir_type;
dir_type newdir;		// instance of directory
dir_type dir1;   //for duplicates




int initfs(char* path, unsigned short total_blocks,unsigned short total_inodes);     //// file system initilization
void create_root();                                                                       //// function to create root directory and its inode

////Functions
void cpin(const char *pathname1 , const char *pathname2);                                  //// Function to copy file contents from external file to v6-file
void cpout_smallfile(const char *pathname1 , const char *pathname2 , int num_block);         //// Copying Small file using cpout
void cpout(const char *pathname1 , const char *pathname2);                                 //// cpout Copying from v-6file System into an external File
void cpin_smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated);      //// Copying from a Small File

////Additional Functions

void chainblocks( unsigned short total_blocks);                                            //// Data chaining procedure
unsigned short allocate_to_free_block();                                                       //// get a free data block
unsigned short allocateinode();                                                            //// allocate inode
void read_block_int(unsigned short *dest, unsigned short bnode);                             //// Read integer array from the required block
void write_block_int(unsigned short *dest, unsigned short bnode);                            //// Write integer array to the required block
void copy_inode(fs_inode current_inode, unsigned int new_inode);                           //// Write to an inode given the inode number
void freeblock(unsigned short block);                                                      //// free data blocks and initialize free array
void update_rootdir(const char *pathname , unsigned short inode_number);                   //// Function to update root directory
void display_files();                                                                      //// display all files


int main(int argc, char *argv[])                                                    ////Main Function
	{	
	signal(SIGINT, SIG_IGN);
	int fs_init = 0;                                                               // bit to check if file system is initialized
	char *p, arr1[256];
	unsigned short n = 0, j , k;
	char buf[BLOCK_SIZE];
	unsigned short no_of_bytes;
	char *name_dir;
	char *cpoutdes;
	char *cpoutsrc;
	unsigned int bnode =0, inode_no=0;
	char *cpindes;
	char *cpinsrc;
	char *file_path;
	char *filename;
	char *n1, *n2;
    
	char c;
	int i = 0;
	char *num3, *num4;
	char *tmp = (char *)malloc(sizeof(char) * 200);
	char *cmd1 = (char *)malloc(sizeof(char) * 200);

	
	printf("\n\nInitialize the file system by using initfs <<name of your v6filesystem>> << total blocks>> << total inodes>>\n");
	while(1)
		{
		printf("\nEnter your command\n");
		scanf(" %[^\n]s", arr1);
		p = strtok(arr1," ");
		if(strcmp(p, "initfs")==0)
			{
			file_path = strtok(NULL, " ");
			n1 = strtok(NULL, " ");
			n2 = strtok(NULL, " ");
			if(access(file_path,X_OK) != -1)
				{
				if( fd = open(file_path, O_RDWR) == -1)
					{
					printf("File system exists, open failed");
					return 1;
					}
				access(file_path, R_OK |W_OK | X_OK);
				printf("Filesystem already exists.\n");
				printf("Same file system to be used\n");
				fs_init=1;
				}
			else
				{
				if (!n1 || !n2)
					printf("Enter all arguments in form initfs v6filesystem 5000 400.\n");
				else
					{
					bnode = atoi(n1);
					inode_no = atoi(n2);
					if(initfs(file_path,bnode, inode_no))
						{
						printf("File System has been Initialized \n");
						fs_init = 1;
						}
					else
						{
						printf("Error: File system initialization error.\n");
						}
					}
				}
			p = NULL;
			printf("\n");
			}
		else if(strcmp(p, "cpin")==0)
			{
			if(fs_init == 0)
				printf("File System has not been initialized.\n\n");
			else
				{
				cpinsrc = strtok(NULL, " ");
				cpindes = strtok(NULL, " ");
				if(!cpinsrc || !cpindes )
					printf("Enter the command in the form cpin externalfile v6-file \n");
				else if((cpinsrc) && (cpindes ))
					{
					cpin(cpinsrc,cpindes);
					//printf("Enter ls command to check");
					}
				}
			p = NULL;
			printf("\n");
			}
		else if(strcmp(p, "cpout")==0)
			{
			if(fs_init == 0)
				printf("File system has not been initialized.\n");
			else
				{
				cpoutsrc = strtok(NULL, " ");
				cpoutdes = strtok(NULL, " ");
				if(!cpoutsrc || !cpoutdes )
					printf("Enter command in the form cpout v6-file externalfile\n");
				else if((cpinsrc) && (cpindes ))
					{
					cpout(cpoutsrc,cpoutdes);
					//printf("Enter ls command to check");
					}
				}
			p = NULL;
			printf("\n");
			}
		else if(strcmp(p, "q")==0)
			{
			printf("\nEXITING FILE SYSTEM NOW....\n");
			lseek(fd,BLOCK_SIZE,0);
			if((no_of_bytes =write(fd,&superBlock,BLOCK_SIZE)) < BLOCK_SIZE)
				{
				printf("\nerror in writing the superBlock block\n");
				}
			lseek(fd,BLOCK_SIZE,0);
			return 0;
			}
		else
			{
			printf("\nInvalid command\n ");
			printf("\n");
			}
		}
	}
	
	 int initfs(char* path, unsigned short total_blocks,unsigned short total_inodes)               // file system initilization
	{
	printf("\nV6 File System\n");
	char buf[BLOCK_SIZE];
	int no_of_bytes;
	if(((total_inodes*32)%BLOCK_SIZE ) == 0)
		superBlock.isize = ((total_inodes*32)/BLOCK_SIZE );
	else
		superBlock.isize = ((total_inodes*32)/BLOCK_SIZE )+1;	

	superBlock.fsize = total_blocks;

	unsigned short i = 0;
	
	if((fd = open(path,O_RDWR|O_CREAT,0600))== -1)          
		{
		printf("\n open() failed with error [%s]\n",strerror(errno));
		return 1;
		}
	
	for (i = 0; i<100; i++)                                                          //assigning superblock values
		superBlock.free[i] =  0;			

	superBlock.nfree = 0;
	superBlock.ninode = 100;
	
	for (i=0; i < 100; i++)
		superBlock.inode[i] = i;		

	superBlock.flock = 'f'; 					
	superBlock.ilock = 'i';					
	superBlock.fmod = 'f';
	superBlock.time[0] = 0000;
	superBlock.time[1] = 1970;
	lseek(fd,BLOCK_SIZE,SEEK_SET);
	lseek(fd,0,SEEK_SET);
	write(fd, &superBlock, 512);

	if((no_of_bytes =write(fd,&superBlock,BLOCK_SIZE)) < BLOCK_SIZE)
		{
		printf("\n error in writing the superBlock block\n");
		return 0;
		}

	for (i=0; i<BLOCK_SIZE; i++)  
		buf[i] = 0;

	lseek(fd,1*BLOCK_SIZE,SEEK_SET);

	for (i=0; i < superBlock.isize; i++)
		write(fd,buf,BLOCK_SIZE);

	chainblocks(total_blocks);	

	for (i=0; i<100; i++) 
		{
		superBlock.free[superBlock.nfree] = i+2+superBlock.isize; //get free block
		++superBlock.nfree;
		}

	create_root()();
	return 1;
	}

	unsigned short allocate_to_free_block()                                                // get a free data block
	{
	unsigned short block;
	block = superBlock.free[--superBlock.nfree];
	superBlock.free[superBlock.nfree] = 0;
	
	if (superBlock.nfree == 0)
		{
		int n=0;
		read_block_int(chainarray, block);
		superBlock.nfree = chainarray[0];
		for(n=0; n<100; n++)
			superBlock.free[n] = chainarray[n+1];
		}
	return block;
	}
	
	void create_root()                                       //function to create root directory and its inode
 	{
 	rootname = "root";
 	rootfd = creat(rootname, 0600);
 	rootfd = open(rootname , O_RDWR | O_APPEND);
 	unsigned int i = 0;
 	unsigned short no_of_bytes;
 	unsigned short dblock = allocate_to_free_block();
 	
	for (i=0;i<14;i++)
		 newdir.filename[i] = 0;

 	newdir.filename[0] = '.';                       //root directory's file name is .
 	newdir.filename[1] = '\0';
 	newdir.inode = 1;                                       // root directory's inode number is 1.
	
	 inode.flags = inode_alloc | directory | 000077;
	 inode.nlinks = 2;
	 inode.uid = '0';
	 inode.gid = '0';
	 inode.size0 = '0';
	 inode.size1 = ISIZE;
	 inode.addr[0] = dblock;
	
	 for (i=1;i<8;i++)
		 inode.addr[i] = 0;

	inode.actime[0] = 0;
	inode.modtime[0] = 0;
	inode.modtime[1] = 0;

	copy_inode(inode, 0);
	lseek(rootfd , BLOCK_SIZE , SEEK_SET);
	write(rootfd , &inode , ISIZE);
	lseek(rootfd, dblock*BLOCK_SIZE, SEEK_SET);

	
	no_of_bytes = write(rootfd, &newdir, 16);                                      //filling 1st entry with .
	if((no_of_bytes) < 16)
		 printf("\n Error in writing root directory \n ");


	newdir.filename[0] = '.';
	newdir.filename[1] = '.';
	newdir.filename[2] = '\0';
	no_of_bytes = write(rootfd, &newdir, 16);
	if((no_of_bytes) < 16)
		 printf("\n Error in writing root directory\n ");
	close(rootfd);
	}

 void cpin(const char *pathname1 , const char *pathname2)                    //Function to copy file contents from external file to v6-file
	{
	struct stat stats;
	int blksize , blocks_allocated , req_blocks;
	int filesize;
	stat(pathname1 , &stats);
	blksize = stats.st_blksize;
	blocks_allocated = stats.st_blocks;
	filesize = stats.st_size;
	req_blocks = filesize / 512;
	
	if(blocks_allocated <= 8)
		{
		// externalfile is small file
		printf("small file , %d\n" , blocks_allocated);
		cpin_smallfile(pathname1 , pathname2 , blocks_allocated); 
		}
	else
		{
		//externalfile is largefile
		printf("Large file , %d\n" , blocks_allocated);
		largefile(pathname1 , pathname2 , blocks_allocated); 
		}
	printf("cpin complete\n");
	}


void cpin_smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated)             //Copying from a Small File
	{
	int f , fd ,i ,j,k,l;
	unsigned short size;
	unsigned short inode_number;
	inode_number = allocateinode();
	struct stat s1;
	stat(pathname1 , &s1);
	size = s1.st_size;
	unsigned short buff[100];
	fs_inode inode;
 	fs_super sb;
	sb.isize = superBlock.isize;
	sb.fsize = superBlock.fsize;
	sb.nfree = 0;
		for(l=0; l<100; l++)
		{
		sb.free[sb.nfree] = l+2+sb.isize ;
		sb.nfree++;
		}

	inode.flags = inode_alloc | pfile | 000077; 
	inode.size0 =0;
	inode.size1 = size;
	blocks_allocated = size/512;
	f = creat(pathname2, 0775);
	f = open(pathname2 , O_RDWR | O_APPEND);
	lseek(f , 0 , SEEK_SET);
	write(f ,&sb , 512);
	update_rootdir(pathname2 , inode_number);
	unsigned short bl;
	for(i=0; i<blocks_allocated; i++)
		{
		inode.addr[i] = sb.free[i];
		sb.free[i] = 0;
		}
	close(f);
	lseek(f , 512 , SEEK_SET);
	write(f , &inode , 32);
	close(f);
	unsigned short buf[256];
	fd = open(pathname1 ,O_RDONLY);
	f = open(pathname2 , O_RDWR | O_APPEND);
	for(j =0; j<=blocks_allocated; j++)
		{
		lseek(fd , 512*j , SEEK_SET);
		read(fd ,&buf , 512);
		lseek(f , 512*(inode.addr[j]) , SEEK_SET);
		write(f , &buf , 512);
		}
	printf("Small file copied\n");
	close(f);
	close(fd);
	fd = open(pathname2 , O_RDONLY); 
	
	}

void cpout(const char *pathname1 , const char *pathname2)               // cpout Copying from v-6file System into an external File
	{
	struct stat stats;
	int blocksize , blocks_allocated , num_blocks;
	int filesize;
	stat(pathname1 , &stats);
	blocksize = stats.st_blksize;
	blocks_allocated = stats.st_blocks;
	filesize = stats.st_size;
	num_blocks = filesize /512;
	
	if(blocks_allocated < 8)
		{
		//v6-file is small file
		printf("Small file , %d\n" , num_blocks);
		cpout_smallfile(pathname1 , pathname2 , num_blocks);  
		}
	else
		{
		//v6-file is large file
		printf("Largefile , %d\n" , num_blocks);
		out_largefile(pathname1 , pathname2 ,  num_blocks);  
		}
	printf("cpout complete\n");
	}


void cpout_smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated)                //Copying Small file using cpout
	{
	int num_blocks = blocks_allocated;
	int f , fd , i;
	unsigned short buf[256];
	unsigned short addr[8];
	f = open(pathname1 , O_RDONLY);
	lseek(f , 520 , SEEK_SET);
	read(f , &addr , 16);
	close(f);
	f = open(pathname1 , O_RDONLY);
	fd = creat(pathname2, 0775);
	fd = open(pathname2, O_RDWR | O_APPEND);
	for(i =0 ; i<num_blocks; i++)
		{
		lseek(f , i*512 , SEEK_SET);
		read(f , &buf , 512);
		lseek(fd , 512*i , SEEK_SET);
		write(fd , &buf , 512);
		}
	close(fd);
	close(f);
	}
	
	
 void chainblocks(unsigned short total_blocks)
	 {
	 unsigned short empty_buffer[256];
	 unsigned short count;
	 unsigned short no_of_blocks = total_blocks/100;
	 unsigned short remaining_blocks = total_blocks%100;
	 unsigned short idx = 0;
	 int i=0;
	 for (idx=0; idx <= 255; idx++)
		 {
		 empty_buffer[idx] = 0;
		 chainarray[idx] = 0;
		 }

	 
	 for (count=0; count < no_of_blocks; count++)                   //chaining for chunks of blocks 100 blocks at a time
		 {
		 chainarray[0] = 100;
	
		 for (i=0;i<100;i++)
			 {
			 if((count == (no_of_blocks - 1)) && (remaining_blocks == 0))
				 {
				 if ((remaining_blocks == 0) && (i==0))
					 {
					 if ((count == (no_of_blocks - 1)) && (i==0))
						 {
						 chainarray[i+1] = 0;
						 continue;
						 }
					 }
				 }
			 chainarray[i+1] = i+(100*(count+1))+(superBlock.isize + 2 );
			 }			

		 write_block_int(chainarray, 2+superBlock.isize+100*count);

		 for (i=1; i<=100;i++)
			 write_block_int(emptybuffer, 2+superBlock.isize+i+ 100*count);
		 }

	
	chainarray[0] = remaining_blocks;                                    //chaining for remaining blocks
	chainarray[1] = 0;
	
	for (i=1;i<=remaining_blocks;i++)
		 chainarray[i+1] = 2+superBlock.isize+i+(100*count);

	 write_block_int(chainarray, 2+superBlock.isize+(100*count));

	 for (i=1; i<=remaining_blocks;i++)
		 write_block_int(chainarray, 2+superBlock.isize+1+i+(100*count));
	 }

unsigned short allocateinode()                             // allocate inode
	{
	unsigned short inumber;
	unsigned int i = 0;
	inumber = superBlock.inode[--superBlock.ninode];
	return inumber;
	}
	
	void copy_inode(fs_inode current_inode, unsigned int new_inode)                     //Write to an inode given the inode number	
	{
	int no_of_bytes;
	lseek(fd,2*BLOCK_SIZE + new_inode*ISIZE,0);
	no_of_bytes=write(fd,&current_inode,ISIZE);

	if((no_of_bytes) < ISIZE)
		printf("\n Error in inode number : %d\n", new_inode);		
	}
	
	
void read_block_int(unsigned short *dest, unsigned short bnode)                                 //Read integer array from the required block
	{
	int flag=0;
	if (bnode > superBlock.isize + superBlock.fsize )
		flag = 1;

	else
		{			
		lseek(fd,bnode*BLOCK_SIZE,SEEK_SET);
		read(fd, dest, BLOCK_SIZE);
		}
	}



void write_block_int(unsigned short *dest, unsigned short bnode)                       //Write integer array to the required block
	{
	int flag1, flag2;
	int no_of_bytes;
	
	if (bnode > superBlock.isize + superBlock.fsize )
		flag1 = 1;		
	else
		{
		lseek(fd,bnode*BLOCK_SIZE,0);
		no_of_bytes=write(fd, dest, BLOCK_SIZE);

		if((no_of_bytes) < BLOCK_SIZE)
			flag2=1;		
		}
	if (flag2 == 1)
		{
		//problem with block
		}
	}

	
void freeblock(unsigned short block)                                    //free data blocks and initialize free array
	{
	superBlock.free[superBlock.nfree] = block;
	++superBlock.nfree;
	}


void update_rootdir(const char *pathname , unsigned short inode_number)             //Function to update root directory
	{
	int i;
	dir_type ndir;
	int size;
	ndir.inode = inode_number;
	strncpy(ndir.filename, pathname , 14);
	size = write(rootfd , &ndir , 16);
	}





void display_files() 															// display all files
	{  
	int i , size;
	fs_inode i_node;
	dir_type d1;
	unsigned short buf;
	rootfd = open(rootname , O_RDWR | O_APPEND);
	lseek(rootfd , 520 , SEEK_SET);
	size = read(rootfd , &i_node.addr[0] , 2);
	for(i=0; i<10; i++)
		{
		lseek(rootfd , (120*512)+(16*i) , SEEK_SET);
		size = read(fd , &d1 , 16);
		printf("%d , %s\n" , d1.inode , d1.filename);
		close(rootfd);
		}
	}


int offset_set(int block)
	{
	int offset =0;
	return block * BLOCK_SIZE + offset;
	}



