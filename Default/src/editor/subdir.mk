################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/editor/gimpnodeeditor.c \
../src/editor/gimpnodeitem.c \
../src/editor/gimpnodepad.c \
../src/editor/gimpnodeview.c 

OBJS += \
./src/editor/gimpnodeeditor.o \
./src/editor/gimpnodeitem.o \
./src/editor/gimpnodepad.o \
./src/editor/gimpnodeview.o 

C_DEPS += \
./src/editor/gimpnodeeditor.d \
./src/editor/gimpnodeitem.d \
./src/editor/gimpnodepad.d \
./src/editor/gimpnodeview.d 


# Each subdirectory must supply rules for building sources it contributes
src/editor/%.o: ../src/editor/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/gtk-2.0 -I/opt/babl/include/babl-0.1 -I/usr/include/libpng12 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/gio-unix-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/pango-1.0 -I/opt/gegl-03/include/gegl-0.3 -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


