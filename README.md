# gstreamer-demo
## 特点
### 1.此例子是gstreamer动态pipe
### 2.动态保存MKV视频文件
### 3.防止进度条丢失
### 4.测试环境ubuntu18.04

## 安装 gst-plugins-good-1.14
### #gst-plugins-good-1.14包中含有 matroskamux 源码可用来调试


### #sudo apt-get install meson
### #cd gst-plugins-good-1.14/
### #mkdir build/ && meson build && ninja -C build/
### #meson build && ninja -C build/


## 编译 dynamicRecordMatroska.c
### #gcc dynamicRecordMatroska.c -o dynamicRecordMatroska  `pkg-config --cflags --libs gstreamer-1.0`
### #./dynamicRecordMatroska
### 执行ctrl+c 可进行动态存储mkv视频；单数开始存储，双数停止存储。
### 此例子可有效防止进度条丢失。
### 由于此例子是利用了ctrl+c来控制record开始停止，需要killall dynamicRecordMatroska 来停止程序