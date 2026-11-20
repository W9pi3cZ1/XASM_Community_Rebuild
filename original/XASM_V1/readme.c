#include<stdio.h>
#include<conio.h>
int main(){
	FILE *fp;
	int i=0;
	char c=0;
	fp=fopen("./readme.md","r");
	while((c=fgetc(fp))!=EOF){
		printf("%c",c);
		if(c==10){
			getch();
		}
	}
	return 0;
}