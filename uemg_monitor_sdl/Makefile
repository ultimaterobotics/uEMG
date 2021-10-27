###
Name := uemg_win
CXX := x86_64-w64-mingw32-g++
CXXFLAGS := -O2 -Wall
Libs := 

$(Name): uemg_win
	$(CXX) -o build_win/$(Name).exe win_main.c quat_math.c device_serial.c $(Libs) $(CXXFLAGS)

clean: 
	rm $(Name)


