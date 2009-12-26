/*
Copyright (c) 2008 Francisco Mu�oz 'Hermes' <www.elotrolado.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of 
  conditions and the following disclaimer. 
- Redistributions in binary form must reproduce the above copyright notice, this list 
  of conditions and the following disclaimer in the documentation and/or other 
  materials provided with the distribution. 
- The names of the contributors may not be used to endorse or promote products derived 
  from this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* memfile: functions to read the Ogg file from memory */



#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>

static struct
{
char *mem;
int size;
int pos;
} file[4];

int f_close(int *f);

int mem_open(char * ogg, int size)
{
static int one=1;
int n;
if(one)
	{one=0;
	 for(n=0;n<4;n++) file[n].size=0;
	}

for(n=0;n<4;n++)
	{
	if(file[n].size==0) {file[n].mem=ogg;file[n].size=size;file[n].pos=0; return (0x666+n);}
	}
return -1;
}

int mem_close(int fd)
{
if(fd>=0x666 && fd<=0x669 ) // it is a memory file descriptor?
	{
	fd-=0x666;
    file[fd].size=0;
	return 0;
	}
else return f_close(&fd);

}

int f_read(void * punt, int bytes, int blocks, int *f)
{
int b;
int c;
int d;


if(bytes*blocks<=0) return 0;

blocks=bytes*blocks;
c=0;


while(blocks>0)
{
b=blocks;if(b>4096) b=4096;

if(*f>=0x666 && *f<=0x669 )
	{
	d=(*f)-0x666;
    if(file[d].size==0) return -1;
	if((file[d].pos+b)>file[d].size) b=file[d].size-file[d].pos;
	if(b>0)
		{memcpy(punt,file[d].mem+file[d].pos,b);
		file[d].pos+=b;
		}
	}
else
b=read( *f,((char *)punt)+c,b);

if(b<=0) {return c/bytes;}
c+=b;
blocks-=b;
}
return c/bytes;
}

int f_seek(int *f,long offset, int mode)
{
int k,d;
mode&=3;
if(*f>=0x666 && *f<=0x669)
	{
	d=(*f)-0x666;
	k=0;
	 
	if(file[d].size==0) return -1;

	if(mode==0)
		{
		if((offset)>=file[d].size) {file[d].pos=file[d].size;k=-1;}
		else 
        if((offset)<0) {file[d].pos=0;k=-1;}
		else file[d].pos=offset;
		}
	if(mode==1)
		{
		if((file[d].pos+offset)>=file[d].size) {file[d].pos=file[d].size;k=-1;}
		else 
        if((file[d].pos+offset)<0) {file[d].pos=0;k=-1;}
		else file[d].pos+=offset;
		}
	if(mode==2)
		{
		
		if((file[d].size+offset)>=file[d].size) {file[d].pos=file[d].size;k=-1;}
		else 
        if((file[d].size+offset)<0) {file[d].pos=0;k=-1;}
		else file[d].pos=file[d].size+offset;
		}

	}
else
k= lseek(*f,(int) offset,mode);

if(k<0) k=-1; else k=0;
return k;

}
int f_close(int *f)
{
int d;
if(*f>=0x666 && *f<=0x669)
	{
	d=(*f)-0x666;
     file[d].size=0;file[d].pos=0;
	if(file[d].mem) {/*free(file[d].mem);*/file[d].mem=(void *) 0;}
	return 0;
	}
	else return close( *f);
return 0;
}

long f_tell(int *f)
{	
int k,d;

if(*f>=0x666 && *f<=0x669)
	{
	d=(*f)-0x666;
	k=file[d].pos;
	}
	else k=lseek(* f,0,1);


return (long) k;

}
