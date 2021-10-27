##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Release
ProjectName            :=uemg_monitor_sdl
ConfigurationName      :=Release
WorkspacePath          :=/home/the_3d6/robotics/nrf5
ProjectPath            :=/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=the_3d6
Date                   :=26/10/21
CodeLitePath           :=/home/the_3d6/.codelite
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)NDEBUG 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="uemg_monitor_sdl.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  $(shell pkg-config --libs gtk+-2.0)
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)SDL $(LibrarySwitch)SDL_ttf $(LibrarySwitch)X11 $(LibrarySwitch)png $(LibrarySwitch)Xtst 
ArLibs                 :=  "SDL" "SDL_ttf" "X11" "png" "Xtst" 
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -O3 -Wall $(Preprocessors)
CFLAGS   :=  -O3 -Wall $(shell pkg-config --cflags gtk+-2.0) $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) $(IntermediateDirectory)/drawing.c$(ObjectSuffix) $(IntermediateDirectory)/simplechart.c$(ObjectSuffix) $(IntermediateDirectory)/device_functions.c$(ObjectSuffix) $(IntermediateDirectory)/fft.c$(ObjectSuffix) $(IntermediateDirectory)/sft.c$(ObjectSuffix) $(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) $(IntermediateDirectory)/nn_mlp.c$(ObjectSuffix) $(IntermediateDirectory)/nn_dataset.c$(ObjectSuffix) \
	$(IntermediateDirectory)/save_processing.c$(ObjectSuffix) $(IntermediateDirectory)/nn_adaptive_map.c$(ObjectSuffix) $(IntermediateDirectory)/quat_math.c$(ObjectSuffix) $(IntermediateDirectory)/keyb_mouse_emu.c$(ObjectSuffix) $(IntermediateDirectory)/pca_processor.c$(ObjectSuffix) $(IntermediateDirectory)/qr_factorize.c$(ObjectSuffix) $(IntermediateDirectory)/win_main.c$(ObjectSuffix) $(IntermediateDirectory)/data_processing.c$(ObjectSuffix) $(IntermediateDirectory)/image_loader.c$(ObjectSuffix) $(IntermediateDirectory)/processing_kmeans.c$(ObjectSuffix) \
	



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Release || $(MakeDirCommand) ./Release


$(IntermediateDirectory)/.d:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.c$(ObjectSuffix): main.c $(IntermediateDirectory)/main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.c$(DependSuffix): main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/main.c$(DependSuffix) -MM main.c

$(IntermediateDirectory)/main.c$(PreprocessSuffix): main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.c$(PreprocessSuffix) main.c

$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix): serial_functions.c $(IntermediateDirectory)/serial_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/serial_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/serial_functions.c$(DependSuffix): serial_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/serial_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/serial_functions.c$(DependSuffix) -MM serial_functions.c

$(IntermediateDirectory)/serial_functions.c$(PreprocessSuffix): serial_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/serial_functions.c$(PreprocessSuffix) serial_functions.c

$(IntermediateDirectory)/drawing.c$(ObjectSuffix): drawing.c $(IntermediateDirectory)/drawing.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/drawing.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drawing.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drawing.c$(DependSuffix): drawing.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/drawing.c$(ObjectSuffix) -MF$(IntermediateDirectory)/drawing.c$(DependSuffix) -MM drawing.c

$(IntermediateDirectory)/drawing.c$(PreprocessSuffix): drawing.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drawing.c$(PreprocessSuffix) drawing.c

$(IntermediateDirectory)/simplechart.c$(ObjectSuffix): simplechart.c $(IntermediateDirectory)/simplechart.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/simplechart.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/simplechart.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/simplechart.c$(DependSuffix): simplechart.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/simplechart.c$(ObjectSuffix) -MF$(IntermediateDirectory)/simplechart.c$(DependSuffix) -MM simplechart.c

$(IntermediateDirectory)/simplechart.c$(PreprocessSuffix): simplechart.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/simplechart.c$(PreprocessSuffix) simplechart.c

$(IntermediateDirectory)/device_functions.c$(ObjectSuffix): device_functions.c $(IntermediateDirectory)/device_functions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/device_functions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/device_functions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/device_functions.c$(DependSuffix): device_functions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/device_functions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/device_functions.c$(DependSuffix) -MM device_functions.c

$(IntermediateDirectory)/device_functions.c$(PreprocessSuffix): device_functions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/device_functions.c$(PreprocessSuffix) device_functions.c

$(IntermediateDirectory)/fft.c$(ObjectSuffix): fft.c $(IntermediateDirectory)/fft.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/fft.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/fft.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/fft.c$(DependSuffix): fft.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/fft.c$(ObjectSuffix) -MF$(IntermediateDirectory)/fft.c$(DependSuffix) -MM fft.c

$(IntermediateDirectory)/fft.c$(PreprocessSuffix): fft.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/fft.c$(PreprocessSuffix) fft.c

$(IntermediateDirectory)/sft.c$(ObjectSuffix): sft.c $(IntermediateDirectory)/sft.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/sft.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sft.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sft.c$(DependSuffix): sft.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sft.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sft.c$(DependSuffix) -MM sft.c

$(IntermediateDirectory)/sft.c$(PreprocessSuffix): sft.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sft.c$(PreprocessSuffix) sft.c

$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix): spectrogram.c $(IntermediateDirectory)/spectrogram.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/spectrogram.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/spectrogram.c$(DependSuffix): spectrogram.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/spectrogram.c$(ObjectSuffix) -MF$(IntermediateDirectory)/spectrogram.c$(DependSuffix) -MM spectrogram.c

$(IntermediateDirectory)/spectrogram.c$(PreprocessSuffix): spectrogram.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/spectrogram.c$(PreprocessSuffix) spectrogram.c

$(IntermediateDirectory)/nn_mlp.c$(ObjectSuffix): nn_mlp.c $(IntermediateDirectory)/nn_mlp.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/nn_mlp.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/nn_mlp.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/nn_mlp.c$(DependSuffix): nn_mlp.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/nn_mlp.c$(ObjectSuffix) -MF$(IntermediateDirectory)/nn_mlp.c$(DependSuffix) -MM nn_mlp.c

$(IntermediateDirectory)/nn_mlp.c$(PreprocessSuffix): nn_mlp.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/nn_mlp.c$(PreprocessSuffix) nn_mlp.c

$(IntermediateDirectory)/nn_dataset.c$(ObjectSuffix): nn_dataset.c $(IntermediateDirectory)/nn_dataset.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/nn_dataset.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/nn_dataset.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/nn_dataset.c$(DependSuffix): nn_dataset.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/nn_dataset.c$(ObjectSuffix) -MF$(IntermediateDirectory)/nn_dataset.c$(DependSuffix) -MM nn_dataset.c

$(IntermediateDirectory)/nn_dataset.c$(PreprocessSuffix): nn_dataset.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/nn_dataset.c$(PreprocessSuffix) nn_dataset.c

$(IntermediateDirectory)/save_processing.c$(ObjectSuffix): save_processing.c $(IntermediateDirectory)/save_processing.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/save_processing.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/save_processing.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/save_processing.c$(DependSuffix): save_processing.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/save_processing.c$(ObjectSuffix) -MF$(IntermediateDirectory)/save_processing.c$(DependSuffix) -MM save_processing.c

$(IntermediateDirectory)/save_processing.c$(PreprocessSuffix): save_processing.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/save_processing.c$(PreprocessSuffix) save_processing.c

$(IntermediateDirectory)/nn_adaptive_map.c$(ObjectSuffix): nn_adaptive_map.c $(IntermediateDirectory)/nn_adaptive_map.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/nn_adaptive_map.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/nn_adaptive_map.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/nn_adaptive_map.c$(DependSuffix): nn_adaptive_map.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/nn_adaptive_map.c$(ObjectSuffix) -MF$(IntermediateDirectory)/nn_adaptive_map.c$(DependSuffix) -MM nn_adaptive_map.c

$(IntermediateDirectory)/nn_adaptive_map.c$(PreprocessSuffix): nn_adaptive_map.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/nn_adaptive_map.c$(PreprocessSuffix) nn_adaptive_map.c

$(IntermediateDirectory)/quat_math.c$(ObjectSuffix): quat_math.c $(IntermediateDirectory)/quat_math.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/quat_math.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/quat_math.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/quat_math.c$(DependSuffix): quat_math.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/quat_math.c$(ObjectSuffix) -MF$(IntermediateDirectory)/quat_math.c$(DependSuffix) -MM quat_math.c

$(IntermediateDirectory)/quat_math.c$(PreprocessSuffix): quat_math.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/quat_math.c$(PreprocessSuffix) quat_math.c

$(IntermediateDirectory)/keyb_mouse_emu.c$(ObjectSuffix): keyb_mouse_emu.c $(IntermediateDirectory)/keyb_mouse_emu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/keyb_mouse_emu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/keyb_mouse_emu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/keyb_mouse_emu.c$(DependSuffix): keyb_mouse_emu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/keyb_mouse_emu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/keyb_mouse_emu.c$(DependSuffix) -MM keyb_mouse_emu.c

$(IntermediateDirectory)/keyb_mouse_emu.c$(PreprocessSuffix): keyb_mouse_emu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/keyb_mouse_emu.c$(PreprocessSuffix) keyb_mouse_emu.c

$(IntermediateDirectory)/pca_processor.c$(ObjectSuffix): pca_processor.c $(IntermediateDirectory)/pca_processor.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/pca_processor.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/pca_processor.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/pca_processor.c$(DependSuffix): pca_processor.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/pca_processor.c$(ObjectSuffix) -MF$(IntermediateDirectory)/pca_processor.c$(DependSuffix) -MM pca_processor.c

$(IntermediateDirectory)/pca_processor.c$(PreprocessSuffix): pca_processor.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/pca_processor.c$(PreprocessSuffix) pca_processor.c

$(IntermediateDirectory)/qr_factorize.c$(ObjectSuffix): qr_factorize.c $(IntermediateDirectory)/qr_factorize.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/qr_factorize.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/qr_factorize.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/qr_factorize.c$(DependSuffix): qr_factorize.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/qr_factorize.c$(ObjectSuffix) -MF$(IntermediateDirectory)/qr_factorize.c$(DependSuffix) -MM qr_factorize.c

$(IntermediateDirectory)/qr_factorize.c$(PreprocessSuffix): qr_factorize.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/qr_factorize.c$(PreprocessSuffix) qr_factorize.c

$(IntermediateDirectory)/win_main.c$(ObjectSuffix): win_main.c $(IntermediateDirectory)/win_main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/win_main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/win_main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/win_main.c$(DependSuffix): win_main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/win_main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/win_main.c$(DependSuffix) -MM win_main.c

$(IntermediateDirectory)/win_main.c$(PreprocessSuffix): win_main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/win_main.c$(PreprocessSuffix) win_main.c

$(IntermediateDirectory)/data_processing.c$(ObjectSuffix): data_processing.c $(IntermediateDirectory)/data_processing.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/data_processing.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/data_processing.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/data_processing.c$(DependSuffix): data_processing.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/data_processing.c$(ObjectSuffix) -MF$(IntermediateDirectory)/data_processing.c$(DependSuffix) -MM data_processing.c

$(IntermediateDirectory)/data_processing.c$(PreprocessSuffix): data_processing.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/data_processing.c$(PreprocessSuffix) data_processing.c

$(IntermediateDirectory)/image_loader.c$(ObjectSuffix): image_loader.c $(IntermediateDirectory)/image_loader.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/image_loader.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/image_loader.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/image_loader.c$(DependSuffix): image_loader.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/image_loader.c$(ObjectSuffix) -MF$(IntermediateDirectory)/image_loader.c$(DependSuffix) -MM image_loader.c

$(IntermediateDirectory)/image_loader.c$(PreprocessSuffix): image_loader.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/image_loader.c$(PreprocessSuffix) image_loader.c

$(IntermediateDirectory)/processing_kmeans.c$(ObjectSuffix): processing_kmeans.c $(IntermediateDirectory)/processing_kmeans.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/the_3d6/robotics/nrf5/nrf52_proj/uemg_monitor_sdl/processing_kmeans.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/processing_kmeans.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/processing_kmeans.c$(DependSuffix): processing_kmeans.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/processing_kmeans.c$(ObjectSuffix) -MF$(IntermediateDirectory)/processing_kmeans.c$(DependSuffix) -MM processing_kmeans.c

$(IntermediateDirectory)/processing_kmeans.c$(PreprocessSuffix): processing_kmeans.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/processing_kmeans.c$(PreprocessSuffix) processing_kmeans.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


