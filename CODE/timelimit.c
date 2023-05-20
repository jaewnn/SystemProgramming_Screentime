#include "timelimit.h"

// you should sudo instruction to use this functions!
 
// split 함수
void splitString(char* str,char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH],int* numTokens){
	char* token = strtok(str,";\n");

	while(token != NULL){
		if(*numTokens >= MAX_TOKENS){
			break;
		}

		strcpy(tokens[*numTokens],token);
		(*numTokens)++;

		token = strtok(NULL,";\n");
	}
}


void execute_remove(char* program_path){
	FILE* fp = fopen("execute_remove.txt","a");
    	// 실행 권한을 제거할 모드
    	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    	// 실행 권한 제거
    	int result;
	
	if((result = chmod(program_path, mode))== -1){
        // 실행 권한 제거 실패
        	perror("chmod");
		return;
    	}

	// 실행 권한 제거한 프로세스의 path -> 파일에 기록
	fprintf(fp,"%s\n",program_path); 
	fclose(fp);
}

void execute_recover(){ // 실행 권한 복구
	FILE* fp = fopen("execute_remove.txt","r");
	if(fp == NULL){
		perror("Error opening file");
		return;
	}

	fseek(fp,0,SEEK_END);
	long file_size = ftell(fp);
	if(file_size == 0){ // case that file information is zero
		printf("NO executable file to recover\n");
		fclose(fp);
		return;
	}
	
	rewind(fp); // file pointer go to first

	char line[MAX_LINE_LENGTH];
	struct stat file_stat;

	while(fgets(line,sizeof(line),fp)){
		line[strcspn(line,"\n")] = '\0'; // remove the newline character

		if(stat(line,&file_stat)==0){
			mode_t current_permissions =file_stat.st_mode;

			current_permissions |= S_IXUSR | S_IXGRP | S_IXOTH; 

			chmod(line, current_permissions); // 실행권한 복구
			time_t timer = time(NULL);
			struct tm* t = localtime(&timer);
			printf("%d/%d/%d %d:%d:%d\tpermission is recovered : %s\n",
					t->tm_hour+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,line);
		}
		else{
			perror("stat");
			break;
		}
	}

	fclose(fp);
	// 파일 권한 복구 완료했으므로 파일 내용 삭제
	FILE* file = fopen("execute_remove.txt", "w"); // 파일을 쓰기 모드로 열기 (기존 내용 삭제)
    	if (file != NULL) {
        	fclose(file); // 파일 닫기
    	}
	else{
		perror("Error opening file");
	}

	refresh();
}

void time_limit(){
	FILE* LT = fopen("left_time.log","r"); // 시간제한 파일을 읽어옴
	FILE* tf = fopen("temp.log","w");
	char line[MAX_LINE_LENGTH];
	char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];
	int numTokens = 0;
	
	if(LT == NULL){
		printf("left_time cannot open file\n");
		return;
	}
	if(tf == NULL){
		printf("temp cannot open temp file\n");
		return;
	}

	fseek(LT,0,SEEK_END);
	long file_size = ftell(LT);
	if(file_size == 0){ // case that file information is zero
		fclose(LT);
		fclose(tf);
		return;
	}
	
	rewind(LT); // file pointer go to first

	int lineCount = 0;	
	while(fgets(line,sizeof(line),LT)){
		lineCount++;
		if(lineCount<=1) continue; // two of initial line is skipped

		char temp[MAX_LINE_LENGTH];
		strcpy(temp,line);
		splitString(temp,tokens,&numTokens);


		if(atoi(tokens[2])<=0){
			if(kill((pid_t)atoi(tokens[1]),SIGTERM)==-1){
				printf("Cannot kill %s\n",tokens[0]);
				continue;
			}
			else{
				execute_remove(tokens[3]);
				time_t timer = time(NULL);
			        struct tm* t = localtime(&timer);
        			printf("%d/%d/%d %d:%d:%d\tkill and can't be executed : %s\n",
                        		t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,tokens[0]);

			}
		}
		else{
			fputs(line,tf);
		}
		numTokens = 0;
	}
	fclose(tf);
	fclose(LT);

	// original file remove
	remove("left_time.log");
	
	// change temp file to original file
	rename("temp.log","left_time.log");

	refresh();
	return;
}
