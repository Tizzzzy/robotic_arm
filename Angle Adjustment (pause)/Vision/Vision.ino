//识别到物块到达指定区域，传送带停止运行，角度错误的物块更改角度完成后，传送带继续运行


/* 根据摄像头与机械臂的相对位置有以下换算关系：
    x+x_offset => y   实际发送给机械臂的为       -y
    170-y+y_offset => x  实际发送给机械臂的为    +x
/*/

int inByte = 0,   //串口缓冲
    num = 0;      //缓冲计数

const int kStepperFrequency = 600;   //  电机运行频率 600us
const int kWaitTimes = 3;    //3s  找不到色块通知间隔
const int kArmMotionTime =5;   //10s
int x_openmv=0, y_openmv=0,angle_openmv=0;   //openmv中坐标
int x_arm=0, y_arm=0,angle_arm=0;            //机械臂中的坐标

/*相机映射至实际平面的尺寸关系 */
float x_scale = 1.06;
float y_scale = 1.03;
int x_offset = 45;
int y_offset = 111;
unsigned long times;
char buf[20];      //接收openmv数据

//电机引脚配置及方向设置
#define DIR 39    
#define STEP 40  
bool direction = LOW;    

void setup() {
  Serial.begin(115200);//usb pc
  Serial1.begin(115200);//摄像头
  Serial2.begin(115200);//机械臂
  pinMode(DIR,OUTPUT);
  pinMode(STEP,OUTPUT);
  digitalWrite(DIR,HIGH);  //high（右）方向   low（左）方向
  Serial.write("MOTION START!\n");
  Serial1.write('S');     //向Openmv发送消息开始识别   
  times = micros();
}

void loop() {
  if((micros()-times)>kStepperFrequency){
    static int counter = 0;
    counter++;
    digitalWrite(STEP,direction);
    direction = !direction;
    if(counter == int(10*1000/6*kWaitTimes)){
      Serial.println("\n没有需要调整的物块\n");
      counter = 0;
    }
    times = micros();
  }
  //get object coordinates from openmv
  if(get_openmv_data()==1){
    x_arm = 170-y_openmv*y_scale+y_offset;
    y_arm = x_openmv*x_scale+x_offset;
    
    angle_arm =CalAngle(angle_openmv);   
    Serial.println("开始调整"); 
    GetToPlace();
    AdjustAngle();
  }
}

//get object coordinates from openmv
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
      }while((buf[counters]>='0')&&(buf[counters]<='9'));//为了忽略最后一个‘\n’得到y值
      
      angle_openmv=0;
      counters++;//jump the letter a      
      do{
        angle_openmv = angle_openmv*10;
        angle_openmv += buf[counters++]-48;
        }while(counters+1<num);         //得到角度值
      num = 0;
      return 1;
    }
  }
  return 0;
}
//move the detected object to the fixed position
void AdjustAngle(){      //调整角度
  delay(kArmMotionTime*1000);
  Serial.write("调整完成\n\n");
  Serial1.write('S');     //向Openmv发送消息开始识别 
  times = micros(); 
}

int CalAngle(int angle){
  int changed_angle;
  if(angle<90){
    changed_angle = angle;   //逆时针（+号）        //机械臂正是逆时针
  }else{
    changed_angle = angle-180;    //顺时针（-）
  }
  //对于长方形，长边为正确方向：
    //如果在0-90之间，逆时针转
    //如果在90-180之间，顺时针转动
  Serial.print("需要调整的角度：");
  Serial.println(changed_angle);
  return changed_angle;
}

void GetToPlace(){
    String commands1="M20G90G01X";      
    commands1.concat(x_arm);
    commands1+="Y";
    commands1.concat(-y_arm);
    commands1+="Z60A0B0C0F2500";
    Serial2.println(commands1);      
    Serial.println(commands1);       //移动至物体上方
    delay(3000);

    String commands2="M20G90G01X";      
    commands2.concat(x_arm);
    commands2+="Y";
    commands2.concat(-y_arm);
    commands2+="Z37A0B0C0F2500";
    Serial2.println(commands2);      
    Serial.println(commands2);      //贴近物体
    delay(1000);

    Serial2.print("M3 S100 M4 E100");     //开气泵
    Serial.println("M3 S100 M4 E100"); 
    delay(1000);

    String commands3="M20G90G01X";      
    commands3.concat(x_arm);
    commands3+="Y";
    commands3.concat(-y_arm);
    commands3+="Z50A0B0C0F1500";
    Serial3.println(commands3);      
    Serial.println(commands3);      //抬高机械臂
    delay(1000);

    String commands4="M20G90G01X";      
    commands4.concat(x_arm);
    commands4+="Y";
    commands4.concat(-y_arm);
    commands4+="Z50A0B0C"; 
    commands4.concat(angle_arm);
    commands4+="F2500";
    Serial2.println(commands4);         
    Serial.println(commands4);         //调整角度值
    delay(1500);

    String commands5="M20G90G01X";      
    commands5.concat(x_arm);
    commands5+="Y";
    commands5.concat(-y_arm);
    commands5+="Z37A0B0C"; 
    commands5.concat(angle_arm);
    commands5+="F2500";
    Serial2.println(commands5);         
    Serial.println(commands5);         //下降
    delay(1500);
    
    Serial2.println("M3S0M4E0");     //关气泵
    Serial.println("M3 S0 M4 E0"); 
    delay(500);
    Serial2.println("M20G90G01X202Y0Z206A0B0C0F2500");//归位
    Serial.println("M20G90G01X202Y0Z206A0B0C0F2500");    
}
