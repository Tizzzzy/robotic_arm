"""
1.import needed module
2.creat the led example,uart example,set lab shresholds,initial the camera
3.loop
    (1)draw the frame and 4 cross
    (2)get one shot
    (3)go through all the blobs in the shot
        a)draw the rect of the blob
        b)if in right angle and last action finish then send to  Arduino the positon and angle
"""

import sensor, image, time, math
from pyb import UART
from pyb import LED
def Init_cam():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames(time = 2000)
    sensor.set_auto_gain(False) # must be turned off for color tracking
    sensor.set_auto_whitebal(False) # must be turned off for color tracking
kDelay = 4     #延时让色块移动过对应区域，防止重复发送消息（根据区域宽度及物块大小以及传送带速度调整）
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
thresholds = [(100, 63, -63, 24, 28, 78),     #1#yellow
             (86, 34, 26, 127, -5, 80),     #2#red
              (44, 82, -51, -19, -1, 27)]     #4#green
# Codes are or'ed together when "merge=True" for "find_blobs".

Init_cam()     #initial the cam
clock = time.clock()

blue_led.off()
green_led.off()
red_led.off()
ti = 0  #计数
buf = '0'
count = 0
print("初始化完成")
# Only blobs that with more pixels than "pixel_threshold" and more area than "area_threshold" are
# returned by "find_blobs" below. Change "pixels_threshold" and "area_threshold" if you change the
# camera resolution. "merge=True" must be set to merge overlapping color blobs for color codes.
kANGLE0 = 10 #与0相比的阈值
kANGLE180 = 10#与180相比的阈值
#是否正确方位
def IsPlaceCorrectly(angle):
    if angle<=kANGLE0 or angle >=180-kANGLE180:
        return True
    else:
        return False


while(True):
    clock.tick()
    img = sensor.snapshot()
    img.draw_cross(30,30,color=0)     #x方向 =260
    img.draw_cross(30,200,color=0)    #y方向 = 170
    img.draw_cross(290,30,color=0)
    img.draw_cross(290,200,color=0)
    #img.draw_line(150,30,150,300,color =0)   //确定区域与传送带位置关系的时候使用，运行时候注释掉，
    #img.draw_line(170,30,170,300,color =0)
    img.draw_cross(160,120)
    img.draw_rectangle(20, 20, 280, 200, color =0, thickness = 2, fill = False)
    numbers = len(img.find_blobs(thresholds, pixels_threshold=300, area_threshold=300, merge=False))   #need to change the area and pixels threashold according to the distance between cam and box
    for blob in img.find_blobs(thresholds, pixels_threshold=300, area_threshold=300, merge=False):
        img.draw_rectangle(blob.rect())
        img.draw_cross(blob.cx(), blob.cy())
        angle = int(blob.rotation()*180.0/math.pi)
        count+=1
        if  150 < blob.cx() < 170:
            if not IsPlaceCorrectly(angle):
                print("need to adjust the angle!")
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(int(blob.rotation()*180.0/math.pi))+'\n')
                print(blob.cx(),' ', blob.cy(),' ',int(blob.rotation()*180.0/math.pi))
                print("延时")
                time.sleep(kDelay*1000)
                ti+=1
                print(ti)
