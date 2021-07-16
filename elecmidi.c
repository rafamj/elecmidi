#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "elecmidi.h"

struct DataDumpType dd;

char* device =  "/dev/dmmidi1" ;
int channel=0;
int octave=4;

#define TRACK_SIZE 18725 //max size of bytes received
#define MAX_LINE 500

unsigned char ioBuffer[TRACK_SIZE+10]; //10=room for the header + EOX
char lineBufferCopy[MAX_LINE];

void sendCommand(unsigned char *command, long int cl, unsigned char *buffer, long int bl) {
   FILE *f = fopen(device, "w");
   if (f == NULL) {
      printf("Error: cannot open %s\n", device);
      exit(1);
   }
   for(int i=0;i<cl;i++) {
     fputc(command[i],f);
   }
   f = freopen(device, "r",f);
   int tries=0;
   buffer[0]=fgetc(f);
   while(buffer[0]!=0xf0 && tries++ < 10) { //wait for the sys ex message
     buffer[0]=fgetc(f);
   }
   if(buffer[0]==0xf0){
     fread(buffer+1, bl-1,1,f);
   } 
   fclose(f);
/*
   if(command[0]!=0xf0 || command[cl-1]!=0xf7) {
     printf("command mal formado %x %x\n",command[0],command[cl-1]);
   }
   if(buffer[0]!=0xf0 || buffer[bl-1]!=0xf7) {
     printf("respuesta mal formada %x %x\n",buffer[0],buffer[bl-1]);
   }
   printf("enviado comando %x respuesta %x\n",command[6],buffer[6]);
   */
}

/*
void deviceInquiry(){
   unsigned char data[6] = {0xf0, 0x7e, channel, 0x06, 0x01, 0xf7};
   unsigned char input[15];

   sendCommand(data,sizeof(data),input, sizeof(input));
   for(int i=0;i<15;i++) printf("%x ",input[i]);
   printf("\n");
}
*/


char *beat2String(int n) {
    switch(n){
      case 0: return "16";
      case 1: return "32";
      case 2: return "8 Tri";
      case 3: return "16 Tri";
    }
    return "";
}

char *key2String(int n) {
    switch(n){
      case 0: return "C";
      case 1: return "C#";
      case 2: return "D";
      case 3: return "D#";
      case 4: return "E";
      case 5: return "F";
      case 6: return "F#";
      case 7: return "G";
      case 8: return "G#";
      case 9: return "A";
      case 10: return "A#";
      case 11: return "B";
    }
    return "";
}

char *scale2String(int n) {
    switch(n){
      case 0: return "Chromatic";
      case 1: return "Ionian";
      case 2: return "Dorian";
      case 3: return "Phrygian";
      case 4: return "Lydian";
      case 5: return "Mixolidian";
      case 6: return "Aeolian";
      case 7: return "Locrian";
      case 8: return "Harm minor";
      case 9: return "Melo minor";
      case 10: return "Major Blues";
      case 11: return "minor Blues";
      case 12: return "Diminished";
      case 13: return "Com.Dim";
      case 14: return "Major Penta";
      case 15: return "minor Penta";
      case 16: return "Raga 1";
      case 17: return "Raga 2";
      case 18: return "Raga 3";
      case 19: return "Arabic";
      case 20: return "Spanish";
      case 21: return "Gypsy";
      case 22: return "Egyptian";
      case 23: return "Hawaiian";
      case 24: return "Pelog";
      case 25: return "Japanese";
      case 26: return "Ryuku";
      case 27: return "Chinese";
      case 28: return "Bass Line";
      case 29: return "Whole Tone";
      case 30: return "minor 3rd";
      case 31: return "Major 3rd";
      case 32: return "4th Interval";
      case 33: return "5th Interval";
      case 34: return "Octave";
    }
    return "";
}

char *offOn2String(unsigned char v){
  if(v==0) {
    return "Off";
  } else {
    return "On";
  }
}

void printError(char *e1,char *e2) {
  printf("%s %s\n",e1,e2);
}

int readInteger(char *line, char **remain){
  char *token;
  int n;
  token=strtok_r(line," ",remain);
  if(!token) {
    printError("Error reading integer. Blank space or end of line found",lineBufferCopy);
    exit(1);
  } 
  if(!isdigit(token[0]) && token[0]!='+' && token[0]!='-') {
    printError("Error reading integer. found ",token);
    printError("Error reading integer.",lineBufferCopy);
    exit(1);
  }
  n=atoi(token);

  return n;
}

long int readRange(char *line){
  char *token;
  char *number1;
  char *number2;
  int n1;
  int n2;
  long int range=0;

    token=strtok_r(line,",",&line);
    while(token) {
      number1=strtok_r(token,"-",&token);
      number2=token;
      //n1=atoi(number1);
      n1=readInteger(number1,&number1);
      if(number2[0]==0) {
        n2=n1;
      } else {
        //n2=atoi(number2);
        n2=readInteger(number2,&number2);
      }
      for(int i=n1;i<=n2;i++) {
            range |= (1<< (i-1)); 
      } 
      token=strtok_r(line,",",&line);
    }
    return range;
}

int readPart(char *line, char **remain){

  long int range;

  range=readRange(line);
  return range & 0xffff;
}

int printPart(int part) {
     printf("lastStep %d %d\n",part+1,(dd.part[part].lastStep+16)%17);
     printf("pattern %d ",part+1);
     for(int s=0; s<64; s++) {
       if(s%4==0) printf(" ");
       if(s%16==0) printf("  ");
       if (dd.part[part].step[s].onOff==1) {
         if(s>0 && dd.part[part].step[s-1].gateTime>=127) {
           printf("_");
	 } else {
           printf("*");
	 }
       } else {
         printf("x");
       }
     }
     printf("\n");

     for(int n=0;n-4;n++) {
       printf("#notes %d     ", n+1);
       for(int s=0; s<32; s++) {
         if(s%4==0) printf("  ");
         if(s%16==0) printf("  ");
           printf("%2d ",dd.part[part].step[s].note[n]);
       }
       printf("\n#notes %d @33 ", n+1);
       for(int s=32; s<64; s++) {
         if(s%4==0) printf("  ");
         if(s%16==0) printf("  ");
           printf("%2d ",dd.part[part].step[s].note[n]);
       }
       printf("\n");
     }
     
     printf("gateTime %d @1-32  ",part+1);
     for(int s=0; s<32; s++) {
       if(s%16==0) printf("  ");
       printf("%3d ",dd.part[part].step[s].gateTime);
     }
     printf("\ngateTime %d @33-64 ",part+1);
     for(int s=32; s<64; s++) {
       if(s%16==0) printf("  ");
       printf("%3d ",dd.part[part].step[s].gateTime);
     }
     printf("\n");
     printf("velocity %d @1-32  ",part+1);
     for(int s=0; s<32; s++) {
       if(s%16==0) printf("  ");
       printf("%3d ",dd.part[part].step[s].velocity);
     }
     printf("\nvelocity %d @33-64 ",part+1);
     for(int s=32; s<64; s++) {
       if(s%16==0) printf("  ");
       printf("%3d ",dd.part[part].step[s].velocity);
     }
     printf("\n\n");
}

int printPattern(char *line) {
   int partRange;
   char *token;
   char *remain;

    token=strtok_r(line," ",&remain);
    if(token==0) {
      partRange=0xffff;
    } else {
      partRange=readPart(token,&remain);
    }
   printf("name %s\n",dd.name);
   printf("length %x\n",dd.length+1);
   printf("\n");
   /*
   int tt=dd->tempo1+256*dd->tempo2;
   printf("tempo %d.%d\n", tt/10,tt%10);
   printf("swing %hd\n",dd->swing);
   printf("beat %s\n",beat2String(dd->beat));// 16, 32, 8 tri, 16 tri
   printf("key %s\n",key2String(dd->key));// 0-11 =C - B 
   printf("scale %s\n",scale2String(dd->scale));
   printf("chordset %x\n",dd->chordset+1);
   printf("play level %d\n",127-dd->playlevel);
   printf("Alternate 13-14 %s\n",offOn2String(dd->alternate1314));
   printf("Alternate 15-16 %s\n",offOn2String(dd->alternate1516));
   printf("Master FX %d\n",dd->masterFX[1]);
   */
//gate time TIE=255

   for(int part=0;part<16;part++) {
     if(partRange & 1) {
       printPart(part);
     }
     partRange = partRange>>1;
   }
   return 0;
}

int checkData(struct DataDumpType *dd){
    if(dd->header[0]!=0x50) {
      return -1;
    }
    if(dd->footer[0]!=0x50) {
      return -2;
    }

    return 0;
}

int decodeData(unsigned char *buffer,struct DataDumpType *dd){
  unsigned char *dest=(unsigned char *)dd;
  long int ni;
  long int no=0;
  unsigned char b7;

  for(ni=0;ni<TRACK_SIZE;ni++) {
    if(ni%8==0) {
      b7=buffer[ni];
    } else {
      dest[no]=buffer[ni];
      if(b7&1) {
        dest[no] +=128;
      }
      b7=b7>>1;
      no++;
    }
  }

  int error=checkData(dd);
  if(error) {
    printf("error %d in received data\n",error);
  }
  return error;
}

long int encodeData(struct DataDumpType *dd, unsigned char *buffer){
  unsigned char *orig=(unsigned char *)dd;
  long int ni;
  long int no=1;
  unsigned char b7=0;

  for(ni=0;ni<sizeof(*dd);ni++) {
    buffer[no]=(orig[ni]&0x7F);

    if(orig[ni] & 0x80) {
      b7 = b7 | 0x80;
    }
    b7= b7>>1;
    
    if(ni%7==6) {
      buffer[no-7]=b7;
      b7=0;
      no++;
    }
    no++;
  }
  return no+1;
}
/*
void patternDataDump(int pattern) {
    unsigned char data[10] = {0xf0, 0x42, 0x30 | channel, 0, 1, 0x23, 0x1c,
                             pattern & 0x7f, pattern>>7, 0xf7};
    struct currentPattern {
                           unsigned char header[7];
			   unsigned char patternLSB;
			   unsigned char patternMSB;
			   unsigned char buffer[TRACK_SIZE];
			  } input;

   
   sendCommand(data,sizeof(data),(char *)&input, sizeof(input));
   decodeData(input.buffer,&dd);
   processPattern(&dd);
}
*/

void currentPatternDataDump() {
    unsigned char data[8] = {0xf0, 0x42, 0x30 | channel, 0, 1, 0x23, 0x10,0xf7};
    struct currentPattern {
                           unsigned char header[7];
			   unsigned char buffer[TRACK_SIZE];
			   unsigned char eox;
			  } *input=(struct currentPattern *)ioBuffer;

   int ok=0;
   int tries=0;
   while(!ok && tries++ <3) {
     sendCommand(data,sizeof(data),(char *)input, sizeof(*input));
     if(input->header[6]==0x24) {
       printf("NACK received\n");
     } else {
       int res=decodeData(input->buffer,&dd);
       ok=(res==0);
     }
   }
   if(!ok) {
     printf("error receiving data\n");
     exit(1);
   }
   //processPattern(&dd);
}

void currentPatternDataSend() {
    unsigned char data[8];
    struct currentPattern {
                           unsigned char header[7];
			   unsigned char buffer[TRACK_SIZE];
			  } *output=(struct currentPattern *)ioBuffer;
   int ok=0;
   int tries=0;
   long int l;

   l=encodeData(&dd,output->buffer);
   l+=7; //header
   while(!ok && tries++ <5) {
     sendCommand(ioBuffer,l,data,sizeof(data));
     if(data[6]==0x24) {
       printf("NACK received\n");
     } else if(data[6]==0x23){
       ok=1;
     } else {
       printf("error sending data %x\n",data[6]);
     }
   }
}

void deleteSpaces(char *s){
char *p=s;

  while(*s) {
    if(*s!=' ' && *s!='\t') {
      *p++=*s;
    }
    s++;
  }
  *p=0;
}


int readStep0(char *line, char **remain){
  char *token;
  int step0;
  
  if(*remain[0]=='@') {
    token=strtok_r(*remain," ",remain);
    //step0=atoi(token+1);
    token++;
    step0=readInteger(token,&token);
  } else {
    return 1;
  }
  if(step0<1 || step0>64) {
    printError("Error reading step number",token);
    return -1;
  }
  return step0;

}

void jumpBlanks(char **line) {
  while (**line!=0 && (**line==' ' || **line=='\t')){
    (*line)++;
  }
}

int note2midi(char note, int octave, int alt) {
  int n;

  if(note>'B') octave--;
  n=(note-'A')*2;
  if(note>='C') n--;
  if(note>='F') n--;
  return 21 + n + alt + octave * 12;
}

int readStepNotes(char *notes,int part,int step){
  int n=0;
  int note;
  int alt;
  char c;
  if(notes[0]=='.') {
    return step;
  }
  while(n<4) {
    alt=0;
    note=toupper(*notes);
    if(note==0)  { //end of string. The remainder notes are put to 0
       while(n<4) {
         dd.part[part].step[step].note[n++]=0;
       }
       break;
    }
    if(note<'A' || note>'G') {
      printError("Error reading note",notes);
      return -1;
    }
    c=*++notes;
    if(isdigit(c)) {
      octave=c-'0';
      c=*++notes;
    } else if(c=='v') {
      if(octave>0) {
        octave--;
        c=*notes++;
      } else {
        printError("Error octave too low",notes);
        return -1;
      }
    } else if(c=='^') {
      if(octave<9) {
        octave++;
        c=*notes++;
      } else {
        printError("Error octave too high",notes);
        return -1;
      }
    }
    if(c=='+') alt=1; 
    if(c=='-') alt=-1; 
    if(alt) notes++;
    int midiNote=note2midi(note,octave,alt);
    midiNote++; //// why??
    if(midiNote>127) {
        printError("Error note too high",notes);
        return -1;
    }
    while(dd.part[part].step[step].onOff==0 && step<64) {
      step++; //search first non silence note
    }
    if(step<64) {
      //printf("putting note %d %d\n",n,midiNote); 
      dd.part[part].step[step].note[n]=midiNote;
    }
    n++;
  }
  step++;
  return step;
}

int readRepeat(char **s) {
  char *remain;

  *s=strtok_r(*s,"*",&remain);
  if(remain[0]==0) {
    return 1;
  } else {
    return readInteger(remain, &remain);
  }
}

int readNotes(char *line) {
  char *token;
  char *remain;
  int partRange;
  int step0=1;
  int step[16];
  int repeat=1;
  
  token=strtok_r(line," ",&remain);
  partRange=readPart(line,&remain);
  if(partRange==-1) {
    return -1;
  }
  jumpBlanks(&remain);

  step0=readStep0(remain,&remain);
  if(step0==-1) {
    return -2;
  }

  token=strtok_r(remain," ",&remain);
  repeat=readRepeat(&token);
  for(int i=0;i<16;i++) { 
    step[i]=step0-1;
  }
  int ok=1;
  int origPartRange=partRange;
  while(token!=NULL && ok) {
    while(repeat--) {
      partRange=origPartRange;
      for(int part=0; part<16; part++) {
        if(partRange&1) {
          step[part]=readStepNotes(token,part,step[part]);
	  if(step[part]>=64) {
	    ok=0;
	  }
          if(step[part]==-1) {
            printError("Error in notes ",line);
            return -4;
          }
        }
        partRange=partRange>>1;	
      }
    }
    char *ant=token;
    token=strtok_r(remain," ",&remain);
    if(token) {
      repeat=readRepeat(&token);
      if(token[0]=='=') {
        token=ant;
      }
    }
  }
  return 0;
}

int readName(char *line) {
    for(int i=0;i<17;i++) {
      char c=line[i];
      if(c==0) {
         dd.name[i]=0;    
	 return 0;
      }
      if(c>=32 && c<127) {
         dd.name[i]=line[i];    
      } else {
        printError("Error in name",line);
        return -3;
      }
    }
    dd.name[17]=0;    
    return 0;
}

int readGateTime(char *line) {
  int partRange;
  int gt;
  long int stepRange=0xFFFFFFFFFFFFFFFF;
  char *remain;
  char *token;

  token=strtok_r(line," ",&remain);
  partRange=readPart(token,&remain);
  if(partRange==-1) {////
    return -1;
  }
  jumpBlanks(&remain);
  token=strtok_r(remain," ",&remain);
  if(remain[0]==0) { //last number
    remain=token;
  } else if(token[0]=='@') {
    stepRange=readRange(token+1);
  } else {
    printError("Error in Gate Time","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  gt=readInteger(remain,&remain);
  if(gt==0) {
    printError("Error in Gate Time","");
    return -1;
  }
  long int origStepRange=stepRange;
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      stepRange=origStepRange;
      for(int step=0; step<64; step++) {
        if(stepRange & 1) {
          dd.part[part].step[step].gateTime=gt;
	}
        stepRange=stepRange>>1;
      }
    }
    partRange=partRange>>1;
  }
  return 0;
}

int readVelocity(char *line) {
  int partRange;
  int v;
  long int stepRange=0xFFFFFFFFFFFFFFFF;
  char *remain;
  char *token;

  token=strtok_r(line," ",&remain);
  partRange=readPart(token,&remain);
  if(partRange==-1) {////
    return -1;
  }
  jumpBlanks(&remain);
  token=strtok_r(remain," ",&remain);
  if(remain[0]==0) { //last number
    remain=token;
  } else if(token[0]=='@') {
    stepRange=readRange(token+1);
  } else {
    printError("Error in Velocity","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  v=readInteger(remain,&remain);
  if(v==0) {
    printError("Error in Velocity","");
    return -1;
  }
  long int origStepRange=stepRange;
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      stepRange=origStepRange;
      for(int step=0; step<64; step++) {
        if(stepRange & 1) {
          dd.part[part].step[step].velocity=v;
        }
        stepRange=stepRange>>1;
      }
    }
    partRange=partRange>>1;
  }
  return 0;
}

int readTranspose(char *line) {
  int partRange;
  int tr;
  long int stepRange=0xFFFFFFFFFFFFFFFF;
  char *remain;
  char *token;

  token=strtok_r(line," ",&remain);
  partRange=readPart(token,&remain);
  if(partRange==-1) {
    return -1;
  }
  jumpBlanks(&remain);
  token=strtok_r(remain," ",&remain);
  if(remain[0]==0) { //last number
    remain=token;
  } else if (token[0]=='@'){
    stepRange=readRange(token+1);
  } else {
    printError("Error in Transpose","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  tr=readInteger(remain,&remain);
  if(tr==0) {
    printError("Error in Transpose","");
    return -1;
  }
  long int origStepRange=stepRange;
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      stepRange=origStepRange;
      for(int step=0; step<64; step++) {
        if(stepRange & 1) {
          for(int n=0;n<4;n++) {
            if(dd.part[part].step[step].note[n]) {
              dd.part[part].step[step].note[n]+=tr;
            }
          }
        }
        stepRange=stepRange>>1;
      }
    }
    partRange=partRange>>1;
  }
  return 0;
}

int readLength(char *line) {
  int length;
  char *remain;

  length=readInteger(line,&remain);
  if(length<1 || length>4) {
    printError("Error in length","");
    return -1;
  }
  dd.length=length-1;
  return 0;
}

int readLastStep(char *line) {
  int partRange;
  int lst;
  char *remain;
  char *token;

  token=strtok_r(line," ",&remain);
  partRange=readPart(line,&remain);
  if(partRange==-1) {
    return -1;
  }
  lst=readInteger(remain,&remain);
  if(lst<1 || lst>16) {
    printError("Error in last Step","");
    return -1;
  }
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      //printf("putting part %d last step\n",lst);
      dd.part[part].lastStep=lst%16;
    }
    partRange=partRange>>1;
  }
  return 0;
}

//silence 'x'
//4 silences 'X'
//note '*'
//prol '_'
//no change '.'

void putSilence(int part, int step) {
  dd.part[part].step[step].onOff=0;
  dd.part[part].step[step].note[0]=0;
  dd.part[part].step[step].note[1]=0;
  dd.part[part].step[step].note[2]=0;
  dd.part[part].step[step].note[3]=0;
  if(dd.part[part].step[step].gateTime>96) {
	   dd.part[part].step[step].gateTime=96;
  }
}

void putTie(int part, int step) {
  step=(step-1)%64;
  dd.part[part].step[step].gateTime=127;
  step=(step+1)%64;
  dd.part[part].step[step].onOff=1;
}

int putSymbolInPattern(int part, int step, char symbol) {
    switch(symbol) {
      case 'x': putSilence(part,step++);break;
      case 'X': putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                break;
      case '*': dd.part[part].step[step++].onOff=1 ;break;
      case '_': putTie(part,(step)%64);step++;break;
      case   0: step=64; break;
    }
    return step;
}

int readPattern(char *line) {
  char *token;
  char *remain;
  int partRange;
  int step0=1;
  int step;

  token=strtok_r(line," ",&remain);
  partRange=readPart(token,&remain);
  if(partRange==-1) {
    return -1;
  }
  jumpBlanks(&remain);
  step0=readStep0(remain,&remain);
  if(step0==-1) {
    return -2;
  }
  deleteSpaces(remain);
  for(int part=0;part<16;part++) {
    if(partRange&1) {
      step=step0-1;
      for(int i=0;i<strlen(remain);i++) {
        step=putSymbolInPattern(part, step, remain[i]);
      } 
      if(step>=64) {
        break;
      }
    }
    partRange=partRange>>1;
  }
  return 0;
}

int readLine(char *line) {
  char *command;
  char *remain;

  command=strtok_r(line," ",&remain);
  if(!command) return 0; //blank line
  if(0==strncmp(command,"pattern",2)) {
     return readPattern(remain);
  } else if(0==strncmp(command,"notes",2)) {
     return readNotes(remain);
  } else if(0==strncmp(command,"name",2)) {
     return readName(remain);
  } else if(0==strncmp(command,"gateTime",2)) {
     return readGateTime(remain);
  } else if(0==strncmp(command,"velocity",2)) {
     return readVelocity(remain);
  } else if(0==strncmp(command,"lastStep",2)) {
     return readLastStep(remain);
  } else if(0==strncmp(command,"length",2)) {
     return readLength(remain);
  } else if(0==strncmp(command,"transpose",2)) {
     return readTranspose(remain);
  } else if(0==strncmp(command,"print",2)) {
     return printPattern(remain);
  } else if(command[0]=='#') { //comment
     return 0;
  } else {
     printError("command not defined.",command);
     return -1;
  }
}

void sendRealtimeMesagge(unsigned char m){
   FILE *f = fopen(device, "w");
   if (f == NULL) {
      printError("Error: cannot open ", device);
      exit(1);
   }
   fputc(m,f);
   fclose(f);
}

int readArguments(int argc, char *argv[]) {
  int n=1;
  while(n<argc) {
    if(strcmp(argv[n],"-d")==0) {
      if(strcmp(argv[n+1],"0")==0) {
        device=0;
      } else {
        device=argv[n+1];
      }
      n+=2;
    } else if(strcmp(argv[n],"-c")==0) {
      channel=atoi(argv[n+1]);
      if(channel>0 && channel<=16) {
        channel--;
      } else {
        printError("channel must be between 1 and 16 ",argv[n+1]);
	return -1;
      }

      n+=2;
    } else {
      printError("Wrong argument ",argv[n]);
      return -1;
    }
  }

  return 0;
}

int main(int argc, char *argv[]) {
char lineBuffer[MAX_LINE];
int i=0;
int res;

  res=readArguments(argc,argv);
  if(res<0) {
    exit(1);
  }
  if(device!=0) {
    sendRealtimeMesagge(0xfc); //stop
    currentPatternDataDump();
  }
  
  char c=getchar();
  while(c!=EOF) {
    if(i<MAX_LINE-1){
      lineBuffer[i++]=c;
    }
    if(c=='\n' || c==EOF) {
      lineBuffer[i-1]=0;
      strcpy(lineBufferCopy,lineBuffer);
      res=readLine(lineBuffer);
      if(res!=0){
        printError(":: ",lineBufferCopy);
        exit(1);
      }
      i=0;
    }
    c=getchar();
  }
  
  if(device!=0) {
    currentPatternDataSend();
    sendRealtimeMesagge(0xfa); //start 
  }
  return 0;
}
