#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include "tables.h"
#include "elecmidi.h"

//#define PRINT_RESERVED 
//#define DELAY 5000
#define SILENCE 'x'
#define NOTE 'o'
#define SILENCE4 'X'
#define PROL '_'
#define NONOTE '.'

struct DataDumpType dd;
struct DataDumpType auxData;

char* device =  "/dev/dmmidi1" ;
int channel=0;
int octave=4;
FILE *midiFile;
int lineNumber=0;

#define TRACK_SIZE 18725 //max size of bytes received
#define MAX_LINE 500

unsigned char ioBuffer[TRACK_SIZE+10]; //10=room for the header + EOX
char lineBufferCopy[MAX_LINE];


void printError(char *e1,char *e2) {
  printf("Error in line %d: %s\n",lineNumber,lineBufferCopy);
  printf("%s %s\n",e1,e2);
  exit(1);
}

void openMidi(char *mode) {
   midiFile = fopen(device, mode);
   if (midiFile == NULL) {
      printError("Error: cannot open ", device);
   }
}

void sendMidiChar(unsigned char m){
   fputc(m,midiFile);
}

unsigned char readMidiChar() {
  return fgetc(midiFile);
}

void readMidiBuffer(unsigned char *buffer, long int bl) {
  fread(buffer, bl,1,midiFile);  
}
/*
void writeMidiBuffer(unsigned char *buffer, long int bl) {
  fwrite(buffer, bl,1,midiFile);  
}
*/
void closeMidi(){
   fclose(midiFile);
}

void sendRealtimeMessage(unsigned char m){
   openMidi("w");
   sendMidiChar(m);
   closeMidi();
}

void sendChannelMessage(unsigned char m1, unsigned  char m2, unsigned char m3){
   openMidi("w");
   sendMidiChar(m1);
   sendMidiChar(m2);
   if ((m1 & 0xf0) != 0xc0) { 
     sendMidiChar(m3);
   }
   closeMidi();
}

void sendCommand(unsigned char *command, long int cl, unsigned char *buffer, long int bl) {
   openMidi("w");
   
   for(int i=0;i<cl;i++) {
     sendMidiChar(command[i]);
   }
   closeMidi();
   //usleep(DELAY); 
   openMidi("r");
   int tries=0;
   buffer[0]=readMidiChar();
   while(buffer[0]!=0xf0 && tries++ < 2) { //wait for the sys ex message
     buffer[0]=readMidiChar();
   }
   if(buffer[0]==0xf0){
     readMidiBuffer(buffer+1, bl-1);
   }
   closeMidi();
}


void jumpBlanks(char **line) {
  if(!*line) return;
  while (**line!=0 && isspace(**line)){
    (*line)++;
  }
}

char *nextToken(char **line) {
   if(!*line) return "";
  jumpBlanks(line);
  return strsep(line," "); //if no more token returns "" and line=0;
}

int readInteger(char **line, int min, int max){
  char *token;
  int n;
  jumpBlanks(line);
  token=*line;
  if(!token) {
    printError("Error reading integer. Blank space or end of line found",lineBufferCopy);
  } 
  if(!isdigit(token[0]) && token[0]!='+' && token[0]!='-') {
    printError("Error reading integer. found ",token);
  }
  //n=atoi(token);
  n=strtol(token,line,10);
  if(!(min==0 && max==0) && (n<min || n>max)) {
    printError("Error integer out of range ",token);
  }

  jumpBlanks(line);
  return n;
}

void readNumber(char **line, char *separator, int *intPart, int *decPart){
  char *token;
  char *ip, *dp;
  int n;
  token=nextToken(line);
  if(!token) {
    printError("Error reading number. Blank space or end of line found",lineBufferCopy);
  } 
  if(!isdigit(token[0])) {
    printError("Error reading number. found ",token);
  }
  ip=strtok_r(token,separator,&dp);
  *intPart=atoi(ip);
  if(separator[0]==':' && strlen(dp)==0) { //special case for motion sequence
    *decPart=-1;
  } else {
    *decPart=atoi(dp);
  }
}

char* readString(char **line) {
  char *token;
  char stringEnd;


    jumpBlanks(line);
    if(*line==0) return 0;
    if(*line[0]=='"' || *line[0]=='\'') {
      stringEnd=*line[0];
      (*line)++;
    } else {
      stringEnd=' ';
    }
    token=*line;
    while(**line && **line !=stringEnd) (*line)++;
    if(stringEnd!=' ' && !**line) {
        printError("Error. Untermitated string.",lineBufferCopy);
    }
    **line=0;
    (*line)++;
    return token;
}

int readTable(char **line, char *table[], int tableLen) {
  if(!*line || *line[0]==0) {
    printError("Error reading value.","End of line found");
  }
  if(isdigit(*line[0])) {
    int n=readInteger(line,1,tableLen);
    return n-1;
  } else {
    char *token=readString(line);
    for(int i=0; i<tableLen;i++) {
      if(strcmp(token,table[i])==0) {
        return i;
      }
    }
    printf("Error reading table in line %d. Item not found, \"%s\"\n",lineNumber, token);
    printf("Values allowed:\n");
    for(int i=0; i<tableLen;i++) {
      printf("%d \"%s\"\n",i,table[i]);
    }
    exit(1);
  }
}

int readOnOffTable(char **line) {
  if(!*line) {
    printError("Error reading On Off value.","End of line found");
  }
  if(isdigit(*line[0])) {
    return readInteger(line,0,1);
  } else {
    return  readTable(line,onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]));
  }
}

long int readRange(char *line, int min, int max){
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
      n1=readInteger(&number1,min,max);

      if(number2[0]==0) {
        n2=n1;
      } else {
        n2=readInteger(&number2,min,max);
      }
      for(int i=n1;i<=n2;i++) {
            range |= (1L<< (i-1)); 
      } 
      token=strtok_r(line,",",&line);
    }
    return range;
}

int readPart(char **line){
  char *token;
  long int range;

  token=nextToken(line);
  if(*token==0) return -1;
  range=readRange(token,1,16);
  return range & 0xffff;
}

char *note2string(int noteNumber,char *buffer) {
  int octave;
  int note;
  char sharp;
 
  if(noteNumber==0) {
    buffer[0]=NONOTE;
    buffer[1]=0;
  } else {
    noteNumber--;
    octave=noteNumber/12-1;
    note=noteNumber%12;
    buffer[0]=keyTable[note][0];
    buffer[1]='0'+octave;
    if(keyTable[note][1]=='#') {
      buffer[2]='+';
    } else {
      buffer[2]=0;
    }
    buffer[3]=0;
  }
  return buffer;
}

char *printTableElement(char *table[], int tableLenght, int i) {
  if(i<0 || i>= tableLenght) return "XXXXXX";
  else return table[i];
}

//prints gateTime and velocity
void printRangeOfValues(char *name,int part,unsigned char *p) {
  int value;
  int flag=0;
  long int range=0;

  value=*p; //first value
  for(int i=1; i<64; i++) {
    if(*(p+sizeof(struct stepType )*i)!=value) {
      flag=1;
    }
  }
  if(!flag) { //all values are equal
    printf("%s %d %d\n",name, part, value);
    return;
  }
  while(flag) {
    flag=0;
    int pendingValue=-1000; //not possible value

    for(int i=0; i<64; i++) {
      if(!(range & (1L<<i))) {
         value=*(p+sizeof(struct stepType)*i);
         printf("%s %d @%d",name, part, i+1);
         for(int j=i+1;j<64;j++) {
           if(*(p+sizeof(struct stepType)*j)==value) {
	     if(*(p+sizeof(struct stepType)*(j-1))==value) { //same value that element before
	       pendingValue=j+1;
	     } else {
	       if(pendingValue!=-1000) {
	         printf("-%d",pendingValue);
		 pendingValue=-1000;
	       }
	       printf(",%d",j+1);
	     }
	     range |= (1L<<j);
	   }
	 }
	 if(pendingValue!=-1000) {
	   printf("-%d",pendingValue);
           pendingValue=-1000;
         }
	 printf(" %d\n",value);
      }
      
    }
    
  }
}

void printPartReserved(int part, unsigned char* p, int size) {
  int baseAddress= p-&dd.part[part].lastStep;
  for(int i=0;i<size;i++) {
    printf("#part %d reserved %d %d\n",part+1,baseAddress+i,*(p+i));
  }
}

void printStepReserved(int part, int step, unsigned char* p, int size) {
  int baseAddress= p-&dd.part[part].lastStep;
  for(int i=0;i<size;i++) {
    printf("#part %d step %d reserved %d %d\n",part+1,step+1,baseAddress+i,*(p+i));
  }
}

void printReserved(char *type, unsigned char* p, int size, int offset) {
  int baseAddress= p-&dd.header[0]-offset;
  for(int i=0;i<size;i++) {
    printf("%s %d %d\n",type,baseAddress+i,*(p+i));
  }
}

int printPart(int part) {
     printf("lastStep %d %d\n",part+1,dd.part[part].lastStep?dd.part[part].lastStep:16);
     printf("mute %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].mute));
     printf("voiceAssign %d \"%s\"\n",part+1,printTableElement(voiceAssignTable,sizeof(voiceAssignTable)/sizeof(voiceAssignTable[0]),dd.part[part].voiceAssign));
     printf("motionSequence %d \"%s\"\n",part+1,printTableElement(motionSequenceTable,sizeof(motionSequenceTable)/sizeof(motionSequenceTable[0]),dd.part[part].motionSequence));
     printf("padVelocity %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].padVelocity));
     printf("scaleMode %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].scaleMode));
     printf("partPriority %d \"%s\"\n",part+1,printTableElement(partPriorityTable,sizeof(partPriorityTable)/sizeof(partPriorityTable[0]),dd.part[part].partPriority));
     printf("oscType %d \"%s\"\n",part+1,printTableElement(oscTypeTable,sizeof(oscTypeTable)/sizeof(oscTypeTable[0]),dd.part[part].oscTypel+256*dd.part[part].oscTypeh));
     printf("oscEdit %d %d\n",part+1,dd.part[part].oscEdit);
     printf("filterType %d \"%s\"\n",part+1,printTableElement(filterTypeTable,sizeof(filterTypeTable)/sizeof(filterTypeTable[0]),dd.part[part].filterType));
     printf("filterCutoff %d %d\n",part+1,dd.part[part].filterCutoff);
     printf("filterReso %d %d\n",part+1,dd.part[part].filterReso);
     printf("filterEG %d %d\n",part+1,(signed char)dd.part[part].filterEG);
     printf("modType %d \"%s\"\n",part+1,printTableElement(modulationTypeTable,sizeof(modulationTypeTable)/sizeof(modulationTypeTable[0]),dd.part[part].modulationType));
     printf("modSpeed %d %d\n",part+1,dd.part[part].modulationSpeed);
     printf("modDepth %d %d\n",part+1,dd.part[part].modulationDepth);
     printf("EGAttack %d %d\n",part+1,dd.part[part].EGAttack);
     printf("EGDecay %d %d\n",part+1,dd.part[part].EGDecay);
     printf("ampLevel %d %d\n",part+1,dd.part[part].ampLevel);
     printf("ampPan %d %d\n",part+1,(signed char)dd.part[part].ampPan);
     printf("ampEG %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].EGOnOff));
     printf("MFXSend %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].MFXSend));
     printf("grooveType %d \"%s\"\n",part+1,printTableElement(grooveTypeTable,sizeof(grooveTypeTable)/sizeof(grooveTypeTable[0]),dd.part[part].grooveType));
     printf("grooveDepth %d %d\n",part+1,dd.part[part].grooveDepth);
     printf("IFXOnOff %d \"%s\"\n",part+1,printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.part[part].IFXOnOff));
     printf("IFXType %d \"%s\"\n",part+1,printTableElement(IFXTypeTable,sizeof(IFXTypeTable)/sizeof(IFXTypeTable[0]),dd.part[part].IFXType));
     printf("IFXEdit %d %d\n",part+1,dd.part[part].IFXEdit);
     printf("oscPitch %d %d\n",part+1,(signed char)dd.part[part].oscPitch);
     printf("oscGlide %d %d\n",part+1,dd.part[part].oscGlide);
     printf("triggers %d ",part+1);
     for(int s=0; s<64; s++) {
       if(s%4==0) printf(" ");
       if(s%16==0) printf("  ");
       if (dd.part[part].step[s].onOff==1) {
         if(s>0 && dd.part[part].step[s-1].gateTime>=127) {
           printf("%c",PROL);
	 } else {
           printf("%c",NOTE);
	 }
       } else {
         printf("%c",SILENCE);
       }
     }
     printf("\n");

     //printf("notes %d     ",part+1);
     for(int s=0; s<64; s++) {
       char buffer[4];

       if(s%16==0)       printf("\nnotes %d @%d ", part+1,s+1);
       else if(s%4==0)  printf("  ");
       /*
       for(int n=0;n<4;n++) {
         //printf("%s %d",note2string(dd.part[part].step[s].note[n],buffer),dd.part[part].step[s].note[n]);
         printf("%d",dd.part[part].step[s].note[n]);
       }
       printf(" ");
       */
       
       if(dd.part[part].step[s].note[0]==0 && dd.part[part].step[s].note[1]==0 && dd.part[part].step[s].note[2]==0 && dd.part[part].step[s].note[3]==0) {
         printf("%c   ",NONOTE);
       } else {
          printf("%s",note2string(dd.part[part].step[s].note[0],buffer));
          if(!(dd.part[part].step[s].note[1]==0 && dd.part[part].step[s].note[2]==0 && dd.part[part].step[s].note[3]==0)) {
            printf("%s",note2string(dd.part[part].step[s].note[1],buffer));
            if(!(dd.part[part].step[s].note[2]==0 && dd.part[part].step[s].note[3]==0)) {
                printf("%s",note2string(dd.part[part].step[s].note[2],buffer));
                if(!dd.part[part].step[s].note[3]==0) {
                  printf("%s",note2string(dd.part[part].step[s].note[3],buffer));
		} else printf(" ");
	    } else printf(" ");
	 } else printf(" ");
       }
       printf(" ");
     
     }
     printf("\n");
     printRangeOfValues("gateTime",part+1,&dd.part[part].step[0].gateTime);
     printRangeOfValues("velocity",part+1,&dd.part[part].step[0].velocity);
#ifdef PRINT_RESERVED
     printPartReserved(part,&dd.part[part].reserved0,sizeof(dd.part[part].reserved0));
     printPartReserved(part,&dd.part[part].reserved1,sizeof(dd.part[part].reserved1));
     printPartReserved(part,&dd.part[part].reserved2,sizeof(dd.part[part].reserved2));
     printPartReserved(part,&dd.part[part].reserved3[0],sizeof(dd.part[part].reserved3));
     printPartReserved(part,&dd.part[part].reserved4[0],sizeof(dd.part[part].reserved4));
     printPartReserved(part,&dd.part[part].reserved5,sizeof(dd.part[part].reserved5));
     printPartReserved(part,&dd.part[part].reserved6[0],sizeof(dd.part[part].reserved6));
     for(int step=0;step<64;step++){
       printStepReserved(part,step,&dd.part[part].step[step].reserved[0],sizeof(dd.part[part].step[step].reserved));
     }

#endif
/*
     unsigned char *p=(unsigned char*)&dd;
     for(int i=0;i<16384;i++){
       printf("#dump %d  %d %x\n",i,*(p+i),*(p+i));
     }
     printf("print\n");
*/
     printf("\n\n");

}

void printSlot(int slot) {
  if (dd.motionSequence.part[slot]==0) return; //Off
  printf("motion %d %d \"%s\"",slot,dd.motionSequence.part[slot],printTableElement(destinationTable,sizeof(destinationTable)/sizeof(destinationTable[0]),dd.motionSequence.destination[slot]));
  for(int i=0;i<64;i++) {
    printf(" %3d",dd.motionSequence.motion[slot][i]);
  }
  printf("\n");
}

int printPattern(char *line) {
   int partRange;
   char *token;
   int all=0;

    //token=strtok_r(line," ",&remain); ///?????
  token=nextToken(&line);
    if(*token==0) {
      all=1;
      partRange=0xffff;
    } else {
      partRange=readPart(&token);
    }
   if(all) {
   printf("name \"%s\"\n",dd.name);
   int tt=dd.tempo1+256*dd.tempo2;
   printf("tempo %d.%d\n", tt/10,tt%10);
   int sw=(signed char)dd.swing;if(sw==48) sw=50; if(sw==-48) sw=-50;
   printf("swing %d%%\n",sw);
   printf("length %x\n",dd.length+1);
   printf("beat \"%s\"\n",printTableElement(beatTable,sizeof(beatTable)/sizeof(beatTable[0]),dd.beat));
   printf("key \"%s\"\n",printTableElement(keyTable,sizeof(keyTable)/sizeof(keyTable[0]),dd.key));
   printf("scale \"%s\"\n",printTableElement(scaleTable,sizeof(scaleTable)/sizeof(scaleTable[0]),dd.scale));
   printf("chordset %x\n",dd.chordset+1);
   printf("level %d\n",127-dd.playlevel);
   printf("alternate1314 \"%s\"\n",printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.alternate1314));
   printf("alternate1516 \"%s\"\n",printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.alternate1516));
   printf("gateArpPattern %d\n",dd.touchScale.gateArpPattern+1);
   printf("gateArpSpeed %d\n",dd.touchScale.gateArpSpeed);
   printf("gateArpTime %d\n",(signed char)dd.touchScale.gateArpTimel+(signed char)dd.touchScale.gateArpTimeh*256);
   printf("MFXType \"%s\"\n",printTableElement(MFXTypeTable,sizeof(MFXTypeTable)/sizeof(MFXTypeTable[0]),dd.masterFX.type));
   printf("MFXPadX %d\n",dd.masterFX.XYpadX);
   printf("MFXPadY %d\n",dd.masterFX.XYpadY);
   printf("MFXHold \"%s\"\n",printTableElement(onOffTable,sizeof(onOffTable)/sizeof(onOffTable[0]),dd.masterFX.MFXHold?1:0));
   printf("chainTo %d\n",dd.chainTo);
   printf("chainRepeat %d\n",dd.chainRepeat);
   printf("\n");
   for(int slot=0;slot<24;slot++) {
     printSlot(slot);
   }
   printf("\n");
   }
   for(int part=0;part<16;part++) {
     if(partRange & 1) {
       printPart(part);
     }
     partRange = partRange>>1;
   }
#ifdef PRINT_RESERVED
     printf("#pattern header  %hhd %hhd %hhd %hhd\n",dd.header[0],dd.header[1],dd.header[2],dd.header[3]);
     printf("#pattern size %d\n",dd.size[0]+256*dd.size[1]+256*256*dd.size[2]+256*256*256*dd.size[3]);
     printf("#pattern version %hhd %hhd %hhd %hhd\n",dd.version[0],dd.version[1],dd.version[2],dd.version[3]);
     printReserved("#pattern reserved",&dd.reserved1[0],sizeof(dd.reserved1),0);
     printReserved("#pattern reserved",&dd.reserved2,sizeof(dd.reserved2),0);
     printReserved("#pattern reserved",&dd.reserved3[0],sizeof(dd.reserved3),0);
     printReserved("#pattern reserved",&dd.reserved4[0],sizeof(dd.reserved4),0);
     printReserved("#pattern reserved",&dd.reserved5[0],sizeof(dd.reserved5),0);
     printReserved("#pattern reserved",&dd.reserved6,sizeof(dd.reserved6),0);
     printReserved("#pattern reserved",&dd.reserved7[0],sizeof(dd.reserved7),0);
     printf("#pattern footer  %hhd %hhd %hhd %hhd\n",dd.footer[0],dd.footer[1],dd.footer[2],dd.footer[3]);
     printReserved("#pattern reserved",&dd.reserved8[0],sizeof(dd.reserved8),0);
     printReserved("#touchScale reserved",&dd.touchScale.reserved1[0],sizeof(dd.touchScale.reserved1),(unsigned char*)&dd.touchScale-&dd.header[0]);
     printReserved("#touchScale reserved",&dd.touchScale.reserved2,sizeof(dd.touchScale.reserved2),(unsigned char*)&dd.touchScale-&dd.header[0]);
     printReserved("#touchScale reserved",&dd.touchScale.reserved3[0],sizeof(dd.touchScale.reserved3),(unsigned char*)&dd.touchScale-&dd.header[0]);
     printReserved("#masterFX reserved",&dd.masterFX.reserved1,sizeof(dd.masterFX.reserved1),(unsigned char*)&dd.masterFX-&dd.header[0]);
     printReserved("#masterFX reserved",&dd.masterFX.reserved2,sizeof(dd.masterFX.reserved2),(unsigned char*)&dd.masterFX-&dd.header[0]);
     printReserved("#masterFX reserved",&dd.masterFX.reserved3[0],sizeof(dd.masterFX.reserved3),(unsigned char*)&dd.masterFX-&dd.header[0]);

#endif
   return 1;
}

int checkData(struct DataDumpType *dd){
    if(dd->header[0]!=80 || dd->header[1]!=84 || dd->header[2]!=83 || dd->header[3]!=84 ) {
      return -1;
    }
    if(dd->footer[0]!=80 || dd->footer[1]!=84 ||dd->footer[2]!=69 ||dd->footer[3]!=68) {
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
  /*
  if(error) {
    printf("error %d in received data\n",error);
  }
  */
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
void sendAckNack(int type) { //0x23=ack 0x24=nack
    unsigned char data[8] = {0xf0, 0x42, 0x30 | channel, 0, 1, 0x23, type,0xf7};
    openMidi("w");
    for(int i=0;i<8;i++) {
     sendMidiChar(data[i]);
    }
   closeMidi();
}
*/
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
//sendAckNack(0x23);
decodeData(input.buffer,&auxData);
}


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
    if(input->header[6]==0x40) {
      int res=decodeData(input->buffer,&dd);
      ok=(res==0);
    } else {
       sleep(1);
    }
/*

    if(input->header[6]==0x24) {
      printf("NACK received\n");
    } else if(input->header[6]==0) {
      printf("null data received\n");
       sleep(1);
       printf("p1\n");
    } else {
      printf("%x received\n",input->header[6]);
      int res=decodeData(input->buffer,&dd);
      ok=(res==0);
    }

  */
  }    
  if(!ok) {
    printf("error receiving data %x\n",input->header[6]);
    exit(1);
  }
}

void currentPatternDataSend() {
unsigned char data[8];
struct currentPattern {
		   unsigned char header[7];
		   unsigned char buffer[TRACK_SIZE];
		   unsigned char EOX;
		  } *output=(struct currentPattern *)ioBuffer;
int ok=0;
int tries=0;
long int l;

output->header[0]=0xf0;
output->header[1]=0x42;
output->header[2]=0x30 | channel;
output->header[3]=0;
output->header[4]=1;
output->header[5]=0x23;
output->header[6]=0x40;
output->EOX=0xF7;
l=encodeData(&dd,output->buffer);
l+=8; //header + EOX
while(!ok && tries++ <5) {
  sendCommand(ioBuffer,l,data,sizeof(data));
  if(data[6]==0x23){ 
    ok=1;
  } else {
    sleep(1);
  }
  /*
  if(data[6]==0x24) {
    printf("NACK received\n");
  } else if(data[6]==0x23){
    ok=1;
  } else if(data[6]==0x26) {
    printf("data format error\n");
  } else {
    printf("error sending data %x\n",data[6]);
  }
  */
}
  if(!ok) {
    printf("error sending data %x\n",data[6]);
    if(data[6]==0x26) {
      printf("data format error\n");
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


int readStep0(char **line){
  char *token;
 
  if(*line && *line[0]=='@') {
    token=nextToken(line);
    token++;
    return readInteger(&token,1,64);
  } else {
    return 1;
  }
}


int note2midi(char note, int octave, int alt) {
  int n;
  
  if(note==NONOTE) return 0;
  if(note>'B') octave--;
  n=(note-'A')*2;
  if(note>='C') n--;
  if(note>='F') n--;
  return 21 + n + alt + octave * 12;
}

//silence 'x'
//4 silences 'X'
//note 'o'
//prol '_'
//no note '.'

void putSilence(int part, int step) {
  dd.part[part].step[step].onOff=0;
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
      case SILENCE: putSilence(part,step++);break;
      case SILENCE4: putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                if(step<64) putSilence(part,step++);
                break;
      case NOTE: dd.part[part].step[step++].onOff=1 ;break;
      case PROL: putTie(part,(step)%64);step++;break;
      case   0: step=64; break;
    }
    return step;
}

int readStepNotes(char *notes,int part,int step, int trig){
  int n=0;
  int note;
  int alt;
  char c;
  while(n<4) {
    alt=0;
    note=toupper(*notes);
    if(note==0 || (trig &&note==PROL))  { //end of string. The remainder notes are put to 0
       while(n<4) {
         dd.part[part].step[step].note[n++]=0;
       }
       break;
    }
    if((note<'A' || note>'G') && note!=NONOTE) {
      printError("Error reading note",notes);
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
      }
    } else if(c=='^') {
      if(octave<9) {
        octave++;
        c=*notes++;
      } else {
        printError("Error octave too high",notes);
      }
    }
    if(c=='+') alt=1; 
    if(c=='-') alt=-1; 
    if(alt) notes++;
    int midiNote=note2midi(note,octave,alt);
    if(midiNote)
      midiNote++; //// why??
    if(midiNote>127) {
        printError("Error note too high",notes);
    }
    if(step<64) {
        dd.part[part].step[step].note[n]=midiNote;
    }
    n++;
  }
  if(trig) {
    dd.part[part].step[step].onOff=0;
    for(int n=0; n<4;n++) {
      if(dd.part[part].step[step].note[n]!=0) dd.part[part].step[step].onOff=1;
    }
  }
  if(*notes==PROL && trig) {
      step++;
      while(*notes==PROL) {
        for(int n=0;n<4;n++) dd.part[part].step[step].note[n]=dd.part[part].step[step-1].note[n];
        step=putSymbolInPattern(part,step,PROL);
        notes++;
      }
  } else {
    step++;
  }
  return step;
}

int readExpression(char *expr, char **buff,int len) {
   char *buffer=*buff;
   char *start=buffer;
   char *end=buffer;

   while(expr && *expr) { //readInteger can make expr null
     if(*expr=='(') {    // '('
       //find matching right parenthesis
       int n=1;
       char *rightp =expr;
       expr++; //after the parenthesis
       *buffer++=' ';
       start=buffer;
       while(*rightp && n>0) {
         rightp++;
         if(*rightp=='(') n++;
         if(*rightp==')') n--;
       }
       if(n!=0) {
         printError("Unclosed parenthesis","");
       }
       *rightp=0;
       len=readExpression(expr,&buffer,len);
       expr=rightp+1;
       end=buffer;
     } else if (*expr==')') {
       printError("Too many right parenthesis","");
     } else if (*expr=='*') {     //   '*'
       *buffer++=' '; //
       end=buffer;
       expr++;
       int n=readInteger(&expr,0,0);
       if(n<=0) {
         printError("Integer must be positive","");
       }
       n--;
       while(n--) {
         for(char *p=start;p<end;p++) {
	   len--;
	   if(!len) {
             printError("expresion too long","");
	   }
	   *buffer++=*p;
	 }
       }
       start=buffer-1;
     } else {   //all other characters
       len--;
       if(len<2) {
         printError("expresion too long","");
       }
       if((isalnum(*expr)||*expr=='+'||*expr=='.') && (isblank(*(expr-1))||*(expr-1)==0)) { //*(expr-1)==0 former ')'
         start=buffer;
       }
       *buffer++=*expr++;
     }
   }
   *buffer=0;
   *buff=buffer;
   return len;
}

int readTriggers(char *line) {
  char *token;
  int partRange;
  int step0=1;
  int step;
  char buffer[MAX_LINE*2]; //just in case
  char *buff=buffer;

  partRange=readPart(&line);
  if(partRange==-1) {
    return -1;
  }
  step0=readStep0(&line);
  if(step0==-1) {
    return -2;
  }
  readExpression(line,&buff,sizeof(buffer));
  line=buffer;
  deleteSpaces(line);
  for(int part=0;part<16;part++) {
    if(partRange&1) {
      step=step0-1;
      for(int i=0;i<strlen(line);i++) {
        step=putSymbolInPattern(part, step, line[i]);
      } 
      if(step>=64) {
        break;
      }
    }
    partRange=partRange>>1;
  }
  return 0;
}

int readNotes(char *line,int trig) {
  char *token;
  int partRange;
  int step0=1;
  int step[16];
  char buffer[MAX_LINE*2]; //just in case
  char *buff=buffer;

  partRange=readPart(&line);
  if(partRange==-1) {
    return -1;
  }
  step0=readStep0(&line);
  if(step0==-1) {
    return -2;
  }

  readExpression(line,&buff,sizeof(buffer));
  line=buffer;
  token=nextToken(&line);
  if(*token==0) return 0;
  for(int i=0;i<16;i++) { 
    step[i]=step0-1;
  }
  int ok=1;
  int origPartRange=partRange;
  while(*token!=0 && ok) {
      partRange=origPartRange;
      for(int part=0; part<16; part++) {
        if(partRange&1) {
          step[part]=readStepNotes(token,part,step[part],trig);
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
  token=nextToken(&line);
  }
  return 0;
}

int readName(char *line) {
  char *remain;
  char *name;
  name=readString(&line);
  if(name==0) {
    dd.name[0]=0;
    return 0;
  }
    for(int i=0;i<17;i++) {
      char c=name[i];
      if(c==0) {
         dd.name[i]=0;    
	 return 0;
      }
      if(c>=32 && c<127) {
         dd.name[i]=name[i];    
      } else {
        printError("Error in name",name);
        return -3;
      }
    }
    dd.name[17]=0;    
    return 0;
}

int readTempo(char *line) {
  int tempo;
  int intPart,decPart;
  char *remain;

  readNumber(&line,".",&intPart,&decPart);
  if(intPart<20 || intPart>300) {
    printError("Error in tempo","");
    return -1;
  }
  while(decPart>=10) {
    decPart=decPart/10;
  }
  tempo=intPart*10+decPart;
  dd.tempo1=tempo%256;
  dd.tempo2=tempo/256;
  return 0;
}

int readSwing(char *line) {
  int swing;

  swing=readInteger(&line,-50,50);
  if(swing>48) swing=48;
  if(swing<-48) swing=-48;
  dd.swing=swing;
  return 0;
}

int readLength(char *line) {
  int length;

  length=readInteger(&line,1,4);
  dd.length=length-1;
  return 0;
}

int readBeat(char *line) {
  int beat;
  beat=readTable(&line,beatTable,sizeof(beatTable)/sizeof(beatTable[0]));
  dd.beat=beat;
  return 0;
}

int readKey(char *line) {
  int key;

  key=readTable(&line,keyTable,sizeof(keyTable)/sizeof(keyTable[0]));
  dd.key=key;
  return 0;
}

int readScale(char *line) {
  int scale;

  scale=readTable(&line,scaleTable,sizeof(scaleTable)/sizeof(scaleTable[0]));
  dd.scale=scale;
  return 0;
}

int readChordset(char *line) {
  int chordset;
  chordset=readInteger(&line,1,5);
  dd.chordset=chordset-1;
  return 0;
}

int readLevel(char *line) {
  int level;

  level=readInteger(&line,0,127);
  dd.playlevel=127-level;
  return 0;
}

int readChainTo(char *line) {
  int cht;

  cht=readInteger(&line,0,250);
  dd.chainTo=cht;
  return 0;
}

int readChainRepeat(char *line) {
  int chr;

  chr=readInteger(&line,0,64);
  dd.chainRepeat=chr;
  return 0;
}

int readAlternate1314(char *line) {
  int a;

  a=readOnOffTable(&line);
  dd.alternate1314=a;
  return 0;
}

int readAlternate1516(char *line) {
  int a;
  char *remain;

  a=readOnOffTable(&line);
  dd.alternate1516=a;
  return 0;
}

int readGateArpPattern(char *line) {
  int p;

  p=readInteger(&line,1,50);
  dd.touchScale.gateArpPattern=p-1;
  return 0;
}

int readGateArpSpeed(char *line) {
  int s;

  s=readInteger(&line,0,127);
  dd.touchScale.gateArpSpeed=s;
  return 0;
}

int readGateArpTime(char *line) {
  int t;

  t=readInteger(&line,-100,100);
  dd.touchScale.gateArpTimel=t%256;
  dd.touchScale.gateArpTimeh=t/256;
  return 0;
}

int readMFXType(char *line) {
  int t;

  t=readTable(&line,MFXTypeTable,sizeof(MFXTypeTable)/sizeof(MFXTypeTable[0]));
  dd.masterFX.type=t;
  return 0;
}

int readMFXPadX(char *line) {
  int x;

  x=readInteger(&line,0,127);
  dd.masterFX.XYpadX=x;
  return 0;
}

int readMFXPadY(char *line) {
  int y;

  y=readInteger(&line,0,127);
  dd.masterFX.XYpadY=y;
  return 0;
}

int readMFXHold(char *line) {
  int h;
  char **remain;
  char *token;

  if(isdigit(line[0])) {
  token=nextToken(&line);
  *remain=line;
    int h=atoi(token);
    if(h<0 || h>127) {
      printError("Error in MFXHold",lineBufferCopy);
    }
  } else if(line[0]=='"') {
    token=++line;
    while(*line && *line !='"') line++;
    if(!*line) {
      printError("Error in MFXHold. Untermitated string.",lineBufferCopy);
    }
    *line++=0;
    remain=&line;
    if(strcmp(token,"On")==0) {
      h=1;
    } else if(strcmp(token,"Off")==0) {
      h=0;
    } else {
      printf("Error reading MFXHold. %s",token);
      printf("Values allowed: \"On\" \"Off\"\n");
      exit(1);
    }
  }
  dd.masterFX.MFXHold=h;
  return 0;
}

void putValueInRange(int partRange, unsigned char *p,unsigned char value) {
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      *(p+part*sizeof(struct partType))=value;
    }
    partRange=partRange>>1;
  }
}

int readLastStep(char *line) {
  int partRange;
  int lst;

  partRange=readPart(&line);
  lst=readInteger(&line,1,16);
  putValueInRange(partRange,&dd.part[0].lastStep,lst%16);
  return 0;
}

int readIntegerPart(char *line, int min, int max, unsigned char *p) {
  int partRange;
  int n;

  partRange=readPart(&line);
  n=readInteger(&line,min,max);
  putValueInRange(partRange,p,n);
  return 0;
}

int readOnOffPart(char *line,unsigned char *p) {
  int partRange;
  int value;

  partRange=readPart(&line);
  value=readOnOffTable(&line);
  putValueInRange(partRange,p,value);
  return 0;
}

int readTablePart(char *line, char *table[], int size, unsigned char *p) {
  int partRange;
  int va;
  char *token;

  partRange=readPart(&line);
  va=readTable(&line,table,size);
  putValueInRange(partRange,p,va);
  return 0;
}

int readGateTime(char *line) {
  int partRange;
  int gt;
  long int stepRange=0xFFFFFFFFFFFFFFFF;
  char *token;

  partRange=readPart(&line);
  if(partRange==-1) {////
    return -1;
  }
  token=nextToken(&line);
  if(line==0) { //last number
    line=token;
  } else if(token[0]=='@') {
    stepRange=readRange(token+1,1,64);
  } else {
    printError("Error in Gate Time","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  gt=readInteger(&line,0,0); //apparently gt can be 255 for ties
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
  char *token;

  partRange=readPart(&line);
  if(partRange==-1) {////
    return -1;
  }
  token=nextToken(&line);
  if(!line) { //last number
    line=token;
  } else if(token[0]=='@') {
    stepRange=readRange(token+1,1,64);
  } else {
    printError("Error in Velocity","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  v=readInteger(&line,0,127);
  for(int part=0;part<16;part++) {
    if(partRange & (1<<part)) {
      for(int step=0; step<64; step++) {
        if(stepRange & (1L<<step)) {
          dd.part[part].step[step].velocity=v;
        }
      }
    }
  }
  return 0;
}

int readTranspose(char *line) {
  int partRange;
  int tr;
  long int stepRange=0xFFFFFFFFFFFFFFFF;
  char *token;

  partRange=readPart(&line);
  if(partRange==-1) {
    return -1;
  }
  token=nextToken(&line);
  if(line==0) { //last number
    line=token;
  } else if (token[0]=='@'){
    stepRange=readRange(token+1,1,64);
  } else {
    printError("Error in Transpose","");
    return -1;
  }

  if(stepRange==0) {
    return -2;
  }
  tr=readInteger(&line,0,0);
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


int start(char *line) {
  sendRealtimeMessage(0xfb);
  sendRealtimeMessage(0xf8);
  return 0;
}

int stop(char *line) {
  sendRealtimeMessage(0xf8);
  sendRealtimeMessage(0xfc);
  sendRealtimeMessage(0xfc);
  return 0;
}

void waitClock(int n) {
  unsigned char c;
  openMidi("r");
  while(n) {
    c=readMidiChar();
    if(c==0xf8) {
      n--;
    }
  }
  closeMidi();
}

int readGoto(char *line) {
  unsigned char mm,bb,pp;
  char *num;

  num=line;
  int pattern=readInteger(&line,1,250);
  pattern--;
  mm=0;
  bb=pattern<128?0:1;
  pp=(pattern%128);

  sendChannelMessage(0xb0 | channel,0,mm);
  sendChannelMessage(0xb0 | channel,0x20,bb);
  sendChannelMessage(0xc0 | channel,pp,0);
  waitClock(1);
  return 0;
}

int readWait(char *line) {
  char *num=line;
  int intPart, decPart;
  int n;

  readNumber(&line,".",&intPart,&decPart);
  int lp=dd.part[0].lastStep;
  if(lp==0) lp=16;
  n=intPart*lp*dd.length+decPart;

  n=n*3+1; // +1 +2 +3

  if(n<0) {
    printError("Wrong time:",num);
    return -1;
  }
  waitClock(n);
  return 0;
}

int readRead(char *line) {
    currentPatternDataDump();
    return 0;
}

int readWrite(char *line) {
    currentPatternDataSend();
    return 0;
}

int readCopy(char *line) {
  int pattern, part,p;
  char *token;

  readNumber(&line,".",&pattern,&part);
  if(pattern<1 || pattern>250) {
    printError("Wrong pattern number:","");
    return -1;
  }
  if(part<1 || part>16) {
    printError("Error in part source","");
    return -1;
  }
  token=nextToken(&line);
  if(strcmp(token,"to")!=0) {
    printError("'to' expected. Found ",token);
    return -1;
  }
  p=readInteger(&line,1,16);

  patternDataDump(pattern);
  unsigned char *orig=(unsigned char *)&auxData.part[part-1];
  unsigned char *dest=(unsigned char *)&dd.part[p-1];
  for(int i=2;i<38;i++) {
    dest[i]=orig[i];
  }
  return 0;
}

int readXchgPart(char *line) {
  unsigned char *base=&dd.part[0].lastStep;
  unsigned char buffer;

  int a=readInteger(&line,1,250);
  int b=readInteger(&line,1,250);
  for(int i=0;i<sizeof(struct partType);i++) {
    buffer=*(base+(a-1)*sizeof(struct partType)+i);
    *(base+(a-1)*sizeof(struct partType)+i)=*(base+(b-1)*sizeof(struct partType)+i);
    *(base+(b-1)*sizeof(struct partType)+i)=buffer;
  }    
}

int oscType(char *line) {
  int partRange;
  int osct;
  char *token;

  partRange=readPart(&line);
  if(partRange==-1) {
    return -1;
  }
  osct=readTable(&line,oscTypeTable,sizeof(oscTypeTable)/sizeof(oscTypeTable[0]));
  for(int part=0;part<16;part++) {
    if(partRange & 1) {
      dd.part[part].oscTypel=osct%256;
      dd.part[part].oscTypeh=osct/256;
    }
    partRange=partRange>>1;
  }
  return 0;
}

void printValue(char *format0,char *format,int addr,int part,int offset) {
  addr += offset;
  if(part!=0) {
    printf(format,part,addr,*((unsigned char*)&dd.part+sizeof(struct partType)*(part-1)+addr));
  } else {
    printf(format0,addr,*((unsigned char*)&dd+addr));
  }
}

int readPeek(char *line) {
  int part=0;
  int addr;
  char type='u';
  int n=1;

  if(line[0]=='p') {
    line++;
    part=readInteger(&line,1,16);
  }
  addr=readInteger(&line,0,0);
  if(line[0]) {
    type=line[0];
    line++;
    if(line[0]) {
      n=readInteger(&line,0,0);
    }
  }
  for(int i=0;i<n;i++) {
    if (type=='d') {
      if(part!=0) {
        printf("part %d %d %d\n",part,addr,*(short *)((unsigned char*)&dd.part+sizeof(struct partType)*(part-1)+addr+i*2));
      } else {
        printf("%d %d\n",addr,*(short *)((unsigned char*)&dd+addr+i*2));
      }
    } else if(type=='c') {
        printValue("%d %c\n","part %d %d %c\n",addr,part,i);
    } else {
        printValue("%d %u\n","part %d %d %u\n",addr,part,i);
    }
  }
  return 1;
}

int readPoke(char *line) {
  int addr;
  int part=0;
  int value;
  char type='u';
  int n=1;


  if(line[0]=='p') {
    line++;
    part=readInteger(&line,1,16);
  }
  addr=readInteger(&line,0,0);
  if(!isdigit(line[0])) {
    type=line[0];
    line++;
    if(isdigit(line[0])) {
      n=readInteger(&line,0,0);
    }
  }
  for(int i=0;i<n;i++) {
    if(type=='c') {
  value=nextToken(&line)[0];
    } else {
      value=readInteger(&line,0,0);
    }
    if (type=='d') {
      if(part!=0) {
        *(short *)((unsigned char *)&dd.part+sizeof(struct partType)*(part-1)+addr+2*i)=value;
      } else {
        *(short *)((unsigned char *)&dd+addr+2*i)=value;
      }
    } else {
      if(part!=0) {
        *((unsigned char *)&dd.part+sizeof(struct partType)*(part-1)+addr+i)=value;
      } else {
        *((unsigned char *)&dd+addr+i)=value;
      }
    }
  }
  return 0;
}

int readMotionSequence(char *line) {
  int slot;
  int part;
  int destination;
  int i=0;
  int lpart,rpart;
  char *token;

  slot=readInteger(&line,0,23);
  part=readInteger(&line,1,16);
  destination=readTable(&line,destinationTable,sizeof(destinationTable)/sizeof(destinationTable[0]));
  dd.motionSequence.part[slot]=part;
  dd.motionSequence.destination[slot]=destination;
  token=nextToken(&line);
  while(token[0] && isdigit(token[0])) {
    unsigned char s;
    readNumber(&token,":",&lpart,&rpart);
    if(rpart==-1) { 
      s=lpart;
      dd.motionSequence.motion[slot][i++]=s;
    } else if(lpart==0) {
      s=rpart;
      dd.motionSequence.motion[slot][i++]=s;
    } else {
      int v0=dd.motionSequence.motion[slot][i-1];
      int n=lpart-i+1,n0=i-1;
      for(;i<=lpart;i++) {
        double sd=v0+(rpart-(double)v0)*(i-n0)/n;
        s=sd+0.5;
        dd.motionSequence.motion[slot][i]=s;
      }
    }
    token=nextToken(&line);
  }
  return 0;
}

int compare(char *input, char *command) {
 return strcmp(input,command);
}

int readLine(char *line) {
  char *command;

  command=nextToken(&line);
  if(strlen(command)==0) return 0; //blank line
  if(0==compare(command,"name")) {
     return readName(line);
  } else if(0==compare(command,"tempo")) {
     return readTempo(line);
  } else if(0==compare(command,"swing")) {
     return readSwing(line);
  } else if(0==compare(command,"length")) {
     return readLength(line);
  } else if(0==compare(command,"beat")) {
     return readBeat(line);
  } else if(0==compare(command,"key")) {
     return readKey(line);
  } else if(0==compare(command,"scale")) {
     return readScale(line);
  } else if(0==compare(command,"chordset")) {
     return readChordset(line);
  } else if(0==compare(command,"level")) {
     return readLevel(line);
  } else if(0==compare(command,"alternate1314")) {
     return readAlternate1314(line);
  } else if(0==compare(command,"alternate1516")) {
     return readAlternate1516(line);
  } else if(0==compare(command,"gateArpPattern")) {
     return readGateArpPattern(line);
  } else if(0==compare(command,"gateArpSpeed")) {
     return readGateArpSpeed(line);
  } else if(0==compare(command,"gateArpTime")) {
     return readGateArpTime(line);
  } else if(0==compare(command,"MFXType")) {
     return readMFXType(line);
  } else if(0==compare(command,"MFXPadX")) {
     return readMFXPadX(line);
  } else if(0==compare(command,"MFXPadY")) {
     return readMFXPadY(line);
  } else if(0==compare(command,"MFXHold")) {
     return readMFXHold(line);
  } else if(0==compare(command,"lastStep")) {
     return readLastStep(line);
  } else if(0==compare(command,"mute")) {
     return readOnOffPart(line,&dd.part[0].mute);
  } else if(0==compare(command,"voiceAssign")) {
     return readTablePart(line,voiceAssignTable,sizeof(voiceAssignTable)/sizeof(voiceAssignTable[0]),&dd.part[0].voiceAssign);
  } else if(0==compare(command,"motionSequence")) {
     return readTablePart(line,motionSequenceTable,sizeof(motionSequenceTable)/sizeof(motionSequenceTable[0]),&dd.part[0].motionSequence);
  } else if(0==compare(command,"padVelocity")) {
     return readOnOffPart(line,&dd.part[0].padVelocity);
  } else if(0==compare(command,"scaleMode")) {
     return readOnOffPart(line,&dd.part[0].scaleMode);
  } else if(0==compare(command,"partPriority")) {
     return readTablePart(line,partPriorityTable,sizeof(partPriorityTable)/sizeof(partPriorityTable[0]),&dd.part[0].partPriority);
  } else if(0==compare(command,"oscType")) {
     return oscType(line);
  } else if(0==compare(command,"oscEdit")) {
     return readIntegerPart(line,0,127,&dd.part[0].oscEdit);
  } else if(0==compare(command,"filterType")) {
     return readTablePart(line,filterTypeTable,sizeof(filterTypeTable)/sizeof(filterTypeTable[0]),&dd.part[0].filterType);
  } else if(0==compare(command,"filterCutoff")) {
     return readIntegerPart(line,0,127,&dd.part[0].filterCutoff);
  } else if(0==compare(command,"filterReso")) {
     return readIntegerPart(line,0,127,&dd.part[0].filterReso);
  } else if(0==compare(command,"filterEG")) {
     return readIntegerPart(line,-63,63,&dd.part[0].filterEG);
  } else if(0==compare(command,"modType")) {
     return readTablePart(line,modulationTypeTable,sizeof(modulationTypeTable)/sizeof(modulationTypeTable[0]),&dd.part[0].modulationType);
  } else if(0==compare(command,"modSpeed")) {
     return readIntegerPart(line,0,127,&dd.part[0].modulationSpeed);
  } else if(0==compare(command,"modDepth")) {
     return readIntegerPart(line,0,127,&dd.part[0].modulationDepth);
  } else if(0==compare(command,"EGAttack")) {
     return readIntegerPart(line,0,127,&dd.part[0].EGAttack);
  } else if(0==compare(command,"EGDecay")) {
     return readIntegerPart(line,0,127,&dd.part[0].EGDecay);
  } else if(0==compare(command,"ampLevel")) {
     return readIntegerPart(line,0,127,&dd.part[0].ampLevel);
  } else if(0==compare(command,"ampPan")) {
     return readIntegerPart(line,-63,63,&dd.part[0].ampPan);
  } else if(0==compare(command,"ampEG")) {
     return readOnOffPart(line,&dd.part[0].EGOnOff);
  } else if(0==compare(command,"MFXSend")) {
     return readOnOffPart(line,&dd.part[0].MFXSend);
  } else if(0==compare(command,"grooveType")) {
     return readTablePart(line,grooveTypeTable,sizeof(grooveTypeTable)/sizeof(grooveTypeTable[0]),&dd.part[0].grooveType);
  } else if(0==compare(command,"grooveDepth")) {
     return readIntegerPart(line,0,127,&dd.part[0].grooveDepth);
  } else if(0==compare(command,"IFXOnOff")) {
     return readOnOffPart(line,&dd.part[0].IFXOnOff);
  } else if(0==compare(command,"IFXType")) {
     return readTablePart(line,IFXTypeTable,sizeof(IFXTypeTable)/sizeof(IFXTypeTable[0]),&dd.part[0].IFXType);
  } else if(0==compare(command,"IFXEdit")) {
     return readIntegerPart(line,0,127,&dd.part[0].IFXEdit);
  } else if(0==compare(command,"oscPitch")) {
     return readIntegerPart(line,-63,63,&dd.part[0].oscPitch);
  } else if(0==compare(command,"oscGlide")) {
     return readIntegerPart(line,0,127,&dd.part[0].oscGlide);
  } else if(0==compare(command,"triggers")) {
     return readTriggers(line);
  } else if(0==compare(command,"notes")) {
     return readNotes(line,0);
  } else if(0==compare(command,"notes+")) {
     return readNotes(line,1);
  } else if(0==compare(command,"gateTime")) {
     return readGateTime(line);
  } else if(0==compare(command,"velocity")) {
     return readVelocity(line);
  } else if(0==compare(command,"transpose")) {
     return readTranspose(line);
  } else if(0==compare(command,"print")) {
     return printPattern(line);
  } else if(0==compare(command,"start")) {
     return start(line);
  } else if(0==compare(command,"stop")) {
     return stop(line);
  } else if(0==compare(command,"goto")) {
     return readGoto(line);
  } else if(0==compare(command,"wait")) {
     return readWait(line);
  } else if(0==compare(command,"read")) {
     return readRead(line);
  } else if(0==compare(command,"write")) {
     return readWrite(line);
  } else if(0==compare(command,"copySound")) {
     return readCopy(line);
  } else if(0==compare(command,"xchgPart")) {
     return readXchgPart(line);
  } else if(0==compare(command,"peek")) {
     return readPeek(line);
  } else if(0==compare(command,"poke")) {
     return readPoke(line);
  } else if(0==compare(command,"motion")) {
     return readMotionSequence(line);
  } else if(0==compare(command,"chainTo")) {
     return readChainTo(line);
  } else if(0==compare(command,"chainRepeat")) {
     return readChainRepeat(line);
  } else if(command[0]=='#') { //comment
     return 0;
  } else {
     printError("command not defined.",command);
     return -1;
  }
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
int immediate=0;


  //start(0);
  //next("10");
  //sendRealtimeMessage(0xFA);
  //sendRealtimeMessage(0xF8);
  //return 0;

  dd.header[0]=80;
  dd.header[2]=83;
  dd.header[1]=dd.header[3]=84;
  dd.footer[0]=80;
  dd.footer[1]=84;
  dd.footer[2]=69;
  dd.footer[3]=68;

  //dd.masterFX.reserved2=4;
  res=readArguments(argc,argv);
  if(res<0) {
    exit(1);
  }
  if(device==0) {
    printError("no midi device found","");
  }
  char c=getchar();
  while(c!=EOF) {
    if(i<MAX_LINE-1){
      lineBuffer[i++]=c;
    }
    if(c=='\n' || c==EOF) {
      i--;
      while(lineBuffer[i]=='\n' || lineBuffer[i]==' ' || lineBuffer[i]=='\t')
        lineBuffer[i--]=0;
      if(lineBuffer[i]=='!') {
        immediate=1;
	lineBuffer[i]=0;
      } else {
        immediate=0;
      }
      strcpy(lineBufferCopy,lineBuffer);
      if(immediate) {
        readRead(0);
      }
      lineNumber++;
      res=readLine(lineBuffer);
      if(res<0){
        printError(":: ",lineBufferCopy);
        exit(1);
      }
      if(!res && immediate) {
        readWrite(0);
      }
      i=0;
    }
    c=getchar();
  }
  
  return 0;
}
