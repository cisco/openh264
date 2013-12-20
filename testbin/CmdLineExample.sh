chmod 777 ../bin/linux/welsenc.exe
../bin/linux/welsenc.exe welsenc_vd_1d.cfg
../bin/linux/welsenc.exe welsenc_vd_rc.cfg

../bin/linux/welsdec.exe test_vd_1d.264 test_vd_1d.yuv
../bin/linux/welsdec.exe test_vd_rc.264 test_vd_rc.yuv
