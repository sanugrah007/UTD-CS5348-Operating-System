/***********************************************************************
Author: Aman Sharma and Anugrah Sharma  
* UTD ID: AXS200055 AND AXS200119
* CS 5348.001 Operating Systems
* Prof. S Venkatesan
* Project - 2 (Demo2)
*****
* Compilation :cc proj2.c
* Run using :- ./a.out
*****
Objectives:
To re design UNIX V6 file system using given guidelines:

mkdir <dir_name>
Create the v6 directory. It should have two entries '.' and '..'

rm <v6 file>
Delete the file v6_file from the v6 file system.
Remove all the data blocks of the file, free the i-node and remove the directory entry.

cd <dir_name>
change working directory of the v6 file system to the v6 directory.
 ***********************************************************************/

#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
//for stat()
#include<sys/types.h>
#include<sys/stat.h>
#include<stdint.h>
#include<math.h>

#define FREE_SIZE 152
#define I_SIZE 200
#define BLOCK_SIZE 1024
#define ADDR_SIZE 11
#define INPUT_SIZE 256

// Superblock Structure

typedef struct {
  unsigned short isize;
  unsigned short fsize;
  unsigned short nfree;
  unsigned int free[FREE_SIZE];
  unsigned short ninode;
  unsigned short inode[I_SIZE];
  char flock;
  char ilock;
  unsigned short fmod;
  unsigned short time[2];
} superblock_type;

superblock_type superBlock;

// I-Node Structure

typedef struct {
unsigned short flags;
unsigned short nlinks;
unsigned short uid;
unsigned short gid;
unsigned int size;
unsigned int addr[ADDR_SIZE];
unsigned short actime[2];
unsigned short modtime[2];
} inode_type;

inode_type inode;

typedef struct {
  unsigned short inode;
  unsigned char filename[14];
} dir_type;

dir_type root;

int fileDescriptor;		//file descriptor
unsigned short cwd_inode_num;

const unsigned short inode_alloc_flag = 0100000;//regular
const unsigned short dir_flag = 040000;//directory
const unsigned short dir_large_file = 010000;//fifo
const unsigned short pfile = 000000;//plain file
const unsigned short dir_access_rights = 000777; // User, Group, & World have all access privileges
const unsigned short INODE_SIZE = 64; // inode has been doubled

int preInitialization();
int initfs(char* path, unsigned short blocks,unsigned short inodes);
void add_block_to_free_list( int blocknumber , unsigned int *empty_buffer );
void create_root();
void cpin(const char* from_filename, const char* to_filename);
void cpout(const char* from_filename, const char* to_filename);
int make_dir(const char* directory, unsigned short parent_dir_inode_num);
int entry_filename_dir(const char* filename, unsigned short dir_inode_num, unsigned short file_inode_num);
int get_free_data_block();
int reallocate_freelist();
short get_free_inode();
short find_dirInode(const char* directory, int dir_inode_num, int is_dir);
void remove_file(const char *fileName);

int main() {

  char input[INPUT_SIZE];
  char *splitter;
  char *cpin_src;
  char *cpin_des;
  char *cpout_src;
  char *cpout_des;
  char *mkdir_path;
  char *cd_path;
  char dir_path[100], dir_or_file[14];
  char *rmfile;
  unsigned short dir_inode_num, inode_num;
  int fs_initcheck = 0;
  int error;
  printf("Size of super block = %d , size of i-node = %d\n",sizeof(superBlock),sizeof(inode));

  while(1) { label:
    printf("\nEnter your command\n");
    scanf(" %[^\n]s", input);
    splitter = strtok(input," ");

    if(strcmp(splitter, "initfs") == 0) {

      preInitialization();
      fs_initcheck = 1;
      cwd_inode_num = 1;
      splitter = NULL;

    }
    else if (strcmp(splitter, "q") == 0) {

     lseek(fileDescriptor, BLOCK_SIZE, 0);
     write(fileDescriptor, &superBlock, BLOCK_SIZE);
     return 0;

    }
    else if (strcmp(splitter, "cpin") == 0) {

      if (fs_initcheck == 1) {
        cpin_src = strtok(NULL, " ");
        cpin_des = strtok(NULL, " ");
        if (!cpin_src || !cpin_des) {
          printf("Enter command in the form of cpin externalfile v6file\n");
        } else if (cpin_src && cpin_des) {
          cpin(cpin_src, cpin_des);
        }
      } else {
        printf("File System not initialized :(\n");
      }
      splitter = NULL;

    }
    else if (strcmp(splitter, "cpout") == 0) {

      if (fs_initcheck == 1) {
        cpout_src = strtok(NULL, " ");
        cpout_des = strtok(NULL, " ");
        if (!cpout_src || !cpout_des) {
          printf("Enter command in the form of cpout v6file externalfile\n");
        } else if (cpout_src && cpout_des) {
          cpout(cpout_src, cpout_des);
        }
      } else {
        printf("File System not initialized :(\n");
      }
      splitter = NULL;

    }
    else if (strcmp(splitter, "mkdir") == 0) {

      if (fs_initcheck == 1) {
        mkdir_path = strtok(NULL, " ");
        if(!mkdir_path) {
          printf("Enter command in the form of mkdir /long/file/path\n");
        }
        else if (mkdir_path) {
          int i;
          for(i = 0; mkdir_path[i] != '\0'; i++){
            dir_path[i] = mkdir_path[i];
          } dir_path[i] = '\0';

          if(dir_path[0] == '/') {  //absolute path
            dir_inode_num = 1;      //setting this to root inode
            if(dir_path[1] == '\0') {
              printf("Root Directory is already created\n");
              goto label;
            }

            int i = 0, j;
            while (dir_path[i] != '\0') {
              i++;
              for(j = 0; dir_path[i] != '/'; i++,j++) {
                if(j > 13) {
                  printf("FileName is greater than 13 characters\n");
                  goto label;
                }
                if(dir_path[i] == '\0') {break;}
                dir_or_file[j] = dir_path[i];
              }
              dir_or_file[j] = '\0';

              if(dir_path[i] == '\0' || (dir_path[i] == '/' && dir_path[i+1] == '\0')){  //end name
                inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
                if(inode_num != 0){
                  printf("Directory %s is already present\n", dir_or_file);
                  goto label;
                }
                error = make_dir(dir_or_file, dir_inode_num);
                if(error == 0) {
                  printf("Could not create directory %s\n", dir_or_file);
                } else {
                  printf("Directory %s is successfully created!\n", dir_or_file);
                }
                goto label;
              } else if (dir_path[i+1] != '\0'){                //middle name
                dir_inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
                if(dir_inode_num == 0) {
                  printf("%s directory is not present\n", dir_or_file);
                  goto label;
                }
              }
            }
          } else {                  //relative path
            dir_inode_num = cwd_inode_num;
            int j;
            for(j = 0; dir_path[j] != '\0'; j++) {
              if(j > 12) {
                printf("FileName is greater than 13 characters\n");
                goto label;
              }
              if(dir_path[j] == '/') {
                printf("Enter correct pathname\n");
                goto label;
              }
              dir_or_file[j] = dir_path[j];
            }
            dir_or_file[j] = '\0';
            inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
            if(inode_num != 0){
              printf("Directory %s is already present\n", dir_or_file);
              goto label;
            }
            error = make_dir(dir_or_file, dir_inode_num);
            if(error == 0) {
              printf("Could not create directory %s\n", dir_or_file);
            } else {
              printf("Directory %s is successfully created!\n", dir_or_file);
            }
            goto label;
          }
        }
      } else {
        printf("File System not initialized :(\n");
      }
      splitter = NULL;
    }
    else if (strcmp(splitter, "cd") == 0) {

      if (fs_initcheck == 1) {
        cd_path = strtok(NULL, " ");
        if(!cd_path) {
          printf("Enter command in the form of cd /long/file/path\n");
        }
        else if (cd_path) {
          int i;
          for(i = 0; cd_path[i] != '\0'; i++){
            dir_path[i] = cd_path[i];
          } dir_path[i] = '\0';
          /* int i;
          for(i = 0; mkdir_path[i] != '\0'; i++){
            dir_path[i] = mkdir_path[i];
          } dir_path[i] = '\0';*/
          if(dir_path[0] == '/') {  //absolute path
            dir_inode_num = 1;      //setting this to root inode
            if(dir_path[1] == '\0') {
              cwd_inode_num = dir_inode_num;
              printf("Working Directory set to Root Directory\n");
              goto label;
            }
            /* int i = 0, j;
            while (dir_path[i] != '\0') {
              i++;
              for(j = 0; dir_path[i] != '/'; i++,j++) {
                if(j > 13) {
                  printf("FileName is greater than 13 characters\n");
                  goto label;
                }
                if(dir_path[i] == '\0') {break;}
                dir_or_file[j] = dir_path[i];
              }
              dir_or_file[j] = '\0';*/

            int i = 0, j;
            while (dir_path[i] != '\0') {
              i++;
              for(j = 0; dir_path[i] != '/';i++,j++) {
                if(j > 13) {
                  printf("FileName is greater than 13 characters\n");
                  goto label;
                }
                if(dir_path[i] == '\0') {break;}
                dir_or_file[j] = dir_path[i];
              }
              dir_or_file[j] = '\0';

              if(dir_path[i] == '\0' || (dir_path[i] == '/' && dir_path[i+1] == '\0')){  //end name
                inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
                if(inode_num != 0){
                  cwd_inode_num = inode_num;
                  printf("Working Directory set to %s\n", dir_or_file);
                }
                else {
                  printf("%s directory is not present. Working Directory unchanged.\n", dir_or_file);
                }
              } else if (dir_path[i+1] != '\0'){                //middle name
                dir_inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
                if(dir_inode_num != 0){
                  cwd_inode_num = dir_inode_num;
                  printf("Working Directory set to %s\n", dir_or_file);
                }
                else {
                  printf("%s directory is not present. Working Directory unchanged.\n", dir_or_file);
                }
              }
              goto label;
            }
          } else {                  //relative path
            dir_inode_num = cwd_inode_num;
            int j;
            for(j = 0; dir_path[j] != '\0'; j++) {
              if(j > 12) {
                printf("FileName is greater than 13 characters\n");
                goto label;
              }
              if(dir_path[j] == '/') {
                printf("Enter correct pathname\n");
                goto label;
              }
              dir_or_file[j] = dir_path[j];
            }
            dir_or_file[j] = '\0';
            inode_num = find_dirInode(dir_or_file, dir_inode_num, 1);
            if(inode_num != 0){
              cwd_inode_num = inode_num;
              printf("Working Directory set to %s\n", dir_or_file);
            } else {
              printf("%s directory is not present. Working Directory unchanged.\n", dir_or_file);
            }
            goto label;
          }
        }
      } else {
        printf("File System not initialized :(\n");
      }
      splitter = NULL;
    }
    else if(strcmp(splitter, "rm")==0){
	    if(fs_initcheck == 1){
        rmfile = strtok(NULL, " ");
        if(!rmfile){
          printf("No file name entered. Please retry\n");
  	    }
  	    else{
          remove_file(rmfile);
        }
	    }
	    splitter = NULL;
      printf("\n");
    }
    else {

      printf("Invalid Command\n");

    }
  }
  return 0;
}

int preInitialization(){

  char *n1, *n2;
  unsigned int numBlocks = 0, numInodes = 0;
  char *filepath;

  filepath = strtok(NULL, " ");
  n1 = strtok(NULL, " ");
  n2 = strtok(NULL, " ");


  if(access(filepath, F_OK) != -1) {

    if(fileDescriptor = open(filepath, O_RDWR, 0600) == -1){

     printf("\n FileSystem already exists but open() failed with error [%s]\n", strerror(errno));
     return 1;
    }
    printf("FileSystem already exists and the same will be used.\n");

  } else {

  	if (!n1 || !n2) {
      printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");
    }
 		else {
  		numBlocks = atoi(n1);
  		numInodes = atoi(n2);

  		if( initfs(filepath, numBlocks, numInodes)){
  		  printf("The FileSystem is initialized\n");
  		} else {
    		printf("Error initializing FileSystem. Exiting... \n");
    		return 1;
  		}
 		}
  }
  return 0;
}

int initfs(char* path, unsigned short blocks, unsigned short inodes) {

   unsigned int buffer[BLOCK_SIZE/4];
   int bytes_written;

   unsigned short i = 0;
   superBlock.fsize = blocks;
   unsigned short inodes_per_block= BLOCK_SIZE/INODE_SIZE;

   if((inodes%inodes_per_block) == 0)
      superBlock.isize = inodes/inodes_per_block;
   else
      superBlock.isize = (inodes/inodes_per_block) + 1;

   if((fileDescriptor = open(path,O_RDWR|O_CREAT,0700))== -1)
       {
         printf("\n open() failed with the following error [%s]\n",strerror(errno));
         return 0;
       }

   for (i = 0; i < FREE_SIZE; i++) {
      superBlock.free[i] =  0;			//initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.
    }
   superBlock.nfree = 0;
   superBlock.ninode = I_SIZE;
   unsigned short max_inode = inodes;
   for (i = 0; i < I_SIZE; i++) {
	    superBlock.inode[i] = i + 1;		//Initializing the inode array to inode numbers
    }
   superBlock.flock = 'a'; 					//flock,ilock and fmode are not used.
   superBlock.ilock = 'b';
   superBlock.fmod = 0;
   superBlock.time[0] = 0;
   superBlock.time[1] = 1970;

   lseek(fileDescriptor, BLOCK_SIZE, SEEK_SET);
   bytes_written = write(fileDescriptor, &superBlock, BLOCK_SIZE); // writing superblock to file system
   if (bytes_written < BLOCK_SIZE) {
     printf("Error in writing superblock to file system :(\n");
     return 0;
   }

   // writing zeroes to all inodes in ilist
   for (i = 0; i < BLOCK_SIZE/4; i++) {
   	  buffer[i] = 0;
    }
   for (i = 0; i < superBlock.isize; i++) {
   	  write(fileDescriptor, buffer, BLOCK_SIZE);
    }
   int data_blocks = blocks - 2 - superBlock.isize;
   int data_blocks_for_free_list = data_blocks - 1;

   // Create root directory
   create_root();

   for ( i = 2 + superBlock.isize + 1; i < data_blocks_for_free_list; i++ ) {
      add_block_to_free_list(i , buffer);
   }

   return 1; //because int return type
}

// Add Data blocks to free list
void add_block_to_free_list(int block_number,  unsigned int *empty_buffer){

  if ( superBlock.nfree == FREE_SIZE ) {

    int free_list_data[BLOCK_SIZE / 4], i;
    free_list_data[0] = FREE_SIZE;

    for ( i = 0; i < BLOCK_SIZE / 4; i++ ) {
       if ( i < FREE_SIZE ) {
         free_list_data[i + 1] = superBlock.free[i];
       } else {
         free_list_data[i + 1] = 0; // getting rid of junk data in the remaining unused bytes of header block
       }
    }

    lseek( fileDescriptor, (block_number) * BLOCK_SIZE, SEEK_SET );
    write( fileDescriptor, free_list_data, BLOCK_SIZE ); // Writing free list to header block

    superBlock.nfree = 0;

  } else {

	  lseek( fileDescriptor, (block_number) * BLOCK_SIZE, SEEK_SET );
    write( fileDescriptor, empty_buffer, BLOCK_SIZE );  // writing 0 to remaining data blocks to get rid of junk data
  }

  superBlock.free[superBlock.nfree] = block_number;  // Assigning blocks to free array
  ++superBlock.nfree;
}

// Create root directory
void create_root() {

  int root_data_block = 2 + superBlock.isize; // Allocating first data block to root directory
  int i;
  unsigned short bytes_written = 0;

  root.inode = 1;   // root directory's inode number is 1.

  /*for (i = 0; i < 14; i++) {    //14 because the dir_type has char[14] filename
    root.filename[i] = 0;       //setting filename to 0, to remove junk value
  }*/
  root.filename[0] = '.';
  root.filename[1] = '\0';

  inode.flags = inode_alloc_flag | dir_flag | dir_access_rights;   		// flag for root directory
  inode.nlinks = 0;
  inode.uid = 0;
  inode.gid = 0;
  inode.size = 32; //should this be changed to 32, becuase 2 entries dot and dotdot
  inode.addr[0] = root_data_block;

  for( i = 1; i < ADDR_SIZE; i++ ) {
    inode.addr[i] = 0;        //setting the remaining addr[i] to 0
  }

  inode.actime[0] = 0;
  inode.modtime[0] = 0;
  inode.modtime[1] = 0;

  lseek(fileDescriptor, 2 * BLOCK_SIZE, SEEK_SET);
  bytes_written = write(fileDescriptor, &inode, INODE_SIZE);   //writing root inode
  if (bytes_written < INODE_SIZE) {
    printf("Error in writing Root Inode\n");
  }

  lseek(fileDescriptor, root_data_block * BLOCK_SIZE, 0);
  bytes_written = write(fileDescriptor, &root, 16);   //writing in root data block . //16 because char[14]+short
  if (bytes_written < 16) {
    printf("Error in writing . entry for root directory\n");
  }

  root.filename[0] = '.';
  root.filename[1] = '.';
  root.filename[2] = '\0';

  bytes_written = write(fileDescriptor, &root, 16);   //writing in root data block ..
  if (bytes_written < 16) {
    printf("Error in writing .. entry for root directory\n");
  }
}

// Function to copy file contents from external file to v6-file
void cpin(const char* from_filename, const char* to_filename){
  printf("\nInside CPIN, copy from %s to %s\n", from_filename, to_filename);
  int bytes_written;

  struct stat statbuf;
  if(stat(from_filename, &statbuf) == 0){
    int filesize, blocks_allocated;
    filesize = statbuf.st_size;   // file size of from_filename
    blocks_allocated = (filesize + (BLOCK_SIZE - 1))/BLOCK_SIZE;    //another way of writing a ceil()
    if (blocks_allocated <= ADDR_SIZE) {
      printf("Copying small file with total blocks %d\n", blocks_allocated);

      unsigned short free_inode;    //store the free inode number to assign to new file
      unsigned short parent_dir_inode_num = cwd_inode_num;
      free_inode = find_dirInode(to_filename, parent_dir_inode_num, 0);
      if(free_inode != 0){
        printf("File %s is already present in v6 FileSystem\n", to_filename);
        return;
      }
      //printf("Past the find_dirInode");
      //getting the inode number
      free_inode = get_free_inode();
      if(free_inode == 0){
        printf("Error in finding suitable inode for allocation\n");
        return;
      }
      //printf("Inode Number for File%d\n", free_inode);

      //update the inode values
      inode_type file_inode;
      file_inode.flags = inode_alloc_flag|pfile|dir_access_rights;
      file_inode.size = filesize;
      int i;
      for (i = 0; i < blocks_allocated; i++) {
        file_inode.addr[i] = get_free_data_block();
        if(file_inode.addr[i] == 0){
          printf("Error in assigning data block\n");
          return;
        }
      }
      for( i = blocks_allocated; i < ADDR_SIZE; i++ ) {
        inode.addr[i] = 0;        //setting the remaining addr[i] to 0
      }
      file_inode.nlinks = 0;
      file_inode.uid = 0;
      file_inode.gid = 0;
      file_inode.actime[0] = 0;
      file_inode.modtime[0] = 0;
      file_inode.modtime[1] = 0;
      lseek(fileDescriptor, (2 * BLOCK_SIZE) + (free_inode * INODE_SIZE), SEEK_SET);
      bytes_written = write(fileDescriptor, &file_inode, INODE_SIZE);
      if (bytes_written < INODE_SIZE) {
        printf("Error in writing the file Inode");
        return;
      }

      //update parent Directory
      if(!entry_filename_dir(to_filename, parent_dir_inode_num, free_inode)){
        printf("Error in making file entry in Working Directory.\n");
        return;
      }

      unsigned short buffer[BLOCK_SIZE/2];
      int fd_dest, fd_source;
      if((fd_source = open(from_filename, O_RDONLY)) == -1)
      {
        printf("Failed to open source file\n");
        return;
      }
      if((fd_dest = open(to_filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) == -1)
      {
        printf("Failed to open destination file\n");
        return;
      }
      int read_size;
      while((read_size = (read(fd_source, &buffer, BLOCK_SIZE))) != 0)
      {
        write(fd_dest, &buffer, read_size);
        bzero(&buffer, BLOCK_SIZE);
      }
      printf("Small file copied\n");
      close(fd_source);
      close(fd_dest);
      printf("CPIN complete!\n");
    }
    else{
      printf("Chosen file is large, please choose small file!\n");
    }
  }
  else{ //stat unsuccessful
    printf("Error in accessing %s with error [%s]\n", from_filename,strerror(errno));
  }
}

// Function to copy file contents from v6-file to external file
void cpout(const char* from_filename, const char* to_filename){
  printf("\nInside CPOUT, copy from %s to %s\n", from_filename, to_filename);
  int bytes_written;
  int fd_dest;
  if((fd_dest = open(to_filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) == -1)
  {
    printf("Failed to open destination file\n");
    return;
  }

  unsigned short dir_inode_num = cwd_inode_num;

  int from_file_inode_num = find_dirInode(from_filename, dir_inode_num, 0);
  if(from_file_inode_num==0)
  {
    printf("No File exists in current directory");
    return;
  }
  inode_type from_file_inode;
  lseek(fileDescriptor,(2 * BLOCK_SIZE)+(from_file_inode_num * INODE_SIZE), SEEK_SET);
  read(fileDescriptor, &from_file_inode, INODE_SIZE);

  unsigned short buffer[BLOCK_SIZE/2];
  int blocks_allocated = (from_file_inode.size + (BLOCK_SIZE - 1))/BLOCK_SIZE;
  int i, read_size;
  if(blocks_allocated <= ADDR_SIZE){
    printf("Copying small file with total blocks %d\n", blocks_allocated);
    for(i = 0; i < blocks_allocated; i++){
      lseek(fileDescriptor, from_file_inode.addr[i] * BLOCK_SIZE, SEEK_SET);
      while((read_size = (read(fileDescriptor, &buffer, BLOCK_SIZE))) != 0)
      {
        write(fd_dest, &buffer, read_size);
        bzero(&buffer, BLOCK_SIZE);
      }
    }
    printf("Small file copied\n");
    close(fd_dest);
    printf("CPOUT complete!\n");
  } else {
    printf("Chosen file is large, please choose small file!\n");
  }

  /*struct stat statbuf;
  if(stat(from_filename, &statbuf) == 0){
    int filesize, blocks_allocated;
    filesize = statbuf.st_size;   // file size of from_filename
    blocks_allocated = (filesize + (BLOCK_SIZE - 1))/BLOCK_SIZE;    //another way of writing a ceil()
    if (blocks_allocated <= ADDR_SIZE) {
      printf("Copying small file with total blocks %d\n", blocks_allocated);
      unsigned short buffer[BLOCK_SIZE/2];
      int fd_dest, fd_source;
      if((fd_source = open(from_filename, O_RDONLY)) == -1)
      {
        printf("Failed to open source file\n");
        return;
      }
      if((fd_dest = open(to_filename, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU|S_IRWXG|S_IRWXO)) == -1)
      {
        printf("Failed to open destination file\n");
        return;
      }
      int read_size;
      while((read_size = (read(fd_source, &buffer, BLOCK_SIZE))) != 0)
      {
        write(fd_dest, &buffer, read_size);
        bzero(&buffer, BLOCK_SIZE);
      }
      printf("Small file copied\n");
      close(fd_source);
      close(fd_dest);
      printf("CPOUT complete!\n");
    }
    else{
      printf("Chosen file is large, please choose small file!\n");
    }
  }
  else{ //stat unsuccessful
    printf("Error in accessing %s with error [%s]\n", from_filename,strerror(errno));
  }*/
}

// Function to make Directory
int make_dir(const char* directory, unsigned short parent_dir_inode_num){
  inode_type new_dir_inode;
  unsigned short new_dir_inode_num;
  char dot_entry[14] = ".";
  char dotdot_entry[14] = "..";

  new_dir_inode.flags = inode_alloc_flag | dir_flag | dir_access_rights;   		// flag for root directory
  new_dir_inode.nlinks = 0;
  new_dir_inode.uid = 0;
  new_dir_inode.gid = 0;
  new_dir_inode.size = 32;
  new_dir_inode.addr[0] = get_free_data_block(); //check this for out of data blocks
  if (new_dir_inode.addr[0] == 0) {
    printf("Out of data blocks\n");
    return 0;
  }
  int i;
  for( i = 1; i < ADDR_SIZE; i++ ) {
    new_dir_inode.addr[i] = 0;        //setting the remaining addr[i] to 0
  }

  new_dir_inode.actime[0] = 0;
  new_dir_inode.modtime[0] = 0;
  new_dir_inode.modtime[1] = 0;

  new_dir_inode_num = get_free_inode();

  lseek(fileDescriptor, new_dir_inode.addr[0] * BLOCK_SIZE, SEEK_SET);
  write(fileDescriptor, &new_dir_inode_num, 2);
  write(fileDescriptor, dot_entry, 14);
  write(fileDescriptor, &parent_dir_inode_num, 2);
  write(fileDescriptor, dotdot_entry, 14);

  lseek(fileDescriptor, ((BLOCK_SIZE * 2) + (new_dir_inode_num * INODE_SIZE)), SEEK_SET);
  write(fileDescriptor, &new_dir_inode, INODE_SIZE);

  //Make the directory entry in the parent Directory
  if(entry_filename_dir(directory, parent_dir_inode_num, new_dir_inode_num)){
    return 1;
  } else {
    printf("Error in making entry in the parent directory\n");
    return 0;
  }

}

int entry_filename_dir(const char* filename, unsigned short dir_inode_num, unsigned short file_inode_num){
  int dir_inode_pos;
  inode_type dir_inode;
  dir_inode_pos = (BLOCK_SIZE * 2) + (dir_inode_num * INODE_SIZE);
  lseek(fileDescriptor, dir_inode_pos, SEEK_SET);
  read(fileDescriptor, &dir_inode, INODE_SIZE);
  printf("Working Directory inode number %d", dir_inode_num);
  if(1){ //If directory
    int dir_size, i;
    unsigned short dir_data_block;
    dir_size = dir_inode.size;
    i = (dir_size + 16)/BLOCK_SIZE;

    if(i < ADDR_SIZE) {
      if(dir_inode.addr[i] == 0){
        dir_inode.addr[i] = get_free_data_block();
        if(dir_inode.addr[i] == 0){
          printf("Out of Data Blocks\n");
          return 0;
        }
      }
    } else {
      printf("Directory Address Space size exceeded\n");
      return 0;
    }

    dir_data_block = dir_inode.addr[i];
    lseek(fileDescriptor, (BLOCK_SIZE * dir_data_block) + (dir_size % BLOCK_SIZE), SEEK_SET);
    write(fileDescriptor, &file_inode_num, 2);
    write(fileDescriptor, filename, 14);
    dir_inode.size = dir_inode.size + 16;
    lseek(fileDescriptor, dir_inode_pos, SEEK_SET);
    write(fileDescriptor, &dir_inode, INODE_SIZE);
    return 1;
  }
  else{
    printf("Entered name is not a directory\n");
    return 0;
  }
}

int get_free_data_block() {
  if(superBlock.nfree == 1) {
    if(reallocate_freelist() != 1) {
      return 0;
    }
  }
  return superBlock.free[--superBlock.nfree];
}

int reallocate_freelist() {
  if (superBlock.free[0] == 0) {
    printf("No more data blocks available for allocation\n");
    return 0;
  } else {
    unsigned short free_list_block, nfree, free_block;
    int i;
    free_list_block = superBlock.free[0];
    lseek(fileDescriptor, free_list_block * BLOCK_SIZE, SEEK_SET);
    read(fileDescriptor, &nfree, 2);
    for (i = 0; i < nfree; i++) {
      read(fileDescriptor, &free_block, 2);
      superBlock.free[i] = free_block;
    }
    superBlock.free[i] = free_list_block;
    superBlock.nfree++;
    return 1;
  }
}

short get_free_inode() {
  int i;
  if(superBlock.ninode == 0) {
    unsigned short max_inode = superBlock.inode[0];
    for (i = 0; i < I_SIZE; i++) {
      if(max_inode > superBlock.inode[i]) {
        max_inode = superBlock.inode[i];
      }
    }
    for(i = 0; i < I_SIZE; i++){ //so that inode 1 is not entered in the inode array
      superBlock.inode[i] = ++max_inode;
      superBlock.ninode++;
    }
  }
  if(superBlock.ninode > 0) {
    superBlock.ninode--;
    return superBlock.inode[superBlock.ninode];
  }
  return 0;
}

short find_dirInode(const char* directory, int dir_inode_num, int is_dir) {
  //printf("find_dirInode started");
  inode_type dir_inode;
  unsigned short new_dir_inode_num;
  char dir_name[14];
  int i, j;
  int inode_pos = (BLOCK_SIZE * 2) + (dir_inode_num * INODE_SIZE);
  lseek(fileDescriptor, inode_pos, SEEK_SET);
  read(fileDescriptor, &dir_inode, INODE_SIZE);
  if(0/*dir_inode.flags != (inode_alloc_flag | dir_flag | dir_access_rights)*/ && is_dir) {
    printf("%s is not a Directory!!\n", directory);
    return 0;
  }
  for(i = 0; i < ADDR_SIZE; i++){
    if(dir_inode.addr[i] == 0) {return 0;}
    int data_block_num = dir_inode.addr[i];
    lseek(fileDescriptor, BLOCK_SIZE * data_block_num, SEEK_SET);
    for(j = 0; j < BLOCK_SIZE/16; j++){
      read(fileDescriptor, &new_dir_inode_num, 2);
      if(new_dir_inode_num != 0) {
        read(fileDescriptor, dir_name, 14);
        //printf("find_dirInode ending");
        if(strcmp(dir_name, directory) == 0) {
          return new_dir_inode_num;
        }
      } else {
        return 0;
      }
    }
    return 0;
  }
}

void remove_file(const char *fileName){

  inode_type fileInode ;
  unsigned short addrcount = 0;
  unsigned short dir_inode_num=cwd_inode_num;                                            //// Parent Directory
  unsigned short inode_num = find_dirInode(fileName,dir_inode_num,0);                    //// Inode number of file
  if(inode_num==0)
  {
    printf("No File exists");
    return;
  }

  lseek(fileDescriptor,(2*BLOCK_SIZE)+(inode_num*INODE_SIZE),SEEK_SET );
  read(fileDescriptor,&fileInode,INODE_SIZE);

  if (0/*fileInode.flags == (inode_alloc_flag | dir_flag | dir_access_rights)*/)
  {
    printf("%s is a directory, not a file\n", fileName);
    return;
  }
    //// Cleanup inode content
  int x;
  int blocks_allocated = (fileInode.size + (BLOCK_SIZE - 1))/BLOCK_SIZE;
  for (x = 0; x < blocks_allocated; x++)
  {
    if (superBlock.nfree == FREE_SIZE)
    {
      lseek(fileDescriptor,BLOCK_SIZE*fileInode.addr[x],SEEK_SET);
      write(fileDescriptor,superBlock.free,FREE_SIZE * 4);
      superBlock.nfree = 0;
    }
      superBlock.free[superBlock.nfree] = fileInode.addr[x];
      superBlock.nfree++;
  }
 ////add free inode
  if (superBlock.ninode == I_SIZE)
      return;

  superBlock.inode[superBlock.ninode] = inode_num;
  superBlock.ninode++;


////remove directory entry
inode_type directory_inode;

lseek(fileDescriptor,(2*BLOCK_SIZE)+(cwd_inode_num*INODE_SIZE),SEEK_SET );
read(fileDescriptor,&directory_inode,INODE_SIZE);

dir_type files[FREE_SIZE];

lseek(fileDescriptor,BLOCK_SIZE*directory_inode.addr[0],SEEK_SET);
read(fileDescriptor,files,directory_inode.size);

int numberOfFiles = directory_inode.size / 16;
int i;
for (i = 0; i < numberOfFiles; i++) {
    if (strcmp(fileName, files[i].filename) == 0) {
        files[i] = files[numberOfFiles - 1];
        break;
    }
}

directory_inode.size -= 16;

lseek(fileDescriptor,BLOCK_SIZE*directory_inode.addr[0],SEEK_SET);
write(fileDescriptor,files,directory_inode.size);

lseek(fileDescriptor,(BLOCK_SIZE*2) + (INODE_SIZE*cwd_inode_num) ,SEEK_SET);
write(fileDescriptor,&directory_inode,INODE_SIZE);

}
