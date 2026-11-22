#include "mock_ui.h"
int main(void){
  header(0,"EVENT - TRACK");
  vbox(0,"TRACK","G1T1",-1,1);
  static const char*ty[]={"NOTE","CHORD","CC","DRUM"}; picker(1,2,"TYPE",ty,4,2,0);
  vbox(3,"STEPS","128",50,0);
  header(1,"OUTPUT");
  vbox(4,"PORT","USB2",-1,0); vbox(5,"CHANNEL","1",6,0);
  box(6,1,0); label(6,"INSTR.",0); cbtext(bx(6)+bw(1)/2,29,"--",2,5); csmall(bx(6)+bw(1)/2,52,"drum only",4);
  box(7,1,0); label(7,"LAYERS",0);
  text(GLCD_FONT_SMALL,bx(7)+6,27,"PAR",1,8); btext(bx(7)+bw(1)-6-tw(GLCD_FONT_NORMAL,"8",2)-1,24,"8",2,15);
  text(GLCD_FONT_SMALL,bx(7)+6,45,"TRG",1,8); btext(bx(7)+bw(1)-6-tw(GLCD_FONT_NORMAL,"8",2)-1,42,"8",2,15);
  header(2,"PARAMETER LAYERS");
  static const char*ly[]={"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P"}; picker(8,2,"LAYER",ly,16,8,0);
  vbox(10,"TYPE","NOTE",-1,0);
  box(11,1,0); label(11,"PARAM",0); cbtext(bx(11)+bw(1)/2,33,"---",1,5);
  header(3,"ACTIONS");
  button(12,2,"PRESETS","SELECT",0);
  box(14,2,1); label(14,"INIT",1); { int cx=bx(14)+bw(2)/2; cbtext(cx,29,"HOLD 2 SEC",1,15); int x=bx(14)+5,w=bw(2)-10; for(int i=0;i<w;++i) px(x+i,52,3); } 
  save_pgm("mock_event.pgm"); return 0; }
