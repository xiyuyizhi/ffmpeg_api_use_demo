#!/bin/sh

path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else

  if [ ! -d "../FFmpeg" ];then
    echo "no exist FFmpeg,clone from github"
    cd ..
    git clone https://github.com/FFmpeg/FFmpeg.git
    cd FFmpeg
    git checkout -b 4.2 origin/release/4.2
    cd ..
  fi

  cd ffmpeg_api_use_demo

  sudo cp scripts/configure_with_debug.sh ../FFmpeg/configure_with_debug.sh

  sudo rm -rf libs2

  cd ../FFmpeg
  sudo make clean
  echo "configure in process..."
  sudo sh configure_with_debug.sh
  sudo make -j8
  sudo make install
  echo "libx comile finish"

fi


