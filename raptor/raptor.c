#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "data.pb-c.h"


Timetable * load_timetable(char * filename){
	FILE * ttfile;
	unsigned char * ttbuf;
	size_t ttbuf_sz;
	ttfile = fopen(filename,"r");
	fseek(ttfile,0L,SEEK_END);
	ttbuf_sz = ftell(ttfile);
	ttbuf = malloc(ttbuf_sz);
	if (!ttbuf){
		printf("Allocation failed\n");
		return NULL;	
	}
	rewind(ttfile);
	fread(ttbuf,ttbuf_sz,1,ttfile);

	return timetable__unpack(NULL,ttbuf_sz,ttbuf);
}

void search_con(Timetable * tt,uint32_t from,uint32_t to,uint32_t time){
	
}

int main(int argc, char ** argv){
	
	Timetable * timetable;
	timetable = load_timetable("tt.bin");
	if (!timetable){
		printf("Failed to load timetable\n");	
		return 1;
	}
	
	uint32_t from;
	uint32_t to;
	uint32_t time;

	if (argc < 4){
		printf("Usage: %s from to time\n",argv[0]);
		return 1;
	}

	from = atoi(argv[1]);
	to = atoi(argv[2]);
	time = atoi(argv[3]);

	printf("Searching from %d to %d at %d\n",from,to,time);

	search_con(timetable,from,to,time);	

	return 0;
}
