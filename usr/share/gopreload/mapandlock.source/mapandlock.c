#include "include.h"

void
mmapFile(char fileToMap[128])
{
	void		*src;
	struct	stat	statbuf;
	off_t		offset, pa_offset;
	size_t		length;
	int		file;

	if ((file = open(fileToMap, O_RDONLY))<0)
	{
		printf("\n\t\t-->(W) Warning: Can't open %s for MAPPING",fileToMap);
	}
	else
	{
	if (lstat(fileToMap, &statbuf) == -1 )
		exit(EXIT_FAILURE);
	
	else
	 {
	 offset = 0;
	 pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
	 length = statbuf.st_size;

	 if ((src =mmap(	NULL,
			length,
			PROT_READ | PROT_EXEC,
			MAP_SHARED | MAP_LOCKED | MAP_POPULATE,
			file,
			pa_offset)) == MAP_FAILED)
		{
		printf("\n\t\t\tmmap error for %",fileToMap);
		}
	 else 
		{
		printf("\n\t\t\t(OK) mmaped %s",fileToMap);
		}
	 }
	 close(file);
	}
	
}

int
munmapFile(FILE *fileUnMap)
{
//  int munmap(void *start, size_t length);

}

void
cacheList(char line[128])
{
	struct	stat	buf;
	FILE		*file;
	char		linea[128];

	if (lstat(line, &buf) <0 )
	{
		printf("\n LSTAT ERROR for %s\n",line);
		exit(0);
	}
	else if (!(S_ISREG(buf.st_mode)))
	{
		printf("\n %s wasnt a regular file\n",line);
		exit(0);
	}

	if ((file=fopen(line, "r")) != (FILE *)0)
	{
		printf("\n\t-------------------------");
		printf("\n\t|-->Open sub-list: %s",line);

		while ((fscanf(file, "%[^\n]", linea)) != EOF )
                {
		fgetc(file); /* Read in '\n' character */
		mmapFile(linea);
		}
	if (!(fclose(file)))
                  printf("\n\t-->Close sub-list  %s\n",line);
		printf("\n\t|-------------------------\n");

	}

}


int
main(int argc, char *argv[])
{
	struct	stat	buf;
	char		line[128];
	char		buffer[128];
	FILE		*fileLIST;

	if (argc != 2)
		{
			printf("Usage: %s <File_List>\n",argv[0]);
			exit(0);
		}
	else if (lstat(argv[1], &buf) <0)
		{
			printf("\n LSTAT ERROR\n");
			exit(0);
		}
	else if (!(S_ISREG(buf.st_mode)))
		{
			printf("\n %s wasnt a regular file\n",argv[1]); 
			exit(0);
		}
	
	if ((fileLIST=fopen(argv[1], "r")) != (FILE *)0)
	{
		printf("\nOpen list: %s",argv[1]);
	
		while ((fscanf(fileLIST, "%[^\n]", line)) != EOF )
		{
			fgetc(fileLIST); /* Read in '\n' character */
			printf("\n->Readed from list %s the sublist %s\n",argv[1],line);
			cacheList(line);
		}

		if (!(fclose(fileLIST)))
			printf("\nClose list %s\n",argv[1]);
	}
	printf("Done, sleeping Forever");
	printf("\n...\n");
	sleep(-1);
}

