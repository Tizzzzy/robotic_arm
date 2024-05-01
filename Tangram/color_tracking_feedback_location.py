"""
识别七巧板
成功拼成一个七巧板后需重新运行该程序
"""

import sensor, image, time, math
from pyb import UART
from pyb import LED
def Init_cam():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames(time = 2000)

blue_led  = LED(3)
green_led  = LED(2)
red_led  = LED(1)
uart = UART(3, 115200, timeout_char = 1000)   #P4(TXD)  P5(RXD)
blue_led.on()
# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
# The below thresholds track in general red/green things. You may wish to tune them...
#thresholds = [(30, 100, 15, 127, 15, 127), # generic_red_thresholds -> index is 0 so code == (1 << 0)
#              (30, 100, -64, -8, -32, 32)] # generic_green_thresholds -> index is 1 so code == (1 << 1)
#(6, 25, 15, 127, -54, 72)
thresholds = [(100, 63, -63, 24, 28, 78),    #1#yellow
              (35, 70, 73, 11, -7, 54),     #2#red(2400,2600) and orange(1250,1350)
              (29, 83, -55, -8, -13, 35),     #4#green
              (23, 62, 5, 49, -42, -5),      #8#purple
              (31, 72, -24, 23, -49, -6),    #16#blue
              ()]                            #4#white
# Codes are or'ed together when "merge=True" for "find_blobs".

Init_cam()     #initial the cam
clock = time.clock()

blue_led.off()
green_led.off()
red_led.off()
index = -1 #按顺序移动对应的物块 -1由从橙色块开始，+1后，从下一个色块开始。（用于调试，可避免从头开始，正常运行为-1）
buf = '0'   #串口缓冲

count = 0   #计数器
highlight =-2   #为了框选出将要移动的物块，该初始值为负数即可
uart_color = '0'  #颜色
direc = '0'       #角度方向
print("初始化完成")
# Only blobs that with more pixels than "pixel_threshold" and more area than "area_threshold" are
# returned by "find_blobs" below. Change "pixels_threshold" and "area_threshold" if you change the
# camera resolution. "merge=True" must be set to merge overlapping color blobs for color codes.
kANGLE0 = 10 #与0相比的阈值
kANGLE180 = 10#与180相比的阈值

while(True):
    clock.tick()
    img = sensor.snapshot()
    img.draw_cross(30,30,color=0)     #x方向 =260
    img.draw_cross(30,210,color=0)    #y方向 = 170
    img.draw_cross(290,30,color=0)
    img.draw_cross(290,210,color=0)
    img.draw_cross(160,120)
    img.draw_rectangle(30, 30, 260, 180, color =0, thickness = 2, fill = False)
    numbers = len(img.find_blobs(thresholds, pixels_threshold=300, area_threshold=300, merge=False))   #need to change the area and pixels threashold according to the distance between cam and box
    if uart.any()>0 :#是否收到指令
        buf=uart.read()
        count = 0
        index+=1     #每收到一个指令+1
        highlight = -2
        print("receieve the : ",buf)
    for blob in img.find_blobs(thresholds, pixels_threshold=300, area_threshold=300, merge=False):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        angle = int(blob.rotation()*180.0/math.pi)
        #print('X: ',blob.cx(),' Y: ', blob.cy())
        #print("angle: ",angle," code",blob.code(),blob.pixels())
        count+=1
        if highlight==((count-1) % numbers):     #作用是黑框圈出当前处理的物块
            img.draw_rectangle(blob.rect(),color = (0,0,0),thickness= 3)  #加深
            #print("angle= ",angle)
        if buf == b'S' :
            if index == 0 and blob.code()==2 and (900<blob.pixels()<1600):   # 橙色（其下颜色类同）
                angle -=33          #33是七巧板拼出的图形该颜色物块对应的角度    #负值需要顺时针，正值需要逆时针
                if angle>0:         #判断角度正负
                    direc = 'P'
                else :
                    direc = 'N'
                angle = abs(angle)  #得到角度绝对值
                print("orange adjust angle= ",angle)
                uart_color = 'O'    #颜色代号O=orgine
                buf = '0'           #清空缓冲，不在进入
                highlight = (count-1) % numbers    #设置这个颜色物块需要加黑框
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
            elif index == 1 and blob.code()==4:   ###绿色
                print("green adjust angle= ",angle)  #待定角度
                uart_color = 'G'
                if angle>0:
                    direc = 'P'
                else :
                    direc = 'N'
                angle = abs(angle)
                buf = '0'
                highlight = (count-1) % numbers
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
            elif index ==2 and blob.code()==16:   ###蓝色
                angle-=48
                if angle>0:
                    direc = 'P'
                else :
                    direc = 'N'
                angle = abs(angle)
                print("blue adjust angle= ",angle)
                uart_color = 'B'
                buf = '0'
                highlight = (count-1) % numbers
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
            elif index==3 and blob.code()==8:    ###紫色
                angle-=46
                if angle>0:
                    direc = 'P'
                else :
                    direc = 'N'
                angle = abs(angle)
                print("purple adjust angle= ",angle)
                uart_color = 'P'
                buf = '0'
                highlight = (count-1) % numbers
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
            elif index ==4 and blob.code()==2 and (2000<blob.pixels()): ##红色
                if angle<90:
                    angle = angle
                else:
                    angle-=180
                if angle>0:
                    direc = 'P'
                else :
                    direc = 'N'
                angle = abs(angle)
                print("red adjust angle= ",angle)
                uart_color = 'R'
                buf = '0'
                highlight = (count-1) % numbers
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
            elif index==5 and blob.code()==1:
                angle -=135
                if angle>0:
                    direc = 'P'
                else :
                    direc = 'N'
                uart_color = 'Y'
                angle = abs(angle)
                buf = '0'
                highlight = (count-1) % numbers
                print("yellow adjust angle= ",angle)
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(angle)+uart_color+direc+'\n')
