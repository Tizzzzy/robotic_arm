//七巧板拼图
/*
整体使用流程
  1）标定相机像素与实际坐标的比例及位置关系
  2）标定机械臂与实际坐标的位置关系
  3）放置好第一个橙色物块后，运行openmv程序
  4）在每次机械臂动作期间放置下一块物块

注意：1 可以将全部物块同时放到摄像头视野后，开启openmv，需要更准确的相机坐标与及西比坐标的对应关系
     2  **对于三角形块有摆放的要求！！** 
 */

/*如果define WAY1 则为方式一，否则为方式二 */
// #define WAY1     //方式一（在摄像头视野内摆放），新方法为二（在摄像头视野外摆放，即方式一的平移） 

int inByte = 0,   //串口缓冲
    num = 0;      //缓冲计数
const int kWaitTime = 4;   //如果找不到色块通知间隔
const int kArmMotionTime =3;   //机械臂动作时间
int x_openmv=0, y_openmv=0,angle_openmv=0;   //openmv中坐标
int x_arm=0, y_arm=0,angle_arm=0;            //机械臂中的坐标

/*相机映射至实际平面的尺寸关系 （根据实际情况修改）*/
float x_scale = 1.06;
float y_scale = 1.056;
int x_offset = 57;
int y_offset = -30;

unsigned long times;
char color='0';        //判断颜色
char posi_or_nei = '-1';     //用于判断正还是负
char buf[20];      //接收openmv数据

//配置
void setup() {
  Serial.begin(115200);//usb pc
  Serial1.begin(115200);//摄像头
  Serial2.begin(115200);//机械臂
  Serial.write("拼图 START!\n");
  #ifdef WAY1
    Serial.println("###方式一###");
  #else
    Serial.println("###方式二###");
  #endif
  Serial1.write('S');     //向Openmv发送消息开启识别   
  times = millis();
}

void loop() {
  if(get_openmv_data()==1){
    x_arm = x_openmv*x_scale+x_offset;
    y_arm = y_openmv*y_scale+y_offset;
    if(posi_or_nei =='P'){     //判断角度方向
      angle_arm = angle_openmv;
    }else if(posi_or_nei=='N'){
      angle_arm = -angle_openmv;
    }
    Serial.print("x_arm: ");
    Serial.print(x_arm);
    Serial.print(" y_arm: ");  
    Serial.print(y_arm);
    Serial.print(" angle_arm: ");
    Serial.print(angle_arm);
    Serial.print(" color: ");
    Serial.print(color);
    Serial.print(" posi_or_nei: ");
    Serial.println(posi_or_nei);
    Serial.println("开始移动"); 

    GetToPlace();

    Finish();
  }
  if((millis()-times)> kWaitTime*1000){    //每隔kWaitTime在没有收到消息的情况下向openmv发消息
    Serial.println("\n等待着\n");
    Serial1.write('S');     //向Openmv发送消息开始识别 
    times = millis();
  }
  
}

//从openmv获取数据坐标及角度
unsigned char get_openmv_data(){
  if (Serial1.available() > 0){
    inByte = Serial1.read();
    buf[num++] = inByte;
    Serial.write(inByte);
    if((inByte=='\n')&&(buf[0]=='X'))    //程序不断进入此函数，直到接收结束，return 1
    {
      int counters=1;//jump the letter x
      x_openmv=0;
      do{
        x_openmv = x_openmv*10;
        x_openmv += buf[counters++] - 48;     //0~9字符-48 = int型的0~9
      }while((buf[counters]>='0')&&(buf[counters]<='9'));      //得到x值
      
      y_openmv=0;
      counters++;//jump the letter y
      do{
        y_openmv = y_openmv*10;
        y_openmv += buf[counters++] - 48;
      }while((buf[counters]>='0')&&(buf[counters]<='9'));//得到y值
      
      angle_openmv=0;
      counters++;//jump the letter a      
      do{
        angle_openmv = angle_openmv*10;
        angle_openmv += buf[counters++]-48;
        }while(counters+3<num);         //得到角度值
      color = buf[counters];    //获得颜色
      posi_or_nei = buf[counters+1];   //获得角度正负
      num = 0;
      return 1;
    }
  }
  return 0;
}
//完成拼图
void Finish(){      //调整角度
  delay(kArmMotionTime*1000);
  Serial.write("拼图完成\n\n");
  Serial1.write('S');     //向Openmv发送消息开始识别 
  times = millis();
}

void GetToPlace(){
    String commands1="M20G90G01X";      
    commands1.concat(x_arm);
    commands1+="Y";
    commands1.concat(-y_arm);
    commands1+="Z40A0B0C0F2500";
    Serial2.println(commands1);      
    Serial.println(commands1);       //移动至物体上方
    delay(2000);

    String commands2="M20G90G01X";      
    commands2.concat(x_arm);
    commands2+="Y";
    commands2.concat(-y_arm);
    commands2+="Z6A0B0C0F2500";
    Serial2.println(commands2);      
    Serial.println(commands2);      //贴近物体
    delay(1000);
    Serial2.print("M3 S100 M4 E100");     //开气泵
    Serial.println("M3 S100 M4 E100"); 
    delay(1000);

    String commands5="M20G90G01X";      
    commands5.concat(x_arm);
    commands5+="Y";
    commands5.concat(-y_arm);
    commands5+="Z40A0B0C0F2500";
    Serial3.println(commands5);      
    Serial.println(commands5);      //抬高机械臂
    delay(2000);

    if(color!='G'){
    String commands4="M20G90G01X";      
    commands4.concat(x_arm);
    commands4+="Y";
    commands4.concat(-y_arm);
    commands4+="Z40A0B0C"; 
    commands4.concat(angle_arm);
    commands4+="F2500";
    Serial2.println(commands4);         
    Serial.println(commands4);         //调整角度值
    delay(2000);}

    move_to_fixed_point(); //移动至指定点并下降，不同色块对应位置不同
    
    Serial2.println("M3S0M4E0");     //关气泵
    Serial.println("M3 S0 M4 E0");
    delay(1000); 
    Serial2.println("M20G90G01X202Y0Z206A0B0C0F2500");//归位
    Serial.println("M20G90G01X202Y0Z206A0B0C0F2500");    
}

void move_to_fixed_point(){
    switch(color){
      case 'O':{      //######橙色
        #ifdef WAY1
          String o_local = "M20G90G01X252Y-139Z80A0B0C";   //到达上空
        #else
          String o_local = "M20G90G01X252Y11Z120A0B0C"; 
        #endif
        o_local.concat(angle_arm);
        o_local += "F2500";
        Serial2.println(o_local);
        Serial.println(o_local);  
        delay(2500);
        #ifdef WAY1
          String o1_local = "M20G90G01X252Y-139Z5A0B0C";    //放下
        #else
          String o1_local = "M20G90G01X252Y11Z12A0B0C"; 
        #endif
        o1_local.concat(angle_arm);
        o1_local += "F2500";
        Serial2.println(o1_local);
        Serial.println(o1_local);  
        }
        break;
      case 'G':{  //############绿色
        #ifdef WAY1
          String g_local = "M20G90G01X244Y-90Z80A0B0C0";
        #else
          String g_local = "M20G90G01X244Y50Z120A0B0C0";
        #endif

        g_local += "F2500";
        Serial2.println(g_local);
        Serial.println(g_local); 
        delay(2500);
        #ifdef WAY1
          String g1_local = "M20G90G01X244Y-90Z10A0B0C0";
        #else
          String g1_local = "M20G90G01X244Y50Z12A0B0C0";
        #endif
        g1_local += "F2500";
        Serial2.println(g1_local);
        Serial.println(g1_local); 
        }
        break;
      case 'B':{      //#####蓝色
        #ifdef WAY1
          String b_local = "M20G90G01X240Y-60Z80A0B0C";
        #else
          String b_local = "M20G90G01X246Y80Z120A0B0C";
        #endif
        b_local.concat(angle_arm);
        b_local += "F2500";
        Serial2.println(b_local);
        Serial.println(b_local); 
        delay(2500);
        #ifdef WAY1
          String b1_local = "M20G90G01X240Y-60Z10A0B0C";
        #else
          String b1_local = "M20G90G01X246Y80Z12A0B0C";
        #endif
        b1_local.concat(angle_arm);
        b1_local += "F2500";
        Serial2.println(b1_local);
        Serial.println(b1_local); 
        }
        break;
      case 'P':{     //#####紫色
        #ifdef WAY1
          String p_local = "M20G90G01X200Y-50Z60A0B0C";
        #else
          String p_local = "M20G90G01X211Y90Z120A0B0C";
        #endif
        p_local.concat(angle_arm);
        p_local += "F2500";
        Serial2.println(p_local);
        Serial.println(p_local); 
        delay(2500);
        #ifdef WAY1
          String p1_local = "M20G90G01X200Y-50Z10A0B0C";
        #else
          String p1_local = "M20G90G01X211Y90Z12A0B0C";
        #endif
        p1_local.concat(angle_arm);
        p1_local += "F2500";
        Serial2.println(p1_local);
        Serial.println(p1_local); 
        }
        break;
      case 'R':{   //######红色#####
        #ifdef WAY1
          String r_local = "M20G90G01X140Y-35Z60A0B0C";
        #else
          String r_local = "M20G90G01X162Y80Z120A0B0C";
        #endif
        r_local.concat(angle_arm);
        r_local += "F2500";
        Serial2.println(r_local);
        Serial.println(r_local);  
        delay(3000);
        #ifdef WAY1
          String r1_local = "M20G90G01X140Y-35Z10A0B0C";
        #else
          String r1_local = "M20G90G01X162Y80Z12A0B0C";
        #endif
        r1_local.concat(angle_arm);
        r1_local += "F2500";
        Serial2.println(r1_local);
        Serial.println(r1_local);        
        }
        break;
      case 'Y':{        //#########黄色
        #ifdef WAY1
          String y_local = "M20G90G01X153Y-95Z60A0B0C";
        #else
          String y_local = "M20G90G01X153Y42Z120A0B0C";
        #endif
        y_local.concat(angle_arm);
        y_local += "F2500";
        Serial2.println(y_local);
        Serial.println(y_local); 
        delay(2500);
        #ifdef WAY1
          String y1_local = "M20G90G01X153Y-95Z10A0B0C";
        #else
          String y1_local = "M20G90G01X153Y42Z12A0B0C";
        #endif
        y1_local.concat(angle_arm);
        y1_local += "F2500";
        Serial2.println(y1_local);
        Serial.println(y1_local); 
        }
        break;
    }
    delay(2500);
}
