"""
1.import needed module
2.creat the led example,uart example,set lab shresholds,initial the camera
3.loop
    (1)draw the frame and 4 cross
    (2)get one shot
    (3)go through all the blobs in the shot
        a)select color
        b)draw the rect of the blob
        c)if is stable the send the axis
        d)update last time x,y
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
thresholds = [(72, 80, -9, 127, 26, 127),     #1#yellow
             (65, 42, 51, 106, -24, 69),     #2#red
              (39, 100,-51,-12, 10, 57)]     #4#green
# Codes are or'ed together when "merge=True" for "find_blobs".

Init_cam()     #initial the cam
clock = time.clock()

blue_led.off()
green_led.off()
red_led.off()

code = 1 ## 1:yellow   2:red    4:green
buf = "00"
# Only blobs that with more pixels than "pixel_threshold" and more area than "area_threshold" are
# returned by "find_blobs" below. Change "pixels_threshold" and "area_threshold" if you change the
# camera resolution. "merge=True" must be set to merge overlapping color blobs for color codes.

while(True):
    clock.tick()

    img = sensor.snapshot()#area_threshold 面积阈值，如果色块被框起来的面积小于这个值，会被过滤掉
                           #pixels_threshold 像素个数阈值，如果色块像素数量小于这个值，会被过滤掉
    img.draw_cross(20,20,color=0)
    img.draw_cross(20,220,color=0)
    img.draw_cross(300,20,color=0)
    img.draw_cross(300,220,color=0)
    img.draw_cross(160,120)
    img.draw_rectangle(20, 20, 280, 200, color =0, thickness = 2, fill = False)
    numbers = img.find_blobs(thresholds, pixels_threshold=100, area_threshold=100, merge=True,margin=4)
    #need to change the area and pixels threashold according to the distance between cam and box
    for blob in img.find_blobs(thresholds, pixels_threshold=100, area_threshold=100, merge=True,margin=4):
#check with color should be detect
        if uart.any()>0 :#接受串口来的指令，选择颜色
            buf=uart.read()              #   read uart is a little slow
            print("recieve : ",chr(buf[0]),end='')
            print(' ',len(numbers))
            if buf[0]==ord('y') :
                code = 1
            if buf[0]==ord('r') :
                code = 2
            if buf[0]==ord('g') :
                code = 4
#check if there is object with right color
        if blob.code() == code:#blob.code() 返回一个16bit数字，每一个bit会对应每一个阈值
            img.draw_rectangle(blob.rect())
            img.draw_cross(blob.cx(), blob.cy())
            if buf[1]==ord('S') :#它以一个字符（长度为1的字符串）作为参数，返回对应的 ASCII 数值，或者 Unicode 数
                print(blob.w(),' ',blob.h())
                print("return axis",end=' ')
                uart.write('X'+str(blob.cx())+'Y'+str(blob.cy())+'A'+str(int(blob.rotation()*180.0/math.pi))+'\n')
                print(blob.cx(),' ', blob.cy(),' ',int(blob.rotation()*180.0/math.pi))#物体框的位置
                #clear the flag
                buf = "00"
