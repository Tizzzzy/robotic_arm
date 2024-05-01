//默认识别黄色，在串口中输入 1 2 识别红或绿色

int inByte = 0,   //串口缓冲
    num = 0;      //缓冲计数
    
const int kWaitTime = 3;   //3s   找不到色块通知间隔
const int kArmMotionTime = 5;   //10s
int x_openmv=0, y_openmv=0,angle_openmv=0;   //openmv中坐标
int x_arm=0, y_arm=0,angle_arm=0;            //机械臂中的坐标
unsigned long times;
char buf[20],      //接收openmv数据
     flag=0;       //作用： 开启或关闭向openmv发送请求
char color_sel[] = {'0','1','2'};   //颜色数组，分别为，黄，红，绿
int num_select = 0;    //颜色序号

void wait_for_finish_moving()//等待移动完成
{
  inByte=0;     //清空buf
  while(inByte!='<'){
     if (Serial2.available() > 0) {
        inByte = Serial2.read();
     }
  }
}

void setup() {
  Serial.begin(115200);//usb pc
  Serial1.begin(115200);//摄像头
  Serial2.begin(115200);//机械臂
  Serial.write("VISION START!\n");
  times = millis();
}

void loop() {
  
  if(flag == 0)      //复位机械臂，根据颜色向openmv发送开始信息
  {
    Serial2.write("g90g01x202y0z140a0b0c0F3000\n");// 移动
    // wait_for_finish_moving();   //等待移动完成
    Serial2.write("g90g01x202y0z143a0b0c0F3000\n");
    delay(100);//wait for the uarm to finish the moving then start the vision tracking
    // wait_for_finish_moving();
    flag = 1;//vision start
    switch(color_sel[num_select]){
      case '0': Serial1.write('y');break;
      case '1': Serial1.write('r');break;
      case '2': Serial1.write('g');break;
      default: break;
    }
    Serial1.write('S');//send vision start command
    Serial.write("开始寻找");
    switch(color_sel[num_select]){
        case '0': Serial.print("<黄>");break;
        case '1': Serial.print("<红>");break;
        case '2': Serial.print("<绿>");break;
        default: break;
      }
    Serial.write("色块\n");
    times = millis();
  }
  
  //get object coordinates from openmv
  if(get_openmv_data()==1)
  {
    flag = 0;         //vision end
    x_arm = x_openmv*0.9143-5;
    y_arm = y_openmv*0.93;
    angle_arm = angle_openmv;    
    String commands="G90G01X"; 
    commands.concat(x_arm);
    commands+="Y";
    commands.concat(-y_arm);
    commands+="Z143A";
    commands.concat(angle_arm);
    commands+="B0C0F1500\n";
    Serial2.print(commands);   //随机动作：移动至某个地方取产品
//    Serial.print(commands);
    Serial.println("开始移动");
    pick_and_palce(); 
    change_color();      //  切换颜色       
  }
}

//get object coordinates from openmv
unsigned char get_openmv_data()
{
  if (Serial1.available() > 0) 
  {
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

  if((millis()-times>kWaitTime*1000)&&(flag==1))//if no object detected, reset the flag every 10s
  {
    //用于清空串口
    while(Serial1.available() > 0)
    {
      inByte = Serial1.read();
    }
    //reset the count of uart
    num = 0;
    times = millis();
    flag = 0;
    Serial.write("   【");
    switch(color_sel[num_select]){
        case '0': Serial.print("<黄>");break;
        case '1': Serial.print("<红>");break;
        case '2': Serial.print("<绿>");break;
        default: break;
      }
    Serial.write("色块未找到】\n\n");//NO OBJECT IN CAMERA
    change_color();
  }
  return 0;
}
//move the detected object to the fixed position
void pick_and_palce()     //固定动作：移动到某个地方
{
  switch(color_sel[num_select]){
      case '0': Serial.print("放到<黄色>收纳盒");break;
      case '1': Serial.print("放到<红色>收纳盒");break;
      case '2': Serial.print("放到<绿色>收纳盒");break;
      default: break;
    }  
  delay(kArmMotionTime*1000);
  Serial.write("放置完成\n\n");
}

void change_color(){
  num_select = ++num_select % 3;
}
