# The executable file name.
# It must be specified.
# PROGRAM   := a.out    # the executable name
PROGRAM   := connector

#source directories
APPSRC := listlib 
# The directories in which source files reside.
# At least one path should be specified.
# SRCDIRS   := .    # current directory
SRCDIRS   :=. $(APPSRC)  

# The source file types (headers excluded).
# At least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
# SRCEXTS   := .c      # C program
# SRCEXTS   := .cpp    # C++ program
# SRCEXTS   := .c .cpp # C/C++ program
SRCEXTS   :=.c 

# The flags used by the cpp (man cpp for more).
# CPPFLAGS := -Wall -Werror # show all warnings and take them as errors
CPPFLAGS := 

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
CFLAGS    := -g
CFLAGS    +=  -Ilistlib -I.

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
CXXFLAGS :=
CXXFLAGS += 

# The library and the link options ( C and C++ common). Need libuuid-deve,openldap-devel,curl-devel
LDFLAGS   := -lz -lm -luuid -lpthread
LDFLAGS    += 

## Implict Section: change the following only when necessary.
##=============================================================================
# The C program compiler. Uncomment it to specify yours explicitly.
CC      = gcc 

# The C++ program compiler. Uncomment it to specify yours explicitly.
CXX     = g++ 

# Uncomment the 2 lines to compile C programs as C++ ones.
#CC      = $(CXX)
#CFLAGS = $(CXXFLAGS) 

# The command used to delete file.
RM    = rm -f 

## Stable Section: usually no need to be changed. But you can add more.
##=============================================================================
SHELL   = /bin/sh
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
OBJS    = $(foreach x,$(SRCEXTS), \
      $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))
DEPS    = $(patsubst %.o,%.d,$(OBJS)) 

.PHONY : all objs clean cleanall rebuild 

all : $(PROGRAM) 
	
# Rules for creating the dependency files (.d).
# 使用GCC生成依赖关系，这里需要做些处理，共有三步:
# A：$(CC) -MM  $(CFLAGS) $< > $@.$$$$ 为每个xxx.c文件生成依赖文件 xxx.d.$$$$，‘$’为随机数字
# B：GCC生成的依赖关系中会去掉.o文件的文件目录，类似于： xxx.o: src/xxx.c include/xxx.h
#    使用sed工具将其替换为 src/xxx.o src/xxx.d: src/xxx.c include/xxx.h，让xxx.d也一样依赖
#    于源文件，这样当.c源文件或着头文件有任何更新时，相应的.d文件也会被更新。注意这里sed的用
#    法和命令行的不太一样，分隔符必须是','而不是'/'，并且-i参数也不能用了，所以只能使用重定向
#    sed 's,.*\.o[: ]*\(.*\)\.c,\1.o $@:\ \1.c\ ,g' < $@.$$$$ > $@
# C：rm -f $@.$$$$ 这个简单，把作为临时文件的 xxx.d.$$$$ 删除
# 注意：
#    这三条命令是用连行符连起来的，否则 $@.$$$$ 文件可能就没法使用了，还有每行最后的';'也不能少
#---------------------------------------------------

%.d : %.c
	@$(CC) -MM  $(CFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[: ]*\(.*\)\.c,\1.o $@:\ \1.c\ ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$ 

# Rules for producing the objects.
#---------------------------------------------------
objs : $(OBJS) 

%.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@ 

# Rules for producing the executable.
#----------------------------------------------
$(PROGRAM) : $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS) 

#将依赖关系包含到Makefile中来
-include $(DEPS) 

rebuild: clean all 

clean :
	@$(RM) $(OBJS) $(DEPS) 

cleanall: clean
	@$(RM) $(PROGRAM) 

### End of the Makefile ## Suggestions are welcome ## All rights reserved ###
###############################################################################
