
xmake clean -a
set VM_64bit=1
set LUAT_USE_GUI=y
xmake f -a x86 -y
xmake -w
