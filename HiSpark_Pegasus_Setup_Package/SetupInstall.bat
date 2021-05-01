@echo off
::以管理员身份运行
@echo off
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
goto UACPrompt
) else ( goto gotAdmin )
:UACPrompt
echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
"%temp%\getadmin.vbs"
exit /B
:gotAdmin
if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
::UTF-8格式
chcp 65001
::解除电脑运行批处理
set pp=HKCU\Software\Policies\Microsoft\Windows\System
reg add HKCU\Software\Policies\Microsoft\Windows\System /v DisableCmd /t REG_DWORD /d 0 /f
::隐藏命令窗口
::if "%1" == "h" goto begin
::mshta vbscript:createobject("wscript.shell").run("""%~nx0"" h",0)(window.close)&&exit
:::begin
REM
::获取脚本所在目录
cd /d %~dp0
::安装LiteOS Studio
echo 正在开始安装LiteOS Studio...，请等待安装完成...
echo 在弹出对话框时，用户需自行点击进行程序安装...
echo 在用户手动安装LiteOS Studio结束后，请勿做其他操作，下一个程序将继续安装...
::set software=HUAWEI-LiteOS-Studio-Setup-x64-1.45.7.exe
::for /f "tokens=*" %%i in ('dir /a/b/s/on "%cd%\*%software%"') do (set setup="%%i")
::%setup%
start /wait software\HUAWEI-LiteOS-Studio-Setup-x64-1.45.7.exe  
::安装python-3.7.6-amd64.exe
echo 正在开始安装python-3.7.6-amd64.exe，请等待安装完成
::set software=python-3.7.6-amd64.exe
::for /f "tokens=*" %%i in ('dir /a/b/s/on "%cd%\*%software%"') do (set setup="%%i")
::%setup%
start /wait software\python-3.7.6-amd64.exe  

::安装git,从远程Gitee仓下载时需要
start /wait software\Git-2.31.1-64-bit.exe
::安装Ch340驱动
echo 在安装驱动前请先将Hi3861主板上电,否则Ch340板载驱动将安装失败...
echo 用户也可双击“CH341SER.EXE”自行安装...
::set software=CH341SER.EXE
::for /f "tokens=*" %%i in ('dir /a/b/s/on "%cd%\*%software%"') do (set setup="%%i")
::%setup%
start /wait software\CH341SER.EXE 

::创建pip文件夹及配置pip.ini内容
echo Config the pip,Please Waiting...
cd /d %UserProfile%
mkdir pip 
cd /d pip
echo [global]>pip.ini
echo	index-url = https://repo.huaweicloud.com/repository/pypi/simple/>>pip.ini
echo	rusted-host = repo.huaweicloud.com>>pip.ini
echo	timeout = 120>>pip.ini
::安装Scons
echo Installing Scons,Please Waiting...
pip install pycryptodome
pip install ecdsa
pip install pywin32
pip install scons
::安装riscv32-unknown-elf
set FileName=hcc_riscv32_win
echo Searching... 
echo Please Waiting...
for %%a in (C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
  if exist %%a:\ (
    for /f "delims=" %%b in ('dir /ad/b/s %%a:"*%FileName%*"') do (
		echo %%b
		cd /d %UserProfile%
		echo d|xcopy "%%b" ".huawei-liteos-studio\tools\hi3861\hcc_riscv32_win"  /e /y
    )
  )
)
::安装openocd
set Name=openocd
echo Searching... 
echo Please Waiting...
for %%a in (C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
  if exist %%a:\ (
    for /f "delims=" %%b in ('dir /ad/b/s %%a:"*%Name%*"') do (
		echo %%b
		cd /d %UserProfile%
		echo d|xcopy "%%b" ".huawei-liteos-studio\tools\hi3861\openocd"  /e /y
    )
  )
)
::替换jlinkBurner.txt
echo config jlinkBurner.txt, Please Waiting...
cd /d %UserProfile%
cd /d .huawei-liteos-studio\tools\hi3861\hiburn
echo  r>jlinkBurner.txt
echo  h>>jlinkBurner.txt
echo  writeCSR 0x7C0 0>>jlinkBurner.txt
echo  writeCSR 0x7C1 0>>jlinkBurner.txt

echo  w4 0x4001003c 0x1fe3>>jlinkBurner.txt
echo  w4 0x4001003c 0x1fff>>jlinkBurner.txt
echo  sleep 10>>jlinkBurner.txt

echo  w4 0x40800200 0xC080EB1E>>jlinkBurner.txt

echo  w4 0x40800308,0x6>>jlinkBurner.txt
echo  w4 0x40800300,0x3>>jlinkBurner.txt
echo  w4 0x40800308,0xC7>>jlinkBurner.txt
echo  w4 0x40800300,0x3>>jlinkBurner.txt
echo  sleep 5210>>jlinkBurner.txt
echo  mem32 400000 256>>jlinkBurner.txt

pause