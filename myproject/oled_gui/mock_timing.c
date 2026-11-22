#include "mock_ui.h"
int main(void){
  header(0,"TIMING - LENGTE");
  vbox(0,"LENGTH","16",6,0); vbox(1,"LOOP","1",0,1);
  static const char*lp[]={"4","8","16","24","32","64","128","256"}; picker(2,2,"PRESETS",lp,8,4,2);
  header(1,"KLOK");
  static const char*tb[]={"1","2","4","8","8T","16","16T","32"}; picker(4,2,"TIMEBASE",tb,8,4,5);
  tbox(6,"SYNC MAAT",1); tbox(7,"MANUAL",0);
  header(2,"RICHTING");
  static const char*dr[]={"FWD","BCK","P-P","PEN","RND","STP","R+S"}; picker(8,2,"DIRECTION",dr,7,4,2);
  vbox(10,"FWD","1",10,0); vbox(11,"BACK","0",0,0);
  header(3,"HERHALING");
  vbox(12,"REPLAY","x1",10,0); vbox(13,"REPEAT","0",0,0);
  box(14,1,0); label(14,"SKIP/ITV",0); text(GLCD_FONT_SMALL,bx(14)+6,28,"SKIP",1,8); btext(bx(14)+bw(1)-6-tw(GLCD_FONT_NORMAL,"0",2)-1,25,"0",2,15); text(GLCD_FONT_SMALL,bx(14)+6,45,"ITV",1,8); btext(bx(14)+bw(1)-6-tw(GLCD_FONT_NORMAL,"4",2)-1,42,"4",2,15);
  tbox(15,"SYNC M",0);
  save_pgm("mock_timing.pgm"); return 0; }
