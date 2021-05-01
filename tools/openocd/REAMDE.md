# OpenOCD-HI3861-RISCV使用方法 (JTAG/DTM+SWD/CoreSight)



HI3861使用 五线JTAG+DTM 或 两线SWD+CoreSight 形式进行OpenOCD Debug

基于 https://github.com/riscv/riscv-openocd/releases/tag/v2018.12.0  进行适配RISCV-CoreSight



目录讲解

 bin/                                存放openocd.exe+依赖的dll
 bin_dir/                         存放需要烧写的HI3861的bin,包括SWD/JTAG,示例
 BUILDTIME                   编译时间
 drivers/                         驱动+工具 
 interface/                      驱动配置文件 包括SWD+JTAG
 load_dump/                 烧写+下载的目录,非必要
 log.d/                            日志存放文件夹,非必要
 pdf/                               openocd+riscv+ft2232d(ft2232h)文档
 README.md                本文件
 target/                           HI3861的DTM+CoreSight

### 前提.HI3861使用OpenOCD的前提

#### 1.Flash中有固件,且固件中设置的JTAG/SWD,对应使用相应的interface+target

```
使用HiBurn(drivers\HiBurn.exe)烧写固件
1.HI3861四组拨码开关全往上(相对于TypeC口位置),切换串口模式
2.打开drivers\HiBurn.exe
3.配置波特率,Settings->Com setting -> baud = 2000000, 如果出问题,调低点
4.选择固件文件select file -> 对应的SWD/JTAG固件,重要. 名字应该带有allinone字样.
5.默认勾选Select all
6.勾选Auto burn
7.选择HI3861对应的串口,connect
8.HI3861硬件复位,等待烧写完成(出现字样
Wait connect success flag (hisilicon) overtime.)
9.使用串口工具,检测是否烧写成功,非必要步骤.drivers\SSCOM_v5.13.1.7z

HiBurn使用,详情咨询 杨玉生 84163305
没有固件,不开启JTAG,涉及安全问题,详情咨询王俊 00291248
```

#### 2.电脑安装相应的驱动

```
安装替换驱动
使用drivers\zadig-2.4.exe
将Dual RS232(Interface 0)转化为WinUSB(v6.1.7600.16385)
必须是Interface 0,FT2232D只有Channel A具有mpsse功能.
见 pdf\DS_FT2232D.pdf
如果找不到Dual RS232(Interface 0)尝试先安装drivers\CDM21228_Setup.exe,再替换(非必要步骤)
详情咨询贾雪奎 wx638039
```



### 一.使用的命令

openocd命令讲解

openocd可执行文件  -f  驱动配置文件  -f 芯片配置文件    -d3 -l 日志文件路径
-d3 -l联合使用,表示开启debug,制定日志文件,可以不使用.

即,只使用 openocd可执行文件  -f  驱动配置文件  -f 芯片配置文件

详情查看pdf\openocd.pdf  /  http://openocd.org/doc-release/pdf/openocd.pdf



#### 1.五线JTAG

cmd: bin\openocd.exe -f interface\Hi-ft2232d-ftdi.cfg  -f target\HI3861L-RISCV-JTAG -d3 -l log.d/20200818-jtag-1.txt

#### 2.两线SWD

cmd: bin\openocd.exe -f interface\Hi-ft2232d-ftdi-swd.cfg  -f target\HI3861L-RISCV-SWD-CORESIGHT -d3 -l log.d/20200812-swd-coresight-1.txt

以上窗口常打开,以开启端口.

#### 3.开启telnet (连接上之后才能使用telnet,也可以使用gdb)

cmd: telnet localhost 4444



### 二.烧写bin的整个流程

```tcl
1.复位+停止
reset halt

2.关闭I/D缓存
#Close I/D Cache
reg csr0x7c0 0
reg csr0x7c1 0

3.复位flash SFC控制器
#reset SFC
mww 0x4001003c 0x1fe3
mww 0x4001003c 0x1fff
sleep 10

4.使能写
#enable flash write
mww 0x40800200 0xC080EB1E

5.擦除芯片;FLASH写0生效,写1无效,如果要写的空间以后全为FF,则不需要擦除,如0x500000之后的空间
#erase whole flash chip
mww 0x40800308 0x6
mww 0x40800300 0x3 
mww 0x40800308 0xC7
mww 0x40800300 0x3
sleep 6000

6.加载镜像到 0x400000,注意路径使用双\\
#load image
load_image "load_dump\\Hi3861_demo_burn_demo_ok.bin" 0x400000 bin
如果是SWD,需要不一样的固件,如,Hi3861_SWD-demo_burn.bin

7.主要做烧写验证,非必要操作.需要指定dump下来的大小.大小与加载进去的一致,以bytes为单位,使用十进制.
#dump image to verify.
dump_image "load_dump\\Hi3861_demo_burn_demo_ok-w2d1.bin" 0x400000 766800
如果是SWD,需要不一样的固件 Hi3861_SWD-demo_burn.bin

8.非必要步骤,使用beyond compare或其它工具,对下载的镜像进行比对.完全一致,说明成功.

```



### 三.脱水版在HI3861L-RISCV-JTAG / HI3861L-RISCV-SWD-CORESIGHT的proc中有.

举例: 

解释: proc load_bin_erase 为TCL的编程,可以在telnet中使用 load_bin_erase,即会一次输入{}中的内容.

```tcl
proc load_bin_erase { } {
reset halt

# Close I/D Cache
reg csr0x7c0 0
reg csr0x7c1 0

# reset SFC
mww 0x4001003c 0x1fe3
mww 0x4001003c 0x1fff
sleep 10

# enable flash write
mww 0x40800200 0xC080EB1E

# erase whole flash chip
mww 0x40800308 0x6
mww 0x40800300 0x3 
mww 0x40800308 0xC7
mww 0x40800300 0x3
sleep 6000

load_image "load_dump\\Hi3861_SWD-demo_burn.bin" 0x400000 bin
dump_image "load_dump\\Hi3861_SWD-demo_burn-w1d1.bin" 0x400000 631472
}

```

### 四.FAQ
#### 请务必务必务必,检测线材的紧密程度!!!

#### 1. 烧写固件失败

使用HiBurn串口烧写的时候,需要把FT2232D卸下来,并且调整拨码开关

拨码开关调节,详情咨询 杨玉生 84163305

有关管脚复用,详情咨询 贾雪奎 wx638039

#### 2.JTAG/SWD连不上

查看FLASH是否有固件
连接串口,使用串口工具连接(如drivers\SSCOM_v5.13.1.7z),按复位按键,看是否有相关字样,显示已经有固件.
没有固件,无法使能JTAG连接(安全设计),具体咨询 王俊 00291248

#### 3.连不上,查看日志有Unsupported DTM version: 15字样

​	Debug: 251 47 arm_dap.c:105 dap_init_all(): Initializing all DAPs ...
​	Debug: 252 47 openocd.c:159 handle_init_command(): Examining targets...
​	Debug: 253 47 target.c:1579 target_call_event_callbacks(): target event 17 (examine-start)
​	Debug: 254 47 riscv.c:781 riscv_examine(): riscv_examine()
​	Debug: 255 47 riscv.c:227 dtmcontrol_scan(): DTMCONTROL: 0x0 -> 0xffffffff
​	Debug: 256 47 riscv.c:791 riscv_examine(): dtmcontrol=0xffffffff
​	Debug: 257 47 riscv.c:793 riscv_examine():   version=0xf
​	Error: 258 47 riscv.c:247 get_target_type(): Unsupported DTM version: 15
​	Debug: 259 47 openocd.c:161 handle_init_command(): target examination failed
​	一般是板子之前错误操作,

1)连着FT2232D重启HI3861

2)断开FT2232D type-c数据线重连(保持HI3861连接+通电)

还是不行,使用HiBurn串口重烧固件

#### 4.日志报错 JTAG-TO-SWD, Can't attach dap via JTAG/SWD.

1)固件不是SWD模式,
2)FT2232D小板没有切换到SWD模式(对应跳线帽配置,咨询贾雪奎 wx638039)
3)驱动没装好
反正就是没连上,没检测到SWD

#### 5.控制台打印SWD DPIDR字样

FT2232D硬件问题,尝试更换FT2232D硬件, 咨询 贾雪奎 wx638039



#### 6.日志中LIBUSB_ERROR_NOT_FOUND

Debug: 211 36 ftdi.c:738 ftdi_initialize(): ftdi interface using shortest path jtag state transitions
Error: 212 216 mpsse.c:195 open_matching_device(): libusb_open() failed with LIBUSB_ERROR_NOT_FOUND
Error: 213 216 mpsse.c:222 open_matching_device(): no device found

没有替换FT2232D驱动为winusb,可能要多次替换,不明原因,也许是华为端口控制问题.重新替换即可.



#### 7.telnet中显示 failed read at 0x16, status=2

基本已经挂掉,如果擦除的是0x400000,即已经擦除全器件,需要重新使用HiBurn烧写固件.



最后编辑 2020年8月18日

