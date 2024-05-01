
/* ###############################ReadMe###################################
实现思路：
  传送带通过中断实现持续运动；
  物块到达摄像头指定区域时候，openmv将坐标和角度发送给Arduino
  Arduino收到串口消息时候，将收到时刻的时间、收到的坐标和角度存储在顺环队列；
  Arduino检测顺环队列是否为空，不空则判断当前时间和队列中第一个节点的记录的
  时间之差是否超过规定值（kWaitTime），如果超过，则使机械臂动作。

  注意：任意两个物块之间的距离需要大于（传送带速度✖机械臂动作时间）

整体使用步骤：
  1）标定相机像素与实际坐标的比例及位置关系
  2）标定机械臂与实际坐标的位置关系
  3）前两步的目的及使机械臂和相机像素的位置一一对应
  4）确定识别物块的像素区域到机械臂末端机构的距离，测出时间。
  5）复位机械臂为正常状态
  6) 开启openmv，复位Arduino
  7）放置木块
  注意：物块从传送带左侧位置放置，放置时候避免手臂等经过摄像头识别物块的区域。
  （这个问题也可以通过调节openmv物块颜色的阈值来调整）
  #######################################################################*/


/* 根据摄像头与机械臂的相对位置有以下换算关系：（摆放的）
    x+x_offset => y      实际发送给机械臂的为    -y
    170-y+y_offset => x  实际发送给机械臂的为    +x

    (202,0,56)为机械臂初始位置坐标，不是复位后的坐标
*/
#include <MsTimer2.h>

const unsigned long kWaitTime =48;   // 物块从摄像头指定区域到机械臂末端需要的时间  单位：s
const int kArmMotionTime =1;         //机械臂动作需要的时间：kArmMotionTime+4   单位：s
int inByte = 0,    //串口缓冲
    num = 0;       //缓冲计数
char buf[20];      //接收openmv数据
int x_openmv=0, y_openmv=0,angle_openmv=0;   //openmv中坐标
int x_arm=0, y_arm=0,angle_arm=0;            //机械臂中的坐标

//相机映射至实际平面的尺寸关系
float scale = 1.03;    //比例系数
int offset = 115;      //偏移值

//定义结构体，存储色块的位置、角度及时间
#define MAXSIZE 10   //最大存储空间 10（可以根据色块摆放密度更改，越大越好，但是占Arduino存储空间，过大也是浪费的，顺环队列会循环利用）
struct Node{
  int x_location;     //x坐标
  int angle;          //角度
  unsigned long times;//时间
};
//定义队列结构，使用队列先入先出特性
struct QueLocal{
  struct Node data[MAXSIZE];   //结构体数组，存储色块信息
  int front;   //指向队列头的前一个位置
  int rear;    //指向队列尾
};
typedef struct QueLocal* ptrlocal;
//创建空队列函数
ptrlocal CreatEmptyQueue(){
  Serial.println("建空队列");
  ptrlocal ptrq = new QueLocal;
  ptrq->front = 0;
  ptrq->rear = 0;
  return ptrq;
}
//入队函数
void AddQueue(Node node, ptrlocal ptr_queue){
  if((ptr_queue->rear+1)%MAXSIZE == ptr_queue->front){
    Serial.println("队列已满");
  }else{
    ptr_queue->rear = (ptr_queue->rear+1)%MAXSIZE;
    ptr_queue->data[ptr_queue->rear] = node;
    Serial.println("入队");
  }
}
//判断队列是否空函数
bool QueueIsEmpty(ptrlocal ptr_queue){
  if(ptr_queue->front == ptr_queue->rear){
    // Serial.println("队列空");
    return true;
  }else{
    
    // Serial.print("队列非空");
    return false;
  }
}
//出队函数（根据主程序的编写，出队可不判断是否队列为空）
Node DeleteQueue(ptrlocal ptr_queue){
    Serial.println("出队");
    ptr_queue->front = (ptr_queue->front+1)%MAXSIZE;
    return ptr_queue->data[ptr_queue->front];
}

//创建一个全局队列，保存物块信息
ptrlocal box_location = CreatEmptyQueue();

//电机引脚配置及转动方向
#define DIR 39    
#define STEP 40  
#define Direc LOW   //HIGH（右）方向   LOW（左）方向

//定时器中断驱动步进电机的函数
void steper(){
    static bool direction = LOW;    
    digitalWrite(STEP,direction);
    direction = !direction;
  }

//配置
void setup() {
  Serial.begin(115200); //usb pc
  Serial1.begin(115200);//摄像头
  Serial2.begin(115200);//机械臂
  MsTimer2::set(1,steper); //每一毫秒进入steper函数
  MsTimer2::start();       //开启定时器中断
  delay(200);
  pinMode(DIR,OUTPUT);
  pinMode(STEP,OUTPUT);
  digitalWrite(DIR,Direc);  //high（右）方向   low（左）方向
  Serial2.println("M20G90G01X202Y0Z56A0B0C0F3000");  //机械臂运行至指定初始位置
  Serial.write("MOTION START!\n");
}

//循环
void loop() {

  if(get_openmv_data()){    //如果获得openmv消息，则将对应的信息入队。
    Node nodeadd;
    nodeadd.times = millis();
    nodeadd.x_location = y_openmv;
    nodeadd.angle = CalAngle(angle_openmv);
    AddQueue(nodeadd,box_location);   //将色块信息入队
  }
  //如果队列不空就比较时间，当队列首位大于指定的时间的时候，就运行机械臂
  if(!QueueIsEmpty(box_location)){
    //如果队列首位时间到达，则运行机械臂，并且删除队列
    unsigned long firstnodetime = 
                  box_location->data[(box_location->front+1)%MAXSIZE].times;
    Serial.println(millis()-firstnodetime);
    if((millis()-firstnodetime) > kWaitTime*1000){
      Serial.println("时间到达，机械臂动作进行调整");
      Node node = DeleteQueue(box_location);
      x_arm = 170 - node.x_location*scale + offset;
      y_arm = 0;
      angle_arm =node.angle;   
      GetToPlace();
    }
  }
}

//处理来自openmv的数据
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
      }while((buf[counters]>='0')&&(buf[counters]<='9'));//为得到y值
      
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

//计算需要调整的角度
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

//动作机械臂 共需要5s
void GetToPlace(){  
    String commands1="M20G90G01X";      
    commands1.concat(x_arm);
    commands1+="Y";
    commands1.concat(-y_arm);
    commands1+="Z56A0B0C0F4000";
    Serial2.println(commands1);      
    Serial.println(commands1);      //移动至物块上空
    delay(800);
    
    String commands2="M20G90G01X";      
    commands2.concat(x_arm);
    commands2+="Y";
    commands2.concat(-y_arm);
    commands2+="Z51A0B0C0F4000";
    Serial2.println(commands2);      
    Serial.println(commands2);      //贴近物体
 

    Serial2.print("M3 S100 M4 E100");     //开气泵
    Serial.println("M3 S100 M4 E100"); 

    String commands3="M20G90G01X";      
    commands3.concat(x_arm);
    commands3+="Y";
    commands3.concat(-y_arm);
    commands3+="Z56A0B0C0F4000";
    Serial2.println(commands3);      
    Serial.println(commands3);      //抬高机械臂
    delay(1000);

    String commands4="M20G90G01X";      
    commands4.concat(x_arm);
    commands4+="Y";
    commands4.concat(-y_arm);
    commands4+="Z56A0B0C"; 
    commands4.concat(angle_arm);
    commands4+="F4000";
    Serial2.println(commands4);         
    Serial.println(commands4);         //调整角度值

    
    Serial2.println("M3S0M4E0");     //关气泵
    Serial.println("M3 S0 M4 E0"); 
    delay(1500);
    Serial2.println("M20G90G01X202Y0Z56A0B0C0F3000");//归位
    Serial.println("M20G90G01X202Y0Z56A0B0C0F3000"); 

    delay(kArmMotionTime*1000);//根据机械臂动作完成的时间进行调整
    Serial.write("调整完成\n\n");  
}
