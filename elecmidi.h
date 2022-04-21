struct stepType {
  unsigned char onOff;
  unsigned char gateTime;
  unsigned char velocity;
  unsigned char triggerOnOff;
  unsigned char note[4];
  unsigned char reserved[4];
};

struct partType {
  unsigned char lastStep;
  unsigned char mute;
  unsigned char voiceAssign;
  unsigned char motionSequence;
  unsigned char padVelocity;
  unsigned char scaleMode;
  unsigned char partPriority;
  unsigned char reserved0;
  unsigned char oscTypel;
  unsigned char oscTypeh;
  unsigned char reserved1;
  unsigned char oscEdit;
  unsigned char filterType;
  unsigned char filterCutoff;
  unsigned char filterReso;
  unsigned char filterEG;
  unsigned char modulationType;
  unsigned char modulationSpeed;
  unsigned char modulationDepth;
  unsigned char reserved2;
  unsigned char EGAttack;
  unsigned char EGDecay;
  unsigned char reserved3[2];
  unsigned char ampLevel;
  unsigned char ampPan;
  unsigned char EGOnOff;
  unsigned char MFXSend;
  unsigned char grooveType;
  unsigned char grooveDepth;
  unsigned char reserved4[2];
  unsigned char IFXOnOff;
  unsigned char IFXType;
  unsigned char IFXEdit;
  unsigned char reserved5;
  unsigned char oscPitch;
  unsigned char oscGlide;
  unsigned char reserved6[10];
  struct stepType step[64];
};

struct motionSequenceType {
  unsigned char part[24];
  unsigned char destination[24];
  unsigned char motion[24][64];
};

struct touchScaleType {
  unsigned char reserved1[5];
  unsigned char gateArpPattern;
  unsigned char gateArpSpeed;
  unsigned char reserved2;
  unsigned char gateArpTimel;
  unsigned char gateArpTimeh;
  unsigned char reserved3[6];
};

struct masterFXType {
  unsigned char reserved1;
  unsigned char type;
  unsigned char XYpadX;
  unsigned char XYpadY;
  unsigned char reserved2;
  unsigned char MFXHold;
  unsigned char reserved3[2];
};

struct DataDumpType{
 unsigned char header[4];
 unsigned char size[4];
 unsigned char reserved1[4];
 unsigned char version[4];
 unsigned char name[18];
 unsigned char tempo1;
 unsigned char tempo2;
 unsigned char swing;
 unsigned char length;
 unsigned char beat;
 unsigned char key;
 unsigned char scale;
 unsigned char chordset;
 unsigned char playlevel;
 unsigned char reserved2;
 struct touchScaleType touchScale;
 struct masterFXType masterFX;
 unsigned char alternate1314;
 unsigned char alternate1516;
 unsigned char reserved3[8];
 unsigned char reserved4[178];
 struct motionSequenceType motionSequence;
 unsigned char reserved5[208];
 struct partType part[16];
 unsigned char chainTo;
 unsigned char reserved6;
 unsigned char chainRepeat;
 unsigned char reserved7[249];
 unsigned char footer[4];
 unsigned char reserved8[1024];
};

