/**GroupMembers:		
 *		1)Abhilash Basappa Gudasi
 *        abg160130@utdallas.edu
 *     	2)Avinash Chandrasekharan
 *        axc166930@utdallas.edu
 * To design and develop a program which will allow a Unix user access 
 * to the file system of a foriegn operating system,the modified Unix Version 6.
 * Program displays the output of the valid unix command typed and finally 
 * while exiting displays number of commands executed(including exit command)
 *
 * Compiled on cs1.utdallas.edu Linux : Linux/CentOS 7.2
 *
 * **********Running the program*************:
 * --> Copy/Create file --> vi fsaccess.c
 * --> Execute the C file --> gcc -o fsaccess fsaccess.c
 * --> Run the executable -->./fsaccess <path/directory_name> 
 *      SUPPORTED COMMANDS: 
 *	1. initfs  <number_of_blocks> <number_of_inodes>
 *		SAMPLE: initfs 400 20
 *	2. cpin <external_source_file_path> <V6_file_name>
 *		SAMPLE: cpin /project2/read.txt V6-file
 *	3. cpout <V6_file_name> <external_destination_file_path>
 *		SAMPLE: cpout V6-file /project2/largefile
 *	4. mkdir <directory_name>
 *		SAMPLE: mkdir v6-dir
 *	5. rm <directory_name>
 *		SAMPLE: rm v6-file
 *	5. q
 *		save(write to superblock) and quit.
 *      #Exit the child process by typing 'q' command
**/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<libgen.h>
#include<fcntl.h>
#include<sys/types.h>
#include<unistd.h>
#include<math.h>
#include<errno.h>
#include<time.h>

//SuperBlock Structure
typedef struct{
unsigned short isize;
unsigned short fsize;
unsigned short nfree;
unsigned short free[100];
unsigned short ninode;
unsigned short inode[100];
unsigned short time[2];
char flock;
char ilock;
char fmod;
}superBlock;

superBlock sb;

//Inode Structure
typedef struct{
unsigned short flags;
char nlinks;
char uid;
char gid;
char size0;
unsigned short size1;
unsigned short addr[8];
unsigned short actime[2];
unsigned short modtime[2];
}iN;
iN iNode;

//Directory Structure
typedef struct{
unsigned short inode;
char filename[14];
}dir;
dir dir1,newdir;

//Global constants
int dircounter = 0;
const unsigned short BLOCK_SIZE = 512;
const unsigned short ISIZE = 32;
const unsigned short inode_alloc = 0100000;
const unsigned short plainfile = 000000;
const unsigned short largefile = 010000;
const unsigned short directory = 040000;

//global variables
struct tm *loc_time;
int fd ;                //file descriptor
unsigned short chainarray[256];         //array used while chaining data blocks.

void writeiNode(int fd, iN inode, unsigned int inumber){
        //sb.ninode--;
        lseek(fd,1024+(inumber*32),SEEK_SET);
  write(fd,&inode,32/*sizeof(iNode)*/);
}


//function to write to an inode given the inode number
void inodewriter(iN inodeinstance, unsigned int inodenumber, int fd)
{
int bytes_written;
lseek(fd,2*BLOCK_SIZE+inodenumber*ISIZE,0);
if((bytes_written=write(fd,&inodeinstance,ISIZE)) < ISIZE)
        printf("\n Error in writing inode number : %d", inodenumber);

}

//function to read integer array from the required block
void blockreaderint(int *target, unsigned int blocknum, int fd)
{
        if (blocknum > sb.isize + sb.fsize + 2)
                printf(" Block number is greater than max file system block size for reading\n");
        else{
                lseek(fd,blocknum*BLOCK_SIZE,SEEK_SET);
                read(fd, target, BLOCK_SIZE);
        }
}


//Allocating block numbers to free list
void add_free_block(int blockNumber, int fd){
        int i;
        if(blockNumber == sb.isize+2){
                lseek(fd,512*(99),SEEK_CUR);
        }
        else{
                lseek(fd,51200,SEEK_CUR);
        }
        while(sb.nfree != 100){
                sb.free[sb.nfree] = blockNumber;
                sb.nfree++;
                blockNumber++;
        }
		write(fd,&sb.nfree,sizeof(sb.nfree));
        write(fd,sb.free,sizeof(sb.free));
        sb.nfree = 0;
        for(i = 0; i < 100;i++){
                 sb.free[i] = 0;
        }
}
unsigned short allocatedatablocknew()
{
	unsigned short block;

	sb.nfree--;

	block = sb.free[sb.nfree];

	if (sb.nfree == 0)
	{
		int n=0;
		blockreaderint(chainarray, block,fd);
		sb.nfree = chainarray[0];
		for(n=0; n<100; n++)
				sb.free[n] = chainarray[n+1];
	}
return block;
}


void allocateDataBlock(int fd){
        unsigned int block;
        int n=0;
        unsigned short buff[512];
        sb.nfree--;
        block = sb.free[sb.nfree];
        if(block == 0){
                printf("The block is zero and no blocks available");
                exit(0);
        }
        if(sb.nfree == 0){
                lseek(fd,block*512,SEEK_SET);
                read(fd, buff, 512);
                sb.nfree = buff[0];
                for(n=0; n<100; n++)
                        sb.free[n] = buff[n+1];
        }
}

void create_root()
{
	unsigned short i = 0;
	unsigned short bytes_written;
	unsigned short datablock = allocatedatablocknew();
	for (i=0;i<14;i++)
			newdir.filename[i] = 0;

	newdir.filename[0] = '.';                       //root directory's file name is .
	newdir.filename[1] = '\0';
	newdir.inode = 1;                               // root directory's inode number is 1.

	iNode.flags = 0100000 | directory | 000077;     // flag for root directory
	iNode.nlinks = 2;
	iNode.uid = '0';
	iNode.gid = '0';
	iNode.size0 = '0';
	iNode.size1 = ISIZE;
	iNode.addr[0] = datablock;

	for (i=1;i<8;i++)
			iNode.addr[i] = 0;

	iNode.actime[0] = 0;
	iNode.modtime[0] = 0;
	iNode.modtime[1] = 0;

	inodewriter(iNode, 0,fd);

	lseek(fd, datablock*BLOCK_SIZE, SEEK_SET);

	//filling 1st entry with .
	if((bytes_written = write(fd, &newdir, 16)) < 16)
			printf("\n Error in writing root directory \n ");

			newdir.filename[1] = '..';
			newdir.filename[2] = '\0';
			// filling with .. in next entry(16 bytes) in data block.

	if((bytes_written = write(fd, &newdir, 16)) < 16)
			printf("\n Error in writing root directory ");
}


//Initializing the V6-file
int initialize_fs(char* path, unsigned short total_blcks,unsigned short total_inodes )
{

char buffer[BLOCK_SIZE];
unsigned short bytes_written;
int dataBlock_size;
int blockcounter = 0;
int nextFree = 0;
unsigned short i = 0;

if((total_inodes%16) == 0)
        sb.isize = total_inodes/16;
else
        sb.isize = (total_inodes/16) + 1;

sb.fsize = total_blcks;

if((fd = open(path,O_RDWR|O_CREAT,0600))== -1){
        printf("\n open() failed with error [%s]\n",strerror(errno));
        return 1;
}

for (i = 0; i<100; i++)
        sb.free[i] =  0;                //initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.

sb.nfree = 0;
sb.ninode = total_inodes;
for (i=0; i < total_inodes; i++)
        sb.inode[i] = i;                //initializing inode array to store inumbers.

sb.flock = 'f';                                 //flock,ilock and fmode are not used.
sb.ilock = 'i';                                 //initializing to fill up block
sb.fmod = 'm';
sb.time[0] = 0;
sb.time[1] = 0;
lseek(fd,BLOCK_SIZE,SEEK_SET);

// Writing to super block
if((bytes_written =write(fd,&sb,BLOCK_SIZE)) < BLOCK_SIZE)
{
        printf("\nERROR : error in writing the super block");
        return 0;
}

//pointing to 3rd block where inode starts
lseek(fd,BLOCK_SIZE*2,SEEK_SET);

// writing zeroes to all inodes in ilist
for (i=0; i<BLOCK_SIZE; i++)
        buffer[i] = 0;
for (i=0; i < sb.isize; i++)
        write(fd,buffer,BLOCK_SIZE);

			dataBlock_size = total_blcks - sb.isize - 2;

			while(dataBlock_size >= 100){
				if(blockcounter == 0){
						add_free_block(2 + sb.isize, fd);
				}
				else{
						add_free_block(2 + sb.isize +(100*blockcounter -1),fd);
				}
				blockcounter++;
				dataBlock_size -= 100;
			}
			nextFree = 2 + sb.isize +(100*blockcounter)-1;
			lseek(fd, 512*(nextFree),SEEK_CUR);
			while(dataBlock_size > 0 && dataBlock_size < 100){
					sb.free[sb.nfree] = nextFree;
					dataBlock_size--;
					nextFree++;
					sb.nfree++;
			}

// Make root directory
create_root();
return 1;
}

//function to write character array to the required block
void blockwriterchar(char *target, unsigned int blocknum, int fd)
{
        int bytes_written;
        if (blocknum > sb.isize + sb.fsize + 2)
                printf(" Block number is greater than max file system block size for writing\n");
        else{
                lseek(fd,blocknum*BLOCK_SIZE,SEEK_SET);
                if((bytes_written=write(fd, target, BLOCK_SIZE)) < BLOCK_SIZE)
                        printf("\n Error in writing block number : %d", blocknum);
        }
}

//function to read character array from the required block
void blockreaderchar(char *target, unsigned int blocknum, int fd)
{
         if (blocknum > sb.isize + sb.fsize + 2)
                printf(" Block number is greater than max file system block size for reading\n");
        else{
                lseek(fd,blocknum*BLOCK_SIZE,SEEK_SET);
                read(fd, target, BLOCK_SIZE);
        }
}

//function to write integer array to the required block
void blockwriterint(unsigned int *target, unsigned int blocknum, int fd)
{
                int bytes_written;
        if (blocknum > sb.isize + sb.fsize + 2)
                printf(" Block number is greater than max file system block size for writing\n");
                else{
        lseek(fd,blocknum*BLOCK_SIZE,SEEK_SET);
        if((bytes_written=write(fd, target, BLOCK_SIZE)) < BLOCK_SIZE)
                        printf("\n Error in writing block number %d", blocknum);
                }
}

void mkdir(int fd,char *filename,unsigned int newinode){
        int blocks_read;
        unsigned int parentinum = 1;                   //since parent is always root directory for this project, inumber is 1.
        char buffertemp[512];
        int i =0;
        unsigned int block_num = sb.free[sb.nfree];			
        strncpy(newdir.filename,filename,14);           //string copy filename contents to directory structure's field
        newdir.inode = newinode;
        lseek(fd,2*512,0);
        iNode.nlinks++;
        // set up this directory's inode
        iNode.flags = 0100000 | 040000 | 000777;
        iNode.nlinks = 2;
        iNode.uid = '0';
        iNode.gid = '0';
        iNode.size0 = '0';
        iNode.size1 = 0;
        for (i=1;i<8;i++)
                    iNode.addr[i] = 0;
        iNode.addr[0] = block_num;
        iNode.actime[0] = 0;
        iNode.modtime[0] = 0;
        iNode.modtime[1] = 0;
        lseek(fd,2*512+(newinode*sb.isize),0);
        if((write(fd,&iNode,sb.isize)) < sb.isize)
                printf("\n Error in writing inode number : %d", newinode);
        lseek(fd,2*512,0);
        blocks_read = read(fd,&iNode,sizeof(iNode));

        iNode.nlinks++;

        if(directorywriter(fd,iNode, newdir))
                return;
        for (i=0;i<512;i++)
                        buffertemp[i] = 0;
        
		// copying to inode numbers and filenames to directory's data block for ".".
        memcpy(buffertemp, &newinode, sizeof(newinode));            //memcpy(used for fixed width character array inbuilt function copies n bytes from memory area newinode to memory  area buffertemp
        buffertemp[2] = '.';
        buffertemp[3] = '\0';
        
		// copying to inode numbers and filenames to directory's data block for ".."
        memcpy(buffertemp+16, &parentinum, sizeof(parentinum));     //memcpy(used for fixed width character array inbuilt function copies n bytes from memory area newinode to memory  area buffertemp
        buffertemp[18] = '.';
		buffertemp[19] = '.';
        buffertemp[20] = '\0';
        lseek(fd,block_num*512,0);
        if((write(fd, buffertemp, 512)) < 512)
                printf("\n Error in writing block number : %d", block_num);

        printf("\n Directory created \n");
 }

int directorywriter(int fd,iN rootinode, dir dir)
{       iN inode;
        int duplicate =0;               //to find duplicate named directories.
        unsigned short addrcount = 0;
        char dirbuf[512];
        int i=0;
        for (addrcount=0;addrcount <= 7;addrcount++)
        {
                lseek(fd,rootinode.addr[addrcount]*512,SEEK_SET);
                for (i=0;i<32;i++)
                {
                        read(fd, &dir1, 16);
                        if(strcmp(dir1.filename,dir.filename) == 0)         //check for duplicate named directories
                        {
							printf("Cannot create directory.The directory name already exists.\n");
							duplicate=1;
							break;
                        }
                }
        }
        if(duplicate !=1)
        {       lseek(fd,2*BLOCK_SIZE,SEEK_SET);
                read(fd,&inode,32);
                for (addrcount=0;addrcount <= 7;addrcount++)                    //for each of the address elements ( addr[0],addr[1] till addr[7]), check which inode is not allocated
                {
                        lseek(fd,inode.addr[addrcount]*512,SEEK_SET);
                        read(fd, dirbuf, 512);
                        for (i=0;i<32;i++)                                                                              //Looping for each directory entry (512/16 = 32 entries in total, where 512 is block size and 16 bytes is directory entry size)
                        {       
                                if (dirbuf[16*i] == 0) // if inode is not allocated
                                {
                                memcpy(dirbuf+16*i,&dir.inode,sizeof(dir.inode));
								memcpy(dirbuf+16*i+sizeof(dir.inode),&dir.filename,sizeof(dir.filename));             //using memcpy function to copy contents of filename and inode number, to store it in directory entry.
                                lseek(fd,inode.addr[addrcount]*512,SEEK_SET);
                                if((write(fd,dirbuf, 512)) < 512)
                                        printf("\n Error in writing block number : %d", inode.addr[addrcount]);
                                return duplicate;
                                }
                        }
                }
        }
        return duplicate;
}


//function that creates indirect blocks. handles large file (file size > 4 KB)
void indirectblocks(int fde,int block_num,int fd)
{
printf("\n----------------makeIndirectBlocks------------------\n");
char reader[BLOCK_SIZE];
unsigned int indirectblocknum[256];              //integer array to store indirect blocknumbers
int i=0;
int j=0;
int bytes_read;
int blocks_written = 0;
int extfilesize = 8 * BLOCK_SIZE;              //filesize is initialized to small file size since data would have been read upto this size.
for(i=0;i<8;i++)
indirectblocknum[i] = iNode.addr[i];            //transfer existing block numbers in addr[] array to new temporary array
 sb.nfree--;
 sb.free[sb.nfree];
iNode.addr[0] = sb.free[sb.nfree];              //allocate a data block which will be used to store the temporary integer array of indirect block numbers

for(i=1;i<8;i++)
iNode.addr[i] = 0;

i=8;
while(1)
		{
			lseek(fde,i*512,SEEK_SET);
			if((bytes_read=read(fde,reader,512)) != 0 )
			{
				sb.nfree--;
				indirectblocknum[i] = sb.free[sb.nfree];  //allocate a data block which will be used to store the temporary integer array of indirect block numbers

				lseek(fd,indirectblocknum[i]*BLOCK_SIZE,SEEK_SET);
				if(write(fd, reader, BLOCK_SIZE) < BLOCK_SIZE)
					printf("\n Error in writing block number : %d", indirectblocknum[i]);
				i++;

				 if(bytes_read < BLOCK_SIZE)
				{
					blockwriterint(indirectblocknum, iNode.addr[j],fd);
					printf("Large File copied\n");
					printf("File size = %d bytes\n",iNode.size1);
					break;
				}
				blocks_written++;

				if(i>255 && j<=7)
				{
					blockwriterint(indirectblocknum, iNode.addr[j],fd);
					sb.nfree--;
					iNode.addr[++j] = sb.free[sb.nfree];
					i=0;
					blocks_written=0;
				}
	   
				if(j>7)
				{
					printf("This file copy is not supported by the file system as the file is very large\n");
					break;
				}
			}
			// When bytes returned by read() system call is 0,
			// reading and writing are complete. Print file size in bytes and exit
			else
			{
				blockwriterint(indirectblocknum, iNode.addr[j],fd);
				printf("Large File copied\n");
				printf("File size = %d bytes\n",iNode.size1);
				break;
			}

		}
}

void freeblock(unsigned short block)
{
sb.free[sb.nfree] = block;
++sb.nfree;
}

void cpout(char* src, char* targ, int fd){
                int indirect = 0;
                int found_dir = 0;
                int src_inumber=0;
                char reader[512];                                               //reader array to read characters (contents of blocks or file contents)
                int reader1[512];                                               //reader array to read integers (block numbers contained in add[] array)
                int bytes_read;
                int targfd;
                int i=0;
                int j=0;
                int addrcount=0;
                int total_blocks=0;
                int remaining_bytes =0;
                int indirect_block_chunks = 0;                                          //each chunk of indirect blocks contain 512 elements that point to data blocks
                int remaining_indirectblks=0;
                int indirectblk_counter=0;
                int bytes_written=0;

                //open or create external file(target file) for read and write
                 if((targfd=(open(targ, O_RDWR | O_CREAT, 0600))) < 0)
                {
                        printf("\nerror opening file: %s\n",targ);
                        return;
                }
                lseek(fd,2*512,SEEK_SET);
                read(fd,&iNode,32);
			    //find the source V6 file in the root directory
                for (addrcount=0;addrcount <= 7;addrcount++)
                {
					if(found_dir !=1)
					{
					lseek(fd,(iNode.addr[addrcount]*512), SEEK_SET);
					for (i=0;i<32;i++)
						{           if(found_dir !=1)
									{
										read(fd, &dir1, 16);
										printf("\nthe file name is %s",dir1.filename);
										if(strcmp(dir1.filename,src) == 0)
										{
										src_inumber = dir1.inode;
										printf("The inumber is %d",src_inumber);
										found_dir =1;
										}
									}
						}
					}
                }

                if(src_inumber == 0)
                {
					printf("File not found in the file system. Unable to proceed\n");
					return;
                }
                lseek(fd, (2*512 + 32*src_inumber), SEEK_SET);
                read(fd, &iNode, 32);

                //check if file is directory. If so display information and return.
                if(iNode.flags & directory)
                {
					printf("The given file name is a directory. A file is required. Please retry.\n");
					return;
                }

                //check if file is a plainfile. If so display information and return.
                if((iNode.flags & plainfile))
                {
					printf("The file name is not a plain file. A plain file is required. Please retry.\n");
					return;
                }
                //check if file is a large file
                if(iNode.flags & largefile)
                {
					indirect = 1;
                }

                total_blocks = (int)ceil(iNode.size1 / 512.0);
                remaining_bytes = iNode.size1%512;
                //read and write small file to external file
                if(indirect == 0)                 //check if it is a small file. indirect = 0 implies the function that makes indirect blocks was not called during cpin.
                {
                printf("\nfile size = %d \n",iNode.size1);

                for(i=0 ; i < total_blocks ; i++)
				{
					lseek(fd,(iNode.addr[i])*BLOCK_SIZE,SEEK_SET);
					read(fd, reader, BLOCK_SIZE);
					//if counter reaches end of the blocks, write remaining bytes(bytes < 512) and return.
					if( i == (total_blocks - 1))
					{
						write(targfd, reader, remaining_bytes);
						printf("Copied to external file Successfully \n");
						return;
					}
					write(targfd, reader, 512);
					}
                }
                //read and write large file to external file
                if(indirect == 1)                       //check if it is a large file. indirect = 1 implies the function that makes indirect blocks was called during cpin.
                {
                total_blocks = iNode.size1/ 512;
                indirect_block_chunks = (int)ceil(total_blocks/512.0);  //each chunk of indirect blocks contain 512 elements that point to data blocks
                remaining_indirectblks = total_blocks%512;
                printf("file size = %d \n",iNode.size1);

                //Loop for chunks of indirect blocks
                for(i=0 ;i < indirect_block_chunks; i++)
                {
					blockreaderint(reader1,iNode.addr[i], fd);                              //store block numbers contained in addr[] array in integer reader array )

					//if counter reaches last chunk of indirect blocks, program loops the remaining and exits after writing the remaining bytes
					if(i == (indirect_block_chunks - 1))
					total_blocks = remaining_indirectblks;
					for(j=0; j < 512 && j < total_blocks; j++)
					{

						blockreaderchar(reader,reader1[j],fd);                  //store block contents pointed by addr[] array in character  reader array )
						if((bytes_written = write(targfd, reader, 512)) == -1)
						{
							printf("\n Error in writing to external file\n");
							return;
						}
						if( j == (total_blocks - 1))
						{
							write(targfd, reader, remaining_bytes);
							printf("Contents were transferred to external file\n");
							return;
						}
					}
				}
		}
}

//getting free inode only allocation performed
//if inode reaches 0, error caught but cannot proceed from that point.
unsigned short allocateinode()
{
unsigned short inumber;
unsigned int i = 0;
sb.ninode--;
inumber = sb.inode[sb.ninode];
return inumber;
}

int cpin(int fd,char *argv1, char *argv2){
        unsigned short inumber, num;
        char buffer[512];
        int i=0,r,*p,j,indirect=0;
        int externFile = open(argv1,O_RDONLY/*,S_IRWXU*/);
        int internFile = open(argv2,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
        if (externFile < 0){
                     perror ("\n not open");
                     return 1;
         }
         off_t fsize;
                 fsize = lseek(externFile, 0, SEEK_END);
         if(sb.ninode>0){
                sb.nfree--;
                inumber = allocateinode();
                iNode.flags = 0100000;
                iNode.size0 = 0;
                iNode.size1 = fsize;
        }
        newdir.inode = inumber;
        memcpy(newdir.filename,argv2,strlen(argv2));
        lseek(externFile,0,SEEK_SET);
        //This is for small files.
        printf("The file size is %d", fsize);
                while(fsize > 0){
                        for(j = 0; j < 512;j++){
                                    buffer[j] = '\0';
                        }
						
                        if(sb.nfree == 0){
                                allocateDataBlock(fd);
                        }
                        lseek(externFile,i*512,SEEK_SET);
                        r = read(externFile,buffer,512);
                        if(r < 0){
                                printf("Error occured %d",r);
                                exit(0);
                        }
                        if(r<512 && i<8){
                                printf("Small file copied\n");
                                printf("File size = %d bytes\n",fsize);
                                break;
                        }

						//Read the 512 bytes, get the free block number and add it to the addr array and lseek till 512*blocknumber and copy the contents into it.
                        iNode.addr[i] = sb.free[sb.nfree];
                        lseek(fd,sb.free[sb.nfree]*512,SEEK_SET);
                        write(fd,buffer,512);
                        fsize -= 512;
                        sb.nfree--;
                        i++;
                        if(i>7){
                              indirectblocks(externFile,iNode.addr[0],fd);
                              indirect = 1;
                              break;
                        }
                }
        if(indirect == 1){
                iNode.flags = iNode.flags | largefile;
        }
        writeiNode(fd,iNode, inumber);
        directorywriter(fd,iNode, newdir);

}

//To remove the file and the directory entry. Free the data and inode blocks and add them to free list.
void removeDir(char *argv1, int fd){
        iN inode ,in;
        int duplicate =0;
        unsigned short addrcount = 0;
        int i=0,j;
        char dirbuff[16];
        char iNodebuff[32];
        char filebuff[512];
        for(j=0;j<16;j++)
        dirbuff[j] = 0;
        for(j=0;j<32;j++)
        iNodebuff[j] = 0;
        for(j=0;j<512;j++)
        filebuff[j] = 0;
        lseek(fd,2*512,SEEK_SET);
        read(fd,&inode,32);
        for (addrcount=0;addrcount <= 7;addrcount++)
        {
			for (i=0;i<32;i++)
			{
				lseek(fd,inode.addr[addrcount]*BLOCK_SIZE+i*16,SEEK_SET);
				read(fd, &dir1, 16);
				if(strcmp(dir1.filename,argv1) == 0)
				{
				write(fd,dirbuff,16);
				lseek(fd,1024+dir1.inode*32,SEEK_SET);
				read(fd,&in,32);
				for(j=0;j<7;j++){
						lseek(fd,in.addr[j]*BLOCK_SIZE,BLOCK_SIZE);
						write(fd,filebuff,BLOCK_SIZE);
				}
				write(fd,iNodebuff,32);
				sb.nfree++;
				sb.free[sb.nfree] = inode.addr[addrcount];
				sb.ninode++;
				sb.inode[sb.ninode] = dir1.inode;
				inode.addr[addrcount] = 0;
				dir1.inode = 0;
				dir1.filename[0] = '\0';
				printf("Successfully removed the file\n");
				}
			}
        }
}

int main(int argc, char *argv[]){
int fsinit = 0;
char input[256];
char *parser,*parser1;
unsigned short n = 0;
char dirbuf[BLOCK_SIZE];
unsigned short i =0;
unsigned short bytes_written;
unsigned short number_of_blocks =0, number_of_inodes=0;

char *filepath = getenv("HOME");
char *home1 = filepath;
char *path = argv[1];
strcat(filepath,path);

        if(access(filepath, F_OK) != -1)
        {
                int fd = open(filepath,O_RDONLY|O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
                if (fd == -1){
                        printf("\n filesystem open() failed with error [%s]\n",strerror(errno));
                        return 1;
                }
        printf("filesystem already exists and the same will be used.\n");
        fsinit=1;
		}

while(1)
{
        printf("\n{V6FileSystemCommand~}\n");
        scanf(" %[^\n]s", input);
        parser = strtok(input," ");
         char *num1, *num2;
        if(strcmp(parser, "initfs")==0)
        {
			num1 = strtok(NULL, " ");
			num2 = strtok(NULL, " ");

			if (!num1 || !num2)
					printf(" All arguments(path, number of inodes and total number of blocks) have not been entered\n");
			else
			{
					number_of_blocks = atoi(num1);
					number_of_inodes = atoi(num2);

					if(initialize_fs(filepath,number_of_blocks, number_of_inodes))
					{
							printf("The file system is initialized\n");
							fsinit = 1;
					}
					else
					{
							printf("Error initializing file system. Exiting... \n");
							return 1;
					}
			}
			parser = NULL;
        }
		
		 else if(strcmp(parser, "q")==0)
				{

					lseek(fd,BLOCK_SIZE,0);

					 if((bytes_written =write(fd,&sb,BLOCK_SIZE)) < BLOCK_SIZE)
					 {
						printf("\nERROR : error in writing the super block");
						return 1;
					 }
				printf("[Success]: Saved all changes\n");
				return 0;
				}
				
		 else if(strcmp(parser,"cpin") == 0){
					num1 = strtok(NULL, " ");
					num2 = strtok(NULL, " ");
					int externFile = open(num1,O_WRONLY,S_IRWXU);
					if (externFile < 0){
							printf("Error: No file found");
					}
					int v6File = open(num2,O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
					cpin(fd,num1,num2);
		} 

		else if(strcmp(parser,"cpout") == 0){
					num1 = strtok(NULL, " ");
					num2 = strtok(NULL, " ");
					if(!num1 || !num2 )
							printf("Required file names(source and target file names) have not been entered. Please retry\n");
					else
					{
						cpout(num1,num2,fd);
					}
		 } 
		 
		 else if(strcmp(parser,"mkdir") == 0){
					num1 = strtok(NULL, " ");
					parser1 = strtok(num1, "/");
					while(parser1 != NULL){
							 sb.ninode--;
							 mkdir(fd,parser1,sb.inode[sb.ninode]);
							 parser1 = strtok(NULL,"/");

					}
		 }
		 
		else if(strcmp(parser,"rm") == 0){
								num1 = strtok(NULL, " ");
								removeDir(num1,fd);
		}
	}
}