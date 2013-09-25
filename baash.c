#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

int readCommand(char* _argv[], char* _c);
int searchPath(char* _paths[]);
void searchFile(char* _arch, char* _paths[], char* _execPath);
int background(char* _argv[]);
int getPipe(char* _argv[], char* _argv1[], char* _argv2[]);
void exePipe(char* _argv1[], char* _argv2[], char* _paths[]);
int redirect(char* _argv[], char _fileName[]);
void outputB(char _fileName[]);
void inputB(char _fileName[]);

int main()
{
    int errorCH;
	int pid;
	int pidFlag;
	int pipeFlag;
	int backgroundFlag;
	int argC;
	char* argV[20];
	char* argv1[20];
	char* argv2[20];
	char executePath[256];
	char command [256];
	char hostname [20];
	gethostname(hostname,20);
	char* user;
	user = getlogin();
	int pathCounter;
	char *paths[20];

	pathCounter = searchPath(paths);
	printf("Baash en ejecución\n");

	while (1)
	{
		strcpy(command, "\n");
		pidFlag = 0;
		pipeFlag = 0;

		printf("[%s@%s:%s]$ ", user, hostname,getcwd(NULL,50));
		fgets(command,256,stdin);


		if(feof(stdin))
		{
			printf("\nExit with Ctrl-D \n");
			return 0;
		}

		if(!strcmp(command, "\n"))
		{
			continue;
		}

		else
		{
			argC = readCommand(argV, command);
			if(!strcmp(command,"exit"))
			{
				printf("Exit \n");
				break;
			}

			if(!strcmp(argV[0],"cd"))
			{
				errorCH = chdir(argV[1]);
//				errorCH = chdir("/proc/");
//				printf("------>  %d\n", errorCH);
				continue;
			}

			int redirFlag = 0;
			int doPipe = 0;
			char fileName[50];
			doPipe = getPipe(argV, argv1, argv2);
//			printf("doPipe --->  %d\n", doPipe);
			redirFlag = redirect(argV, fileName);
//            		printf("redirFlag --->  %d\n", redirFlag);
			backgroundFlag = background(argV);
//			printf("backgroundFlag --->  %d\n", backgroundFlag);
			if(backgroundFlag)
			{
				argV[argC-1] = NULL;
				argC--;
			}

			searchFile(argV[0], paths, executePath);
			if(executePath[0] == 'X')
				printf("File not found\n");
			else{
				pid = fork();
				if (pid<0) {
					perror("Child");
					exit(1);
				}
				else if (pid == 0)
				{
					if(redirFlag == 2)
					{
						outputB(fileName);
					}
					else if(redirFlag == 1)
					{
						freopen(fileName,"r",stdin);
					}
					else if(doPipe == 1)
					{
						exePipe(argv1, argv2, paths);
						pipeFlag = 1;
					}
					if(!pipeFlag)
					{
						execv(executePath, argV);
						perror(executePath);
						exit(1);
					}
				}
				else
				{
					pidFlag = -1;
				}
				if(backgroundFlag)
					waitpid(pid,&pidFlag,WNOHANG);
				else
					waitpid(pid,&pidFlag,0);
			}
		}
	}
	return 0;
}

int readCommand(char* _argv[], char* _c){
	int args = 0;

	_argv[0] = strtok(_c, " \n");
	for(args = 1; args < 20; args++){
		_argv[args] = strtok(NULL, " \n");
		if (_argv[args] == NULL)
			break;
	}
	return args;
}

int searchPath(char* _paths[]){
	int pathCounter;
	char* pathVar = getenv("PATH");

	_paths[0] = strtok(pathVar, ":");
	for(pathCounter = 1; pathCounter < 20; pathCounter++){
		_paths[pathCounter] = strtok(NULL,":");
		if (_paths[pathCounter] == NULL)
			break;
	}

	strtok(NULL,":");
	return pathCounter+1;
}

void searchFile(char* _arch, char* _paths[], char* _execPath){
	char returnPath[50];
	int result;
	char searchDir[50] = "";
	char* file;
	strcpy(returnPath, _arch);

	if(_arch[0] == '/' || (_arch[0] == '.' && _arch[1] == '.' && _arch[2] == '/')){
		char* dir;
		char* nextDir;
		int absolutPath = 0;

		if(_arch[0] == '/')
			searchDir[0] = '/';

		dir = strtok(_arch,"/");
		nextDir = strtok(NULL,"/");

		if(nextDir != NULL)
			strcat(searchDir,dir);
		else
		{
			nextDir = dir;
			absolutPath = 1;
		}

		while((nextDir != NULL) && !absolutPath)
		{
			dir = nextDir;
			nextDir = strtok(NULL,"/");
			strcat(searchDir,"/");
			if(nextDir != NULL)
				strcat(searchDir,dir);
		}
		file = dir;
	}

	else if(_arch[0] == '.' && _arch[1] == '/')
	{
		getcwd(searchDir, 50);
		strcat(searchDir,"/");
		file = strtok(_arch, "/");
		file = strtok(NULL,"/");
	}

	else
	{
		int i;
		char aux[50];
		for(i = 0; i < 20; i++)
		{
			if(_paths[i] == NULL)
				break;
			strcpy(aux,_paths[i]);
			strcat(aux,"/");
			strcat(aux,_arch);
			result = access(aux, F_OK);
			if(!result)
			{
				strcpy(_execPath, aux);
				return;
			}
		}
		_execPath[0] = 'X';
		return;
	}

	strcat(searchDir, file);
	result = access(searchDir, F_OK);
	if(!result)
		strcpy(_execPath, searchDir);
	else
		_execPath[0] = 'X';
}

int background(char* _argv[]){
	int i;
	for(i = 0; i < 20; i++)
	{
		if(_argv[i] == NULL)
			break;
	}
	if(!strcmp(_argv[i-1], "&"))
		return 1;
	return 0;
}

int getPipe(char* _argv[], char* _argv1[], char* _argv2[]){
	int indexArg, aux, indexArg2;

	for(indexArg = 0; _argv[indexArg] != NULL; indexArg++){
		aux = strcmp("|", _argv[indexArg]);
		if(aux == 0)
			break;
		_argv1[indexArg] = (char*) malloc ( strlen(_argv[indexArg] + 1) ) ;
		strcpy(_argv1[indexArg], _argv[indexArg]);
	}
	_argv1[indexArg] = '\0';

	if(_argv[indexArg] == NULL)
		return 0;
	indexArg++;

	for(indexArg2 = 0; _argv[indexArg] != NULL; indexArg2++)
	{

		if(_argv[indexArg] == NULL)
			break;
		_argv2[indexArg2] = (char*) malloc( strlen(_argv[indexArg] + 1) );
		strcpy(_argv2[indexArg2], _argv[indexArg]);
		indexArg++;
	}
	_argv2[indexArg2] = '\0';
	return 1;
}

void exePipe(char* _argv1[], char* _argv2[], char* _paths[]){
	char executePath[256];

	int fd[2];
	pipe(fd);
	if (fork()==0)
	{
		close(fd[0]);
		dup2(fd[1],1);
		close(fd[1]);
		searchFile(_argv1[0], _paths, executePath);
		execv(executePath, _argv1);
		perror(executePath);
		exit(1);
	}
	else
	{
		close(fd[1]);
		dup2(fd[0],0);
		close(fd[0]);
		searchFile(_argv2[0], _paths, executePath);
		execv(executePath, _argv2);
		perror(executePath);
		exit(1);
	}
}

int redirect(char* _argv[], char _fileName[])
{
	int i;
	for (i = 0; i < 20; i++)
	{

		if(_argv[i] == NULL)
		{
			_fileName = NULL;
			return 0;
		}
		else if (!strcmp(_argv[i], "<"))
		{
			strcpy(_fileName, _argv[i+1]);
			_argv[i] = NULL;
			_argv[i+1] = NULL;
			return 1;
		}
		else if (!strcmp(_argv[i], ">"))
		{
			strcpy(_fileName, _argv[i+1]);
			_argv[i] = NULL;
			_argv[i+1] = NULL;
			return 2;
		}
	}
	return 0;
}

void outputB(char _fileName[])
{
	int fid;
	int flags,perm;
	flags = O_WRONLY|O_CREAT|O_TRUNC;
	perm = S_IWUSR|S_IRUSR;

	fid = open(_fileName, flags, perm);
	if (fid<0)
	{
		perror("open");
		exit(1);
	}
	close(STDOUT_FILENO);
	if (dup(fid)<0)
	{
		perror("dup");
		exit(1);
	}
	close(fid);
}

void inputB(char _fileName[])
{
	int fid;
	int flags,perm;
	flags = O_RDONLY;
	perm = S_IWUSR|S_IRUSR;

	close(STDIN_FILENO);
	fid = open(_fileName, flags, perm);
	if (fid<0)
	{
		perror("open");
		exit(1);
	}
	if (dup(fid)<0)
	{
		perror("dup");
		exit(1);
	}
	close(fid);
}
